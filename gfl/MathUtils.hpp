#pragma once

#include <cassert>
#include <type_traits>

#include "Alias.hpp"
#include "FunQual.hpp"

namespace gfl {
template<typename... Ts>
concept AllIntegral = (std::is_integral_v<Ts> && ...);

template<typename... Ts>
concept AllArithmetic = (std::is_arithmetic_v<Ts> && ...);

// Integer division returning f64
template<typename T1, typename T2> requires AllIntegral<T1, T2>
GFL_HOST_DEVICE constexpr f64 div(T1 const a, T2 const b) noexcept {
    assert(b != 0);
    f64 const x = scast<f64>(a);
    f64 const y = scast<f64>(b);
    return x / y;
}

// Ceiling division: (a + b - 1) / b
template<typename TOut, typename T1, typename T2> requires AllIntegral<TOut, T1, T2>
GFL_HOST_DEVICE constexpr TOut ceilDiv(T1 const a, T2 const b) noexcept {
    if constexpr (std::is_signed_v<T1>) assert(a >= 0);
    assert(b > 0);
    using TComm = std::common_type_t<T1, T2>;
    TComm const x = scast<TComm>(a);
    TComm const y = scast<TComm>(b);
    return scast<TOut>((x + y - 1) / y);
}

// Round down to nearest multiple of b
template<typename TOut, typename T1, typename T2> requires AllIntegral<TOut, T1, T2>
GFL_HOST_DEVICE constexpr TOut roundDown(T1 const a, T2 const b) noexcept {
    if constexpr (std::is_signed_v<T1>) assert(a >= 0);
    assert(b > 0);
    using TComm = std::common_type_t<T1, T2>;
    TComm const x = scast<TComm>(a);
    TComm const y = scast<TComm>(b);
    return scast<TOut>((x / y) * y);
}

// Round up to nearest multiple of b
template<typename TOut, typename T1, typename T2> requires AllIntegral<TOut, T1, T2>
GFL_HOST_DEVICE constexpr TOut roundUp(T1 const a, T2 const b) noexcept {
    if constexpr (std::is_signed_v<T1>) assert(a >= 0);
    assert(b > 0);
    using TComm = std::common_type_t<T1, T2>;
    TComm const x = scast<TComm>(a);
    TComm const y = scast<TComm>(b);
    return scast<TOut>(((x + y - 1) / y) * y);
}

// Minimum of two or more values
template<typename TOut, typename T1, typename T2, typename... TRest> requires AllArithmetic<TOut, T1, T2, TRest...>
GFL_HOST_DEVICE constexpr TOut min(T1 const a, T2 const b, TRest const... rest) noexcept {
    using TComm = std::common_type_t<T1, T2, TRest...>;
    TComm result = scast<TComm>(a) < scast<TComm>(b) ? scast<TComm>(a) : scast<TComm>(b);
    ((result = scast<TComm>(rest) < result ? scast<TComm>(rest) : result), ...);
    return scast<TOut>(result);
}

// Maximum of two or more values
template<typename TOut, typename T1, typename T2, typename... TRest> requires AllArithmetic<TOut, T1, T2, TRest...>
GFL_HOST_DEVICE constexpr TOut max(T1 const a, T2 const b, TRest const... rest) noexcept {
    using TComm = std::common_type_t<T1, T2, TRest...>;
    TComm result = scast<TComm>(a) > scast<TComm>(b) ? scast<TComm>(a) : scast<TComm>(b);
    ((result = scast<TComm>(rest) > result ? scast<TComm>(rest) : result), ...);
    return scast<TOut>(result);
}
}