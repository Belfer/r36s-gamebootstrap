#pragma once

#include <types.hpp>

template <typename T>
struct ring_buff_t
{
    ring_buff_t(u32 capacity) : capacity(capacity) { data = new T[capacity]; }
    ~ring_buff_t() { delete[] data; }
    ring_buff_t(const ring_buff_t&) = delete;
    ring_buff_t& operator=(const ring_buff_t&) = delete;
    ring_buff_t(ring_buff_t&&) = delete;
    ring_buff_t& operator=(ring_buff_t&&) = delete;

    inline void add(const T& v)
    {
        if (count() >= capacity) return;
        data[end] = v;
        end = (end + 1) % capacity;
    }

    inline u32 count() const { return (end + capacity - start) % capacity; }
    inline void consume(u32 n) { start = (start + (n > count() ? count() : n)) % capacity; }

    inline const T* get_data() const { return data; }
    inline u32 get_capacity() const { return capacity; }
    inline u32 get_start() const { return start; }
    inline u32 get_end() const { return end; }

    inline T& operator[](u32 idx) { return data[idx]; }

private:
    T* data{ nullptr };
    u32 capacity{ 0 };
    u32 start{ 0 };
    u32 end{ 0 };
};