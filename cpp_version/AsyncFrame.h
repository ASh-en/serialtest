#ifndef __ASYNC_FRAME_DISPATCHER_H__
#define __ASYNC_FRAME_DISPATCHER_H__

#include <cstdint>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <atomic>
#include <string>
#include <memory>


// 使用 C++ using 别名替代 typedef
using U8 = uint8_t;
using U16 = uint16_t;
using U32 = uint32_t;
using S32 = int32_t;

// 常量定义使用 constexpr
constexpr U32 FRAME_QUEUE_SIZE = 1024;   // 队列大小
constexpr U8 MAX_FRAME_TYPE = 127;      // 帧类型最大 256 种
constexpr U8 MAX_CMD_LEN = 0x65;        // 最大业务命令长度，需与 CmdFrm.h 保持一致

// 异步帧调度器类（单例模式）
class AsyncFrameDispatcher {
private:
    std::vector<std::vector<U8>> m_frameQueue;
    U32 m_queueHead;           // 读指针
    U32 m_queueTail;           // 写指针
    std::mutex m_queueMutex;   // 队列互斥锁
    std::condition_variable m_frameCondition; // 帧到达条件变量
    std::thread m_processThread; // 处理线程
    std::vector<std::function<S32(const std::vector<U8>&, U32)>> m_frameHandlers; // 回调函数表
    std::atomic<bool> m_running; // 运行状态标志
    std::mutex m_handlerMutex; // 回调函数表互斥锁

    // 私有构造函数（单例模式）
    AsyncFrameDispatcher();
    
    // 处理线程函数
    void processThreadFunc();
    
    // 分发帧
    void dispatchFrame(const std::vector<U8>& frame, U32 len);

public:
    // 获取单例实例
    static AsyncFrameDispatcher& getInstance();
    
    // 禁止拷贝构造和赋值操作
    AsyncFrameDispatcher(const AsyncFrameDispatcher&) = delete;
    AsyncFrameDispatcher& operator=(const AsyncFrameDispatcher&) = delete;
    
    // 允许移动构造和移动赋值
    AsyncFrameDispatcher(AsyncFrameDispatcher&&) noexcept = default;
    AsyncFrameDispatcher& operator=(AsyncFrameDispatcher&&) noexcept = default;
    
    // 初始化
    void init();
    
    // 反初始化
    void uninit();
    
    // 注册帧处理函数
    void registerFrameHandler(U8 frameType, std::function<S32(const std::vector<U8>&, U32)> handler);
    
    // 注销帧处理函数
    void unregisterFrameHandler(U8 frameType);
    
    // 将一帧放入队列
    S32 pushFrameToQueue(const std::vector<U8>& frame, U32 len);
};


#endif /*__ASYNC_FRAME_DISPATCHER_H__*/