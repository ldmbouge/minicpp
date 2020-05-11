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

#ifndef __QUEUE_H
#define __QUEUE_H

#include <iostream>
#include <utility>

template <typename T> class CQueue;

template <typename T> class Location {
   T   _val;
   int _pos;
   friend class CQueue<T>;   
public:
   T value() const noexcept { return _val;}
   T operator*() const noexcept { return _val;}
};

template <typename T> class CQueue  {
   int _mxs;
   int _enter;
   int _exit;
   int _mask;
   int _nbs;
   int _mxSeg;
   Location<T>*  _locs;
   Location<T>** _vlocs;
   Location<T>** _data;
   void resize() {
      const int newSize = _mxs << 1;
      Location<T>* nl = new Location<T>[_mxs];
      Location<T>** nd = new Location<T>*[newSize];
      int at = 0,cur = _exit;
      do {
         nd[at] = _data[cur];
         nd[at]->_pos = at;
         ++at;
         cur = (cur + 1) & _mask;
      } while (cur != _enter);
      nd[at++] = _data[cur];
      while (at < newSize) {
         nd[at] = nl + at - _mxs;
         ++at;
      }
      delete[]_data;
      if (_nbs >= _mxSeg) {
         Location** nvl = new Location<T>*[_mxSeg << 1];
         for(int i=0;i < _mxSeg;++i) nvl[i] = _vlocs[i];
         delete[]_vlocs;
         _vlocs = nvl;
         _mxSeg <<= 1;
      }
      _vlocs[_nbs++] = nl;
      _data = nd;
      _exit = 0;
      _enter = _mxs - 1;
      _mxs = newSize;
      _mask = _mxs - 1;
   }
public:
   CQueue(int sz = 32) : _mxs(sz),_mxSeg(8),_nbs(1) {
      _locs = new Location<T>[_mxs];
      _vlocs = new Location<T>*[_mxSeg];
      _data = new Location<T>*[_mxs];
      _vlocs[0] = _locs;
      for(auto i=0u;i < _mxs;++i)
         _data[i] = _locs+i;
      _mask = _mxs - 1;
      _enter = _exit = 0;
   }
   ~CQueue() {
      for(int i=0u;i < _nbs;++i)
         delete[]_vlocs[i];
      delete[]_vlocs;
      delete[]_data;
   }
   void clear() noexcept { _enter = _exit = 0;}
   bool empty() const noexcept { return _enter == _exit;}
   const Location<T>* enQueue(const T& v) {
      int nb = (_mxs + _enter - _exit) & _mask;
      if (nb == _mxs - 1) resize();
      Location<T>* at = _data[_enter];
      at->_val = v;
      at->_pos = _enter;
      _enter = (_enter + 1) & _mask;
      return at;
   }
   T deQueue() {
      if (_enter != _exit) {
         T rv = _data[_exit]->_val;
         _exit = (_exit + 1) & _mask;
         return rv;
      } else return T();
   }
   void retract(const T& val) {
      int cur = _exit;
      do {
         if (_data[cur]->_val == val) {
            retract(_data[cur]);
            return;
         }
         cur = (cur + 1) & _mask;
      } while (cur != _enter);      
   }
   void retract(const Location<T>* loc) {
      int at = loc->_pos;
      if (at == _exit)
         _exit = (_exit + 1) & _mask;
      else if (at == _enter)
         _enter = (_enter > 0) ? _enter - 1 : _mask;
      else {
         std::swap(_data[at],_data[_exit]);
         _data[at]->_pos = at;
         _exit = (_exit + 1) & _mask;
      }
   }
};

#endif

