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

#ifndef mdddelta_hpp
#define mdddelta_hpp

#include "store.hpp"
#include "mddnode.hpp"
#include "utilities.hpp"

class MDDStateDelta {
   MDDPropSet _set;
public:
   MDDStateDelta(const MDDStateSpec* spec,long long* buf,int nbp)
      : _set(buf,nbp) {}
   operator MDDPropSet&() { return _set;}
   operator const MDDPropSet&() const { return _set;}
   void clear() noexcept { _set.clear();}
   short size() const noexcept { return _set.size();}
   void add(const MDDPropSet& ps) { _set.unionWith(ps);}
   friend std::ostream& operator<<(std::ostream& os,const MDDStateDelta& sd) {
      return os << sd._set;
   }
};

class MDDDelta {
   MDDNodeFactory*  _nf;
   Pool::Ptr      _pool;
   MDDStateDelta**   _t;
   int             _csz;
   MDDStateDelta _empty;
   void adaptDelta();
public:
   MDDDelta(MDDNodeFactory* nf,int nb)
      : _nf(nf),_pool(new Pool),
        _t(nullptr),_csz(0),
        _empty(nullptr,new long long[propNbWords(nb)],nb)
   {
   }
   void clear() {
      _pool->clear();
      memset(_t,'\0',sizeof(MDDStateDelta*)*_csz);
   }
   MDDStateDelta* makeDelta(MDDNode* n) {
      if (_nf->peakNodes() > _csz) adaptDelta();
      auto& entry = _t[n->getId()];
      if (entry == nullptr) {
         MDDStateSpec* spec =  n->getState().getSpec();
         entry = new (_pool) MDDStateDelta(spec,new (_pool) long long[propNbWords(spec->size())],spec->size());
      } else
         entry->clear();
      return entry;
   }
   void setDelta(MDDNode* n,const MDDState& ns) {      
      auto entry = makeDelta(n);
      n->getState().diffWith(ns,*entry);
   }
   void setDelta(MDDNode* n,const MDDPropSet& ps) {
      auto entry = makeDelta(n);
      entry->add(ps);
   }
   const MDDStateDelta& getDelta(MDDNode* n) {
      if (n->getId() >= _csz)
         return _empty;
      auto entry = _t[n->getId()];
      if (entry)
         return *entry;
      else return _empty;
   }
   friend std::ostream& operator<<(std::ostream& os,const MDDDelta& d) {
      for(int i=0;i < d._csz;i++) {
         if (d._t[i] && d._t[i]->size() > 0) {
            os << "Delta[" << i << "] = " << *d._t[i] << '\n';
         }
      }
      return os;
   }
};

#endif
