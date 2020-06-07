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
#include "fail.hpp"
#include <iostream>

#if defined(_WIN64)
#include <intrin.h>
#endif

BitDomain::BitDomain(Trailer::Ptr eng,Storage::Ptr store,int min,int max)
    : _min(eng,min),
      _max(eng,max),
      _sz(eng,max - min + 1),
      _imin(min)
{
    const int nb = (_sz >> 5) + ((_sz & 0x1f) != 0); // number of 32-bit words
    _dom = (trail<int>*)store->allocate(sizeof(trail<int>) * nb); // allocate storage from stack allocator
    for(int i=0;i<nb;i++)
       new (_dom+i) trail<int>(eng,0xffffffff);  // placement-new for each reversible.
    const bool partial = _sz & 0x1f;
    if (partial)
        _dom[nb - 1] = _dom[nb - 1] & ~(0xffffffff << (max - min + 1) % 32);    
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
void BitDomain::setZero(int at)
{
    at -= _imin;
    const int mw = at >> 5,  mb = at & 0x1f;
    _dom[mw] = _dom[mw] & ~(0x1 << mb);
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
    x.bind(v);
    x.change();
    if (minChanged) x.changeMin(v);
    if (maxChanged) x.changeMax(v);
}

void BitDomain::remove(int v,IntNotifier& x)
{
    if (v < _min || v > _max)
        return;
    if (_min.value() == _max.value())
        x.empty();
    bool minChanged = v == _min;
    bool maxChanged = v == _max;
    if (minChanged) {
        _sz = _sz - 1;
       _min = findMin(_min + 1);
        x.changeMin(_min);
        if (_sz == 1) x.bind(_min);
        if (_sz == 0) x.empty();
        x.change();
        x.remove(v);
    } else if (maxChanged) {
        _sz = _sz - 1;
        _max = findMax(_max - 1);
        x.changeMax(_max);
        if (_sz == 1) x.bind(_max);
        if (_sz == 0) x.empty();
        x.change();
        x.remove(v);
    } else if (member(v)) {
        setZero(v);
        x.remove(v);
        _sz = _sz - 1;
        if (_sz == 1) x.bind(min());
        if (_sz == 0) x.empty();
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
    x.changeMin(_min);
    x.change();
    if (_sz==0) x.empty();
    if (_sz==1) x.bind(min());
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
    x.changeMax(_max);
    x.change();
    if (_sz==0) x.empty();
    if (_sz==1) x.bind(min());
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
            x.bind(v);
            x.change();
            if (maxChanged) x.changeMax(v);
            if (minChanged) x.changeMin(v);
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
        if (maxChanged) x.changeMax(max());
        if (minChanged) x.changeMin(min());
        if (_dom.size()==1) x.bind(min());
    }
}

void SparseSetDomain::removeBelow(int newMin,IntNotifier& x)
{
    if (_dom.min() < newMin) {
        _dom.removeBelow(newMin);
        switch(_dom.size()) {
            case 0: x.empty();break;
            case 1: x.bind(min());
            default:
                x.changeMin(min());
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
            case 1: x.bind(min());
            default:
                x.changeMax(max());
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

