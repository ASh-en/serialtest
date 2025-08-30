#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "SerialPort.h"
#include "CmdFrm.h"
#include "FrmBuf.h"


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

void SerialPort_Initialize(SerialPort* sp, U32 recvBufSize, U32 sendBufSize) 
{
    sp->portHandle = NULL;
    sp->onDataReceivedHandler = NULL;
    sp->onDataSentHandler = NULL;
    sp->timeoutMilliSeconds = 0;
    sp->workingThread = NULL;
    sp->workingThreadId = 0;
    sp->isOpen = FALSE;
    sp->TotalCount = 0;
    sp->mRecvRngId = NULL;
    sp->mSendRngId = NULL;
    ZeroMemory(&sp->olRead, sizeof(OVERLAPPED));
    ZeroMemory(&sp->olWrite, sizeof(OVERLAPPED));
    if(recvBufSize != 0)
    {
        sp->mRecvRngId = Sr_CreateDynamic(recvBufSize);
    }
    if(sendBufSize != 0)
    {
        sp->mSendRngId = Sr_CreateDynamic(sendBufSize);
    }

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
    printf("Info: Total %d bytes\r\n", sp->TotalCount);
    sp->portHandle = NULL;
    sp->onDataReceivedHandler = NULL;
    sp->onDataSentHandler = NULL;
    sp->timeoutMilliSeconds = 0;
    sp->workingThread = NULL;
    sp->workingThreadId = 0; 
    sp->isOpen = FALSE;
    sp->TotalCount = 0;
    ZeroMemory(&sp->olRead, sizeof(OVERLAPPED));
    ZeroMemory(&sp->olWrite, sizeof(OVERLAPPED));
    if(sp->mRecvRngId != NULL)
    {
        Sr_DestroyDynamic(sp->mRecvRngId);
        sp->mRecvRngId = NULL;
    }   
    if(sp->mSendRngId != NULL)
    {
        Sr_DestroyDynamic(sp->mSendRngId);
        sp->mSendRngId = NULL;
    }   
    
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
    S16  portNameLength;
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
        FILE_FLAG_OVERLAPPED, // ✅ 异步
        NULL
    );

    if ((sp->portHandle == 0) || ((unsigned long)sp->portHandle == 0xffffffff))
    {
        sp->portHandle = 0;
        printf("Error: Cannot open COM port %s\n", portName);
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
    portTimeOuts.ReadIntervalTimeout = MAXDWORD; // 非阻塞读
    portTimeOuts.ReadTotalTimeoutMultiplier = 0;
    portTimeOuts.ReadTotalTimeoutConstant = 0;  // 整体超时由参数决定

    //OVERLAPPED olRead = {0};
    //olRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  // 自动重置事件

    if (!SetCommTimeouts(sp->portHandle, &portTimeOuts))
    {
        return FALSE;
    }


    // 创建事件 & 绑定 OVERLAPPED
    sp->hEventRead  = CreateEvent(NULL, TRUE, FALSE, NULL);
    sp->hEventWrite = CreateEvent(NULL, TRUE, FALSE, NULL);

    ZeroMemory(&sp->olRead, sizeof(OVERLAPPED));
    ZeroMemory(&sp->olWrite, sizeof(OVERLAPPED));
    sp->olRead.hEvent  = sp->hEventRead;
    sp->olWrite.hEvent = sp->hEventWrite;

    sp->isOpen = TRUE;

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
    sp->isOpen = FALSE;
    if (sp->portHandle)
    {
        CloseHandle(sp->portHandle);
        sp->portHandle = NULL;
        Sleep(SERIALPORT_INTERNAL_TIMEOUT*4);
    }

    // 关闭事件句柄
    if (sp->hEventRead)  
    {
        CloseHandle(sp->hEventRead);
        sp->hEventRead = NULL;
    }
    if (sp->hEventWrite) 
    {
        CloseHandle(sp->hEventWrite);
        sp->hEventWrite = NULL;
    }

    // 等待线程退出
    if (sp->workingThread)
    { 
        WaitForSingleObject(sp->workingThread, 2000);
        CloseHandle(sp->workingThread);
        sp->workingThread = NULL;
        sp->workingThreadId = 0;
    }
}





S32 SerialPort_WriteBuffer(SerialPort* sp, const U8* data, S32 length)
{
    DWORD bytesWritten = 0;
    BOOL ok = WriteFile(
        sp->portHandle,
        data,
        length,
        &bytesWritten,
        &sp->olWrite
    );

    if (!ok && GetLastError() == ERROR_IO_PENDING) 
    {
        WaitForSingleObject(sp->hEventWrite, INFINITE);
        GetOverlappedResult(sp->portHandle, &sp->olWrite, &bytesWritten, FALSE);
    }

    if (bytesWritten > 0 && sp->onDataSentHandler) 
    {
        sp->onDataSentHandler(data, bytesWritten);
    }

    return (S32)bytesWritten;
}



DWORD WINAPI SerialPort_WaitForData(LPVOID lpParam)
{
    SerialPort* sp = (SerialPort*)lpParam;
    U8 buffer[64];
    DWORD bytesRead = 0;

    while (sp->isOpen) 
    {
        ResetEvent(sp->hEventRead);

        BOOL ok = ReadFile(
            sp->portHandle,
            buffer,
            sizeof(buffer),
            &bytesRead,
            &sp->olRead   // ✅ 异步
        );

        if (!ok && GetLastError() == ERROR_IO_PENDING) 
        {
            // 等待完成
            DWORD waitRes = WaitForSingleObject(sp->hEventRead, sp->timeoutMilliSeconds);
            if (waitRes == WAIT_OBJECT_0) 
            {
                GetOverlappedResult(sp->portHandle, &sp->olRead, &bytesRead, FALSE);
            } else if (waitRes == WAIT_TIMEOUT) 
            {
                continue; // 超时，无数据
            }
        }

        sp->TotalCount += bytesRead;

        if (bytesRead > 0 && sp->mRecvRngId != NULL && sp->mRecvRngId->buf != NULL)
        {
            U32 result = Sr_BufPut(sp->mRecvRngId, buffer, bytesRead); // 默认放入接收缓冲区
            //printf("Info: Put %d bytes to recv buffer, Total %d bytes\r\n", result, sp->TotalCount);
            //printf("Info: Data received %d bytes, Total %d bytes\r\n", bytesRead, sp->TotalCount);
        }

        if (bytesRead > 0 && sp->onDataReceivedHandler) 
        {
            sp->onDataReceivedHandler(buffer, bytesRead);   //支持使用回调函数对数据的其他处理
        }
    }
    return 0;
}


