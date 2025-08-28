#pragma once

#include <windows.h>
#include "types.hpp"

class TSerialPort
{
private:
    HANDLE m_portHandle;
    HANDLE m_workingThread;
    DWORD  m_workingThreadId;
    
    S32    m_timeoutMilliSeconds;
    S32    m_maxPacketLength;
    
    void (*m_OnDataReceivedHandler)(const U8* pData, S32 dataLength);
    void (*m_OnDataSentHandler)(void);
    
    CRITICAL_SECTION m_criticalSectionRead;
    CRITICAL_SECTION m_criticalSectionWrite;
    
    S32 __ReadBuffer(U8* pData, S32 dataLength, S32 timeOutMS=-1);		
    S32 __WriteBuffer(const U8* pData, S32 dataLength);	
    
    
public:	
    TSerialPort();
    ~TSerialPort();
    
    S32 GetMaxTimeout();
    void* GetDataReceivedHandler();
    void* GetDataSentHandler();
    
    bool Open(S32 comPortNumber, S32 baudRate, S32 timeoutMS=50);
    
    bool OpenAsync( S32 comPortNumber, S32 baudRate, 
                    void (*OnDataReceivedHandler)(const U8* pData, S32 dataLength),
                    void (*OnDataSentHandler)(void),            
                    S32 timeoutMS=100
                   );
    
    void Close();
    bool IsOpen();
    
    S32 ReadBuffer(U8* pData, S32 dataLength, S32 timeOutMS=-1);		
    S32 WriteBuffer(const U8* pData, S32 dataLength);	
    
    S32 ReadLine(char* pLine, S32 maxBufferSize, S32 timeOutMS=-1);
    S32 WriteLine(char* pLine, bool addCRatEnd=true);
    
    
};

DWORD WINAPI SerialPort_WaitForData( LPVOID lpParam );
