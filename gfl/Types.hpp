#pragma once

#include <cstdint>
#include "FunQual.hpp"

namespace gfl {
// Fixed-width types
using i8 = int8_t;
using u8 = uint8_t;

using i16 = int16_t;
using u16 = uint16_t;

using i32 = int32_t;
using u32 = uint32_t;

using i64 = int64_t;
using u64 = uint64_t;

// Reference: https://developer.nvidia.com/blog/implementing-high-precision-decimal-arithmetic-with-cuda-int128
using i128 = __int128;
using u128 = unsigned __int128;

using f32 = float;
using f64 = double;

using uptr = std::uintptr_t;
using usize = std::size_t;

// Printf format helper types
using lld = long long int;
using llu = unsigned long long int;

// Type-changing casts: name the target type.
template<typename To, typename From>
GFL_HOST_DEVICE constexpr To scast(From from) noexcept {
    return static_cast<To>(from);
}

template<typename To, typename From>
GFL_HOST_DEVICE constexpr To rcast(From from) noexcept {
    return reinterpret_cast<To>(from);
}

// Qualifier-adjusting casts: type deduced, only const-ness changes.
template<typename T>
GFL_HOST_DEVICE constexpr T const* asConst(T* ptr) noexcept {
    return ptr;
}
template<typename T>
GFL_HOST_DEVICE constexpr T const& asConst(T& ref) noexcept {
    return ref;
}

template<typename T>
GFL_HOST_DEVICE constexpr T* asMut(T const* ptr) noexcept {
    return const_cast<T*>(ptr);
}
template<typename T>
GFL_HOST_DEVICE constexpr T& asMut(T const& ref) noexcept {
    return const_cast<T&>(ref);
}
}