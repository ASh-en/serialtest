#include "TcpSocket.h"
#include <iostream>

// 静态成员初始化
bool TcpSocket::initializeWinsock() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    return (result == 0);
}

void TcpSocket::cleanupWinsock() {
    WSACleanup();
}

TcpSocket::TcpSocket(U32 recvBufSize, U32 sendBufSize) 
    : m_socket(INVALID_SOCKET),
      m_workingThread(nullptr),
      m_workingThreadId(0),
      m_timeoutMilliSeconds(1000),
      m_totalByteCount(0) {
    // 创建缓冲区
    if (recvBufSize > 0) {
        m_recvBuffer = FrameBuffer::create(recvBufSize);
    }
    if (sendBufSize > 0) {
        m_sendBuffer = FrameBuffer::create(sendBufSize);
    }
    // 确保Winsock已初始化
    static bool winsockInitialized = initializeWinsock();
}

TcpSocket::~TcpSocket() {
    disconnect();
}

TcpSocket::TcpSocket(TcpSocket&& other) noexcept 
    : m_socket(other.m_socket),
      m_workingThread(other.m_workingThread),
      m_workingThreadId(other.m_workingThreadId),
      m_timeoutMilliSeconds(other.m_timeoutMilliSeconds),
      m_totalByteCount(other.m_totalByteCount.load()),
      m_onDataReceived(std::move(other.m_onDataReceived)),
      m_onDataSent(std::move(other.m_onDataSent)),
      m_recvBuffer(std::move(other.m_recvBuffer)),
      m_sendBuffer(std::move(other.m_sendBuffer)) {
    
    // 重置源对象
    other.m_socket = INVALID_SOCKET;
    other.m_workingThread = nullptr;
    other.m_workingThreadId = 0;
    other.m_isOpen = false;
}

TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept {
    if (this != &other) {
        disconnect();
        
        m_socket = other.m_socket;
        m_workingThread = other.m_workingThread;
        m_workingThreadId = other.m_workingThreadId;
        m_timeoutMilliSeconds = other.m_timeoutMilliSeconds;
        m_isOpen = other.m_isOpen.load();
        m_totalByteCount = other.m_totalByteCount.load();
        m_onDataReceived = std::move(other.m_onDataReceived);
        m_onDataSent = std::move(other.m_onDataSent);
        m_recvBuffer = std::move(other.m_recvBuffer);
        m_sendBuffer = std::move(other.m_sendBuffer);
        
        // 重置源对象
        other.m_socket = INVALID_SOCKET;
        other.m_workingThread = nullptr;
        other.m_workingThreadId = 0;
        other.m_isOpen = false;
    }
    return *this;
}

bool TcpSocket::connect(const std::string& address, int port, int timeoutMS) {
    if (m_isOpen) {
        return true;
    }
    
    // 创建套接字
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        return false;
    }
    
    // 设置套接字为非阻塞
    u_long mode = 1;
    if (ioctlsocket(m_socket, FIONBIO, &mode) != 0) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }
    
    // 设置服务器地址
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, address.c_str(), &(serverAddr.sin_addr));
    
    // 连接服务器
    int result = ::connect(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (result == SOCKET_ERROR) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            return false;
        }
        
        // 使用select等待连接完成
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(m_socket, &writeSet);
        
        timeval timeout;
        timeout.tv_sec = timeoutMS / 1000;
        timeout.tv_usec = (timeoutMS % 1000) * 1000;
        
        result = select(0, NULL, &writeSet, NULL, &timeout);
        if (result <= 0) {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            return false;
        }
        
        // 检查连接是否成功
        int optVal;
        int optLen = sizeof(optVal);
        getsockopt(m_socket, SOL_SOCKET, SO_ERROR, (char*)&optVal, &optLen);
        if (optVal != 0) {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            return false;
        }
    }
    
    // 恢复为阻塞模式
    mode = 0;
    ioctlsocket(m_socket, FIONBIO, &mode);
    
    m_timeoutMilliSeconds = timeoutMS;
    m_isOpen = true;
    
    // 启动接收线程
    m_workingThread = CreateThread(NULL, 0, waitForDataThread, this, 0, &m_workingThreadId);
    
    return true;
}

void TcpSocket::disconnect() {
    if (m_isOpen) {
        m_isOpen = false;
        
        if (m_workingThread) {
            WaitForSingleObject(m_workingThread, INFINITE);
            CloseHandle(m_workingThread);
            m_workingThread = nullptr;
        }
        
        if (m_socket != INVALID_SOCKET) {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
        }
    }
}

S32 TcpSocket::readBufferInternal(std::vector<U8>& data, S32 length, S32 timeoutMS) {
    if (!m_isOpen || m_socket == INVALID_SOCKET || data.empty() || length <= 0) {
        return -1;
    }
    
    // 设置接收超时
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(m_socket, &readSet);
    
    timeval timeout;
    timeout.tv_sec = timeoutMS / 1000;
    timeout.tv_usec = (timeoutMS % 1000) * 1000;
    
    int result = select(0, &readSet, NULL, NULL, &timeout);
    if (result <= 0) {
        return (result == 0) ? 0 : -1; // 超时或错误
    }
    
    // 接收数据
    int bytesRead = recv(m_socket, (char*)data.data(), length, 0);
    if (bytesRead > 0) {
        m_totalByteCount += bytesRead;
    }
    
    return bytesRead;
}

S32 TcpSocket::writeBufferInternal(const std::vector<U8>& data) {
    if (!m_isOpen || m_socket == INVALID_SOCKET || data.empty()) {
        return -1;
    }
    
    // 发送数据
    int bytesSent = send(m_socket, (const char*)data.data(), data.size(), 0);
    if (bytesSent > 0) {
        m_totalByteCount += bytesSent;
        
        // 触发回调
        if (m_onDataSent) {
            m_onDataSent(data, bytesSent);
        }
    }
    
    return bytesSent;
}

S32 TcpSocket::readBuffer(std::vector<U8>& data, S32 length, S32 timeoutMS) {
    return readBufferInternal(data, length, timeoutMS);
}

S32 TcpSocket::writeBuffer(const std::vector<U8>& data) {
    return writeBufferInternal(data);
}

void TcpSocket::setDataReceivedCallback(std::function<S32(const std::vector<U8>&, S32)> callback) {
    m_onDataReceived = callback;
}

void TcpSocket::setDataSentCallback(std::function<void(const std::vector<U8>&, S32)> callback) {
    m_onDataSent = callback;
}

DWORD WINAPI TcpSocket::waitForDataThread(LPVOID lpParam) {
    TcpSocket* pThis = static_cast<TcpSocket*>(lpParam);
    std::vector<U8> buffer(1024);
    
    while (pThis->m_isOpen) {
        S32 bytesRead = pThis->readBufferInternal(buffer, sizeof(buffer), 100);
        if (bytesRead > 0 && pThis->m_onDataReceived) {
            pThis->m_onDataReceived(buffer, bytesRead);
        }
        
        // 短暂睡眠，减少CPU占用
        Sleep(1);
    }
    
    return 0;
}