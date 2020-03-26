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

namespace Factory {
   MDDProperty::Ptr makeProperty(enum MDDProperty::Direction dir,short id,unsigned short ofs,int init,int max)
   {
      MDDProperty::Ptr rv;
      if (max <= 1)
         rv = new MDDPBit(dir,id,ofs,init);
      else if (max <= 255)
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
   : _exist(nullptr)
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

MDDConstraintDescriptor& MDDSpec::makeConstraintDescriptor(const Factory::Veci& v,const char* n)
{
   return constraints.emplace_back(v,n);
   //return constraints[constraints.size()-1];
}

int MDDStateSpec::addState(MDDConstraintDescriptor& d, int init,int max)
{
   int aid = (int)_attrs.size();
   _attrs.push_back(Factory::makeProperty(MDDProperty::Down,aid, 0, init, max));
   d.addProperty(aid);
   return aid;
}
int MDDStateSpec::addStateUp(MDDConstraintDescriptor& d, int init,int max)
{
   int aid = (int)_attrs.size();
   _attrs.push_back(Factory::makeProperty(MDDProperty::Up,aid, 0, init, max));
   d.addProperty(aid);
   return aid;
}
int MDDStateSpec::addBSState(MDDConstraintDescriptor& d,int nbb,unsigned char init)
{
   int aid = (int)_attrs.size();
   _attrs.push_back(Factory::makeBSProperty(MDDProperty::Down,aid,0,nbb,init));
   d.addProperty(aid);
   return aid;
}
int MDDStateSpec::addBSStateUp(MDDConstraintDescriptor& d,int nbb,unsigned char init)
{
   int aid = (int)_attrs.size();
   _attrs.push_back(Factory::makeBSProperty(MDDProperty::Up,aid,0,nbb,init));
   d.addProperty(aid);
   return aid;
}

std::vector<int> MDDStateSpec::addStates(MDDConstraintDescriptor& d,int from, int to,int max, std::function<int(int)> clo)
{
   std::vector<int> res;
   for(int i = from; i <= to; i++)
      res.push_back(addState(d,clo(i),max));
   return res;
}
std::vector<int> MDDStateSpec::addStates(MDDConstraintDescriptor& d,int max, std::initializer_list<int> inputs)
{
   std::vector<int> res;
   for(auto& v : inputs)
      res.push_back(addState(d,v,max));
   return res;
}

int MDDSpec::addState(MDDConstraintDescriptor& d,int init,int max)
{
   auto rv = MDDStateSpec::addState(d,init,max);
   return rv;
}
int MDDSpec::addStateUp(MDDConstraintDescriptor& d,int init,int max)
{
   auto rv = MDDStateSpec::addStateUp(d,init,max);
   return rv;
}
int MDDSpec::addBSState(MDDConstraintDescriptor& d,int nbb,unsigned char init)
{
   auto rv = MDDStateSpec::addBSState(d,nbb,init);
   return rv;   
}
int MDDSpec::addBSStateUp(MDDConstraintDescriptor& d,int nbb,unsigned char init)
{
   auto rv = MDDStateSpec::addBSStateUp(d,nbb,init);
   return rv;   
}

void MDDSpec::onFixpoint(std::function<void(const MDDState&)> onFix)
{
   _onFix.emplace_back(onFix);
}

void MDDSpec::addArc(const MDDConstraintDescriptor& d,ArcFun a){
   auto& b = _exist;
   if(_exist == nullptr)
      _exist = [=] (const auto& p,const auto& c,var<int>::Ptr var, int val,bool up) -> bool {
                  return (!d.inScope(var) || a(p,c, var, val,up));
               };
   else
      _exist = [=] (const auto& p,const auto& c,var<int>::Ptr var, int val,bool up) -> bool {
                  return (!d.inScope(var) || a(p,c,var, val,up)) && b(p,c, var, val,up);
               };
}
void MDDSpec::addTransition(int p,lambdaTrans t)
{   
   for(auto& cd : constraints)
      if (cd.ownsProperty(p)) {
         if (isUp(p)) {
            cd.registerUp((int)_uptrans.size());
            _uptrans.emplace_back(std::move(t));
         } else {
            cd.registerTransition((int)_transition.size());
            _transition.emplace_back(std::move(t));
         }
         break;
      }  
}
void MDDSpec::addRelaxation(int p,lambdaRelax r)
{
   for(auto& cd : constraints)
      if (cd.ownsProperty(p)) {
         cd.registerRelaxation((int)_relaxation.size());
         break;
      }  
   _relaxation.emplace_back(std::move(r));
}
void MDDSpec::addSimilarity(int p,lambdaSim s)
{
   for(auto& cd : constraints)
      if (cd.ownsProperty(p)) {
         cd.registerSimilarity((int)_similarity.size());
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

bool MDDSpec::exist(const MDDState& a,const MDDState& c,var<int>::Ptr x,int v,bool up)
{
   return _exist(a,c,x,v,up);
}

void MDDSpec::createState(MDDState& result,const MDDState& parent,unsigned l,var<int>::Ptr var,int v)
{
   result.clear();
   for(auto& c :constraints) {
      if(c.inScope(var))
         for(auto i : c.transitions()) 
            _transition[i](result,parent,var,v);
      else
         for(auto i : c) 
            result.setProp(i,parent);
   }
   result.relax(parent.isRelaxed());
}

MDDState MDDSpec::createState(Storage::Ptr& mem,const MDDState& parent,unsigned l,var<int>::Ptr var, int v)
{
   MDDState result(this,(char*)mem->allocate(layoutSize()));
   result.copyState(parent);
   for(auto& c :constraints) {
      if(c.inScope(var))
         for(auto i : c.transitions()) 
            _transition[i](result,parent,var,v);
      // else
      //    for(auto i : c) 
      //       result.setProp(i, parent);
   }
   result.relax(parent.isRelaxed());
   return result;
}

void MDDSpec::updateState(bool set,MDDState& target,const MDDState& source,var<int>::Ptr var,int v)
{
   // when set is true. compute T^{Up}(source,var,val) and store in target
   // when set is false compute Relax(target,T^{Up}(source,var,val)) and store in target.
   MDDState temp(this,(char*)alloca(sizeof(char)*layoutSize()));
   temp.copyState(target);
   for(auto& c : constraints) {
      if (c.inScope(var))
         for(auto l : c.uptrans())
            _uptrans[l](temp,source,var,v);
      else
         for(auto i : c)
            if (isUp(i))
               temp.setProp(i,source);
   }   
   if (set)
      target.copyState(temp);
   else 
      relaxation(target,temp);          
}


double MDDSpec::similarity(const MDDState& a,const MDDState& b) 
{
   double dist = 0;
   for(auto& cstr : constraints) {
      for(auto p : cstr.similarities()) {
         double abSim = _similarity[p](a,b);
         dist += abSim;
      }
   }
   return dist;
}

void MDDSpec::relaxation(MDDState& a,const MDDState& b)
{
   ///bool aRelax = a.isRelaxed();
   //a.clear();
   //MDDState orig(this,(char*)alloca(layoutSize()));
   //orig.copyState(a);
   // for(auto& cstr : constraints)
   //    for(auto p : cstr.relaxations()) 
   //       _relaxation[p](a,a,b);
   for(const auto& relax : _relaxation)
      relax(a,a,b);
   //a.relax(aRelax || a.stateChange(orig));
   a.relax();
}

MDDState MDDSpec::relaxation(Storage::Ptr& mem,const MDDState& a,const MDDState& b)
{
   MDDState result(this,(char*)mem->allocate(layoutSize()));
   for(auto& cstr : constraints) 
      for(auto p : cstr.relaxations()) 
         _relaxation[p](result,a,b);       
   result.relax();
   return result;
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
