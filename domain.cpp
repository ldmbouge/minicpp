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
 */

#include "domain.hpp"
#include "gfl/BitUtils.hpp"
#include "fail.hpp"
#include <iostream>

#if defined(_WIN64)
#include <intrin.h>
#endif

BitDomain::BitDomain(Trailer::Ptr eng,Storage::Ptr store,int min,int max)
    : _min(eng,min),
      _max(eng,max),
      _sz(eng,max - min + 1),
      _imin(min),
      _nWords((max - min + 1 + 31) / 32)
{
   const int nb = _nWords; // number of 32-bit words
   _dom = (trail<int>*)store->allocate(sizeof(trail<int>) * nb); // allocate storage from stack allocator
   for(int i=0;i<nb;i++)
      new (_dom+i) trail<int>(eng,0xffffffff);  // placement-new for each reversible.
   const bool partial = _sz & 0x1f;
   if (partial)
      _dom[nb - 1] = _dom[nb - 1] & ~(0xffffffff << (max - min + 1) % 32);
}

// Copies the domain out as a bitmap over the window [min,max], a whole 32-bit
// word of values at a time rather than a value at a time. The value `min + k`
// lands at bit `31 - (k % 32)` of `dst[k / 32]`, most significant bit first.
//
// Only the words spanning [_min,_max] are written; the rest are left alone, so
// the caller clears `dst` (or the region it cares about) beforehand. The window
// need not line up with the domain's own word grid -- the bits are shifted into
// place -- but a caller that shares one buffer between variables wants `min` to
// be the same for all of them.
void BitDomain::dump(int min,int max,unsigned int* dst) const
{
   assert(min <= _min && _max <= max);
   const int delta = min - _imin;   // window origin in the domain's coordinates
   const int lo = _min - min, hi = _max - min;

   auto word = [this](int i) noexcept -> unsigned int {
      return 0 <= i && i < _nWords ? (unsigned int)_dom[i] : 0u;
   };

   for(int d = lo >> 5; d <= hi >> 5; ++d) {
      // Gather the 32 values of this destination word, which straddle two
      // source words whenever the window is not aligned with the domain
      const int q = delta + (d << 5);
      const int s = q >> 5, b = q & 0x1f;   // arithmetic shift: floor for q < 0
      gfl::u32 bits = word(s) >> b;
      if (b) bits |= word(s + 1) << (32 - b);

      // Values outside [_min,_max] are not ours to report: the words the domain
      // keeps may still carry stale bits below _min or above _max. The masks
      // clamp on their own when this word lies wholly inside the live range
      const int lob = lo - (d << 5), hib = hi - (d << 5);
      bits &= gfl::highMask<gfl::u32>(32 - lob) & gfl::lowMask<gfl::u32>(hib + 1);

      // The domain counts bits up from the least significant end, the bitmap
      // reads them down from the most significant one
      dst[d] = gfl::bitReverse<gfl::u32>(bits);
   }
}

int BitDomain::count(int from,int to) const
{
    from = from  - _imin;
    to   = to - _imin + 1;
    int fw = from >> 5,tw = to >> 5;
    int fb = from & 0x1f,tb = to & 0x1f;
    int nc = 0;
    if (fw == tw) {
        const unsigned int wm = (0xFFFFFFFF << fb) & ~(0xFFFFFFFF << tb);
        const unsigned int bits = _dom[fw] & wm;
#if defined(_WIN64)
        nc = __popcnt(bits);
#else
        nc = __builtin_popcount(bits);
#endif
    } else {
      unsigned int wm = (0xFFFFFFFF << fb);
      unsigned int bits;
      while (fw < tw) {
          bits = _dom[fw] & wm;
#if defined(_WIN64)
          nc += __popcnt(bits);
#else
          nc += __builtin_popcount(bits);
#endif
          fw += 1;
          wm = 0xFFFFFFFF;
      }
      wm = ~(0xFFFFFFFF << tb);
      bits = _dom[fw] & wm;
#if defined(_WIN64)
      nc += __popcnt(bits);
#else
      nc += __builtin_popcount(bits);
#endif
    }
    return nc;
}

int BitDomain::findMin(int from) const
{
    from -= _imin;
    int mw = from >> 5, mb = from & 0x1f;
    unsigned int mask = 0x1 << mb;
    while ((_dom[mw] & mask) == 0) {
        mask <<= 1;
        ++mb;
        if (mask == 0) {
            ++mw;
            mb = 0;
            mask = 0x1;
        }
    }
    return _imin  + ((mw << 5) + mb);
}
int BitDomain::findMax(int from) const
{
    from -= _imin;
    int mw = from >> 5, mb = from & 0x1f;
    unsigned int mask = 0x1 << mb;
    while ((_dom[mw] & mask) == 0) {
        mask >>= 1;
        --mb;
        if (mask == 0) {
            --mw;
            mb = 31;
            mask = 0x80000000;
        }
    }
    return _imin + ((mw << 5) + mb);
}

void BitDomain::assign(int v,IntNotifier& x)  // removeAllBut(v,x)
{
    if (_sz == 1 && v == _min)
        return;
    if (v < _min || v > _max || !GETBIT(v)) {
        _sz = 0;
        x.empty();
        return;
    }
    bool minChanged = _min != v;
    bool maxChanged = _max != v;
    _min = v;
    _max = v;
    _sz  = 1;
    x.bind();
    x.change();
    if (minChanged) x.changeMin();
    if (maxChanged) x.changeMax();
}

void BitDomain::remove(int v,IntNotifier& x)
{
   const int theMin = _min,theMax = _max;
    if (v < theMin || v > theMax)
        return;
    const bool minChanged = v == theMin;
    const bool maxChanged = v == theMax;
    if (minChanged) {
        const int sz = _sz -= 1;
       _min = findMin(theMin + 1);
        x.changeMin();
        if (sz == 1) x.bind();
        if (sz == 0) x.empty();
        x.change();
    } else if (maxChanged) {
        const int sz = _sz -= 1;
        _max = findMax(theMax - 1);
        x.changeMax();
        if (sz == 1) x.bind();
        if (sz == 0) x.empty();
        x.change();
    } else if (member(v)) {
        setZero(v);
        const int sz = _sz -= 1;
        if (sz == 1) x.bind();
        if (sz == 0) x.empty();
        x.change();
    }
}

void BitDomain::removeBelow(int newMin,IntNotifier& x)
{
    if (newMin <= _min)
        return;
    if (newMin > _max)
        x.empty();
    bool isCompact = (_max - _min + 1) == _sz;
    int nbRemove = isCompact ? newMin - _min : count(_min,newMin - 1);
    _sz = _sz - nbRemove;
    if (!isCompact)
        newMin = findMin(newMin);
    _min = newMin;
    x.changeMin();
    x.change();
    if (_sz==0) x.empty();
    if (_sz==1) x.bind();
}

void BitDomain::removeAbove(int newMax,IntNotifier& x)
{
    if (newMax >= _max)
        return;
    if (newMax < _min)
        x.empty();
    bool isCompact = (_max - _min + 1) == _sz;
    int nbRemove = isCompact ? _max - newMax : count(newMax + 1,_max);
    _sz = _sz - nbRemove;
    if (!isCompact)
        newMax = findMax(newMax);
    _max = newMax;
    x.changeMax();
    x.change();
    if (_sz==0) x.empty();
    if (_sz==1) x.bind();
}

std::ostream& operator<<(std::ostream& os,const BitDomain& x)
{
    if (x.size()==1)
        os << x.min();
    else {
        os << '(' << x.size() << ")[";
        bool first = true;
        bool seq = false;
        int lastIn=x._min;
        for(int k = x._min;k <= x._max;k++) {
            if (x.member(k)) {
                if (first) {
                    os << k;
                    first = seq = false;
                    lastIn = k;
                } else {
                    if (lastIn + 1 == k) {
                        lastIn = k;
                        seq = true;
                    } else {
                        if (seq)
                            os << ".." << lastIn << ',' << k;
                        else
                            os << ',' << k;
                        lastIn = k;
                        seq = false;
                    }
                }
            }
        }
        if (seq)
            os << ".." << lastIn << ']';
        else os << ']';
    }
    return os;
}


// ======================================================================
// SparseSet

SparseSet::SparseSet(Trailer::Ptr eng,int n,int ofs)
    : _values(n),
      _indexes(n),
      _size(eng,n),
      _min(eng,0),
      _max(eng,n-1),
      _ofs(ofs),
      _n(n)
{
    for(int i=0;i < n;i++)
        _values[i] = _indexes[i] = i;
}

void SparseSet::exchangePositions(int val1,int val2)
{
    int i1 = _indexes[val1],i2 = _indexes[val2];
    _values[i1] = val2;
    _values[i2] = val1;
    _indexes[val1] = i2;
    _indexes[val2] = i1;
}

void SparseSet::updateBoundsValRemoved(int val)
{
    updateMaxValRemoved(val);
    updateMinValRemoved(val);
}
void SparseSet::updateMaxValRemoved(int val)
{
    if (!isEmpty() && _max == val) {
        assert(!internalContains(val));
        for(int v=val-1; v >= _min;v--) {
            if (internalContains(v)) {
                _max = v;
                return;
            }
        }
    }
}
void SparseSet::updateMinValRemoved(int val)
{
    if (!isEmpty() && _min == val) {
        assert(!internalContains(val));
        for(int v=val+1;v <= _max;v++) {
            if (internalContains(v)) {
                _min = v;
                return;
            }
        }
    }
}

bool SparseSet::remove(int val)
{
    if (!contains(val))
        return false;
    val -= _ofs;
    assert(checkVal(val));
    int s = _size;
    exchangePositions(val,_values[s - 1]);
    _size = s - 1;
    updateBoundsValRemoved(val);
    return true;
}

void SparseSet::removeAllBut(int v)
{
    assert(contains(v));
    v -= _ofs;
    assert(checkVal(v));
    int val = _values[0];
    int index = _indexes[v];
    _indexes[v] = 0;
    _values[0] = v;
    _indexes[val] = index;
    _values[index] = val;
    _min = v;
    _max = v;
    _size = 1;
}

void SparseSet::removeBelow(int value)
{
    if (max() < value)
        removeAll();
    else
        for(int v= min() ; v < value;v++)
            remove(v);
}

void SparseSet::removeAbove(int value)
{
    if (min() > value)
        removeAll();
    else
        for(int v = value + 1; v <= max();v++)
            remove(v);
}

void SparseSetDomain::assign(int v,IntNotifier& x)
{
    if (_dom.contains(v)) {
        if (_dom.size() != 1) {
            bool maxChanged = max() != v;
            bool minChanged = min() != v;
            _dom.removeAllBut(v);
            if (_dom.size() == 0)
                x.empty();
            x.bind();
            x.change();
            if (maxChanged) x.changeMax();
            if (minChanged) x.changeMin();
        }
    } else {
        _dom.removeAll();
        x.empty();
    }
}

void SparseSetDomain::remove(int v,IntNotifier& x)
{
    if (_dom.contains(v)) {
        bool maxChanged = max() == v;
        bool minChanged = min() == v;
        _dom.remove(v);
        if (_dom.size()==0)
            x.empty();
        x.change();
        if (maxChanged) x.changeMax();
        if (minChanged) x.changeMin();
        if (_dom.size()==1) x.bind();
    }
}

void SparseSetDomain::removeBelow(int newMin,IntNotifier& x)
{
    if (_dom.min() < newMin) {
        _dom.removeBelow(newMin);
        switch(_dom.size()) {
            case 0: x.empty();break;
            case 1: x.bind();
            default:
                x.changeMin();
                x.change();
                break;
        }
    }
}

void SparseSetDomain::removeAbove(int newMax,IntNotifier& x)
{
    if (_dom.max()  > newMax) {
        _dom.removeAbove(newMax);
        switch(_dom.size()) {
            case 0: x.empty();break;
            case 1: x.bind();
            default:
                x.changeMax();
                x.change();
                break;
        }
    }
}

std::ostream& operator<<(std::ostream& os,const SparseSetDomain& x)
{
    os << '{';
    for(int i = x.min();i <= x.max()-1;i++)
        if (x.member(i))
            os << i << ',';
    if (x.size() > 0) os << x.max();
    os << '}';
    return os;
}

