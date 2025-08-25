#ifndef __SERIAL_PORT_H__
#define __SERIAL_PORT_H__


#include <windows.h>
void    SerialPort_Initialize(void);
void    SerialPort_Uninitialize(void);
int     SerialPort_GetMaxTimeout();
int     SerialPort_GetDataReceivedHandler();
int     SerialPort_GetDataSentHandler();
BOOL    SerialPort_OpenAsync(int comPortNumber, 
                             int baudRate,                             
                             void (*OnDataReceivedHandler)(const unsigned char* pData, int dataLength),
                             void (*OnDataSentHandler)(void),                         
                             int timeoutMS);
BOOL    SerialPort_Open(int comPortNumber, int baudRate, int timeoutMS);
void    SerialPort_Close();
BOOL    SerialPort_IsOpen();
int     SerialPort_ReadBuffer(unsigned char* pData, int dataLength, int timeOutMS);
int     SerialPort_WriteBuffer(const unsigned char* pData, int dataLength);
int     SerialPort_WriteLine(char* pLine, BOOL addCRatEnd);
int     SerialPort_ReadLine(char* pLine, int maxBufferSize, int timeOutMS);


#endif /* __SERIAL_PORT_H__ */