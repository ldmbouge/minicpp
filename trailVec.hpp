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

#ifndef __TRAILVEC_H
#define __TRAILVEC_H

#include <memory>
#include <iterator>
#include "trailable.hpp"
#include "store.hpp"
#include <assert.h>

template <class T,typename SZT = std::size_t> class TVec {
   Trailer::Ptr   _t;
   SZT           _sz;
   SZT          _msz;
   T*          _data;
public:
   class TVecSetter {
      Trailer::Ptr _t;
      T* _at;
   public:
      TVecSetter(Trailer::Ptr t,T* at) : _t(t),_at(at) {}
      TVecSetter& operator=(T&& v) {
         _t->trail(new (_t) TrailEntry<T>(_at));
         *_at = std::move(v);
         return *this;
      }
      template <class U> TVecSetter& operator=(const U& nv) {
         _t->trail(new (_t) TrailEntry<T>(_at));
         *_at  = nv;
         return *this;
      }
      operator T() const { return *_at;}
      T operator->() const noexcept { return *_at;}
   };
   template <class U> class TrailEntry : public Entry {
      U* _at;
      U  _old;
   public:
      TrailEntry(U* ptr) : _at(ptr),_old(*ptr) {}
      void restore() { *_at = _old;}
   };
   TVec() : _t(nullptr),_sz(0),_msz(0),_data(nullptr) {}
   TVec(Trailer::Ptr t,Storage::Ptr mem,SZT s)
      : _t(t),_sz(0),_msz(s) {
      _data = new (mem) T[_msz];
   }
   TVec(TVec&& v)
      : _t(v._t),_sz(v._sz),_msz(v._msz),
        _data(std::move(v._data))
   {}
   TVec& operator=(TVec&& s) {
      _t = std::move(s._t);
      _sz = s._sz;
      _msz = s._msz;
      _data = std::move(s._data);
      return *this;
   }
   Trailer::Ptr getTrail() { return _t;}
   void clear() {
      _t->trail(new (_t) TrailEntry<SZT>(&_sz));
      _sz = 0;
   }
   SZT size() const { return _sz;}
   void push_back(const T& p,Storage::Ptr mem) {
      if (_sz >= _msz) {
         SZT newSize = _msz << 1;
         T* nd = new (mem) T[newSize];
         for(int i=0;i< _msz;i++)
            nd[i] = _data[i];
         _t->trail(new (_t) TrailEntry<T*>(&_data));
         _t->trail(new (_t) TrailEntry<SZT>(&_msz));
         _data = nd;
         _msz = newSize;
      }
      at(_sz,p);
      _t->trail(new (_t) TrailEntry<SZT>(&_sz));
      _sz += 1;
       assert(_sz > 0);
   }
   SZT remove(SZT i) {
      assert(_sz > 0);
      assert(i >= 0 && i < _sz);
      if (i < _sz - 1) 
         at(i,_data[_sz - 1]);      
      _t->trail(new (_t) TrailEntry<SZT>(&_sz));
      _sz -= 1;
      return _sz;
   }
   T get(SZT i) const                { return _data[i];}
   T operator[](SZT i) const         { return _data[i];}
   TVecSetter operator[](SZT i)      { return TVecSetter(_t,_data+i);}
   void at(SZT i,const T& nv) {
      _t->trail(new (_t) TrailEntry<T>(_data+i));
      _data[i] = nv;
   }
   class iterator: public std::iterator<std::input_iterator_tag,T,long> {
      T*    _data;
      long   _num;
      iterator(T* d,long num = 0) : _data(d),_num(num) {}
   public:
      iterator& operator++()   { _num = _num + 1; return *this;}
      iterator operator++(int) { iterator retval = *this; ++(*this); return retval;}
      iterator& operator--()   { _num = _num - 1; return *this;}
      iterator operator--(int) { iterator retval = *this; --(*this); return retval;}
      bool operator==(iterator other) const {return _num == other._num;}
      bool operator!=(iterator other) const {return !(*this == other);}
      T& operator*() const {return _data[_num];}
      friend class TVec;
   };
   using reverse_iterator = std::reverse_iterator<iterator>;
   iterator begin() const { return iterator(_data,0);}
   iterator end()   const { return iterator(_data,_sz);}
   reverse_iterator rbegin() const { return reverse_iterator(end());}
   reverse_iterator rend()   const { return reverse_iterator(begin());}
};

#endif
