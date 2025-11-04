#include "emat_communicater.h"
#include <QDebug>


// C++实现的帧处理回调函数

    S32 ProcParam(const std::vector<U8>& frame, U32 len)
    {
        auto cmd=EmatCommunicater::instance().m_commandQueue.front();
        qDebug() << "ProcParam";
        if(frame[1]==0x55){
            //读取参数
            if(frame[2]==0xFF){
                qDebug() << "ProcParam Read All Params";
                for (int i = 0; i < PARAM_SIZE; i++)
                {
                    // EmatCommunicater::instance().mDeviceParam.arrParam[i].index = i;
                    EmatCommunicater::instance().mDeviceParam.arrParam[i].value = frame[2*i+3]<<8 | frame[2*i+4];
                    qDebug() << "Param Index:"<<i<<" Value:"<<EmatCommunicater::instance().mDeviceParam.arrParam[i].value;
                }
                if(cmd[4]==0x11&&cmd[5]==0x55&&cmd[6]==frame[2]){
                    EmatCommunicater::instance().m_commandQueue.pop();
                }
            }
            else{
                qDebug() << "ProcParam Read Param Index:"<<int(frame[2])<<" Value:"<<(frame[3]<<8 | frame[4]);
            }

        }
        else if(frame[1]==0xAA){
            if(cmd[4]==0x11&&cmd[5]==0xAA&&cmd[6]==frame[2]){
                EmatCommunicater::instance().m_commandQueue.pop();
            }
            //写入参数
        }
        else if(frame[1]==0x5A){
            //重置参数
        }
        return 1; // 成功处理
    }
    S32 ProcWave(const std::vector<U8>& frame, U32 len)
    {
        // qDebug() << "ProcWave";
        //信息帧
        if(frame[1]==0x00){
            qDebug() << "ProcWave Info Frame";
            auto cmd=EmatCommunicater::instance().m_commandQueue.front();
            if(cmd[4]==0x22){
                EmatCommunicater::instance().m_commandQueue.pop();
            }
            EmatCommunicater::instance().WaveData.thick = float(frame[6]<<8 | frame[7])/1000.0f;
            EmatCommunicater::instance().WaveData.wave_pos_first = float(frame[8]<<8 | frame[9])/100.0f;
            EmatCommunicater::instance().WaveData.wave_pos_second = float(frame[10]<<8 | frame[11])/100.0f;
            EmatCommunicater::instance().WaveData.curGain = float(frame[12]<<8 | frame[13])/10.0f;
            EmatCommunicater::instance().WaveData.excitation_freq = float(frame[14]<<8 | frame[15])/100.0f;
            EmatCommunicater::instance().WaveData.measureControlMode = frame[16]<<8 | frame[17];
            // qDebug() << "ProcWave Thick:"<<EmatCommunicater::instance().WaveData.thick
            // <<" wave_pos_first:"<<EmatCommunicater::instance().WaveData.wave_pos_first
            // <<" wave_pos_second:"<<EmatCommunicater::instance().WaveData.wave_pos_second
            // <<" curGain:"<<EmatCommunicater::instance().WaveData.curGain
            // <<" excitation_freq:"<<EmatCommunicater::instance().WaveData.excitation_freq
            // <<" measureControlMode:"<<EmatCommunicater::instance().WaveData.measureControlMode;
        }
        //确认帧
        else if(frame[1]==0xFF){ 
            qDebug() << "ProcWave Ack Frame"<<frame[2];
        }
        //数据帧
        else{
            qDebug() << "ProcWave Data Frame"<<frame[1];
        }
        return 1; // 成功处理
    }

    S32 ProcThkCmd(const std::vector<U8>& frame, U32 len)
    {
        auto cmd=EmatCommunicater::instance().m_commandQueue.front();
		qDebug() << "ProcThkCmd";
        if(frame[1]==0x55 && frame[2]==0x33){
            if(cmd[4]==0x33 && cmd[5]==0x55){
                EmatCommunicater::instance().m_commandQueue.pop();
            }
        }
        else if(frame[1]==0xAA && frame[2]==0x33){
            if(cmd[4]==0x33 && cmd[5]==0xAA){
                EmatCommunicater::instance().m_commandQueue.pop();
            }
            EmatCommunicater::instance().setThickness(0.0);
        }

        return 1; // 成功处理
    }
    
    S32 ProcThickness(const std::vector<U8>& frame, U32 len)
    {
        float thickness = float(frame[2]<<8 | frame[3])/1000.0f;
        qDebug() << "ProcThickness"<<thickness;
        EmatCommunicater::instance().setThickness(thickness);
        return 1; // 成功处理
    }

    S32 ProcElectriCmd(const std::vector<U8>& frame, U32 len)
    {
        qDebug() << "ProcElectriCmd";
        EmatCommunicater::instance().electricValue = INT16(frame[2]<<8 | frame[3]);
        return 1; // 成功处理
    }


// 数据接收回调实现
S32 EmatCommunicater::onDataReceived(const std::vector<U8>& data, S32 length)
{
    if (!data.empty() && length > 0) {
        // 将数据放入命令帧处理器
        m_commandFrame.putFrameData(data, length);
        // 处理帧
        m_commandFrame.processFrame(true); // true表示异步处理
    }
    return 0;
}

// 数据发送回调实现
void EmatCommunicater::onDataSent(const std::vector<U8>& data, S32 length)
{
    static U32 count=0;
    static U32 TotalCount=0;
    TotalCount += length;
    count++; 
    qDebug() << "onDataSent"<<count<<" "<<TotalCount;
}

bool EmatCommunicater::registerFrameHandler(U8 frameType, S32 (*handler)(const std::vector<U8>& frame, U32 len)) {
    AsyncFrameDispatcher::getInstance().registerFrameHandler(frameType, handler);
    return true;
}

void EmatCommunicater::initializeCallbacks()
{
    registerFrameHandler(0x11, ProcParam); // 处理参数帧类型
    registerFrameHandler(0x22, ProcWave); // 处理波形帧类型
    registerFrameHandler(0x33, ProcThkCmd); // 处理电量帧类型
    registerFrameHandler(0x35, ProcThickness); // 处理厚度数据帧类型
    registerFrameHandler(0x41, nullptr); // 时间校准
    registerFrameHandler(0x42, nullptr); // 版本信息
    registerFrameHandler(0x44, ProcElectriCmd); // 电量信息
}

EmatCommunicater& EmatCommunicater::instance() {
    static EmatCommunicater instance;
    return instance;
}

EmatCommunicater::EmatCommunicater() : 
    m_isConnected(false), 
    m_frameBuffer(std::vector<U8>(MAX_RB_LEN, 0)), 
    m_commandFrame(m_frameBuffer) {
    // 初始化异步帧调度器
    AsyncFrameDispatcher::getInstance().init();
    initializeCallbacks();
    init_device_param(mDeviceParam);
}

EmatCommunicater::~EmatCommunicater() {
    //关闭接受线程
    if(recieveThreadRunning) {
        StopReceiveThread();
    }
    //关闭串口
    if(m_communicator && isConnected()) {
        disconnect();
    }

    // 反初始化异步帧调度器
    AsyncFrameDispatcher::getInstance().uninit();
}

bool EmatCommunicater::connect(ConnectionType type, const std::string& comPort, int baudRate, int timeoutMS) {
    // 连接逻辑
    if (m_isConnected) {
        return m_isConnected;
    }
    m_currentConnectionType = type;
    // 使用新的C++接口打开串口
    if(type==ConnectionType::SERIAL) {
        m_communicator = std::make_unique<SerialPort>();
    } else {
        m_communicator = std::make_unique<TcpSocket>();
    }
    m_communicator->setDataReceivedCallback(
        std::bind(&EmatCommunicater::onDataReceived, this, std::placeholders::_1, std::placeholders::_2)
    );
    m_communicator->setDataSentCallback(
        std::bind(&EmatCommunicater::onDataSent, this, std::placeholders::_1, std::placeholders::_2)
    );
    if(!m_communicator->connect(comPort, baudRate, timeoutMS)) {
        return false;
    }
    StartReceiveThread();
    
    m_isConnected = true;
    return m_isConnected;
}

void EmatCommunicater::disconnect() {

    StopReceiveThread();
    // 断开连接逻辑
    m_communicator->disconnect();
    m_isConnected = false;
}

void EmatCommunicater::ProcessReceivedData() {
    // 由于使用了回调函数处理数据，这里的实现可能需要简化或调整
    // 具体取决于项目需求
    // while(recieveThreadRunning) {
    //     m_commandFrame.processFrame(true); // true表示异步处理
    // }
    while(recieveThreadRunning) {
        while(!m_commandQueue.empty()){
            auto cmd = m_commandQueue.front();
            m_communicator->sendCommand(cmd);
            Sleep(50); // 等待50ms,确保命令发送完成
        }
        Sleep(500); // 等待500ms
    }
}

// 启动接收线程（可能需要根据新的设计调整）
void EmatCommunicater::StartReceiveThread() {
    recieveThreadRunning = true;
    if (!receiveThread.joinable()) {
        receiveThread = std::thread(&EmatCommunicater::ProcessReceivedData, this);
        receiveThread.detach();
    }
}

// 停止接收线程
void EmatCommunicater::StopReceiveThread() {
    recieveThreadRunning = false;
    if (receiveThread.joinable()) {
        receiveThread.join();
    }
}

// 发送开始厚度测量指令
void EmatCommunicater::StartThicknessCmd() {
    U16 nSendLen = 0;
    std::vector<U8> nData = {0x33,0x55,0x00,0x00};
     std::vector<U8> Frm(10, 0);
    // 转换命令为帧
    nSendLen = CommandFrame::cmdToFrame(Frm, nData, 4);
    m_commandQueue.push(Frm);
    // 发送命令
    m_communicator->sendCommand(Frm);
}

void EmatCommunicater::StartThickness() {
    if(m_isConnected==false){
        return;
    }
    int count=0;
    StartThicknessCmd();
}

// 发送停止厚度测量指令
void EmatCommunicater::StopThicknessCmd() {
    U16 nSendLen = 0;
    std::vector<U8> nData = {0x33,0xAA,0xA5,0xA5};
    std::vector<U8> Frm(10, 0);
    // 转换命令为帧
    nSendLen = CommandFrame::cmdToFrame(Frm, nData, 4);
    m_commandQueue.push(Frm);
    // 发送命令
    m_communicator->sendCommand(Frm);
}

void EmatCommunicater::StopThickness() {
    if(m_isConnected==false){
        return;
    }
    int count=0;
    StopThicknessCmd();
}

void EmatCommunicater::setThickness(float value) {
    thicknessValue = value;
    // 触发qt信号,通知界面更新
    emit thicknessValueChanged(value);
}

// 获取波形
void EmatCommunicater::GetWave() {
    U16 nSendLen = 0;
    std::vector<U8> nData = {0x22,0x55,0xA5,0xA5,0xA5,0xA5};
    std::vector<U8> Frm(12, 0);
    // 转换命令为帧
    nSendLen = CommandFrame::cmdToFrame(Frm, nData, 6);
    m_commandQueue.push(Frm);
    // 发送命令
    m_communicator->sendCommand(Frm);
}

// 获取电量
void EmatCommunicater::GetElectric() {
    U16 nSendLen = 0;
    std::vector<U8> nData = {0x44,0x00,0x00,0xA5};
    std::vector<U8> Frm(10, 0);
    // 转换命令为帧
    nSendLen = CommandFrame::cmdToFrame(Frm, nData, 4);
    m_commandQueue.push(Frm);
    // 发送命令
    m_communicator->sendCommand(Frm);
}

void EmatCommunicater::GetVersion() {
    U16 nSendLen = 0;
    std::vector<U8> nData = {0x42,0x55,0x00,0xA5};
    std::vector<U8> Frm(10, 0);
    // 转换命令为帧
    nSendLen = CommandFrame::cmdToFrame(Frm, nData, 4);
    m_commandQueue.push(Frm);
    // 发送命令
    m_communicator->sendCommand(Frm);
}

// 重置参数
void EmatCommunicater::ResetParma() {
    U16 nSendLen = 0;
    std::vector<U8> nData = {0x11,0x5A,0xFF,0xA5};
    std::vector<U8> Frm(10, 0);
    // 转换命令为帧
    nSendLen = CommandFrame::cmdToFrame(Frm, nData, 4);
    m_commandQueue.push(Frm);
    // 发送命令
    m_communicator->sendCommand(Frm);
}

//修改单个参数
void EmatCommunicater::SendParam(int index, int value) {
    U16 nSendLen = 0;
    //value写入两个字节
    std::vector<U8> nData = {0x11,0xAA,(U8)index,(U8)(value >> 8),(U8)(value & 0xFF),0x1A};
    std::vector<U8> Frm(12, 0);
    // 转换命令为帧
    nSendLen = CommandFrame::cmdToFrame(Frm, nData, 6);
    m_commandQueue.push(Frm);
    // 发送命令
    m_communicator->sendCommand(Frm);
}

//读取单个参数
void EmatCommunicater::ReadParam(int index){
    U16 nSendLen = 0;
    std::vector<U8> nData = {0x11,0x55,(U8)index,0xA5};
    std::vector<U8> Frm(10, 0);
    // 转换命令为帧
    nSendLen = CommandFrame::cmdToFrame(Frm, nData, 4);
    m_commandQueue.push(Frm);
    // 发送命令
    m_communicator->sendCommand(Frm);
}

//读取所有参数
void EmatCommunicater::GetAllParam() {
    U16 nSendLen = 0;
    std::vector<U8> nData = {0x11,0x55,0xFF,0xA5};
    std::vector<U8> Frm(10, 0);
    // 转换命令为帧
    nSendLen = CommandFrame::cmdToFrame(Frm, nData, 4);
    m_commandQueue.push(Frm);
    // 发送命令
    m_communicator->sendCommand(Frm);
}

void EmatCommunicater::init_device_param(DEVICE_ULTRA_PARAM_U& deviceParam) {

    deviceParam.stParam.aluminiumAlloyGainLimit.index = 0;
    deviceParam.stParam.aluminiumAlloyGainLimit.value = 300;
    
    deviceParam.stParam.zeroPointEnvelope.index = 1;
    deviceParam.stParam.zeroPointEnvelope.value = 2470;

    deviceParam.stParam.ultraSpeed.index = 2;
    deviceParam.stParam.ultraSpeed.value = 32500;

    deviceParam.stParam.zeroPointOrigin.index = 3;
    deviceParam.stParam.zeroPointOrigin.value = 2470;
    
    deviceParam.stParam.titaniumAlloyGainLimit.index = 4;
    deviceParam.stParam.titaniumAlloyGainLimit.value = 420;

    deviceParam.stParam.gate1Start.index = 4;
    deviceParam.stParam.gate1Start.value = 100;

    deviceParam.stParam.gate1End.index = 5;
    deviceParam.stParam.gate1End.value = 180;

    deviceParam.stParam.gate1End.index = 6;
    deviceParam.stParam.gate1End.value = 180;

    deviceParam.stParam.calibrationThick.index = 7;
    deviceParam.stParam.calibrationThick.value = 1200;

    deviceParam.stParam.temperature.index = 8;
    deviceParam.stParam.temperature.value = 25;

    deviceParam.stParam.smoothTime.index = 9;
    deviceParam.stParam.smoothTime.value = 4;
    // deviceParam.stParam.curveSmooth.index = 9;
    // deviceParam.stParam.curveSmooth.value = USING_CURVE_SMOOTH;

    deviceParam.stParam.Gain1.index = 10;
    deviceParam.stParam.Gain1.value = 200;

    // deviceParam.stParam.sensorGain.index = 11;
    // deviceParam.stParam.sensorGain.value = 300;

    deviceParam.stParam.isDirectional.index = 11;
    deviceParam.stParam.isDirectional.value = PROBE_WITH_DIRECTION;

    // deviceParam.stParam.ferqSelect.index = 12;
    // deviceParam.stParam.ferqSelect.value = LOW_FREQUENCY;

    deviceParam.stParam.waveType.index = 12;
    deviceParam.stParam.waveType.value = ORIGIN_WAVE;

    deviceParam.stParam.presetThickness.index = 13;
    deviceParam.stParam.presetThickness.value = 0;

    deviceParam.stParam.compensationEnable.index = 14;
    deviceParam.stParam.compensationEnable.value = 0;

    deviceParam.stParam.pulseNumber.index = 15;
    deviceParam.stParam.pulseNumber.value = 2;

    deviceParam.stParam.excitationFrequency.index = 16;
    deviceParam.stParam.excitationFrequency.value = 500;

    deviceParam.stParam.measureControlMode.index = 17;
    deviceParam.stParam.measureControlMode.value = PP_CONTROL_MODE;

    deviceParam.stParam.manualMeasureMode.index = 18;
    deviceParam.stParam.manualMeasureMode.value = MANUAL_PP_CONTROL_MODE;

    deviceParam.stParam.selfMultiplying.index = 19;
    deviceParam.stParam.selfMultiplying.value = SELF_MULTIPLYING;

    deviceParam.stParam.thicknessSmooth.index = 20;
    deviceParam.stParam.thicknessSmooth.value = 30;

    deviceParam.stParam.IsOnMeasurePos.index = 21;
    deviceParam.stParam.IsOnMeasurePos.value = 0;

    deviceParam.stParam.measureMode.index = 22;
    deviceParam.stParam.measureMode.value = MEASURE_AUTO;

    deviceParam.stParam.isWirelessDownload.index = 23;
    deviceParam.stParam.isWirelessDownload.value = 0;

    deviceParam.stParam.wirelessBaudRate.index = 24;
    deviceParam.stParam.wirelessBaudRate.value = 1152;

    deviceParam.stParam.wirelessMode.index = 25;
    deviceParam.stParam.wirelessMode.value = 0;

    deviceParam.stParam.setWirelessModule.index = 26;
    deviceParam.stParam.setWirelessModule.value = 0;

    deviceParam.stParam.wirelessChannel.index = 27;
    deviceParam.stParam.wirelessChannel.value = 0;

    deviceParam.stParam.startDisplay.index = 28;
    deviceParam.stParam.startDisplay.value = 2;

    deviceParam.stParam.stopDisplay.index = 29;
    deviceParam.stParam.stopDisplay.value = 30;

    deviceParam.stParam.lcdDisplayTxt.index = 30;
    deviceParam.stParam.lcdDisplayTxt.value = DISPLAY_THICKNESS;

    deviceParam.stParam.measureMaterial.index = 31;
    deviceParam.stParam.measureMaterial.value = ALUMINIUM_ALLOY;

    deviceParam.stParam.sendWaveSegment.index = 32;
    deviceParam.stParam.sendWaveSegment.value = 0;

    deviceParam.stParam.communicateMode.index = 33;
    deviceParam.stParam.communicateMode.value = 0;

    // deviceParam.stParam.SNR.index = 15;
    // deviceParam.stParam.SNR.value = 10;

    // deviceParam.stParam.dampingRadio.index = 16;
    // deviceParam.stParam.dampingRadio.value = 10;

    // deviceParam.stParam.envelopeWidth.index = 17;
    // deviceParam.stParam.envelopeWidth.value = 2470;

    // deviceParam.stParam.widthHeightRadio.index = 18;
    // deviceParam.stParam.widthHeightRadio.value = 10;

    // deviceParam.stParam.thresholdValue.index = 19;
    // deviceParam.stParam.thresholdValue.value = 10;

    // deviceParam.stParam.emitInterval.index = 20;
    // deviceParam.stParam.emitInterval.value = 1000;



    // deviceParam.stParam.excitationFrequency.index = 22;
    // deviceParam.stParam.excitationFrequency.value = 500;

    // deviceParam.stParam.startDisplay.index = 23;
    // deviceParam.stParam.startDisplay.value = 2;

    // deviceParam.stParam.stopDisplay.index = 24;
    // deviceParam.stParam.stopDisplay.value = 30;

    // deviceParam.stParam.communicateMode.index = 25;
    // deviceParam.stParam.communicateMode.value = 0;



 

    // deviceParam.stParam.lcdDisplayTxt.index = 28;
    // deviceParam.stParam.lcdDisplayTxt.value = DISPLAY_THICKNESS;



    // deviceParam.stParam.lcdDisplayTxt.index = 30;
    // deviceParam.stParam.thicknessSmooth.value = 30;

    // deviceParam.stParam.lcdDisplayTxt.index = 31;
    // deviceParam.stParam.thicknessSmooth.value = TITANIUM_ALLOY;

    qDebug() << "Device parameters initialized.";
}

bool EmatCommunicater::isConnected() const {
    return m_isConnected;
}