//
//  mddstate.cpp
//  minicpp
//
//  Created by Waldy on 10/28/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

#include "mddstate.hpp"
#include <algorithm>

namespace Factory {
   MDDProperty::Ptr makeProperty(short id,unsigned short ofs,int init,int max)
   {
      MDDProperty::Ptr rv;
      if (max <= 255)
         rv = new MDDPByte(id,ofs,init,max);
      else
         rv = new MDDPInt(id,ofs,init,max);
      return rv;
   }
}


MDDConstraintDescriptor::MDDConstraintDescriptor(const Factory::Veci& vars, const char* name)
: _vars(vars), _vset(vars), _name(name){}

MDDConstraintDescriptor::MDDConstraintDescriptor(const MDDConstraintDescriptor& d)
: _vars(d._vars), _vset(d._vset), _properties(d._properties), _name(d._name) {}

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
   _lsz = 0;
   for(auto a : _attrs) {
      a->_ofs = _lsz;
      _lsz += a->storageSize();
   }
}

MDDConstraintDescriptor& MDDSpec::makeConstraintDescriptor(const Factory::Veci& v,const char* n)
{
   MDDConstraintDescriptor d(v,n);
   constraints.push_back(d);
   return constraints[constraints.size()-1];
}

int MDDStateSpec::addState(MDDConstraintDescriptor& d, int init,int max)
{
   int aid = (int)_attrs.size();
   _attrs.push_back(Factory::makeProperty(aid, 0, init, max));
   d.addProperty(aid);
   return aid;
}

void MDDStateSpec::addStates(MDDConstraintDescriptor& d,int from, int to, std::function<int(int)> clo)
{
   for(int i = from; i <= to; i++)
      addState(d,clo(i));
}
void MDDStateSpec::addStates(MDDConstraintDescriptor& d,std::initializer_list<int> inputs)
{
   for(auto& v : inputs)
      addState(d,v);
}

int MDDSpec::addState(MDDConstraintDescriptor& d,int init,int max)
{
   auto rv = MDDStateSpec::addState(d,init,max);
   transistionLambdas.push_back(nullptr);
   relaxationLambdas.push_back(nullptr);
   similarityLambdas.push_back(nullptr);
   return rv;
}

void MDDSpec::addArc(const MDDConstraintDescriptor& d,std::function<bool(const MDDState&, var<int>::Ptr, int)> a){
    auto& b = arcLambda;
    if(arcLambda == nullptr)
       arcLambda = [=] (const MDDState& p, var<int>::Ptr var, int val) -> bool {
                           return (a(p, var, val) || !d.member(var));
                         };
   else
       arcLambda = [=] (const MDDState& p, var<int>::Ptr var, int val) -> bool {
                     return (a(p, var, val) || !d.member(var)) && b(p, var, val);
                   };
}
void MDDSpec::addTransition(int p,std::function<int(const MDDState&, var<int>::Ptr, int)> t)
{
    transistionLambdas[p] = t;
}
void MDDSpec::addRelaxation(int p,std::function<int(const MDDState&,const MDDState&)> r)
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
   for(int k=0;k < size();k++)
      rootState.init(k);
   rootState.hash();
   std::cout << "ROOT:" << rootState << std::endl;
   return rootState;
}

std::pair<MDDState,bool> MDDSpec::
createState(Storage::Ptr& mem,const MDDState& parent,
                                              var<int>::Ptr var, int v)
{
  if(arcLambda(parent, var, v)){
       MDDState result(this,(char*)mem->allocate(layoutSize()));
       for(auto& c :constraints){
          auto& p = c.properties();
          for(auto i : p){
             if(c.member(var))
                result.set(i,transistionLambdas[i](parent,var,v));
            else
               result.set(i, parent.at(i));
          }
      }
       result.hash();
       return std::pair<MDDState,bool>(result,true);
    }
    return std::pair<MDDState,bool>(MDDState(),false);
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

      mdd.addArc(d,[=] (const MDDState& p,var<int>::Ptr var, int val) -> bool {
         return (p.at(minC) + values.member(val) <= ub) &&
                ((p.at(maxC) + values.member(val) +  p.at(rem) - 1) >= lb);
      });

      mdd.addTransition(minC,[minC,values] (const auto& p,auto x, int v) -> int { return p.at(minC) + values.member(v);});
      mdd.addTransition(maxC,[maxC,values] (const auto& p,auto x, int v) -> int { return p.at(maxC) + values.member(v);});
      mdd.addTransition(rem,[rem] (const auto& p,auto x,int v) -> int { return p.at(rem) - 1;});

      mdd.addRelaxation(minC,[minC](auto l,auto r) -> int { return std::min(l.at(minC), r.at(minC));});
      mdd.addRelaxation(maxC,[maxC](auto l,auto r) -> int { return std::max(l.at(maxC), r.at(maxC));});
      mdd.addRelaxation(rem ,[rem](auto l,auto r) -> int { return l.at(rem);});

      mdd.addSimilarity(minC,[minC](auto l,auto r) -> double { return abs(l.at(minC) - r.at(minC)); });
      mdd.addSimilarity(maxC,[maxC](auto l,auto r) -> double { return abs(l.at(maxC) - r.at(maxC)); });
      mdd.addSimilarity(rem ,[rem] (auto l,auto r) -> double { return 0; });
   }

   void allDiffMDD(MDDSpec& mdd, const Factory::Veci& vars)
   {
      int os = (int) mdd.size();
      mdd.append(vars);
      MDDConstraintDescriptor d(vars,"allDiffMDD");
      auto udom = domRange(vars);
      int minDom = udom.first, maxDom = udom.second;

      mdd.addStates(d,minDom,maxDom,[] (int i) -> int   { return 1;});
      mdd.addArc(d,[=] (auto p,auto var,int val) -> bool { return p.at(os+val-minDom);});

      for(int i = minDom; i <= maxDom; i++){
         int idx = os+i-minDom;
         mdd.addTransition(idx,[idx,i](const auto& p,auto var, int val) -> int { return  p.at(idx) && i != val;});
         mdd.addRelaxation(idx,[idx](auto l,auto r) -> int { return l.at(idx) || r.at(idx);});
         mdd.addSimilarity(idx,[idx](auto l,auto r) -> double { return abs(l.at(idx)- r.at(idx));});
      }
   }

    void  seqMDD(MDDSpec& spec,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues)
   {

      int minFIdx = 0,minLIdx = len-1;
      int maxFIdx = len,maxLIdx = len*2-1;
      int os = (int) spec.size();
      spec.append(vars);
      ValueSet values(rawValues);
      MDDConstraintDescriptor desc(vars,"seqMDD");

      spec.addStates(desc,minFIdx,minLIdx,[=] (int i) -> int { return (i - minLIdx); });
      spec.addStates(desc,maxFIdx,maxLIdx,[=] (int i) -> int { return (i - maxLIdx); });

      minLIdx += os; minFIdx+=os;
      maxLIdx += os; maxFIdx+=os;

      spec.addArc(desc,[=] (auto p,auto x,int v) -> bool {
                     bool inS = values.member(v);
                     int minv = p.at(maxLIdx) - p.at(minFIdx) + inS;
                     return (p.at(os) < 0 && minv >= lb && p.at(minLIdx) + inS <= ub)
                        ||  (minv >= lb && p.at(minLIdx) - p.at(maxFIdx) + inS <= ub);
                  });

      lambdaMap d = toDict(minFIdx,maxLIdx,[=] (int i) -> lambdaTrans {
         if (i == maxLIdx || i == minLIdx)
            return [=] (const auto& p,auto x, int v) -> int {return p.at(i)+values.member(v);};
         return [i] (const auto& p,auto x, int v) -> int {return p.at(i+1);};
      });
      spec.addTransitions(d);

      for(int i = minFIdx; i <= minLIdx; i++)
         spec.addRelaxation(i,[i](auto l,auto r)->int{return std::min(l.at(i),r.at(i));});

      for(int i = maxFIdx; i <= maxLIdx; i++)
         spec.addRelaxation(i,[i](auto l,auto r)->int{return std::max(l.at(i),r.at(i));});

      for(int i = minFIdx; i <= maxLIdx; i++)
         spec.addSimilarity(i,[i](auto l,auto r)->double{return abs(l.at(i)- r.at(i));});
   }

   void  gccMDD(MDDSpec& spec,const Factory::Veci& vars,const std::map<int,int>& ub)
   {
      int os = (int) spec.size();
      spec.append(vars);
      auto udom = domRange(vars);
      int domSize = udom.second - udom.first + 1;
      int minFDom = os, minLDom = os + domSize-1;
      int maxFDom = os + domSize,maxLDom = os + domSize*2-1;
      int minDom = udom.first;
      ValueMap<int> values(udom.first, udom.second,0,ub);
      MDDConstraintDescriptor desc(vars,"gccMDD");

      spec.addStates(desc,minFDom, maxLDom, [] (int i) -> int { return 0; });

      spec.addArc(desc,[=](auto p,auto x,int v)->bool{return p.at(os+v-minDom) < values[v];});

      lambdaMap d = toDict(minFDom,maxLDom,[=] (int i) -> lambdaTrans {
              if (i <= minLDom)
                 return [=] (const auto& p,auto x, int v) -> int {return p.at(i) + ((v - minDom) == i);};
              return [=] (const auto& p,auto x, int v) -> int {return p.at(i) + ((v - minDom) == (i - domSize));};
           });
      spec.addTransitions(d);

      for(ORInt i = minFDom; i <= minLDom; i++){
         spec.addRelaxation(i,[i](auto l,auto r)->int{return std::min(l.at(i),r.at(i));});
         spec.addSimilarity(i,[i](auto l,auto r)->double{return std::min(l.at(i),r.at(i));});
      }

      for(ORInt i = maxFDom; i <= maxLDom; i++){
         spec.addRelaxation(i,[i](auto l,auto r)->int{return std::max(l.at(i),r.at(i));});
         spec.addSimilarity(i,[i](auto l,auto r)->double{return std::max(l.at(i),r.at(i));});
      }
   }
}
