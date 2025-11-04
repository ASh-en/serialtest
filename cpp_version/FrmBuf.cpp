#include "FrmBuf.h"
#include <cstring>


// 构造函数：使用外部提供的缓冲区内容
FrameBuffer::FrameBuffer(std::vector<U8> buffer)
    : buf(std::move(buffer)), isDynamic(true) {
    if (buf.empty()) {
        throw std::invalid_argument("Buffer cannot be empty");
    }
    bufSize = static_cast<U32>(buf.size());
}

// 静态工厂方法：创建固定大小的缓冲区
std::unique_ptr<FrameBuffer> FrameBuffer::create(U32 size) {
    if (size == 0) {
        throw std::invalid_argument("Buffer size cannot be zero");
    }
    
    return std::unique_ptr<FrameBuffer>(new FrameBuffer(std::vector<U8>(size)));
}

// 获取缓冲区中已使用的字节数
U32 FrameBuffer::getBytesCount() const noexcept {
    std::lock_guard<std::mutex> guard(lock);
    return usedSize;
}

// 获取缓冲区总容量
U32 FrameBuffer::getCapacity() const noexcept {
    return bufSize;
}

// 获取可用空间
U32 FrameBuffer::getAvailableSpace() const noexcept {
    std::lock_guard<std::mutex> guard(lock);
    return bufSize - usedSize;
}

// 向缓冲区写入数据
S32 FrameBuffer::put(const U8* data, U32 dataLen) {
    if (data == nullptr || dataLen == 0) {
        return -1; // 无数据可写
    }
    
    std::lock_guard<std::mutex> guard(lock);
    
    const U32 free = bufSize - usedSize;
    const U32 actualPut = (dataLen > free) ? free : dataLen;
    
    if (actualPut == 0) {
        return 0; // 缓冲区已满
    }
    
    const U32 tail = bufSize - pToBuf;
    
    if (actualPut <= tail) {
        // 数据可以全部放入缓冲区尾部
        std::memcpy(&buf[pToBuf], data, actualPut);
        pToBuf = (pToBuf + actualPut) % bufSize;
    } else {
        // 数据需要分两部分放入缓冲区
        std::memcpy(&buf[pToBuf], data, tail);
        std::memcpy(buf.data(), data + tail, actualPut - tail);
        pToBuf = actualPut - tail;
    }
    
    usedSize += actualPut;
    
    return static_cast<S32>(actualPut);
}

// 从缓冲区读取数据并删除
S32 FrameBuffer::get(U8* buffer, U32 bufferLen) {
    if (buffer == nullptr || bufferLen == 0) {
        return -1; // 目标缓冲区为空
    }
    
    std::lock_guard<std::mutex> guard(lock);
    
    const U32 available = usedSize;
    const U32 actualGet = (bufferLen > available) ? available : bufferLen;
    
    if (actualGet == 0) {
        return 0; // 缓冲区为空
    }
    
    const U32 tail = bufSize - pFromBuf;
    
    if (actualGet <= tail) {
        // 数据可以全部从缓冲区尾部读取
        std::memcpy(buffer, &buf[pFromBuf], actualGet);
        pFromBuf = (pFromBuf + actualGet) % bufSize;
    } else {
        // 数据需要分两部分从缓冲区读取
        std::memcpy(buffer, &buf[pFromBuf], tail);
        std::memcpy(buffer + tail, buf.data(), actualGet - tail);
        pFromBuf = actualGet - tail;
    }
    
    usedSize -= actualGet;
    
    return static_cast<S32>(actualGet);
}

// 从缓冲区读取数据但不删除
S32 FrameBuffer::peek(U8* buffer, U32 bufferLen) const {
    if (buffer == nullptr || bufferLen == 0) {
        return -1; // 目标缓冲区为空
    }
    
    std::lock_guard<std::mutex> guard(lock);
    
    const U32 available = usedSize;
    const U32 actualGet = (bufferLen > available) ? available : bufferLen;
    
    if (actualGet == 0) {
        return 0; // 缓冲区为空
    }
    
    const U32 tail = bufSize - pFromBuf;
    
    if (actualGet <= tail) {
        std::memcpy(buffer, &buf[pFromBuf], actualGet);
    } else {
        std::memcpy(buffer, &buf[pFromBuf], tail);
        std::memcpy(buffer + tail, buf.data(), actualGet - tail);
    }
    
    return static_cast<S32>(actualGet);
}

// 丢弃缓冲区中的数据
S32 FrameBuffer::drop(U32 dropbytes) {
    if (dropbytes == 0) {
        return -1; // 没有数据可丢弃
    }
    
    std::lock_guard<std::mutex> guard(lock);
    
    const U32 available = usedSize;
    const U32 actualDrop = (dropbytes > available) ? available : dropbytes;
    
    if (actualDrop == 0) {
        return 0; // 缓冲区为空
    }
    
    // 计算新的读取位置
    pFromBuf = (pFromBuf + actualDrop) % bufSize;
    usedSize -= actualDrop;
    
    return static_cast<S32>(actualDrop);
}

// 清空缓冲区
void FrameBuffer::clear() noexcept {
    std::lock_guard<std::mutex> guard(lock);
    pFromBuf = 0;
    pToBuf = 0;
    usedSize = 0;
}

// 检查缓冲区是否为空
bool FrameBuffer::empty() const noexcept {
    std::lock_guard<std::mutex> guard(lock);
    return usedSize == 0;
}

// 检查缓冲区是否已满
bool FrameBuffer::full() const noexcept {
    std::lock_guard<std::mutex> guard(lock);
    return usedSize == bufSize;
}
