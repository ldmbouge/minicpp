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


/*
  enum Prop { vec_int , state_int };


  union RType {
  void* vp;
  int i;
  };

  class MDDPropetry {
  public:
  MDDPropetry() {}
  MDDPropetry(int v){ this->value.i = v; type = state_int;}
  MDDPropetry(std::vector<int> vp){
  std::vector<int>* v = new std::vector<int>(vp);
  this->value.vp = v;
  type = vec_int;
  }
  MDDPropetry(const MDDPropetry& p){
  if(p.type == vec_int){
  auto vp = static_cast<std::vector<int>*>(p.value.vp);
  std::vector<int>* v = new std::vector<int>(*vp);
  this->value.vp = v;
  this->type = vec_int;
  }
  if(p.type == state_int){
  this->value.i = p.value.i;
  this->type = state_int;
  }
  }

  RType getValue() { return value; }
  void setValue(int v) { this->value.i = v; }
  private:
  RType value;
  Prop type;
  };
*/

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
public:
   MDDSpec():arcLambda(nullptr){ this->baseState = new MDDState();};
   void addState(int s) { baseState->addState(s); };
   void addStates(int from, int to, std::function<int(int)> clo);
   void addStates(std::initializer_list<int> inputs);
   void addArc(std::function<bool(const MDDState::Ptr&, var<int>::Ptr, int)> a);
   void addTransistion(std::function<int(const MDDState::Ptr&, var<int>::Ptr, int)> t);
   void addRelaxation(std::function<int(MDDState::Ptr, MDDState::Ptr)>);
   void addSimilarity(std::function<double(MDDState::Ptr, MDDState::Ptr)>);
   void addTransistions(int from, int to,std::function<int(const MDDState::Ptr&, var<int>::Ptr, int,int)> t);
   void addRelaxations(int from, int to, std::function<int(MDDState::Ptr, MDDState::Ptr, int)>);
   void addSimilarities(int from, int to, std::function<double(MDDState::Ptr, MDDState::Ptr,int)>);
   
   MDDState::Ptr createState(Storage::Ptr& mem,const MDDState::Ptr& state, var<int>::Ptr var, int v);
   
   auto getArcs()         { return arcLambda;}
   auto getRelaxations()  { return relaxationLambdas;}
   auto getSimilarities() { return similarityLambdas;}
    
   void append(const Factory::Veci& x);
   std::vector<var<int>::Ptr> getVars(){ return x; }
   MDDState::Ptr baseState;
private:
   std::vector<var<int>::Ptr> x;
   std::function<bool(const MDDState::Ptr&, var<int>::Ptr, int)> arcLambda;
   std::vector<std::function<int(const MDDState::Ptr&, var<int>::Ptr, int)>> transistionLambdas;
   std::vector<std::function<int(MDDState::Ptr, MDDState::Ptr)>> relaxationLambdas;
   std::vector<std::function<double(MDDState::Ptr, MDDState::Ptr)>> similarityLambdas;
};

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

std::pair<int,int> domRange(const Factory::Veci& vars);

namespace Factory {

   inline void amongMDD(MDDSpec& mdd, const Factory::Veci& x, int lb, int ub, std::set<int> rawValues){
      int stateSize = (int) mdd.baseState->size();
      mdd.append(x);
      ValueSet values(rawValues);

      int minC = 0 + stateSize, maxC = 1 + stateSize, rem = 2 + stateSize; //state idx
      
      mdd.addStates({0,0,(int) x.size()});
        
      auto a = [=] (MDDState::Ptr p, var<int>::Ptr var, int val) -> bool {
                  return (p->at(minC) + values.member(val) <= ub) && ((p->at(maxC) + values.member(val) +  p->at(rem) - 1) >= lb);
               };
        
      mdd.addArc(a);
        
      mdd.addTransistion([=] (const MDDState::Ptr& p, var<int>::Ptr var, int val) -> int { return p->at(minC) + values.member(val);});
      mdd.addTransistion([=] (const MDDState::Ptr& p, var<int>::Ptr var, int val) -> int { return p->at(maxC) + values.member(val);});
      mdd.addTransistion([=] (const MDDState::Ptr& p, var<int>::Ptr var, int val) -> int { return p->at(rem) - 1;});
        
      mdd.addRelaxation([=] (MDDState::Ptr l, MDDState::Ptr r) -> int { return std::min(l->at(minC), r->at(minC));});
      mdd.addRelaxation([=] (MDDState::Ptr l, MDDState::Ptr r) -> int { return std::max(l->at(maxC), r->at(maxC));});
      mdd.addRelaxation([=] (MDDState::Ptr l, MDDState::Ptr r) -> int { return l->at(rem);});

      mdd.addSimilarity([=] (MDDState::Ptr l, MDDState::Ptr r) -> double { return abs(l->at(minC) - r->at(minC)); });
      mdd.addSimilarity([=] (MDDState::Ptr l, MDDState::Ptr r) -> double { return abs(l->at(maxC) - r->at(maxC)); });
      mdd.addSimilarity([=] (MDDState::Ptr l, MDDState::Ptr r) -> double { return 0; });
   }

 inline void allDiffMDD(MDDSpec& mdd, const Factory::Veci& vars){
    int offset = (int) mdd.baseState->size();
    mdd.append(vars);
    auto udom = domRange(vars);
    int minDom = udom.first, maxDom = udom.second;
    
    mdd.addStates(minDom,maxDom,[=] (int i) -> int { return 1; });
      
    mdd.addArc([=] (MDDState::Ptr p, var<int>::Ptr var, int val) -> bool {
       return p->at(offset+val-minDom);
    });
    
    for(int i = minDom; i <= maxDom; i++){
       mdd.addTransistion([=] (const MDDState::Ptr& p, var<int>::Ptr var, int val) -> int {
          return  p->at(offset+i-minDom) && i != val;
       });
       mdd.addRelaxation([=] (MDDState::Ptr l, MDDState::Ptr r) -> int { return l->at(offset+i-minDom) || r->at(offset+i-minDom);});
       mdd.addSimilarity([=] (MDDState::Ptr l, MDDState::Ptr r) -> double { return abs(l->at(offset+i-minDom)- r->at(offset+i-minDom)); });
    }
 }

inline void  seqMDD(MDDSpec& spec,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues){
   int minFIdx = 0,minLIdx = len-1;
   int maxFIdx = len,maxLIdx = len*2-1;
   int offset = (int) spec.baseState->size();
   spec.append(vars);
   ValueSet values(rawValues);
   
   spec.addStates(minFIdx,minLIdx,[=] (int i) -> int { return (i - minLIdx); });
   spec.addStates(maxFIdx,maxLIdx,[=] (int i) -> int { return (i - maxLIdx); });
   
   minLIdx += offset; minFIdx+=offset;
   maxLIdx += offset; maxFIdx+=offset;
   
   spec.addArc([=] (MDDState::Ptr p, var<int>::Ptr var, int val) -> bool {
      bool inS = values.member(val);
      int minv = p->at(maxLIdx) - p->at(minFIdx) + inS;
         return (p->at(offset) < 0 && minv >= lb && p->at(minLIdx) + inS <= ub) ||  (minv >= lb && p->at(minLIdx) - p->at(maxFIdx) + inS <= ub);
      });
   
   for(int i = minFIdx; i < minLIdx; i++)
        spec.addTransistion([=] (const MDDState::Ptr& p, var<int>::Ptr var, int val) -> int {
            return  p->at(i+1);
         });
   
   spec.addTransistion([=] (const MDDState::Ptr& p, var<int>::Ptr var, int val) -> int {
      return  p->at(minLIdx)+values.member(val);
   });
   
   for(int i = maxFIdx; i < maxLIdx; i++)
      spec.addTransistion([=] (const MDDState::Ptr& p, var<int>::Ptr var, int val) -> int {
          return  p->at(i+1);
       });
   
   spec.addTransistion([=] (const MDDState::Ptr& p, var<int>::Ptr var, int val) -> int {
      return  p->at(maxLIdx)+values.member(val);
   });
   
   for(int i = minFIdx; i <= minLIdx; i++)
      spec.addRelaxation([=] (MDDState::Ptr l, MDDState::Ptr r) -> int { return std::min(l->at(i),r->at(i));});
   
    for(int i = maxFIdx; i <= maxLIdx; i++)
       spec.addRelaxation([=] (MDDState::Ptr l, MDDState::Ptr r) -> int { return std::max(l->at(i),r->at(i));});
   
   for(int i = minFIdx; i <= maxLIdx; i++)
      spec.addSimilarity([=] (MDDState::Ptr l, MDDState::Ptr r) -> double { return abs(l->at(i)- r->at(i)); });
}
}



#endif /* mddstate_hpp */
