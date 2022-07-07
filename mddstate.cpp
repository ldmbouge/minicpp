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
 *
 * Contributions by Waldemar Cruz, Rebecca Gentzel, Willem Jan Van Hoeve
 */

#include "mddstate.hpp"
#include "mddnode.hpp"
#include <algorithm>
#include <limits.h>
#include <future>
#include <thread>

void printSet(const MDDIntSet& s) {
   std::cout << s << std::endl;
}


namespace Factory {
   MDDProperty::Ptr makeProperty(short id,unsigned short ofs,int init,int max,enum RelaxWith rw)
   {
      MDDProperty::Ptr rv;
      if (max == 1)
         rv = new MDDPBit(id,ofs,init,rw);
      else if (max <= 127)
         rv = new MDDPByte(id,ofs,init,max,rw);
      else
         rv = new MDDPInt(id,ofs,init,max,rw);
      return rv;
   }
   MDDPInt::Ptr makePInt(short id,unsigned short ofs,int init,int max,enum RelaxWith rw)
   {
      MDDPInt::Ptr rv = new MDDPInt(id,ofs,init,max,rw);
      return rv;
   }
   MDDPBitSequence::Ptr makeBSProperty(short id,unsigned short ofs,int nbb,unsigned char init,enum RelaxWith rw)
   {
      MDDPBitSequence::Ptr rv = new MDDPBitSequence(id,ofs,nbb,init,rw);
      return rv;
   }
   MDDPSWindow<short>::Ptr makeWinProperty(short id,unsigned short ofs,int len,int init,int finit,enum RelaxWith rw)
   {
      MDDPSWindow<short>::Ptr rv = new MDDPSWindow<short>(id,ofs,len,init,finit,rw);
      return rv;
   }
}

MDDCstrDesc::MDDCstrDesc(const MDDCstrDesc& d)
   : _vars(d._vars), _vset(d._vset), _name(d._name),
     _propertiesDown(d._propertiesDown),
     _propertiesUp(d._propertiesUp),
     _propertiesCombined(d._propertiesCombined),
     _downTId(d._downTId),
     _upTId(d._upTId),
     _downRId(d._downRId),
     _upRId(d._upRId),
     _uid(d._uid),
     _sid(d._sid)
{}

MDDSpec::MDDSpec()
{
    _approximateSplitting = false;
    _nodePriorityAggregateStrategy = 1;
    _candidatePriorityAggregateStrategy = 1;
}

bool MDDCstrDesc::ownsDownProperty(int p) const
{
   auto at = std::find(_propertiesDown.begin(),_propertiesDown.end(),p);
   return at != _propertiesDown.end();
} 
bool MDDCstrDesc::ownsUpProperty(int p) const
{
   auto at = std::find(_propertiesUp.begin(),_propertiesUp.end(),p);
   return at != _propertiesUp.end();
}
bool MDDCstrDesc::ownsCombinedProperty(int p) const
{
   auto at = std::find(_propertiesCombined.begin(),_propertiesCombined.end(),p);
   return at != _propertiesCombined.end();
}


void MDDSpec::varOrder()
{
   std::sort(x.begin(),x.end(),[](const var<int>::Ptr& a,const var<int>::Ptr& b) {
      return a->getId() < b->getId();
   });
}

MDDStateSpec::MDDStateSpec()
{
   _mxpDown = 4;
   _mxpUp = 4;
   _mxpCombined = 4;
   _nbpDown = 0;
   _nbpUp = 0;
   _nbpCombined = 0;
   _attrsDown = new MDDProperty::Ptr[_mxpDown];
   _attrsUp = new MDDProperty::Ptr[_mxpUp];
   _attrsCombined = new MDDProperty::Ptr[_mxpCombined];
   _omapDown = nullptr;
   _omapUp = nullptr;
   _omapDownToCombined = nullptr;
   _omapUpToCombined = nullptr;
   _omapCombinedToDown = nullptr;
   _omapCombinedToUp = nullptr;
}

void MDDStateSpec::addDownProperty(MDDProperty::Ptr p) noexcept
{
   if (_nbpDown == _mxpDown) {
      MDDProperty::Ptr* ns = new MDDProperty::Ptr[_mxpDown<<1];
      for(size_t i =0;i < _nbpDown;i++)
         ns[i] = _attrsDown[i];
      delete[]_attrsDown;
      _attrsDown = ns;
      _mxpDown = _mxpDown << 1;
   }
   _attrsDown[_nbpDown++] = p;
   p->setDirection(Down);
}
void MDDStateSpec::addUpProperty(MDDProperty::Ptr p) noexcept
{
   if (_nbpUp == _mxpUp) {
      MDDProperty::Ptr* ns = new MDDProperty::Ptr[_mxpUp<<1];
      for(size_t i =0;i < _nbpUp;i++)
         ns[i] = _attrsUp[i];
      delete[]_attrsUp;
      _attrsUp = ns;
      _mxpUp = _mxpUp << 1;
   }
   _attrsUp[_nbpUp++] = p;
   p->setDirection(Up);
}
void MDDStateSpec::addCombinedProperty(MDDProperty::Ptr p) noexcept
{
   if (_nbpCombined == _mxpCombined) {
      MDDProperty::Ptr* ns = new MDDProperty::Ptr[_mxpCombined<<1];
      for(size_t i =0;i < _nbpCombined;i++)
         ns[i] = _attrsCombined[i];
      delete[]_attrsCombined;
      _attrsCombined = ns;
      _mxpCombined = _mxpCombined << 1;
   }
   _attrsCombined[_nbpCombined++] = p;
   p->setDirection(Bi);
}

void MDDStateSpec::layout()
{
   size_t lszBit = 0;
   for(size_t p = 0;p <_nbpDown;p++) {
      auto a = _attrsDown[p];
      lszBit = a->setOffset(lszBit);
   }
   size_t boB = lszBit & 0x7;
   if (boB != 0)
      lszBit = (lszBit | 0x7) + 1;
   _lszDown = lszBit >> 3;
   _lszDown = (_lszDown & 0x7) ? (_lszDown | 0x7)+1 : _lszDown;
   assert(_lszDown % 8 == 0); // # bytes is always a multiple of 8.
   std::cout << "Down State requires:" << _lszDown << " bytes" << std::endl;

   lszBit = 0;
   for(size_t p = 0;p <_nbpUp;p++) {
      auto a = _attrsUp[p];
      lszBit = a->setOffset(lszBit);
   }
   boB = lszBit & 0x7;
   if (boB != 0)
      lszBit = (lszBit | 0x7) + 1;
   _lszUp = lszBit >> 3;
   _lszUp = (_lszUp & 0x7) ? (_lszUp | 0x7)+1 : _lszUp;
   assert(_lszUp % 8 == 0); // # bytes is always a multiple of 8.
   std::cout << "Up State requires:" << _lszUp << " bytes" << std::endl;

   lszBit = 0;
   for(size_t p = 0;p <_nbpCombined;p++) {
      auto a = _attrsCombined[p];
      lszBit = a->setOffset(lszBit);
   }
   boB = lszBit & 0x7;
   if (boB != 0)
      lszBit = (lszBit | 0x7) + 1;
   _lszCombined = lszBit >> 3;
   _lszCombined = (_lszCombined & 0x7) ? (_lszCombined | 0x7)+1 : _lszCombined;
   assert(_lszCombined % 8 == 0); // # bytes is always a multiple of 8.
   std::cout << "Combined State requires:" << _lszCombined << " bytes" << std::endl;
}

MDDPBitSequence::Ptr MDDStateSpec::downBSState(MDDCstrDesc::Ptr d,int nbb,unsigned char init,enum RelaxWith rw, int cPriority)
{
   int aid = (int)_nbpDown;
   MDDPBitSequence::Ptr p = Factory::makeBSProperty(aid,0,nbb,init,rw); 
   addDownProperty(p);
   d->addDownProperty(aid);
   return p;
}

MDDPBitSequence::Ptr MDDStateSpec::upBSState(MDDCstrDesc::Ptr d,int nbb,unsigned char init,enum RelaxWith rw, int cPriority)
{
   int aid = (int)_nbpUp;
   MDDPBitSequence::Ptr p = Factory::makeBSProperty(aid,0,nbb,init,rw); 
   addUpProperty(p);
   d->addUpProperty(aid);
   return p;
}
MDDPBitSequence::Ptr MDDStateSpec::combinedBSState(MDDCstrDesc::Ptr d,int nbb,unsigned char init,enum RelaxWith rw, int cPriority)
{
   int aid = (int)_nbpCombined;
   MDDPBitSequence::Ptr p = Factory::makeBSProperty(aid,0,nbb,init,rw);
   addCombinedProperty(p);
   d->addCombinedProperty(aid);
   return p;
}
MDDPInt::Ptr MDDStateSpec::downIntState(MDDCstrDesc::Ptr d, int init,int max,enum RelaxWith rw, int cPriority)
{
   int aid = (int)_nbpDown;
   MDDPInt::Ptr p = Factory::makePInt(aid, 0, init, max,rw);
   addDownProperty(p);
   d->addDownProperty(aid);
   return p;
}
MDDPInt::Ptr MDDStateSpec::upIntState(MDDCstrDesc::Ptr d,int init,int max,enum RelaxWith rw,int cPriority)
{
   int aid = (int)_nbpUp;
   MDDPInt::Ptr p = Factory::makePInt(aid, 0, init, max,rw);
   addUpProperty(p);
   d->addUpProperty(aid);
   return p;
}
MDDPInt::Ptr MDDStateSpec::combinedIntState(MDDCstrDesc::Ptr d, int init,int max,enum RelaxWith rw, int cPriority)
{
   int aid = (int)_nbpCombined;
   MDDPInt::Ptr p = Factory::makePInt(aid, 0, init, max,rw);
   addCombinedProperty(p);
   d->addCombinedProperty(aid);
   return p;
}
MDDPSWindow<short>::Ptr MDDStateSpec::downSWState(MDDCstrDesc::Ptr d,int len,int init,int finit,enum RelaxWith rw, int constraintPriority)
{
   int aid = (int)_nbpDown;
   MDDPSWindow<short>::Ptr p =Factory::makeWinProperty(aid,0,len,init,finit,rw);
   addDownProperty(p);
   d->addDownProperty(aid);
   return p;
}
MDDPSWindow<short>::Ptr MDDStateSpec::upSWState(MDDCstrDesc::Ptr d,int len,int init,int finit,enum RelaxWith rw, int constraintPriority)
{
   int aid = (int)_nbpUp;
   MDDPSWindow<short>::Ptr p = Factory::makeWinProperty(aid,0,len,init,finit,rw);
   addUpProperty(p);
   d->addUpProperty(aid);
   return p;
}
MDDPSWindow<short>::Ptr MDDStateSpec::combinedSWState(MDDCstrDesc::Ptr d,int len,int init,int finit,enum RelaxWith rw, int constraintPriority)
{
   int aid = (int)_nbpCombined;
   MDDPSWindow<short>::Ptr p = Factory::makeWinProperty(aid,0,len,init,finit,rw);
   addCombinedProperty(p);
   d->addCombinedProperty(aid);
   return p;
}

// --------------------------------------------------------------------------------

std::vector<int> MDDStateSpec::addDownStates(MDDCstrDesc::Ptr d,int from, int to,int max, std::function<int(int)> clo)
{
   std::vector<int> res;
   for(int i = from; i <= to; i++)
      res.push_back(downIntState(d,clo(i),max)->getId());
   return res;
}
std::vector<int> MDDStateSpec::addUpStates(MDDCstrDesc::Ptr d,int from, int to,int max, std::function<int(int)> clo)
{
   std::vector<int> res;
   for(int i = from; i <= to; i++)
      res.push_back(upIntState(d,clo(i),max)->getId());
   return res;
}
std::vector<int> MDDStateSpec::addCombinedStates(MDDCstrDesc::Ptr d,int from, int to,int max, std::function<int(int)> clo)
{
   std::vector<int> res;
   for(int i = from; i <= to; i++)
      res.push_back(combinedIntState(d,clo(i),max)->getId());
   return res;
}
std::vector<int> MDDStateSpec::addDownStates(MDDCstrDesc::Ptr d,int max, std::initializer_list<int> inputs)
{
   std::vector<int> res;
   for(auto& v : inputs)
      res.push_back(downIntState(d,v,max)->getId());
   return res;
}
std::vector<int> MDDStateSpec::addUpStates(MDDCstrDesc::Ptr d,int max, std::initializer_list<int> inputs)
{
   std::vector<int> res;
   for(auto& v : inputs)
      res.push_back(upIntState(d,v,max)->getId());
   return res;
}
std::vector<int> MDDStateSpec::addCombinedStates(MDDCstrDesc::Ptr d,int max, std::initializer_list<int> inputs)
{
   std::vector<int> res;
   for(auto& v : inputs)
      res.push_back(combinedIntState(d,v,max)->getId());
   return res;
}

// --------------------------------------------------------------------------------
// MDDSpec
// --------------------------------------------------------------------------------

MDDPBitSequence::Ptr MDDSpec::downBSState(MDDCstrDesc::Ptr d,int nbb,unsigned char init,enum RelaxWith rw,int cPriority)
{
   auto rv = MDDStateSpec::downBSState(d,nbb,init,rw,cPriority);
   _propertiesByPriorities[cPriority].emplace_back(rv->getId());
   return rv;   
}
MDDPInt::Ptr MDDSpec::downIntState(MDDCstrDesc::Ptr d,int init,int max,enum RelaxWith rw, int cPriority)
{
   auto rv = MDDStateSpec::downIntState(d,init,max,rw,cPriority);
   _propertiesByPriorities[cPriority].emplace_back(rv->getId());
   return rv;
}
MDDPBitSequence::Ptr MDDSpec::upBSState(MDDCstrDesc::Ptr d,int nbb,unsigned char init,enum RelaxWith rw, int cPriority)
{
   auto rv = MDDStateSpec::upBSState(d,nbb,init,rw);
   _propertiesByPriorities[cPriority].emplace_back(rv->getId());
   return rv;
}
MDDPInt::Ptr MDDSpec::upIntState(MDDCstrDesc::Ptr d,int init,int max,enum RelaxWith rw, int cPriority)
{
   auto rv = MDDStateSpec::upIntState(d,init,max,rw,cPriority);
   _propertiesByPriorities[cPriority].emplace_back(rv->getId());
   return rv;
}
MDDPBitSequence::Ptr MDDSpec::combinedBSState(MDDCstrDesc::Ptr d,int nbb,unsigned char init,enum RelaxWith rw,int cPriority)
{
   auto rv = MDDStateSpec::combinedBSState(d,nbb,init,rw,cPriority);
   _propertiesByPriorities[cPriority].emplace_back(rv->getId());
   return rv;   
}
MDDPInt::Ptr MDDSpec::combinedIntState(MDDCstrDesc::Ptr d,int init,int max,enum RelaxWith rw, int cPriority)
{
   auto rv = MDDStateSpec::combinedIntState(d,init,max,rw,cPriority);
   _propertiesByPriorities[cPriority].emplace_back(rv->getId());
   return rv;
}
MDDPSWindow<short>::Ptr MDDSpec::downSWState(MDDCstrDesc::Ptr d,int len,int init,int finit,enum RelaxWith rw, int cPriority)
{
   auto rv = MDDStateSpec::downSWState(d,len,init,finit,rw,cPriority);
   _propertiesByPriorities[cPriority].emplace_back(rv->getId());
   return rv;
}
MDDPSWindow<short>::Ptr MDDSpec::upSWState(MDDCstrDesc::Ptr d,int len,int init,int finit,enum RelaxWith rw, int cPriority)
{
   auto rv = MDDStateSpec::upSWState(d,len,init,finit,rw,cPriority);
   _propertiesByPriorities[cPriority].emplace_back(rv->getId());
   return rv;
}
MDDPSWindow<short>::Ptr MDDSpec::combinedSWState(MDDCstrDesc::Ptr d,int len,int init,int finit,enum RelaxWith rw, int cPriority)
{
   auto rv = MDDStateSpec::combinedSWState(d,len,init,finit,rw,cPriority);
   _propertiesByPriorities[cPriority].emplace_back(rv->getId());
   return rv;
}


int MDDSpec::addDownState(MDDCstrDesc::Ptr d,int init,int max,enum RelaxWith rw, int constraintPriority)
{
   auto rv = downIntState(d,init,max,rw,constraintPriority);
   return rv->getId();
}
 int MDDSpec::addUpState(MDDCstrDesc::Ptr d,int init,int max,enum RelaxWith rw, int constraintPriority)
{
   auto rv = upIntState(d,init,max,rw,constraintPriority);
   return rv->getId();
}
int MDDSpec::addCombinedState(MDDCstrDesc::Ptr d,int init,int max,enum RelaxWith rw, int constraintPriority)
{
   auto rv = combinedIntState(d,init,max,rw,constraintPriority);
   return rv->getId();
}
int MDDSpec::addDownBSState(MDDCstrDesc::Ptr d,int nbb,unsigned char init,enum RelaxWith rw, int constraintPriority)
{
   auto rv = downBSState(d,nbb,init,rw,constraintPriority);
   return rv->getId();
}
int MDDSpec::addUpBSState(MDDCstrDesc::Ptr d,int nbb,unsigned char init,enum RelaxWith rw, int constraintPriority)
{
   auto rv = upBSState(d,nbb,init,rw,constraintPriority);
   return rv->getId();
}
int MDDSpec::addCombinedBSState(MDDCstrDesc::Ptr d,int nbb,unsigned char init,enum RelaxWith rw, int constraintPriority)
{
   auto rv = combinedBSState(d,nbb,init,rw,constraintPriority);
   return rv->getId();
}
int MDDSpec::addDownSWState(MDDCstrDesc::Ptr d,int len,int init,int finit,enum RelaxWith rw, int cPriority)
{
   auto rv = downSWState(d,len,init,finit,rw,cPriority);
   return rv->getId();
}
int MDDSpec::addUpSWState(MDDCstrDesc::Ptr d,int len,int init,int finit,enum RelaxWith rw, int cPriority)
{
   auto rv = upSWState(d,len,init,finit,rw,cPriority);
   return rv->getId();
}
int MDDSpec::addCombinedSWState(MDDCstrDesc::Ptr d,int len,int init,int finit,enum RelaxWith rw, int cPriority)
{
   auto rv = combinedSWState(d,len,init,finit,rw,cPriority);
   return rv->getId();
}

// --------------------------------------------------------------------------------


void MDDSpec::onFixpoint(FixFun onFix)
{
   _onFix.emplace_back(onFix);
}
void MDDSpec::splitOnLargest(SplitFun onSplit, int constraintPriority)
{
   _onSplit.emplace_back(onSplit);
   _onSplitByPriorities[constraintPriority].emplace_back(onSplit);
}
void MDDSpec::candidateByLargest(CandidateFun candidateSplit, int constraintPriority)
{
   _candidateSplit.emplace_back(candidateSplit);
   _candidateSplitByPriorities[constraintPriority].emplace_back(candidateSplit);
}
void MDDSpec::equivalenceClassValue(EquivalenceValueFun equivalenceValue, int constraintPriority)
{
   _equivalenceValue.emplace_back(equivalenceValue);
   _equivalenceValueByPriorities[constraintPriority].emplace_back(equivalenceValue);
}
int MDDSpec::numEquivalenceClasses()
{
   return (int)_equivalenceValue.size();
}
bool MDDSpec::equivalentForConstraintPriority(const MDDState& left, const MDDState& right, int constraintPriority) const
{
   for (int p : _propertiesByPriorities[constraintPriority])
      if (_attrsDown[p]->diff(left._mem, right._mem))
         return false;
   return true;
}

void MDDSpec::updateNode(MDDState& result,const MDDPack& n) const noexcept
{
   for(auto& fun : _updates)
      fun(result,n);
   result.computeHash();
}

int nbAECall = 0;
int nbAEFail = 0;

bool MDDSpec::exist(const MDDPack& parent,
                    const MDDPack& child,
                    const var<int>::Ptr& x,int v) const noexcept
{
   //std::cout << "DOWN:" << pDown << "\n";
   //std::cout << "COMB:" << pCombined << "\n";
   ++nbAECall;
   bool arcOk = true;
   for(const auto& exist : _scopedExists[x->getId()]) {
      arcOk = exist(parent,child,x,v);
      if (!arcOk) {
         ++nbAEFail;
         break;
      }
   }
   return arcOk;
}

int nbCONSCall = 0;
int nbCONSFail = 0;

bool MDDSpec::consistent(const MDDPack& pack) const noexcept
{
   ++nbCONSCall;
   bool cons = true;
   for(auto& consFun : _nodeExists) {
      cons = consFun(pack);
      if (!cons) {
         ++nbCONSFail;
         break;
      }
   }
   return cons;
}

void MDDSpec::nodeExist(NodeFun a)
{
   _nodeExists.emplace_back(std::move(a));
}
void MDDSpec::arcExist(MDDCstrDesc::Ptr d,ArcFun a)
{
   _exists.emplace_back(std::make_pair<MDDCstrDesc::Ptr,ArcFun>(std::move(d),std::move(a)));
}
void MDDSpec::updateNode(MDDCstrDesc::Ptr cd,MDDProperty::Ptr p,std::set<MDDProperty::Ptr> spDown,std::set<MDDProperty::Ptr> spUp,UpdateFun nf)
{
   if (cd->ownsCombinedProperty(p->getId())) {
      int uid = (int)_updates.size();
      cd->registerUpdate(uid);
      std::set<int> spd,spu;
      for(auto p : spDown) spd.insert(p->getId());
      for(auto p : spUp) spu.insert(p->getId());
      _attrsCombined[p->getId()]->setAntecedents(spd,spu);
      _attrsCombined[p->getId()]->setTransition(uid);
      _updates.emplace_back(std::move(nf));
   }
}
void MDDSpec::transitionDown(MDDCstrDesc::Ptr cd,int p,std::set<int> spDown,std::set<int> spCombined,lambdaTrans t)
{
   if (cd->ownsDownProperty(p)) {
      int tid = (int)_downTransition.size();
      cd->registerDown(tid);
      _attrsDown[p]->setAntecedents(spDown,spCombined);
      _attrsDown[p]->setTransition(tid);
      _downTransition.emplace_back(std::move(t));
   }
}
void MDDSpec::transitionDown2(MDDCstrDesc::Ptr cd,MDDProperty::Ptr p,std::set<MDDProperty::Ptr> spDown,std::set<MDDProperty::Ptr> spCombined,lambdaTrans t)
{
   std::set<int> pD,pC;
   for(auto p : spDown) pD.insert(p->getId());
   for(auto p : spCombined) pC.insert(p->getId());
   transitionDown(cd,p->getId(),pD,pC,t);
}


void MDDSpec::transitionUp(MDDCstrDesc::Ptr cd,int p,std::set<int> spUp,std::set<int> spCombined,lambdaTrans t)
{
   if (cd->ownsUpProperty(p)) {
      int tid = (int)_upTransition.size();
      cd->registerUp(tid);
      _attrsUp[p]->setAntecedents(spUp,spCombined);
      _attrsUp[p]->setTransition(tid);
      _upTransition.emplace_back(std::move(t));
   }
}
void MDDSpec::transitionUp2(MDDCstrDesc::Ptr cd,MDDProperty::Ptr p,std::set<MDDProperty::Ptr> spDown,std::set<MDDProperty::Ptr> spCombined,lambdaTrans t)
{
   std::set<int> pD,pC;
   for(auto p : spDown) pD.insert(p->getId());
   for(auto p : spCombined) pC.insert(p->getId());
   transitionUp(cd,p->getId(),pD,pC,t);
}

void MDDSpec::transitionDown(MDDCstrDesc::Ptr cd,const lambdaMap& map)
{
   for(auto& kv : map) {
      const auto& spDown = std::get<0>(kv.second);
      const auto& spCombined = std::get<1>(kv.second);
      const auto& f  = std::get<2>(kv.second);
      transitionDown(cd,kv.first,spDown,spCombined,f);
   }
}

void MDDSpec::transitionUp(MDDCstrDesc::Ptr cd,const lambdaMap& map)
{
   for(auto& kv : map) {
      const auto& spUp = std::get<0>(kv.second);
      const auto& spCombined = std::get<1>(kv.second);
      const auto& f  = std::get<2>(kv.second);
      transitionUp(cd,kv.first,spUp,spCombined,f);
   }
}

MDDState MDDSpec::rootState(Storage::Ptr& mem)
{
   MDDState rootState(this,(char*)mem->allocate(layoutSizeDown()),Down);
   for(size_t k=0;k < sizeDown();k++)
      rootState.init(k);
   //std::cout << "ROOT:" << rootState << std::endl;
   return rootState;
}

MDDState MDDSpec::sinkState(Storage::Ptr& mem)
{
   MDDState sinkState;
   if (layoutSizeUp()) {
     sinkState = MDDState(this,(char*)mem->allocate(layoutSizeUp()),Up);
     for(size_t k=0;k < sizeUp();k++)
        sinkState.init(k);
   } else sinkState = MDDState(this, nullptr, Up);
   return sinkState;
}


void MDDSpec::reachedFixpoint(const MDDState& sinkDown,const MDDState& sinkUp,const MDDState& sinkCombined)
{
   for(auto& fix : _onFix)
      fix(sinkDown,sinkUp,sinkCombined);
}

void LayerDesc::zoning(const MDDSpec& spec)
{
   zoningDown(spec);
   zoningUp(spec);
   for(auto p : _dframe) _dprop.setProp(p);
   for(auto p : _uframe) _uprop.setProp(p);
}

void LayerDesc::zoningUp(const MDDSpec& spec)
{
   int fstProp = -1,lstProp = -1;
   std::set<int> zp;
   for(auto p : _uframe) {
      if (fstProp == -1)
         zp.insert(fstProp = lstProp = p);
      else {
         if (p == fstProp - 1)
            zp.insert(fstProp = p);
         else if (p == lstProp + 1)
            zp.insert(lstProp = p);
         else if (p >= fstProp && p <= lstProp)
            zp.insert(p);
         else {
            auto sOfs = spec.startOfsUp(fstProp);
            auto eOfs = spec.endOfsUp(lstProp);
            _uzones.emplace_back(Zone(sOfs,eOfs,zp));
            zp.clear();
            zp.insert(fstProp = lstProp = p);
         }
      }
   }
   if (fstProp != -1) {
      auto sOfs = spec.startOfsUp(fstProp);
      auto eOfs = spec.endOfsUp(lstProp);
      _uzones.emplace_back(Zone(sOfs,eOfs,zp));
   }
}

void LayerDesc::zoningDown(const MDDSpec& spec)
{
   int fstProp = -1,lstProp = -1;
   std::set<int> zp;
   for(auto p : _dframe) {
      if (fstProp == -1) 
         zp.insert(fstProp = lstProp = p);
      else {
         if (p == fstProp - 1)
            zp.insert(fstProp = p);
         else if (p == lstProp + 1)
            zp.insert(lstProp = p);
         else if (p >= fstProp && p <= lstProp)
            zp.insert(p);
         else {
            auto sOfs = spec.startOfsDown(fstProp);
            auto eOfs = spec.endOfsDown(lstProp);
            _dzones.emplace_back(Zone(sOfs,eOfs,zp));
            zp.clear();
            zp.insert(fstProp = lstProp = p);
         }
      }
   }
   if (fstProp != -1) {
      auto sOfs = spec.startOfsDown(fstProp);
      auto eOfs = spec.endOfsDown(lstProp);
      _dzones.emplace_back(Zone(sOfs,eOfs,zp));
   }
}

void MDDSpec::compile()
{
   _omapDown = new MDDPropSet[_nbpDown];
   _omapUp = new MDDPropSet[_nbpUp];
   _omapUpToCombined = new MDDPropSet[_nbpUp];
   _omapDownToCombined = new MDDPropSet[_nbpDown];
   _omapCombinedToDown = new MDDPropSet[_nbpCombined];
   _omapCombinedToUp = new MDDPropSet[_nbpCombined];
   for(size_t i=0;i < _nbpDown;i++) {
      _omapDown[i] = MDDPropSet(_nbpDown);
      _omapDownToCombined[i] = MDDPropSet(_nbpCombined);
   }
   for(size_t i=0;i < _nbpUp;i++) {
      _omapUp[i] = MDDPropSet(_nbpUp);
      _omapUpToCombined[i] = MDDPropSet(_nbpCombined);
   }
   for(size_t i=0;i < _nbpCombined;i++) {
      _omapCombinedToDown[i] = MDDPropSet(_nbpDown);
      _omapCombinedToUp[i] = MDDPropSet(_nbpUp);
   }
   for(size_t p=0;p < _nbpDown;p++) {
      auto& outDown = _omapDown[p];
      for(size_t s=0;s < _nbpDown;s++) {
         const auto& ants = _attrsDown[s]->antecedents();
         if (ants.find(p)!= ants.end()) {
            outDown.setProp(s);
         }
      }
      //std::cout << "omapDown[" << p << "] = " << outDown << '\n';
      if (_nbpCombined) {
         auto& outDownToCombined = _omapDownToCombined[p];
         for(size_t s=0;s < _nbpCombined;s++) {
            const auto& ants = _attrsCombined[s]->antecedents();
            if (ants.find(p)!= ants.end()) {
               outDownToCombined.setProp(s);
            }
         }
         //std::cout << "omapDownToCombined[" << p << "] = " << outDownToCombined << '\n';
      }
   }
   for(size_t p=0;p < _nbpUp;p++) {
      auto& outUp = _omapUp[p];
      for(size_t s=0;s < _nbpUp;s++) {
         const auto& ants = _attrsUp[s]->antecedents();
         if (ants.find(p)!= ants.end()) {
            outUp.setProp(s);
         }
      }
      //std::cout << "omapUp[" << p << "] = " << outUp << '\n';
      if (_nbpCombined) {
         auto& outUpToCombined = _omapUpToCombined[p];
         for(size_t s=0;s < _nbpCombined;s++) {
            const auto& ants = _attrsCombined[s]->antecedentsSecondary();
            if (ants.find(p)!= ants.end()) {
               outUpToCombined.setProp(s);
            }
         }
         //std::cout << "omapUpToCombined[" << p << "] = " << outUpToCombined << '\n';
      }
   }
   for(size_t p=0;p < _nbpCombined;p++) {
      auto& outCombinedToDown = _omapCombinedToDown[p];
      auto& outCombinedToUp = _omapCombinedToUp[p];
      for(size_t s=0;s < _nbpDown;s++) {
         const auto& ants = _attrsDown[s]->antecedentsSecondary();
         if (ants.find(p)!= ants.end()) {
            outCombinedToDown.setProp(s);
         }
      }
      for(size_t s=0;s < _nbpUp;s++) {
         const auto& ants = _attrsUp[s]->antecedentsSecondary();
         if (ants.find(p)!= ants.end()) {
            outCombinedToUp.setProp(s);
         }
      }
      //std::cout << "omapCombinedToDown[" << p << "] = " << outCombinedToDown << '\n';
      //std::cout << "omapCombinedToUp[" << p << "] = " << outCombinedToUp << '\n';
   }
   const unsigned nbL = (unsigned)x.size();
   _transLayer.reserve(nbL);
   _uptransLayer.reserve(nbL);
   _frameLayer.reserve(nbL);
   for(auto i = 0u;i < nbL;i++) {
      int reboot = 0;
      //int rebootSum = 0;
      int numConstraints = 0;
      auto& layer   = _transLayer.emplace_back(std::vector<lambdaTrans>());
      auto& upLayer = _uptransLayer.emplace_back(std::vector<lambdaTrans>());
      auto& frame   = _frameLayer.emplace_back(LayerDesc(_nbpDown,_nbpUp));
      for(auto& c : constraints) {
         if (c->inScope(x[i]))  {
            for(auto j : c->downTransitions())
               layer.emplace_back(_downTransition[j]);
            for(auto j : c->upTransitions())
               upLayer.emplace_back(_upTransition[j]);
            if (c->vars().size() < nbL/2) {
               int highestLayer = c->vars()[0]->getId();
               reboot = std::max(reboot, (int) i - highestLayer);
               //rebootSum += (int) i - highestLayer;
               numConstraints++;
            }
         } else { // x[i] does not appear in constraint c. So the properties of c should be subject to frame axioms (copied)
            for(auto j : c->propertiesDown())
               frame.addDownProp(j);
            for(auto j : c->propertiesUp())
               frame.addUpProp(j);
         }
      }
      //if (reboot) reboot--;
      _rebootByLayer.emplace_back(reboot);
      //std::cout << "Reboot for layer " << i << ": " << reboot << "\n";
      //_rebootByLayer.emplace_back(std::ceil(rebootSum * 1.0/numConstraints));
      frame.zoning(*this);
   }
   int lid,uid;
   std::tie(lid,uid) = idRange(x);
   const int sz = uid + 1;
   _scopedExists.resize(sz);
   for(auto& exist : _exists) {
      auto& cd  = std::get<0>(exist);
      auto& fun = std::get<1>(exist);
      auto& vars = cd->vars();
      for(auto& v : vars)
         _scopedExists[v->getId()].emplace_back(fun);
   }
   for(size_t p = 0 ; p < _nbpDown;p++) {
      if (_xDownRelax.find(p) == _xDownRelax.end()){
         _defaultDownRelax.push_back(p);
      }
   }
   for(size_t p = 0 ; p < _nbpUp;p++) {
      if (_xUpRelax.find(p) == _xUpRelax.end())
         _defaultUpRelax.push_back(p);
   }
}

void MDDSpec::fullStateDown(MDDState& result,const MDDPack& parent,unsigned l,const var<int>::Ptr& var,const MDDIntSet& v)
{
   result.clear();
   result.zero();
   _frameLayer[l].frameDown(result,parent.down);
   for(const auto& t : _transLayer[l])
      t(result,parent,var,v);
   result.relax(parent.down.isRelaxed() || v.size() > 1);
}

void MDDSpec::incrStateDown(const MDDPropSet& out,MDDState& result,const MDDPack& parent,unsigned l,const var<int>::Ptr& var,
                              const MDDIntSet& v)
{
   result.clear();
   for(auto p : out) {
      int tid = _frameLayer[l].hasDownProp(p) ? -1 : _attrsDown[p]->getTransition();
      if (tid != -1)
         _downTransition[tid](result,parent,var,v); // actual transition
      else 
         result.setProp(p,parent.down); // frame axiom
   }
   result.relax(parent.down.isRelaxed() || v.size() > 1);
}

void MDDSpec::relaxationDown(MDDState& a,const MDDState& b) const noexcept
{
   for(auto p : _defaultDownRelax) {
      switch(_attrsDown[p]->relaxFun()) {
         case MinFun: a.minWith(p,b);break;
         case MaxFun: a.maxWith(p,b);break;
         case External: break;
      }
   }
   for(const auto& relax : _downRelaxation)
      relax(a,a,b);
   a.computeHash();
}

void MDDSpec::relaxationUp(MDDState& a,const MDDState& b) const noexcept
{
   for(auto p : _defaultUpRelax) {
      switch(_attrsUp[p]->relaxFun()) {
         case MinFun: a.minWith(p,b);break;
         case MaxFun: a.maxWith(p,b);break;
         case External: break;
      }
   }
   for(const auto& relax : _upRelaxation)
      relax(a,a,b);
   a.computeHash();
}

void MDDSpec::relaxationDownIncr(const MDDPropSet& out,MDDState& a,const MDDState& b) const noexcept
{
   for(auto p : out) {
      switch(_attrsDown[p]->relaxFun()) {
         case MinFun: a.minWith(p,b);break;
         case MaxFun: a.maxWith(p,b);break;
         case External:
            _downRelaxation[_attrsDown[p]->getRelax()](a,a,b);
            break;
      }
   }
   a.computeHash();
}

void MDDSpec::relaxationUpIncr(const MDDPropSet& out,MDDState& a,const MDDState& b) const noexcept
{
   for(auto p : out) {
      switch(_attrsUp[p]->relaxFun()) {
         case MinFun: a.minWith(p,b);break;
         case MaxFun: a.maxWith(p,b);break;
         case External:
            _upRelaxation[_attrsUp[p]->getRelax()](a,a,b);
            break;
      }
   }
   a.computeHash();
}

void MDDSpec::fullStateUp(MDDState& target,const MDDPack& child,unsigned l,const var<int>::Ptr& var,const MDDIntSet& v)
{
   target.clear();
   _frameLayer[l].frameUp(target,child.up);
   for(const auto& t : _uptransLayer[l])
      t(target,child,var,v);
   target.relax(child.up.isRelaxed() || v.size() > 1);
}

void MDDSpec::incrStateUp(const MDDPropSet& out,MDDState& target,const MDDPack& child,unsigned l,const var<int>::Ptr& var,const MDDIntSet& v)
{
   target.clear();
   for(auto p : out) {
      int tid = _frameLayer[l].hasUpProp(p) ? -1 : _attrsUp[p]->getTransition();
      if (tid != -1)
         _upTransition[tid](target,child,var,v); // actual transition
      else 
         target.setProp(p,child.up); // frame axiom
   }
   target.relax(child.up.isRelaxed() || v.size() > 1);
}

void MDDSpec::setConstraintPrioritySize(int size)
{
   _onSplitByPriorities.reserve(size);
   _equivalenceValueByPriorities.reserve(size);
   _propertiesByPriorities.reserve(size);
   for (int i = 0; i <= size; i++) {
      _onSplitByPriorities.push_back(std::vector<SplitFun>());
      _candidateSplitByPriorities.push_back(std::vector<CandidateFun>());
      _equivalenceValueByPriorities.push_back(std::vector<EquivalenceValueFun>());
      _propertiesByPriorities.push_back(std::vector<int>());
   }
}

std::ostream& operator<<(std::ostream& os,const MDDSpec& s)
{
   os << "Spec Down(";
   for(size_t p=0;p < s._nbpDown;p++) {
      s._attrsDown[p]->print(os);
         os << ' ';
   }
   os << ')';
   os << "\nSpec Up(";
   for(size_t p=0;p < s._nbpUp;p++) {
      s._attrsUp[p]->print(os);
      os << ' ';
   }
   os << ')';
   os << "\nSpec Combined(";
   for(size_t p=0;p < s._nbpCombined;p++) {
      s._attrsCombined[p]->print(os);
      os << ' ';
   }
   os << ')';
   return os;
}



int nbCSDown  = 0;
int hitCSDown = 0;
int nbCSUp  = 0;
int hitCSUp = 0;

MDDStateFactory::MDDStateFactory(MDDSpec* spec)
   : _mddspec(spec),
     _mem(new Pool()),
     _downHash(_mem,300149),
     _upHash(_mem,300149),
     _mark(_mem->mark()),
     _enabled(false)
{
}


//void MDDStateFactory::createState(MDDState& result,const MDDState& pDown,const MDDState& pCombined,int layer,const var<int>::Ptr x,const MDDIntSet& vals,bool up)
//{
//   nbCS++;
//   _mddspec->createState(result,pDown,pCombined,layer,x,vals,up);
//   result.computeHash();
//}

void MDDStateFactory::createStateDown(MDDState& result,const MDDPack& parent,int layer,const var<int>::Ptr x,const MDDIntSet& vals,bool up)
{
   if (vals.isSingleton()) {
      MDDSKey key { &parent.down, &parent.comb, layer, vals.singleton() };
      MDDState* match = nullptr;
      auto loc = _downHash.get(key,match);
      if (loc) {
         ++hitCSDown;
         result.copyState(*match);
      } else {
         nbCSDown++;
         _mddspec->fullStateDown(result,parent,layer,x,MDDIntSet(vals.singleton()));
         result.computeHash();
         MDDState* pdc = new (_mem) MDDState(parent.down.clone(_mem));
         MDDState* pcc = new (_mem) MDDState(parent.comb.clone(_mem));
         MDDSKey ikey { pdc, pcc, layer, vals.singleton() };
         _downHash.insert(ikey,new (_mem) MDDState(result.clone(_mem)));
      }
   } else {
      nbCSDown++;
      _mddspec->fullStateDown(result,parent,layer,x,vals);
      result.computeHash();
   }
}
void MDDStateFactory::createStateUp(MDDState& result,const MDDPack& child,int layer,const var<int>::Ptr x,const MDDIntSet& vals)
{
   if (vals.isSingleton()) {
      MDDSKey key { &child.up, &child.comb, layer, vals.singleton() };
      MDDState* match = nullptr;
      auto loc = _upHash.get(key,match);
      if (loc) {
         ++hitCSUp;
         result.copyState(*match);
      } else {
         nbCSUp++;
         _mddspec->fullStateUp(result,child,layer,x,MDDIntSet(vals.singleton()));
         result.computeHash();
         MDDState* cuc = new (_mem) MDDState(child.up.clone(_mem));
         MDDState* ccc = new (_mem) MDDState(child.comb.clone(_mem));
         MDDSKey ikey { cuc, ccc, layer, vals.singleton() };
         _upHash.insert(ikey,new (_mem) MDDState(result.clone(_mem)));
      }
   } else {
      nbCSUp++;
      _mddspec->fullStateUp(result,child,layer,x,vals);
      result.computeHash();
   }
}

void MDDStateFactory::splitState(MDDState*& result,MDDNode* n,const MDDPack& parent,int layer,const var<int>::Ptr x,int val)
{
   MDDSKey key { &parent.down, &parent.comb, layer, val };
   auto loc = _downHash.get(key,result);
   if (loc) {
      ++hitCSDown;
      result = new (_mem) MDDState(result->clone(_mem));
   } else {
      nbCSDown++;
      result = new (_mem) MDDState(_mddspec,new (_mem) char[_mddspec->layoutSizeDown()],Down);
      _mddspec->fullStateDown(*result,parent,layer,x,MDDIntSet(val));
      result->computeHash();
      MDDState* pdc = new (_mem) MDDState(parent.down.clone(_mem));
      MDDState* pcc = new (_mem) MDDState(parent.comb.clone(_mem));
      MDDSKey ikey { pdc, pcc, layer, val };
      _downHash.insert(ikey,new (_mem) MDDState(result->clone(_mem)));
   }
}

void MDDStateFactory::clear()
{
   _downHash.clear();
   _upHash.clear();
   _mem->clear(_mark);
   _enabled = true;
}
