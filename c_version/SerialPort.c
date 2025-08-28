#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "SerialPort.h"
#include "CmdFrm.h"
#include "FrmBuf.h"



static HANDLE m_portHandle = 0;                                                                         // 串口句柄
static HANDLE m_workingThread = 0;                                                                      // 工作线程句柄
static DWORD  m_workingThreadId = 0;                                                                    // 工作线程ID   
static S32    m_timeoutMilliSeconds = 0;                                                                // 读写超时时间，单位毫秒
static S32  (*m_OnDataReceivedHandler)(const U8* pData, S32 dataLength) = NULL;                         // 数据接收回调函数
static void (*m_OnDataSentHandler)(void) = NULL;                                                        // 数据发送完成回调函数
static CRITICAL_SECTION m_criticalSectionRead;                                                          // 读操作临界区
static CRITICAL_SECTION m_criticalSectionWrite;                                                         // 写操作临界区 

DWORD WINAPI    SerialPort_WaitForData( LPVOID lpParam );                                               // 等待数据线程函数 
S32             SerialPort__WriteBuffer(const U8* pData, S32 dataLength);                               // 内部写数据函数
S32             SerialPort__ReadBuffer(U8* pData, S32 dataLength, S32 timeOutMS);                       // 内部读数据函数

#define SERIALPORT_INTERNAL_TIMEOUT 1                                                                   // 内部读写超时时间，单位毫秒

/**
 * @fn void SerialPort_Initialize(void)
 * @brief 初始化串口模块内部状态和资源
 *
 * 该函数在使用串口前调用，用于初始化全局变量和同步对象。
 * 包括：
 * 1. 串口句柄置空
 * 2. 数据接收与发送回调函数指针置空
 * 3. 超时时间清零
 * 4. 工作线程句柄与ID清零
 * 5. 初始化读写临界区，用于多线程访问串口的同步
 *
 * @note 必须在首次使用串口操作前调用一次。
 *       对应的资源释放由 SerialPort_Uninitialize() 完成。
 */

void SerialPort_Initialize(void)
{
	m_portHandle = NULL;
	m_OnDataReceivedHandler = NULL;	
    m_OnDataSentHandler = NULL;	
    m_timeoutMilliSeconds = 0;
    m_workingThread = NULL;
    m_workingThreadId = 0;
    InitializeCriticalSection(&m_criticalSectionRead);
    InitializeCriticalSection(&m_criticalSectionWrite);
}


/**
 * @fn void SerialPort_Uninitialize(void)
 * @brief 释放串口模块占用的资源
 *
 * 该函数用于关闭已打开的串口并释放初始化时创建的同步对象（临界区）。
 * 步骤包括：
 * 1. 如果串口已打开，调用 SerialPort_Close() 关闭串口句柄。
 * 2. 删除读写临界区，释放系统资源。
 *
 * @note 在程序退出或不再使用串口时调用，确保资源正确释放。
 */
void SerialPort_Uninitialize(void)
{
    if (m_portHandle)
    {
        SerialPort_Close();  
    }
    DeleteCriticalSection(&m_criticalSectionRead);
    DeleteCriticalSection(&m_criticalSectionWrite);
}

/**
 * @fn S32 SerialPort_GetMaxTimeout(void)
 * @brief 获取串口的最大超时时间
 *
 * 该函数返回串口模块当前设置的最大超时时间（毫秒）。
 * 超时时间用于串口读操作中，控制等待数据的最长时间。
 *
 * @return  当前串口模块的最大超时时间（毫秒）
 * @note    返回值为 SerialPort_Open 或 SerialPort_OpenAsync 设置的 timeoutMS。
 */
S32 SerialPort_GetMaxTimeout()
{
    return m_timeoutMilliSeconds;
}

/**
 * @fn BOOL SerialPort_OpenAsync(S32 comPortNumber, S32 baudRate,
 *                               void (*OnDataReceivedHandler)(const U8* pData, S32 dataLength),
 *                               void (*OnDataSentHandler)(void),
 *                               S32 timeoutMS)
 * @brief 打开串口并启动异步接收线程
 *
 * 该函数会打开指定的串口，并在串口打开成功后根据提供的回调函数启动异步线程，
 * 用于接收数据或在发送完成时通知调用者。
 *
 * @param comPortNumber             串口号，例如 COM1 -> 1
 * @param baudRate                  串口波特率，例如 9600, 115200
 * @param OnDataReceivedHandler     数据接收回调函数指针，当串口有数据到达时调用
 *                                  如果不需要接收回调，可以传 NULL
 * @param OnDataSentHandler         数据发送完成回调函数指针，当写入缓冲完成时调用
 *                                  如果不需要发送回调，可以传 NULL
 * @param timeoutMS                 串口读操作超时时间（毫秒），用于内部超时判断
 *
 * @return TRUE  打开串口成功，异步线程已启动（如果提供了回调）
 *         FALSE 打开串口失败
 *
 * @note 1. 回调函数中应避免阻塞操作，否则可能影响串口接收性能
 *       2. 如果 OnDataReceivedHandler 和 OnDataSentHandler 都为 NULL，
 *          则不会启动异步线程
 *       3. 调用 SerialPort_Close() 可以关闭串口并停止异步线程
 */
BOOL SerialPort_OpenAsync(S32 comPortNumber, S32 baudRate,                             
                     S32 (*OnDataReceivedHandler)(const U8* pData, S32 dataLength),
                     void (*OnDataSentHandler)(void),                         
                     S32 timeoutMS
                    )

{
    BOOL result = SerialPort_Open(comPortNumber, baudRate, timeoutMS);
    if (result)
    {
        m_OnDataReceivedHandler = OnDataReceivedHandler;
        m_OnDataSentHandler     = OnDataSentHandler;
        if (m_OnDataSentHandler || m_OnDataReceivedHandler)
        {
            m_workingThread = CreateThread(NULL, 0, SerialPort_WaitForData, NULL, 0, &m_workingThreadId);
        }
    }
    return result;
}

/**
 * @fn BOOL SerialPort_Open(S32 comPortNumber, S32 baudRate, S32 timeoutMS)
 * @brief 打开串口并进行基本配置
 *
 * 该函数用于打开指定 COM 端口，并设置波特率、数据位、校验位、停止位以及读超时。
 * 成功打开后，将返回一个有效的句柄用于后续读写操作。
 *
 * @param comPortNumber         串口号，例如 COM1 对应 1
 * @param baudRate              波特率，例如 9600, 115200
 * @param timeoutMS             串口读操作超时时间（毫秒），最大支持 15000 ms
 *
 * @return  TRUE 成功打开串口，FALSE 打开失败
 * @note    如果端口号无效、超时过大或 CreateFile 打开失败，会返回 FALSE
 */
BOOL SerialPort_Open(S32 comPortNumber, S32 baudRate, S32 timeoutMS)
{
	char portName[12];
	S32  portNameLength;
    DCB  portSettings;
    COMMTIMEOUTS portTimeOuts;

	if (comPortNumber<0) return FALSE;
	if (comPortNumber>255) return FALSE;
    if (timeoutMS>15000) return FALSE;

	portNameLength = sprintf(portName, "\\\\.\\COM%i", comPortNumber );

	
    // 打开串口
    m_portHandle = CreateFileA(
        portName,                           // 端口名
        GENERIC_READ | GENERIC_WRITE,       // 读写权限
        0,                                  // 不共享
        NULL,                               // 安全属性
        OPEN_EXISTING,                      // 打开已有端口
        0,                                  // 默认属性
        NULL                                // 模板句柄
    );

    if ((m_portHandle==0) || ((unsigned long)m_portHandle==0xffffffff))
    {
        m_portHandle = 0;
        return FALSE;
    }

    m_timeoutMilliSeconds = timeoutMS;
	
    // 初始化 DCB 结构体
	memset(&portSettings, 0, sizeof(portSettings));

	portSettings.BaudRate = baudRate;
    portSettings.ByteSize = 8;
    portSettings.Parity   = NOPARITY;
    portSettings.StopBits = ONESTOPBIT;

    // 应用串口设置
	if (!SetCommState(m_portHandle, &portSettings))
	{
		return FALSE;
	}
    
    // 设置串口超时
    memset(&portTimeOuts, 0, sizeof(portTimeOuts));
    portTimeOuts.ReadIntervalTimeout = SERIALPORT_INTERNAL_TIMEOUT;
    portTimeOuts.ReadTotalTimeoutMultiplier = 0;    
    portTimeOuts.ReadTotalTimeoutConstant = SERIALPORT_INTERNAL_TIMEOUT;    

    if (!SetCommTimeouts(m_portHandle, &portTimeOuts))
    {
        return FALSE;
    }
	return (m_portHandle!=0);
}


/**
 * @fn void SerialPort_Close(void)
 * @brief 关闭已打开的串口并释放资源
 *
 * 该函数用于关闭串口句柄，确保在关闭过程中不会有线程正在读写串口。
 * 使用临界区保护读写操作，避免在关闭时发生资源冲突。
 *
 * @param   无
 * @return  无
 * @note    调用该函数后，串口句柄 m_portHandle 被置为 NULL，工作线程相关句柄和 ID 也被清零。
 */
void SerialPort_Close()
{    
    EnterCriticalSection(&m_criticalSectionRead);       // 进入临界区，防止在读取缓冲区过程中关闭串口
    EnterCriticalSection(&m_criticalSectionWrite);      // 进入临界区，防止在写入缓冲区过程中关闭串口


    // 等待线程退出
    if (m_workingThread)
    { 
        WaitForSingleObject(m_workingThread, 2000);     // 等待线程结束（最多等待 2 秒）
        CloseHandle(m_workingThread);                   // 释放线程句柄
        m_workingThread = NULL;                         // 清空工作线程句柄
        m_workingThreadId = 0;                          // 清空工作线程 ID
    }
    // 如果串口句柄有效，则关闭
	if (m_portHandle)
	{
		CloseHandle(m_portHandle);			            // 调用 Windows API 关闭串口句柄
        m_portHandle = NULL;                            // 置空全局串口句柄
        Sleep(SERIALPORT_INTERNAL_TIMEOUT*4);           // 确保任何挂起操作完成
	}

    // 离开临界区，允许其他线程访问串口
    LeaveCriticalSection(&m_criticalSectionRead);
    LeaveCriticalSection(&m_criticalSectionWrite);
}


/**
 * @fn BOOL SerialPort_IsOpen(void)
 * @brief 检查串口是否已打开
 *
 * 该函数用于判断串口句柄是否有效，以确定串口当前是否处于打开状态。
 *
 * @param  无
 * @return BOOL
 *         - TRUE  : 串口已打开
 *         - FALSE : 串口未打开
 */
BOOL SerialPort_IsOpen()
{
    return (m_portHandle!=NULL);
}



/**
 * @fn S32 SerialPort__WriteBuffer(const U8* pData, S32 dataLength)
 * @brief 向串口发送指定长度的数据（低级写函数）
 *
 * 该函数直接向串口写入数据，不进行线程同步控制或回调处理，
 * 适合作为内部写函数，由上层封装函数（如 SerialPort_WriteBuffer）调用。
 *
 * @param pData       指向要发送的数据缓冲区
 * @param dataLength  数据长度（字节数）
 * @return S32
 *         - 发送成功的字节数
 *         - 0 表示串口未打开或写入失败
 */
S32 SerialPort__WriteBuffer(const U8* pData, S32 dataLength)
{
    DWORD bytesWritten = 0;

	if (m_portHandle==NULL)
	{
		return 0;
	}    

    // 调用 Win32 API WriteFile 向串口写数据
    if (!WriteFile(m_portHandle, pData, dataLength, &bytesWritten, NULL))
    {
        return 0;
    }    
	return (S32)bytesWritten;   // 返回实际写入的字节数
}


/**
 * @fn S32 SerialPort__ReadBuffer(U8* pData, S32 dataLength, S32 timeOutMS)
 * @brief 从串口读取指定长度的数据（低级读取函数）
 *
 * 该函数直接从串口读取数据，不进行线程同步控制或回调处理。
 * 支持超时机制，根据 timeOutMS 参数决定读取等待时间。
 *
 * @param pData       指向接收缓冲区
 * @param dataLength  期望读取的字节数
 * @param timeOutMS   最大等待时间（毫秒），小于0时使用默认超时时间
 * @return S32
 *         - 实际读取的字节数
 *         - 0 表示串口未打开或超时未读取到数据
 */
S32 SerialPort__ReadBuffer(U8* pData, S32 dataLength, S32 timeOutMS)
{
    DWORD bytesRead,bytesReadTotal;
    S32   timeOutCounter, bytesLeft;

	if (m_portHandle==NULL)
	{
		return 0;
	}
    if (timeOutMS<0)
    {
        timeOutMS = m_timeoutMilliSeconds;
    }
    
    bytesRead = 0;
    bytesReadTotal = 0;
    timeOutCounter = timeOutMS;
    bytesLeft = dataLength;

    // 超时时间非常短，直接睡眠等待一次读取
    if (timeOutMS<SERIALPORT_INTERNAL_TIMEOUT*2)
    {
        Sleep(timeOutMS);
        ReadFile(m_portHandle, pData, dataLength, &bytesRead, NULL);
        bytesReadTotal += bytesRead;
    } else {
        // 循环读取数据，直到超时或读取完成
        while(timeOutCounter>0)
        {            
            bytesRead = 0;
            // 从串口读取数据
            ReadFile(m_portHandle, pData+bytesReadTotal, dataLength, &bytesRead, NULL);
            if (bytesRead)
            {
                // 更新剩余读取长度和总读取字节数
                dataLength     -= bytesRead;
                bytesReadTotal += bytesRead;
            
                if (dataLength==0) break;           // 全部读取完成，退出循环
                timeOutCounter = timeOutMS;         // 重新设置超时计数
            } else {
                timeOutCounter -= SERIALPORT_INTERNAL_TIMEOUT;          // 减少等待计数
            }            

        }
    }
	return bytesReadTotal;          // 返回实际读取的字节数
}


/**
 * @fn S32 SerialPort_ReadBuffer(U8* pData, S32 dataLength, S32 timeOutMS)
 * @brief 从串口读取指定长度的数据（线程安全封装）
 *
 * 该函数是对 SerialPort__ReadBuffer 的封装，加入了临界区保护，
 * 以确保在多线程环境下读取操作的安全性，防止同时访问串口导致冲突。
 *
 * @param pData       指向接收缓冲区
 * @param dataLength  期望读取的字节数
 * @param timeOutMS   最大等待时间（毫秒），可小于0使用默认超时
 * @return S32
 *         - 实际读取的字节数
 *         - 0 表示串口未打开或超时未读取到数据
 */
S32 SerialPort_ReadBuffer(U8* pData, S32 dataLength, S32 timeOutMS)
{
    S32 result;
    EnterCriticalSection(&m_criticalSectionRead);                       // 进入读取临界区，保证同一时间只有一个线程访问串口读取
    result = SerialPort__ReadBuffer(pData, dataLength, timeOutMS);      // 调用低级读取函数，实际从串口获取数据
    LeaveCriticalSection(&m_criticalSectionRead);                       // 离开读取临界区，允许其他线程访问串口 
    return result;                                                      // 返回读取到的字节数
}

/**
 * @fn S32 SerialPort_WriteBuffer(const U8* pData, S32 dataLength)
 * @brief 向串口写入指定长度的数据（线程安全封装）
 *
 * 该函数是对 SerialPort__WriteBuffer 的封装，加入了临界区保护，
 * 以确保在多线程环境下写入操作的安全性，防止同时访问串口导致冲突。
 * 同时，如果注册了数据发送回调，会在写入完成后触发。
 *
 * @param pData       指向发送缓冲区
 * @param dataLength  要写入的字节数
 * @return S32
 *         - 实际写入的字节数
 *         - 0 表示串口未打开或写入失败
 */
S32 SerialPort_WriteBuffer(const U8* pData, S32 dataLength)
{
    S32 result;
    EnterCriticalSection(&m_criticalSectionWrite);                      // 进入写操作临界区，保证同一时间只有一个线程访问串口写入
    result = SerialPort__WriteBuffer(pData, dataLength);                // 调用低级写入函数，实际向串口发送数据
    LeaveCriticalSection(&m_criticalSectionWrite);                      // 离开写操作临界区，允许其他线程写入或关闭串口

    // 如果注册了数据发送完成回调，则调用回调函数
    if (m_OnDataSentHandler)
    {
        m_OnDataSentHandler();
    }
    return result;                                                      // 返回实际写入的字节数    
}

/**
 * @fn S32 SerialPort_WriteLine(char* pLine, BOOL addCRatEnd)
 * @brief 向串口发送一行字符串，可选择在末尾添加回车符
 *
 * 该函数在 SerialPort_WriteBuffer 基础上增加了：
 * 1. 字符串长度检查
 * 2. 可选自动添加回车符
 * 3. 线程安全写入保护
 * 4. 发送完成回调触发
 *
 * @param pLine         要发送的字符串
 * @param addCRatEnd    TRUE 表示在末尾添加回车符（0x0D），FALSE 不添加
 * @return S32
 *         - 实际写入的字节数
 *         - 0 表示串口未打开或输入无效
 */
S32 SerialPort_WriteLine(char* pLine, BOOL addCRatEnd)
{
    S32 lineLength, result;

	if (m_portHandle==NULL)
	{
		return 0;
	}
    if (pLine==NULL)
    {
        return 0;
    }
    lineLength = strlen(pLine);
    if (lineLength==0)
    {
        return 0;
    }

    EnterCriticalSection(&m_criticalSectionWrite);
    result = SerialPort__WriteBuffer((U8*)pLine, lineLength);
    if (addCRatEnd && pLine[lineLength-1]!=0x0D)
    {
        char cr = 13;
        result+=SerialPort__WriteBuffer((U8*)&cr, 1);
    }
    LeaveCriticalSection(&m_criticalSectionWrite);
    if (m_OnDataSentHandler)
    {
        m_OnDataSentHandler();
    }
    return result;
}



/**
 * @fn S32 SerialPort_ReadLine(char* pLine, S32 maxBufferSize, S32 timeOutMS)
 * @brief 从串口读取一行数据（最多 maxBufferSize-1 字节），并在末尾添加字符串结束符
 *
 * 该函数会在指定超时时间内读取串口缓冲区的数据：
 * 1. 线程安全读取
 * 2. 自动在读取数据末尾添加 '\0'，便于作为字符串处理
 *
 * @param pLine          存储读取数据的缓冲区
 * @param maxBufferSize  缓冲区大小
 * @param timeOutMS      超时时间（毫秒），如果为负数则使用默认超时
 * @return S32
 *         - 实际读取的字节数
 *         - 0 表示串口未打开或输入无效或无数据
 */
S32 SerialPort_ReadLine(char* pLine, S32 maxBufferSize, S32 timeOutMS)
{
    S32 result;
	if (m_portHandle==NULL)
	{
		return 0;
	}
    if (pLine==NULL)
    {
        return 0;
    }

    EnterCriticalSection(&m_criticalSectionRead);
    result = SerialPort__ReadBuffer((U8*)pLine, maxBufferSize-1, timeOutMS);	
    if (result<maxBufferSize)
    {
        pLine[result] = 0;
    }
    LeaveCriticalSection(&m_criticalSectionRead);
    return result;
}


/**
 * @fn DWORD WINAPI SerialPort_WaitForData(LPVOID lpParam)
 * @brief 串口异步接收线程函数，持续监听串口并回调处理数据
 *
 * 该线程函数在串口打开时运行：
 * 1. 循环读取串口数据
 * 2. 当有数据到达时调用回调函数 m_OnDataReceivedHandler
 * 3. 使用固定大小缓冲区 packet 存储临时数据
 *
 * @param lpParam 线程参数（未使用，可传 NULL）
 * @return DWORD 返回线程退出代码（总是 0）
 */
DWORD WINAPI SerialPort_WaitForData( LPVOID lpParam )
{
    static U8 packet[64];            // 静态缓冲区用于临时存放每次读取的数据
    S32 packetSize = sizeof(packet);            // 缓冲区大小
    S32 bytesRead = 0;                          // 实际读取字节数

    // 循环监听串口，只要串口处于打开状态
    while(SerialPort_IsOpen())
    {
        bytesRead = SerialPort_ReadBuffer(packet, packetSize, SERIALPORT_INTERNAL_TIMEOUT);    // 从串口读取数据，超时使用 SERIALPORT_INTERNAL_TIMEOUT
        if (bytesRead)
        {
            // 调用用户提供的回调函数处理数据
            if (m_OnDataReceivedHandler)
            {
                m_OnDataReceivedHandler(packet, bytesRead);
                //printf("Received %d bytes data.\n", bytesRead);
            }
        }
    }    
    return 0;
}





