//
//  mddstate.hpp
//  minicpp
//
//  Created by Waldy on 10/28/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

#ifndef mddstate_hpp
#define mddstate_hpp

#include "handle.hpp"
#include "intvar.hpp"
#include <set>
#include <cstring>
#include <map>

class MDDSpec;

class MDDProperty {
protected:
   short _id;
   unsigned short _ofs;
   virtual size_t storageSize() const = 0;
public:
   typedef handle_ptr<MDDProperty> Ptr;
   MDDProperty(const MDDProperty& p) : _id(p._id),_ofs(p._ofs) {}
   MDDProperty(MDDProperty&& p) : _id(p._id),_ofs(p._ofs) {}
   MDDProperty(short id,unsigned short ofs) : _id(id),_ofs(ofs) {}
   MDDProperty& operator=(const MDDProperty& p) { _id = p._id;_ofs = p._ofs; return *this;}
   virtual int get(char* buf) const = 0;
   virtual void setInt(char* buf,int v) = 0;
   virtual void print(std::ostream& os) const  {}
   friend class MDDSpec;
};

class MDDPInt :public MDDProperty {
   int _init;
   int _max;
   size_t storageSize() const override     { return sizeof(int);}
public:
   typedef handle_ptr<MDDProperty> Ptr;
   MDDPInt(short id,unsigned short ofs,int init,int max=0x7fffffff)
      : MDDProperty(id,ofs),_init(init),_max(max) {}
   int get(char* buf) const override       { return *reinterpret_cast<int*>(buf+_ofs);}
   void setInt(char* buf,int v) override   { *reinterpret_cast<int*>(buf+_ofs) = v;}  
   void print(std::ostream& os) const override  {
      os << "PInt(" << _id << ',' << _ofs << ',' << _init << ',' << _max << ')';
   }
   friend class MDDSpec;
};

inline std::ostream& operator<<(std::ostream& os,MDDProperty::Ptr p)
{
   p->print(os);return os;
}

class ValueSet {
   char* _data;
   int  _min,_max;
   int  _sz;
public:
   ValueSet(const std::set<int>& s) {
      _min = *s.begin();
      _max = *s.begin();
      for(auto v : s) {
         _min = _min < v ? _min : v;
         _max = _max > v ? _max : v;
      }
      _sz = _max - _min + 1;
      _data = new char[_sz];
      memset(_data,0,sizeof(char)*_sz);
      for(auto v : s)
         _data[v - _min] = 1;
   }
   bool member(int v) const {
      if (_min <= v && v <= _max)
         return _data[v - _min];
      else return false;
   }
};

template <typename T>
class ValueMap {
   T* _data;
   int  _min,_max;
   int  _sz;
public:
   ValueMap(int min, int max, T defaut, const std::map<int,T>& s) {
      _min = min;
      _max = max;
      _sz = _max - _min + 1;
      _data = new T[_sz];
      memset(_data,defaut,sizeof(T)*_sz);
      for(auto kv : s)
         _data[kv.first - min] = kv.second;
   }
   ValueMap(int min, int max, std::function<T(int)>& clo){
      _min = min;
      _max = max;
      _sz = _max - _min + 1;
      _data = new T[_sz];
      for(int i = min; i <= max; i++)
         _data[i-_min] = clo(i);
   }
   const T& operator[](int idx) const{
      return _data[idx - _min];
   }
};

class MDDState {  // An actual state of an MDDNode. 
   std::vector<int> _state;  // the state per. se
   long              _hash;  // a hash value of the state to speed up equality testing. 
public:
   typedef handle_ptr<MDDState> Ptr;
   MDDState() : _state(),_hash(0) {}
   MDDState(size_t sz) : _state(sz),_hash(0) {}
   MDDState(const MDDState& s) : _state(s._state),_hash(s._hash) {}
   MDDState(MDDState&& s) : _state(std::move(s._state)),_hash(s._hash) {}
   void addState(int s) { _state.push_back(s);}
   auto size()  const { return _state.size();}
   auto begin() { return _state.begin();}   // to iterate
   auto end()   { return _state.end();}
   int at(int i) const { return _state[i];}
   int operator[](int i) const { return _state[i];}  // to _read_ a state property
   void set(int i,int val) { _state[i] = val;}       // sets a state property 
   long long hash() { 
      long long ttl = 0;
      for(auto v : _state)
         ttl = (ttl << 8) + (ttl >> (64 - 8)) + v;
      _hash = ttl;
      return _hash;
   }
   long long getHash() const noexcept { return _hash;}
   bool operator==(const MDDState& s) const {    // equality test likely O(1) when different. 
      if (_hash == s._hash) {
         bool eq = true;
         for(std::vector<int>::size_type i=0;eq && i < _state.size();i++) 
            eq = _state[i] == s._state[i];         
         return eq;
      } else return false;
   }
   friend std::ostream& operator<<(std::ostream& os,const MDDState& s) {
      os << '[';
      for(auto v : s._state)
         os << v << " ";
      return os << ']';
   }
   friend bool operator==(const MDDState::Ptr& s1,const MDDState::Ptr& s2) { return s1->operator==(*s2);}
};

class MDDSpec {
   std::vector<MDDProperty::Ptr> _attrs;
   size_t _lsz;
public:
   MDDSpec();
   void layout();
   int addState(int init,int max=0x7fffffff);
   void addStates(int from, int to, std::function<int(int)> clo);
   void addStates(std::initializer_list<int> inputs);
   auto size() const { return baseState->size();} // should be _attrs
   
   void addArc(std::function<bool(const MDDState::Ptr&, var<int>::Ptr, int)> a);
   void addTransition(int,std::function<int(const MDDState::Ptr&, var<int>::Ptr, int)>);
   void addRelaxation(int,std::function<int(MDDState::Ptr, MDDState::Ptr)>);
   void addSimilarity(int,std::function<double(MDDState::Ptr, MDDState::Ptr)>);
   void addTransitions(std::map<int,std::function<int(const MDDState::Ptr&, var<int>::Ptr, int)>>& map);
   
   MDDState::Ptr createState(Storage::Ptr& mem,const MDDState::Ptr& state, var<int>::Ptr var, int v);
   MDDState::Ptr rootState(Storage::Ptr& mem);
   
   auto getArcs()         { return arcLambda;}
   auto getRelaxations()  { return relaxationLambdas;}
   auto getSimilarities() { return similarityLambdas;}
    
   void append(const Factory::Veci& x);
   std::vector<var<int>::Ptr>& getVars(){ return x; }
   friend std::ostream& operator<<(std::ostream& os,const MDDSpec& s) {
      os << "Spec(";
      for(auto a : s._attrs)
         os << a << ' ';
      os << ')';
      return os;
   }
private:
   MDDState::Ptr baseState;
   std::vector<var<int>::Ptr> x;
   std::function<bool(const MDDState::Ptr&, var<int>::Ptr, int)> arcLambda;
   std::vector<std::function<int(const MDDState::Ptr&, var<int>::Ptr, int)>> transistionLambdas;
   std::vector<std::function<int(MDDState::Ptr, MDDState::Ptr)>> relaxationLambdas;
   std::vector<std::function<double(MDDState::Ptr, MDDState::Ptr)>> similarityLambdas;
};


typedef std::function<int(const MDDState::Ptr&, var<int>::Ptr, int)> lambdaTrans;
typedef std::map<int,lambdaTrans> lambdaMap;
std::pair<int,int> domRange(const Factory::Veci& vars);

namespace Factory {

   inline void amongMDD(MDDSpec& mdd, const Factory::Veci& x, int lb, int ub, std::set<int> rawValues)
   {
      //int sz = (int) mdd.size();
      mdd.append(x);
      ValueSet values(rawValues);
      const int minC = mdd.addState(0);
      const int maxC = mdd.addState(0);
      const int rem  = mdd.addState((int)x.size());
        
      auto a = [=] (MDDState::Ptr p, var<int>::Ptr var, int val) -> bool {
                  return (p->at(minC) + values.member(val) <= ub) &&
                     ((p->at(maxC) + values.member(val) +  p->at(rem) - 1) >= lb);
               };
        
      mdd.addArc(a);
        
      mdd.addTransition(minC,[=] (const auto& p,auto x, int val) -> int { return p->at(minC) + values.member(val);});
      mdd.addTransition(maxC,[=] (const auto& p,auto x, int val) -> int { return p->at(maxC) + values.member(val);});
      mdd.addTransition(rem,[=] (const auto& p,auto x,int val) -> int { return p->at(rem) - 1;});
        
      mdd.addRelaxation(minC,[=](auto l,auto r) -> int { return std::min(l->at(minC), r->at(minC));});
      mdd.addRelaxation(maxC,[=](auto l,auto r) -> int { return std::max(l->at(maxC), r->at(maxC));});
      mdd.addRelaxation(rem ,[=](auto l,auto r) -> int { return l->at(rem);});

      mdd.addSimilarity(minC,[=](auto l,auto r) -> double { return abs(l->at(minC) - r->at(minC)); });
      mdd.addSimilarity(maxC,[=](auto l,auto r) -> double { return abs(l->at(maxC) - r->at(maxC)); });
      mdd.addSimilarity(rem ,[=] (auto l,auto r) -> double { return 0; });
   }

   inline lambdaMap toDict(int min, int max, std::function<std::function<int(const MDDState::Ptr&, var<int>::Ptr, int)>(int)> clo)
   {
      lambdaMap r;
      for(int i = min; i <= max; i++)
         r[i] = clo(i);
      return r;
   }


   inline void allDiffMDD(MDDSpec& mdd, const Factory::Veci& vars)
   {
      int os = (int) mdd.size();
      mdd.append(vars);
      auto udom = domRange(vars);
      int minDom = udom.first, maxDom = udom.second;
    
      mdd.addStates(minDom,maxDom,[=] (int i) -> int   { return 1; });      
      mdd.addArc([=] (auto p,auto var,int val) -> bool { return p->at(os+val-minDom);});
      
      for(int i = minDom; i <= maxDom; i++){
         int idx = os+i-minDom;
         mdd.addTransition(idx,[idx,i](const auto& p,auto var, int val) -> int { return  p->at(idx) && i != val;});
         mdd.addRelaxation(idx,[idx](auto l,auto r) -> int { return l->at(idx) || r->at(idx);});
         mdd.addSimilarity(idx,[idx](auto l,auto r) -> double { return abs(l->at(idx)- r->at(idx));});
      }
   }

   inline void  seqMDD(MDDSpec& spec,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues)
   {
      
      int minFIdx = 0,minLIdx = len-1;
      int maxFIdx = len,maxLIdx = len*2-1;
      int os = (int) spec.size();
      spec.append(vars);
      ValueSet values(rawValues);
   
      spec.addStates(minFIdx,minLIdx,[=] (int i) -> int { return (i - minLIdx); });
      spec.addStates(maxFIdx,maxLIdx,[=] (int i) -> int { return (i - maxLIdx); });
   
      minLIdx += os; minFIdx+=os;
      maxLIdx += os; maxFIdx+=os;
   
      spec.addArc([=] (auto p,auto x,int v) -> bool {
                     bool inS = values.member(v);
                     int minv = p->at(maxLIdx) - p->at(minFIdx) + inS;
                     return (p->at(os) < 0 && minv >= lb && p->at(minLIdx) + inS <= ub)
                        ||  (minv >= lb && p->at(minLIdx) - p->at(maxFIdx) + inS <= ub);
                  });
      
      lambdaMap d = toDict(minFIdx,maxLIdx,[=] (int i) -> lambdaTrans {
         if (i == maxLIdx || i == minLIdx)
            return [=] (const auto& p,auto x, int v) -> int {return p->at(i)+values.member(v);};
         return [i] (const auto& p,auto x, int v) -> int {return p->at(i+1);};
      });
      spec.addTransitions(d);
   
      for(int i = minFIdx; i <= minLIdx; i++)
         spec.addRelaxation(i,[i](auto l,auto r)->int{return std::min(l->at(i),r->at(i));});
   
      for(int i = maxFIdx; i <= maxLIdx; i++)
         spec.addRelaxation(i,[i](auto l,auto r)->int{return std::max(l->at(i),r->at(i));});
   
      for(int i = minFIdx; i <= maxLIdx; i++)
         spec.addSimilarity(i,[i](auto l,auto r)->double{return abs(l->at(i)- r->at(i));});
   }

   inline void  gccMDD(MDDSpec& spec,const Factory::Veci& vars,const std::map<int,int>& ub)
   {
      int os = (int) spec.size();
      spec.append(vars);
      auto udom = domRange(vars);
      int domSize = udom.second - udom.first + 1;
      int minFDom = os, minLDom = os + domSize-1;
      int maxFDom = os + domSize,maxLDom = os + domSize*2-1;
      int minDom = udom.first;
      ValueMap<int> values(udom.first, udom.second,0,ub);
   
      spec.addStates(minFDom, maxLDom, [] (int i) -> int { return 0; });
   
      spec.addArc([=](auto p,auto x,int v)->bool{return p->at(os+v-minDom) < values[v];});

      lambdaMap d = toDict(minFDom,maxLDom,[=] (int i) -> lambdaTrans {
              if (i <= minLDom)
                 return [=] (const auto& p,auto x, int v) -> int {return p->at(i) + ((v - minDom) == i);};
              return [=] (const auto& p,auto x, int v) -> int {return p->at(i) + ((v - minDom) == (i - domSize));};
           });
      spec.addTransitions(d);
   
      for(ORInt i = minFDom; i <= minLDom; i++){
         spec.addRelaxation(i,[i](auto l,auto r)->int{return std::min(l->at(i),r->at(i));});
         spec.addSimilarity(i,[i](auto l,auto r)->double{return std::min(l->at(i),r->at(i));});
      }
   
      for(ORInt i = maxFDom; i <= maxLDom; i++){
         spec.addRelaxation(i,[i](auto l,auto r)->int{return std::max(l->at(i),r->at(i));});
         spec.addSimilarity(i,[i](auto l,auto r)->double{return std::max(l->at(i),r->at(i));});
      }
   }
}
#endif /* mddstate_hpp */
