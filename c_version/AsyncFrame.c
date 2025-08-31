#include "AsyncFrame.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ProcCmd.h"

// ==== 全局变量 ====
static FrameMsg g_frameQueue[FRAME_QUEUE_SIZE];
static U32 g_qHead = 0;   // 读指针
static U32 g_qTail = 0;   // 写指针

static CRITICAL_SECTION g_qLock;
static HANDLE g_hFrameEvent = NULL;  // 有新帧时触发事件
static HANDLE g_hProcThread = NULL;

static FrameHandler g_frameHandlers[MAX_FRAME_TYPE]; // 回调表
static U32 g_running = 0;


void RegFrameHandlers(void)
{
    RegisterFrameHandler(0x11, ProcParaCmd);   // 参数指令
    RegisterFrameHandler(0x22, ProcWaveCmd);   // 波形指令
    RegisterFrameHandler(0xAA, ProcTestCmd);    // 测试指令
    RegisterFrameHandler(0xFF, ProcErrorCmd);   // 错误指令
}


// ==== 分发函数 ====
static void DispatchFrame(const U8* frame, U32 len) 
{
    U8 frameType = frame[0];
    if (g_frameHandlers[frameType]) 
    {
        g_frameHandlers[frameType](frame, len);
    } 
    else 
    {
        printf("Unknown frame type: 0x%02X\n", frameType);
    }
}

// ==== 处理线程 ====
static DWORD WINAPI ProcThread(LPVOID lpParam) 
{
    while (g_running) 
    {
        WaitForSingleObject(g_hFrameEvent, INFINITE);
        while (1) 
        {
            EnterCriticalSection(&g_qLock);
            if (g_qHead == g_qTail) 
            {
                ResetEvent(g_hFrameEvent);
                LeaveCriticalSection(&g_qLock);
                break;
            }
            FrameMsg *msg = &g_frameQueue[g_qHead];
            g_qHead = (g_qHead + 1) % FRAME_QUEUE_SIZE;
            LeaveCriticalSection(&g_qLock);

            DispatchFrame(msg->frameBuf, msg->len);
        }
    }
    return 0;
}

// ==== API 实现 ====
void AsyncFrame_Init(void) 
{
    InitializeCriticalSection(&g_qLock);
    g_hFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    memset(g_frameHandlers, 0, sizeof(g_frameHandlers));

    RegFrameHandlers(); // 注册帧处理函数
    g_running = 1;
    g_hProcThread = CreateThread(NULL, 0, ProcThread, NULL, 0, NULL);

    printf("=== Async Frame Dispatcher Init ===\n");
}

void AsyncFrame_Uninit(void) 
{
    g_running = 0;
    if (g_hFrameEvent) SetEvent(g_hFrameEvent); // 唤醒线程退出

    if (g_hProcThread) 
    {
        WaitForSingleObject(g_hProcThread, INFINITE);
        CloseHandle(g_hProcThread);
        g_hProcThread = NULL;
    }

    if (g_hFrameEvent) 
    {
        CloseHandle(g_hFrameEvent);
        g_hFrameEvent = NULL;
    }
    g_qHead = g_qTail = 0;
    for (U32 i = 0; i < MAX_FRAME_TYPE; i++) 
    {
        g_frameHandlers[i] = NULL;
    }   

    DeleteCriticalSection(&g_qLock);
    printf("=== Async Frame Dispatcher Deinit ===\n");
}

void RegisterFrameHandler(U8 frameType, FrameHandler handler) 
{
    g_frameHandlers[frameType] = handler;
}

void UnregisterFrameHandler(U8 frameType) 
{
    g_frameHandlers[frameType] = NULL;
}

S32 PushFrameToQueue(const U8* frame, U32 len) 
{
    if (!frame || len <= 0 || len > uFRAME_MAX_LEN) return -1;

    EnterCriticalSection(&g_qLock);
    U32 nextTail = (g_qTail + 1) % FRAME_QUEUE_SIZE;
    if (nextTail != g_qHead) 
    { // 队列不满
        memcpy(g_frameQueue[g_qTail].frameBuf, frame, len);
        g_frameQueue[g_qTail].len = len;
        g_qTail = nextTail;
        SetEvent(g_hFrameEvent);
        LeaveCriticalSection(&g_qLock);
        return 0;
    } 
    else 
    {
        LeaveCriticalSection(&g_qLock);
        printf("Frame queue full, dropping frame!\n");
        return -2; // 队列满
    }
}
