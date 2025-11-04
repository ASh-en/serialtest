#ifndef __SERIAL_PORT_H__
#define __SERIAL_PORT_H__

#include <string>
#include <functional>
#include <mutex>
#include <windows.h>
#include "FrmBuf.h"
#include "ICommunicator.h" // 添加抽象接口
#include <memory>
// 使用 C++ 类型别名
// typedef uint8_t   U8;
// typedef uint32_t  U32;
// typedef int32_t   S32;

class SerialPort : public ICommunicator { // 实现抽象接口
private:
    HANDLE m_portHandle;                                  // 串口句柄
    HANDLE m_workingThread;                               // 工作线程句柄
    HANDLE m_eventRead;                                   // 读取事件
    HANDLE m_eventWrite;                                  // 写入事件
    DWORD m_workingThreadId;                              // 工作线程ID
    S32 m_timeoutMilliSeconds;                            // 读写超时时间
    OVERLAPPED m_overlappedRead;                          // 重叠IO结构（读）
    OVERLAPPED m_overlappedWrite;                         // 重叠IO结构（写）
    // std::atomic<bool> m_isOpen;                           // 串口是否打开
    std::atomic<U32> m_totalByteCount;                    // 统计总字节数
    
    // 回调函数
    std::function<S32(const std::vector<U8>&, S32)> m_onDataReceived;  // 数据接收回调
    std::function<void(const std::vector<U8>&, S32)> m_onDataSent;     // 数据发送回调
    
    // 缓冲区
    std::unique_ptr<FrameBuffer> m_recvBuffer;            // 接收缓冲区
    std::unique_ptr<FrameBuffer> m_sendBuffer;            // 发送缓冲区
    
    // 内部读写函数
    S32 writeBufferInternal(const std::vector<U8>& data, S32 length);
    S32 readBufferInternal(std::vector<U8>& data, S32 length, S32 timeoutMS);
    
    // 工作线程函数
    static DWORD WINAPI waitForDataThread(LPVOID lpParam);

        // 打开与关闭串口
    CommunicatorResult openAsync(std::string comPort,
                               S32 baudRate,
                               std::function<S32(const std::vector<U8>&, S32)> onRecv,
                               std::function<void(const std::vector<U8>&, S32)> onSent = nullptr,
                               S32 timeoutMS = 1000);
    
    CommunicatorResult open(std::string comPort, S32 baudRate, S32 timeoutMS = 1000);
    void close();

public:
    // 构造函数
    SerialPort(U32 recvBufSize = 4096, U32 sendBufSize = 4096);
    
    // 析构函数
    ~SerialPort();
    
    // 禁止拷贝构造和赋值操作
    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;
    
    // 允许移动构造和移动赋值
    SerialPort(SerialPort&&) noexcept;
    SerialPort& operator=(SerialPort&&) noexcept;
    
    // 实现ICommunicator接口
    bool connect(const std::string& address, int baudRate,int timeoutMS = 1000) override {
        auto result = openAsync(address, baudRate, m_onDataReceived, m_onDataSent, timeoutMS);
        return result == CommunicatorResult::SUCCESS;
    }
    
    void disconnect() override { close(); }
    
    S32 readBuffer(std::vector<U8>& data, S32 length, S32 timeoutMS = 1000) override;
    S32 writeBuffer(const std::vector<U8>& data) override;
    
    // void sendCommand(const U8* buffer, S32 byteLen) override;
    
    void setDataReceivedCallback(std::function<S32(const std::vector<U8>&, S32)> callback) override {
        m_onDataReceived = callback;
    }
    
    void setDataSentCallback(std::function<void(const std::vector<U8>&, S32)> callback) override {
        m_onDataSent = callback;
    }
    
    // 状态查询
    U32 getTotalByteCount() const { return m_totalByteCount.load(); }
};

#endif /* __SERIAL_PORT_H__ */