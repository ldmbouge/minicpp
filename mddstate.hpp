/*
 * mini-cp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License  v3
 * as published by the Free Software Foundation.
 *
 * mini-cp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY.
 * See the GNU Lesser General Public License  for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with mini-cp. If not, see http://www.gnu.org/licenses/lgpl-3.0.en.html
 *
 * Copyright (c)  2018. by Laurent Michel, Pierre Schaus, Pascal Van Hentenryck
 */

#ifndef mddstate_hpp
#define mddstate_hpp

#include "handle.hpp"
#include "intvar.hpp"
#include "utilities.hpp"
#include <set>
#include <cstring>
#include <map>
#include <bitset>

class MDDState;
typedef std::function<bool(const MDDState&,const MDDState&,var<int>::Ptr,int,bool)> ArcFun;
typedef std::function<void(MDDState&,const MDDState&, var<int>::Ptr, int)> lambdaTrans;
typedef std::function<void(MDDState&,const MDDState&,const MDDState&)> lambdaRelax;
typedef std::function<double(const MDDState&,const MDDState&)> lambdaSim;
typedef std::map<int,lambdaTrans> lambdaMap;

class MDDStateSpec;

class MDDConstraintDescriptor {
   const Factory::Veci&   _vars;
   ValueSet               _vset;
   const char*            _name;
   std::vector<int> _properties;
   std::vector<int> _tid; // transition ids
   std::vector<int> _rid; // relaxation ids
   std::vector<int> _sid; // similarity ids
   std::vector<int> _utid; // up transition ids
public:
   MDDConstraintDescriptor(const Factory::Veci& vars, const char* name);
   MDDConstraintDescriptor(const MDDConstraintDescriptor&);
   void addProperty(int p) {_properties.push_back(p);}
   bool ownsProperty(int p) const {
      auto at = std::find(_properties.begin(),_properties.end(),p);
      return at != _properties.end();
   }
   const std::vector<int>& transitions() const  { return _tid;}
   const std::vector<int>& relaxations() const  { return _rid;}
   const std::vector<int>& similarities() const { return _sid;}
   const std::vector<int>& uptrans() const      { return _utid;}
   void registerTransition(int t) { _tid.emplace_back(t);}
   void registerRelaxation(int t) { _rid.emplace_back(t);}
   void registerSimilarity(int t) { _sid.emplace_back(t);}
   void registerUp(int t)         { _utid.emplace_back(t);}
   bool inScope(var<int>::Ptr x) const { return _vset.member(x->getId());}
   const Factory::Veci& vars() const { return _vars;}
   std::vector<int>& properties() { return _properties;}
   auto begin() { return _properties.begin();}
   auto end()   { return _properties.end();}
   void print (std::ostream& os) const { os << _name << "(" << _vars << ")" << std::endl;}
};

class MDDBSValue {
   unsigned long long* const _buf;
   const unsigned short      _nbw;
   const int                _bLen;
public:
   MDDBSValue(char* buf,short nbw,int nbb)
      : _buf(reinterpret_cast<unsigned long long*>(buf)),_nbw(nbw),_bLen(nbb) {}
   short nbWords() const { return _nbw;}
   int  bitLen() const { return _bLen;}
   MDDBSValue& operator=(const MDDBSValue& v) {
      for(int i=0;i <_nbw;i++)
         _buf[i] = v._buf[i];
      assert(_bLen == v._bLen);
      assert(_nbw == v._nbw);
      return *this;
   }
   bool getBit(int ofs) const {
      const int wIdx = ofs / 64;
      const int bOfs = ofs % 64;
      const unsigned long long bmask = 0x1ull << bOfs;
      return (_buf[wIdx] & bmask) == bmask;
   }
   void clear(int ofs) {
      const int wIdx = ofs / 64;
      const int bOfs = ofs % 64;
      const unsigned long long bmask = 0x1ull << bOfs;
      _buf[wIdx] &= ~bmask;
   }
   void set(int ofs) {
      const int wIdx = ofs / 64;
      const int bOfs = ofs % 64;
      const unsigned long long bmask = 0x1ull << bOfs;
      _buf[wIdx] |= bmask;      
   }
   unsigned long long cardinality() const {
      unsigned long long nbb = 0;
      for(unsigned i = 0;i < _nbw;i++) 
         nbb += __builtin_popcountl(_buf[i]);      
      return nbb;
   }
   MDDBSValue& setBinOR(const MDDBSValue& a,const MDDBSValue& b) {
      for(int i=0;i < _nbw;i++)
         _buf[i] = a._buf[i] | b._buf[i];
      return *this;
   }
   MDDBSValue& setBinAND(const MDDBSValue& a,const MDDBSValue& b) {
      for(int i=0;i < _nbw;i++)
         _buf[i] = a._buf[i] & b._buf[i];
      return *this;
   }
   MDDBSValue& setBinXOR(const MDDBSValue& a,const MDDBSValue& b) {
      for(int i=0;i < _nbw;i++)
         _buf[i] = a._buf[i] ^ b._buf[i];
      return *this;
   }
   MDDBSValue& NOT() {
      for(int i=0;i <_nbw;i++)
         _buf[i] = ~_buf[i];
      return *this;
   }
   friend bool operator==(const MDDBSValue& a,const MDDBSValue& b) {
      bool eq = a._nbw == b._nbw && a._bLen == b._bLen;
      for(unsigned i = 0 ;eq && i < a._nbw;i++)
         eq = a._buf[i] == b._buf[i];
      return eq;
   }
   friend std::ostream& operator<<(std::ostream& os,const MDDBSValue& v) {
      os << '[';
      unsigned nbb = v._bLen;
      int val = 0;
      for(int i=0;i < v._nbw;i++) {
         unsigned long long w = v._buf[i];
         const unsigned biw = nbb >= 64 ? 64 : nbb;
         nbb -= 64;
         unsigned long long mask = 1ull;
         unsigned bOfs = 0;
         while (bOfs != biw) {
            bool hasValue = ((w & mask)==mask);
            if (hasValue) os << val << ',';
            val++;
            bOfs++;
            mask <<=1;
         }
      }
      os << ']';
      return os;
   }
};

class MDDProperty {
public:
   enum Direction {Down,Up};
protected:
   short _id;
   unsigned short _ofs;
   enum Direction   _d;
   virtual size_t storageSize() const = 0;  // given in _bits_
   virtual size_t setOffset(size_t bitOffset) = 0;
public:
   typedef handle_ptr<MDDProperty> Ptr;
   MDDProperty(const MDDProperty& p) : _id(p._id),_ofs(p._ofs),_d(p._d) {}
   MDDProperty(MDDProperty&& p) : _id(p._id),_ofs(p._ofs),_d(p._d) {}
   MDDProperty(short id,unsigned short ofs,enum Direction dir = Down) : _id(id),_ofs(ofs),_d(dir) {}
   MDDProperty& operator=(const MDDProperty& p) { _id = p._id;_ofs = p._ofs; _d = p._d;return *this;}
   size_t size() const { return storageSize() >> 3;}
   bool isUp() const { return _d == Up;}
   virtual void init(char* buf) const              {}
   virtual int get(char* buf) const                { return 0;}
   virtual MDDBSValue getBS(char* buf) const       { return MDDBSValue(nullptr,0,0);}
   virtual void setInt(char* buf,int v)            {}
   virtual void setByte(char* buf,unsigned char v) {}
   virtual MDDBSValue setBS(char* buf,const MDDBSValue& v)  { return MDDBSValue(nullptr,0,0);}
   virtual void setProp(char* buf,char* from)  {}
   virtual void print(std::ostream& os) const  {}
   virtual void stream(char* buf,std::ostream& os) const {}
   friend class MDDStateSpec;
};

inline std::ostream& operator<<(std::ostream& os,const enum MDDProperty::Direction& d) {
   switch(d) {
      case MDDProperty::Down: return os << "Down";
      case MDDProperty::Up:   return os << "Up";
   }
}

namespace Factory {
   inline MDDProperty::Ptr makeProperty(enum MDDProperty::Direction dir,short id,unsigned short ofs,int init,int max=0x7fffffff);
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
   MDDPInt(enum Direction dir,short id,unsigned short ofs,int init,int max=0x7fffffff)
      : MDDProperty(id,ofs,dir),_init(init),_max(max) {}
   void init(char* buf) const override     { *reinterpret_cast<int*>(buf+_ofs) = _init;}
   int get(char* buf) const override       { return *reinterpret_cast<int*>(buf+_ofs);}
   void setInt(char* buf,int v) override   { *reinterpret_cast<int*>(buf+_ofs) = v;}
   void stream(char* buf,std::ostream& os) const override { os << *reinterpret_cast<int*>(buf+_ofs);}
   void setProp(char* buf,char* from) override  { setInt(buf,get(from));}
   void print(std::ostream& os) const override  {
      os << "PInt(" << _d << ',' << _id << ',' << _ofs << ',' << _init << ',' << _max << ')';
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
   MDDPByte(enum Direction dir,short id,unsigned short ofs,unsigned char init,unsigned char max=255)
      : MDDProperty(id,ofs,dir),_init(init),_max(max) {}
   void init(char* buf) const override     { buf[_ofs] = _init;}
   int get(char* buf) const override       { int rv =  (unsigned char)buf[_ofs];return rv;}
   void setInt(char* buf,int v) override   { buf[_ofs] = (unsigned char)v;}
   void setByte(char* buf,unsigned char v) override { buf[_ofs] = v;}
   void stream(char* buf,std::ostream& os) const override { int v = (unsigned char)buf[_ofs];os << v;}
   void setProp(char* buf,char* from)  override { setByte(buf,get(from));}
   void print(std::ostream& os) const override  {
      os << "PByte(" << _d << ',' << _id << ',' << _ofs << ',' << (int)_init << ',' << (int)_max << ')';
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
   MDDPBit(enum Direction dir,short id,unsigned short ofs,unsigned char init)
      : MDDProperty(id,ofs,dir),_init(init),_bmask(0x1 << (id & 0x7)) {}
   void init(char* buf) const override     { buf[_ofs] = _init ? (buf[_ofs] | _bmask) : (buf[_ofs] & ~_bmask);}
   int get(char* buf) const override       { return ((unsigned char)buf[_ofs] & _bmask) == _bmask;}
   void setInt(char* buf,int v) override   { if (v) buf[_ofs] |= _bmask; else buf[_ofs] &= ~_bmask;}
   void setByte(char* buf,unsigned char v) override { buf[_ofs] = v ? (buf[_ofs] | _bmask) : (buf[_ofs] & ~_bmask);}
   void stream(char* buf,std::ostream& os) const override { os << (((unsigned char)buf[_ofs] & _bmask) == _bmask);}
   void setProp(char* buf,char* from) override { setInt(buf,get(from));}
   void print(std::ostream& os) const override  {
      os << "PBit(" << _d << ',' << _id << ',' << _ofs << ',' << (int)_init << ',' << (int)_bmask << ')';
   }
   friend class MDDStateSpec;   
};

class MDDPBitSequence : public MDDProperty {
   const int    _nbBits;
   unsigned char  _init;
   const short _nbWords; 
   size_t storageSize() const override     {
      int up;
      if (_nbBits % 64) {
         up = ((_nbBits / 64) + 1) * 64;
      } else up = _nbBits;
      return up;
   }
   size_t setOffset(size_t bitOffset) override {
      _ofs = bitOffset >> 3;
      _ofs = ((_ofs & 0x7) != 0)  ? (_ofs | 0x7)+1 : _ofs; // 8-byte align
      return (_ofs << 3) + storageSize();
   }
 public:
   MDDPBitSequence(enum Direction dir,short id,unsigned short ofs,int nbbits,unsigned char init) // init = 0 | 1
      : MDDProperty(id,ofs,dir),_nbBits(nbbits),_init(init),
        _nbWords((_nbBits % 64) ? _nbBits / 64 + 1 : _nbBits/64) {}   
   void init(char* buf) const override {
      unsigned long long* ptr = reinterpret_cast<unsigned long long*>(buf + _ofs);
      unsigned long long bmask = (_init) ? ~0x0ull : 0x0ull;
      for(int i=0;i < _nbWords - 1;i++)
         ptr[i] = bmask;
      if (_init) {
         int nbr = _nbBits % 64;
         unsigned long long lm = (1ull << (nbr+1)) - 1;
         ptr[_nbWords-1] = lm;
      }
   }
   MDDBSValue getBS(char* buf) const override   { return MDDBSValue(buf + _ofs,_nbWords,_nbBits);}
   MDDBSValue setBS(char* buf,const MDDBSValue& v) override {
      MDDBSValue dest(buf+_ofs,_nbWords,_nbBits);
      dest = v;
      return dest;
   }
   void setProp(char* buf,char* from) override {
      unsigned long long* p0 = reinterpret_cast<unsigned long long*>(buf + _ofs);
      unsigned long long* p1 = reinterpret_cast<unsigned long long*>(from + _ofs);
      for(int i=0;i < _nbWords;i++)
         p0[i] = p1[i];
   }
   void stream(char* buf,std::ostream& os) const override { os << getBS(buf);}
   void print(std::ostream& os) const override  {
      os << "PBS(" << _d << ',' << _id << ',' << _ofs << ',' << _nbBits << ',' << (int)_init << ')';
   }
   friend class MDDStateSpec;   
};


class MDDStateSpec {
protected:
   std::vector<MDDProperty::Ptr> _attrs;
   size_t _lsz;
public:
   MDDStateSpec() {}
   auto layoutSize() const noexcept { return _lsz;}
   void layout();
   virtual void varOrder() {}
   bool isUp(int p) const noexcept { return _attrs[p]->isUp();}
   auto size() const noexcept { return _attrs.size();}
   virtual int addState(MDDConstraintDescriptor&d, int init,int max=0x7fffffff);
   virtual int addStateUp(MDDConstraintDescriptor&d, int init,int max=0x7fffffff);
   virtual int addBSState(MDDConstraintDescriptor& d,int nbb,unsigned char init);
   virtual int addBSStateUp(MDDConstraintDescriptor& d,int nbb,unsigned char init);
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
   mutable float      _rip;
   struct Flags {
      bool         _relaxed:1;
      mutable bool  _ripped:1;
      bool          _unused:6;
   } _flags;
   class TrailState : public Entry {
      MDDState* _at;
      char*   _from;
      int       _sz;
      Flags  _f;
      float _ip;
   public:
      TrailState(MDDState* at,char* from,int sz) : _at(at),_from(from),_sz(sz)
      {
         memcpy(_from,_at->_mem,_sz);
         _f = _at->_flags;
         _ip = _at->_rip;
      }
      void restore() noexcept override {
         memcpy(_at->_mem,_from,_sz);
         _at->_flags = _f;
         _at->_rip   = _ip;
      }
   };
public:
   MDDState() : _spec(nullptr),_mem(nullptr),_flags({false,false}) {}
   MDDState(MDDStateSpec* s,char* b,bool relax=false) 
      : _spec(s),_mem(b),_flags({relax,false}) {
      if (_spec)
         memset(_mem,0,_spec->layoutSize());
   }
   MDDState(MDDStateSpec* s,char* b,/*int hash,*/bool relax,float rip,bool ripped) 
      : _spec(s),_mem(b),_rip(rip) {
         _flags._relaxed = relax;
         _flags._ripped = ripped;
      }
   MDDState(const MDDState& s) 
      : _spec(s._spec),_mem(s._mem),/*_hash(s._hash),*/_rip(s._rip),_flags(s._flags) {}
   void initState(const MDDState& s) {
      _spec = s._spec;
      _mem = s._mem;
      _flags = s._flags;
      _rip  = s._rip;
   }
   void copyState(const MDDState& s) {
      auto sz = _spec->layoutSize();
      memcpy(_mem,s._mem,sz);
      _flags = s._flags;
      _rip = s._rip;      
   }
   MDDState& assign(const MDDState& s,Trailer::Ptr t,Storage::Ptr mem) {
      auto sz = _spec->layoutSize();
      char* block = (char*)mem->allocate(sizeof(char)* sz);//new (mem) char[sz];
      t->trail(new (t) TrailState(this,block,(int)sz));      
      assert(_spec == s._spec || _spec == nullptr);
      assert(_mem != nullptr);
      _spec = s._spec;
      memcpy(_mem,s._mem,sz);
      _flags = s._flags;
      _rip = s._rip;
      return *this;
   }
   MDDState& operator=(MDDState&& s) { 
      assert(_spec == s._spec || _spec == nullptr);
      _spec = std::move(s._spec);
      _mem  = std::move(s._mem);      
      _rip  = std::move(s._rip);
      _flags = std::move(s._flags);
      return *this;
   }
   MDDState clone(Storage::Ptr mem) const {
      char* block = _spec ? (char*)mem->allocate(sizeof(char)*_spec->layoutSize()) : nullptr;
      if (_spec)  memcpy(block,_mem,_spec->layoutSize());
      return MDDState(_spec,block,/*_hash,*/_flags._relaxed,_rip,_flags._ripped);
   }
   bool valid() const noexcept         { return _mem != nullptr;}
   auto layoutSize() const noexcept    { return _spec->layoutSize();}   
   void init(int i) const  noexcept    { _spec->_attrs[i]->init(_mem);}
   int at(int i) const noexcept           { return _spec->_attrs[i]->get(_mem);}
   MDDBSValue getBS(int i) const noexcept { return _spec->_attrs[i]->getBS(_mem);}
   int operator[](int i) const noexcept   { return _spec->_attrs[i]->get(_mem);}  // to _read_ a state property
   void set(int i,int val) noexcept       { _spec->_attrs[i]->setInt(_mem,val);}  // to set a state property
   MDDBSValue setBS(int i,const MDDBSValue& val) noexcept { return _spec->_attrs[i]->setBS(_mem,val);}
   void setProp(int i,const MDDState& from) noexcept { _spec->_attrs[i]->setProp(_mem,from._mem);}
   int byteSize(int i) const noexcept   { return (int)_spec->_attrs[i]->size();}
   void clear() noexcept                { _flags._ripped = false;_flags._relaxed = false;}
   bool isRelaxed() const noexcept      { return _flags._relaxed;}
   void relax(bool r = true) noexcept   { _flags._relaxed = r;}
   float inner(const MDDState& s) const {
      if (_flags._ripped) 
         return _rip;
      _flags._ripped = true;

      unsigned long long* m0 = reinterpret_cast<unsigned long long*>(_mem);
      unsigned long long* m1 = reinterpret_cast<unsigned long long*>(s._mem);
      if (_mem && s._mem) {
         auto up = layoutSize() / 8;
         unsigned long long asw = 0;
         for(auto k=0u;k < up;k++) {
            asw <<= 1;
            asw += __builtin_popcountl(~(m0[k] ^ m1[k]));
         }
         _rip = asw;
      } else _rip = 0;
      return _rip;
   }
   
   int hash() {
      const auto nbw = _spec->layoutSize() / 4;
      int nlb = _spec->layoutSize() & 0x3;
      char* sfx = _mem + (nbw << 2);
      unsigned int* b = reinterpret_cast<unsigned int*>(_mem);
      unsigned int ttl = 0;
      for(auto s = 0u;s <nbw;s++)
         ttl = (ttl << 8) + (ttl >> (32-8)) + b[s];
      while(nlb-- > 0)
         ttl = (ttl << 8) + (ttl >> (32-8)) + *sfx++;
      return ttl;
   }
   bool stateChange(const MDDState& b) const {
      return memcmp(_mem,b._mem,_spec->layoutSize())!=0;
   }
   bool operator==(const MDDState& s) const {    // equality test likely O(1) when different.
      return memcmp(_mem,s._mem,_spec->layoutSize())==0 && _flags._relaxed == s._flags._relaxed;
   }
   bool operator!=(const MDDState& s) const {
      return memcmp(_mem,s._mem,_spec->layoutSize())!=0 || _flags._relaxed != s._flags._relaxed;
   }
   friend std::ostream& operator<<(std::ostream& os,const MDDState& s) {
      os << (s._flags._relaxed ? 'T' : 'F') << '[';
      if(s._spec != nullptr)
         for(auto atr : s._spec->_attrs) {
            atr->stream(s._mem,os);
            os << ' ';
         }
      return os << ']';
   }
};

class MDDSpec: public MDDStateSpec {
public:
   MDDSpec();
   MDDConstraintDescriptor& makeConstraintDescriptor(const Factory::Veci&, const char*);
   void varOrder() override;
   int addState(MDDConstraintDescriptor& d, int init,int max=0x7fffffff) override;
   int addStateUp(MDDConstraintDescriptor& d, int init,int max=0x7fffffff) override;
   int addState(MDDConstraintDescriptor& d,int init,size_t max) {return addState(d,init,(int)max);}
   int addBSState(MDDConstraintDescriptor& d,int nbb,unsigned char init) override;
   int addBSStateUp(MDDConstraintDescriptor& d,int nbb,unsigned char init) override;
   void addArc(const MDDConstraintDescriptor& d,ArcFun a);
   void addTransition(int,lambdaTrans);
   void addRelaxation(int,lambdaRelax);
   void addSimilarity(int,lambdaSim);
   void addTransitions(const lambdaMap& map);
   bool exist(const MDDState& a,const MDDState& c,var<int>::Ptr x,int v,bool up);
   double similarity(const MDDState& a,const MDDState& b);
   void createState(MDDState& result,const MDDState& parent,unsigned l,var<int>::Ptr var,int v);
   MDDState createState(Storage::Ptr& mem,const MDDState& state,unsigned l,var<int>::Ptr var, int v);
   void updateState(bool set,MDDState& target,const MDDState& source,var<int>::Ptr var,int v);
   void relaxation(MDDState& a,const MDDState& b);
   MDDState relaxation(Storage::Ptr& mem,const MDDState& a,const MDDState& b);
   MDDState rootState(Storage::Ptr& mem);
   bool usesUp() const { return _uptrans.size() > 0;}
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
   ArcFun                        _exist;
   std::vector<lambdaTrans> _transition;
   std::vector<lambdaRelax> _relaxation;
   std::vector<lambdaSim>   _similarity;
   std::vector<lambdaTrans> _uptrans;
};


std::pair<int,int> domRange(const Factory::Veci& vars);

#endif /* mddstate_hpp */

