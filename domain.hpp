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

#ifndef __BITDOMAIN_H
#define __BITDOMAIN_H

#include <assert.h>
#include "handle.hpp"
#include "trailable.hpp"
#include "store.hpp"

#define GETBIT(b) ((_dom[((b) - _imin)>>5] & (0x1 << (((b)-_imin) & 0x1f)))!=0)

struct IntNotifier   {
    virtual void empty() = 0;
    virtual void bind() = 0;
    virtual void change() = 0;
    virtual void changeMin() = 0;
    virtual void changeMax() = 0;
};
 
class BitDomain {
    trail<int>*               _dom;
    trail<int>       _min,_max,_sz;
    const int        _imin,_imax, _nbw;
    int count(int from,int to) const;
    int findMin(int from) const;
    int findMax(int from) const;
    void setZero(int at);
public:
    typedef handle_ptr<BitDomain>  Ptr;
    BitDomain(Trailer::Ptr eng,Storage::Ptr store,int min,int max);
    int min() const { return _min;}
    int max() const { return _max;}
    int size() const { return _sz;}
    bool isBound() const { return _sz == 1;}
    bool member(int v) const { return _min <= v && v <= _max && GETBIT(v);}
    
    void assign(int v,IntNotifier& x);
    void remove(int v,IntNotifier& x);
    void removeBelow(int newMin,IntNotifier& x);
    void removeAbove(int newMax,IntNotifier& x);
    friend std::ostream& operator<<(std::ostream& os,const BitDomain& x);

    class iterator : public std::iterator<std::input_iterator_tag,short,short> {
        trail<int>* _dom;
        int _cw;             // current word
        int _cwi;            // current word index
        const int _imin, _nbw, _a, _o;  
        iterator(trail<int>* dom, const int nbw, const int imin, const int a, const int o) 
          : _dom(dom), _cw(dom->value()), _cwi(0), _nbw(nbw), _imin(imin), _a(a), _o(o) {}    // begin constructor
        iterator(trail<int>* dom, const int nbw) 
          : _dom(dom), _cw(0), _cwi(nbw), _nbw(nbw), _imin(0), _a(1), _o(0) {}    // end constructor
    public:
        iterator& operator++();
        bool operator==(iterator) const;
        bool operator!=(iterator) const;
        int operator*() const;
        friend class BitDomain;
    };
    iterator begin(const int a, const int o) const { return iterator(_dom, _nbw, _imin, a, o);}
    iterator end() const {return iterator(_dom, _nbw);}
};

class SparseSet {
    std::vector<int> _values;
    std::vector<int> _indexes;
    trail<int>       _size,_min,_max;
    int              _ofs,_n;
    void exchangePositions(int val1,int val2);
    bool checkVal(int val) const { assert( val <= _values.size()-1);return true;}
    bool internalContains(int val) const {
        if (val < 0 || val >= _n)
            return false;
        else return _indexes[val] < _size;
    }
    void updateBoundsValRemoved(int val);
    void updateMaxValRemoved(int val);
    void updateMinValRemoved(int val);
public:
    SparseSet(Trailer::Ptr eng,int n,int ofs);
    bool isEmpty() const { return _size == 0;}
    int size() const { return _size;}
    int min() const { return _min + _ofs;}
    int max() const { return _max + _ofs;}
    bool contains(int val) const {
        val -= _ofs;
        if (val < 0 || val >= _n) return false;
        return _indexes[val] < _size;
    }
    bool remove(int val);
    void removeAllBut(int v);
    void removeAll() { _size = 0;}
    void removeBelow(int value);
    void removeAbove(int value);
};

class SparseSetDomain {
    SparseSet _dom;
public:
    typedef handle_ptr<SparseSetDomain>  Ptr;
    SparseSetDomain(Trailer::Ptr trail,Storage::Ptr store,int min,int max)
        : _dom(trail,max - min + 1,min) {}
    int min() const { return _dom.min();}
    int max() const { return _dom.max();}
    int size() const { return _dom.size();}
    bool member(int v) const { return _dom.contains(v);}
    bool isBound() const { return _dom.size() == 1;}

    void assign(int v,IntNotifier& x);
    void remove(int v,IntNotifier& x);
    void removeBelow(int newMin,IntNotifier& x);
    void removeAbove(int newMax,IntNotifier& x);
    friend std::ostream& operator<<(std::ostream& os,const SparseSetDomain& x);
};
    
#endif
