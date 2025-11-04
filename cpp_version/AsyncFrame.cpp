#include <iostream>
#include <cstring>
#include "AsyncFrame.h"



// 构造函数
AsyncFrameDispatcher::AsyncFrameDispatcher()
    : m_queueHead(0),
      m_queueTail(0),
      m_running(false) {
    // 预分配队列空间
    m_frameQueue.resize(FRAME_QUEUE_SIZE);
    for (auto& frame : m_frameQueue) {
        frame.resize(MAX_CMD_LEN);
    }
    // 初始化处理函数表
    m_frameHandlers.resize(MAX_FRAME_TYPE);
}

// 获取单例实例
AsyncFrameDispatcher& AsyncFrameDispatcher::getInstance() {
    static AsyncFrameDispatcher instance;
    return instance;
}

// 初始化
void AsyncFrameDispatcher::init() {
    if (!m_running) {
        m_running = true;
        m_queueHead = m_queueTail = 0;
        // 清空回调函数表
        std::fill(m_frameHandlers.begin(), m_frameHandlers.end(), nullptr);
        // 创建处理线程
        m_processThread = std::thread(&AsyncFrameDispatcher::processThreadFunc, this);
        std::cout << "=== Async Frame Dispatcher Init ===" << std::endl;
    }
}

// 反初始化
void AsyncFrameDispatcher::uninit() {
    if (m_running) {
        m_running = false;
        m_frameCondition.notify_one(); // 唤醒线程退出
        
        if (m_processThread.joinable()) {
            m_processThread.join();
        }
        
        m_queueHead = m_queueTail = 0;
        // 清空回调函数表
        std::fill(m_frameHandlers.begin(), m_frameHandlers.end(), nullptr);
        
        std::cout << "=== Async Frame Dispatcher Deinit ===" << std::endl;
    }
}

// 注册帧处理函数
void AsyncFrameDispatcher::registerFrameHandler(U8 frameType, std::function<S32(const std::vector<U8>&, U32)> handler) {
    if (frameType < MAX_FRAME_TYPE) {
        std::lock_guard<std::mutex> guard(m_handlerMutex);
        m_frameHandlers[frameType] = std::move(handler);
    }
}

// 注销帧处理函数
void AsyncFrameDispatcher::unregisterFrameHandler(U8 frameType) {
    if (frameType < MAX_FRAME_TYPE) {
        std::lock_guard<std::mutex> guard(m_handlerMutex);
        m_frameHandlers[frameType] = nullptr;
    }
}

// 将一帧放入队列
S32 AsyncFrameDispatcher::pushFrameToQueue(const std::vector<U8>& frame, U32 len) {
    if (frame.empty() || len <= 0 || len > MAX_CMD_LEN) {
        return -1;
    }
    
    std::lock_guard<std::mutex> guard(m_queueMutex);
    U32 nextTail = (m_queueTail + 1) % FRAME_QUEUE_SIZE;
    
    if (nextTail != m_queueHead) {
        // 队列不满，放入数据
        if (m_frameQueue[m_queueTail].size() < len) {
            m_frameQueue[m_queueTail].resize(len);
        }
        std::memcpy(m_frameQueue[m_queueTail].data(), frame.data(), len);
        m_queueTail = nextTail;
        m_frameCondition.notify_one(); // 通知处理线程
        return 0;
    } else {
        // 队列已满
        std::cerr << "Frame queue full, dropping frame!" << std::endl;
        return -2;
    }
}

// 处理线程函数
void AsyncFrameDispatcher::processThreadFunc() {
    while (m_running) {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        // 等待有新帧或停止信号
        m_frameCondition.wait(lock, [this] { 
            return !m_running || m_queueHead != m_queueTail; 
        });
        
        // 如果是停止信号，退出循环
        if (!m_running) {
            break;
        }
        
        // 处理队列中的所有帧
        while (m_queueHead != m_queueTail) {
            // 创建副本，避免长时间占用锁
            std::vector<U8> frame = m_frameQueue[m_queueHead];
            U32 len = frame.size();
            m_queueHead = (m_queueHead + 1) % FRAME_QUEUE_SIZE;
            lock.unlock(); // 处理帧前解锁
            
            dispatchFrame(frame, len);
            
            lock.lock(); // 重新获取锁，检查队列是否还有数据
        }
    }
}

// 分发帧
void AsyncFrameDispatcher::dispatchFrame(const std::vector<U8>& frame, U32 len) {
    if (frame.empty() || len == 0) {
        return;
    }
    
    U8 frameType = frame[0];
    
    // 获取对应的处理函数
    std::function<S32(const std::vector<U8>&, U32)> handler = nullptr;
    {
        std::lock_guard<std::mutex> guard(m_handlerMutex);
        if (frameType < MAX_FRAME_TYPE) {
            handler = m_frameHandlers[frameType];
        }
    }
    
    if (handler) {
        try {
            handler(frame, len);
        } catch (const std::exception& e) {
            std::cerr << "Exception in frame handler for type 0x" 
                      << std::hex << static_cast<int>(frameType) 
                      << std::dec << ": " << e.what() << std::endl;
        }
    } else {
        std::cerr << "Unknown frame type: 0x" 
                  << std::hex << static_cast<int>(frameType) << std::dec << std::endl;
    }
}
