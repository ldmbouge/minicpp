#pragma once

#include <format>

#include "Alias.hpp"
#include "ArrayView.hpp"
#include "BitSet.hpp"
#include "ChronoUtils.hpp"
#include "MemUtils.hpp"
#include "PackedUInts.hpp"
#include "VectorView.hpp"

namespace std {
template<typename T, typename Ptr> struct formatter<gfl::ArrayView<T, Ptr>, char> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext> auto format(gfl::ArrayView<T, Ptr> const& v, FormatContext& ctx) const {
        auto out = ctx.out();
        *out++ = '[';
        for (gfl::i64 i = 0; i < v.size(); ++i) {
            if (i != 0) *out++ = ',';
            out = std::format_to(out, "{}", v[i]);
        }
        *out++ = ']';
        return out;
    }
};
template<typename T, typename Ptr> struct formatter<gfl::VectorView<T, Ptr>, char> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext> auto format(gfl::VectorView<T, Ptr> const& v, FormatContext& ctx) const { return std::format_to(ctx.out(), "{}", v.view()); }
};
template<gfl::i32 MaxValue, gfl::i32 Size> struct formatter<gfl::PackedUInts<MaxValue, Size>> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(gfl::PackedUInts<MaxValue, Size> const& packed, std::format_context& ctx) const {
        auto out = ctx.out();
        *out++ = '[';
        for (gfl::i64 i = 0; i < Size; ++i) {
            if (i != 0) *out++ = ',';
            out = std::format_to(out, "{}", packed.get(i));
        }
        *out++ = ']';
        return out;
    }
};
template<gfl::i32 Capacity> struct formatter<gfl::BitSet<Capacity>, char> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext> auto format(gfl::BitSet<Capacity> const& set, FormatContext& ctx) const {
        auto out = ctx.out();
        *out++ = '[';
        bool first = true;
        for (auto const value : set) {
            if (!first) *out++ = ',';
            out = std::format_to(out, "{}", value);
            first = false;
        }
        *out++ = ']';
        return out;
    }
};
template<typename Unit> struct formatter<gfl::MemorySize<Unit>, char> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext> auto format(gfl::MemorySize<Unit> const& size, FormatContext& ctx) const {
        using namespace gfl;
        constexpr char const* names[] = {B::name, KiB::name, MiB::name, GiB::name, TiB::name};
        f64 value = size.template as<B>();
        i32 index = 0;
        while (value > 999.0 && index < 4) {
            value /= 1024.0;
            ++index;
        }
        return std::format_to(ctx.out(), "{:.2f} {}", value, names[index]);
    }
};
template<typename Rep, typename Period> struct formatter<std::chrono::duration<Rep, Period>, char> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(std::chrono::duration<Rep, Period> const& d, std::format_context& ctx) const {
        using namespace gfl;
        constexpr char const* names[] = {"ns", "us", "ms", " s"};
        f64 value = scast<f64>(std::chrono::duration_cast<ns>(d).count());
        i32 index = 0;
        while (value > 999.0 && index < 3) {
            value /= 1000.0;
            ++index;
        }
        return std::format_to(ctx.out(), "{:.2f} {}", value, names[index]);
    }
};
}
