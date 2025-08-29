#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "SerialPort.h"
#include "CmdFrm.h"
#include "FrmBuf.h"


DWORD WINAPI    SerialPort_WaitForData( LPVOID lpParam );                                               // 等待数据线程函数 
S32             SerialPort__WriteBuffer(SerialPort* sp, const U8* pData, S32 dataLength);               // 内部写数据函数
S32             SerialPort__ReadBuffer(SerialPort* sp, U8* pData, S32 dataLength, S32 timeOutMS);       // 内部读数据函数


#define SERIALPORT_INTERNAL_TIMEOUT 1                                                                   // 内部读写超时时间，单位毫秒

/**
 * @fn void SerialPort_Initialize(SerialPort* sp)
 * @brief 初始化串口实例
 *
 * 初始化给定的 SerialPort 实例，清空句柄和回调函数指针，
 * 并初始化读写临界区。每个串口实例必须在使用前调用。
 *
 * @param sp [in/out] 指向 SerialPort 实例的指针
 *
 * @note 对应的资源释放由 SerialPort_Uninitialize() 完成。
 */

void SerialPort_Initialize(SerialPort* sp)
{
    sp->portHandle = NULL;
    sp->onDataReceivedHandler = NULL;
    sp->onDataSentHandler = NULL;
    sp->timeoutMilliSeconds = 0;
    sp->workingThread = NULL;
    sp->workingThreadId = 0;
    InitializeCriticalSection(&sp->criticalSectionRead);
    InitializeCriticalSection(&sp->criticalSectionWrite);
}


/**
 * @fn void SerialPort_Uninitialize(SerialPort* sp)
 * @brief 释放串口实例资源
 *
 * 关闭串口（如果已打开），并删除临界区。
 * 调用后，该 SerialPort 实例不可再直接使用，需重新初始化。
 *
 * @param sp [in/out] 指向 SerialPort 实例的指针
 * @note 该函数会调用 SerialPort_Close() 以确保串口关闭。
 */
void SerialPort_Uninitialize(SerialPort* sp)
{
    SerialPort_Close(sp);
    sp->portHandle = NULL;
    sp->onDataReceivedHandler = NULL;
    sp->onDataSentHandler = NULL;
    sp->timeoutMilliSeconds = 0;
    sp->workingThread = NULL;
    sp->workingThreadId = 0; 
    DeleteCriticalSection(&sp->criticalSectionRead);
    DeleteCriticalSection(&sp->criticalSectionWrite);
}

/**
 * @fn S32 SerialPort_GetMaxTimeout(SerialPort* sp)
 * @brief 获取当前串口的最大超时时间
 *
 * 返回在 SerialPort_Open 或 SerialPort_OpenAsync 时设置的读超时时间。
 *
 * @param sp [in] 指向 SerialPort 实例的指针
 * @return 串口读超时（毫秒）
 * @note 该值仅供参考，实际读操作可指定不同的超时时间。
 */
S32 SerialPort_GetMaxTimeout(SerialPort* sp)
{
    return sp->timeoutMilliSeconds;
}

/**
 * @fn BOOL    SerialPort_OpenAsync(SerialPort* sp, 
 *                                  S32 comPort, 
 *                                  S32 baudRate, 
 *                                  SerialPort_OnDataReceived onRecv, 
 *                                  SerialPort_OnDataSent onSent, 
 *                                  S32 timeoutMS)
 * @brief 异步方式打开串口
 *
 * 打开指定的 COM 口，并启动一个后台线程持续监听串口数据。
 * 数据到达时通过回调通知用户，写入完成后也会触发发送回调。
 *
 * @param sp        [in/out] 串口实例
 * @param comPort   [in] COM 口号（如 1 = COM1）
 * @param baudRate  [in] 波特率（如 9600, 115200）
 * @param onRecv    [in] 数据接收回调函数指针，可为 NULL
 * @param onSent    [in] 数据发送回调函数指针，可为 NULL
 * @param timeoutMS [in] 默认读超时（ms）
 *
 * @retval TRUE  打开成功并启动线程
 * @retval FALSE 打开失败
 */
BOOL SerialPort_OpenAsync(SerialPort* sp,
                          S32 comPortNumber, 
                          S32 baudRate,                             
                          SerialPort_OnDataReceived onRecv,
                          SerialPort_OnDataSent onSent,                         
                          S32 timeoutMS)
{
    BOOL result = SerialPort_Open(sp, comPortNumber, baudRate, timeoutMS);
    if (result)
    {
        sp->onDataReceivedHandler = onRecv;
        sp->onDataSentHandler     = onSent;
        if (sp->onDataSentHandler || sp->onDataReceivedHandler)
        {
            sp->workingThread = CreateThread(NULL, 0, SerialPort_WaitForData, sp, 0, &sp->workingThreadId);
        }
    }
    return result;
}


/**
 * @fn BOOL SerialPort_Open(SerialPort* sp, S32 comPort, S32 baudRate, S32 timeoutMS)
 * @brief 同步方式打开串口
 *
 * 打开指定的 COM 口并完成波特率和超时设置。
 * 本函数不会启动后台监听线程。
 *
 * @param sp        [in/out] 串口实例
 * @param comPort   [in] COM 口号
 * @param baudRate  [in] 波特率
 * @param timeoutMS [in] 默认读超时（ms）
 *
 * @retval TRUE  打开成功
 * @retval FALSE 打开失败
 */
BOOL SerialPort_Open(SerialPort* sp, S32 comPortNumber, S32 baudRate, S32 timeoutMS)
{
    char portName[12];
    S32  portNameLength;
    DCB  portSettings;
    COMMTIMEOUTS portTimeOuts;

    if (comPortNumber < 0) return FALSE;
    if (comPortNumber > 255) return FALSE;
    if (timeoutMS > 15000) return FALSE;

    portNameLength = sprintf(portName, "\\\\.\\COM%i", comPortNumber);

    // 打开串口
    sp->portHandle = CreateFileA(
        portName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if ((sp->portHandle == 0) || ((unsigned long)sp->portHandle == 0xffffffff))
    {
        sp->portHandle = 0;
        return FALSE;
    }

    sp->timeoutMilliSeconds = timeoutMS;

    SetupComm(sp->portHandle, 4096, 4096); // 设置接收/发送缓冲区
    PurgeComm(sp->portHandle, PURGE_RXCLEAR | PURGE_TXCLEAR); // 清空缓冲区

    // 初始化 DCB
    memset(&portSettings, 0, sizeof(portSettings));
    portSettings.BaudRate = baudRate;
    portSettings.ByteSize = 8;
    portSettings.Parity   = NOPARITY;
    portSettings.StopBits = ONESTOPBIT;

    if (!SetCommState(sp->portHandle, &portSettings))
    {
        return FALSE;
    }

    memset(&portTimeOuts, 0, sizeof(portTimeOuts));
    portTimeOuts.ReadIntervalTimeout = SERIALPORT_INTERNAL_TIMEOUT;
    portTimeOuts.ReadTotalTimeoutMultiplier = 0;
    portTimeOuts.ReadTotalTimeoutConstant = SERIALPORT_INTERNAL_TIMEOUT;

    if (!SetCommTimeouts(sp->portHandle, &portTimeOuts))
    {
        return FALSE;
    }

    return (sp->portHandle != 0);
}

/**
 * @fn void SerialPort_Close(SerialPort* sp)
 * @brief 关闭串口
 *
 * 关闭 COM 口句柄，并确保后台监听线程安全退出。
 * 在关闭时使用临界区保护，防止并发读写冲突。
 *
 * @param sp [in/out] 串口实例
 */
void SerialPort_Close(SerialPort* sp)
{    
    EnterCriticalSection(&sp->criticalSectionRead);
    EnterCriticalSection(&sp->criticalSectionWrite);

    

    if (sp->portHandle)
    {
        CloseHandle(sp->portHandle);
        sp->portHandle = NULL;
        Sleep(SERIALPORT_INTERNAL_TIMEOUT*4);
    }

    LeaveCriticalSection(&sp->criticalSectionRead);
    LeaveCriticalSection(&sp->criticalSectionWrite);
    // 等待线程退出
    if (sp->workingThread)
    { 
        WaitForSingleObject(sp->workingThread, 2000);
        CloseHandle(sp->workingThread);
        sp->workingThread = NULL;
        sp->workingThreadId = 0;
    }

    
}


/**
 * @fn BOOL SerialPort_IsOpen(SerialPort* sp)
 * @brief 判断串口是否已打开
 *
 * 检查串口句柄是否有效。
 *
 * @param sp [in] 串口实例
 * @retval TRUE  已打开
 * @retval FALSE 未打开
 */
BOOL SerialPort_IsOpen(SerialPort* sp)
{
    return (sp->portHandle != NULL);
}


/**
 * @fn S32 SerialPort__WriteBuffer(SerialPort* sp, const U8* pData, S32 dataLength)
 * @brief 内部写函数（非线程安全）
 *
 * 直接调用 WriteFile 向串口写入数据。
 * 需在外部用临界区保护。
 *
 * @param sp         [in] 串口实例
 * @param pData      [in] 数据缓冲区
 * @param dataLength [in] 数据长度
 * @return 实际写入的字节数
 */
S32 SerialPort__WriteBuffer(SerialPort* sp, const U8* pData, S32 dataLength)
{
    DWORD bytesWritten = 0;
    if (sp->portHandle == NULL)
        return 0;

    if (!WriteFile(sp->portHandle, pData, dataLength, &bytesWritten, NULL))
        return 0;

    return (S32)bytesWritten;
}


/**
 * @fn S32 SerialPort__ReadBuffer(SerialPort* sp, U8* pData, S32 dataLength, S32 timeOutMS)
 * @brief 内部读函数（非线程安全）
 *
 * 直接调用 ReadFile 从串口读取数据，
 * 支持超时机制。
 *
 * @param sp         [in] 串口实例
 * @param pData      [out] 接收缓冲区
 * @param dataLength [in] 期望读取的字节数
 * @param timeOutMS  [in] 最大等待时间（ms）
 * @return  S32
 *          - 实际读取的字节数
 *          - 0 表示串口未打开或超时未读取到数据
 */
S32 SerialPort__ReadBuffer(SerialPort* sp, U8* pData, S32 dataLength, S32 timeOutMS)
{
    DWORD bytesRead, bytesReadTotal;
    S32   timeOutCounter;

    if (sp->portHandle == NULL)
        return 0;

    if (timeOutMS < 0)
        timeOutMS = sp->timeoutMilliSeconds;

    bytesRead = 0;
    bytesReadTotal = 0;
    timeOutCounter = timeOutMS;
    // 超时时间非常短，直接睡眠等待一次读取
    if (timeOutMS < SERIALPORT_INTERNAL_TIMEOUT*2)
    {
        Sleep(timeOutMS);
        ReadFile(sp->portHandle, pData, dataLength, &bytesRead, NULL);
        bytesReadTotal += bytesRead;
    } 
    else 
    {
        // 循环读取数据，直到超时或读取完成
        while (timeOutCounter > 0)
        {            
            bytesRead = 0;
            // 从串口读取数据
            ReadFile(sp->portHandle, pData+bytesReadTotal, dataLength, &bytesRead, NULL);
            if (bytesRead)
            {
                // 更新剩余读取长度和总读取字节数                                
                dataLength     -= bytesRead;
                bytesReadTotal += bytesRead;
                if (dataLength == 0) break;                             // 全部读取完成，退出循环
                timeOutCounter = timeOutMS;                             // 重新设置超时计数  
            } 
            else 
            {
                timeOutCounter -= SERIALPORT_INTERNAL_TIMEOUT;          // 减少等待计数
            }
        }
    }
    static U32 TotalCount = 0;
    if (bytesReadTotal > 0)
    {
        TotalCount += bytesReadTotal;
        printf("Info: Read %d bytes, Total %d bytes\r\n", bytesReadTotal, TotalCount);
    }
    return bytesReadTotal;                                              // 返回实际读取的字节数
}


/**
 * @fn S32 SerialPort_ReadBuffer(SerialPort* sp, U8* pData, S32 dataLength, S32 timeOutMS)
 * @brief 线程安全的串口读取
 *
 * 在 SerialPort__ReadBuffer 基础上加入临界区保护，
 * 保证在多线程环境下安全。
 *
 * @param sp         [in] 串口实例
 * @param pData      [out] 接收缓冲区
 * @param dataLength [in] 期望读取字节数
 * @param timeOutMS  [in] 最大等待时间（ms）
 * @return S32
 *         - 实际读取的字节数
 *         - 0 表示串口未打开或超时未读取到数据
 */

S32 SerialPort_ReadBuffer(SerialPort* sp, U8* pData, S32 dataLength, S32 timeOutMS)
{
    S32 result;
    EnterCriticalSection(&sp->criticalSectionRead);                     // 进入读操作临界区，保证同一时间只有一个线程访问串口读取
    result = SerialPort__ReadBuffer(sp, pData, dataLength, timeOutMS);  // 调用低级读取函数，实际从串口获取数据
    LeaveCriticalSection(&sp->criticalSectionRead);                     // 离开读操作临界区，允许其他线程访问串口
    return result;                                                      // 返回读取到的字节数
}

/**
 * @fn S32 SerialPort_WriteBuffer(SerialPort* sp, const U8* pData, S32 dataLength)
 * @brief 线程安全的串口写入
 *
 * 在 SerialPort__WriteBuffer 基础上加入临界区保护，
 * 并在写入完成后触发发送回调。
 *
 * @param sp         [in] 串口实例
 * @param pData      [in] 数据缓冲区
 * @param dataLength [in] 数据长度
 * @return S32
 *         - 实际写入的字节数   
 *         - 0 表示串口未打开或输入无效
 */

S32 SerialPort_WriteBuffer(SerialPort* sp, const U8* pData, S32 dataLength)
{
    S32 result;
    EnterCriticalSection(&sp->criticalSectionWrite);                    // 进入写操作临界区，保证同一时间只有一个线程访问串口写入
    result = SerialPort__WriteBuffer(sp, pData, dataLength);            // 调用低级写入函数，实际向串口发送数据
    LeaveCriticalSection(&sp->criticalSectionWrite);                    // 离开写操作临界区，允许其他线程写入或关闭串口
    // 如果注册了数据发送完成回调，则调用回调函数
    if (sp->onDataSentHandler)
    {
        sp->onDataSentHandler(pData, result);
    }
    return result;                  // 返回实际写入的字节数    
}
/**
 * @fn S32 SerialPort_WriteLine(SerialPort* sp, char* pLine, BOOL addCRatEnd)
 * @brief 写入一行字符串
 *
 * 向串口发送一行字符串，可选自动附加回车符 (CR)。
 * 线程安全。
 *
 * @param sp        [in] 串口实例
 * @param pLine     [in] 字符串
 * @param addCRatEnd [in] 是否在末尾添加 CR (0x0D)
 * @return S32
 *         - 实际写入的字节数
 *         - 0 表示串口未打开或输入无效
 */
S32 SerialPort_WriteLine(SerialPort* sp, char* pLine, BOOL addCRatEnd)
{
    S32 lineLength, result;

    if (sp->portHandle == NULL)
        return 0;
    if (pLine == NULL)
        return 0;

    lineLength = strlen(pLine);
    if (lineLength == 0)
        return 0;

    EnterCriticalSection(&sp->criticalSectionWrite);
    result = SerialPort__WriteBuffer(sp, (U8*)pLine, lineLength);
    if (addCRatEnd && pLine[lineLength-1] != 0x0D)
    {
        char cr = 13;
        result += SerialPort__WriteBuffer(sp, (U8*)&cr, 1);
    }
    LeaveCriticalSection(&sp->criticalSectionWrite);

    if (sp->onDataSentHandler)
    {
        sp->onDataSentHandler(pLine, result);
    }
        
    return result;
}



/**
 * @fn S32 SerialPort_ReadLine(SerialPort* sp, char* pLine, S32 maxBufferSize, S32 timeOutMS)
 * @brief 读取一行字符串
 *
 * 从串口读取数据，存入缓冲区并在末尾自动添加 '\0'。
 * 线程安全。
 *
 * @param sp            [in] 串口实例
 * @param pLine         [out] 缓冲区
 * @param maxBufferSize [in] 缓冲区大小
 * @param timeOutMS     [in] 超时时间（ms）
 * @return S32
 *         - 实际读取的字节数（不包括末尾的 '\0'）
 *         - 0 表示串口未打开或输入无效
 */
S32 SerialPort_ReadLine(SerialPort* sp, char* pLine, S32 maxBufferSize, S32 timeOutMS)
{
    S32 result;
    if (sp->portHandle == NULL)
        return 0;
    if (pLine == NULL)
        return 0;

    EnterCriticalSection(&sp->criticalSectionRead);
    result = SerialPort__ReadBuffer(sp, (U8*)pLine, maxBufferSize-1, timeOutMS);
    if (result < maxBufferSize)
    {
        pLine[result] = 0;
    }
    LeaveCriticalSection(&sp->criticalSectionRead);
    return result;
}



/**
 * @fn DWORD WINAPI SerialPort_WaitForData(LPVOID lpParam)
 * @brief 串口监听线程函数
 *
 * 后台线程循环读取串口数据，并在有数据时调用用户注册的
 * 数据接收回调函数。
 *
 * @param lpParam [in] 指向 SerialPort 实例的指针
 * @return 始终返回 0
 */

DWORD WINAPI SerialPort_WaitForData(LPVOID lpParam)
{
    SerialPort* sp = (SerialPort*)lpParam;              // 获取传入的串口实例指针
    U8 packet[64];                                      // 静态缓冲区用于临时存放每次读取的数据    
    S32 packetSize = sizeof(packet);                    // 缓冲区大小 
    S32 bytesRead = 0;                                  // 实际读取字节数

    // 循环监听串口，只要串口处于打开状态
    while (SerialPort_IsOpen(sp))
    {
        bytesRead = SerialPort_ReadBuffer(sp, packet, packetSize, SERIALPORT_INTERNAL_TIMEOUT);         // 从串口读取数据，超时使用 SERIALPORT_INTERNAL_TIMEOUT
        if (bytesRead)
        {
            if (sp->onDataReceivedHandler)
            {
                sp->onDataReceivedHandler(packet, bytesRead);
            }
        }
    }
    return 0;
}





