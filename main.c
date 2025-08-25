#include <stdio.h>
#include <conio.h>
#include "serial_port.h"
#include "CmdFrm.h"

int main()
{
    SerialPortPara para;
    initSerialPortPara(&para,"COM4",9600,MY_PARITY_NONE,1,STOPBITS_ONE,true,true);
    printSerialPortPara(&para);

    HANDLE hCom = openSerialPort(&para);
    if(hCom==INVALID_HANDLE_VALUE) return -1;
    
    GloabalRingBufInit();
    startReceiveThread(hCom,NULL);
    while(1)
    {
        ProcPrmFrame();
        Sleep(10);

        if (kbhit()) 
        {   // 检测键盘输入
            char ch = getch();
            if (ch == 'q' || ch == 'Q')
            { // 输入 q/Q 退出
                break;
            }
        }
    }
    

    stopReceiveThread();
    closeSerialPort(hCom);
    return 0;
}
