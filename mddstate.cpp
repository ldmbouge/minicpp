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
     _properties(d._properties),
     _tid(d._tid),
     _rid(d._rid),
     _sid(d._sid),
     _utid(d._utid)
{}

MDDSpec::MDDSpec()
{
    _approximateSplitting = false;
}

void MDDSpec::varOrder()
{
   std::sort(x.begin(),x.end(),[](const var<int>::Ptr& a,const var<int>::Ptr& b) {
      return a->getId() < b->getId();
   });
}

MDDStateSpec::MDDStateSpec()
{
   _mxp = 4;
   _nbp = 0;
   _attrs = new MDDProperty*[_mxp];
   _omap = nullptr;
}

void MDDStateSpec::addProperty(MDDProperty::Ptr p) noexcept
{
   if (_nbp == _mxp) {
      MDDProperty** ns = new MDDProperty*[_mxp<<1];
      for(short i =0;i < _nbp;i++)
         ns[i] = _attrs[i];
      delete[]_attrs;
      _attrs = ns;
      _mxp = _mxp << 1;
   }
   _attrs[_nbp++] = p.get();
}

void MDDStateSpec::layout()
{
   size_t lszBit = 0;
   for(int p = 0;p <_nbp;p++) {
      auto a = _attrs[p];
      lszBit = a->setOffset(lszBit);
   }
   size_t boB = lszBit & 0x7;
   if (boB != 0)
      lszBit = (lszBit | 0x7) + 1;
   _lsz = lszBit >> 3;
   _lsz = (_lsz & 0x7) ? (_lsz | 0x7)+1 : _lsz;
   assert(_lsz % 8 == 0); // # bytes is always a multiple of 8.
   std::cout << "State requires:" << _lsz << " bytes" << std::endl;
}

int MDDStateSpec::addState(MDDConstraintDescriptor::Ptr d, int init,int max,enum RelaxWith rw)
{
   int aid = (int)_nbp;
   addProperty(Factory::makeProperty(aid, 0, init, max,rw));
   d->addProperty(aid);
   return aid;
}
int MDDStateSpec::addBSState(MDDConstraintDescriptor::Ptr d,int nbb,unsigned char init,enum RelaxWith rw)
{
   int aid = (int)_nbp;
   addProperty(Factory::makeBSProperty(aid,0,nbb,init,rw));
   d->addProperty(aid);
   return aid;
}
int MDDStateSpec::addSWState(MDDConstraintDescriptor::Ptr d,int len,int init,int finit,enum RelaxWith rw)
{
   int aid = (int)_nbp;
   addProperty(Factory::makeWinProperty(aid,0,len,init,finit,rw));
   d->addProperty(aid);
   return aid;
}


std::vector<int> MDDStateSpec::addStates(MDDConstraintDescriptor::Ptr d,int from, int to,int max, std::function<int(int)> clo)
{
   std::vector<int> res;
   for(int i = from; i <= to; i++)
      res.push_back(addState(d,clo(i),max));
   return res;
}
std::vector<int> MDDStateSpec::addStates(MDDConstraintDescriptor::Ptr d,int max, std::initializer_list<int> inputs)
{
   std::vector<int> res;
   for(auto& v : inputs)
      res.push_back(addState(d,v,max));
   return res;
}

int MDDSpec::addState(MDDConstraintDescriptor::Ptr d,int init,int max,enum RelaxWith rw)
{
   auto rv = MDDStateSpec::addState(d,init,max,rw);
   return rv;
}
int MDDSpec::addBSState(MDDConstraintDescriptor::Ptr d,int nbb,unsigned char init,enum RelaxWith rw)
{
   auto rv = MDDStateSpec::addBSState(d,nbb,init,rw);
   return rv;
}
int MDDSpec::addSWState(MDDConstraintDescriptor::Ptr d,int len,int init,int finit,enum RelaxWith rw)
{
   auto rv = MDDStateSpec::addSWState(d,len,init,finit,rw);
   return rv;
}

void MDDSpec::onFixpoint(FixFun onFix)
{
   _onFix.emplace_back(onFix);
}
void MDDSpec::splitOnLargest(SplitFun onSplit)
{
   _onSplit.emplace_back(onSplit);
}
void MDDSpec::equivalenceClassValue(EquivalenceValueFun equivalenceValue)
{
   _equivalenceValue.emplace_back(equivalenceValue);
}
int MDDSpec::numEquivalenceClasses()
{
   return (int)_equivalenceValue.size();
}

void MDDSpec::updateNode(MDDState& a) const noexcept
{
   for(auto& fun : _updates)
      fun(a);
   a.computeHash();
}

int nbAECall = 0;
int nbAEFail = 0;

bool MDDSpec::exist(const MDDState& a,const MDDState& c,const var<int>::Ptr& x,int v,bool up) const noexcept
{
   ++nbAECall;
   bool arcOk = true;
   for(const auto& exist : _scopedExists[x->getId()]) {
      arcOk = exist(a,c,x,v,up);
      if (!arcOk) {
         ++nbAEFail;
         break;
      }
   }
   return arcOk;
}

int nbCONSCall = 0;
int nbCONSFail = 0;

bool MDDSpec::consistent(const MDDState& a,const var<int>::Ptr& x) const noexcept
{
   ++nbCONSCall;
   bool cons = true;
   for(auto& consFun : _scopedConsistent[x->getId()]) {
      cons = consFun(a);
      if (!cons) {
         ++nbCONSFail;
         break;
      }
   }
   return cons;
}

void MDDSpec::nodeExist(MDDConstraintDescriptor::Ptr d,NodeFun a)
{
   _nodeExists.emplace_back(std::make_pair<MDDConstraintDescriptor::Ptr,NodeFun>(std::move(d),std::move(a)));
}
void MDDSpec::arcExist(MDDConstraintDescriptor::Ptr d,ArcFun a)
{
   _exists.emplace_back(std::make_pair<MDDConstraintDescriptor::Ptr,ArcFun>(std::move(d),std::move(a)));
}
void MDDSpec::updateNode(UpdateFun nf)
{
   _updates.emplace_back(std::move(nf));
}
void MDDSpec::transitionDown(int p,std::set<int> sp,lambdaTrans t)
{
   for(auto& cd : constraints)
      if (cd->ownsProperty(p)) {
         int tid = (int)_transition.size();
         cd->registerDown(tid);
         _attrs[p]->setDirection(Down);
         _attrs[p]->setAntecedents(sp);
         _attrs[p]->setDown(tid);
         _transition.emplace_back(std::move(t));
         break;
      }
}

void MDDSpec::transitionUp(int p,std::set<int> sp,lambdaTrans t)
{
   for(auto& cd : constraints)
      if (cd->ownsProperty(p)) {
         int tid = (int)_uptrans.size();
         cd->registerUp(tid);
         _attrs[p]->setDirection(Up);
         _attrs[p]->setUp(tid);
         _uptrans.emplace_back(std::move(t));
         break;
      }
}

void MDDSpec::transitionDown(const lambdaMap& map)
{
   for(auto& kv : map) {
      const auto& sp = std::get<0>(kv.second);
      const auto& f  = std::get<1>(kv.second);
      transitionDown(kv.first,sp,f);
   }
}

void MDDSpec::transitionUp(const lambdaMap& map)
{
   for(auto& kv : map) {
      const auto& sp = std::get<0>(kv.second);
      const auto& f  = std::get<1>(kv.second);
      transitionUp(kv.first,sp,f);
   }
}

MDDState MDDSpec::rootState(Storage::Ptr& mem)
{
   MDDState rootState(this,(char*)mem->allocate(layoutSize()));
   for(auto k=0;k < size();k++)
      rootState.init(k);
   std::cout << "ROOT:" << rootState << std::endl;
   return rootState;
}


void MDDSpec::reachedFixpoint(const MDDState& sink)
{
   for(auto& fix : _onFix)
      fix(sink);
}

void LayerDesc::zoning(const MDDSpec& spec)
{
   zoningDown(spec);
   zoningUp(spec);
   for(auto p : _dframe) _dprop.setProp(p);
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
            auto sOfs = spec.startOfs(fstProp);
            auto eOfs = spec.endOfs(lstProp);
            _uzones.emplace_back(Zone(sOfs,eOfs,zp));
            zp.clear();
            zp.insert(fstProp = lstProp = p);
         }
      }
   }
   if (fstProp != -1) {
      auto sOfs = spec.startOfs(fstProp);
      auto eOfs = spec.endOfs(lstProp);
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
            auto sOfs = spec.startOfs(fstProp);
            auto eOfs = spec.endOfs(lstProp);
            _dzones.emplace_back(Zone(sOfs,eOfs,zp));
            zp.clear();
            zp.insert(fstProp = lstProp = p);
         }
      }
   }
   if (fstProp != -1) {
      auto sOfs = spec.startOfs(fstProp);
      auto eOfs = spec.endOfs(lstProp);
      _dzones.emplace_back(Zone(sOfs,eOfs,zp));
   }
}

void MDDSpec::compile()
{
   _omap = new MDDPropSet[_nbp];
   for(int i=0;i < _nbp;i++) _omap[i] = MDDPropSet(_nbp);
   for(int p=0;p < _nbp;p++) {
      auto& out = _omap[p];
      for(int s=0;s < _nbp;s++) {
         const auto& ants = _attrs[s]->antecedents();
         if (ants.find(p)!= ants.end()) {
            out.setProp(s);
         }
      }
      std::cout << "omap[" << p << "] = " << out << '\n';
   }
   const unsigned nbL = (unsigned)x.size();
   _transLayer.reserve(nbL);
   _frameLayer.reserve(nbL);
   _uptransLayer.reserve(nbL);
   for(auto i = 0u;i < nbL;i++) {
      auto& layer   = _transLayer.emplace_back(std::vector<lambdaTrans>());
      auto& upLayer = _uptransLayer.emplace_back(std::vector<lambdaTrans>());
      auto& frame   = _frameLayer.emplace_back(LayerDesc(_nbp));
      for(auto& c : constraints) {
         if (c->inScope(x[i]))  {
            for(auto j : c->transitions())
               layer.emplace_back(_transition[j]);
            for(auto j : c->uptrans())
               upLayer.emplace_back(_uptrans[j]);
         } else { // x[i] does not appear in constraint c. So the properties of c should be subject to frame axioms (copied)
            for(auto j : c->properties()) {
               if (isDown(j))
                  frame.addDownProp(j);
               if (isUp(j))
                  frame.addUpProp(j);
            }
         }
      }
      frame.zoning(*this);
   }
   int lid,uid;
   std::tie(lid,uid) = idRange(x);
   const int sz = uid + 1;
   _scopedExists.resize(sz);
   _scopedConsistent.resize(sz);
   for(auto& exist : _exists) {
      auto& cd  = std::get<0>(exist);
      auto& fun = std::get<1>(exist);
      auto& vars = cd->vars();
      for(auto& v : vars)
         _scopedExists[v->getId()].emplace_back(fun);
   }
   for(auto& nex : _nodeExists) {
      auto& cd  = std::get<0>(nex);
      auto& fun = std::get<1>(nex);
      auto& vars = cd->vars();
      for(auto& v : vars)
         _scopedConsistent[v->getId()].emplace_back(fun);
   }
   std::set<int> upProps;
   int fstUp = -1,lstUp = -1;
   for(int i=0;i < _nbp;i++) {
      if (!_attrs[i]->isUp()) continue;
      if (fstUp == -1)
         upProps.insert(fstUp = lstUp = i);
      else {
         if (i == fstUp - 1)
            upProps.insert(fstUp = i);
         else if (i == lstUp + 1)
            upProps.insert(lstUp = i);
         else if (i >= fstUp && i <= lstUp)
            upProps.insert(i);
         else {
            _upZones.emplace_back(Zone(startOfs(fstUp),endOfs(lstUp),upProps));
            upProps.clear();
            upProps.insert(fstUp = lstUp = i);
         }
      }
   }
   if (fstUp != -1)
      _upZones.emplace_back(Zone(startOfs(fstUp),endOfs(lstUp),upProps));
   for(int p = 0 ; p < _nbp;p++) {
      if (_xRelax.find(p) == _xRelax.end())
         _dRelax.push_back(p);
   }
}

void MDDSpec::copyStateUp(MDDState& result,const MDDState& source)
{
   if (usesUp()) {
      for(const auto& z : _upZones)
         result.copyZone(z,source);
   }
}

void MDDSpec::createState(MDDState& result,const MDDState& parent,unsigned l,const var<int>::Ptr& var,const MDDIntSet& v,bool hasUp)
{
   result.clear();
   for(const auto& t : _transLayer[l])
      t(result,parent,var,v,hasUp);
   _frameLayer[l].frameDown(result,parent);
   result.relaxDown(parent.isDownRelaxed() || v.size() > 1);
}

void MDDSpec::createStateIncr(const MDDPropSet& out,MDDState& result,const MDDState& parent,unsigned l,const var<int>::Ptr& var,
                              const MDDIntSet& v,bool hasUp)
{
   result.clear();
   for(auto p : out) {
      int tid = _frameLayer[l].hasProp(p) ? -1 : _attrs[p]->getDown();
      if (tid != -1)
         _transition[tid](result,parent,var,v,hasUp); // actual transition
      else 
         result.setProp(p,parent); // frame axiom
   }
   result.relaxDown(parent.isDownRelaxed() || v.size() > 1);
}

void MDDSpec::relaxation(MDDState& a,const MDDState& b) const noexcept
{
   for(auto p : _dRelax) {
      switch(_attrs[p]->relaxFun()) {
         case MinFun: a.minWith(p,b);break;
         case MaxFun: a.maxWith(p,b);break;
         case External: break;
      }
   }
   for(const auto& relax : _relaxation)
      relax(a,a,b);
   a.computeHash();
}

void MDDSpec::relaxationIncr(const MDDPropSet& out,MDDState& a,const MDDState& b) const noexcept
{
   for(auto p : out) {
      switch(_attrs[p]->relaxFun()) {
         case MinFun: a.minWith(p,b);break;
         case MaxFun: a.maxWith(p,b);break;
         case External:
            _relaxation[_attrs[p]->getRelax()](a,a,b);
            break;
      }
   }
   a.computeHash();
}

void MDDSpec::updateState(MDDState& target,const MDDState& source,unsigned l,const var<int>::Ptr& var,const MDDIntSet& v)
{
   for(const auto& t : _uptransLayer[l])
      t(target,source,var,v,true);
   _frameLayer[l].frameUp(target,source);
   target.relaxUp(source.isUpRelaxed() || v.size() > 1);
}


int nbCS  = 0;
int hitCS = 0;
int vs = 0;

MDDStateFactory::MDDStateFactory(MDDSpec* spec)
   : _mddspec(spec),
     _mem(new Pool()),
     _hash(_mem,300149),
     _mark(_mem->mark()),
     _enabled(false)
{
}


void MDDStateFactory::createState(MDDState& result,const MDDState& parent,int layer,const var<int>::Ptr x,const MDDIntSet& vals,bool up)
{
   nbCS++;
   _mddspec->createState(result,parent,layer,x,vals,up);
   result.computeHash();
}

bool MDDStateFactory::splitState(MDDState*& result,MDDNode* n,const MDDState& parent,int layer,const var<int>::Ptr x,int val)
{
   auto mark = _mem->mark();
   const int nbb = (int)_mddspec->layoutSize();
   char* membuf = new (_mem) char[nbb];
   bzero(membuf,nbb);
   MDDState* upState = new (_mem) MDDState(_mddspec,membuf);
   _mddspec->copyStateUp(*upState,n->getState());

   MDDSKey key { &parent, upState, val };
   auto loc = _hash.get(key,result);
   if (loc) {
      ++hitCS;
      _mem->clear(mark);
      return true;
   } else {
      nbCS++;
      result = new (_mem) MDDState(_mddspec,new (_mem) char[nbb]);
      _mddspec->copyStateUp(*result,n->getState());
      _mddspec->createState(*result,parent,layer,x,MDDIntSet(val),true);
      _mddspec->updateNode(*result);
      bool isOk = _mddspec->consistent(*result,x);
      if (isOk) {
         result->computeHash();
         MDDState* pc = new (_mem) MDDState(parent.clone(_mem));
         MDDSKey ikey { pc, upState, val };
         _hash.insert(ikey,result);
      }
      return isOk;
   }
}

void MDDStateFactory::clear()
{
   _hash.clear();
   _mem->clear(_mark);
   _enabled = true;
}
