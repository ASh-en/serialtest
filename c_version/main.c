#include <stdio.h>
#include <conio.h>

#include "CmdFrm.h"
#include "SerialPort.h"
#include "ProcCmd.h"

void DataSent()
{
    printf("Info: Data sent\r\n");
}

int main(int argc, char* argv[])
{
    
    int comPort = 5;          // 默认使用 COM5 端口
    int baudRate = 115200;     // 默认波特率 115200
    int timeoutMS = 50;     // 默认超时时间 50 毫秒

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

    SerialPort_Initialize();
    GloabalRingBufInit();

    if (!SerialPort_OpenAsync(comPort, baudRate, PutPrmFrame, DataSent, timeoutMS)) {
        printf("Failed to open serial port COM%d\n", comPort);
        return -1;
    }

    printf("Serial port COM%d opened successfully.\n", comPort);
    printf("Press 'q' to quit.\n");

    while (1) {
        ProcPrmFrame(); // 处理接收到的命令帧

        if (_kbhit()) {
            char ch = _getch();
            if (ch == 'q' || ch == 'Q') {
                break; // 按 'q' 键退出
            }
        }

        Sleep(10); // 避免 CPU 占用过高
    }

    SerialPort_Close();
    SerialPort_Uninitialize();

    printf("Serial port closed. Exiting program.\n");
    return 0;
}