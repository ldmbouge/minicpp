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
   operator MDDPropSet&()             { return _set;}
   operator const MDDPropSet&() const { return _set;}
   void setProp(int p)       noexcept { _set.setProp(p);}
   void clear() noexcept              { _set.clear();}
   short size() const noexcept        { return _set.size();}
   void add(const MDDPropSet& ps)     { _set.unionWith(ps);}
   friend std::ostream& operator<<(std::ostream& os,const MDDStateDelta& sd) {
      return os << sd._set;
   }
};

class MDDDelta {
   MDDSpec&       _spec;
   MDDNodeFactory*  _nf;
   Pool::Ptr      _pool;
   MDDStateDelta**   _t;
   int             _csz;
   MDDStateDelta _empty;
   enum Direction  _dir;
   void adaptDelta();
public:
   MDDDelta(MDDSpec& spec,MDDNodeFactory* nf,int nb,enum Direction dir)
      : _spec(spec),_nf(nf),_pool(new Pool),
        _t(nullptr),_csz(0),
        _empty(nullptr,new long long[propNbWords(nb)],nb),
        _dir(dir)
   {}
   const MDDProperty::Ptr propAt(int i) const noexcept {
      switch (_dir) {
         case Down: return _spec._attrsDown[i]; break;
         case Up: return _spec._attrsUp[i]; break;
         case Bi: return _spec._attrsCombined[i]; break;
         default: return nullptr; break;
      }
   }
   int nbp() const noexcept {
      switch (_dir) {
         case Down: return _spec._nbpDown; break;
         case Up: return _spec._nbpUp; break;
         case Bi: return _spec._nbpCombined; break;
         default: return -1; break;
      }
   }
   void clear() {
      _pool->clear();
      memset(_t,'\0',sizeof(MDDStateDelta*)*_csz);
   }
   const MDDState* stateFrom(MDDNode* n) {
      switch (_dir) {
         case Down: return &(n->getDownState()); break;
         case Up: return &(n->getUpState()); break;
         case Bi: return &(n->getCombinedState()); break;
         default: return nullptr; break;
      }
   }
   MDDStateDelta* makeDelta(MDDNode* n) {
      if (_nf->peakNodes() > _csz) adaptDelta();
      auto& entry = _t[n->getId()];
      if (entry == nullptr) {
         MDDStateSpec* spec =  stateFrom(n)->getSpec();
         entry = new (_pool) MDDStateDelta(spec,new (_pool) long long[propNbWords(spec->size(_dir))],spec->size(_dir));
      } else
         entry->clear();
      return entry;
   }
   void setDelta(MDDNode* n,const MDDState& ns) {      
      auto entry = makeDelta(n);
      const MDDState* nState = stateFrom(n);
      for(int p=0;p < nbp();++p)
         if (propAt(p)->diff(nState->_mem,ns._mem))
            entry->setProp(p);                             
      //stateFrom(n)->diffWith(ns,*entry);
   }
   void setDelta(MDDNode* n,const MDDState& ns,const MDDPropSet& ps) {
      auto entry = makeDelta(n);
      const MDDState* nState = stateFrom(n);
      for(auto p : ps)
         if (propAt(p)->diff(nState->_mem,ns._mem))
            entry->setProp(p);
      //stateFrom(n)->diffWith(ps,ns,*entry);
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
