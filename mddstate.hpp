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
#include "utilities.hpp"
#include <set>
#include <cstring>
#include <map>

class MDDStateSpec;

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
   virtual void init(char* buf) const = 0;
   virtual int get(char* buf) const = 0;
   virtual void setInt(char* buf,int v)            {}
   virtual void setByte(char* buf,unsigned char v) {}
   virtual void print(std::ostream& os) const  {}
   friend class MDDStateSpec;
};

namespace Factory {
   inline MDDProperty::Ptr makeProperty(short id,unsigned short ofs,int init,int max=0x7fffffff);
}

class MDDPInt :public MDDProperty {
   int _init;
   int _max;
   size_t storageSize() const override     { return sizeof(int);}
public:
   typedef handle_ptr<MDDPInt> Ptr;
   MDDPInt(short id,unsigned short ofs,int init,int max=0x7fffffff)
      : MDDProperty(id,ofs),_init(init),_max(max) {}
   void init(char* buf) const override     { *reinterpret_cast<int*>(buf+_ofs) = _init;}
   int get(char* buf) const override       { return *reinterpret_cast<int*>(buf+_ofs);}
   void setInt(char* buf,int v) override   { *reinterpret_cast<int*>(buf+_ofs) = v;}  
   void print(std::ostream& os) const override  {
      os << "PInt(" << _id << ',' << _ofs << ',' << _init << ',' << _max << ')';
   }
   friend class MDDStateSpec;
};

class MDDPByte :public MDDProperty {
   unsigned char _init;
   unsigned char  _max;
   size_t storageSize() const override     { return sizeof(unsigned char);}
public:
   typedef handle_ptr<MDDPByte> Ptr;
   MDDPByte(short id,unsigned short ofs,unsigned char init,unsigned char max=255)
      : MDDProperty(id,ofs),_init(init),_max(max) {}
   void init(char* buf) const override     { buf[_ofs] = _init;}
   int get(char* buf) const override       { return (unsigned char)buf[_ofs];}
   void setInt(char* buf,int v) override   { buf[_ofs] = v;}  
   void setByte(char* buf,unsigned char v) override { buf[_ofs] = v;}  
   void print(std::ostream& os) const override  {
      os << "PByte(" << _id << ',' << _ofs << ',' << (int)_init << ',' << (int)_max << ')';
   }
   friend class MDDStateSpec;
};

class MDDStateSpec {
protected:
   std::vector<MDDProperty::Ptr> _attrs;
   size_t _lsz;
public:
   MDDStateSpec() {}   
   auto layoutSize() const { return _lsz;}
   void layout();
   auto size() const { return _attrs.size();}
   virtual int addState(int init,int max=0x7fffffff);
   void addStates(int from, int to, std::function<int(int)> clo);
   void addStates(std::initializer_list<int> inputs);
   friend class MDDState;
};

inline std::ostream& operator<<(std::ostream& os,MDDProperty::Ptr p)
{
   p->print(os);return os;
}

class MDDState {  // An actual state of an MDDNode.
   MDDStateSpec*     _spec;
   char*              _mem;
   int               _hash;  // a hash value of the state to speed up equality testing. 
public:
   typedef handle_ptr<MDDState> Ptr;
   MDDState() : _spec(nullptr),_mem(nullptr),_hash(0) {}
   MDDState(MDDStateSpec* s,char* b) : _spec(s),_mem(b),_hash(0) {}
   MDDState(const MDDState& s) : _spec(s._spec),_mem(s._mem),_hash(s._hash) {}
   void init(int i) const      { _spec->_attrs[i]->init(_mem);}
   int at(int i) const         { return _spec->_attrs[i]->get(_mem);}
   int operator[](int i) const { return _spec->_attrs[i]->get(_mem);}  // to _read_ a state property
   void set(int i,int val)     { _spec->_attrs[i]->setInt(_mem,val);}        // to set a state property 
   int hash() {
      const int nbw = (int)_spec->layoutSize() / 4;
      int* b = reinterpret_cast<int*>(_mem);
      int ttl = 0;
      for(size_t s = 0;s <nbw;s++)
         ttl = (ttl << 8) + (ttl >> (32-8)) + b[s];
      _hash = ttl;
      return _hash;
   }
   int getHash() const noexcept { return _hash;}
   bool operator==(const MDDState& s) const {    // equality test likely O(1) when different. 
      if (_hash == s._hash) 
         return memcmp(_mem,s._mem,_spec->layoutSize())==0;
      else return false;
   }
   friend std::ostream& operator<<(std::ostream& os,const MDDState& s) {
      os << '[';
      for(auto atr : s._spec->_attrs)
         os << atr->get(s._mem) << " ";
      return os << ']';
   }
   friend bool operator==(const MDDState::Ptr& s1,const MDDState::Ptr& s2) { return s1->operator==(*s2);}
};



class MDDSpec: public MDDStateSpec {
public:
   MDDSpec();
   int addState(int init,int max=0x7fffffff) override;
   int addState(int init,size_t max) {return addState(init,(int)max);}
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
   inline lambdaMap toDict(int min, int max,std::function<lambdaTrans(int)> clo)
   {
      lambdaMap r;
      for(int i = min; i <= max; i++)
         r[i] = clo(i);
      return r;
   }
   void amongMDD(MDDSpec& mdd, const Factory::Veci& x, int lb, int ub, std::set<int> rawValues);
   void allDiffMDD(MDDSpec& mdd, const Factory::Veci& vars);
   void seqMDD(MDDSpec& spec,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues);
   void gccMDD(MDDSpec& spec,const Factory::Veci& vars,const std::map<int,int>& ub);
}
#endif /* mddstate_hpp */
