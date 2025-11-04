#include "SerialPort.h"
#include <iostream>
#include <sstream>
#include <cstring>

// 内部常量定义
constexpr S32 SERIALPORT_INTERNAL_TIMEOUT = 1;  // 内部读写超时时间，单位毫秒
constexpr DWORD MAX_BUFFER_SIZE = 256;          // 内部读写缓冲区大小

// 构造函数
SerialPort::SerialPort(U32 recvBufSize, U32 sendBufSize) 
    : m_portHandle(nullptr),
      m_workingThread(nullptr),
      m_eventRead(nullptr),
      m_eventWrite(nullptr),
      m_workingThreadId(0),
      m_timeoutMilliSeconds(0),
      m_totalByteCount(0) {
    
    // 初始化重叠IO结构
    ZeroMemory(&m_overlappedRead, sizeof(OVERLAPPED));
    ZeroMemory(&m_overlappedWrite, sizeof(OVERLAPPED));
    
    // 创建缓冲区
    if (recvBufSize > 0) {
        m_recvBuffer = FrameBuffer::create(recvBufSize);
    }
    if (sendBufSize > 0) {
        m_sendBuffer = FrameBuffer::create(sendBufSize);
    }
}

// 析构函数
SerialPort::~SerialPort() {
    close();

}

// // 移动构造函数
// SerialPort::SerialPort(SerialPort&& other) noexcept 
//     : m_portHandle(other.m_portHandle),
//       m_workingThread(other.m_workingThread),
//       m_eventRead(other.m_eventRead),
//       m_eventWrite(other.m_eventWrite),
//       m_workingThreadId(other.m_workingThreadId),
//       m_timeoutMilliSeconds(other.m_timeoutMilliSeconds),
//       m_isOpen(other.m_isOpen.load()),
//       m_totalByteCount(other.m_totalByteCount.load()),
//       m_onDataReceived(std::move(other.m_onDataReceived)),
//       m_onDataSent(std::move(other.m_onDataSent)),
//       m_recvBuffer(std::move(other.m_recvBuffer)),
//       m_sendBuffer(std::move(other.m_sendBuffer)) {
    
//     // 初始化原对象的句柄
//     other.m_portHandle = nullptr;
//     other.m_workingThread = nullptr;
//     other.m_eventRead = nullptr;
//     other.m_eventWrite = nullptr;
//     other.m_workingThreadId = 0;
//     other.m_isOpen.store(false);
//     other.m_totalByteCount.store(0);
    
//     // 重新设置重叠IO结构的事件句柄
//     m_overlappedRead.hEvent = m_eventRead;
//     m_overlappedWrite.hEvent = m_eventWrite;
// }

// // 移动赋值操作符
// SerialPort& SerialPort::operator=(SerialPort&& other) noexcept {
//     if (this != &other) {
//         // 关闭当前串口
//         close();
        
//         // 移动资源
//         m_portHandle = other.m_portHandle;
//         m_workingThread = other.m_workingThread;
//         m_eventRead = other.m_eventRead;
//         m_eventWrite = other.m_eventWrite;
//         m_workingThreadId = other.m_workingThreadId;
//         m_timeoutMilliSeconds = other.m_timeoutMilliSeconds;
//         m_isOpen.store(other.m_isOpen.load());
//         m_totalByteCount.store(other.m_totalByteCount.load());
//         m_onDataReceived = std::move(other.m_onDataReceived);
//         m_onDataSent = std::move(other.m_onDataSent);
//         m_recvBuffer = std::move(other.m_recvBuffer);
//         m_sendBuffer = std::move(other.m_sendBuffer);
        
//         // 初始化原对象的句柄
//         other.m_portHandle = nullptr;
//         other.m_workingThread = nullptr;
//         other.m_eventRead = nullptr;
//         other.m_eventWrite = nullptr;
//         other.m_workingThreadId = 0;
//         other.m_isOpen.store(false);
//         other.m_totalByteCount.store(0);
        
//         // 重新设置重叠IO结构的事件句柄
//         m_overlappedRead.hEvent = m_eventRead;
//         m_overlappedWrite.hEvent = m_eventWrite;
//     }
//     return *this;
// }

// 异步方式打开串口
CommunicatorResult SerialPort::openAsync(std::string comPort,
                                      S32 baudRate,
                                      std::function<S32(const std::vector<U8>&, S32)> onRecv,
                                      std::function<void(const std::vector<U8>&, S32)> onSent,
                                      S32 timeoutMS) {
    CommunicatorResult result = open(comPort, baudRate, timeoutMS);
    
    if (result == CommunicatorResult::SUCCESS) {
        m_onDataReceived = std::move(onRecv);
        m_onDataSent = std::move(onSent);
        
        // 创建工作线程
        m_workingThread = CreateThread(nullptr, 0, waitForDataThread, this, 0, &m_workingThreadId);
        if (m_workingThread == nullptr) {
            std::cerr << "Error: Failed to create serial port working thread" << std::endl;
            close();
            return CommunicatorResult::FAILED;
        }
    }
    
    return result;
}

// 同步方式打开串口
CommunicatorResult SerialPort::open(std::string comPort, S32 baudRate, S32 timeoutMS) {
    // 参数校验
    if (comPort.empty() || timeoutMS > 15000) {
        return CommunicatorResult::INVALID_PARAM;
    }
    
    // 关闭已打开的串口
    close();
    
    // 构建串口名称
    std::ostringstream portNameStream;
    portNameStream << "\\\\.\\" << comPort;
    std::string portName = portNameStream.str();
    
    // 打开串口
    m_portHandle = CreateFileA(
        portName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,  // 异步模式
        nullptr
    );
    
    if (m_portHandle == nullptr || m_portHandle == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Cannot open COM port " << portName << std::endl;
        m_portHandle = nullptr;
        return CommunicatorResult::FAILED;
    }
    
    // 设置超时时间
    m_timeoutMilliSeconds = timeoutMS;
    
    // 设置缓冲区大小
    SetupComm(m_portHandle, 40960, 40960);
    PurgeComm(m_portHandle, PURGE_RXCLEAR | PURGE_TXCLEAR);  // 清空缓冲区
    
    // 配置串口参数
    DCB portSettings = {0};
    portSettings.DCBlength = sizeof(DCB);
    if (!GetCommState(m_portHandle, &portSettings)) {
        std::cerr << "Error: Failed to get serial port state" << std::endl;
        close();
        return CommunicatorResult::FAILED;
    }
    
    portSettings.BaudRate = baudRate;
    portSettings.ByteSize = 8;
    portSettings.Parity = NOPARITY;
    portSettings.StopBits = ONESTOPBIT;
    
    if (!SetCommState(m_portHandle, &portSettings)) {
        std::cerr << "Error: Failed to set serial port state" << std::endl;
        close();
        return CommunicatorResult::FAILED;
    }
    
    // 配置超时
    COMMTIMEOUTS portTimeOuts = {0};
    portTimeOuts.ReadIntervalTimeout = MAXDWORD;  // 非阻塞读
    portTimeOuts.ReadTotalTimeoutMultiplier = 0;
    portTimeOuts.ReadTotalTimeoutConstant = 0;    // 整体超时由参数决定
    
    if (!SetCommTimeouts(m_portHandle, &portTimeOuts)) {
        std::cerr << "Error: Failed to set serial port timeouts" << std::endl;
        close();
        return CommunicatorResult::FAILED;
    }
    
    // 创建事件句柄
    m_eventRead = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    m_eventWrite = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    
    if (m_eventRead == nullptr || m_eventWrite == nullptr) {
        std::cerr << "Error: Failed to create serial port events" << std::endl;
        close();
        return CommunicatorResult::FAILED;
    }
    
    // 设置重叠IO结构的事件句柄
    ZeroMemory(&m_overlappedRead, sizeof(OVERLAPPED));
    ZeroMemory(&m_overlappedWrite, sizeof(OVERLAPPED));
    m_overlappedRead.hEvent = m_eventRead;
    m_overlappedWrite.hEvent = m_eventWrite;
    
    // 标记串口已打开
    m_isOpen.store(true);
    
    return CommunicatorResult::SUCCESS;
}

// 关闭串口
void SerialPort::close() {
    // 标记串口为关闭状态
    m_isOpen.store(false);
    
    // 关闭串口句柄
    if (m_portHandle != nullptr) {
        CloseHandle(m_portHandle);
        m_portHandle = nullptr;
        Sleep(SERIALPORT_INTERNAL_TIMEOUT * 4);
    }
    
    // 关闭事件句柄
    if (m_eventRead != nullptr) {
        CloseHandle(m_eventRead);
        m_eventRead = nullptr;
    }
    if (m_eventWrite != nullptr) {
        CloseHandle(m_eventWrite);
        m_eventWrite = nullptr;
    }
    
    // 等待工作线程退出
    if (m_workingThread != nullptr) {
        WaitForSingleObject(m_workingThread, 2000);
        CloseHandle(m_workingThread);
        m_workingThread = nullptr;
        m_workingThreadId = 0;
    }
    
    // 清除回调函数
    m_onDataReceived = nullptr;
    m_onDataSent = nullptr;
        
    // 输出统计信息
    std::cout << "Info: Total " << m_totalByteCount.load() << " bytes" << std::endl;
}

// 内部写数据函数
S32 SerialPort::writeBufferInternal(const std::vector<U8>& data, S32 length) {
    if (!m_isOpen || m_portHandle == nullptr || data.empty() || length <= 0) {
        return -1;
    }
    
    DWORD bytesWritten = 0;
    BOOL result = WriteFile(
        m_portHandle,
        data.data(),
        length,
        &bytesWritten,
        &m_overlappedWrite
    );
    
    // 处理异步操作
    if (!result && GetLastError() == ERROR_IO_PENDING) {
        WaitForSingleObject(m_eventWrite, INFINITE);
        GetOverlappedResult(m_portHandle, &m_overlappedWrite, &bytesWritten, FALSE);
    }
    
    // 触发发送完成回调
    if (bytesWritten > 0 && m_onDataSent) {
        m_onDataSent(data, bytesWritten);
    }
    
    return static_cast<S32>(bytesWritten);
}

// 内部读数据函数
S32 SerialPort::readBufferInternal(std::vector<U8>& data, S32 length, S32 timeoutMS) {
    if (!m_isOpen || m_portHandle == nullptr || data.empty() || length <= 0) {
        return -1;
    }
    
    DWORD bytesRead = 0;
    BOOL result = ReadFile(
        m_portHandle,
        data.data(),
        length,
        &bytesRead,
        &m_overlappedRead
    );
    
    // 处理异步操作
    if (!result && GetLastError() == ERROR_IO_PENDING) {
        DWORD waitResult = WaitForSingleObject(m_eventRead, timeoutMS);
        if (waitResult == WAIT_OBJECT_0) {
            GetOverlappedResult(m_portHandle, &m_overlappedRead, &bytesRead, FALSE);
        } else if (waitResult == WAIT_TIMEOUT) {
            return -2;  // 超时
        }
    }
    
    return static_cast<S32>(bytesRead);
}

// 读数据接口
S32 SerialPort::readBuffer(std::vector<U8>& data, S32 length, S32 timeoutMS) {
    if (m_recvBuffer && m_recvBuffer->getBytesCount() > 0) {
        // 如果接收缓冲区有数据，优先从缓冲区读取
        return m_recvBuffer->get(data.data(), length);
    }
    
    // 否则直接从串口读取
    return readBufferInternal(data, length, timeoutMS);
}

// 写数据接口
S32 SerialPort::writeBuffer(const std::vector<U8>& data) {
    return writeBufferInternal(data, data.size());
}

// 发送命令应答
// void SerialPort::sendCommand(const U8* buffer, S32 byteLen) {
//     writeBuffer(buffer, byteLen);
//     // // 如果有发送缓冲区，则使用缓冲区
//     // if (m_sendBuffer) {
//     //     m_sendBuffer->put(buffer, byteLen);
//     //     // 注意：这里只是将数据放入缓冲区，实际发送需要另外的处理逻辑
//     //     // 可以添加一个发送线程来处理缓冲区数据的发送
//     //     std::thread([this](const U8* buffer, S32 byteLen) {
//     //         writeBuffer(buffer, byteLen);
//     //     }, buffer, byteLen).detach();
//     // } else {
//     //     // 否则直接发送
//     //     writeBuffer(buffer, byteLen);
//     // }
// }

// 异步数据接收线程函数
DWORD WINAPI SerialPort::waitForDataThread(LPVOID lpParam) {
    SerialPort* sp = static_cast<SerialPort*>(lpParam);
    U8 buffer[MAX_BUFFER_SIZE];
    DWORD bytesRead = 0;
    
    while (sp->m_isOpen.load()) {
        // 重置事件
        ResetEvent(sp->m_eventRead);
        
        // 异步读取数据
        BOOL result = ReadFile(
            sp->m_portHandle,
            buffer,
            sizeof(buffer),
            &bytesRead,
            &sp->m_overlappedRead
        );
        
        // 处理异步操作
        if (!result && GetLastError() == ERROR_IO_PENDING) {
            DWORD waitResult = WaitForSingleObject(sp->m_eventRead, sp->m_timeoutMilliSeconds);
            if (waitResult == WAIT_OBJECT_0) {
                GetOverlappedResult(sp->m_portHandle, &sp->m_overlappedRead, &bytesRead, FALSE);
            } else if (waitResult == WAIT_TIMEOUT) {
                continue;  // 超时，继续下一次读取
            }
        }
        
        // 更新统计计数
        sp->m_totalByteCount.fetch_add(bytesRead);
        
        // 数据放入接收缓冲区
        if (bytesRead > 0 && sp->m_recvBuffer) {
            sp->m_recvBuffer->put(buffer, bytesRead);
        }
        
        // 触发数据接收回调
        if (bytesRead > 0 && sp->m_onDataReceived) {
            sp->m_onDataReceived(std::vector<U8>(buffer, buffer + bytesRead), bytesRead);
        }
    }
    
    return 0;
}