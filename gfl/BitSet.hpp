#pragma once

#include <cassert>
#include <iterator>

#include "Alias.hpp"
#include "Backend.hpp"
#include "BitUtils.hpp"
#include "DebugUtils.hpp"
#include "FunQual.hpp"
#include "HashUtils.hpp"
#include "MathUtils.hpp"

namespace gfl {
template<i32 Capacity> class BitSet {
    static_assert(Capacity > 0);

public:
    using WordType = u64;
    static constexpr i32 WordSizeBits = digits<WordType>();
    static constexpr i32 NumWords = ceilDiv<i32>(Capacity, WordSizeBits);
    static constexpr i32 TailBits = Capacity % WordSizeBits;

private:
    WordType _words[NumWords]{};
    static constexpr WordType TailMask = lowMask<WordType>(TailBits);
    GFL_HOST_DEVICE void maskTail() noexcept {
        if constexpr (TailBits != 0) _words[NumWords - 1] &= TailMask;
    }

public:
    BitSet() noexcept = default;
    GFL_HOST_DEVICE explicit BitSet(i32 const value) noexcept : _words{} { insert(value); }
    GFL_HOST_DEVICE BitSet(i32 const min, i32 const max) noexcept : _words{} {
        assert(0 <= min);
        assert(min <= max);
        assert(max < Capacity);
        i32 const minBit = min % WordSizeBits;
        i32 const maxBit = max % WordSizeBits;
        // NumWords == 1 implies minWord == maxWord == 0 for every valid
        // (min, max): both single-word branches always execute the same
        // ordinary case, but writing it as a distinct compile-time path keeps
        // the multi-word branch's indices (which reach NumWords) out of this
        // instantiation entirely, rather than relying on the (NDEBUG-stripped
        // at -O3) asserts above to prove them unreachable.
        if constexpr (NumWords == 1) {
            _words[0] = highMask<WordType>(WordSizeBits - minBit) & lowMask<WordType>(maxBit + 1);
        } else {
            i32 const minWord = min / WordSizeBits;
            i32 const maxWord = max / WordSizeBits;
            if (minWord == maxWord) {
                _words[minWord] = highMask<WordType>(WordSizeBits - minBit) & lowMask<WordType>(maxBit + 1);
                return;
            }
            _words[minWord] = highMask<WordType>(WordSizeBits - minBit);
            _words[maxWord] = lowMask<WordType>(maxBit + 1);
            for (i32 word = minWord + 1; word < maxWord; ++word) _words[word] = fullMask<WordType>();
        }
    }
    GFL_HOST_DEVICE BitSet(initializer_list<i32> const values) noexcept : _words{} {
        for (i32 const value : values) insert(value);
    }
    GFL_HOST_DEVICE static constexpr i32 capacity() noexcept { return Capacity; }
    GFL_HOST_DEVICE static constexpr i32 maximum() noexcept { return Capacity - 1; }
    GFL_HOST_DEVICE static constexpr i32 nWords() noexcept { return NumWords; }
    GFL_HOST_DEVICE static constexpr i32 nWords(i32 const maxValue) noexcept {
        assert(maxValue >= 0);
        return ceilDiv<i32>(maxValue + 1, WordSizeBits);
    }
    GFL_HOST_DEVICE WordType word(i32 const i) const noexcept {
        assert(0 <= i && i < NumWords);
        return _words[i];
    }

    class Iterator {
    public:
        using difference_type = i32;
        using value_type = i32;
        using iterator_category = std::forward_iterator_tag;
        using iterator_concept = std::forward_iterator_tag;

    private:
        WordType const* _word{};
        i32 _wIdx{};
        i32 _bIdx{};
        GFL_HOST_DEVICE
        void seek() noexcept {
            while (_wIdx < NumWords) {
                WordType const bits = *_word & highMask<WordType>(WordSizeBits - _bIdx);
                if (bits != 0) {
                    _bIdx = lsb(bits);
                    return;
                }
                ++_word;
                ++_wIdx;
                _bIdx = 0;
            }
        }

    public:
        Iterator() noexcept = default;
        GFL_HOST_DEVICE Iterator(BitSet const& set, i32 const value) noexcept {
            assert(0 <= value && value <= Capacity);
            if (value == Capacity) {
                _word = set._words + NumWords;
                _wIdx = NumWords;
                _bIdx = 0;
                return;
            }
            _wIdx = value / WordSizeBits;
            _bIdx = value % WordSizeBits;
            _word = set._words + _wIdx;
            seek();
        }
        GFL_HOST_DEVICE i32 operator*() const noexcept {
            assert(_wIdx < NumWords);
            return _wIdx * WordSizeBits + _bIdx;
        }
        GFL_HOST_DEVICE Iterator& operator++() noexcept {
            assert(_wIdx < NumWords);
            ++_bIdx;
            if (_bIdx == WordSizeBits) {
                _bIdx = 0;
                ++_word;
                ++_wIdx;
            }
            seek();
            return *this;
        }
        GFL_HOST_DEVICE Iterator operator++(int) noexcept {
            Iterator const tmp = *this;
            ++(*this);
            return tmp;
        }
        GFL_HOST_DEVICE bool operator==(Iterator const& other) const noexcept { return _wIdx == other._wIdx && _bIdx == other._bIdx; }
        GFL_HOST_DEVICE bool operator!=(Iterator const& other) const noexcept { return !(*this == other); }
    };

    GFL_HOST_DEVICE Iterator begin() const noexcept { return {*this, 0}; }
    GFL_HOST_DEVICE Iterator end() const noexcept { return {*this, Capacity}; }
    GFL_HOST_DEVICE Iterator begin() noexcept { return {*this, 0}; }
    GFL_HOST_DEVICE Iterator end() noexcept { return {*this, Capacity}; }
    GFL_HOST_DEVICE i32 size() const noexcept {
        i32 result = 0;
        for (i32 i = 0; i < NumWords; ++i) result += gfl::popcount(_words[i]);
        return result;
    }
    GFL_HOST_DEVICE bool empty() const noexcept {
        for (i32 i = 0; i < NumWords; ++i) {
            if (_words[i] != 0) return false;
        }
        return true;
    }
    GFL_HOST_DEVICE bool contains(i32 const value) const noexcept {
        if (value < 0 || value >= Capacity) return false;
        return test(_words[value / WordSizeBits], value % WordSizeBits);
    }
    GFL_HOST_DEVICE i32 ith(i32 i) const noexcept {
        assert(i >= 0);
        for (i32 j = 0; j < NumWords; ++j) {
            WordType bits = _words[j];
            i32 const count = gfl::popcount(bits);
            if (i >= count) {
                i -= count;
                continue;
            }
            while (i-- > 0) bits = clearLsb(bits);
            return j * WordSizeBits + lsb(bits);
        }
        abort();
    }

    GFL_HOST_DEVICE i32 rank(i32 const value) const noexcept {
        assert(value >= 0);
        i32 const limit = gfl::min<i32>(value, Capacity);
        i32 const word = limit / WordSizeBits;
        i32 result = 0;
        for (i32 i = 0; i < word; ++i) result += gfl::popcount(_words[i]);
        i32 const bits = limit % WordSizeBits;
        if (bits != 0 && word < NumWords) result += gfl::popcount(_words[word] & lowMask<WordType>(bits));
        return result;
    }
    // Count of set bits in [min, max] (inclusive) — same word-boundary
    // math as the (min, max) range constructor below, applied directly
    // against this set's own words instead of writing out a mask to AND
    // against afterward. One pass touching only the words in range,
    // unlike rank(max+1) - rank(min) (two independent scans from word 0,
    // each redoing the same low words the other already covered).
    GFL_HOST_DEVICE i32 popcount(i32 const min, i32 const max) const noexcept {
        assert(0 <= min);
        assert(min <= max);
        assert(max < Capacity);
        i32 const minBit = min % WordSizeBits;
        i32 const maxBit = max % WordSizeBits;
        if constexpr (NumWords == 1) {
            return gfl::popcount(_words[0] & highMask<WordType>(WordSizeBits - minBit) & lowMask<WordType>(maxBit + 1));
        } else {
            i32 const minWord = min / WordSizeBits;
            i32 const maxWord = max / WordSizeBits;
            if (minWord == maxWord) return gfl::popcount(_words[minWord] & highMask<WordType>(WordSizeBits - minBit) & lowMask<WordType>(maxBit + 1));
            i32 result = gfl::popcount(_words[minWord] & highMask<WordType>(WordSizeBits - minBit));
            result += gfl::popcount(_words[maxWord] & lowMask<WordType>(maxBit + 1));
            for (i32 word = minWord + 1; word < maxWord; ++word) result += gfl::popcount(_words[word]);
            return result;
        }
    }
    GFL_HOST_DEVICE i32 smallest() const noexcept {
        assert(!empty());
        for (i32 i = 0; i < NumWords; ++i)
            if (_words[i] != 0) return i * WordSizeBits + lsb(_words[i]);
        return highest<WordType>();
    }
    GFL_HOST_DEVICE i32 largest() const noexcept {
        assert(!empty());
        for (i32 i = NumWords - 1; i >= 0; --i)
            if (_words[i] != 0) return i * WordSizeBits + msb(_words[i]);
        return highest<WordType>();
    }
    GFL_HOST_DEVICE tuple<i32, i32, i32> summary() const noexcept {
        i32 smallest = highest<i32>();
        i32 largest = lowest<i32>();
        i32 count = 0;
        for (i32 i = 0; i < NumWords; ++i) {
            WordType const word = _words[i];
            if (word == 0) continue;
            i32 const offset = i * WordSizeBits;
            smallest = min<i32>(smallest, offset + lsb(word));
            largest = max<i32>(largest, offset + msb(word));
            count += gfl::popcount(word);
        }
        return {smallest, largest, count};
    }
    GFL_HOST_DEVICE void clear() noexcept {
        for (i32 i = 0; i < NumWords; ++i) _words[i] = 0;
    }
    GFL_HOST_DEVICE void insert(i32 const value) noexcept {
        assert(0 <= value && value < Capacity);
        _words[value / WordSizeBits] |= mask<WordType>(value % WordSizeBits);
    }
    GFL_HOST_DEVICE void remove(i32 const value) noexcept {
        assert(0 <= value && value < Capacity);
        _words[value / WordSizeBits] &= ~mask<WordType>(value % WordSizeBits);
    }
    GFL_HOST_DEVICE BitSet& unionWith(BitSet const& other) noexcept {
        for (i32 i = 0; i < NumWords; ++i) _words[i] |= other._words[i];
        return *this;
    }
    GFL_HOST_DEVICE
    BitSet& interWith(BitSet const& other) noexcept {
        for (i32 i = 0; i < NumWords; ++i) _words[i] &= other._words[i];
        return *this;
    }
    GFL_HOST_DEVICE BitSet& diffWith(BitSet const& other) noexcept {
        for (i32 i = 0; i < NumWords; ++i) _words[i] &= ~other._words[i];
        return *this;
    }
    GFL_HOST_DEVICE BitSet& operator|=(BitSet const& other) noexcept { return unionWith(other); }
    GFL_HOST_DEVICE BitSet& operator&=(BitSet const& other) noexcept { return interWith(other); }
    GFL_HOST_DEVICE BitSet& operator-=(BitSet const& other) noexcept { return diffWith(other); }
    GFL_HOST_DEVICE
    BitSet& complement() noexcept {
        for (i32 i = 0; i < NumWords; ++i) _words[i] = ~_words[i];
        maskTail();
        return *this;
    }
    GFL_HOST_DEVICE friend bool operator<=(BitSet const& a, BitSet const& b) noexcept {
        for (i32 i = 0; i < NumWords; ++i)
            if ((a._words[i] & ~b._words[i]) != 0) return false;
        return true;
    }
    GFL_HOST_DEVICE BitSet shiftLeft(i32 const shift) const noexcept {
        assert(shift >= 0);
        BitSet result;
        if (shift >= Capacity) return result;
        i32 const wordShift = shift / WordSizeBits;
        i32 const bitShift = shift % WordSizeBits;
        for (i32 i = 0; i < NumWords; ++i) {
            i32 const source = i - wordShift;
            WordType const upper = 0 <= source && source < NumWords ? _words[source] << bitShift : 0;
            WordType const lower = bitShift != 0 && source > 0 ? _words[source - 1] >> (WordSizeBits - bitShift) : 0;
            result._words[i] = upper | lower;
        }
        result.maskTail();
        return result;
    }
    GFL_HOST_DEVICE
    BitSet shiftRight(i32 const shift) const noexcept {
        assert(shift >= 0);
        BitSet result;
        if (shift >= Capacity) return result;
        i32 const wordShift = shift / WordSizeBits;
        i32 const bitShift = shift % WordSizeBits;
        for (i32 i = 0; i < NumWords; ++i) {
            i32 const source = i + wordShift;
            WordType const lower = source < NumWords ? _words[source] >> bitShift : 0;
            WordType const upper = bitShift != 0 && source + 1 < NumWords ? _words[source + 1] << (WordSizeBits - bitShift) : 0;
            result._words[i] = lower | upper;
        }
        result.maskTail();
        return result;
    }
    GFL_HOST_DEVICE
    BitSet reversed() const noexcept {
        BitSet result;
        for (i32 i = 0; i < NumWords; ++i) result._words[i] = bitReverse(_words[NumWords - 1 - i]);
        if constexpr (TailBits != 0) result = result.shiftRight(WordSizeBits - TailBits);
        result.maskTail();
        return result;
    }
    GFL_HOST_DEVICE BitSet(i32 const offset, BitSet const& set) noexcept : _words{} {
        assert(0 <= offset && offset < Capacity);
        *this = set.reversed().shiftRight(Capacity - offset - 1);
    }

    GFL_HOST_DEVICE f32 interOverUnion(BitSet const& a, BitSet const& b) const noexcept {
        i32 interSz = 0;
        i32 unionSz = 0;
        for (i32 i = 0; i < NumWords; ++i) {
            interSz += gfl::popcount(a._words[i] & b._words[i]);
            unionSz += gfl::popcount(a._words[i] | b._words[i]);
        }
        return unionSz == 0 ? 1.0f : as<f32>(div(interSz, unionSz));
    }
    GFL_DEVICE void print() const noexcept {
        printf("[");
        bool first = true;
        for (i32 const value : *this) {
            if (!first) printf(",");
            printf("%d", value);
            first = false;
        }
        printf("]\n");
    }
    GFL_DEVICE void printBinary() const noexcept {
        printf("[");
        for (i32 i = 0; i < NumWords; ++i) {
            if (i != 0) printf(",");
            gfl::printBinary(_words[i]);
        }
        printf("]\n");
    }
    GFL_HOST_DEVICE u64 hash() const noexcept {
        u64 seed = 0;
        for (i32 i = 0; i < NumWords; ++i) hashCombine(seed, _words[i]);
        return seed;
    }
    GFL_HOST_DEVICE friend bool operator==(BitSet const& a, BitSet const& b) noexcept {
        for (i32 i = 0; i < NumWords; ++i)
            if (a._words[i] != b._words[i]) return false;
        return true;
    }
    GFL_HOST_DEVICE friend bool operator!=(BitSet const& a, BitSet const& b) noexcept { return !(a == b); }
    GFL_HOST_DEVICE friend BitSet operator|(BitSet a, BitSet const& b) noexcept { return a.unionWith(b); }
    GFL_HOST_DEVICE friend BitSet operator&(BitSet a, BitSet const& b) noexcept { return a.interWith(b); }
    GFL_HOST_DEVICE friend BitSet operator-(BitSet a, BitSet const& b) noexcept { return a.diffWith(b); }
    GFL_HOST_DEVICE friend BitSet operator-(BitSet a, i32 const value) noexcept {
        a.remove(value);
        return a;
    }
    GFL_HOST_DEVICE friend BitSet operator~(BitSet a) noexcept { return a.complement(); }
    GFL_HOST_DEVICE friend BitSet operator-(i32 const value, BitSet const& set) noexcept { return BitSet(value, set); }
};

template<typename T> inline constexpr bool isBitSet = false;
template<i32 Capacity> inline constexpr bool isBitSet<BitSet<Capacity>> = true;

static_assert(isBitSet<BitSet<128>>);
static_assert(!isBitSet<int>);
} // namespace gfl