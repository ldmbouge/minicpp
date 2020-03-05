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
   MDDProperty::Ptr makeProperty(short id,unsigned short ofs,int init,int max)
   {
      MDDProperty::Ptr rv;
      if (max <= 1)
         rv = new MDDPBit(id,ofs,init);
      else if (max <= 255)
         rv = new MDDPByte(id,ofs,init,max);
      else
         rv = new MDDPInt(id,ofs,init,max);
      return rv;
   }
   MDDProperty::Ptr makeBSProperty(short id,unsigned short ofs,int nbb,unsigned char init)
   {
      MDDProperty::Ptr rv = new MDDPBitSequence(id,ofs,nbb,init);
      return rv;
   }
}


MDDConstraintDescriptor::MDDConstraintDescriptor(const Factory::Veci& vars, const char* name)
: _vars(vars), _vset(vars), _name(name){}

MDDConstraintDescriptor::MDDConstraintDescriptor(const MDDConstraintDescriptor& d)
: _vars(d._vars), _vset(d._vset), _name(d._name), _properties(d._properties) {}

MDDSpec::MDDSpec()
   :arcLambda(nullptr)
{}

void MDDSpec::append(const Factory::Veci& y)
{
    for(auto e : y)
       if(std::find(x.cbegin(),x.cend(),e) == x.cend())
          x.push_back(e);
    std::cout << "size of x: " << x.size() << std::endl;
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
   constraints.emplace_back(v,n);
   return constraints[constraints.size()-1];
}

int MDDStateSpec::addState(MDDConstraintDescriptor& d, int init,int max)
{
   int aid = (int)_attrs.size();
   _attrs.push_back(Factory::makeProperty(aid, 0, init, max));
   d.addProperty(aid);
   return aid;
}
int MDDStateSpec::addBSState(MDDConstraintDescriptor& d,int nbb,unsigned char init)
{
   int aid = (int)_attrs.size();
   _attrs.push_back(Factory::makeBSProperty(aid,0,nbb,init));
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
   transistionLambdas.push_back(nullptr);
   relaxationLambdas.push_back(nullptr);
   similarityLambdas.push_back(nullptr);
   return rv;
}
int MDDSpec::addBSState(MDDConstraintDescriptor& d,int nbb,unsigned char init)
{
   auto rv = MDDStateSpec::addBSState(d,nbb,init);
   transistionLambdas.push_back(nullptr);
   relaxationLambdas.push_back(nullptr);
   similarityLambdas.push_back(nullptr);
   return rv;   
}


void MDDSpec::addArc(const MDDConstraintDescriptor& d,lambdaArc a){
    auto& b = arcLambda;
    if(arcLambda == nullptr)
       arcLambda = [=] (const MDDState& p,unsigned layer,var<int>::Ptr var, int val) -> bool {
                      return (a(p,layer, var, val) || !d.member(var));
                   };
   else
      arcLambda = [=] (const MDDState& p,unsigned layer,var<int>::Ptr var, int val) -> bool {
                     return (a(p,layer,var, val) || !d.member(var)) && b(p,layer, var, val);
                   };
}
void MDDSpec::addTransition(int p,std::function<void(MDDState&,const MDDState&, var<int>::Ptr, int)> t)
{
    transistionLambdas[p] = t;
}
void MDDSpec::addRelaxation(int p,std::function<void(MDDState&,const MDDState&,const MDDState&)> r)
{
    relaxationLambdas[p] = r;
}
void MDDSpec::addSimilarity(int p,std::function<double(const MDDState&,const MDDState&)> s)
{
    similarityLambdas[p] = s;
}
void MDDSpec::addTransitions(lambdaMap& map)
{
   for(auto& kv : map)
      transistionLambdas[kv.first] = kv.second;
}
MDDState MDDSpec::rootState(Storage::Ptr& mem)
{
   MDDState rootState(this,(char*)mem->allocate(layoutSize()));
   for(auto k=0u;k < size();k++)
      rootState.init(k);
   rootState.hash();
   std::cout << "ROOT:" << rootState << std::endl;
   return rootState;
}

bool MDDSpec::exist(const MDDState& a,unsigned l,var<int>::Ptr x,int v)
{
   return arcLambda(a,l,x,v);
}

bool MDDSpec::createState(MDDState& result,const MDDState& parent,unsigned l,var<int>::Ptr var,int v)
{
  result.clear();
  if(arcLambda(parent,l,var, v)) {
     for(auto& c :constraints) {
        if(c.member(var))
           for(auto i : c) 
              transistionLambdas[i](result,parent,var,v);
        else
           for(auto i : c) 
              result.setProp(i,parent);
     }
     result.hash();
     result.relax(parent.isRelaxed());
     return true;
  }
  return false;
}

std::pair<MDDState,bool> MDDSpec::createState(Storage::Ptr& mem,const MDDState& parent,unsigned l,
                                              var<int>::Ptr var, int v)
{
   if(arcLambda(parent,l,var, v)){
       MDDState result(this,(char*)mem->allocate(layoutSize()));
       for(auto& c :constraints) {
          if(c.member(var))
             for(auto i : c) 
                transistionLambdas[i](result,parent,var,v);
          else
             for(auto i : c) 
                result.setProp(i, parent);
       }
       result.hash();
       result.relax(parent.isRelaxed());
       return std::pair<MDDState,bool>(result,true);
    }
    return std::pair<MDDState,bool>(MDDState(),false);
}

double MDDSpec::similarity(const MDDState& a,const MDDState& b) 
{
  double dist = 0;
  for(auto& cstr : constraints) {
    for(auto p : cstr) {
      double abSim = similarityLambdas[p](a,b);
      dist += abSim;
    }
  }
  return dist;
}

void MDDSpec::relaxation(MDDState& a,const MDDState& b)
{
  a.clear();
  for(auto& cstr : constraints)
    for(auto p : cstr) 
       relaxationLambdas[p](a,a,b);      
}

MDDState MDDSpec::relaxation(Storage::Ptr& mem,const MDDState& a,const MDDState& b)
{
  MDDState result(this,(char*)mem->allocate(layoutSize()));
  for(auto& cstr : constraints) {
    for(auto p : cstr) {
       relaxationLambdas[p](result,a,b);
    }
  }   
  result.hash();
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

namespace Factory {
   void amongMDD(MDDSpec& mdd, const Factory::Veci& x, int lb, int ub, std::set<int> rawValues) {
      mdd.append(x);
      ValueSet values(rawValues);
      auto& d = mdd.makeConstraintDescriptor(x,"amongMDD");
      const int minC = mdd.addState(d,0,x.size());
      const int maxC = mdd.addState(d,0,x.size());
      const int rem  = mdd.addState(d,(int)x.size(),x.size());

      mdd.addArc(d,[=] (const MDDState& p,unsigned l,var<int>::Ptr var, int val) -> bool {
         return (p.at(minC) + values.member(val) <= ub) &&
                ((p.at(maxC) + values.member(val) +  p.at(rem) - 1) >= lb);
      });

      mdd.addTransition(minC,[minC,values] (auto& out,const auto& p,auto x, int v) { out.set(minC,p.at(minC) + values.member(v));});
      mdd.addTransition(maxC,[maxC,values] (auto& out,const auto& p,auto x, int v) { out.set(maxC,p.at(maxC) + values.member(v));});
      mdd.addTransition(rem,[rem] (auto& out,const auto& p,auto x,int v)           { out.set(rem,p.at(rem) - 1);});

      mdd.addRelaxation(minC,[minC](auto& out,const auto& l,const auto& r) { out.set(minC,std::min(l.at(minC), r.at(minC)));});
      mdd.addRelaxation(maxC,[maxC](auto& out,const auto& l,const auto& r) { out.set(maxC,std::max(l.at(maxC), r.at(maxC)));});
      mdd.addRelaxation(rem ,[rem](auto& out,const auto& l,const auto& r)  { out.set(rem,l.at(rem));});

      mdd.addSimilarity(minC,[minC](auto l,auto r) -> double { return abs(l.at(minC) - r.at(minC)); });
      mdd.addSimilarity(maxC,[maxC](auto l,auto r) -> double { return abs(l.at(maxC) - r.at(maxC)); });
      mdd.addSimilarity(rem ,[] (auto l,auto r) -> double { return 0; });
   }

   void allDiffMDD(MDDSpec& mdd, const Factory::Veci& vars)
   {
      mdd.append(vars);
      auto& d = mdd.makeConstraintDescriptor(vars,"allDiffMdd");
      auto udom = domRange(vars);
      int minDom = udom.first;
      const int all  = mdd.addBSState(d,udom.second - udom.first + 1,0);
      const int some = mdd.addBSState(d,udom.second - udom.first + 1,0);
      const int len  = mdd.addState(d,0,vars.size());
      
      mdd.addTransition(all,[minDom,all](auto& out,const auto& in,auto var,int val) {
                               out.setBS(all,in.getBS(all));
                               out.getBS(all).set(val - minDom);
                            });
      mdd.addTransition(some,[minDom,some](auto& out,const auto& in,auto var,int val) {
                                out.setBS(some,in.getBS(some));
                                out.getBS(some).set(val - minDom);
                            });
      mdd.addTransition(len,[len,d](auto& out,const auto& in,auto var,int val) { out.set(len,in.at(len) + d.member(var));});
      
      mdd.addRelaxation(all,[all](auto& out,const auto& l,const auto& r)     {
                               out.getBS(all).setBinAND(l.getBS(all),r.getBS(all));
                            });
      mdd.addRelaxation(some,[some](auto& out,const auto& l,const auto& r)     {
                                out.getBS(some).setBinOR(l.getBS(some),r.getBS(some));
                            });
      mdd.addRelaxation(len,[len](auto& out,const auto& l,const auto& r)     { out.set(len,l.at(len));});

      mdd.addArc(d,[minDom,some,all,len](const auto& p,unsigned l,auto var,int val) -> bool  {
                      bool notOk = p.getBS(all).getBit(val - minDom) ||
                         (p.getBS(some).cardinality() == p.at(len)  && p.getBS(some).getBit(val - minDom));
                      return !notOk;
                   });
   }

   void seqMDD(MDDSpec& spec,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues)
   {
      int min = 0,minLIdx = len-1;
      int maxFIdx = len,max = len*2-1;
      spec.append(vars);
      ValueSet values(rawValues);
      auto& desc = spec.makeConstraintDescriptor(vars,"seqMDD");

      std::vector<int> ps = spec.addStates(desc,min,max,SHRT_MAX,[max,len,minLIdx] (int i) -> int {
         return (i - (i <= minLIdx ? minLIdx : (i > len ? max : 0)));
      });
      int p0 = ps[0]; int pminL = ps[minLIdx]; int pmaxF = ps[maxFIdx];
      int pmin = ps[min]; int pmax = ps[max];
      spec.addArc(desc,[=] (const auto& p,unsigned l,auto x,int v) -> bool {
                     bool inS = values.member(v);
                     int minv = p.at(pmax) - p.at(pmin) + inS;
                     return (p.at(p0) < 0 && minv >= lb && p.at(pminL) + inS <= ub)
                        ||  (minv >= lb && p.at(pminL) - p.at(pmaxF) + inS <= ub);
                  });

      lambdaMap d = toDict(min,max,ps,[=] (int i,int pi) -> lambdaTrans {
         if (i == max || i == minLIdx)
            return [pi,values] (auto& out,const auto& p,auto x, int v)  { out.set(pi,p.at(pi)+values.member(v));};
         return [pi] (auto& out,const auto& p,auto x, int v)  { out.set(pi+1,p.at(pi+1));};
      });
      
      spec.addTransitions(d);

      for(int i = min; i <= minLIdx; i++){
         int p = ps[i];
         spec.addRelaxation(p,[p](auto& out,const auto& l,const auto& r) { out.set(p,std::min(l.at(p),r.at(p)));});
      }
      for(int i = maxFIdx; i <= max; i++){
         int p = ps[i];
         spec.addRelaxation(p,[p](auto& out,const auto& l,const auto& r) { out.set(p,std::max(l.at(p),r.at(p)));});
      }
      for(auto i : ps)
         spec.addSimilarity(i,[i](auto l,auto r)->double{return abs(l.at(i)- r.at(i));});
   }

   void gccMDD(MDDSpec& spec,const Factory::Veci& vars,const std::map<int,int>& ub)
   {
      spec.append(vars);
      int sz = (int) vars.size();
      auto udom = domRange(vars);
      int dz = udom.second - udom.first + 1;
      int minFDom = 0, minLDom = dz-1;
      int maxFDom = dz,maxLDom = dz*2-1;
      int min = udom.first;
      ValueMap<int> values(udom.first, udom.second,0,ub);
      auto& desc = spec.makeConstraintDescriptor(vars,"gccMDD");

      std::vector<int> ps = spec.addStates(desc,minFDom, maxLDom,sz,[] (int i) -> int { return 0; });

      spec.addArc(desc,[=](const auto& p,unsigned l,auto x,int v)->bool{
                          return p.at(ps[v-min]) < values[v];
                       });

      lambdaMap d = toDict(minFDom,maxLDom,ps,[dz,min,minLDom,ps] (int i,int pi) -> lambdaTrans {
              if (i <= minLDom)
                 return [=] (auto& out,const auto& p,auto x, int v) { out.set(pi,p.at(pi) + ((v - min) == i));};
              return [=] (auto& out,const auto& p,auto x, int v)    { out.set(pi,p.at(pi) + ((v - min) == (i - dz)));};
           });
      spec.addTransitions(d);

      for(ORInt i = minFDom; i <= minLDom; i++){
         int p = ps[i];
         spec.addRelaxation(p,[p](auto& out,auto l,auto r)  { out.set(p,std::min(l.at(p),r.at(p)));});
         spec.addSimilarity(p,[p](auto l,auto r)->double{return std::min(l.at(p),r.at(p));});
      }

      for(ORInt i = maxFDom; i <= maxLDom; i++){
         int p = ps[i];
         spec.addRelaxation(p,[p](auto& out,auto l,auto r) { out.set(p,std::max(l.at(p),r.at(p)));});
         spec.addSimilarity(p,[p](auto l,auto r)->double{return std::max(l.at(p),r.at(p));});
      }
   }
}
