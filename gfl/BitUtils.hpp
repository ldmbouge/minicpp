#pragma once

#include <cassert>
#include <cstdio>

#include "Alias.hpp"
#include "FunQual.hpp"

namespace gfl {
template<typename T> requires std::is_unsigned_v<T>
GFL_HOST_DEVICE T bitReverse(T const x) noexcept {
    if constexpr (sizeof(T) == 8) {
#ifdef __CUDA_ARCH__
        return __brevll(x);
#else
        u64 y = x;
        y = ((y >> 1) & 0x5555555555555555llu) | ((y & 0x5555555555555555llu) << 1);
        y = ((y >> 2) & 0x3333333333333333llu) | ((y & 0x3333333333333333llu) << 2);
        y = ((y >> 4) & 0x0F0F0F0F0F0F0F0Fllu) | ((y & 0x0F0F0F0F0F0F0F0Fllu) << 4);
        y = ((y >> 8) & 0x00FF00FF00FF00FFllu) | ((y & 0x00FF00FF00FF00FFllu) << 8);
        y = ((y >> 16) & 0x0000FFFF0000FFFFllu) | ((y & 0x0000FFFF0000FFFFllu) << 16);
        y = (y >> 32) | (y << 32);
        return y;
#endif
    } else {
        u32 y = scast<u32>(x);
#ifdef __CUDA_ARCH__
        y = __brev(y);
#else
        y = ((y >> 1) & 0x55555555u) | ((y & 0x55555555u) << 1);
        y = ((y >> 2) & 0x33333333u) | ((y & 0x33333333u) << 2);
        y = ((y >> 4) & 0x0F0F0F0Fu) | ((y & 0x0F0F0F0Fu) << 4);
        y = ((y >> 8) & 0x00FF00FFu) | ((y & 0x00FF00FFu) << 8);
        y = (y >> 16) | (y << 16);
#endif
        return scast<T>(y >> (32 - digits<T>()));
    }
}
template<typename T> requires std::is_unsigned_v<T>
GFL_HOST_DEVICE constexpr T mask(i32 const pos) noexcept {
    assert(0 <= pos && pos < digits<T>());
    return T{1} << pos;
}
template<typename T> requires std::is_unsigned_v<T>
GFL_HOST_DEVICE constexpr T fullMask() noexcept {
    return ~T{0};
}
template<typename T> requires std::is_unsigned_v<T>
GFL_HOST_DEVICE constexpr T lowMask(i32 const count) noexcept {
    if (count <= 0) return T{0};
    if (count >= digits<T>()) return fullMask<T>();
    return fullMask<T>() >> (digits<T>() - count);
}
template<typename T> requires std::is_unsigned_v<T>
GFL_HOST_DEVICE constexpr T highMask(i32 const count) noexcept {
    if (count <= 0) return T{0};
    if (count >= digits<T>()) return fullMask<T>();
    return fullMask<T>() << (digits<T>() - count);
}
template<typename T> requires std::is_unsigned_v<T>
GFL_HOST_DEVICE constexpr i32 msb(T const x) noexcept {
    assert(x != 0);
    return digits<T>() - 1 - clz(x);
}
template<typename T> requires std::is_unsigned_v<T>
GFL_HOST_DEVICE constexpr i32 lsb(T const x) noexcept {
    assert(x != 0);
    return ctz(x);
}
template<typename T> requires std::is_unsigned_v<T>
GFL_HOST_DEVICE constexpr T clearMsb(T const x) noexcept {
    assert(x != 0);
    return x & ~mask<T>(msb(x));
}
template<typename T> requires std::is_unsigned_v<T>
GFL_HOST_DEVICE constexpr T clearLsb(T const x) noexcept {
    assert(x != 0);
    return x & (x - 1); // Equivalent to: x & ~mask<T>(lsb(x));
}
template<typename T> requires std::is_unsigned_v<T>
GFL_HOST_DEVICE constexpr bool test(T const x, i32 const pos) noexcept {
    assert(0 <= pos && pos < digits<T>());
    return (x & mask<T>(pos)) != 0;
}
template<typename T> requires std::is_unsigned_v<T>
GFL_HOST_DEVICE void printBinary(T const x) noexcept {
    for (i32 i = digits<T>() - 1; i >= 0; --i) { printf("%c", test(x, i) ? '1' : '0'); }
}
static_assert(highMask<u8>(3) == 0b11100000u);
static_assert(lowMask<u8>(3) == 0b00000111u);
static_assert(highMask<u8>(0) == 0u && highMask<u8>(8) == 0xFFu);
static_assert(lowMask<u8>(0) == 0u && lowMask<u8>(8) == 0xFFu);
}