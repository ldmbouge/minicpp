#ifndef __BITDOMAIN_H
#define __BITDOMAIN_H

#include <vector>
#include <utility>
#include "handle.hpp"
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
    const int        _imin,_imax;
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
};

#endif
