#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "../types.h"

#define FRAME_SIZE  0x6A  // Frame size (106 bytes as per your data)
#define FRAME_COUNT 5000  // Number of frames to send

// Build one test frame
void BuildTestFrame(U8 *buf) 
{
    U8 testFrame[FRAME_SIZE] = {
        0x66,0x00,0x00,0x64,0xAA,0x55,0x00,0x00,
        0x11,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x22,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x33,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x44,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x55,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x77,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x88,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0xAA,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0xBB,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0xCC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0xDD,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0xEE,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0xFD,0x99
    };
    memcpy(buf, testFrame, FRAME_SIZE);
}

int TestSendFrames()
{
    HANDLE hCom;
    DCB dcb;
    COMMTIMEOUTS timeouts;
    BOOL fSuccess;
    DWORD bytesWritten;
    DWORD totalBytes = 0;

    // Open COM3
    hCom = CreateFile("COM3",
                      GENERIC_READ | GENERIC_WRITE,
                      0,    // exclusive access
                      NULL, // default security attributes
                      OPEN_EXISTING,
                      0,
                      NULL);
    if (hCom == INVALID_HANDLE_VALUE) {
        printf("Failed to open COM3, error code: %lu\n", GetLastError());
        return -1;
    }

    // Get current state
    fSuccess = GetCommState(hCom, &dcb);
    if (!fSuccess) {
        printf("GetCommState failed\n");
        CloseHandle(hCom);
        return -1;
    }

    // Configure serial port
    dcb.BaudRate = CBR_115200;  // baud rate
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;

    fSuccess = SetCommState(hCom, &dcb);
    if (!fSuccess) {
        printf("SetCommState failed\n");
        CloseHandle(hCom);
        return -1;
    }

    // Configure timeouts
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    SetCommTimeouts(hCom, &timeouts);

    printf("Opened COM3 successfully. Start sending frames...\n");

    U8 buf[FRAME_SIZE];

    DWORD startTick = GetTickCount(); // Start time

    for (int i = 0; i < FRAME_COUNT; i++) {
        BuildTestFrame(buf);

        fSuccess = WriteFile(hCom, buf, FRAME_SIZE, &bytesWritten, NULL);
        if (!fSuccess || bytesWritten != FRAME_SIZE) {
            printf("Send failed at frame %d\n", i+1);
            break;
        }

        totalBytes += bytesWritten;

        if ((i+1) % 1000 == 0) {
            printf("Sent %d frames...\n", i+1);
        }
        Sleep(1);  // small delay, avoid overloading serial buffer
    }

    DWORD endTick = GetTickCount(); // End time
    DWORD elapsed = endTick - startTick;

    printf("Finished sending.\n");
    printf("Total frames sent: %d\n", FRAME_COUNT);
    printf("Total bytes sent : %lu\n", totalBytes);
    printf("Elapsed time     : %lu ms\n", elapsed);

    CloseHandle(hCom);
    return 0;
}

int main()
{
    return TestSendFrames();
}
