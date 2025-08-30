#ifndef __SERIAL_PORT_H__
#define __SERIAL_PORT_H__

#include <windows.h>
#include "types.h"

/* 数据接收回调函数类型 */
typedef S32 (*SerialPort_OnDataReceived)(const U8* pData, S32 dataLength);
/* 数据发送完成回调函数类型 */
typedef void (*SerialPort_OnDataSent)(const U8* pData, S32 dataLength);

/**
 * 串口实例结构体
 * 每个串口实例都需要独立维护该结构体，
 * 避免全局变量导致的多实例冲突。
 */
typedef struct SerialPort
{
    HANDLE  portHandle;                                 // 串口句柄
    HANDLE  workingThread;                              // 工作线程句柄
    HANDLE hEventRead;
    HANDLE hEventWrite;
    DWORD   workingThreadId;                            // 工作线程ID
    S32     timeoutMilliSeconds;                        // 读写超时时间

    OVERLAPPED olRead;                                     
    OVERLAPPED olWrite;
    BOOL isOpen;

    SerialPort_OnDataReceived onDataReceivedHandler;    // 数据接收回调
    SerialPort_OnDataSent     onDataSentHandler;        // 数据发送回调

    CRITICAL_SECTION criticalSectionRead;               // 读操作临界区
    CRITICAL_SECTION criticalSectionWrite;              // 写操作临界区


    U32 TotalCount;                                     // 统计总字节数
} SerialPort;

/* 初始化与销毁 */
void    SerialPort_Initialize(SerialPort* sp);
void    SerialPort_Uninitialize(SerialPort* sp);

/* 打开与关闭 */
BOOL    SerialPort_OpenAsync(SerialPort* sp,
                             S32 comPortNumber,
                             S32 baudRate,
                             SerialPort_OnDataReceived onRecv,
                             SerialPort_OnDataSent onSent,
                             S32 timeoutMS);

BOOL    SerialPort_A_OpenAsync(SerialPort* sp,
                             S32 comPortNumber,
                             S32 baudRate,
                             SerialPort_OnDataReceived onRecv,
                             SerialPort_OnDataSent onSent,
                             S32 timeoutMS);

BOOL    SerialPort_Open(SerialPort* sp, S32 comPortNumber, S32 baudRate, S32 timeoutMS);
BOOL  SerialPort_A_Open(SerialPort* sp, S32 comPortNumber, S32 baudRate, S32 timeoutMS);
void    SerialPort_Close(SerialPort* sp);

/* 读写接口 */
S32     SerialPort_ReadBuffer(SerialPort* sp, U8* pData, S32 dataLength, S32 timeOutMS);
S32     SerialPort_WriteBuffer(SerialPort* sp, const U8* pData, S32 dataLength);

S32     SerialPort_A_WriteBuffer(SerialPort* sp, const U8* pData, S32 dataLength);
S32     SerialPort_A_ReadBuffer(SerialPort* sp, U8* pData, S32 dataLength, S32 timeOutMS);
DWORD   WINAPI SerialPort_WaitForData(LPVOID lpParam);
DWORD   WINAPI SerialPort_A_WaitForData(LPVOID lpParam);    

#endif /* __SERIAL_PORT_H__ */
