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
#include <unordered_map>
#include <bitset>
#include <utility>
#include <xmmintrin.h>
#include <limits.h>
#include "hashtable.hpp"
#include "xxhash.hpp"

#define CACHING 1


class MDDIntSet {
   short  _mxs,_sz;
   bool  _isSingle;
   union {
      int*      _buf;
      int    _single;
   };
public:
   template <class Allocator>
   MDDIntSet(Allocator p,const MDDIntSet& s) {
      if (s._isSingle) {
         _single = s._single;
         _mxs = _sz = 1;
         _isSingle = true; 
      } else {
         _mxs = s._mxs;
         _sz  = s._sz;
         _isSingle = false;
         _buf = new (p) int[_mxs];
         memcpy(_buf,s._buf,sizeof(int)*_mxs);
      }
   }
   MDDIntSet() { _buf = 0;_mxs=_sz=0;_isSingle=false;}
   MDDIntSet(int v) : _mxs(1),_sz(1),_isSingle(true) {
      _single = v;
   }
   MDDIntSet(char* buf,int mxs,std::initializer_list<int> vals)
      : _mxs(mxs),_sz(0),_isSingle(false) {
      _buf = reinterpret_cast<int*>(buf);
      for(int v : vals) 
         _buf[_sz++] = v;      
   }
   MDDIntSet(char* buf,int mxs) : _mxs(mxs),_sz(0),_isSingle(false) {
      _buf = reinterpret_cast<int*>(buf);       
   }
   void clear() noexcept { _sz = 0;_isSingle=false;}
   constexpr void add(int v) noexcept {
      assert(_sz < _mxs);
      _buf[_sz++] = v;
   }
   void insert(int v) noexcept {
      if (contains(v)) return;
      if(_sz >= _mxs) {
         std::cerr << "Oops... overflowing MDDIntSet\n";
         exit(1);
      }
      _buf[_sz++] = v;
   }
   constexpr const bool contains(int v) const noexcept {
      if (_isSingle)
         return _single == v;
      else {
         for(short i=0;i < _sz;i++)
            if (_buf[i]==v) return true;     
         return false;
      }
   }
   const bool memberOutside(const ValueSet& S) const noexcept {
      if (_isSingle) return !S.member(_single);
      else {
         for(short k=0;k <_sz;k++)
            if (!S.member(_buf[k])) return true;
         return false;
      }
   }
   const bool allInside(const ValueSet& S) const noexcept {
      if (_isSingle) return S.member(_single);
      else {
         for(short k=0;k <_sz;k++)
            if (!S.member(_buf[k])) return false;
         return true;
      }
   }
   const bool memberInside(const ValueSet& S) const noexcept {
      if (_isSingle) return S.member(_single);
      else {
         for(short k=0;k <_sz;k++)
            if (S.member(_buf[k])) return true;
         return false;
      }
   }
   friend bool operator==(const MDDIntSet& a,const MDDIntSet& b) {
      if (a._isSingle == b._isSingle) {
         if (a._isSingle) return a._single == b._single;
         else {
            if (a._sz == b._sz) {
               for(auto i : a) 
                  if (!b.contains(i)) return false;               
               return true;
            } else return false;
         }
      } else return false;
   }
   int hash() const noexcept {
      if (_isSingle) return _single;
      else {
         int ttl=0;
         for(int k=0;k < _sz;++k)
            ttl += _buf[k];
         return ttl * _sz;
      }
   }
   constexpr const short size() const { return _sz;}
   constexpr const bool isEmpty() const { return _sz == 0;}
   constexpr const bool isSingleton() const { return _isSingle || _sz == 1;}
   constexpr const int  singleton() const {
      if (_isSingle)
         return _single;
      else  {
         assert(_sz == 1);
         return _buf[0];
      }
   }
   //constexpr operator int() const { return singleton();}
   class iterator: public std::iterator<std::input_iterator_tag,int,short> {
      union {
         int*    _data;
         int     _val;
      };
      short   _num;
      bool    _single;
      iterator(int* d,long num = 0) : _data(d),_num(num),_single(false) {}
      iterator(int v,long num = 0) : _val(v),_num(num),_single(true) {}
   public:
      iterator& operator++()   { _num = _num + 1; return *this;}
      iterator operator++(int) { iterator retval = *this; ++(*this); return retval;}
      iterator& operator--()   { _num = _num - 1; return *this;}
      iterator operator--(int) { iterator retval = *this; --(*this); return retval;}
      bool operator==(iterator other) const {return _num == other._num;}
      bool operator!=(iterator other) const {return !(*this == other);}
      int operator*() const   { return _single ? _val : _data[_num];}
      friend class MDDIntSet;
   };
   iterator begin() const { return _isSingle ? iterator(_single,0) : iterator(_buf,0);}
   iterator end()   const { return _isSingle ? iterator(_single,_sz) : iterator(_buf,_sz);}
   iterator cbegin() const noexcept { return begin();}
   iterator cend()   const noexcept { return end();}
   friend std::ostream& operator<<(std::ostream& os,const MDDIntSet& s) {
      os << '{';
      for(int v : s)
         os << v << ',';
      return os << '\b' << '}';
   }
};

void printSet(const MDDIntSet& s);

enum RelaxWith { External, MinFun,MaxFun};

class MDDState;
class MDDNode;
typedef std::function<bool(const MDDState&)> NodeFun;
typedef std::function<bool(const MDDState&,const MDDState&,const var<int>::Ptr&,int,bool)> ArcFun;
typedef std::function<void(const MDDState&)> FixFun;
typedef std::function<void(MDDState&)> UpdateFun;
typedef std::function<void(MDDState&,const MDDState&,const var<int>::Ptr&,const MDDIntSet&,bool)> lambdaTrans;
typedef std::function<void(MDDState&,const MDDState&,const MDDState&)> lambdaRelax;
typedef std::function<double(const MDDState&,const MDDState&)> lambdaSim;
typedef std::function<double(const MDDNode&)> SplitFun;
typedef std::pair<std::set<int>,lambdaTrans> TransDesc;
typedef std::map<int,TransDesc> lambdaMap;
class MDDStateSpec;

class Zone {
   unsigned short _startOfs;
   unsigned short _length;
   std::set<int> _props;
public:
   Zone(unsigned short so,unsigned short eo,const std::set<int>& ps) : _startOfs(so),_length(eo-so),_props(ps) {}   
   friend std::ostream& operator<<(std::ostream& os,const Zone& z) {
      os << "zone(" << z._startOfs << "-->" << (int)z._startOfs + z._length << ")";return os;
   }
   void copy(char* dst,char* src) const noexcept { memcpy(dst+_startOfs,src+_startOfs,_length);}
};


class MDDConstraintDescriptor {
   Factory::Veci          _vars;
   ValueSet               _vset;
   const char*            _name;
   std::vector<int> _properties;
   std::vector<int> _tid; // transition ids
   std::vector<int> _rid; // relaxation ids
   std::vector<int> _sid; // similarity ids
   std::vector<int> _utid; // up transition ids
public:
   typedef handle_ptr<MDDConstraintDescriptor> Ptr;
   template <class Vec>
   MDDConstraintDescriptor(const Vec& vars, const char* name) 
      : _vars(vars.size(),Factory::alloci(vars[0]->getStore())),
        _vset(vars),
        _name(name)
   {
      for(typename Vec::size_type i=0;i < vars.size();i++)
         _vars[i] = vars[i];
   }
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
   void registerDown(int t)       { _tid.emplace_back(t);}
   void registerUp(int t)         { _utid.emplace_back(t);}
   void registerRelaxation(int t) { _rid.emplace_back(t);}
   void registerSimilarity(int t) { _sid.emplace_back(t);}
   bool inScope(const var<int>::Ptr& x) const noexcept { return _vset.member(x->getId());}
   const Factory::Veci& vars() const { return _vars;}
   std::vector<int>& properties() { return _properties;}
   auto begin() { return _properties.begin();}
   auto end()   { return _properties.end();}
   void print (std::ostream& os) const { os << _name << "(" << _vars << ")\n";}
};

class MDDBSValue {
   unsigned long long* _buf;
   const  short        _nbw;
public:
   MDDBSValue(char* buf,short nbw)
      : _buf(reinterpret_cast<unsigned long long*>(buf)),_nbw(nbw) {}   
   MDDBSValue(MDDBSValue&& v) : _buf(v._buf),_nbw(v._nbw) { v._buf = nullptr;}
   short nbWords() const noexcept { return _nbw;}
   MDDBSValue& operator=(const MDDBSValue& v) noexcept {
      for(int i=0;i <_nbw;i++)
         _buf[i] = v._buf[i];
      assert(_nbw == v._nbw);
      return *this;
   }
   constexpr bool getBit(const int ofs) const noexcept {
      const int wIdx = ofs >> 6;
      const int bOfs = ofs & ((1<<6) - 1);
      return (_buf[wIdx] >> bOfs) & 0x1;
   }
   void clear(const int ofs) noexcept {
      const int wIdx = ofs >> 6;
      const int bOfs = ofs & ((1<<6) - 1);
      const unsigned long long bmask = 0x1ull << bOfs;
      _buf[wIdx] &= ~bmask;
   }
   void set(const int ofs) noexcept {
      const int wIdx = ofs >> 6;
      const int bOfs = ofs & ((1<<6)-1);
      const unsigned long long bmask = 0x1ull << bOfs;
      _buf[wIdx] |= bmask;      
   }
   constexpr int cardinality() const noexcept {
      int nbb = 0;
      for(int i = (int)_nbw-1;i >= 0;--i) 
         nbb += __builtin_popcountll(_buf[i]);
      //nbb += _mm_popcnt_u64(_buf[i]);
      return nbb;
   }
   __attribute__((always_inline)) inline MDDBSValue& setBinOR(const MDDBSValue& a,const MDDBSValue& b) noexcept {
      switch(_nbw) {
         case 1: _buf[0] = a._buf[0] | b._buf[0];return *this;
         case 2: {
            __m128i p0 = *(__m128i*) a._buf;
            __m128i p1 = *(__m128i*) b._buf;
            *(__m128i*)_buf = _mm_or_si128(p0,p1);
         } return *this;
         default:
            for(int i=0;i < _nbw;i++)
               _buf[i] = a._buf[i] | b._buf[i];
            return *this;
      }
   }
   MDDBSValue& setBinAND(const MDDBSValue& a,const MDDBSValue& b) noexcept {
      for(int i=0;i < _nbw;i++)
         _buf[i] = a._buf[i] & b._buf[i];
      return *this;
   }
   MDDBSValue& setBinXOR(const MDDBSValue& a,const MDDBSValue& b) noexcept {
      for(int i=0;i < _nbw;i++)
         _buf[i] = a._buf[i] ^ b._buf[i];
      return *this;
   }
   MDDBSValue& NOT() noexcept {
      for(int i=0;i <_nbw;i++)
         _buf[i] = ~_buf[i];
      return *this;
   }
   class iterator : public std::iterator<std::input_iterator_tag,short,short> {
      unsigned long long* _t;
      const short _nbw;
      short _cwi;    // current word index
      unsigned long long _cw; // current word
      iterator(unsigned long long* t,short nbw,short at)
         : _t(t),_nbw(nbw),_cwi(at),_cw((at < nbw) ? t[at] : 0) {
         while (_cw == 0 && ++_cwi < _nbw) 
            _cw = _t[_cwi];         
      }
      iterator(unsigned long long* t,short nbw) : _t(t),_nbw(nbw),_cwi(nbw),_cw(0) {} // end constructor
   public:
      iterator& operator++()  noexcept {
         unsigned long long test = _cw & -_cw;  // only leaves LSB at 1
         _cw ^= test;                  // clear LSB
         while (_cw == 0 && ++_cwi < _nbw)  // all bits at zero-> done with this word.            
            _cw = _t[_cwi];        
         return *this;
      }
      iterator operator++(int) { iterator retval = *this; ++(*this); return retval;}
      bool operator==(iterator other) const {return _cwi == other._cwi && _cw == other._cw;}
      bool operator!=(iterator other) const {return !(*this == other);}
      short operator*() const   { return (_cwi<<6) + __builtin_ctzl(_cw);}
      friend class MDDBSValue;
   };
   iterator begin() const { return iterator(_buf,_nbw,0);}
   iterator end()   const { return iterator(_buf,_nbw);}

   friend bool operator==(const MDDBSValue& a,const MDDBSValue& b) {
      bool eq = a._nbw == b._nbw;
      for(short i = 0 ;eq && i < a._nbw;i++)
         eq = a._buf[i] == b._buf[i];
      return eq;
   }
   friend bool operator!=(const MDDBSValue& a,const MDDBSValue& b) {
      bool eq = a._nbw == b._nbw;
      for(short i = 0 ;eq && i < a._nbw;i++)
         eq = a._buf[i] == b._buf[i];
      return !eq;
   }
};

template <class ET>
class MDDSWin {
   ET* _buf;
   int _nb;
public:
   MDDSWin(ET* buf,int nb) : _buf(buf),_nb(nb) {}
   MDDSWin(MDDSWin<ET>&& v) : _buf(v._buf),_nb(v._nb) {}
   int nbWords() const noexcept { return _nb;}
   MDDSWin<ET>& operator=(const MDDSWin<ET>& v) noexcept {
      for(int i=0;i < _nb;++i) _buf[i] = v._buf[i];
      return *this;
   }
   ET get(int ofs) const noexcept { return _buf[ofs];}
   ET last() const noexcept { return _buf[_nb-1];}
   ET first() const noexcept { return _buf[0];}
   void set(int ofs,ET v) noexcept { _buf[ofs] = v;}
   void setFirst(ET v) noexcept { _buf[0] = v;}
   MDDSWin<ET>& assignSlideBy(const MDDSWin<ET>& v,const int shift=0) {
      for(int i=0;i < _nb - shift;++i) _buf[i+shift] = v._buf[i];
      return *this;
   }
   friend bool operator==(const MDDSWin<ET>& a,const MDDSWin<ET>& b) noexcept {
      bool eq = a._nb == b._nb;
      for(int i=0;eq && i < a._nb;++i)
         eq = a._buf[i] == b._buf[i];
      return eq;
   }
   friend bool operator!=(const MDDSWin<ET>& a,const MDDSWin<ET>& b) noexcept {
      bool eq = a._nb == b._nb;
      for(int i=0;eq && i < a._nb;++i)
         eq = a._buf[i] == b._buf[i];
      return !eq;
   }
};

enum Direction { None=0,Down=1,Up=2,Bi=3 };

class MDDProperty {
protected:
   short _id;
   unsigned short _ofs; // offset in bytes within block
   unsigned short _bsz; // size in bytes.
   enum Direction _dir;
   enum RelaxWith _rw;
   std::set<int>  _sp;
   int            _downTId;
   int            _upTId;
   int            _rid;
   virtual size_t storageSize() const = 0;  // given in _bits_
   virtual size_t setOffset(size_t bitOffset) = 0;
public:
   typedef handle_ptr<MDDProperty> Ptr;
   MDDProperty(const MDDProperty& p)
      : _id(p._id),_ofs(p._ofs),_bsz(p._bsz),_dir(p._dir),_rw(p._rw),_downTId(p._downTId),_upTId(p._upTId),_rid(p._rid) {}
   MDDProperty(MDDProperty&& p)
      : _id(p._id),_ofs(p._ofs),_bsz(p._bsz),_dir(p._dir),_rw(p._rw),_downTId(p._downTId),_upTId(p._upTId),_rid(p._rid) {}
   MDDProperty(short id,unsigned short ofs,unsigned short bsz,enum RelaxWith rw = External)
      : _id(id),_ofs(ofs),_bsz(bsz),_dir(None),_rw(rw) { _downTId = _upTId = -1;_rid = -1;}
   MDDProperty& operator=(const MDDProperty& p) {
      _id = p._id;_ofs = p._ofs; _bsz = p._bsz;_dir = p._dir;_rw = p._rw;
      _downTId = p._downTId;
      _upTId = p._upTId;
      _rid = p._rid;
      return *this;
   }
   enum RelaxWith relaxFun() const noexcept { return _rw;}
   unsigned short startOfs() const noexcept { return _ofs;}
   unsigned short endOfs() const noexcept   { return _ofs + _bsz;}
   size_t size() const noexcept { return storageSize() >> 3;}
   void setDirection(enum Direction d) { _dir = (enum Direction)(_dir | d);}
   void setAntecedents(const std::set<int>& sp) { _sp = sp;}
   void setDown(int tid) noexcept  { _downTId = tid;}
   void setUp(int tid) noexcept    { _upTId = tid;}
   void setRelax(int rid) noexcept { _rid = rid;}
   int getDown() const noexcept    { return _downTId;}
   int getUp() const noexcept      { return _upTId;}
   int getRelax() const noexcept   { return _rid;}
   const std::set<int>& antecedents() const { return _sp;}
   bool isUp() const noexcept    { assert(_dir != None);return (_dir & Up) == Up;}
   bool isDown() const noexcept  { assert(_dir != None);return (_dir & Down) == Down;}
   virtual void init(char* buf) const noexcept              {}
   virtual int get(char* buf) const noexcept                { return 0;}
   virtual void minWith(char* buf,char* other) const noexcept {}
   virtual void maxWith(char* buf,char* other) const noexcept {}
   virtual bool diff(char* buf,char* other) const noexcept  { return false;}
   int getInt(char* buf) const noexcept                     { return *reinterpret_cast<int*>(buf + _ofs);}
   int getByte(char* buf) const noexcept                    { return buf[_ofs];}
   MDDBSValue getBS(char* buf) const noexcept               { return MDDBSValue(buf + _ofs,_bsz >> 3);}
   template <class ET>
   MDDSWin<ET> getSW(char* buf) const noexcept              { return MDDSWin<ET>(buf + _ofs,_bsz / sizeof(ET));}
   virtual void set(char* buf,int v) noexcept               {}
   void setInt(char* buf,int v) noexcept                    { *reinterpret_cast<int*>(buf+_ofs) = v;}
   void setByte(char* buf,char v) noexcept         { buf[_ofs] = v;}
   MDDBSValue setBS(char* buf,const MDDBSValue& v) noexcept {
      MDDBSValue dest(buf + _ofs,_bsz >> 3);
      dest = v;
      return dest;
   }
   void setProp(char* buf,char* from) noexcept   {
      switch(_bsz) {
         case 1: buf[_ofs] = from[_ofs];break;
         case 2: *reinterpret_cast<short*>(buf+_ofs) = *reinterpret_cast<short*>(from+_ofs);break;
         case 4: *reinterpret_cast<int*>(buf+_ofs) = *reinterpret_cast<int*>(from+_ofs);break;
         case 8: *reinterpret_cast<long long*>(buf+_ofs) = *reinterpret_cast<long long*>(from+_ofs);break;
         default: memcpy(buf + _ofs,from + _ofs,_bsz);break;
      }
   }
   virtual void print(std::ostream& os) const  = 0;
   virtual void stream(char* buf,std::ostream& os) const {}
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
   MDDPInt(short id,unsigned short ofs,int init,int max,enum RelaxWith rw)
      : MDDProperty(id,ofs,4,rw),_init(init),_max(max) {}
   void init(char* buf) const noexcept override      { *reinterpret_cast<int*>(buf+_ofs) = _init;}
   int get(char* buf) const noexcept override        { return *reinterpret_cast<int*>(buf+_ofs);}
   void set(char* buf,int v) noexcept override       { *reinterpret_cast<int*>(buf+_ofs) = v;}
   void stream(char* buf,std::ostream& os) const override { os << *reinterpret_cast<int*>(buf+_ofs);}
   void minWith(char* buf,char* other) const noexcept override {
      int* o = reinterpret_cast<int*>(buf+_ofs);
      int* i = reinterpret_cast<int*>(other+_ofs);
      *o = std::min(*o,*i);
   }
   void maxWith(char* buf,char* other) const noexcept override {
      int* o = reinterpret_cast<int*>(buf+_ofs);
      int* i = reinterpret_cast<int*>(other+_ofs);
      *o = std::max(*o,*i);
   }
   bool diff(char* buf,char* other) const noexcept  override {
      int o = *reinterpret_cast<int*>(buf+_ofs);
      int i = *reinterpret_cast<int*>(other+_ofs);
      return o != i;
   }
   void print(std::ostream& os) const override  {
      os << "PInt(" << _id << ',' << _ofs << ',' << _init << ',' << _max << ')';
   }
   friend class MDDStateSpec;
};

class MDDPByte :public MDDProperty {
   char _init;
   char  _max;
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
   MDDPByte(short id,unsigned short ofs,char init,char max,enum RelaxWith rw)
      : MDDProperty(id,ofs,1,rw),_init(init),_max(max) {}
   void init(char* buf) const  noexcept override     { buf[_ofs] = _init;}
   int get(char* buf) const  noexcept override       { return buf[_ofs];}
   void set(char* buf,int v) noexcept override       { buf[_ofs] = (char)v;}
   void stream(char* buf,std::ostream& os) const override { int v = buf[_ofs];os << v;}
   void minWith(char* buf,char* other) const noexcept override {
      buf[_ofs] = std::min(buf[_ofs],other[_ofs]);
   }
   void maxWith(char* buf,char* other) const noexcept override {
      buf[_ofs] = std::max(buf[_ofs],other[_ofs]);
   }
   bool diff(char* buf,char* other) const noexcept  override {
      return buf[_ofs] != other[_ofs];
   }
   void print(std::ostream& os) const override  {
      os << "PByte(" << _id << ',' << _ofs << ',' << (int)_init << ',' << (int)_max << ')';
   }
   friend class MDDStateSpec;
};

class MDDPBitSequence : public MDDProperty {
   const int    _nbBits;
   unsigned char  _init;
   size_t storageSize() const override     {
      int up;
      if (_nbBits % 64) {
         up = ((_nbBits / 64) + 1) * 64;
      } else up = _nbBits;
      return up;
   }
   size_t setOffset(size_t bitOffset) override {
      _ofs = bitOffset >> 3;
      _ofs = ((_ofs & 0xF) != 0)  ? (_ofs | 0xF)+1 : _ofs; // 16-byte align
      return (_ofs << 3) + storageSize();
   }
 public:
   MDDPBitSequence(short id,unsigned short ofs,int nbbits,unsigned char init) // init = 0 | 1
      : MDDProperty(id,ofs,8 * ((nbbits % 64) ? nbbits/64 + 1 : nbbits/64)),_nbBits(nbbits),_init(init)
   {}   
   void init(char* buf) const noexcept override {
      unsigned long long* ptr = reinterpret_cast<unsigned long long*>(buf + _ofs);
      unsigned short nbw = _bsz >> 3;
      for(unsigned short i=0u;i < nbw;++i) ptr[i] = 0x0ull;
      unsigned long long bmask = (_init) ? ~0x0ull : 0x0ull;
      short nbWords = _bsz >> 3;
      for(int i=0;i < nbWords - 1;i++)
         ptr[i] = bmask;
      if (_init) {
         int nbr = _nbBits % 64;
         unsigned long long lm = (1ull << (nbr+1)) - 1;
         ptr[nbWords-1] = lm;
      }
   }
   bool diff(char* buf,char* other) const noexcept  override {
      return getBS(buf) != getBS(other);
   }
   void stream(char* buf,std::ostream& os) const override {
      unsigned long long* words = reinterpret_cast<unsigned long long*>(buf + _ofs);
      os << '[';
      unsigned nbb = _nbBits;
      int val = 0;
      short nbWords = _bsz >> 3;
      for(int i=0;i < nbWords;i++) {
         unsigned long long w = words[i];
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
   }
   void print(std::ostream& os) const override  {
      os << "PBS(" << _id << ',' << _ofs << ',' << _nbBits << ',' << (int)_init << ')';
   }
   friend class MDDStateSpec;   
};

template <class ET = unsigned char>
class MDDPSWindow : public MDDProperty {
   ET    _eltInit;
   ET    _fstInit;
   const int _len; // number of elements in window (0,...._len-1)
   size_t storageSize() const override {
      return _len * sizeof(ET) * 8; // number of element * size of element in bytes * number of bits per byte.
   }
   size_t setOffset(size_t bitOffset) override {
      constexpr int bitAlign = std::alignment_of<ET>::value * sizeof(char); // get byte, then bit alignment for ET
      if (bitOffset & (bitAlign - 1)) // if not properly aligned for ET's requirements. 
         bitOffset = (bitOffset | (bitAlign - 1)) + 1;  // realign by setting all alignment bits to 1 and adding 1. 
      _ofs = bitOffset >> 3; // _ofs is the start location with alignment respected
      return bitOffset + storageSize();
   }
public:
   MDDPSWindow(short id,unsigned short ofs,int len,ET eInit,ET fInit,enum RelaxWith rw)
      : MDDProperty(id,ofs,len * sizeof(ET),rw),_eltInit(eInit),_fstInit(fInit),_len(len) {}
   void init(char* buf) const noexcept override {
      ET* ptr = reinterpret_cast<ET*>(buf + _ofs);
      for(int i=0;i < _len;++i)
         ptr[i] = _eltInit;
      ptr[0] = _fstInit;
   }
   void minWith(char* buf,char* other) const noexcept override {
      ET* a = reinterpret_cast<ET*>(buf + _ofs);
      ET* b = reinterpret_cast<ET*>(other + _ofs);
      for(int i=0;i < _len;++i)
         a[i] = std::min(a[i],b[i]);
   }
   void maxWith(char* buf,char* other) const noexcept override {
      ET* a = reinterpret_cast<ET*>(buf + _ofs);
      ET* b = reinterpret_cast<ET*>(other + _ofs);
      for(int i=0;i < _len;++i)
         a[i] = std::max(a[i],b[i]);
   }
   bool diff(char* buf,char* other) const noexcept override {
      ET* a = reinterpret_cast<ET*>(buf + _ofs);
      ET* b = reinterpret_cast<ET*>(other + _ofs);
      for(int i=0;i < _len;++i)
         if (a[i] != b[i]) return true;
      return false;      
   }
   void stream(char* buf,std::ostream& os) const override {
      os << '<';
      ET* ptr = reinterpret_cast<ET*>(buf + _ofs);
      for(int i=0;i < _len;++i) {
         os << (int)(ptr[i]);
         if (i < _len - 1)
            os << ',';        
      }
      os << '>';
   }
   void print(std::ostream& os) const override {
      os << "PW(" << _id << ',' << _ofs << ',' << _len << ')';
   }
   friend class MDDStateSpec;
};

class MDDStateSpec {
protected:
   MDDProperty** _attrs;
   MDDPropSet*   _omap;
   short _mxp;
   short _nbp;
   size_t _lsz;
   enum RelaxWith _relax;
   void addProperty(MDDProperty::Ptr p) noexcept;
public:
   MDDStateSpec();
   const auto layoutSize() const noexcept { return _lsz;}
   void layout();
   virtual void varOrder() {}
   auto size() const noexcept { return _nbp;}
   bool isUp(int p) const noexcept { return _attrs[p]->isUp();}
   bool isDown(int p) const noexcept { return _attrs[p]->isDown();}
   unsigned short startOfs(int p) const noexcept { return _attrs[p]->startOfs();}
   unsigned short endOfs(int p) const noexcept { return _attrs[p]->endOfs();}
   virtual int addState(MDDConstraintDescriptor::Ptr d, int init,int max,enum RelaxWith rw = External);
   virtual int addBSState(MDDConstraintDescriptor::Ptr d,int nbb,unsigned char init);
   virtual int addSWState(MDDConstraintDescriptor::Ptr d,int len,int init,int finit,enum RelaxWith rw = External);
   std::vector<int> addStates(MDDConstraintDescriptor::Ptr d,int from, int to, int max,std::function<int(int)> clo);
   std::vector<int> addStates(MDDConstraintDescriptor::Ptr d,int max,std::initializer_list<int> inputs);
   void outputSet(MDDPropSet& out,const MDDPropSet& in) const noexcept {
      for(auto p : in)
         out.unionWith(_omap[p]);
   }
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
   mutable int       _hash;
   mutable float      _rip;
   struct Flags {
      bool          _drelax:1;
      bool          _urelax:1;
      mutable bool  _ripped:1;
      mutable bool  _hashed:1;
      bool          _unused:4;
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
   MDDState() : _spec(nullptr),_mem(nullptr),_hash(0),_rip(0),_flags({false,false,false,false}) {}
   MDDState(MDDStateSpec* s,char* b,bool relax=false) 
      : _spec(s),_mem(b),_hash(0),_rip(0),_flags({relax,false,false,false}) {
      // if (_spec)
      //    memset(_mem,0,_spec->layoutSize());
   }
   MDDState(MDDStateSpec* s,char* b,int hash,float rip,const Flags& f) 
      : _spec(s),_mem(b),_hash(hash),_rip(rip),_flags(f) {}
   MDDState(const MDDState& s) 
      : _spec(s._spec),_mem(s._mem),_hash(s._hash),_rip(s._rip),_flags(s._flags) {}
   void initState(const MDDState& s) {
      _spec = s._spec;
      _mem = s._mem;
      _flags = s._flags;
      _rip  = s._rip;
      _hash = s._hash;
   }
   void copyState(const MDDState& s) {
      auto sz = _spec->layoutSize();
      memcpy(_mem,s._mem,sz);
      _flags = s._flags;
      _rip = s._rip;
      _hash = s._hash;
   }
   MDDStateSpec* getSpec() const noexcept { return _spec;}
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
      _hash = s._hash;
      return *this;
   }
   MDDState& operator=(MDDState&& s) { 
      assert(_spec == s._spec || _spec == nullptr);
      _spec = std::move(s._spec);
      _mem  = std::move(s._mem);      
      _rip  = std::move(s._rip);
      _hash = std::move(s._hash);
      _flags = std::move(s._flags);
      return *this;
   }
   template <class Allocator>
   MDDState clone(Allocator pool) const {
      char* block = _spec ? new (pool) char[_spec->layoutSize()] : nullptr;
      if (_spec)  memcpy(block,_mem,_spec->layoutSize());
      return MDDState(_spec,block,_hash,_rip,_flags);
   }
   bool valid() const noexcept         { return _mem != nullptr;}
   auto layoutSize() const noexcept    { return _spec->layoutSize();}   
   void init(int i) const  noexcept    { _spec->_attrs[i]->init(_mem);}
   int operator[](int i) const noexcept   { return _spec->_attrs[i]->getInt(_mem);}  // to _read_ a state property (fast)
   int at(int i) const noexcept           { return _spec->_attrs[i]->get(_mem);}
   int byte(int i) const noexcept         { return _spec->_attrs[i]->getByte(_mem);}
   MDDBSValue getBS(int i) const noexcept { return _spec->_attrs[i]->getBS(_mem);}
   MDDSWin<char> getSW(int i) const noexcept { return _spec->_attrs[i]->getSW<char>(_mem);}
   void set(int i,int val) noexcept       { _spec->_attrs[i]->set(_mem,val);}  // to set a state property (slow)
   void setInt(int i,int val) noexcept    { _spec->_attrs[i]->setInt(_mem,val);}  // to set a state property (fast)
   void setByte(int i,int val) noexcept   { _spec->_attrs[i]->setByte(_mem,val);}  // to set a state property (fast)  
   MDDBSValue setBS(int i,const MDDBSValue& val) noexcept { return _spec->_attrs[i]->setBS(_mem,val);} // (fast)
   void setProp(int i,const MDDState& from) noexcept { _spec->_attrs[i]->setProp(_mem,from._mem);} // (fast)
   int byteSize(int i) const noexcept   { return (int)_spec->_attrs[i]->size();}
   void copyZone(const Zone& z,const MDDState& in) noexcept { z.copy(_mem,in._mem);}
   void clear() noexcept                { _flags._ripped = false;_flags._drelax = false;_flags._urelax = false;_flags._hashed=false;}
   bool isRelaxed() const noexcept          { return _flags._drelax || _flags._urelax;}
   bool isUpRelaxed() const noexcept        { return _flags._urelax;}
   bool isDownRelaxed() const noexcept      { return _flags._drelax;}
   void relaxDown(bool r = true) noexcept   { _flags._drelax = r;}
   void relaxUp(bool r = true) noexcept     { _flags._urelax = r;}
   void minWith(int i,const MDDState& s) const noexcept { _spec->_attrs[i]->minWith(_mem,s._mem);}
   void maxWith(int i,const MDDState& s) const noexcept { _spec->_attrs[i]->maxWith(_mem,s._mem);}
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
   int hash() const noexcept {
      if (_flags._hashed)
         return _hash;
      return computeHash();
   }
   //constexpr int computeHash() const noexcept { return 0;}
   int computeHash() const noexcept {
#if 0 && !defined(CACHING)
      return _hash = 0;
#else      
      xxh::hash_t<64> hv = xxh::xxhash<64>(_mem,_spec->layoutSize());
      _flags._hashed = true;
      return _hash = (int)hv;
      // const auto nbw = _spec->layoutSize() / 4;
      // int nlb = _spec->layoutSize() & 0x3;
      // char* sfx = _mem + (nbw << 2);
      // unsigned int* b = reinterpret_cast<unsigned int*>(_mem);
      // unsigned int ttl = 0;
      // for(auto s = 0u;s <nbw;s++)
      //    ttl = (ttl << 8) + (ttl >> (32-8)) + b[s];
      // while(nlb-- > 0)
      //    ttl = (ttl << 8) + (ttl >> (32-8)) + *sfx++;
      // _flags._hashed = true;
      // return _hash = ttl;
#endif  
   }
   bool stateEqual(const MDDState& b) const noexcept {
      return memcmp(_mem,b._mem,_spec->layoutSize())==0;
   }
   bool stateChange(const MDDState& b) const noexcept {
      return memcmp(_mem,b._mem,_spec->layoutSize())!=0;
   }
   bool operator==(const MDDState& s) const {    
      return (!_flags._hashed || !s._flags._hashed || _hash == s._hash) &&
         _flags._drelax == s._flags._drelax &&         
         //_flags._urelax == s._flags._urelax &&
         memcmp(_mem,s._mem,_spec->layoutSize())==0;
   }
   bool operator!=(const MDDState& s) const {
      return ! this->operator==(s);
      //_flags._drelax != s._flags._drelax || memcmp(_mem,s._mem,_spec->layoutSize())!=0;
   }
   void diffWith(const MDDState& s,MDDPropSet& into) const {
      for(int p=0;p < _spec->_nbp;++p) 
         if (s._spec->_attrs[p]->diff(_mem,s._mem))
            into.setProp(p);      
   }
   friend std::ostream& operator<<(std::ostream& os,const MDDState& s) {
      os << (s._flags._drelax ? 'T' : 'F')
         << (s._flags._urelax ? 'T' : 'F') << '[';
      for(int p=0;p < s._spec->_nbp;p++) {
         auto atr = s._spec->_attrs[p];
         atr->stream(s._mem,os);
         os << ' ';
      }      
      return os << ']';
   }
};

class MDDSpec;
class LayerDesc {
   std::vector<int> _dframe;
   std::vector<int> _uframe;
   std::vector<Zone> _dzones;
   std::vector<Zone> _uzones;
   MDDPropSet         _dprop;
public:
   LayerDesc(int nbp) : _dprop(nbp) {}
   const std::vector<int>& downProps() const { return _dframe;}
   const std::vector<int>& upProps() const { return _uframe;}
   bool hasProp(int p) const noexcept { return _dprop.hasProp(p);}
   void addDownProp(int p) { _dframe.emplace_back(p);}
   void addUpProp(int p)   { _uframe.emplace_back(p);}
   void zoningUp(const MDDSpec& spec);
   void zoningDown(const MDDSpec& spec);
   void zoning(const MDDSpec& spec);
   void frameDown(MDDState& out,const MDDState& in) {
      for(const auto& z : _dzones)
         out.copyZone(z,in);
   }
   void frameUp(MDDState& out,const MDDState& in) {
      for(const auto& z : _uzones)
         out.copyZone(z,in);
   }
};   

class MDDSpec: public MDDStateSpec {
public:
   MDDSpec();
   // End-user API to define an ADD
   template <class Container>
   MDDConstraintDescriptor::Ptr makeConstraintDescriptor(const Container& v, const char* n) {
      return constraints.emplace_back(new MDDConstraintDescriptor(v,n));
   }
   int addState(MDDConstraintDescriptor::Ptr d,int init,int max,enum RelaxWith rw=External) override;
   int addState(MDDConstraintDescriptor::Ptr d,int init,size_t max,enum RelaxWith rw=External) {
      return addState(d,init,(int)max,rw);
   }
   int addBSState(MDDConstraintDescriptor::Ptr d,int nbb,unsigned char init) override;
   int addSWState(MDDConstraintDescriptor::Ptr d,int len,int init,int finit,enum RelaxWith rw = External) override;
   void nodeExist(const MDDConstraintDescriptor::Ptr d,NodeFun a);
   void arcExist(const MDDConstraintDescriptor::Ptr d,ArcFun a);
   void updateNode(UpdateFun update);
   void transitionDown(int,std::set<int> sp,lambdaTrans);
   void transitionUp(int,std::set<int> sp,lambdaTrans);
   template <typename LR> void addRelaxation(int p,LR r) {
      _xRelax.insert(p);
      int rid = (int)_relaxation.size();
      for(auto& cd : constraints)
         if (cd->ownsProperty(p)) {
            cd->registerRelaxation(rid);
            _attrs[p]->setRelax(rid);
            break;
         }  
      _relaxation.emplace_back(std::move(r));
   }
   void addSimilarity(int,lambdaSim);
   void transitionDown(const lambdaMap& map);
   void transitionUp(const lambdaMap& map);
   double similarity(const MDDState& a,const MDDState& b);
   void onFixpoint(FixFun onFix);
   void splitOnLargest(SplitFun onSplit);
   // Internal methods.
   void varOrder() override;
   bool consistent(const MDDState& a,const var<int>::Ptr& x) const noexcept;
   void updateNode(MDDState& a) const noexcept;
   bool exist(const MDDState& a,const MDDState& c,const var<int>::Ptr& x,int v,bool up) const noexcept;
   void copyStateUp(MDDState& result,const MDDState& source);
   void createState(MDDState& result,const MDDState& parent,unsigned l,const var<int>::Ptr& var,const MDDIntSet& v,bool up);
   void createStateIncr(const MDDPropSet& out,MDDState& result,const MDDState& parent,unsigned l,const var<int>::Ptr& var,const MDDIntSet& v,bool hasUp);
   void updateState(MDDState& target,const MDDState& source,unsigned l,const var<int>::Ptr& var,const MDDIntSet& v);
   void relaxation(MDDState& a,const MDDState& b) const noexcept;
   void relaxationIncr(const MDDPropSet& out,MDDState& a,const MDDState& b) const noexcept;
   MDDState rootState(Storage::Ptr& mem);
   bool usesUp() const { return _uptrans.size() > 0;}
   template <class Container> void append(const Container& y) {
      for(auto e : y)
         if(std::find(x.cbegin(),x.cend(),e) == x.cend())
            x.push_back(e);
      std::cout << "size of x: " << x.size() << std::endl;
   }
   template <class Container> void addGlobal(const Container& y) {
      for(auto e : y)
         if(std::find(z.cbegin(),z.cend(),e) == z.cend())
            z.push_back(e);
      std::cout << "size of z: " << z.size() << std::endl;
   }
   void reachedFixpoint(const MDDState& sink);
   double splitPriority(const MDDNode& n) const;
   bool hasSplitRule() const noexcept { return _onSplit.size() > 0;}
   void compile();
   std::vector<var<int>::Ptr>& getVars(){ return x; }
   std::vector<var<int>::Ptr>& getGlobals() { return z;}
   friend std::ostream& operator<<(std::ostream& os,const MDDSpec& s) {
      os << "Spec(";
      for(int p=0;p < s._nbp;p++) {
         s._attrs[p]->print(os);
         os << ' ';
      }
      os << ')';
      return os;
   }
private:
   void init();
   std::vector<MDDConstraintDescriptor::Ptr> constraints;
   std::vector<var<int>::Ptr> x;
   std::vector<var<int>::Ptr> z;
   std::vector<std::pair<MDDConstraintDescriptor::Ptr,ArcFun>>  _exists;
   std::vector<std::pair<MDDConstraintDescriptor::Ptr,NodeFun>> _nodeExists;
   std::vector<UpdateFun>      _updates; // this is a list of function that applies to every node. 
   std::vector<lambdaTrans> _transition;
   std::vector<lambdaRelax> _relaxation;
   std::vector<lambdaSim>   _similarity;
   std::vector<lambdaTrans> _uptrans;
   std::vector<FixFun>        _onFix;
   std::vector<SplitFun>      _onSplit;
   std::vector<std::vector<lambdaTrans>> _transLayer;
   std::vector<std::vector<lambdaTrans>> _uptransLayer;
   std::vector<LayerDesc> _frameLayer;
   std::vector<std::vector<ArcFun>> _scopedExists; // 1st dimension indexed by varId. 2nd dimension is a list.
   std::vector<std::vector<NodeFun>> _scopedConsistent;
   std::vector<Zone> _upZones;
   std::set<int>      _xRelax;
   std::vector<int>   _dRelax;
};


class MDDSpec;
class MDDStateFactory {
   struct MDDSKey {
      const MDDState*   _s;
      const int         _v;
   };
   struct EQtoMDDSKey {
      bool operator()(const MDDSKey& a,const MDDSKey& b) const noexcept {
         return a._v == b._v && a._s->operator==(*b._s);
      }
   };
   struct HashMDDSKey {
      std::size_t operator()(const MDDSKey& key) const noexcept {
         return key._s->hash() ^ key._v;
      }
   };
   MDDSpec*      _mddspec;
   Pool::Ptr         _mem;
   //std::unordered_map<MDDSKey,MDDState*,HashMDDSKey,EQtoMDDSKey> _hash;
   Hashtable<MDDSKey,MDDState*,HashMDDSKey,EQtoMDDSKey> _hash;
   PoolMark         _mark;
   bool          _enabled;
public:
   MDDStateFactory(MDDSpec* spec);
   void createState(MDDState& result,const MDDState& parent,int layer,const var<int>::Ptr x,const MDDIntSet& vals,bool up);
   bool splitState(MDDState*& result,MDDNode* n,const MDDState& parent,int layer,const var<int>::Ptr x,int val);
   void clear();
   void enable() noexcept { _enabled = true;}
   void disable() noexcept { _enabled = false;}
   unsigned size() const noexcept { return _hash.size();}
};


template <typename Container> std::pair<int,int> domRange(const Container& vars) {
   std::pair<int,int> udom;
   udom.first = INT_MAX;
   udom.second = -INT_MAX;
   for(auto& x : vars){
      udom.first = (udom.first > x->min()) ? x->min() : udom.first;
      udom.second = (udom.second < x->max()) ? x->max() : udom.second;
   }
   return udom;
}  

template <typename Container> std::pair<int,int> idRange(const Container& vars) {
   int low = std::numeric_limits<int>::max();
   int up  = std::numeric_limits<int>::min();
   for(auto& x : vars) {
      low = std::min(low,x->getId());
      up  = std::max(up,x->getId());
   }       
   return std::make_pair(low,up);
}



#endif /* mddstate_hpp */

