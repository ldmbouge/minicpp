//
//  mddstate.cpp
//  minicpp
//
//  Created by Waldy on 10/28/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

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
      if (max <= 127)
         rv = new MDDPByte(id,ofs,init,max,rw);
      else
         rv = new MDDPInt(id,ofs,init,max,rw);
      return rv;
   }
   MDDProperty::Ptr makeBSProperty(short id,unsigned short ofs,int nbb,unsigned char init,enum RelaxWith rw)
   {
      MDDProperty::Ptr rv = new MDDPBitSequence(id,ofs,nbb,init,rw);
      return rv;
   }
   MDDProperty::Ptr makeWinProperty(short id,unsigned short ofs,int len,int init,int finit,enum RelaxWith rw)
   {
      MDDProperty::Ptr rv = new MDDPSWindow<short>(id,ofs,len,init,finit,rw);
      return rv;
   }
}

MDDConstraintDescriptor::MDDConstraintDescriptor(const MDDConstraintDescriptor& d)
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
   _attrsDown = new MDDProperty*[_mxpDown];
   _attrsUp = new MDDProperty*[_mxpUp];
   _attrsCombined = new MDDProperty*[_mxpCombined];
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
      MDDProperty** ns = new MDDProperty*[_mxpDown<<1];
      for(short i =0;i < _nbpDown;i++)
         ns[i] = _attrsDown[i];
      delete[]_attrsDown;
      _attrsDown = ns;
      _mxpDown = _mxpDown << 1;
   }
   _attrsDown[_nbpDown++] = p.get();
   p->setDirection(Down);
}
void MDDStateSpec::addUpProperty(MDDProperty::Ptr p) noexcept
{
   if (_nbpUp == _mxpUp) {
      MDDProperty** ns = new MDDProperty*[_mxpUp<<1];
      for(short i =0;i < _nbpUp;i++)
         ns[i] = _attrsUp[i];
      delete[]_attrsUp;
      _attrsUp = ns;
      _mxpUp = _mxpUp << 1;
   }
   _attrsUp[_nbpUp++] = p.get();
   p->setDirection(Up);
}
void MDDStateSpec::addCombinedProperty(MDDProperty::Ptr p) noexcept
{
   if (_nbpCombined == _mxpCombined) {
      MDDProperty** ns = new MDDProperty*[_mxpCombined<<1];
      for(short i =0;i < _nbpCombined;i++)
         ns[i] = _attrsCombined[i];
      delete[]_attrsCombined;
      _attrsCombined = ns;
      _mxpCombined = _mxpCombined << 1;
   }
   _attrsCombined[_nbpCombined++] = p.get();
   p->setDirection(Bi);
}

void MDDStateSpec::layout()
{
   size_t lszBit = 0;
   for(int p = 0;p <_nbpDown;p++) {
      auto a = _attrsDown[p];
      lszBit = a->setOffset(lszBit);
   }
   size_t boB = lszBit & 0x7;
   if (boB != 0)
      lszBit = (lszBit | 0x7) + 1;
   _lszDown = lszBit >> 3;
   _lszDown = (_lszDown & 0x7) ? (_lszDown | 0x7)+1 : _lszDown;
   assert(_lszDown % 8 == 0); // # bytes is always a multiple of 8.
   //std::cout << "Down State requires:" << _lszDown << " bytes" << std::endl;

   lszBit = 0;
   for(int p = 0;p <_nbpUp;p++) {
      auto a = _attrsUp[p];
      lszBit = a->setOffset(lszBit);
   }
   boB = lszBit & 0x7;
   if (boB != 0)
      lszBit = (lszBit | 0x7) + 1;
   _lszUp = lszBit >> 3;
   _lszUp = (_lszUp & 0x7) ? (_lszUp | 0x7)+1 : _lszUp;
   assert(_lszUp % 8 == 0); // # bytes is always a multiple of 8.
   //std::cout << "Up State requires:" << _lszUp << " bytes" << std::endl;

   lszBit = 0;
   for(int p = 0;p <_nbpCombined;p++) {
      auto a = _attrsCombined[p];
      lszBit = a->setOffset(lszBit);
   }
   boB = lszBit & 0x7;
   if (boB != 0)
      lszBit = (lszBit | 0x7) + 1;
   _lszCombined = lszBit >> 3;
   _lszCombined = (_lszCombined & 0x7) ? (_lszCombined | 0x7)+1 : _lszCombined;
   assert(_lszCombined % 8 == 0); // # bytes is always a multiple of 8.
   //std::cout << "Combined State requires:" << _lszCombined << " bytes" << std::endl;
}

int MDDStateSpec::addDownState(MDDConstraintDescriptor::Ptr d, int init,int max,enum RelaxWith rw, int constraintPriority)
{
   int aid = (int)_nbpDown;
   addDownProperty(Factory::makeProperty(aid, 0, init, max,rw));
   d->addDownProperty(aid);
   return aid;
}
int MDDStateSpec::addUpState(MDDConstraintDescriptor::Ptr d, int init,int max,enum RelaxWith rw, int constraintPriority)
{
   int aid = (int)_nbpUp;
   addUpProperty(Factory::makeProperty(aid, 0, init, max,rw));
   d->addUpProperty(aid);
   return aid;
}
int MDDStateSpec::addCombinedState(MDDConstraintDescriptor::Ptr d, int init,int max,enum RelaxWith rw, int constraintPriority)
{
   int aid = (int)_nbpCombined;
   addCombinedProperty(Factory::makeProperty(aid, 0, init, max,rw));
   d->addCombinedProperty(aid);
   return aid;
}
int MDDStateSpec::addDownBSState(MDDConstraintDescriptor::Ptr d,int nbb,unsigned char init,enum RelaxWith rw, int constraintPriority)
{
   int aid = (int)_nbpDown;
   addDownProperty(Factory::makeBSProperty(aid,0,nbb,init,rw));
   d->addDownProperty(aid);
   return aid;
}
int MDDStateSpec::addUpBSState(MDDConstraintDescriptor::Ptr d,int nbb,unsigned char init,enum RelaxWith rw, int constraintPriority)
{
   int aid = (int)_nbpUp;
   addUpProperty(Factory::makeBSProperty(aid,0,nbb,init,rw));
   d->addUpProperty(aid);
   return aid;
}
int MDDStateSpec::addCombinedBSState(MDDConstraintDescriptor::Ptr d,int nbb,unsigned char init,enum RelaxWith rw, int constraintPriority)
{
   int aid = (int)_nbpCombined;
   addCombinedProperty(Factory::makeBSProperty(aid,0,nbb,init,rw));
   d->addCombinedProperty(aid);
   return aid;
}
int MDDStateSpec::addDownSWState(MDDConstraintDescriptor::Ptr d,int len,int init,int finit,enum RelaxWith rw, int constraintPriority)
{
   int aid = (int)_nbpDown;
   addDownProperty(Factory::makeWinProperty(aid,0,len,init,finit,rw));
   d->addDownProperty(aid);
   return aid;
}
int MDDStateSpec::addUpSWState(MDDConstraintDescriptor::Ptr d,int len,int init,int finit,enum RelaxWith rw, int constraintPriority)
{
   int aid = (int)_nbpUp;
   addUpProperty(Factory::makeWinProperty(aid,0,len,init,finit,rw));
   d->addUpProperty(aid);
   return aid;
}
int MDDStateSpec::addCombinedSWState(MDDConstraintDescriptor::Ptr d,int len,int init,int finit,enum RelaxWith rw, int constraintPriority)
{
   int aid = (int)_nbpCombined;
   addCombinedProperty(Factory::makeWinProperty(aid,0,len,init,finit,rw));
   d->addCombinedProperty(aid);
   return aid;
}


std::vector<int> MDDStateSpec::addDownStates(MDDConstraintDescriptor::Ptr d,int from, int to,int max, std::function<int(int)> clo)
{
   std::vector<int> res;
   for(int i = from; i <= to; i++)
      res.push_back(addDownState(d,clo(i),max));
   return res;
}
std::vector<int> MDDStateSpec::addUpStates(MDDConstraintDescriptor::Ptr d,int from, int to,int max, std::function<int(int)> clo)
{
   std::vector<int> res;
   for(int i = from; i <= to; i++)
      res.push_back(addUpState(d,clo(i),max));
   return res;
}
std::vector<int> MDDStateSpec::addCombinedStates(MDDConstraintDescriptor::Ptr d,int from, int to,int max, std::function<int(int)> clo)
{
   std::vector<int> res;
   for(int i = from; i <= to; i++)
      res.push_back(addCombinedState(d,clo(i),max));
   return res;
}
std::vector<int> MDDStateSpec::addDownStates(MDDConstraintDescriptor::Ptr d,int max, std::initializer_list<int> inputs)
{
   std::vector<int> res;
   for(auto& v : inputs)
      res.push_back(addDownState(d,v,max));
   return res;
}
std::vector<int> MDDStateSpec::addUpStates(MDDConstraintDescriptor::Ptr d,int max, std::initializer_list<int> inputs)
{
   std::vector<int> res;
   for(auto& v : inputs)
      res.push_back(addUpState(d,v,max));
   return res;
}
std::vector<int> MDDStateSpec::addCombinedStates(MDDConstraintDescriptor::Ptr d,int max, std::initializer_list<int> inputs)
{
   std::vector<int> res;
   for(auto& v : inputs)
      res.push_back(addCombinedState(d,v,max));
   return res;
}

int MDDSpec::addDownState(MDDConstraintDescriptor::Ptr d,int init,int max,enum RelaxWith rw, int constraintPriority)
{
   auto rv = MDDStateSpec::addDownState(d,init,max,rw);
   _propertiesByPriorities[constraintPriority].emplace_back(rv);
   return rv;
}
int MDDSpec::addUpState(MDDConstraintDescriptor::Ptr d,int init,int max,enum RelaxWith rw, int constraintPriority)
{
   auto rv = MDDStateSpec::addUpState(d,init,max,rw);
   _propertiesByPriorities[constraintPriority].emplace_back(rv);
   return rv;
}
int MDDSpec::addCombinedState(MDDConstraintDescriptor::Ptr d,int init,int max,enum RelaxWith rw, int constraintPriority)
{
   auto rv = MDDStateSpec::addCombinedState(d,init,max,rw);
   _propertiesByPriorities[constraintPriority].emplace_back(rv);
   return rv;
}
int MDDSpec::addDownBSState(MDDConstraintDescriptor::Ptr d,int nbb,unsigned char init,enum RelaxWith rw, int constraintPriority)
{
   auto rv = MDDStateSpec::addDownBSState(d,nbb,init,rw);
   _propertiesByPriorities[constraintPriority].emplace_back(rv);
   return rv;
}
int MDDSpec::addUpBSState(MDDConstraintDescriptor::Ptr d,int nbb,unsigned char init,enum RelaxWith rw, int constraintPriority)
{
   auto rv = MDDStateSpec::addUpBSState(d,nbb,init,rw);
   _propertiesByPriorities[constraintPriority].emplace_back(rv);
   return rv;
}
int MDDSpec::addCombinedBSState(MDDConstraintDescriptor::Ptr d,int nbb,unsigned char init,enum RelaxWith rw, int constraintPriority)
{
   auto rv = MDDStateSpec::addCombinedBSState(d,nbb,init,rw);
   _propertiesByPriorities[constraintPriority].emplace_back(rv);
   return rv;
}
int MDDSpec::addDownSWState(MDDConstraintDescriptor::Ptr d,int len,int init,int finit,enum RelaxWith rw, int constraintPriority)
{
   auto rv = MDDStateSpec::addDownSWState(d,len,init,finit,rw);
   _propertiesByPriorities[constraintPriority].emplace_back(rv);
   return rv;
}
int MDDSpec::addUpSWState(MDDConstraintDescriptor::Ptr d,int len,int init,int finit,enum RelaxWith rw, int constraintPriority)
{
   auto rv = MDDStateSpec::addUpSWState(d,len,init,finit,rw);
   _propertiesByPriorities[constraintPriority].emplace_back(rv);
   return rv;
}
int MDDSpec::addCombinedSWState(MDDConstraintDescriptor::Ptr d,int len,int init,int finit,enum RelaxWith rw, int constraintPriority)
{
   auto rv = MDDStateSpec::addCombinedSWState(d,len,init,finit,rw);
   _propertiesByPriorities[constraintPriority].emplace_back(rv);
   return rv;
}

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

void MDDSpec::updateNode(MDDState& result,const MDDState& down,const MDDState& up) const noexcept
{
   for(auto& fun : _updates)
      fun(result,down,up);
   result.computeHash();
}

int nbAECall = 0;
int nbAEFail = 0;

bool MDDSpec::exist(const MDDState& parentDown,const MDDState& parentCombined,const MDDState& childUp,const MDDState& childCombined,const var<int>::Ptr& x,int v,bool up) const noexcept
{
   ++nbAECall;
   bool arcOk = true;
   for(const auto& exist : _scopedExists[x->getId()]) {
      arcOk = exist(parentDown,parentCombined,childUp,childCombined,x,v,up);
      if (!arcOk) {
         ++nbAEFail;
         break;
      }
   }
   return arcOk;
}

int nbCONSCall = 0;
int nbCONSFail = 0;

bool MDDSpec::consistent(const MDDState& down,const MDDState& up,const MDDState& combined) const noexcept
{
   ++nbCONSCall;
   bool cons = true;
   for(auto& consFun : _nodeExists) {
      cons = consFun(down,up,combined);
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
void MDDSpec::arcExist(MDDConstraintDescriptor::Ptr d,ArcFun a)
{
   _exists.emplace_back(std::make_pair<MDDConstraintDescriptor::Ptr,ArcFun>(std::move(d),std::move(a)));
}
void MDDSpec::updateNode(int p,std::set<int> spDown,std::set<int> spUp,UpdateFun nf)
{
   for (auto& cd : constraints)
      if (cd->ownsCombinedProperty(p)) {
         int uid = (int)_updates.size();
         cd->registerUpdate(uid);
         _attrsCombined[p]->setAntecedents(spDown,spUp);
         _attrsCombined[p]->setTransition(uid);
         _updates.emplace_back(std::move(nf));
         break;
      }
}
void MDDSpec::transitionDown(int p,std::set<int> spDown,std::set<int> spCombined,lambdaTrans t)
{
   for(auto& cd : constraints)
      if (cd->ownsDownProperty(p)) {
         int tid = (int)_downTransition.size();
         cd->registerDown(tid);
         _attrsDown[p]->setAntecedents(spDown,spCombined);
         _attrsDown[p]->setTransition(tid);
         _downTransition.emplace_back(std::move(t));
         break;
      }
}

void MDDSpec::transitionUp(int p,std::set<int> spUp,std::set<int> spCombined,lambdaTrans t)
{
   for(auto& cd : constraints)
      if (cd->ownsUpProperty(p)) {
         int tid = (int)_upTransition.size();
         cd->registerUp(tid);
         _attrsUp[p]->setAntecedents(spUp,spCombined);
         _attrsUp[p]->setTransition(tid);
         _upTransition.emplace_back(std::move(t));
         break;
      }
}

void MDDSpec::transitionDown(const lambdaMap& map)
{
   for(auto& kv : map) {
      const auto& spDown = std::get<0>(kv.second);
      const auto& spCombined = std::get<1>(kv.second);
      const auto& f  = std::get<2>(kv.second);
      transitionDown(kv.first,spDown,spCombined,f);
   }
}

void MDDSpec::transitionUp(const lambdaMap& map)
{
   for(auto& kv : map) {
      const auto& spUp = std::get<0>(kv.second);
      const auto& spCombined = std::get<1>(kv.second);
      const auto& f  = std::get<2>(kv.second);
      transitionUp(kv.first,spUp,spCombined,f);
   }
}

MDDState MDDSpec::rootState(Storage::Ptr& mem)
{
   MDDState rootState(this,(char*)mem->allocate(layoutSizeDown()),Down);
   for(auto k=0;k < sizeDown();k++)
      rootState.init(k);
   //std::cout << "ROOT:" << rootState << std::endl;
   return rootState;
}

MDDState MDDSpec::sinkState(Storage::Ptr& mem)
{
   MDDState sinkState(this,(char*)mem->allocate(layoutSizeUp()),Up);
   for(auto k=0;k < sizeUp();k++)
      sinkState.init(k);
   //std::cout << "SINK:" << sinkState << std::endl;
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
   for(int i=0;i < _nbpDown;i++) {
      _omapDown[i] = MDDPropSet(_nbpDown);
      _omapDownToCombined[i] = MDDPropSet(_nbpCombined);
   }
   for(int i=0;i < _nbpUp;i++) {
      _omapUp[i] = MDDPropSet(_nbpUp);
      _omapUpToCombined[i] = MDDPropSet(_nbpCombined);
   }
   for(int i=0;i < _nbpCombined;i++) {
      _omapCombinedToDown[i] = MDDPropSet(_nbpDown);
      _omapCombinedToUp[i] = MDDPropSet(_nbpUp);
   }
   for(int p=0;p < _nbpDown;p++) {
      auto& outDown = _omapDown[p];
      for(int s=0;s < _nbpDown;s++) {
         const auto& ants = _attrsDown[s]->antecedents();
         if (ants.find(p)!= ants.end()) {
            outDown.setProp(s);
         }
      }
      //std::cout << "omapDown[" << p << "] = " << outDown << '\n';
      if (_nbpCombined) {
         auto& outDownToCombined = _omapDownToCombined[p];
         for(int s=0;s < _nbpCombined;s++) {
            const auto& ants = _attrsCombined[s]->antecedents();
            if (ants.find(p)!= ants.end()) {
               outDownToCombined.setProp(s);
            }
         }
         //std::cout << "omapDownToCombined[" << p << "] = " << outDownToCombined << '\n';
      }
   }
   for(int p=0;p < _nbpUp;p++) {
      auto& outUp = _omapUp[p];
      for(int s=0;s < _nbpUp;s++) {
         const auto& ants = _attrsUp[s]->antecedents();
         if (ants.find(p)!= ants.end()) {
            outUp.setProp(s);
         }
      }
      //std::cout << "omapUp[" << p << "] = " << outUp << '\n';
      if (_nbpCombined) {
         auto& outUpToCombined = _omapUpToCombined[p];
         for(int s=0;s < _nbpCombined;s++) {
            const auto& ants = _attrsCombined[s]->antecedentsSecondary();
            if (ants.find(p)!= ants.end()) {
               outUpToCombined.setProp(s);
            }
         }
         //std::cout << "omapUpToCombined[" << p << "] = " << outUpToCombined << '\n';
      }
   }
   for(int p=0;p < _nbpCombined;p++) {
      auto& outCombinedToDown = _omapCombinedToDown[p];
      auto& outCombinedToUp = _omapCombinedToUp[p];
      for(int s=0;s < _nbpDown;s++) {
         const auto& ants = _attrsDown[s]->antecedentsSecondary();
         if (ants.find(p)!= ants.end()) {
            outCombinedToDown.setProp(s);
         }
      }
      for(int s=0;s < _nbpUp;s++) {
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
      auto& layer   = _transLayer.emplace_back(std::vector<lambdaTrans>());
      auto& upLayer = _uptransLayer.emplace_back(std::vector<lambdaTrans>());
      auto& frame   = _frameLayer.emplace_back(LayerDesc(_nbpDown,_nbpUp));
      for(auto& c : constraints) {
         if (c->inScope(x[i]))  {
            for(auto j : c->downTransitions())
               layer.emplace_back(_downTransition[j]);
            for(auto j : c->upTransitions())
               upLayer.emplace_back(_upTransition[j]);
         } else { // x[i] does not appear in constraint c. So the properties of c should be subject to frame axioms (copied)
            for(auto j : c->propertiesDown())
               frame.addDownProp(j);
            for(auto j : c->propertiesUp())
               frame.addUpProp(j);
         }
      }
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
   for(int p = 0 ; p < _nbpDown;p++) {
      if (_xDownRelax.find(p) == _xDownRelax.end()){
         _defaultDownRelax.push_back(p);
      }
   }
   for(int p = 0 ; p < _nbpUp;p++) {
      if (_xUpRelax.find(p) == _xUpRelax.end())
         _defaultUpRelax.push_back(p);
   }
}

void MDDSpec::fullStateDown(MDDState& result,const MDDState& parentDown,const MDDState& parentCombined,unsigned l,const var<int>::Ptr& var,const MDDIntSet& v,bool hasUp)
{
   result.clear();
   for(const auto& t : _transLayer[l])
      t(result,parentDown,parentCombined,var,v,hasUp);
   _frameLayer[l].frameDown(result,parentDown);
   result.relax(parentDown.isRelaxed() || v.size() > 1);
}

void MDDSpec::incrStateDown(const MDDPropSet& out,MDDState& result,const MDDState& parentDown,const MDDState& parentCombined,unsigned l,const var<int>::Ptr& var,
                              const MDDIntSet& v,bool hasUp)
{
   result.clear();
   for(auto p : out) {
      int tid = _frameLayer[l].hasDownProp(p) ? -1 : _attrsDown[p]->getTransition();
      if (tid != -1)
         _downTransition[tid](result,parentDown,parentCombined,var,v,hasUp); // actual transition
      else 
         result.setProp(p,parentDown); // frame axiom
   }
   result.relax(parentDown.isRelaxed() || v.size() > 1);
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
void MDDSpec::fullStateUp(MDDState& target,const MDDState& childUp,const MDDState& childCombined,unsigned l,const var<int>::Ptr& var,const MDDIntSet& v)
{
   target.clear();
   for(const auto& t : _uptransLayer[l])
      t(target,childUp,childCombined,var,v,true);
   _frameLayer[l].frameUp(target,childUp);
   target.relax(childUp.isRelaxed() || v.size() > 1);
}
void MDDSpec::incrStateUp(const MDDPropSet& out,MDDState& target,const MDDState& childUp,const MDDState& childCombined,unsigned l,const var<int>::Ptr& var,const MDDIntSet& v)
{
   target.clear();
   for(auto p : out) {
      int tid = _frameLayer[l].hasUpProp(p) ? -1 : _attrsUp[p]->getTransition();
      if (tid != -1)
         _upTransition[tid](target,childUp,childCombined,var,v,true); // actual transition
      else 
         target.setProp(p,childUp); // frame axiom
   }
   target.relax(childUp.isRelaxed() || v.size() > 1);
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


//void MDDStateFactory::createState(MDDState& result,const MDDState& parentDown,const MDDState& parentCombined,int layer,const var<int>::Ptr x,const MDDIntSet& vals,bool up)
//{
//   nbCS++;
//   _mddspec->createState(result,parentDown,parentCombined,layer,x,vals,up);
//   result.computeHash();
//}

void MDDStateFactory::createStateDown(MDDState& result,const MDDState& parentDown,const MDDState& parentCombined,int layer,const var<int>::Ptr x,const MDDIntSet& vals,bool up)
{
   //std::cout << "Start createStateDown\n";
   if (vals.isSingleton()) {
      //std::cout << "Singleton\n";
      MDDSKey key { &parentDown, &parentCombined, layer, vals.singleton() };
      //std::cout << "Made key\n";
      MDDState* match = nullptr;
      auto loc = _downHash.get(key,match);
      //std::cout << "Made loc " << loc << "\n";
      if (loc) {
         ++hitCSDown;
         result.copyState(*match);
      } else {
         nbCSDown++;
         _mddspec->fullStateDown(result,parentDown,parentCombined,layer,x,MDDIntSet(vals.singleton()),up);
         result.computeHash();
         MDDState* pdc = new (_mem) MDDState(parentDown.clone(_mem));
         MDDState* pcc = new (_mem) MDDState(parentCombined.clone(_mem));
         MDDSKey ikey { pdc, pcc, layer, vals.singleton() };
         _downHash.insert(ikey,new (_mem) MDDState(result.clone(_mem)));
      }
   } else {
      nbCSDown++;
      //std::cout << "Pre fullStateDown\n";
      _mddspec->fullStateDown(result,parentDown,parentCombined,layer,x,vals,up);
      //std::cout << "Post fullStateDown\n";
      result.computeHash();
   }
   //std::cout << "End createStateDown\n";
}
void MDDStateFactory::createStateUp(MDDState& result,const MDDState& childUp,const MDDState& childCombined,int layer,const var<int>::Ptr x,const MDDIntSet& vals)
{
   if (vals.isSingleton()) {
      MDDSKey key { &childUp, &childCombined, layer, vals.singleton() };
      MDDState* match = nullptr;
      auto loc = _upHash.get(key,match);
      if (loc) {
         ++hitCSUp;
         result.copyState(*match);
      } else {
         nbCSUp++;
         _mddspec->fullStateUp(result,childUp,childCombined,layer,x,MDDIntSet(vals.singleton()));
         result.computeHash();
         MDDState* cuc = new (_mem) MDDState(childUp.clone(_mem));
         MDDState* ccc = new (_mem) MDDState(childCombined.clone(_mem));
         MDDSKey ikey { cuc, ccc, layer, vals.singleton() };
         _upHash.insert(ikey,new (_mem) MDDState(result.clone(_mem)));
      }
   } else {
      nbCSUp++;
      _mddspec->fullStateUp(result,childUp,childCombined,layer,x,vals);
      result.computeHash();
   }
}

void MDDStateFactory::splitState(MDDState*& result,MDDNode* n,const MDDState& parentDown,const MDDState& parentCombined,int layer,const var<int>::Ptr x,int val)
{
//std::cout << "Start splitState\n";
   MDDSKey key { &parentDown, &parentCombined, layer, val };
//std::cout << "Pre loc\n";
   auto loc = _downHash.get(key,result);
   if (loc) {
//std::cout << "loc\n";
      ++hitCSDown;
      result = new (_mem) MDDState(result->clone(_mem));
   } else {
//std::cout << "not loc\n";
      nbCSDown++;
      result = new (_mem) MDDState(_mddspec,new (_mem) char[_mddspec->layoutSizeDown()],Down);
      //std::cout << "Pre fullState\n";
      _mddspec->fullStateDown(*result,parentDown,parentCombined,layer,x,MDDIntSet(val),true);
      //std::cout << "Post fullState\n";
      result->computeHash();
      MDDState* pdc = new (_mem) MDDState(parentDown.clone(_mem));
      MDDState* pcc = new (_mem) MDDState(parentCombined.clone(_mem));
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
