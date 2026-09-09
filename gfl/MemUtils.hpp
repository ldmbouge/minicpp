#pragma once

#include <format>
#include <string>

#include "Alias.hpp"
#include "FunQual.hpp"

namespace gfl {
inline constexpr i32 DefaultAlign = 16;
struct B {
    static constexpr f64 value = 1.0;
    static constexpr char name[] = "  B";
};
struct KiB {
    static constexpr f64 value = 1024.0;
    static constexpr char name[] = "KiB";
};
struct MiB {
    static constexpr f64 value = 1024.0 * KiB::value;
    static constexpr char name[] = "MiB";
};
struct GiB {
    static constexpr f64 value = 1024.0 * MiB::value;
    static constexpr char name[] = "GiB";
};
struct TiB {
    static constexpr f64 value = 1024.0 * GiB::value;
    static constexpr char name[] = "TiB";
};
template<typename Unit> class MemorySize {
    f64 _bytes;

public:
    GFL_HOST_DEVICE constexpr explicit MemorySize(f64 const value) noexcept : _bytes(value * Unit::value) {}
    template<typename To = B> GFL_HOST_DEVICE constexpr f64 as() const noexcept { return _bytes / To::value; }
    GFL_DEVICE void print() const noexcept {
        constexpr f64 values[] = {B::value, KiB::value, MiB::value, GiB::value, TiB::value};
        constexpr char const* names[] = {B::name, KiB::name, MiB::name, GiB::name, TiB::name};
        f64 value = _bytes;
        i32 index = 0;
        while (value > 999.0 && index < 4) {
            value /= 1024.0;
            ++index;
        }
        printf("%.2f %s\n", value / values[index], names[index]);
    }
};
}