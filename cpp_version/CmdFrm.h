#ifndef __COMMAND_FRAME_H__
#define __COMMAND_FRAME_H__

#include "FrmBuf.h"
#include <cstdint>
#include <vector>


// 使用 C++ using 别名替代 typedef
using U8 = uint8_t;
using U16 = uint16_t;
using U32 = uint32_t;
using S32 = int32_t;

// 外部声明，用于统计帧数
extern U32 frameCount;

// 使用 constexpr 替代枚举常量
constexpr U8 FRAME_HEAD = 0x66;                // 帧头
constexpr U8 FRAME_END = 0x99;                 // 帧尾
constexpr U32 MAX_RB_LEN = 0x2000;             // 环形缓存区长度
// constexpr U8 MAX_CMD_LEN = 0x65;               // 最大业务命令长度
constexpr U8 MIN_CMD_LEN = 0x04;               // 最小业务命令长度
constexpr U8 uFRAME_HEAD_LEN = 4;              // 传输帧头长度（1字节帧头+1字节设备号+2字节长度）
constexpr U8 uFRAME_END_LEN = 2;               // 传输帧尾长度（1字节校验码+1字节帧尾）
constexpr U8 uFRAME_DEV_IDX = 1;               // 设备号序号
constexpr U8 uFRAME_LEN_H_IDX = 2;             // 长度高位序号
constexpr U8 uFRAME_LEN_L_IDX = 3;             // 长度低位序号
constexpr U8 uFRAME_HE_ND_LEN = uFRAME_HEAD_LEN + uFRAME_END_LEN; // 传输帧结构长度
constexpr U8 uFRAME_MIN_LEN = uFRAME_HEAD_LEN + MIN_CMD_LEN + uFRAME_END_LEN; // 传输帧最小长度
constexpr U16 uFRAME_MAX_LEN = uFRAME_HEAD_LEN + MAX_CMD_LEN + uFRAME_END_LEN;  // 传输帧最大长度

// 帧处理状态枚举
enum class FrameBufState {
    FIND_HEAD,                        // 找帧头
    WAIT_LEN,                         // 等长度字段
    WAIT_AND_CHECK_FRAME              // 等完整帧并校验帧
};

// 命令帧处理类
class CommandFrame {
private:
    FrameBuffer& m_recvBuffer;        // 引用接收缓冲区
    std::vector<U8> m_readBuffer;     // 读取缓冲区
    FrameBufState m_state;            // 当前帧处理状态
    S32 m_expectedLen;                // 当前帧的期望长度
    U16 m_frameShortCount;            // 不完整帧计数

public:
    // 构造函数
    CommandFrame(FrameBuffer& buffer);
    
    // 析构函数
    ~CommandFrame() = default;
    
    // 禁止拷贝构造和赋值操作
    CommandFrame(const CommandFrame&) = delete;
    CommandFrame& operator=(const CommandFrame&) = delete;
    
    // 允许移动构造和移动赋值
    CommandFrame(CommandFrame&&) noexcept = default;
    CommandFrame& operator=(CommandFrame&&) noexcept = default;
    
    // 将数据放入接收缓冲区
    S32 putFrameData(const std::vector<U8>& buffer, S32 dataByte);
    
    // 检查并提取完整帧
    U32 hasCompleteFrame(std::vector<U8>& buffer);
    
    // 处理参数帧
    void processFrame(bool asyncMode);
    
    // 将命令转换为帧格式
    static U16 cmdToFrame(std::vector<U8>& frame, const std::vector<U8>& cmd, U16 cmdLen);
};


#endif /*__COMMAND_FRAME_H__*/