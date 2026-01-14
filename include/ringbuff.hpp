#pragma once

#include <device.hpp>

template <typename T, u32 Size>
struct ring_buff_t
{
    ring_buff_t() = default;

    inline u32 size() const { return Size; }
    inline u32 count() const { return (end + (Size + 1) - start) % (Size + 1); }
    inline void consume(u32 n) { start = (start + (n > Size ? Size : n)) % (Size + 1); }

    inline void add(const T& v)
    {
        if (count() >= Size) return;
        data[end % Size] = v;
        end = (end + 1) % (Size + 1);
    }

    T data[Size]{};
    u32 start{ 0 };
    u32 end{ 0 };
};