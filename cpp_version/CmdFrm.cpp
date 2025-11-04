#include "AsyncFrame.h"
#include <iostream>
#include <vector>
#include "CmdFrm.h"


// 全局变量定义
U32 frameCount = 0;

// 构造函数
CommandFrame::CommandFrame(FrameBuffer& buffer)
    : m_recvBuffer(buffer),
      m_readBuffer(MAX_CMD_LEN),
      m_state(FrameBufState::FIND_HEAD),
      m_expectedLen(0),
      m_frameShortCount(0) {
}

// 将数据放入接收缓冲区
S32 CommandFrame::putFrameData(const std::vector<U8>& buffer, S32 dataByte) {
    if (buffer.empty() || dataByte <= 0) {
        return -1;
    }
    return m_recvBuffer.put(buffer.data(), dataByte);
}

// 检查并提取完整帧
U32 CommandFrame::hasCompleteFrame(std::vector<U8>& buffer) {
    U32 available = m_recvBuffer.getBytesCount();

    if (available < uFRAME_MIN_LEN) {
        return 0; // 可用数据不足以构成最小帧
    }

    while (true) { // 循环驱动状态迁移
        switch (m_state) {
        case FrameBufState::FIND_HEAD:
            while (available >= 1) {
                U8 tempByte;
                m_recvBuffer.peek(&tempByte, 1);
                if (tempByte == FRAME_HEAD) {
                    m_state = FrameBufState::WAIT_LEN; // 状态切换
                    break;                             // 跳出 switch，重新进入 while
                } else {
                    m_recvBuffer.drop(1); // 丢掉垃圾字节
                    available--;
                }
            }

            // 如果没有足够数据，就退出
            if (m_state != FrameBufState::WAIT_LEN) {
                return 0;
            }
            break;

        case FrameBufState::WAIT_LEN:
            if (available >= uFRAME_MIN_LEN) {
                // 使用 std::vector 替代 C 风格数组
                std::vector<U8> tempBuffer(uFRAME_MIN_LEN);
                m_recvBuffer.peek(tempBuffer.data(), uFRAME_MIN_LEN);
                m_expectedLen = ((static_cast<U16>(tempBuffer[uFRAME_LEN_H_IDX]) << 8) |
                               tempBuffer[uFRAME_LEN_L_IDX]) + uFRAME_HE_ND_LEN;

                if (m_expectedLen < uFRAME_MIN_LEN || m_expectedLen > uFRAME_MAX_LEN) {
                    // 长度非法，回到找帧头
                    m_recvBuffer.drop(1);
                    m_expectedLen = 0;
                    available--;
                    m_state = FrameBufState::FIND_HEAD;
                    break; // 回到 while，再进 FIND_HEAD
                } else {
                    m_state = FrameBufState::WAIT_AND_CHECK_FRAME;
                    break; // 回到 while，再进 WAIT_AND_CHECK_FRAME
                }
            }
            return 0; // 数据不够，退出函数

        case FrameBufState::WAIT_AND_CHECK_FRAME:
            if (available >= static_cast<U32>(m_expectedLen)) {
                // 可以取出完整帧
                std::vector<U8> tempFrame(m_expectedLen);
                m_recvBuffer.peek(tempFrame.data(), m_expectedLen);
                if (tempFrame[m_expectedLen - 1] != FRAME_END) {
                    // 帧尾错误，丢掉一个字节重新找帧头
                    m_recvBuffer.drop(1);
                    m_expectedLen = 0;
                    available--;
                    m_state = FrameBufState::FIND_HEAD;
                    break;
                } else {
                    // 校验
                    U8 checksum = 0;
                    for (S32 i = 0; i < m_expectedLen - uFRAME_END_LEN; ++i) {
                        checksum ^= tempFrame[i];
                    }
                    if (tempFrame[m_expectedLen - 2] == checksum) {
                        // 提取负载
                        m_recvBuffer.drop(uFRAME_HEAD_LEN);
                        m_recvBuffer.get(buffer.data(), m_expectedLen - uFRAME_HE_ND_LEN);
                        m_recvBuffer.drop(uFRAME_END_LEN);

                        frameCount++;
                        m_state = FrameBufState::FIND_HEAD;
                        available -= m_expectedLen;
                        // 返回完整帧长度
                        S32 resultLen = m_expectedLen - uFRAME_HE_ND_LEN;
                        m_expectedLen = 0;
                        return resultLen; // 返回完整命令帧长度
                    } else {
                        // 校验失败，丢掉该帧
                        m_recvBuffer.drop(m_expectedLen);
                        m_state = FrameBufState::FIND_HEAD;
                        break;
                    }
                }
            }
            return 0; // 数据不够，退出函数
        }

        // 如果状态没发生变化，避免死循环
        if (m_state == FrameBufState::FIND_HEAD && available < uFRAME_MIN_LEN) {
            return 0;
        }
    }
}

// 处理参数帧
void CommandFrame::processFrame(bool asyncMode) {
    U16 len = 0;
    
    if ((len = hasCompleteFrame(m_readBuffer)) > 0) {
        if (asyncMode) {
            // 异步模式，入队列
            AsyncFrameDispatcher::getInstance().pushFrameToQueue(m_readBuffer, len);
        } else {
            // 同步模式，直接处理命令
            U8 frameType = m_readBuffer[0];
            // 这里可以根据需要添加具体的命令处理逻辑
            std::cerr << "Sync processing frame type: 0x" << std::hex << static_cast<int>(frameType) << std::dec << std::endl;
        }
    }
}

// 将命令转换为帧格式
U16 CommandFrame::cmdToFrame(std::vector<U8>& frame, const std::vector<U8>& cmd, U16 cmdLen) {
    U8 checksum = 0;
    U16 i, frameLen;

    frame[0] = FRAME_HEAD;
    frame[uFRAME_DEV_IDX] = 0; // 设备号，目前请求端为0，响应端为1
    frame[uFRAME_LEN_H_IDX] = static_cast<U8>(cmdLen >> 8);
    frame[uFRAME_LEN_L_IDX] = static_cast<U8>(cmdLen & 0xFF);

    frameLen = cmdLen + uFRAME_HEAD_LEN;
    checksum = frame[0] ^ frame[1] ^ frame[2] ^ frame[3];
    for (i = uFRAME_HEAD_LEN; i < frameLen; i++) {
        frame[i] = cmd[i - uFRAME_HEAD_LEN];
        checksum ^= frame[i];
    }
    frame[frameLen++] = checksum;
    frame[frameLen++] = FRAME_END;
    return frameLen;
}

