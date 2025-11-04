#pragma once

#include "SerialPort.h"
#include "TcpSocket.h" // 添加TCP支持
#include "ICommunicator.h" // 添加抽象接口
#include "AsyncFrame.h"
#include "CmdFrm.h"
#include <QObject>
#include "paramDefine.h"
#include <thread>
#include <memory>


// 使用 C++ using 别名替代 typedef
using U8 = uint8_t;
using U16 = uint16_t;
using U32 = uint32_t;
using S32 = int32_t;

// constexpr U32 MAX_RB_LEN = 0x0400;             // 环形缓存区长度
// 连接类型枚举
enum class ConnectionType {
    SERIAL,
    TCP
};

class EmatCommunicater : public QObject
{
    Q_OBJECT
public:
    // 获取单例实例
    static EmatCommunicater& instance();

    // 删除拷贝构造和赋值运算符，防止复制
    EmatCommunicater(const EmatCommunicater&) = delete;
    EmatCommunicater& operator=(const EmatCommunicater&) = delete;

    void initializeCallbacks();
    bool registerFrameHandler(U8 frameType, S32 (*handler)(const std::vector<U8>& frame, U32 len));

    // 修改连接方法，支持选择连接类型
    bool connect(ConnectionType type, const std::string& address, int portOrBaud, int timeoutMS);
    void disconnect();
    bool isConnected() const;

    void StartThicknessCmd();
    void StopThicknessCmd();

    void StartThickness();
    void StopThickness();

    void GetWave();
    void ResetParma();
    void ReadParam(int index);
    void GetElectric();
    void SendParam(int index, int value);
    void GetAllParam();
    
    void GetVersion();
    void ProcessReceivedData();
    void Sendcmd(const U8* pData, S32 dataLength);
    void setThickness(float value);
    float getThickness() const { return thicknessValue; }
    
    void StartReceiveThread();
    void StopReceiveThread();

    INT16 electricValue = 50; // 当前电量值

    DEVICE_ULTRA_PARAM_U mDeviceParam;//设备参数

    DISPLAY_WAVE_DATA WaveData;//波形数据

    // 数据接收回调函数
    S32 onDataReceived(const std::vector<U8>& data, S32 length);

    // 数据发送回调函数
    void onDataSent(const std::vector<U8>& data, S32 length);

    // 命令帧处理器
    CommandFrame m_commandFrame;

    std::queue<std::vector<U8>> m_commandQueue;

public slots:
    // 可以添加槽函数来响应信号

private slots:
    // 内部槽函数

public:
    signals:
        void thicknessValueChanged(float value);
        void electricValueChanged(INT16 value);


private:
    // S32 ProcThickness(const U8* frame, U32 len);
    // S32 ProcThkCmd(const U8* frame, U32 len);
    // S32 ProcWave(const U8* frame, U32 len);
    // S32 ProcParam(const U8* frame, U32 len);
    // S32 ProcElectriCmd(const U8* frame, U32 len);   

    // 私有构造函数
    EmatCommunicater();
    ~EmatCommunicater();
    void init_device_param(DEVICE_ULTRA_PARAM_U& deviceParam);
    bool recieveThreadRunning = false;
    std::thread receiveThread;
    std::unique_ptr<ICommunicator> m_communicator; // 使用抽象接口指针代替具体实现
    float thicknessValue = 0.0f; // 当前厚度值
    // 通信接口相关成员
    bool m_isConnected = false;
    ConnectionType m_currentConnectionType = ConnectionType::SERIAL; // 当前连接类型
    // 用于命令帧处理的缓冲区
    FrameBuffer m_frameBuffer;
};