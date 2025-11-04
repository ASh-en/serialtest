#ifndef __TCP_SOCKET_H__
#define __TCP_SOCKET_H__

#include "ICommunicator.h"
#include "FrmBuf.h"
#include <string>
#include <functional>
#include <mutex>
#include <atomic>
#include <memory>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

class TcpSocket : public ICommunicator {
private:
    SOCKET m_socket;                                     // TCP套接字句柄
    HANDLE m_workingThread;                              // 工作线程句柄
    DWORD m_workingThreadId;                             // 工作线程ID
    S32 m_timeoutMilliSeconds;                           // 读写超时时间
    // std::atomic<bool> m_isOpen;                          // 套接字是否打开
    std::atomic<U32> m_totalByteCount;                   // 统计总字节数
    
    // 回调函数
    std::function<S32(const std::vector<U8>&, S32)> m_onDataReceived; // 数据接收回调
    std::function<void(const std::vector<U8>&, S32)> m_onDataSent;    // 数据发送回调
    
    // 缓冲区
    std::unique_ptr<FrameBuffer> m_recvBuffer;           // 接收缓冲区
    std::unique_ptr<FrameBuffer> m_sendBuffer;           // 发送缓冲区
    
    // 内部读写函数
    S32 writeBufferInternal(const std::vector<U8>& data);
    S32 readBufferInternal(std::vector<U8>& data, S32 length, S32 timeoutMS);
    
    // 工作线程函数
    static DWORD WINAPI waitForDataThread(LPVOID lpParam);

public:
    // 构造函数
    TcpSocket(U32 recvBufSize = 4096, U32 sendBufSize = 4096);
    
    // 析构函数
    ~TcpSocket();
    
    // 禁止拷贝构造和赋值操作
    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;
    
    // 允许移动构造和移动赋值
    TcpSocket(TcpSocket&&) noexcept;
    TcpSocket& operator=(TcpSocket&&) noexcept;
    
    // 实现ICommunicator接口
    bool connect(const std::string& address, int port, int timeoutMS = 1000) override;
    void disconnect() override;
    bool isConnected() const override { return m_isOpen.load(); }
    
    S32 readBuffer(std::vector<U8>& data, S32 length, S32 timeoutMS = 1000) override;
    S32 writeBuffer(const std::vector<U8>& data) override;
    
    
    void setDataReceivedCallback(std::function<S32(const std::vector<U8>&, S32)> callback) override;
    void setDataSentCallback(std::function<void(const std::vector<U8>&, S32)> callback) override;
    
    // 初始化Winsock
    static bool initializeWinsock();
    static void cleanupWinsock();
};

#endif /* __TCP_SOCKET_H__ */