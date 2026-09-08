#pragma once

#include <type_traits>

#include "Alias.hpp"
#include "FunQual.hpp"

namespace gfl {
template<typename T> GFL_HOST_DEVICE constexpr u64 toUInt64(T const v) noexcept {
    static_assert(std::is_arithmetic_v<T> && sizeof(T) <= 8);
    u64 bits = 0;
    if constexpr (sizeof(T) == 1)
        bits = bit_cast<u8>(v);
    else if constexpr (sizeof(T) == 2)
        bits = bit_cast<u16>(v);
    else if constexpr (sizeof(T) == 4)
        bits = bit_cast<u32>(v);
    else
        bits = bit_cast<u64>(v);
    return bits;
}

GFL_HOST_DEVICE constexpr u32 hashFold(u64 const h) noexcept { return scast<u32>(h ^ (h >> 32)); }

// https://softwareengineering.stackexchange.com/a/402543
// https://www.boost.org/doc/libs/1_55_0/doc/html/hash/reference.html#boost.hash_combine
GFL_HOST_DEVICE constexpr void hashCombine(u64& seed, u64 const h) noexcept { seed ^= h + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2); }

// One value's own contribution to a combined hash: its .hash() if it has
// one (a BitSet, or any other type that hashes itself), otherwise it must
// be an arithmetic type, hashed via its raw bits (toUInt64).
template<typename T> GFL_HOST_DEVICE constexpr u64 hashPart(T const& v) noexcept {
    if constexpr (requires { v.hash(); }) {
        return v.hash();
    } else {
        return toUInt64(v);
    }
}

template<typename... Ts> GFL_HOST_DEVICE constexpr u64 hashOf(Ts const&... vs) noexcept {
    static_assert(sizeof...(Ts) > 0);
    u64 seed = 0;
    (hashCombine(seed, hashPart(vs)), ...);
    return seed;
}
}