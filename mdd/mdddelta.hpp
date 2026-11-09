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
   MDDSpec*       _spec;
   MDDNodeFactory*  _nf;
   Pool::Ptr      _pool;
   MDDStateDelta**   _t;
   int             _csz;
   MDDStateDelta _empty;
   const enum Direction  _dir;
   const int _np; // number of properties
   const MDDProperty::Ptr* _pa; // property array
   void adaptDelta();
   static int nbProps(MDDSpec& spec,const enum Direction dir) {
      switch (dir) {
         case Down: return spec._nbpDown;
         case Up: return spec._nbpUp;
         case Bi: return spec._nbpCombined;
         default: return -1;
      }
   }
   static const MDDProperty::Ptr* propArray(MDDSpec& spec,const enum Direction dir) {
      switch (dir) {
         case Down: return spec._attrsDown;
         case Up: return spec._attrsUp;
         case Bi: return spec._attrsCombined;
         default: return nullptr; break;
      }
   }
public:
   MDDDelta(MDDSpec& spec,MDDNodeFactory* nf,int nb,enum Direction dir)
      : _spec(&spec),
        _nf(nf),_pool(new Pool),
        _t(nullptr),_csz(0),
        _empty(nullptr,new long long[propNbWords(nb)],nb),
        _dir(dir),
        _np(nbProps(spec,dir)),
        _pa(propArray(spec,dir))
   {}
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
         entry = new (_pool) MDDStateDelta(_spec,
                                           new (_pool) long long[propNbWords(_spec->size(_dir))],
                                           _spec->size(_dir));
      } else
         entry->clear();
      return entry;
   }
   void setDelta(MDDNode* n,const MDDState& ns) {      
      MDDStateDelta* entry;
      if (n->getId() >= _csz) entry = makeDelta(n);
      else {
        entry = _t[n->getId()];
        if (!entry) entry = makeDelta(n);
      }
      const MDDState* nState = stateFrom(n);
      for(int p=0;p < _np;++p)
         if (_pa[p]->diff(nState->_mem,ns._mem))
            entry->setProp(p);                             
   }
   void setDelta(MDDNode* n,const MDDState& ns,const MDDPropSet& ps) {
      MDDStateDelta* entry;
      if (n->getId() >= _csz) entry = makeDelta(n);
      else {
        entry = _t[n->getId()];
        if (!entry) entry = makeDelta(n);
      }
      const MDDState* nState = stateFrom(n);
      for(auto p : ps)
         if (_pa[p]->diff(nState->_mem,ns._mem))
            entry->setProp(p);
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
