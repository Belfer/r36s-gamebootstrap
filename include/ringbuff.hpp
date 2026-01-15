#pragma once

#include <device.hpp>

template <typename T, u32 Size>
struct ring_buff_t
{
    ring_buff_t() = default;

    inline u32 size() const { return Size; }
    inline u32 count() const { return (end + Size - start) % Size; }
    inline void consume(u32 n) { start = (start + (n > count() ? count() : n)) % Size; }

    inline void add(const T& v)
    {
        if (count() >= Size) return;
        data[end] = v;
        end = (end + 1) % Size;
    }

    T data[Size]{};
    u32 start{ 0 };
    u32 end{ 0 };
};