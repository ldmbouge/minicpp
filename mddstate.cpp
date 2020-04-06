//
//  mddstate.cpp
//  minicpp
//
//  Created by Waldy on 10/28/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

#include "mddstate.hpp"
#include <algorithm>
#include <limits.h>

void printSet(const MDDIntSet& s) {
   std::cout << s << std::endl;
}


namespace Factory {
   MDDProperty::Ptr makeProperty(enum MDDProperty::Direction dir,short id,unsigned short ofs,int init,int max)
   {
      MDDProperty::Ptr rv;
      if (max <= 255)
         rv = new MDDPByte(dir,id,ofs,init,max);
      else
         rv = new MDDPInt(dir,id,ofs,init,max);
      return rv;
   }
   MDDProperty::Ptr makeBSProperty(enum MDDProperty::Direction dir,short id,unsigned short ofs,int nbb,unsigned char init)
   {
      MDDProperty::Ptr rv = new MDDPBitSequence(dir,id,ofs,nbb,init);
      return rv;
   }
}


MDDConstraintDescriptor::MDDConstraintDescriptor(const Factory::Veci& vars, const char* name)
   : _vars(vars), _vset(vars), _name(name){}

MDDConstraintDescriptor::MDDConstraintDescriptor(const MDDConstraintDescriptor& d)
   : _vars(d._vars), _vset(d._vset), _name(d._name),
     _properties(d._properties),
     _tid(d._tid),
     _rid(d._rid),
     _sid(d._sid),
     _utid(d._utid)
{}

MDDSpec::MDDSpec()
{}

void MDDSpec::append(const Factory::Veci& y)
{
   for(auto e : y)
      if(std::find(x.cbegin(),x.cend(),e) == x.cend())
         x.push_back(e);
   std::cout << "size of x: " << x.size() << std::endl;
}

void MDDSpec::varOrder()
{
   std::sort(x.begin(),x.end(),[](const var<int>::Ptr& a,const var<int>::Ptr& b) {
                                  return a->getId() < b->getId();
                               });   
}

void MDDStateSpec::layout()
{
   size_t lszBit = 0;
   for(auto a : _attrs) {
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

MDDConstraintDescriptor::Ptr MDDSpec::makeConstraintDescriptor(const Factory::Veci& v,const char* n)
{
   return constraints.emplace_back(new MDDConstraintDescriptor(v,n));
}

int MDDStateSpec::addState(MDDConstraintDescriptor::Ptr d, int init,int max)
{
   int aid = (int)_attrs.size();
   _attrs.push_back(Factory::makeProperty(MDDProperty::Down,aid, 0, init, max));
   d->addProperty(aid);
   return aid;
}
int MDDStateSpec::addStateUp(MDDConstraintDescriptor::Ptr d, int init,int max)
{
   int aid = (int)_attrs.size();
   _attrs.push_back(Factory::makeProperty(MDDProperty::Up,aid, 0, init, max));
   d->addProperty(aid);
   return aid;
}
int MDDStateSpec::addBSState(MDDConstraintDescriptor::Ptr d,int nbb,unsigned char init)
{
   int aid = (int)_attrs.size();
   _attrs.push_back(Factory::makeBSProperty(MDDProperty::Down,aid,0,nbb,init));
   d->addProperty(aid);
   return aid;
}
int MDDStateSpec::addBSStateUp(MDDConstraintDescriptor::Ptr d,int nbb,unsigned char init)
{
   int aid = (int)_attrs.size();
   _attrs.push_back(Factory::makeBSProperty(MDDProperty::Up,aid,0,nbb,init));
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

int MDDSpec::addState(MDDConstraintDescriptor::Ptr d,int init,int max)
{
   auto rv = MDDStateSpec::addState(d,init,max);
   return rv;
}
int MDDSpec::addStateUp(MDDConstraintDescriptor::Ptr d,int init,int max)
{
   auto rv = MDDStateSpec::addStateUp(d,init,max);
   return rv;
}
int MDDSpec::addBSState(MDDConstraintDescriptor::Ptr d,int nbb,unsigned char init)
{
   auto rv = MDDStateSpec::addBSState(d,nbb,init);
   return rv;   
}
int MDDSpec::addBSStateUp(MDDConstraintDescriptor::Ptr d,int nbb,unsigned char init)
{
   auto rv = MDDStateSpec::addBSStateUp(d,nbb,init);
   return rv;   
}

void MDDSpec::onFixpoint(std::function<void(const MDDState&)> onFix)
{
   _onFix.emplace_back(onFix);
}
bool MDDSpec::exist(const MDDState& a,const MDDState& c,var<int>::Ptr x,int v,bool up) const noexcept
{
   bool arcOk = true;
   for(auto& exist :  _exists) {
      if (std::get<0>(exist)->inScope(x))
         arcOk = std::get<1>(exist)(a,c,x,v,up);
      if (!arcOk) break;
   }
   return arcOk;
      //return _exist(a,c,x,v,up);   
}

void MDDSpec::addArc(MDDConstraintDescriptor::Ptr d,ArcFun a)
{
   _exists.emplace_back(std::make_pair<MDDConstraintDescriptor::Ptr,ArcFun>(std::move(d),std::move(a)));
   /*
   auto& b = _exist;
   if(_exist == nullptr)
      _exist = [d,a] (const auto& p,const auto& c,var<int>::Ptr var, int val,bool up) -> bool {
                  return (!d.inScope(var) || a(p,c, var, val,up));
               };
   else
      _exist = [d,a,b] (const auto& p,const auto& c,var<int>::Ptr var, int val,bool up) -> bool {
                  return (!d.inScope(var) || a(p,c,var, val,up)) && b(p,c, var, val,up);
               };
   */
}
void MDDSpec::addTransition(int p,lambdaTrans t)
{   
   for(auto& cd : constraints)
      if (cd->ownsProperty(p)) {
         if (isUp(p)) {
            cd->registerUp((int)_uptrans.size());
            _uptrans.emplace_back(std::move(t));
         } else {
            cd->registerTransition((int)_transition.size());
            _transition.emplace_back(std::move(t));
         }
         break;
      }  
}
void MDDSpec::addSimilarity(int p,lambdaSim s)
{
   for(auto& cd : constraints)
      if (cd->ownsProperty(p)) {
         cd->registerSimilarity((int)_similarity.size());
         break;
      }  
   _similarity.emplace_back(std::move(s));
}
void MDDSpec::addTransitions(const lambdaMap& map)
{
   for(auto& kv : map)
      addTransition(kv.first,kv.second);
}
MDDState MDDSpec::rootState(Storage::Ptr& mem)
{
   MDDState rootState(this,(char*)mem->allocate(layoutSize()));
   for(auto k=0u;k < size();k++)
      rootState.init(k);
   std::cout << "ROOT:" << rootState << std::endl;
   return rootState;
}


void MDDSpec::reachedFixpoint(const MDDState& sink)
{
   for(auto& fix : _onFix)
      fix(sink);
}

void MDDSpec::compile()
{
   const unsigned nbL = (unsigned)x.size();
   _transLayer.reserve(nbL);
   _frameLayer.reserve(nbL);
   _uptransLayer.reserve(nbL);
   _upframeLayer.reserve(nbL);
   for(auto i = 0u;i < nbL;i++) {      
      auto& layer   = _transLayer.emplace_back(std::vector<lambdaTrans>());
      auto& upLayer = _uptransLayer.emplace_back(std::vector<lambdaTrans>());
      auto& frame   = _frameLayer.emplace_back(std::vector<int>());
      auto& upFrame = _upframeLayer.emplace_back(std::vector<int>());
      for(auto& c : constraints)
         if (c->inScope(x[i]))  {
            for(auto j : c->transitions())
               layer.emplace_back(_transition[j]);
            for(auto j : c->uptrans())
               upLayer.emplace_back(_uptrans[j]);
         }
         else {
            for(auto j : *c)
               frame.emplace_back(j);
            for(auto j : *c)
               if (isUp(j))
                  upFrame.emplace_back(j);                   
         }
   }
}

void MDDSpec::createState(MDDState& result,const MDDState& parent,unsigned l,var<int>::Ptr var,const MDDIntSet& v,bool hasUp)
{
   result.clear();
   for(const auto& t : _transLayer[l])
      t(result,parent,var,v,hasUp);
   for(auto p : _frameLayer[l])
      result.setProp(p,parent);
   result.relax(parent.isRelaxed());
}

void MDDSpec::updateState(bool set,MDDState& target,const MDDState& source,unsigned l,var<int>::Ptr var,const MDDIntSet& v)
{
   // when set is true. compute T^{Up}(source,var,val) and store in target
   // when set is false compute Relax(target,T^{Up}(source,var,val)) and store in target.
   if (set) {
      for(const auto& t : _uptransLayer[l])
         t(target,source,var,v,true);
      for(auto p : _upframeLayer[l])
         target.setProp(p,source);
   } else {
      MDDState temp(this,(char*)alloca(sizeof(char)*layoutSize()));
      temp.copyState(target);
      for(const auto& t : _uptransLayer[l])
         t(temp,source,var,v,true);
      for(auto p : _upframeLayer[l])
         temp.setProp(p,source);
      relaxation(target,temp);          
   }
}


double MDDSpec::similarity(const MDDState& a,const MDDState& b) 
{
   double dist = 0;
   for(auto& cstr : constraints) {
      for(auto p : cstr->similarities()) {
         double abSim = _similarity[p](a,b);
         dist += abSim;
      }
   }
   return dist;
}

std::pair<int,int> domRange(const Factory::Veci& vars)
{
   std::pair<int,int> udom;
   udom.first = INT_MAX;
   udom.second = -INT_MAX;
   for(auto& x : vars){
      udom.first = (udom.first > x->min()) ? x->min() : udom.first;
      udom.second = (udom.second < x->max()) ? x->max() : udom.second;
   }
   return udom;
}
