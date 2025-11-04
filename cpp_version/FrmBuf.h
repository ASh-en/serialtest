#pragma once

#include <cstdint>
#include <mutex>
#include <memory>
#include <vector>
#include <stdexcept>

// 使用C++11的using别名代替typedef
using U8 = uint8_t;
using U32 = uint32_t;
using S32 = int32_t;

// C++帧缓冲类，支持安全的环形缓冲区操作
class FrameBuffer {
private:
    U32 pFromBuf{0};      // 待读取的位置 - 使用就地初始化
    U32 pToBuf{0};        // 待写入的位置
    U32 bufSize{0};       // 缓冲区长度
    U32 usedSize{0};      // 已使用的缓冲区长度
    std::vector<U8> buf;  // 使用vector作为内部存储，自动管理内存
    mutable std::mutex lock; // C++互斥锁
    bool isDynamic{false};    // 标记缓冲区管理方式

public:
    // 构造函数：使用外部提供的缓冲区内容
    explicit FrameBuffer(std::vector<U8> buffer);
    
    // 禁止使用空构造函数
    FrameBuffer() = delete;
    
    // 析构函数 - 使用编译器生成的即可
    ~FrameBuffer() = default;
    
    // 静态工厂方法：创建固定大小的缓冲区
    static std::unique_ptr<FrameBuffer> create(U32 size);
    
    // 获取缓冲区中已使用的字节数
    U32 getBytesCount() const noexcept;
    
    // 获取缓冲区总容量
    U32 getCapacity() const noexcept;
    
    // 获取可用空间
    U32 getAvailableSpace() const noexcept;
    
    // 向缓冲区写入数据 - 使用指针+长度替代span
    S32 put(const U8* data, U32 dataLen);
    
    // 从缓冲区读取数据并删除
    S32 get(U8* buffer, U32 bufferLen);
    
    // 从缓冲区读取数据但不删除
    S32 peek(U8* buffer, U32 bufferLen) const;
    
    // 丢弃缓冲区中的数据
    S32 drop(U32 dropbytes);
    
    // 清空缓冲区
    void clear() noexcept;
    
    // 检查缓冲区是否为空
    bool empty() const noexcept;
    
    // 检查缓冲区是否已满
    bool full() const noexcept;
    
    // 禁止拷贝构造和赋值操作
    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;
    
    // 允许移动构造和移动赋值
    FrameBuffer(FrameBuffer&&) noexcept = default;
    FrameBuffer& operator=(FrameBuffer&&) noexcept = default;
};
