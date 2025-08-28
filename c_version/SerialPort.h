#ifndef __SERIAL_PORT_H__
#define __SERIAL_PORT_H__


#include <windows.h>
#include "types.h"
void    SerialPort_Initialize(void);
void    SerialPort_Uninitialize(void);
S32     SerialPort_GetMaxTimeout();
S32     SerialPort_GetDataReceivedHandler();
S32     SerialPort_GetDataSentHandler();
BOOL    SerialPort_OpenAsync(S32 comPortNumber, 
                             S32 baudRate,                             
                             S32 (*OnDataReceivedHandler)(const U8* pData, S32 dataLength),
                             void (*OnDataSentHandler)(void),                         
                             S32 timeoutMS);
BOOL    SerialPort_Open(S32 comPortNumber, S32 baudRate, S32 timeoutMS);
void    SerialPort_Close();
BOOL    SerialPort_IsOpen();
S32     SerialPort_ReadBuffer(U8* pData, S32 dataLength, S32 timeOutMS);
S32     SerialPort_WriteBuffer(const U8* pData, S32 dataLength);
S32     SerialPort_WriteLine(char* pLine, BOOL addCRatEnd);
S32     SerialPort_ReadLine(char* pLine, S32 maxBufferSize, S32 timeOutMS);


#endif /* __SERIAL_PORT_H__ */