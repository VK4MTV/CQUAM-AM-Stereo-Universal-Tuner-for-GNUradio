#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// RingBuffer.h  –  Lock-free single-producer / single-consumer ring buffer
// ─────────────────────────────────────────────────────────────────────────────
#include <atomic>
#include <cstddef>
#include <cassert>

template<typename T, std::size_t Capacity>
class RingBuffer
{
    static_assert((Capacity & (Capacity - 1)) == 0,
                  "Capacity must be a power of two");
public:
    RingBuffer() : head_(0), tail_(0) {}

    // Producer: returns false if full
    bool push(const T& item)
    {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t next = (head + 1) & (Capacity - 1);
        if (next == tail_.load(std::memory_order_acquire))
            return false;   // full
        buf_[head] = item;
        head_.store(next, std::memory_order_release);
        return true;
    }

    // Producer: bulk push; returns number of items actually pushed
    std::size_t push(const T* data, std::size_t count)
    {
        std::size_t pushed = 0;
        for (std::size_t i = 0; i < count; ++i) {
            if (!push(data[i])) break;
            ++pushed;
        }
        return pushed;
    }

    // Consumer: returns false if empty
    bool pop(T& item)
    {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire))
            return false;   // empty
        item = buf_[tail];
        tail_.store((tail + 1) & (Capacity - 1), std::memory_order_release);
        return true;
    }

    // Consumer: bulk pop; returns number of items popped
    std::size_t pop(T* data, std::size_t maxCount)
    {
        std::size_t popped = 0;
        while (popped < maxCount) {
            if (!pop(data[popped])) break;
            ++popped;
        }
        return popped;
    }

    std::size_t size() const
    {
        const std::size_t h = head_.load(std::memory_order_acquire);
        const std::size_t t = tail_.load(std::memory_order_acquire);
        return (h - t + Capacity) & (Capacity - 1);
    }

    bool empty() const { return head_.load() == tail_.load(); }

private:
    T                   buf_[Capacity];
    std::atomic<std::size_t> head_;
    alignas(64) std::atomic<std::size_t> tail_;
};
