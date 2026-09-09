#pragma once

#include <cstdint>
#include <type_traits>

#include "Backend.hpp"
#include "FunQual.hpp"

namespace gfl {
// Types
// - https://developer.nvidia.com/blog/implementing-high-precision-decimal-arithmetic-with-cuda-int128
using i8 = int8_t;
using u8 = uint8_t;
using i16 = int16_t;
using u16 = uint16_t;
using i32 = int32_t;
using u32 = uint32_t;
using i64 = int64_t;
using u64 = uint64_t;
using i128 = __int128;
using u128 = unsigned __int128;
using f32 = float;
using f64 = double;
using uptr = std::uintptr_t;
using usize = std::size_t;
using lld = long long int;
using llu = unsigned long long int;

// Traits
// nvcc's front end does not classify __int128 as an integral type, so
// std::is_integral_v, std::is_unsigned_v and std::is_arithmetic_v all answer
// false for i128/u128 in a CUDA translation unit while clang answers true.
// These put the two back, so a header constrained on them behaves the same
// whichever compiler sees it. Everything else defers to the standard traits.
template<typename T> inline constexpr bool isInt128 = std::is_same_v<std::remove_cv_t<T>, i128> || std::is_same_v<std::remove_cv_t<T>, u128>;
template<typename T> inline constexpr bool isIntegral = std::is_integral_v<T> || isInt128<T>;
template<typename T> inline constexpr bool isUnsigned = std::is_unsigned_v<T> || std::is_same_v<std::remove_cv_t<T>, u128>;
template<typename T> inline constexpr bool isArithmetic = std::is_arithmetic_v<T> || isInt128<T>;
template<typename T> concept Unsigned = isUnsigned<T>;
template<typename T> concept Integral = isIntegral<T>;
template<typename T> concept Arithmetic = isArithmetic<T>;

// Limits
template<typename T> GFL_HOST_DEVICE constexpr T highest() noexcept { return numeric_limits<T>::max(); }
template<typename T> GFL_HOST_DEVICE constexpr T lowest() noexcept { return numeric_limits<T>::lowest(); }
template<typename T> GFL_HOST_DEVICE constexpr i32 digits() noexcept { return numeric_limits<T>::digits; }

// Bits
template<Unsigned T>
GFL_HOST_DEVICE constexpr i32 clz(T const x) noexcept {
    return countl_zero(x);
}
template<Unsigned T>
GFL_HOST_DEVICE constexpr i32 ctz(T const x) noexcept {
    return countr_zero(x);
}

// Casts
template<typename To, typename From> GFL_HOST_DEVICE constexpr To scast(From const& from) noexcept { return static_cast<To>(from); }
template<typename To, typename From> GFL_HOST_DEVICE inline To rcast(From from) noexcept { return reinterpret_cast<To>(from); }
template<typename T> GFL_HOST_DEVICE constexpr T const* asConst(T* ptr) noexcept { return ptr; }
template<typename T> GFL_HOST_DEVICE constexpr T const& asConst(T& ref) noexcept { return ref; }
template<typename T> GFL_HOST_DEVICE constexpr T* asMut(T const* ptr) noexcept { return const_cast<T*>(ptr); }
template<typename T> GFL_HOST_DEVICE constexpr T& asMut(T const& ref) noexcept { return const_cast<T&>(ref); }
template<typename To, typename From> requires(isArithmetic<From> && isArithmetic<To>)
GFL_HOST_DEVICE constexpr To as(From from) noexcept {
    return scast<To>(from);
}
template<typename From> GFL_HOST_DEVICE constexpr usize asSize(From from) noexcept { return as<usize>(from); }
template<typename From> GFL_HOST_DEVICE constexpr bool asBool(From from) noexcept { return as<bool>(from); }
}