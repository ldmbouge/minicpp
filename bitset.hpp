/*
 * mini-cp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License  v3
 * as published by the Free Software Foundation.
 *
 * mini-cp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY.
 * See the GNU Lesser General Public License  for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with mini-cp. If not, see http://www.gnu.org/licenses/lgpl-3.0.en.html
 *
 * Copyright (c)  2018. by Laurent Michel, Pierre Schaus, Pascal Van Hentenryck
 * 
 * StaticBitSet Implementation: Tim Curry
 */

#ifndef __BITSET_H
#define __BITSET_H

#include "trailable.hpp"
#include "handle.hpp"
#include "store.hpp"
#include <vector>
#include <bitset>
#include <algorithm>
#include <cstdint>
#include <type_traits>

/**
 * Bit layout shared by StaticBitSet and SparseBitSet: bit `pos` lives in word `pos / bits`
 * at the `pos % bits`-th position counted from the most significant bit.
 * W is any 32- or 64-bit integer type (int, unsigned, long, uint64_t, ...).
 */
template <class W> struct BitWord {
   static_assert(std::is_integral<W>::value && (sizeof(W) == 4 || sizeof(W) == 8),
                 "bitset words must be 32- or 64-bit integers");
   using U = typename std::make_unsigned<W>::type;
   static constexpr int bits = sizeof(W) * 8;
   static constexpr U   all  = ~U(0);
   static int  nbWords(int sz)     { return sz / bits + (sz % bits != 0); }
   static W    mask(int pos)       { return W(U(1) << (bits - 1 - pos % bits)); }
   static W    lastWord(int sz)    { return W(all << (bits - sz % bits)); } // first sz % bits bits set
   static int  popcount(W w)       { return __builtin_popcountll((unsigned long long)U(w)); }
};

template <class W = uint64_t> class StaticBitSet {
   using BW = BitWord<W>;
   std::vector<W>              _words;
   int                         _sz;
public:
   using Word = W;
   StaticBitSet() {}
   StaticBitSet(int sz) : _words(BW::nbWords(sz),W(BW::all)),_sz(sz) {
      if (sz % BW::bits)
         _words.back() = BW::lastWord(sz);
   }
   StaticBitSet(StaticBitSet&& bs) : _words(std::move(bs._words)),_sz(bs._sz) {}
  //W operator[] (int i) { return _words[i];}
   W operator[] (int i) const { return _words[i];}
   void clear()                     { std::fill(_words.begin(),_words.end(),W(0));}
   void remove (int pos)            { _words[pos / BW::bits] &= ~BW::mask(pos);}
   bool contains(int pos) const     { return (_words[pos / BW::bits] & BW::mask(pos)) != 0;}
   void unionWith(const StaticBitSet& o) {          // this |= o
      for (auto i = 0u; i < _words.size(); i++)
         _words[i] |= o._words[i];
   }
   void intersectWith(const StaticBitSet& o) {      // this &= o
      for (auto i = 0u; i < _words.size(); i++)
         _words[i] &= o._words[i];
   }
   int count() const {                              // number of bits set
      int c = 0;
      for (W w : _words)
         c += BW::popcount(w);
      return c;
   }
   int intersectCount(const StaticBitSet& o) const { // number of bits set in this & o
      int c = 0;
      for (auto i = 0u; i < _words.size(); i++)
         c += BW::popcount(_words[i] & o._words[i]);
      return c;
   }
};

template <class W = uint64_t> class SparseBitSet {
   using BW = BitWord<W>;
   std::vector<trail<W>>       _words;  // length = nbWords
   std::vector<int>            _index;  // length = nbWords, position -> word; words at positions <= _limit are non-zero
   std::vector<int>            _pos;    // length = nbWords, word -> position (inverse of _index)
   std::vector<W>              _mask;   // length = nbWords
   trail<int>                  _limit;
   int                         _sz;
   int                         _nbWords;
   void swapOut(int i) {                // move the word at position i past _limit
      const int w = _index[i];
      const int last = _index[_limit];
      _index[i] = last;
      _pos[last] = i;
      _index[_limit] = w;
      _pos[w] = _limit;
      _limit = _limit - 1;
   }
public:
   using Word = W;
   SparseBitSet(Trailer::Ptr eng, Storage::Ptr store, int sz)
      : _sz(sz)
   {
      _nbWords = BW::nbWords(_sz);
      _limit = trail<int>(eng,_nbWords-1);
      for (int i = 0; i < _nbWords; i++) {
         _words.emplace_back(trail<W>(eng, W(BW::all)));
         _index.emplace_back(int(i));
         _pos.emplace_back(int(i));
         _mask.emplace_back(W(0));
      }
      if (_sz % BW::bits)
         _words[_nbWords - 1] = BW::lastWord(_sz);
   }
   bool isEmpty() { return _limit == -1;}
   void clearBit(int b) {
      const int at = b / BW::bits;     // word holding bit b
      const int i = _pos[at];          // its current position in _index
      if (i <= _limit) {               // otherwise the word is already zero
         _words[at] = _words[at] & ~BW::mask(b);
         if (_words[at] == 0)
            swapOut(i);
      }
   }
   void clearMask() {
      for (int i = 0; i <= _limit; i++)
         _mask[_index[i]] = 0;
   }
   void reverseMask() {
      for (int i = 0; i <= _limit; i++)
         _mask[_index[i]] = ~(_mask[_index[i]]);
   }
   void addToMask(const StaticBitSet<W>& m) {
      for (int i = 0; i <= _limit; i++) {
         const int offset = _index[i];
         _mask[offset] = (_mask[offset] | m[offset]);
      }
   }
   void intersectWithMask() {
      for (int i = _limit; i >= 0; i--) {
         const int offset = _index[i];
         const W w = (_words[offset] & _mask[offset]);
         _words[offset] = w;
         if (w == 0)
            swapOut(i);
      }
   }
   int intersectIndex(const StaticBitSet<W>& m) {
      for (int i = 0; i <= _limit; i++) {
         const int offset = _index[i];
         if ((_words[offset] & m[offset]) != 0)
            return offset;
      }
      return -1;
   }
   trail<W>& operator[] (int i) { return _words[i];}
   W operator[] (int i) const { return _words[i].value();}
   friend std::ostream& operator<<(std::ostream& os,const SparseBitSet& sbs) {
      os << "L=" << sbs._limit << " S="<< sbs._sz << " #W=" << sbs._nbWords;
      os << "\tIDX=[";
      for(auto i=0;i<=sbs._limit;i++)
         os << sbs._index[i] << ' ';
      os << "] WORDS=[";
      for(auto i=0;i<=sbs._limit;i++) {
         std::bitset<BW::bits> wi((typename BW::U)sbs._words[sbs._index[i]].value());
         os << wi << ' ';
      }
      return os << ']';
   }
};

#endif
