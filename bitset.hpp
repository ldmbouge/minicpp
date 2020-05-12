#ifndef __BITSET_H
#define __BITSET_H

#include "trailable.hpp"
#include "handle.hpp"
#include "store.hpp"
#include <vector>

class StaticBitSet {
    std::vector<int>            _words;
    int                         _sz;
    // int                         _imin;
    // int                         _imax;
    public:
        StaticBitSet() {}
        StaticBitSet(int sz);
        StaticBitSet(StaticBitSet&& bs) : _words(std::move(bs._words)),_sz(bs._sz) {}
        int operator[] (int i) { return _words[i];}
        void remove (int pos);
        bool contains(int pos);
};

class SparseBitSet {
    std::vector<trail<int>>     _words;  // length = nbWords
    std::vector<int>            _index;  // length = nbWords
    std::vector<int>            _mask;   // length = nbWords
    trail<int>                  _limit;
    int                         _sz;
    int                         _nbWords;
    public:
        SparseBitSet(Trailer::Ptr eng, Storage::Ptr store, int sz);
        bool isEmpty() { return _limit == -1;}
        void clearMask();
        void reverseMask();
        void addToMask(StaticBitSet& m);
        void intersectWithMask();
        int intersectIndex(StaticBitSet& m);
        trail<int>& operator[] (int i) { return _words[i];}
        int operator[] (int i) const { return _words[i].value();}
};



#endif