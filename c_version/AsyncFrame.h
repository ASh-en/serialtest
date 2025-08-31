#ifndef __ASYNC_FRAME_H__
#define __ASYNC_FRAME_H__

#include <windows.h>
#include "types.h"
#include "CmdFrm.h"

#ifdef __cplusplus
extern "C" {
#endif

// ================== 配置 ==================

#define FRAME_QUEUE_SIZE 128    // 队列大小
#define MAX_FRAME_TYPE   256    // 帧类型最大 256 种



// ==== 帧消息结构 ====
typedef struct {
    U8 frameBuf[MAX_CMD_LEN];
    S32 len;
} FrameMsg;

// ==== 回调函数类型 ====
typedef S32 (*FrameHandler)(const U8* frame, U32 len);

// ==== API ====

// 初始化（必须先调用）
void AsyncFrame_Init(void);

// 反初始化（释放资源）
void AsyncFrame_Uninit(void);

// 注册帧处理函数
void RegisterFrameHandler(U8 frameType, FrameHandler handler);

// 注销帧处理函数
void UnregisterFrameHandler(U8 frameType);

// 将一帧放入队列（接收线程调用）
S32 PushFrameToQueue(const U8* frame, U32 len);

#ifdef __cplusplus
}
#endif

#endif // __ASYNC_FRAME_H__
