#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>

#include "CmdFrm.h"
#include "SerialPort.h"
#include "ProcCmd.h"
extern SerialPort sp;  // 串口实例对象
static U32 count = 0;
static U32 TotalCount = 0;
/* 数据发送完成回调 */
void DataSent(const U8* pData, S32 dataLength)
{
    
    TotalCount += dataLength;
    count++; 
}
S32 DataRecv(const U8* pData, S32 dataLength)
{
    static U32 count = 0;
    static U32 TotalCount = 0;
    TotalCount += dataLength;
    printf("Count %d, Info: Data received %d bytes, Total %d bytes\r\n", ++count, dataLength, TotalCount);  
    return 0;
}

/* 程序入口 */
int main(int argc, char* argv[])
{
    int comPort   = 4;       // 默认 COM5
    int baudRate  = 115200;  // 默认波特率
    int timeoutMS = 50;      // 默认超时时间

    if (argc >= 2) {
        comPort = atoi(argv[1]);
    }
    if (argc >= 3) {
        baudRate = atoi(argv[2]);
    }
    if (argc >= 4) {
        timeoutMS = atoi(argv[3]);
    }

    printf("Opening serial port COM%d at %d baud...\n", comPort, baudRate);

    
    SerialPort_Initialize(&sp, 0x400, 0); // 初始化串口实例，设置收发缓冲区大小
    //GloabalRingBufInit();

    if (!SerialPort_OpenAsync(&sp, comPort, baudRate, NULL, DataSent, timeoutMS)) {
        printf("Failed to open serial port COM%d\n", comPort);
        SerialPort_Uninitialize(&sp);
        return -1;
    }

    printf("Serial port COM%d opened successfully.\n", comPort);
    printf("Press 'q' to quit.\n");

    while (1) {
        ProcPrmFrame(sp.mRecvRngId); // 处理接收到的命令帧

        if (_kbhit()) {
            char ch = _getch();
            if (ch == 'q' || ch == 'Q') {
                break; // 按 'q' 键退出
            }
        }

        Sleep(1); // 降低 CPU 占用
    }
    printf("Count %d, Info: Data sent  Total %d bytes\r\n", count, TotalCount);

    SerialPort_Close(&sp);
    SerialPort_Uninitialize(&sp);

    printf("Serial port closed. Exiting program.\n");
    return 0;
}
