#ifndef __BITDOMAIN_H
#define __BITDOMAIN_H

#include <vector>
#include <utility>
#include "handle.hpp"
#include "engine.hpp"

#define GETBIT(b) ((_dom[((b) - _imin)>>5] & (0x1 << (((b)-_imin) & 0x1f)))!=0)

struct IntNotifier   {
   virtual void bindEvt() = 0;
   virtual void domEvt(int sz) = 0;
   virtual void updateMinEvt(int sz) = 0;
   virtual void updateMaxEvt(int sz) = 0;
};
 
class BitDomain {
   Engine::Ptr          _engine;       // for memory management (custom stack allocator)
   rev<int>*               _dom;
   rev<int>       _min,_max,_sz;
   const int        _imin,_imax;
   int count(int from,int to) const;
   int findMin(int from) const;
   int findMax(int from) const;
   void setZero(int at);
public:
   typedef handle_ptr<BitDomain>  Ptr;
   BitDomain(Engine::Ptr eng,int min,int max);
   int getMin() const { return _min;}
   int getMax() const { return _max;}
   int getSize() const { return _sz;}
   bool isBound() const { return _sz == 1;}
   bool member(int v) const { return _min <= v && v <= _max && GETBIT(v);}

   void bind(int v,IntNotifier& x);
   void remove(int v,IntNotifier& x);
   void updateMin(int newMin,IntNotifier& x);
   void updateMax(int newMax,IntNotifier& x);
   friend std::ostream& operator<<(std::ostream& os,const BitDomain& x);
};

#endif
