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
#include "trailVec.hpp"
#include <set>
#include <cstring>
#include <map>
#include <unordered_map>
#include <bitset>
#include <utility>
#if defined(__x86_64__)
#include <xmmintrin.h>
#endif
#include <limits>
#include "hashtable.hpp"
#define XXH_INLINE_ALL
#include "xxhash.h"

/**
 * @brief Representation for a set of integers.
 *
 * The class treats singletons slightly differently (for speed).
 */
class MDDIntSet {
   short  _mxs,_sz;
   bool  _isSingle;
   union {
      int*      _buf;
      int    _single;
   };
public:
   /**
    * Copy Constructor with allocator
    * @param p the allocator to use
    * @param s the set to copy
    */
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
   /**
    * Default constructor. Creates an empty set.
    */
   MDDIntSet() { _buf = 0;_mxs=_sz=0;_isSingle=false;}
   /**
    * Singleton constructor. It creates a singleton (can't add afterwards)
    * @param v the value in the set
    */
   MDDIntSet(int v) : _mxs(1),_sz(1),_isSingle(true) {
      _single = v;
   }
   /**
    * List constructor. It creates a set from the provided list of values and grabs memory from the given buffer
    * @param buf a pointer to a buffer large enough to accommodate all the values in `val`
    * @param vals the list of values to populate the set.
    * 
    */
   MDDIntSet(char* buf,int mxs,std::initializer_list<int> vals)
      : _mxs(mxs),_sz(0),_isSingle(false) {
      _buf = reinterpret_cast<int*>(buf);
      for(int v : vals) 
         _buf[_sz++] = v;
   }
   /**
    * Basic constructor. It creates an empty set capable to hold up to `mxs` values.
    * @param buf a pointer to a buffer large enough to accommodate `mxs` integers (32-bit wide each)
    * @param mxs the maximum size of the array
    */
   MDDIntSet(char* buf,int mxs) : _mxs(mxs),_sz(0),_isSingle(false) {
      _buf = reinterpret_cast<int*>(buf);
   }
   /**
    * Empties the set of all values.
    */     
   void clear() noexcept { _sz = 0;_isSingle=false;}
   /**
    * Adds a value to the set (not checking for duplicates)
    * @param v the value to add.
    * Notes: the storage must be sufficient to add an element. There is no duplicate check.
    */
   constexpr void add(int v) noexcept {
      assert(_sz < _mxs);
      _buf[_sz++] = v;
   }
   /**
    * Adds a value to the set (if the value is already present, nothing is done)
    * @param v the value to add
    */
   void insert(int v) noexcept {
      if (contains(v)) return;
      if(_sz >= _mxs) {
         std::cerr << "Oops... overflowing MDDIntSet\n";
         exit(1);
      }
      _buf[_sz++] = v;
   }
   /**
    * Checks membership
    * @param v the value to check
    * @return true if and only if \f$v \in this\f$
    */
   constexpr const bool contains(int v) const noexcept {
      if (_isSingle)
         return _single == v;
      else {
         for(short i=0;i < _sz;i++)
            if (_buf[i]==v) return true;
         return false;
      }
   }
   /**
    * Checks whether a member  of this set is *NOT* in S (is therefore outside of S)
    * @param S the set to check against
    * @return \f$ \exists v \in this : v \notin S\f$
    */
   const bool memberOutside(const ValueSet& S) const noexcept {
      if (_isSingle) return !S.member(_single);
      else {
         for(short k=0;k <_sz;k++)
            if (!S.member(_buf[k])) return true;
         return false;
      }
   }
   /**
    * Checks whether *all* members of this set are in S (all inside of S)
    * @param S the set to check against
    * @return \f$ this \subseteq S\f$
    */
   const bool allInside(const ValueSet& S) const noexcept {
      if (_isSingle) return S.member(_single);
      else {
         for(short k=0;k <_sz;k++)
            if (!S.member(_buf[k])) return false;
         return true;
      }
   }
   /**
    * Checks whether there is at least one member of this set in S (one inside of S)
    * @param S the set to check against
    * @return \f$ this \cap S \neq \emptyset\f$
    */
   const bool memberInside(const ValueSet& S) const noexcept {
      if (_isSingle) return S.member(_single);
      else {
         for(short k=0;k <_sz;k++)
            if (S.member(_buf[k])) return true;
         return false;
      }
   }
   /**
    * Compute and returns the smallest element in the set
    * @return the smallest element
    */
   const int min() const noexcept {
      if (_isSingle) return _single;
      else {
         int min = std::numeric_limits<int>::max();
         for(short i=0;i < _sz;++i)
            min = std::min(min,_buf[i]);
         return min;
      }
   }
   /**
    * Compute and returns the largest element in the set
    * @return the largest element
    */
   const int max() const noexcept {
      if (_isSingle) return _single;
      else {
         int max = std::numeric_limits<int>::min();
         for(short i=0;i < _sz;++i)
            max = std::max(max,_buf[i]);
         return max;
      }
   }
   /**
    * Equality test.
    * @param a a set of integers
    * @param b a set of integers
    * @return true if and only if both sets are equal.
    */   
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
   /**
    * Computes a 32-bit hash value for the set.
    * @param a hash
    * Note : This is a very naive hash. (not that is matters much)
    */
   int hash() const noexcept {
      if (_isSingle) return _single;
      else {
         int ttl=0;
         for(int k=0;k < _sz;++k)
            ttl += _buf[k];
         return ttl * _sz;
      }
   }
   /**
    * Returns the number of elements in the set
    * @return number of values in the set
    */
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
   /**
    * @brief C++ STL style iterator.
    *
    * This returns an open range `[begin()..end() )` thas is usable with C++ for loops.
    * One obtains iterator instances with the convenience function begin/end provided below
    */
   class iterator { 
      union {
         int*    _data;
         int     _val;
      };
      short   _num;
      bool    _single;
      iterator(int* d,long num = 0) : _data(d),_num(num),_single(false) {}
      iterator(int v,long num = 0) : _val(v),_num(num),_single(true) {}
   public:
      using iterator_category = std::forward_iterator_tag;
      using value_type = int;
      using difference_type = int;
      using pointer = int*;
      using reference = int&;
      iterator& operator++()   { _num = _num + 1; return *this;}
      iterator operator++(int) { iterator retval = *this; ++(*this); return retval;}
      iterator& operator--()   { _num = _num - 1; return *this;}
      iterator operator--(int) { iterator retval = *this; --(*this); return retval;}
      bool operator==(iterator other) const {return _num == other._num;}
      bool operator!=(iterator other) const {return !(*this == other);}
      int operator*() const   { return _single ? _val : _data[_num];}
      friend class MDDIntSet;
   };
   /** 
    * Iterator pointing to the first element of the set
    * @return the start iterator
    */   
   iterator begin() const { return _isSingle ? iterator(_single,0) : iterator(_buf,0);}
   /** 
    * Iterator pointing just past the last element of the set
    * @return the end iterator
    */   
   iterator end()   const { return _isSingle ? iterator(_single,_sz) : iterator(_buf,_sz);}
   iterator cbegin() const noexcept { return begin();}
   iterator cend()   const noexcept { return end();}
   /**
    * Convenience operator to print a set
    * @param os the stream to print to
    * @param s the set to print
    * @return the stream after printing (for chaining sake)
    */
   friend std::ostream& operator<<(std::ostream& os,const MDDIntSet& s) {
      os << '{';
      for(int v : s)
         os << v << ',';
      return os << '\b' << '}';
   }
};

void printSet(const MDDIntSet& s);

enum RelaxWith { External, MinFun,MaxFun};

struct MDDPack;
class MDDState;
class MDDNode;
typedef std::function<bool(const MDDPack&)> NodeFun;
typedef std::function<bool(const MDDPack&,const MDDPack&,const var<int>::Ptr&,int)> ArcFun;
typedef std::function<void(const MDDPack&)> FixFun;
typedef std::function<void(MDDState&,const MDDPack&)> UpdateFun;
typedef std::function<void(MDDState&,const MDDPack&,const var<int>::Ptr&,const MDDIntSet&)> lambdaTrans;
typedef std::function<void(MDDState&,const MDDState&,const MDDState&)> lambdaRelax;
typedef std::function<double(const MDDState&,const MDDState&)> lambdaSim;
typedef std::function<double(const MDDNode&)> SplitFun;
typedef std::function<double(const MDDState&, void*, int)> CandidateFun;
typedef std::function<int(const MDDState&,const MDDState&)> EquivalenceValueFun;
//typedef std::function<int*(TVec<MDDNode*>*)> ValueScoreFun;
typedef std::function<int(TVec<MDDNode*>*)> BestValueFun;
class MDDStateSpec;

/**
 * @brief Memory zone descriptor
 *
 * Instances of this class are used to describe consecutive "blocks" of memory  within MDDState instances.
 * Internal use only.
 */
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

/**
 * @brief MDD Constraint Descriptor class.
 *
 * Each MDD LTS added to a propagator is associated to one such descriptor.
 * Its purpose is to track everything to model this LTS in the propagator.
 */
class MDDCstrDesc {
   Factory::Veci          _vars;
   ValueSet               _vset;
   const char*            _name;
   std::vector<int> _propertiesDown;
   std::vector<int> _propertiesUp;
   std::vector<int> _propertiesCombined;
   std::vector<int> _downTId; // transition ids
   std::vector<int> _upTId; // up transition ids
   std::vector<int> _downRId; // relaxation ids
   std::vector<int> _upRId; // relaxation ids
   std::vector<int> _uid; // update ids
   std::vector<int> _sid; // similarity ids
public:
   typedef handle_ptr<MDDCstrDesc> Ptr;
   template <class Vec> MDDCstrDesc(const Vec& vars, const char* name)
      : _vars(vars.size(),Factory::alloci(vars[0]->getStore())),
        _vset(vars),
        _name(name)
   {
      for(typename Vec::size_type i=0;i < vars.size();i++)
         _vars[i] = vars[i];
   }
   MDDCstrDesc(const MDDCstrDesc&);
   void addDownProperty(int p) {_propertiesDown.push_back(p);}
   void addUpProperty(int p) {_propertiesUp.push_back(p);}
   void addCombinedProperty(int p) {_propertiesCombined.push_back(p);}
   bool ownsDownProperty(int p) const;
   bool ownsUpProperty(int p) const;
   bool ownsCombinedProperty(int p) const;
   const std::vector<int>& downTransitions() const  { return _downTId;}
   const std::vector<int>& upTransitions() const    { return _upTId;}
   const std::vector<int>& downRelaxations() const  { return _downRId;}
   const std::vector<int>& upRelaxations() const    { return _upRId;}
   const std::vector<int>& similarities() const     { return _sid;}
   void registerDown(int t)                         { _downTId.emplace_back(t);}
   void registerUp(int t)                           { _upTId.emplace_back(t);}
   void registerDownRelaxation(int t)               { _downRId.emplace_back(t);}
   void registerUpRelaxation(int t)                 { _upRId.emplace_back(t);}
   void registerUpdate(int t)                       { _uid.emplace_back(t);}
   void registerSimilarity(int t)                   { _sid.emplace_back(t);}
   bool inScope(const var<int>::Ptr& x) const noexcept { return _vset.member(x->getId());}
   const Factory::Veci& vars() const      { return _vars;}
   std::vector<int>& propertiesDown()     { return _propertiesDown;}
   std::vector<int>& propertiesUp()       { return _propertiesUp;}
   std::vector<int>& propertiesCombined() { return _propertiesCombined;}
   auto downBegin() { return _propertiesDown.begin();}
   auto downEnd()   { return _propertiesDown.end();}
   auto upBegin() { return _propertiesUp.begin();}
   auto upEnd()   { return _propertiesUp.end();}
   auto combinedBegin() { return _propertiesCombined.begin();}
   auto combinedEnd()   { return _propertiesCombined.end();}
   void print (std::ostream& os) const { os << _name << "(" << _vars << ")\n";}
};

/**
 * @brief A Bit Sequence _value_. 
 *
 * This class is used to represent values in MDDState that hold a 0-based sequence of bits (of a given length)
 * @see MDDState, MDDProperty, MDDPBitSequence
 */ 
class MDDBSValue {
   unsigned long long* _buf;
   const  short        _nbw;
public:
   MDDBSValue(char* buf,short nbw)
      : _buf((unsigned long long*)(buf)),_nbw(nbw) {}
   MDDBSValue(const MDDBSValue& v)
      : _buf(v._buf),_nbw(v._nbw) {}
   MDDBSValue(MDDBSValue&& v) : _buf(v._buf),_nbw(v._nbw) { v._buf = nullptr;}
   short nbWords() const noexcept { return _nbw;}
   MDDBSValue& operator=(const MDDBSValue& v) noexcept {
      for(int i=0;i <_nbw;++i)
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
      return nbb;
   }
   __attribute__((always_inline)) inline MDDBSValue& setBinOR(const MDDBSValue& a,const MDDBSValue& b) noexcept {
      switch(_nbw) {
         case 1: _buf[0] = a._buf[0] | b._buf[0];return *this;
#if defined(__x86_64__)
         case 2: {
            __m128i p0 = *(__m128i*) a._buf;
            __m128i p1 = *(__m128i*) b._buf;
            *(__m128i*)_buf = _mm_or_si128(p0,p1);
         } return *this;
#endif
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
   class iterator { 
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
      using iterator_category = std::forward_iterator_tag;
      using value_type = short;
      using difference_type = short;
      using pointer = short*;
      using reference = short&;
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
      for(short i = 0 ;eq && i < a._nbw;++i)
         eq = a._buf[i] == b._buf[i];
      return eq;
   }
   friend bool operator!=(const MDDBSValue& a,const MDDBSValue& b) {
      bool eq = a._nbw == b._nbw;
      for(short i = 0 ;eq && i < a._nbw;++i)
         eq = a._buf[i] == b._buf[i];
      return !eq;
   }
};

/**
 * @brief A sliding window _value_. 
 *
 * This class is used to represent values in MDDState
 * @see MDDState, MDDProperty, MDDPSWindow<T>
 */ 
template <class ET>
class MDDSWin {
   ET* _buf;
   int _nb;
public:
   MDDSWin(ET* buf,int nb) : _buf(buf),_nb(nb) {}
   MDDSWin(MDDSWin<ET>&& v) : _buf(v._buf),_nb(v._nb) {}
   MDDSWin(const MDDSWin<short>& v) : _buf(v._buf),_nb(v._nb) {}
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

/**
 * Indicate the direction for a state.
 */
enum Direction { None=0,Down=1,Up=2,Bi=3};

/**
 * @brief Abstract class representing a property to be held in an MDDState
 *
 * It is subclassed for each type of property one could store in an MDD.
 * The abstract class only handles very generic aspect such as storage size and layout along with remembering
 * how to relax  values of this property.
 * @see MDDPInt, MDDPBit, MDDPByte, MDDPBitSequence, MDDPSWin<T> 
 */ 
class MDDProperty {
protected:
   int _id;
   unsigned int _ofs; // offset in bytes within block
   unsigned short _bsz; // size in bytes.
   enum Direction _dir;
   enum RelaxWith _rw;
   std::set<int>  _sp1; // if property is down or combined, sp1 is down antecedent.  if property is up, sp1 is up
   std::set<int>  _sp2; // if property is down or up, sp2 is combined antecedent.  if property is combined, sp2 is up
   int            _tid;
   int            _rid;
   virtual size_t storageSize() const = 0;  // given in _bits_
   virtual size_t setOffset(size_t bitOffset) = 0;
public:
   typedef handle_ptr<MDDProperty> Ptr;
   MDDProperty(const MDDProperty& p)
      : _id(p._id),_ofs(p._ofs),_bsz(p._bsz),_dir(p._dir),_rw(p._rw),_tid(p._tid),_rid(p._rid) {}
   MDDProperty(MDDProperty&& p)
      : _id(p._id),_ofs(p._ofs),_bsz(p._bsz),_dir(p._dir),_rw(p._rw),_tid(p._tid),_rid(p._rid) {}
   MDDProperty(int id,unsigned int ofs,unsigned short bsz,enum RelaxWith rw = External)
      : _id(id),_ofs(ofs),_bsz(bsz),_dir(Down),_rw(rw) { _tid = -1;_rid = -1;}
   MDDProperty& operator=(const MDDProperty& p) {
      _id = p._id;_ofs = p._ofs; _bsz = p._bsz;_dir = p._dir;_rw = p._rw;
      _tid = p._tid;
      _rid = p._rid;
      return *this;
   }
   const int getId() const { return _id;}
   enum RelaxWith relaxFun() const noexcept { return _rw;}
   unsigned int startOfs() const noexcept { return _ofs;}
   unsigned int endOfs() const noexcept   { return _ofs + _bsz;}
   size_t size() const noexcept { return storageSize() >> 3;}
   void setDirection(enum Direction d) { _dir = d;}
   void setAntecedents(const std::set<int>& sp,const std::set<int>& sp2) { _sp1 = sp; _sp2 = sp2; }
   void setTransition(int tid) noexcept { _tid = tid;}
   void setRelax(int rid) noexcept { _rid = rid;}
   int getTransition() const noexcept    { return _tid;}
   int getRelax() const noexcept   { return _rid;}
   const std::set<int>& antecedents() const { return _sp1;}
   const std::set<int>& antecedentsSecondary() const { return _sp2;}
   bool isUp() const noexcept                                 { return (_dir & Up) == Up;}
   bool isDown() const noexcept                               { return (_dir & Down) == Down;}
   virtual void init(char* buf) const noexcept                {}
   virtual void minWith(char* buf,char* other) const noexcept {}
   virtual void maxWith(char* buf,char* other) const noexcept {}
   virtual bool diff(char* buf,char* other) const noexcept    { return false;}
   int getByte(char* buf) const noexcept                      { return buf[_ofs];}
   MDDBSValue getBS(char* buf) const noexcept                 { return MDDBSValue(buf + _ofs,_bsz >> 3);}
   template <class ET>
   MDDSWin<ET> getSW(char* buf) const noexcept                { return MDDSWin<ET>(reinterpret_cast<short*>(buf + _ofs),_bsz / sizeof(ET));}
   void setInt(char* buf,int v) const noexcept                { *reinterpret_cast<int*>(buf+_ofs) = v;}
   void setByte(char* buf,char v) const noexcept              { buf[_ofs] = v;}
   MDDBSValue setBS(char* buf,const MDDBSValue& v) const noexcept {
      MDDBSValue dest(buf + _ofs,_bsz >> 3);
      dest = v;
      return dest;
   }
   virtual void setProp(char* buf,char* from) const noexcept {
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

/**
 * @brief MDD State 32-bit Signed Integer Property
 *
 * This is a concrete proprety that refers to a specific integer property within an MDD State.
 * Since an MDD State is a block of bytes, a property _value_ is held at a specific offset within
 * this block of bytes. The `_ofs` attribute inherited from MDDProperty holds the offset.
 * The class provides a few methods to read/write to this relative location within an MDDState.
 * Those are use *internally* only and the user-level API is more abstract (but uses those).
 * @see MDDPropValue<MDDPInt>
 */
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
   MDDPInt(int id,unsigned int ofs,int init,int max,enum RelaxWith rw)
      : MDDProperty(id,ofs,4,rw),_init(init),_max(max) {}
   void init(char* buf) const noexcept override      { *reinterpret_cast<int*>(buf+_ofs) = _init;}
   int getInt(char* buf) const noexcept                    { return *reinterpret_cast<int*>(buf+_ofs);}
   void setInt(char* buf,int v) const noexcept             { *reinterpret_cast<int*>(buf+_ofs) = v;}
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

/**
 * @brief MDD State 1-bit Boolean Property
 *
 * This is a concrete proprety that refers to a specific bit property within an MDD State.
 * Since an MDD State is a block of bytes, a property _value_ is held at a specific offset within
 * this block of bytes. The `_ofs` attribute inherited from MDDProperty holds the offset.
 * The class provides a few methods to read/write to this relative location within an MDDState.
 * Those are use *internally* only and the user-level API is more abstract (but uses those).
 * @see MDDPropValue<MDDPBit>
 */
class MDDPBit :public MDDProperty {
   bool _init;
   char _bitmask;
   size_t storageSize() const override { return 1;}
   size_t setOffset(size_t bitOffset) override {
      size_t boW = bitOffset & 0x7;
      _bitmask = 0x1 << boW;
      _ofs = bitOffset >> 3;
      return bitOffset + storageSize();
   }
public:
   typedef handle_ptr<MDDPBit> Ptr;
   MDDPBit(int id,unsigned int ofs,int init,enum RelaxWith rw)
      : MDDProperty(id,ofs,1,rw),_init(init) {}
   void init(char* buf) const  noexcept override     {
      if (_init)
         buf[_ofs] |= _bitmask;
      else
         buf[_ofs] &= ~_bitmask;
   }
   int getInt(char* buf) const  noexcept  { return (buf[_ofs] & _bitmask) == _bitmask;}
   int getBit(char* buf) const  noexcept  { return (buf[_ofs] & _bitmask) == _bitmask;}
   void setBit(char* buf,int v) const noexcept {
      if (v)
         buf[_ofs] |= _bitmask;
      else
         buf[_ofs] &= ~_bitmask;
   }
   void stream(char* buf,std::ostream& os) const override { bool v = buf[_ofs] & _bitmask;os << (int)v;}
   void minWith(char* buf,char* other) const noexcept override {
      if (!(buf[_ofs] & other[_ofs] &  _bitmask))
        buf[_ofs] &= ~_bitmask;
   }
   void maxWith(char* buf,char* other) const noexcept override {
      buf[_ofs] |= (other[_ofs] & _bitmask);
   }
   bool diff(char* buf,char* other) const noexcept  override {
      return (buf[_ofs] & _bitmask) != (other[_ofs] & _bitmask);
   }
   void setProp(char* buf,char* from) const noexcept override {
      if ((from[_ofs] & _bitmask) == _bitmask)
         buf[_ofs] |= _bitmask;
      else
         buf[_ofs] &= ~_bitmask;
   }
   void print(std::ostream& os) const override  {
      os << "PBit(" << _id << ',' << _ofs << ',' << (int)_bitmask << ',' << (int)_init << ')';
   }
   friend class MDDStateSpec;
};

/**
 * @brief MDD State 8-bit signed integer Property
 *
 * This is a concrete proprety that refers to a specific bit property within an MDD State.
 * Since an MDD State is a block of bytes, a property _value_ is held at a specific offset within
 * this block of bytes. The `_ofs` attribute inherited from MDDProperty holds the offset.
 * The class provides a few methods to read/write to this relative location within an MDDState.
 * Those are use *internally* only and the user-level API is more abstract (but uses those).
 * @see MDDPropValue<MDDPByte>
 */
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
   MDDPByte(int id,unsigned int ofs,char init,char max,enum RelaxWith rw)
      : MDDProperty(id,ofs,1,rw),_init(init),_max(max) {}
   void init(char* buf) const  noexcept override     { buf[_ofs] = _init;}
   int getByte(char* buf) const  noexcept               { return buf[_ofs];}
   void setByte(char* buf,int v) const noexcept        { buf[_ofs] = (char)v;}
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

/**
 * @brief MDD State bit sequence Property
 *
 * This is a concrete proprety that refers to a specific bit sequence property within an MDD State.
 * Since an MDD State is a block of bytes, a property _value_ is held at a specific offset within
 * this block of bytes. The `_ofs` attribute inherited from MDDProperty holds the offset.
 * The class provides a few methods to read/write to this relative location within an MDDState.
 * Those are use *internally* only and the user-level API is more abstract (but uses those).
 * @see MDDPropValue<MDDPBitSequence>
 */
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
      size_t boW = bitOffset & 0x1F;
      if (boW != 0) 
         bitOffset = (bitOffset | 0x1F) + 1;
      _ofs = bitOffset >> 3;
      _ofs = ((_ofs & 0xF) != 0)  ? (_ofs | 0xF)+1 : _ofs; // 16-byte align
      return (_ofs << 3) + storageSize();
   }
 public:
   typedef handle_ptr<MDDPBitSequence> Ptr;
   MDDPBitSequence(int id,unsigned int ofs,int nbbits,unsigned char init,enum RelaxWith rw) // init = 0 | 1
      : MDDProperty(id,ofs,8 * ((nbbits % 64) ? nbbits/64 + 1 : nbbits/64),rw),_nbBits(nbbits),_init(init)
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
   void minWith(char* buf,char* other) const noexcept override {
      auto a = getBS(buf);
      auto b = getBS(other);
      a.setBinOR(a,b);
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

/**
 * @brief MDD State Sliding Window Properties
 *
 * This is a concrete proprety that refers to a specific sliding window property within an MDD State.
 * Since an MDD State is a block of bytes, a property _value_ is held at a specific offset within
 * this block of bytes. The `_ofs` attribute inherited from MDDProperty holds the offset.
 * The class provides a few methods to read/write to this relative location within an MDDState.
 * Those are use *internally* only and the user-level API is more abstract (but uses those).
 * @see MDDPropValue<MDDPSWindow<short>>
 */
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
   typedef handle_ptr<MDDPSWindow<ET>> Ptr;
   MDDPSWindow(int id,unsigned int ofs,int len,ET eInit,ET fInit,enum RelaxWith rw)
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

/**
 * @brief This is the specification of an MDDState
 *
 * A state is <Down[,Up][,Combined]> (Both up and combined are optional, all three are MDDState instances)
 * The class holds all the necessary attribute to compute all the properties via
 * transitions and relaxations for any part of the state. It is intimately related
 * to the MDDState and the properties they hold.
 * @see MDDState, MDDProperty
 */
class MDDStateSpec {
protected:
   MDDProperty::Ptr* _attrsDown;
   MDDProperty::Ptr* _attrsUp;
   MDDProperty::Ptr* _attrsCombined;
   MDDPropSet*   _omapDown;
   MDDPropSet*   _omapUp;
   MDDPropSet*   _omapDownToCombined;
   MDDPropSet*   _omapUpToCombined;
   MDDPropSet*   _omapCombinedToDown;
   MDDPropSet*   _omapCombinedToUp;
   size_t _mxpDown;
   size_t _mxpUp;
   size_t _mxpCombined;
   size_t _nbpDown;
   size_t _nbpUp;
   size_t _nbpCombined;
   size_t _lszDown;
   size_t _lszUp;
   size_t _lszCombined;
   enum RelaxWith _relax;
   size_t _width;
   void addDownProperty(MDDProperty::Ptr p) noexcept;
   void addUpProperty(MDDProperty::Ptr p) noexcept;
   void addCombinedProperty(MDDProperty::Ptr p) noexcept;
public:
   MDDStateSpec();
   void setWidth(size_t w) { _width = w;}
   size_t getWidth() const { return _width;}
   const auto layoutSizeDown() const noexcept { return _lszDown;}
   const auto layoutSizeUp() const noexcept { return _lszUp;}
   const auto layoutSizeCombined() const noexcept { return _lszCombined;}
   const auto layoutSize(enum Direction dir) const noexcept {
      switch (dir) {
         case Down: return layoutSizeDown(); break;
         case Up: return layoutSizeUp(); break;
         case Bi: return layoutSizeCombined(); break;
         default: return (size_t)-1; break;
      }
   }
   void layout();
   virtual void varOrder() {}
   auto sizeDown() const noexcept { return _nbpDown;}
   auto sizeUp() const noexcept { return _nbpUp;}
   auto sizeCombined() const noexcept { return _nbpCombined;}
   auto size(enum Direction dir) const noexcept {
      switch (dir) {
         case Down: return sizeDown(); break;
         case Up: return sizeUp(); break;
         case Bi: return sizeCombined(); break;
         default: return (size_t)-1; break;
      }
   }
   unsigned int startOfsDown(int p) const noexcept { return _attrsDown[p]->startOfs();}
   unsigned int startOfsUp(int p) const noexcept { return _attrsUp[p]->startOfs();}
   unsigned int endOfsDown(int p) const noexcept { return _attrsDown[p]->endOfs();}
   unsigned int endOfsUp(int p) const noexcept { return _attrsUp[p]->endOfs();}
   virtual MDDPByte::Ptr downByteState(MDDCstrDesc::Ptr d, int init,int max,enum RelaxWith rw=External, int cPriority=0, bool restrictedReducedInclude=true);
   virtual MDDPInt::Ptr downIntState(MDDCstrDesc::Ptr d, int init,int max,enum RelaxWith rw=External, int cPriority=0, bool restrictedReducedInclude=true);
   virtual MDDPBitSequence::Ptr downBSState(MDDCstrDesc::Ptr d,int nbb,unsigned char init,enum RelaxWith rw=External,int cPriority=0, bool restrictedReducedInclude=true);
   virtual MDDPSWindow<short>::Ptr downSWState(MDDCstrDesc::Ptr d,int len,int init,int finit,enum RelaxWith rw=External, int cPriority=0, bool restrictedReducedInclude=true);
   virtual MDDPByte::Ptr upByteState(MDDCstrDesc::Ptr d,int init,int max,enum RelaxWith rw=External,int cPriority=0);
   virtual MDDPInt::Ptr upIntState(MDDCstrDesc::Ptr d,int init,int max,enum RelaxWith rw=External,int cPriority=0);
   virtual MDDPBitSequence::Ptr upBSState(MDDCstrDesc::Ptr d,int nbb,unsigned char init,enum RelaxWith rw=External, int cPriority=0);
   virtual MDDPSWindow<short>::Ptr upSWState(MDDCstrDesc::Ptr d,int len,int init,int finit,enum RelaxWith rw=External, int cPriority=0);
   virtual MDDPByte::Ptr combinedByteState(MDDCstrDesc::Ptr d,int init,int max,enum RelaxWith rw=External,int cPriority = 0);   
   virtual MDDPInt::Ptr combinedIntState(MDDCstrDesc::Ptr d,int init,int max,enum RelaxWith rw=External,int cPriority = 0);   
   virtual MDDPBitSequence::Ptr combinedBSState(MDDCstrDesc::Ptr d,int nbb,unsigned char init,enum RelaxWith rw = External,int cPriority=0);
   virtual MDDPSWindow<short>::Ptr combinedSWState(MDDCstrDesc::Ptr d,int len,int init,int finit,enum RelaxWith rw=External, int cPriority=0);

   MDDPInt::Ptr intPropUp(int p) { return (MDDPInt*)_attrsUp[p].get();}
   
   void outputSetDown(MDDPropSet& out,const MDDPropSet& down,const MDDPropSet& combined) const noexcept {
      for(auto p : down)
         out.unionWith(_omapDown[p]);
      if (_nbpCombined)
         for(auto p : combined)
            out.unionWith(_omapCombinedToDown[p]);
   }
   void outputSetUp(MDDPropSet& out,const MDDPropSet& up,const MDDPropSet& combined) const noexcept {
      for(auto p : up)
         out.unionWith(_omapUp[p]);
      if (_nbpCombined)
         for(auto p : combined) {
            out.unionWith(_omapCombinedToUp[p]);
      }
   }
   void outputSetCombinedFromDown(MDDPropSet& out,const MDDPropSet& down) const noexcept {
      for(auto p : down)
         out.unionWith(_omapDownToCombined[p]);
   }
   void outputSetCombinedFromUp(MDDPropSet& out,const MDDPropSet& up) const noexcept {
      for(auto p : up)
         out.unionWith(_omapUpToCombined[p]);
   }
   virtual size_t nodeUB() const = 0;
   friend class MDDState;
   friend class MDDDelta;
   void printState(std::ostream& os,const MDDState* sPtr) const noexcept;    
};

inline std::ostream& operator<<(std::ostream& os,MDDProperty::Ptr p)
{
   p->print(os);return os;
}

inline std::ostream& operator<<(std::ostream& os,MDDCstrDesc& p)
{
   p.print(os);return os;
}

/**
 * @brief Abstract generic property value class.
 *
 * Template specializations provide concrete classes for concrete property types.
 */
template <typename Prop> class MDDPropValue {};

template <> class MDDPropValue<MDDPBitSequence> {
   MDDBSValue         _lhs;
   MDDPropValue<MDDPBitSequence>(MDDPBitSequence::Ptr p,char* mem) : _lhs(p->getBS(mem)) {}
public:
   MDDPropValue<MDDPBitSequence>& operator=(const MDDPropValue<MDDPBitSequence>& v) noexcept {
      _lhs = v._lhs; // this invokes the assignment operator on MDDBSValue that copies the bits to the destination
      return *this;
   }
   operator MDDBSValue() const { return _lhs;} // this invokes the copy constructor (only copies the shell)
   void set(int k)             { _lhs.set(k);}
   constexpr bool getBit(const int ofs) const noexcept { return _lhs.getBit(ofs);}
   constexpr int cardinality() const noexcept { return _lhs.cardinality();}
   inline void setBinOR(const MDDBSValue& a,const MDDBSValue& b) noexcept  { _lhs.setBinOR(a,b);}
   inline void setBinAND(const MDDBSValue& a,const MDDBSValue& b) noexcept { _lhs.setBinAND(a,b);}
   friend class MDDState;
};

/**
 * @brief Property value for a signed 32-bit integer
 *
 * One obtains an instance of this type from an MDDState.
 * It has the necessary methods to safely _read_ and _write_ to the MDDState. Instances and methods of this class
 * are used to implement transitions, relaxations and arc/node existence functions. They are the top-level end-user API.
 * Note how one necessarily obtains a _constant_ MDDPropValue<T> by calling the []  operator on an MDDState. If the
 * state happens to be constant, one obtains a constant MDDPropValue<T> that can only be used to _read_ the state.
 * If, however, the state is mutable, then the MDDPropValue<T> is also mutable and can be use to _write_ to the state.
 *
 * For instance, consider this code in the among LTS
 * ```
 *     mdd.arcExist(d,[=] (const auto& parent,const auto& child,auto,const auto& val) {
 *        bool vinS = values.member(val);
 *        return (parent.down[minC] + vinS <= ub) &&
 *              ((parent.down[maxC] + vinS +  parent.down[rem] - 1) >= lb);
 *     });
 * ```
 * Note how the arc existence receives the parent,child and the value `val` on the arc.
 * The `vinS` variable check whether the variable on the arc is one of the value among authorizes.
 * Finally, the arc existence reads properties with the [] operator on the down state of the parent (`parent.down`)
 * and accesses `minC`, `maxC` and `rem`  to validate against the lower (`lb`) and upper (`ub`)
 * bounds. Transitions would _write_ to the properties as shown below
 *
 * ```
 *      mdd.transitionDown(d,maxC,{maxC},{},[maxC,tv] (auto& out,const auto& parent,const auto&, const auto& val) {
 *        out[maxC] = parent.down[maxC] + val.contains(tv);
 *     });
 * ```
 * Indeed, `out` is the output (the down state of the child) while `parent` is the parent node and `val` the set
 * of labels on arcs leading to this node. Clearly, the `out[maxC]` on the left-hand side accesses the `maxC`
 * property and then uses the assignment operator to set a new value based on the expression on the right-hand side
 * which _reads_ from the `maxC` property (`parent.down[maxC]`).
 * 
 * @see MDDState::operator[](MDDPInt::Ptr), MDDState::operator[](MDDPInt::Ptr) const
 */
template <> class MDDPropValue<MDDPInt> {
   MDDPInt::Ptr  _p;
   char*       _mem;
   MDDPropValue<MDDPInt>(MDDPInt::Ptr p,char* mem): _p(p),_mem(mem) {}
public:
   /**
    * @brief Assignment operator to write the value held in another property into this location.
    * @param v the property to read from
    * @return ourselve
    */
   MDDPropValue<MDDPInt>& operator=(const MDDPropValue<MDDPInt>& v) noexcept { _p->setInt(_mem,v);return *this;}
   /**
    * @brief Assignment operator to write the value `v` into this one.
    * @param v the value to write into this location
    * @return ourselve
    */
   MDDPropValue<MDDPInt>& operator=(const int& v) noexcept { _p->setInt(_mem,v);return *this;}
   /**
    * @brief Casting operator to _read_ the value held at the location denoted by this class.
    * @param value held in the MDDState at this location
    */
   operator int() const { return _p->getInt(_mem);}
   /**
    * The MDDState must befriend this class to call its constructor.
    * @see MDDState::operator[](MDDPInt::Ptr), MDDState::operator[](MDDPInt::Ptr) const
    */
   friend class MDDState;
};

template <> class MDDPropValue<MDDPByte> {
   MDDPByte::Ptr  _p;
   char*        _mem;
   MDDPropValue<MDDPByte>(MDDPByte::Ptr p,char* mem): _p(p),_mem(mem) {}
public:
   MDDPropValue<MDDPByte>& operator=(const MDDPropValue<MDDPByte>& v) noexcept { _p->setInt(_mem,v);return *this;}
   MDDPropValue<MDDPByte>& operator=(const char v) noexcept { _p->setByte(_mem,v);return *this;}
   bool operator==(char v) const noexcept { return _p->getByte(_mem) == v;}
   bool operator==(int v) const noexcept  { return _p->getByte(_mem) == (char)v;}   
   operator int()  const { return _p->getByte(_mem);}
   friend int operator+(const MDDPropValue<MDDPByte>& p,int v) { return p.operator int() + v;}
   friend class MDDState;
};

template <> class MDDPropValue<MDDPSWindow<short>> {
   MDDSWin<short>         _lhs;
   MDDPropValue<MDDPSWindow<short>>(MDDPSWindow<short>::Ptr p,char* mem)
      : _lhs(p->getSW<short>(mem)) {}
public:
   MDDPropValue<MDDPSWindow<short>>& operator=(const MDDPropValue<MDDPSWindow<short>>& v) noexcept {
      _lhs = v._lhs;
      return *this;
   }
   operator MDDSWin<short>() const { return _lhs;}
   short get(int ofs) const noexcept { return _lhs.get(ofs);}
   short last() const noexcept  { return _lhs.last();}
   short first() const noexcept { return _lhs.first();}
   friend class MDDState;
};

class MDDState {  // An actual state of an MDDNode.
   MDDStateSpec*     _spec;
   char*              _mem;
   mutable int       _hash;
   int  _magic;
   enum Direction     _dir;        // State knows whether up / down / combined
   struct Flags {
      bool           _relax:1;
      mutable bool  _hashed:1;
      bool          _unused:4;
   } _flags;
   class TrailState : public Entry {
      MDDState* _at;
      char*   _from;
      int       _sz;
      Flags  _f;
   public:
      TrailState(MDDState* at,char* from,int sz) : _at(at),_from(from),_sz(sz)
      {
         memcpy(_from,_at->_mem,_sz);
         _f = _at->_flags;
      }
      void restore() noexcept override {
         memcpy(_at->_mem,_from,_sz);
         _at->_flags = _f;
      }
   };
public:
   MDDState(Trailer::Ptr trail,MDDStateSpec* s,char* b,enum Direction dir,bool relax=false,bool unused=false) 
      : _spec(s),_mem(b),_hash(0),_magic(trail->magic()),_dir(dir),_flags({relax,false,unused})
   {
      memset(b,0,_spec->layoutSize(_dir));
   }
   MDDState(Trailer::Ptr trail,MDDStateSpec* s,char* b,int hash,enum Direction dir,const Flags& f) 
      : _spec(s),_mem(b),_hash(hash),_magic(trail->magic()),_dir(dir),_flags(f)
   {}
   MDDState(const MDDState& s) 
      : _spec(s._spec),_mem(s._mem),_hash(s._hash),_magic(s._magic),_dir(s._dir),_flags(s._flags) {}
   void initState(const MDDState& s) {
      _spec = s._spec;
      _mem = s._mem;
      _flags = s._flags;
      _hash = s._hash;
      _magic = s._magic;
      _dir = s._dir;
   }
   void copyState(const MDDState& s) {
      auto sz = _spec->layoutSize(_dir);
      memcpy(_mem,s._mem,sz);
      _flags = s._flags;
      _hash = s._hash;
      _dir = s._dir;
      _magic = s._magic;
   }
   MDDStateSpec* getSpec() const noexcept { return _spec;}
   MDDState& assign(const MDDState& s,Trailer::Ptr t,Storage::Ptr mem) {
      auto sz = _spec->layoutSize(_dir);
      if (sz) {
         if (_magic != t->magic()) {
            char* block = (char*)mem->allocate(sizeof(char)* sz);
            t->trail(new (t) TrailState(this,block,(int)sz));
            _magic = t->magic();
         }
         assert(_mem != nullptr);
         memcpy(_mem,s._mem,sz);
      }
      assert(_spec == s._spec || _spec == nullptr);
      _spec = s._spec;
      _flags = s._flags;
      _hash = s._hash;
      _dir = s._dir;
      return *this;
   }
   MDDState& operator=(MDDState&& s) {
      assert(_spec == s._spec || _spec == nullptr);
      _spec = std::move(s._spec);
      _mem  = std::move(s._mem);
      _hash = std::move(s._hash);
      _flags = std::move(s._flags);
      _dir = std::move(s._dir);
      _magic = std::move(s._magic);
      return *this;
   }
   template <class Allocator>
   MDDState clone(Trailer::Ptr trail,Allocator pool) const {
      const size_t sz = _spec ? _spec->layoutSize(_dir) : 0;
      char* block = sz ? new (pool) char[sz] : nullptr;
      if (sz)  memcpy(block,_mem,sz);
      return MDDState(trail,_spec,block,_hash,_dir,_flags);
   }
   bool unused() const noexcept        { return _flags._unused; }
   bool valid() const noexcept         { return _mem != nullptr;}
   auto layoutSize() const noexcept    { return _spec->layoutSize(_dir);}
   void init(MDDProperty::Ptr i) const  noexcept                   { i->init(_mem); }
   auto operator[](MDDPBitSequence::Ptr i) noexcept                { return MDDPropValue<MDDPBitSequence>(i,_mem);}
   const auto operator[](MDDPBitSequence::Ptr i) const noexcept    { return MDDPropValue<MDDPBitSequence>(i,_mem);}
   /**
    * @brief Mutable access operator.
    * @param i an integer property
    * @return an instance of MDDPropValue<MDDPInt> referring to property `i` within this state.
    */
   auto operator[](MDDPInt::Ptr i) noexcept                        { return MDDPropValue<MDDPInt>(i,_mem);}
   /**
    * @brief Immutable access operator.
    * @param i an integer property
    * @return a constant instance of MDDPropValue<MDDPInt> referring to property `i` within this state.
    */
   const auto operator[](MDDPInt::Ptr i) const noexcept            { return MDDPropValue<MDDPInt>(i,_mem);}
   auto operator[](MDDPByte::Ptr i) noexcept                       { return MDDPropValue<MDDPByte>(i,_mem);}
   const auto operator[](MDDPByte::Ptr i) const noexcept           { return MDDPropValue<MDDPByte>(i,_mem);}
   auto operator[](MDDPSWindow<short>::Ptr i) noexcept             { return MDDPropValue<MDDPSWindow<short>>(i,_mem);}
   const auto operator[](MDDPSWindow<short>::Ptr i) const noexcept { return MDDPropValue<MDDPSWindow<short>>(i,_mem);}
      
   void setProp(MDDProperty::Ptr i,const MDDState& from) noexcept  { i->setProp(_mem,from._mem); } // (fast)
   void copyZone(const Zone& z,const MDDState& in) noexcept        { z.copy(_mem,in._mem);}
   void clear() noexcept                { _flags._relax = false;_flags._hashed=false;}
   void zero() noexcept                 { memset(_mem, 0, _spec->layoutSize(_dir)); }
   bool isRelaxed() const noexcept      { return _flags._relax;}
   void relax(bool r = true) noexcept   { _flags._relax = r;}
   int hash() const noexcept {
      if (_flags._hashed)
         return _hash;
      return computeHash();
   }
   int computeHash() const noexcept {
      //xxh::hash_t<64> hv = xxh::xxhash<64>(_mem,_spec->layoutSize(_dir));
      long long hv = XXH3_64bits(_mem,_spec->layoutSize(_dir));
      _flags._hashed = true;
      return _hash = (int)hv;
   }
   bool stateEqual(const MDDState& b) const noexcept {
      return memcmp(_mem,b._mem,_spec->layoutSize(_dir))==0;
   }
   bool stateChange(const MDDState& b) const noexcept {
      return memcmp(_mem,b._mem,_spec->layoutSize(_dir))!=0;
   }
   bool operator==(const MDDState& s) const {
      return (_flags._unused && s._flags._unused) || ((!_flags._hashed || !s._flags._hashed || _hash == s._hash) &&
         _flags._relax == s._flags._relax &&
         memcmp(_mem,s._mem,_spec->layoutSize(_dir))==0);
   }
   bool operator!=(const MDDState& s) const {
      return ! this->operator==(s);
   }
   friend std::ostream& operator<<(std::ostream& os,const MDDState& s) {
      s._spec->printState(os,&s);
      return os;
   }
   friend class MDDStateSpec;
   friend class MDDSpec;
   friend class MDDDelta;
};

struct MDDPack {
   MDDState& down;
   MDDState& up;
   MDDState& comb;
   MDDPack(MDDState& d,MDDState& u,MDDState& c) : down(d),up(u),comb(c) {}
};


class MDDSpec;
class LayerDesc {
   std::vector<int> _dframe;
   std::vector<int> _uframe;
   std::vector<Zone> _dzones;
   std::vector<Zone> _uzones;
   MDDPropSet         _dprop;
   MDDPropSet         _uprop;
public:
   LayerDesc(int nbpDown, int nbpUp) : _dprop(nbpDown), _uprop(nbpUp) {}
   const std::vector<int>& downProps() const { return _dframe;}
   const std::vector<int>& upProps() const { return _uframe;}
   bool hasDownProp(int p) const noexcept { return _dprop.hasProp(p);}
   bool hasUpProp(int p) const noexcept { return _uprop.hasProp(p);}
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
   void oldTransitionDown(MDDCstrDesc::Ptr,int,std::set<int> spDown,std::set<int> spCombined,lambdaTrans);
   void oldTransitionUp(MDDCstrDesc::Ptr,int,std::set<int> spDown,std::set<int> spCombined,lambdaTrans);
public:
   MDDSpec();
   // End-user API to define an ADD
   template <class Container>
   MDDCstrDesc::Ptr makeConstraintDescriptor(const Container& v,const char* n) {
      append(v);
      return constraints.emplace_back(new MDDCstrDesc(v,n));
   }
   MDDPBitSequence::Ptr downBSState(MDDCstrDesc::Ptr d,int nbb,unsigned char init,enum RelaxWith rw = External, int cPriority = 0, bool restrictedReducedInclude=true) override;
   MDDPBitSequence::Ptr upBSState(MDDCstrDesc::Ptr d,int nbb,unsigned char init,enum RelaxWith rw = External, int cPriority = 0) override;
   MDDPBitSequence::Ptr combinedBSState(MDDCstrDesc::Ptr d,int nbb,unsigned char init,enum RelaxWith rw = External, int cPriority = 0) override;
   MDDPByte::Ptr downByteState(MDDCstrDesc::Ptr d,int init,int max,enum RelaxWith rw=External,int cPriority = 0, bool restrictedReducedInclude=true) override;
   MDDPByte::Ptr upByteState(MDDCstrDesc::Ptr d,int init,int max,enum RelaxWith rw=External,int cPriority = 0) override;
   MDDPByte::Ptr combinedByteState(MDDCstrDesc::Ptr d,int init,int max,enum RelaxWith rw=External,int cPriority = 0) override;
   MDDPInt::Ptr downIntState(MDDCstrDesc::Ptr d,int init,int max,enum RelaxWith rw=External,int cPriority = 0, bool restrictedReducedInclude=true) override;
   MDDPInt::Ptr upIntState(MDDCstrDesc::Ptr d,int init,int max,enum RelaxWith rw=External,int cPriority = 0) override;
   MDDPInt::Ptr combinedIntState(MDDCstrDesc::Ptr d,int init,int max,enum RelaxWith rw=External,int cPriority = 0) override;
   MDDPSWindow<short>::Ptr downSWState(MDDCstrDesc::Ptr d,int len,int init,int finit,enum RelaxWith rw=External, int cPriority=0, bool restrictedReducedInclude=true) override;
   MDDPSWindow<short>::Ptr upSWState(MDDCstrDesc::Ptr d,int len,int init,int finit,enum RelaxWith rw=External, int cPriority=0) override;
   MDDPSWindow<short>::Ptr combinedSWState(MDDCstrDesc::Ptr d,int len,int init,int finit,enum RelaxWith rw=External, int cPriority=0) override;
   size_t nodeUB() const override { return x.size() * getWidth() * 2;}

   void nodeExist(NodeFun a);
   void arcExist(const MDDCstrDesc::Ptr d,ArcFun a);
   void updateNode(MDDCstrDesc::Ptr cd,MDDProperty::Ptr,std::set<MDDProperty::Ptr> spDown,std::set<MDDProperty::Ptr> spUp,UpdateFun update);
   void transitionDown(MDDCstrDesc::Ptr,MDDProperty::Ptr,std::initializer_list<MDDProperty::Ptr> down,std::initializer_list<MDDProperty::Ptr> comb,lambdaTrans);
   void transitionUp(MDDCstrDesc::Ptr,MDDProperty::Ptr,std::initializer_list<MDDProperty::Ptr> up,std::initializer_list<MDDProperty::Ptr> comb,lambdaTrans);
   template <typename LR> void addRelaxationDown(MDDCstrDesc::Ptr cd,MDDProperty::Ptr p,LR r) {
      _xDownRelax.insert(p->getId());
      int rid = (int)_downRelaxation.size();
      if (cd->ownsDownProperty(p->getId())) {
         cd->registerDownRelaxation(rid);
         _attrsDown[p->getId()]->setRelax(rid);
      }  
      _downRelaxation.emplace_back(std::move(r));
   }
   template <typename LR> void addRelaxationUp(MDDCstrDesc::Ptr cd,MDDProperty::Ptr p,LR r) {
      _xUpRelax.insert(p->getId());
      int rid = (int)_upRelaxation.size();
      if (cd->ownsUpProperty(p->getId())) {
         cd->registerUpRelaxation(rid);
         _attrsUp[p->getId()]->setRelax(rid);
      }  
      _upRelaxation.emplace_back(std::move(r));
   }
   void addSimilarity(int,lambdaSim);
   double similarity(const MDDState& a,const MDDState& b);
   void onFixpoint(FixFun onFix);
   void onRestrictedFixpoint(FixFun onFix);
   void splitOnLargest(SplitFun onSplit, int cPriority = 0);
   void candidateByLargest(CandidateFun candidateSplit, int cPriority = 0);
   void equivalenceClassValue(EquivalenceValueFun equivalenceValue, int cPriority = 0);
   //void valueScoring(ValueScoreFun valueSelection);
   void bestValue(BestValueFun bestValue);
   int numEquivalenceClasses();
   // Internal methods.
   void varOrder() override;
   bool consistent(const MDDPack& pack) const noexcept;
   void updateNode(MDDState& result,const MDDPack& n) const noexcept;
   bool exist(const MDDPack& parent,const MDDPack& child,const var<int>::Ptr& x,int v) const noexcept;
   void fullStateDown(MDDState& result,const MDDPack& parent,unsigned l,const var<int>::Ptr& var,const MDDIntSet& v);
   void incrStateDown(const MDDPropSet& out,MDDState& result,const MDDPack& parent,unsigned l,const var<int>::Ptr& var,const MDDIntSet& v);
   void fullStateUp(MDDState& target,const MDDPack& child,unsigned l,const var<int>::Ptr& var,const MDDIntSet& v);
   void incrStateUp(const MDDPropSet& out,MDDState& target,const MDDPack& child,unsigned l,const var<int>::Ptr& var,const MDDIntSet& v);
   void relaxationDown(MDDState& a,const MDDState& b) const noexcept;
   void relaxationUp(MDDState& a,const MDDState& b) const noexcept;
   void relaxationDownIncr(const MDDPropSet& out,MDDState& a,const MDDState& b) const noexcept;
   void relaxationUpIncr(const MDDPropSet& out,MDDState& a,const MDDState& b) const noexcept;
   MDDState rootState(Trailer::Ptr t,Storage::Ptr& mem);
   MDDState sinkState(Trailer::Ptr t,Storage::Ptr& mem);
   bool usesUp() const { return _upTransition.size() > 0;}
   bool usesCombined() const { return _updates.size() > 0;}
   void onlyUseApproximateForFirstIteration() { _onlyApproximateFirstIteration = true; }
   void finishedFirstPropagate() { if (_onlyApproximateFirstIteration) _approximateSplitting = false; }
   void useApproximateEquivalence() { _approximateSplitting = true;}
   void dontUseApproximateEquivalence() { _approximateSplitting = false;}
   bool approxEquivalence() const { return _approximateSplitting;}
   void setNodePriorityAggregateStrategy(int aggregateStrategy) { _nodePriorityAggregateStrategy = aggregateStrategy; }
   void setCandidatePriorityAggregateStrategy(int aggregateStrategy) { _candidatePriorityAggregateStrategy = aggregateStrategy; }
   int nodePriorityAggregateStrategy() const { return _nodePriorityAggregateStrategy; }
   void setConstraintPrioritySize(int size);
   template <class Container> void append(const Container& y) {
      for(auto e : y)
         if(std::find(x.cbegin(),x.cend(),e) == x.cend())
            x.push_back(e);
   }
   void addGlobal(std::initializer_list<var<int>::Ptr> y) {
      for(auto e : y)
         if(std::find(z.cbegin(),z.cend(),e) == z.cend())
            z.push_back(e);
   }
   void reachedFixpoint(const MDDPack& sink);
   void restrictedFixpoint(const MDDPack& sink);
   double nodeSplitPriority(const MDDNode& n, int cPriority) const;
   double candidateSplitPriority(const MDDState& state, void* arcs, int numArcs, int cPriority) const;
   std::vector<int> equivalenceValue(const MDDState& downState, const MDDState& upState, int cPriority = 0);
   int bestValueFor(TVec<MDDNode*>* layer);
   //int valueScoreFor(TVec<MDDNode*>* layer);
   bool hasNodeSplitRule() const noexcept { return _onSplit.size() > 0;}
   bool hasCandidateSplitRule() const noexcept { return _candidateSplit.size() > 0;}
   bool equivalentForConstraintPriority(const MDDState& left, const MDDState& right, int cPriority) const;
   bool equivalentForRestricted(const MDDState& left, const MDDState& right) const;
   int rebootFor(int l) const noexcept { return _rebootByLayer[l]; }
   void compile();
   std::vector<var<int>::Ptr>& getVars(){ return x; }
   std::vector<var<int>::Ptr>& getGlobals() { return z;}
   friend std::ostream& operator<<(std::ostream& os,const MDDSpec& s);
private:
   void init();
   std::vector<MDDCstrDesc::Ptr> constraints;
   std::vector<var<int>::Ptr> x;
   std::vector<var<int>::Ptr> z;
   std::vector<std::pair<MDDCstrDesc::Ptr,ArcFun>>  _exists;
   std::vector<NodeFun> _nodeExists;
   std::vector<UpdateFun>      _updates; // this is a list of function that applies to every node. 
   std::vector<lambdaTrans> _downTransition;
   std::vector<lambdaRelax> _downRelaxation;
   std::vector<lambdaRelax> _upRelaxation;
   std::vector<lambdaSim>   _similarity;
   std::vector<lambdaTrans> _upTransition;
   std::vector<FixFun>        _onFix;
   std::vector<FixFun>        _restrictedFix;
   std::vector<SplitFun>      _onSplit;
   std::vector<CandidateFun>      _candidateSplit;
   //std::vector<ValueScoreFun>      _valueScore;
   BestValueFun      _bestValue;
   std::vector<EquivalenceValueFun> _equivalenceValue;
   std::vector<std::vector<lambdaTrans>> _transLayer;
   std::vector<std::vector<lambdaTrans>> _uptransLayer;
   std::vector<LayerDesc> _frameLayer;
   std::vector<std::vector<ArcFun>> _scopedExists; // 1st dimension indexed by varId. 2nd dimension is a list.
   std::set<int>      _xDownRelax;
   std::set<int>      _xUpRelax;
   std::vector<int>   _defaultDownRelax;
   std::vector<int>   _defaultUpRelax;
   bool _onlyApproximateFirstIteration;
   bool _approximateSplitting;
   int _nodePriorityAggregateStrategy;
   int _candidatePriorityAggregateStrategy;
   std::vector<std::vector<SplitFun>> _onSplitByPriorities;
   std::vector<std::vector<CandidateFun>> _candidateSplitByPriorities;
   std::vector<std::vector<EquivalenceValueFun>> _equivalenceValueByPriorities;
   std::vector<std::vector<int>> _propertiesByPriorities;
   std::vector<int> _restrictedReducedProperties; //Properties used in restricted MDD to reduce.  Mainly doesn't include properties for objective
   std::vector<int> _rebootByLayer;
};

class MDDSpec;
class MDDStateFactory {
   struct MDDSKey {
      const MDDState*   _s0;
      const MDDState*   _s1;
      const int          _l;
      const int          _v;
   };
   struct EQtoMDDSKey {
      bool operator()(const MDDSKey& a,const MDDSKey& b) const noexcept {
         return a._l == b._l && a._v == b._v && a._s0->operator==(*b._s0) && a._s1->operator==(*b._s1);
      }
   };
   struct HashMDDSKey {
      std::size_t operator()(const MDDSKey& key) const noexcept {
         return key._s0->hash() ^ key._v;
      }
   };
   Trailer::Ptr    _trail;
   MDDSpec*      _mddspec;
   Pool::Ptr         _mem;
   Hashtable<MDDSKey,MDDState*,HashMDDSKey,EQtoMDDSKey> _downHash;
   Hashtable<MDDSKey,MDDState*,HashMDDSKey,EQtoMDDSKey> _upHash;
   PoolMark         _mark;
   bool          _enabled;
public:
   MDDStateFactory(Trailer::Ptr trail,MDDSpec* spec);
   MDDState* createCombinedState(bool forRestricted=false);
   void createStateDown(MDDState& result,const MDDPack& parent,int layer,const var<int>::Ptr x,const MDDIntSet& vals,bool up);
   void createStateUp(MDDState& result,const MDDPack& child,int layer,const var<int>::Ptr x,const MDDIntSet& vals);
   void splitState(MDDState*& result,MDDNode* n,const MDDPack& parent,int layer,const var<int>::Ptr x,int val);
   void clear();
   void enable() noexcept { _enabled = true;}
   void disable() noexcept { _enabled = false;}
   unsigned sizeDown() const noexcept { return _downHash.size();}
   unsigned sizeUp() const noexcept { return _upHash.size();}
};


template <typename Container> std::pair<int,int> domRange(const Container& vars) {
   std::pair<int,int> udom;
   udom.first = INT_MAX;
   udom.second = -INT_MAX;
   for(const auto& x : vars){
      udom.first  = std::min(udom.first,x->min());
      udom.second = std::max(udom.second,x->max());
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

