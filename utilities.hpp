//
//  utilities.hpp
//  mdd
//
//  Created by zitoun on 1/11/20.
//  Copyright © 2020 zitoun. All rights reserved.
//

#ifndef utilities_h
#define utilities_h

#include <set>
#include <map>
#include <string.h>

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
   ValueSet(const Factory::Veci& s) {
      _min = (s.size()) ? s[0]->getId() : 0;
      _max = (s.size()) ? s[0]->getId() : -1;
      for(auto v : s) {
         _min = _min < v->getId() ? _min : v->getId();
         _max = _max > v->getId() ? _max : v->getId();
      }
      _sz = _max - _min + 1;
      _data = new char[_sz];
      memset(_data,0,sizeof(char)*_sz);
      for(auto v : s)
         _data[v->getId() - _min] = 1;
   }
   ValueSet(const std::vector<var<int>::Ptr>& s) {
      _min = (s.size()) ? s[0]->getId() : 0;
      _max = (s.size()) ? s[0]->getId() : -1;
      for(auto v : s) {
         _min = _min < v->getId() ? _min : v->getId();
         _max = _max > v->getId() ? _max : v->getId();
      }
      _sz = _max - _min + 1;
      _data = new char[_sz];
      memset(_data,0,sizeof(char)*_sz);
      for(auto v : s)
         _data[v->getId() - _min] = 1;
   }
   ValueSet(const ValueSet& s) : _min(s._min), _max(s._max), _sz(s._sz)
   {
      _data = new char[_sz];
      memcpy(_data,s._data,sizeof(char)*_sz);
   }
   bool member(int v) const noexcept {
      if (_min <= v && v <= _max)
         return _data[v - _min];
      else return false;
   }
   friend inline std::ostream& operator<<(std::ostream& os,const ValueSet& s)
   {
      os << "{" << s._min << "," << s._max << std::endl;
      for(int i = s._min; i <= s._max; i++)
         s.member(i);
      return os << "}" << std::endl;
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

#endif /* utilities_h */
