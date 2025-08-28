#include "SerialPort.hpp"
#include <stdio.h>
#include <stdlib.h>

#define SERIALPORT_INTERNAL_TIMEOUT 1

TSerialPort::TSerialPort()
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

TSerialPort::~TSerialPort()
{
    if (m_portHandle)
    {
        Close();        
    }
    DeleteCriticalSection(&m_criticalSectionRead);
    DeleteCriticalSection(&m_criticalSectionWrite);
}

S32 TSerialPort::GetMaxTimeout()
{
    return m_timeoutMilliSeconds;
}

void* TSerialPort::GetDataReceivedHandler()
{
    return m_OnDataReceivedHandler;
}

void* TSerialPort::GetDataSentHandler()
{
    return m_OnDataSentHandler;
}

bool TSerialPort::OpenAsync(S32 comPortNumber, 
                            S32 baudRate,                             
                            void (*OnDataReceivedHandler)(const U8* pData, S32 dataLength),
                            void (*OnDataSentHandler)(void),                         
                            S32 timeoutMS 
                            )                            
{
    bool result = Open(comPortNumber, baudRate, timeoutMS);
    if (result)
    {
        m_OnDataReceivedHandler = OnDataReceivedHandler;
        m_OnDataSentHandler     = OnDataSentHandler;
        if (m_OnDataSentHandler || m_OnDataReceivedHandler)
        {
            m_workingThread = CreateThread(NULL, 0, SerialPort_WaitForData, this, 0, &m_workingThreadId);
        }
    }
    return result;
}

bool TSerialPort::Open(S32 comPortNumber, S32 baudRate, S32 timeoutMS)
{
    char portName[12];
    S32  portNameLength;
    if (comPortNumber<0) return false;
    if (comPortNumber>255) return false;
    if (timeoutMS>15000) return false;
    
    portNameLength = sprintf(portName, "\\\\.\\COM%i", comPortNumber );
    
    m_portHandle = CreateFileA(portName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if ((m_portHandle==0) || ((unsigned long)m_portHandle==0xffffffff))
    {
        m_portHandle = 0;
        return false;
    }

    m_timeoutMilliSeconds = timeoutMS;
    
    DCB portSettings;
    memset(&portSettings, 0, sizeof(portSettings));
    
    portSettings.BaudRate = baudRate;
    portSettings.ByteSize = 8;
    portSettings.Parity   = NOPARITY;
    portSettings.StopBits = ONESTOPBIT;
    
    if (!SetCommState(m_portHandle, &portSettings))
    {
        return false;
    }
    
    COMMTIMEOUTS portTimeOuts;                   
    memset(&portTimeOuts, 0, sizeof(portTimeOuts));
    portTimeOuts.ReadIntervalTimeout = SERIALPORT_INTERNAL_TIMEOUT;
    portTimeOuts.ReadTotalTimeoutMultiplier = 0;    
    portTimeOuts.ReadTotalTimeoutConstant = SERIALPORT_INTERNAL_TIMEOUT;    

    if (!SetCommTimeouts(m_portHandle, &portTimeOuts))
    {
        return false;
    }
    return (m_portHandle!=0);
}

void TSerialPort::Close()
{    
    EnterCriticalSection(&m_criticalSectionRead);  //prevents port closing if ReadBuffer  is not complete
    EnterCriticalSection(&m_criticalSectionWrite); //prevents port closing if WriteBuffer is not complete
    if (m_portHandle)
    {
        CloseHandle(m_portHandle);			
        m_portHandle = NULL;
        Sleep(SERIALPORT_INTERNAL_TIMEOUT*4);
        m_workingThread = NULL;
        m_workingThreadId = 0;      
    }
    LeaveCriticalSection(&m_criticalSectionRead);
    LeaveCriticalSection(&m_criticalSectionWrite);
}

bool TSerialPort::IsOpen()
{
    return (m_portHandle!=0);
}

S32 TSerialPort::__WriteBuffer(const U8* pData, S32 dataLength)
{
    if (m_portHandle==NULL)
    {
        return 0;
    }
    DWORD bytesWritten = 0;
    
    if (!WriteFile(m_portHandle, pData, dataLength, &bytesWritten, NULL))
    {
        return 0;
    }    
    return (S32)bytesWritten;   
}

S32 TSerialPort::__ReadBuffer(U8* pData, S32 dataLength, S32 timeOutMS)
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

    if (timeOutMS<SERIALPORT_INTERNAL_TIMEOUT*2)
    {
        Sleep(timeOutMS);
        ReadFile(m_portHandle, pData, dataLength, &bytesRead, NULL);
        bytesReadTotal += bytesRead;
    } else {
        while(timeOutCounter>0)
        {            
            bytesRead = 0;
            ReadFile(m_portHandle, pData+bytesReadTotal, dataLength, &bytesRead, NULL);
            if (bytesRead)
            {
                dataLength     -= bytesRead;
                bytesReadTotal += bytesRead;
                if (dataLength==0) break;
                timeOutCounter = timeOutMS;
            } else {
                timeOutCounter -= SERIALPORT_INTERNAL_TIMEOUT;
            }            
        }
    }
    if (bytesReadTotal)
    {
        if (m_OnDataReceivedHandler)
        {
            m_OnDataReceivedHandler(pData, bytesReadTotal);
        }
    }
	return bytesReadTotal;
}

S32 TSerialPort::ReadBuffer(U8* pData, S32 dataLength, S32 timeOutMS)
{
    EnterCriticalSection(&m_criticalSectionRead);
    S32 result = __ReadBuffer(pData, dataLength, timeOutMS);
    LeaveCriticalSection(&m_criticalSectionRead);
    return result;
}


S32 TSerialPort::WriteBuffer(const U8* pData, S32 dataLength)
{
    EnterCriticalSection(&m_criticalSectionWrite);
    S32 result = __WriteBuffer(pData, dataLength);
    LeaveCriticalSection(&m_criticalSectionWrite);
    if (m_OnDataSentHandler)
    {
        m_OnDataSentHandler();
    }
    return result;
}


S32 TSerialPort::WriteLine(char* pLine, bool addCRatEnd)
{
    
    if (m_portHandle==NULL)
    {
        return 0;
    }
    if (pLine==NULL)
    {
        return 0;
    }
    S32 lineLength = strlen(pLine);
    if (lineLength==0)
    {
        return 0;
    }
    
    EnterCriticalSection(&m_criticalSectionWrite);
    S32 result = __WriteBuffer((U8*)pLine, lineLength);
    if (pLine[lineLength-1]!=0x0D)
    {
        char cr = 13;
        result+=__WriteBuffer((U8*)&cr, 1);
    }
    LeaveCriticalSection(&m_criticalSectionWrite);
    if (m_OnDataSentHandler)
    {
        m_OnDataSentHandler();
    }
    return result;
}

S32 TSerialPort::ReadLine(char* pLine, S32 maxBufferSize, S32 timeOutMS)
{
    if (m_portHandle==NULL)
    {
        return 0;
    }
    if (pLine==NULL)
    {
        return 0;
    }
    
    EnterCriticalSection(&m_criticalSectionRead);
    S32 result = __ReadBuffer((U8*)pLine, maxBufferSize-1, timeOutMS);	
    if (result<maxBufferSize)
    {
        pLine[result] = 0;
    }
    LeaveCriticalSection(&m_criticalSectionRead);
    return result;
}

DWORD WINAPI SerialPort_WaitForData( LPVOID lpParam )       //该回调函数必须是static 或全局函数，故无法使用类成员函数
{
    TSerialPort* serialPort = (TSerialPort*)lpParam;
    
    static U8 packet[64];
    S32 packetSize = sizeof(packet);   
    while(serialPort->IsOpen())
    {
        serialPort->ReadBuffer(packet, packetSize, SERIALPORT_INTERNAL_TIMEOUT);
    }    
    return 0;
}