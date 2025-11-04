#pragma once

#include <string>
#include <functional>
#include <atomic>
#include <queue>
// 使用 C++ 类型别名
typedef uint8_t   U8;
typedef uint32_t  U32;
typedef int32_t   S32;

enum class CommunicatorResult {
    SUCCESS,
    FAILED,
    TIMEOUT,
    NOT_OPEN,
    INVALID_PARAM
};

// 通信抽象接口类
class ICommunicator {
public:
    virtual ~ICommunicator() = default;
    
    // 连接与断开
    virtual bool connect(const std::string& address, int portOrBaud, int timeoutMS) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const { return m_isOpen.load(); };
    
    // 数据收发
    virtual S32 readBuffer(std::vector<U8>& data, S32 length, S32 timeoutMS = 1000) = 0;
    virtual S32 writeBuffer(const std::vector<U8>& data) = 0;
    
    // 发送命令
    virtual void sendCommand(const std::vector<U8>& buffer) {writeBuffer(buffer);};
    
    // 设置回调函数
    virtual void setDataReceivedCallback(std::function<S32(const std::vector<U8>&, S32)> callback) = 0;
    virtual void setDataSentCallback(std::function<void(const std::vector<U8>&, S32)> callback) = 0;

    std::atomic<bool> m_isOpen{ false }; // 通信是否打开
};