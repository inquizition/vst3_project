//------------------------------------------------------------------------
// spsc_ringbuffer.h
// Single-Producer Single-Consumer lock-free ring buffer.
// Safe to write from the audio thread and read from the UI thread.
//------------------------------------------------------------------------

#pragma once
#include <array>
#include <atomic>
#include <cstring>

template <typename T, size_t Capacity>
class SpscRingBuffer
{
public:
    // Called from AUDIO THREAD — non-blocking
    // Writes up to `count` items from `src`. Returns number actually written.
    size_t write(const T* src, size_t count)
    {
        const size_t writePos = mWritePos.load(std::memory_order_relaxed);
        const size_t readPos  = mReadPos.load(std::memory_order_acquire);
        const size_t available = Capacity - (writePos - readPos);

        if (available == 0 || count == 0)
            return 0;

        const size_t toWrite = count < available ? count : available;
        const size_t idx     = writePos % Capacity;
        const size_t tail    = Capacity - idx;

        if (toWrite <= tail)
        {
            std::memcpy(&mBuffer[idx], src, toWrite * sizeof(T));
        }
        else
        {
            std::memcpy(&mBuffer[idx], src,        tail         * sizeof(T));
            std::memcpy(&mBuffer[0],   src + tail, (toWrite - tail) * sizeof(T));
        }

        mWritePos.store(writePos + toWrite, std::memory_order_release);
        return toWrite;
    }

    // Called from UI THREAD — non-blocking
    // Reads up to `count` items into `dst`. Returns number actually read.
    size_t read(T* dst, size_t count)
    {
        const size_t readPos  = mReadPos.load(std::memory_order_relaxed);
        const size_t writePos = mWritePos.load(std::memory_order_acquire);
        const size_t available = writePos - readPos;

        if (available == 0 || count == 0)
            return 0;

        const size_t toRead = count < available ? count : available;
        const size_t idx    = readPos % Capacity;
        const size_t tail   = Capacity - idx;

        if (toRead <= tail)
        {
            std::memcpy(dst, &mBuffer[idx], toRead * sizeof(T));
        }
        else
        {
            std::memcpy(dst,        &mBuffer[idx], tail          * sizeof(T));
            std::memcpy(dst + tail, &mBuffer[0],   (toRead - tail) * sizeof(T));
        }

        mReadPos.store(readPos + toRead, std::memory_order_release);
        return toRead;
    }

    size_t availableToRead() const
    {
        return mWritePos.load(std::memory_order_acquire)
             - mReadPos.load(std::memory_order_acquire);
    }

    void reset()
    {
        mReadPos.store(0,  std::memory_order_release);
        mWritePos.store(0, std::memory_order_release);
    }

private:
    std::array<T, Capacity> mBuffer {};
    alignas(64) std::atomic<size_t> mWritePos { 0 };
    alignas(64) std::atomic<size_t> mReadPos  { 0 };
};
