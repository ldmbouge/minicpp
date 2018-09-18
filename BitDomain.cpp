#include "BitDomain.hpp"
#include "fail.hpp"
#include <iostream>

BitDomain::BitDomain(Trailer::Ptr eng,Storage::Ptr store,int min,int max)
    : _min(eng,min),
      _max(eng,max),
      _sz(eng,max - min + 1),
      _imin(min),
      _imax(max)
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
        nc = __builtin_popcount(bits);
    } else {
      unsigned int wm = (0xFFFFFFFF << fb);
      unsigned int bits;
      while (fw < tw) {
          bits = _dom[fw] & wm;
          nc += __builtin_popcount(bits);
          fw += 1;
          wm = 0xFFFFFFFF;
      }
      wm = ~(0xFFFFFFFF << tb);
      bits = _dom[fw] & wm;
      nc += __builtin_popcount(bits);
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
    x.bind();
    x.change();
    if (minChanged) x.changeMin();
    if (maxChanged) x.changeMax();
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
        x.changeMin();
        if (_sz == 1) x.bind();
        if (_sz == 0) x.empty();
        x.change();
    } else if (maxChanged) {
        _sz = _sz - 1;
        _max = findMax(_max - 1);
        x.changeMax();
        if (_sz == 1) x.bind();
        if (_sz == 0) x.empty();
        x.change();
    } else if (member(v)) {
        setZero(v);
        _sz = _sz - 1;
        if (_sz == 1) x.bind();
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
        int lastIn=x._min,firstIn = x._min;
        for(int k = x._min;k <= x._max;k++) {
            if (x.member(k)) {
                if (first) {
                    os << k;
                    first = seq = false;
                    lastIn = firstIn = k;
                } else {
                    if (lastIn + 1 == k) {
                        lastIn = k;
                        seq = true;
                    } else {
                        if (seq)
                            os << ".." << lastIn << ',' << k;
                        else
                            os << ',' << k;
                        firstIn = lastIn = k;
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
