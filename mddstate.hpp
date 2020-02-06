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

class MDDState;
typedef std::function<int(const MDDState&, var<int>::Ptr, int)> lambdaTrans;
typedef std::map<int,lambdaTrans> lambdaMap;

class MDDStateSpec;

class MDDConstraintDescriptor {
   const Factory::Veci&   _vars;
   ValueSet               _vset;
   const char*            _name;
   std::vector<int> _properties;
public :
   MDDConstraintDescriptor(const Factory::Veci& vars, const char* name);
   MDDConstraintDescriptor(const MDDConstraintDescriptor&);
   void addProperty(int p) {_properties.push_back(p);}
   bool member(var<int>::Ptr x) const { return _vset.member(x->getId());}
   const Factory::Veci& vars() const { return _vars;}
   std::vector<int>& properties() { return _properties;}
   auto begin() { return _properties.begin();}
   auto end()   { return _properties.end();}
   void print (std::ostream& os) const
   {
      os << _name << "(" << _vars << ")" << std::endl;
   }
   MDDConstraintDescriptor operator=(const MDDConstraintDescriptor& d)
   {
      return MDDConstraintDescriptor(d);
   }
};

class MDDProperty {
protected:
   short _id;
   unsigned short _ofs;
   virtual size_t storageSize() const = 0;  // given in _bits_
   virtual size_t setOffset(size_t bitOffset) = 0;
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
   size_t storageSize() const override     { return 32;}
   size_t setOffset(size_t bitOffset) override {
      size_t boW = bitOffset & 0x1F;
      if (boW != 0) 
         bitOffset = (bitOffset | 0x1F) + 1;
      _ofs = bitOffset >> 3;
      return bitOffset + storageSize();
   }
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
   size_t storageSize() const override     { return 8;}
   size_t setOffset(size_t bitOffset) override {
      size_t boW = bitOffset & 0x7;
      if (boW != 0) 
         bitOffset = (bitOffset | 0x7) + 1;
      _ofs = bitOffset >> 3;
      return bitOffset + storageSize();
   }
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

class MDDPBit : public MDDProperty {   // the offset is a byte offset.
   unsigned char _init;        // in {0,1}
   const unsigned char _bmask; // mask with 1 at correct bit position
   size_t storageSize() const override     { return 1;}
   size_t setOffset(size_t bitOffset) override {
      _ofs = bitOffset >> 3;
      return bitOffset + storageSize();
   }
public:
   MDDPBit(short id,unsigned short ofs,unsigned char init)
      : MDDProperty(id,ofs),_init(init),_bmask(0x1 << (id & 0x7)) {}
   void init(char* buf) const override     { buf[_ofs] = _init ? (buf[_ofs] | _bmask) : (buf[_ofs] & ~_bmask);}
   int get(char* buf) const override       { return ((unsigned char)buf[_ofs] & _bmask) == _bmask;}
   void setInt(char* buf,int v) override   { if (v) buf[_ofs] |= _bmask; else buf[_ofs] &= ~_bmask;}
   void setByte(char* buf,unsigned char v) override { buf[_ofs] = v ? (buf[_ofs] | _bmask) : (buf[_ofs] & ~_bmask);}
   void print(std::ostream& os) const override  {
      os << "PBit(" << _id << ',' << _ofs << ',' << (int)_init << ',' << (int)_bmask << ')';
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
   virtual int addState(MDDConstraintDescriptor&d, int init,int max=0x7fffffff);
   std::vector<int> addStates(MDDConstraintDescriptor&d,int from, int to, int max,std::function<int(int)> clo);
   std::vector<int> addStates(MDDConstraintDescriptor&d,int max,std::initializer_list<int> inputs);
   friend class MDDState;
   friend std::ostream& operator<<(std::ostream& os,const MDDState& s);
};

inline std::ostream& operator<<(std::ostream& os,MDDProperty::Ptr p)
{
   p->print(os);return os;
}

inline std::ostream& operator<<(std::ostream& os,MDDConstraintDescriptor& p)
{
   p.print(os);return os;
}

class MDDState {  // An actual state of an MDDNode.
   MDDStateSpec*     _spec;
   char*              _mem;
   int               _hash;  // a hash value of the state to speed up equality testing.
   bool           _relaxed;
public:
   MDDState() : _spec(nullptr),_mem(nullptr),_hash(0),_relaxed(false) {}
   MDDState(MDDStateSpec* s,char* b,bool relax=false) 
      : _spec(s),_mem(b),_hash(0),_relaxed(false) {}
   MDDState(const MDDState& s) 
      : _spec(s._spec),_mem(s._mem),_hash(s._hash),_relaxed(s._relaxed) {}
   MDDState& operator=(const MDDState& s) { 
      assert(_spec == s._spec || _spec == nullptr);
      assert(_mem != nullptr);
      _spec = s._spec;
      memcpy(_mem,s._mem,_spec->layoutSize());
      _hash = s._hash;
      _relaxed = s._relaxed;
      return *this;
   }
   MDDState& operator=(MDDState&& s) { 
      assert(_spec == s._spec || _spec == nullptr);
      _spec = std::move(s._spec);
      _mem  = std::move(s._mem);      
      _hash = std::move(s._hash);
      _relaxed = std::move(s._relaxed);
      return *this;
   }
   void init(int i) const      { _spec->_attrs[i]->init(_mem);}
   int at(int i) const         { return _spec->_attrs[i]->get(_mem);}
   int operator[](int i) const { return _spec->_attrs[i]->get(_mem);}  // to _read_ a state property
   void set(int i,int val)     { _spec->_attrs[i]->setInt(_mem,val);}        // to set a state property
   bool isRelaxed() const      { return _relaxed;}      
   void relax(bool r = true)   { _relaxed = r;}
   int hash() {
      const int nbw = (int)_spec->layoutSize() / 4;
      int nlb = _spec->layoutSize() & 0x3;
      char* sfx = _mem + (nbw << 2);
      int* b = reinterpret_cast<int*>(_mem);
      int ttl = 0;
      for(size_t s = 0;s <nbw;s++)
         ttl = (ttl << 8) + (ttl >> (32-8)) + b[s];
      while(nlb-- > 0)
         ttl = (ttl << 8) + (ttl >> (32-8)) + *sfx++;
      _hash = ttl;
      return _hash;
   }
   int getHash() const noexcept { return _hash;}
   bool operator==(const MDDState& s) const {    // equality test likely O(1) when different.
      if (_hash == s._hash)
         return memcmp(_mem,s._mem,_spec->layoutSize())==0;
      else return false;
   }
   bool operator!=(const MDDState& s) const {
      if (_hash == s._hash)
         return memcmp(_mem,s._mem,_spec->layoutSize())!=0;
      else return true;
   }
   friend std::ostream& operator<<(std::ostream& os,const MDDState& s) {
      os << (s._relaxed ? 'T' : 'F') << '[';
      if(s._spec != nullptr)
         for(auto atr : s._spec->_attrs)
            os << atr->get(s._mem) << " ";
      return os << ']';
   }
};

class MDDSpec: public MDDStateSpec {
public:
   MDDSpec();
   MDDConstraintDescriptor& makeConstraintDescriptor(const Factory::Veci&, const char*);
   int addState(MDDConstraintDescriptor& d, int init,int max=0x7fffffff) override;
   int addState(MDDConstraintDescriptor& d,int init,size_t max) {return addState(d,init,(int)max);}
   void addArc(const MDDConstraintDescriptor& d,std::function<bool(const MDDState&, var<int>::Ptr, int)> a);
   void addTransition(int,std::function<int(const MDDState&, var<int>::Ptr, int)>);
   void addRelaxation(int,std::function<int(const MDDState&,const MDDState&)>);
   void addSimilarity(int,std::function<double(const MDDState&,const MDDState&)>);
   void addTransitions(lambdaMap& map);
   bool exist(const MDDState& a,var<int>::Ptr x,int v);
   double similarity(const MDDState& a,const MDDState& b);
   std::pair<MDDState,bool> createState(Storage::Ptr& mem,const MDDState& state,var<int>::Ptr var, int v);
   MDDState relaxation(Storage::Ptr& mem,const MDDState& a,const MDDState& b);
   MDDState rootState(Storage::Ptr& mem);

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
   void init();
   std::vector<MDDConstraintDescriptor> constraints;
   std::vector<var<int>::Ptr> x;
   std::function<bool(const MDDState&, var<int>::Ptr, int)> arcLambda;
   std::vector<std::function<int(const MDDState&, var<int>::Ptr, int)>> transistionLambdas;
   std::vector<std::function<int(const MDDState&,const MDDState&)>> relaxationLambdas;
   std::vector<std::function<double(const MDDState&,const MDDState&)>> similarityLambdas;
};


std::pair<int,int> domRange(const Factory::Veci& vars);

namespace Factory {
   inline lambdaMap toDict(int min, int max,std::vector<int>& p,std::function<lambdaTrans(int,int)> clo)
   {
      lambdaMap r;
      for(int i = min; i <= max; i++)
         r[p[i]] = clo(i,p[i]);
      return r;
   }
   void amongMDD(MDDSpec& mdd, const Factory::Veci& x, int lb, int ub, std::set<int> rawValues);
   void allDiffMDD(MDDSpec& mdd, const Factory::Veci& vars);
   void seqMDD(MDDSpec& spec,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues);
   void gccMDD(MDDSpec& spec,const Factory::Veci& vars,const std::map<int,int>& ub);
}
#endif /* mddstate_hpp */

