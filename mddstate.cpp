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
            if (_attrs[i]->isUp())
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
   bool aRelax = a.isRelaxed();
  a.clear();
  MDDState orig(this,(char*)alloca(layoutSize()));
  orig.copyState(a);
  for(auto& cstr : constraints)
     for(auto p : cstr.relaxations()) 
        _relaxation[p](a,a,b);
  a.relax(aRelax || a.stateChange(orig));
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

namespace Factory {
   void amongMDD(MDDSpec& mdd, const Factory::Veci& x, int lb, int ub, std::set<int> rawValues) {
      mdd.append(x);
      ValueSet values(rawValues);
      auto& d = mdd.makeConstraintDescriptor(x,"amongMDD");
      const int minC = mdd.addState(d,0,x.size());
      const int maxC = mdd.addState(d,0,x.size());
      const int rem  = mdd.addState(d,(int)x.size(),x.size());

      mdd.addArc(d,[=] (const auto& p,const auto& c,var<int>::Ptr var, int val,bool) -> bool {
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
      const int n    = (int)vars.size();
      const int all  = mdd.addBSState(d,udom.second - udom.first + 1,0);
      const int some = mdd.addBSState(d,udom.second - udom.first + 1,0);
      const int len  = mdd.addState(d,0,vars.size());
      const int allu = mdd.addBSStateUp(d,udom.second - udom.first + 1,0);
      const int someu = mdd.addBSStateUp(d,udom.second - udom.first + 1,1);
      
      mdd.addTransition(all,[minDom,all](auto& out,const auto& in,auto var,int val) {
                               out.setBS(all,in.getBS(all)).set(val - minDom);
                            });
      mdd.addTransition(some,[minDom,some](auto& out,const auto& in,auto var,int val) {
                                out.setBS(some,in.getBS(some)).set(val - minDom);
                            });
      mdd.addTransition(len,[len,d](auto& out,const auto& in,auto var,int val) { out.set(len,in.at(len) + 1);});
      mdd.addTransition(allu,[minDom,allu](auto& out,const auto& in,auto var,int val) {
                                  out.setBS(allu,in.getBS(allu)).set(val - minDom);
                               });
      mdd.addTransition(someu,[minDom,someu](auto& out,const auto& in,auto var,int val) {
                                  out.setBS(someu,in.getBS(someu)).set(val - minDom);
                               });
      
      mdd.addRelaxation(all,[all](auto& out,const auto& l,const auto& r)     {
                               out.getBS(all).setBinAND(l.getBS(all),r.getBS(all));
                            });
      mdd.addRelaxation(some,[some](auto& out,const auto& l,const auto& r)     {
                                out.getBS(some).setBinOR(l.getBS(some),r.getBS(some));
                            });
      mdd.addRelaxation(len,[len](auto& out,const auto& l,const auto& r)     { out.set(len,l.at(len));});
      mdd.addRelaxation(allu,[allu](auto& out,const auto& l,const auto& r)     {
                               out.getBS(allu).setBinAND(l.getBS(allu),r.getBS(allu));
                            });
      mdd.addRelaxation(someu,[someu](auto& out,const auto& l,const auto& r)     {
                                out.getBS(someu).setBinOR(l.getBS(someu),r.getBS(someu));
                            });

      mdd.addArc(d,[minDom,some,all,len,someu,allu,n](const auto& p,const auto& c,auto var,int val,bool) -> bool  {
                      bool notOk = p.getBS(all).getBit(val - minDom) ||
                         (p.getBS(some).cardinality() == (unsigned)p.at(len)  && p.getBS(some).getBit(val - minDom));
                      bool upNotOk = c.getBS(allu).getBit(val - minDom) ||
                         (c.getBS(someu).cardinality() == n - c.at(len) && c.getBS(someu).getBit(val - minDom));
                      MDDBSValue suV = c.getBS(someu);
                      MDDBSValue both((char*)alloca(sizeof(unsigned long long)*suV.nbWords()),suV.nbWords(),suV.bitLen());
                      both.setBinOR(suV,p.getBS(some)).set(val-minDom);
                      bool mixNotOk = both.cardinality() < n;
                      return !notOk && !upNotOk && !mixNotOk;
                   });
      mdd.addSimilarity(all,[all](const auto& l,const auto& r) -> double {
                               MDDBSValue lv = l.getBS(all);
                               MDDBSValue tmp((char*)alloca(sizeof(char)*l.byteSize(all)),lv.nbWords(),lv.bitLen());
                               tmp.setBinXOR(lv,r.getBS(all));
                               return tmp.cardinality();
                            });
      mdd.addSimilarity(some,[some](const auto& l,const auto& r) -> double {
                               MDDBSValue lv = l.getBS(some);
                               MDDBSValue tmp((char*)alloca(sizeof(char)*l.byteSize(some)),lv.nbWords(),lv.bitLen());
                               tmp.setBinXOR(lv,r.getBS(some));
                               return tmp.cardinality();
                            });
      mdd.addSimilarity(len,[](const auto& l,const auto& r) -> double {
                               return 0;
                            });
   }

   void seqMDD(MDDSpec& spec,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues)
   {
      int minFIdx = 0,minLIdx = len-1;
      int maxFIdx = len,maxLIdx = len*2-1;
      spec.append(vars);
      ValueSet values(rawValues);
      auto& desc = spec.makeConstraintDescriptor(vars,"seqMDD");

      std::vector<int> ps = spec.addStates(desc,minFIdx,maxLIdx,SHRT_MAX,[maxLIdx,len,minLIdx] (int i) -> int {
         return (i - (i <= minLIdx ? minLIdx : (i >= len ? maxLIdx : 0)));
      });
      int p0 = ps[0];
      const int pminL = ps[minLIdx];
      const int pmaxF = ps[maxFIdx];
      const int pmin = ps[minFIdx];
      const int pmax = ps[maxLIdx];
      spec.addArc(desc,[=] (const auto& p,const auto& c,auto x,int v,bool) -> bool {
                          bool inS = values.member(v);
                          int minv = p.at(pmax) - p.at(pmin) + inS;
                          return (p.at(p0) < 0 &&  minv >= lb && p.at(pminL) + inS               <= ub)
                             ||  (p.at(p0) >= 0 && minv >= lb && p.at(pminL) - p.at(pmaxF) + inS <= ub);
                       });
      
      spec.addTransitions(toDict(ps[minFIdx],
                                 ps[minLIdx]-1,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,int v) { out.set(i,p.at(i+1));};}));
      spec.addTransitions(toDict(ps[maxFIdx],
                                 ps[maxLIdx]-1,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,int v) { out.set(i,p.at(i+1));};}));
      spec.addTransition(ps[minLIdx],[values,
                                      k=ps[minLIdx]](auto& out,const auto& p,auto x,int v) { out.set(k,p.at(k)+values.member(v));});
      spec.addTransition(ps[maxLIdx],[values,
                                      k=ps[maxLIdx]](auto& out,const auto& p,auto x,int v) { out.set(k,p.at(k)+values.member(v));});
      
      for(int i = minFIdx; i <= minLIdx; i++)
         spec.addRelaxation(ps[i],[p=ps[i]](auto& out,const auto& l,const auto& r) { out.set(p,std::min(l.at(p),r.at(p)));});
      for(int i = maxFIdx; i <= maxLIdx; i++)
         spec.addRelaxation(ps[i],[p=ps[i]](auto& out,const auto& l,const auto& r) { out.set(p,std::max(l.at(p),r.at(p)));});
      
      for(auto i : ps)
         spec.addSimilarity(i,[i](auto l,auto r)->double{return abs(l.at(i)- r.at(i));});
   }

   void seqMDD2(MDDSpec& spec,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues)
   {
      int minFIdx = 0,minLIdx = len-1;
      int maxFIdx = len,maxLIdx = len*2-1;
      int nb = len*2;
      spec.append(vars);
      ValueSet values(rawValues);
      auto& desc = spec.makeConstraintDescriptor(vars,"seqMDD");
      std::vector<int> ps(nb+1);
      for(int i = minFIdx;i < nb;i++)
         ps[i] = spec.addState(desc,0,len);       // init @ 0, largest value is length of window. 
      ps[nb] = spec.addState(desc,0,INT_MAX); // init @ 0, largest value is number of variables. 

      const int minL = ps[minLIdx];
      const int maxF = ps[maxFIdx];
      const int minF = ps[minFIdx];
      const int maxL = ps[maxLIdx];
      const int pnb  = ps[nb];

      spec.addTransitions(toDict(minF,minL-1,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,int v) { out.set(i,p.at(i+1));};}));
      spec.addTransitions(toDict(maxF,maxL-1,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,int v) { out.set(i,p.at(i+1));};}));
      spec.addTransition(minL,[values,minL](auto& out,const auto& p,auto x,int v) { out.set(minL,p.at(minL)+values.member(v));});
      spec.addTransition(maxL,[values,maxL](auto& out,const auto& p,auto x,int v) { out.set(maxL,p.at(maxL)+values.member(v));});
      spec.addTransition(pnb,[pnb](auto& out,const auto& p,auto x,int v) {
                                out.set(pnb,p.at(pnb)+1);
                             });

      spec.addArc(desc,[=] (const auto& p,const auto& c,auto x,int v,bool) -> bool {
                          bool inS = values.member(v);
                          if (p.at(pnb) >= len - 1) {
                             bool c0 = p.at(maxL) + inS - p.at(minF) >= lb;
                             bool c1 = p.at(minL) + inS - p.at(maxF) <= ub;
                             return c0 && c1;
                          } else {
                             bool c0 = len - (p.at(pnb)+1) + p.at(maxL) + inS >= lb;
                             bool c1 = p.at(minL) + inS <= ub;
                             return c0 && c1;
                          }
                       });      
      
      for(int i = minFIdx; i <= minLIdx; i++)
         spec.addRelaxation(ps[i],[p=ps[i]](auto& out,const auto& l,const auto& r) { out.set(p,std::min(l.at(p),r.at(p)));});
      for(int i = maxFIdx; i <= maxLIdx; i++)
         spec.addRelaxation(ps[i],[p=ps[i]](auto& out,const auto& l,const auto& r) { out.set(p,std::max(l.at(p),r.at(p)));});
      spec.addRelaxation(pnb,[pnb](auto& out,const auto& l,const auto& r) { out.set(pnb,l.at(pnb));});
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

      spec.addArc(desc,[=](const auto& p,const auto& c,auto x,int v,bool)->bool{
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


  void sumMDD(MDDSpec& mdd, const Factory::Veci& vars, const std::vector<int>& array, int lb, int ub) {
      // Enforce
      //   sum(i, array[i]*vars[i]) >= lb and
      //   sum(i, array[i]*vars[i]) <= ub
      mdd.append(vars);
     
      auto& d = mdd.makeConstraintDescriptor(vars,"sumMDD");

      // Define the states: minimum and maximum weighted value (initialize at 0, maximum is INT_MAX (when negative values are allowed).
      const int minW = mdd.addState(d, 0, INT_MAX);
      const int maxW = mdd.addState(d, 0, INT_MAX);
      const int minWup = mdd.addStateUp(d, 0, INT_MAX);
      const int maxWup = mdd.addStateUp(d, 0, INT_MAX);

      // State 'len' is needed to capture the index i, to express array[i]*val when vars[i]=val.
      const int len  = mdd.addState(d, 0, vars.size());

      // The lower bound needs the bottom-up state information to be effective.
      mdd.addArc(d,[=] (const auto& p, const auto& c, var<int>::Ptr var, int val,bool upPass) -> bool {
	  if (upPass==true) {
             return ((p.at(minW) + val*array[p.at(len)] + c.at(minWup) <= ub) &&
                     (p.at(maxW) + val*array[p.at(len)] + c.at(maxWup) >= lb));
	  } else {
	    return ((p.at(minW) + val*array[p.at(len)]  <= ub) );
	  }
      });

      mdd.addTransition(minW,[minW,array,len] (auto& out,const auto& p,auto var, int val) {
	  out.set(minW, p.at(minW) + array[p.at(len)]*val);});
      mdd.addTransition(maxW,[maxW,array,len] (auto& out,const auto& p,auto var, int val) {
	  out.set(maxW, p.at(maxW) + array[p.at(len)]*val);});

      mdd.addTransition(minWup,[minWup,array,len] (auto& out,const auto& in,auto var, int val) {
	  if (in.at(len) >= 1) {
	    out.set(minWup, in.at(minWup) + array[in.at(len)-1]*val);
	  }
	});
      mdd.addTransition(maxWup,[maxWup,array,len] (auto& out,const auto& in,auto var, int val) {
	  if (in.at(len) >= 1) {
	    out.set(maxWup, in.at(maxWup) + array[in.at(len)-1]*val);
	  }
	});
      
      mdd.addTransition(len, [len] (auto& out,const auto& p,auto var, int val) {
	  out.set(len,  p.at(len) + 1);});      

      mdd.addRelaxation(minW,[minW](auto& out,const auto& l,const auto& r) { out.set(minW,std::min(l.at(minW), r.at(minW)));});
      mdd.addRelaxation(maxW,[maxW](auto& out,const auto& l,const auto& r) { out.set(maxW,std::max(l.at(maxW), r.at(maxW)));});
      mdd.addRelaxation(minWup,[minWup](auto& out,const auto& l,const auto& r) { out.set(minWup,std::min(l.at(minWup), r.at(minWup)));});
      mdd.addRelaxation(maxWup,[maxWup](auto& out,const auto& l,const auto& r) { out.set(maxWup,std::max(l.at(maxWup), r.at(maxWup)));});
      mdd.addRelaxation(len, [len](auto& out,const auto& l,const auto& r)  { out.set(len,l.at(len));});

      mdd.addSimilarity(minW,[minW](auto l,auto r) -> double { return abs(l.at(minW) - r.at(minW)); });
      mdd.addSimilarity(maxW,[maxW](auto l,auto r) -> double { return abs(l.at(maxW) - r.at(maxW)); });
      mdd.addSimilarity(minWup,[minWup](auto l,auto r) -> double { return abs(l.at(minWup) - r.at(minWup)); });
      mdd.addSimilarity(maxWup,[maxWup](auto l,auto r) -> double { return abs(l.at(maxWup) - r.at(maxWup)); });
      mdd.addSimilarity(len ,[] (auto l,auto r) -> double { return 0; }); 
  }
}
