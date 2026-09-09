#pragma once

#include <cassert>
#include <format>

#include "Alias.hpp"
#include "BitUtils.hpp"
#include "FunQual.hpp"
#include "MathUtils.hpp"

namespace gfl {
template<i32 MaxValue, i32 Size> class PackedUInts {
    using WordType = u16;
    using WindowType = u32;
    static_assert(Size >= 1);
    static_assert(MaxValue >= 1);
    static_assert(MaxValue <= highest<WordType>());
    static constexpr i32 MaxValueBits = bit_width(as<u32>(MaxValue));
    static constexpr i32 WordSizeBits = digits<WordType>();
    static constexpr i32 NumWords = ceilDiv<i32>(MaxValueBits * Size, WordSizeBits);
    static constexpr WindowType Mask = lowMask<WindowType>(MaxValueBits);
    WordType _words[NumWords + 1]{}; // +1: spare word for the window
public:
    GFL_HOST_DEVICE constexpr WordType get(i32 const i) const noexcept {
        assert(0 <= i);
        assert(i < Size);
        i32 const j = i * MaxValueBits;
        i32 const wIdx = j / WordSizeBits;
        i32 const wOff = j % WordSizeBits;
        WindowType const win = scast<WindowType>(_words[wIdx]) | (scast<WindowType>(_words[wIdx + 1]) << WordSizeBits);
        return scast<WordType>((win >> wOff) & Mask);
    }
    GFL_HOST_DEVICE constexpr void set(i32 const i, u32 const value) noexcept {
        assert(0 <= i);
        assert(i < Size);
        assert(value <= MaxValue);
        i32 const j = i * MaxValueBits;
        i32 const wIdx = j / WordSizeBits;
        i32 const wOff = j % WordSizeBits;
        WindowType const old = scast<WindowType>(_words[wIdx]) | (scast<WindowType>(_words[wIdx + 1]) << WordSizeBits);
        WindowType const win = (old & ~(Mask << wOff)) | (scast<WindowType>(value) << wOff);
        _words[wIdx] = scast<WordType>(win);
        _words[wIdx + 1] = scast<WordType>(win >> WordSizeBits);
    }
    GFL_HOST_DEVICE static constexpr i32 size() noexcept { return Size; }
    GFL_HOST_DEVICE constexpr void clear() noexcept {
        for (i32 i = 0; i < NumWords + 1; ++i) _words[i] = 0;
    }
};
}