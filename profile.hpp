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

#ifndef __PROFILE_HPP
#define __PROFILE_HPP

#include <vector>
#include <handle.hpp>
#include <iostream>

class Profile {
public:  
  struct Point {
      int _key;
      int _value;
      friend std::ostream &operator<<(std::ostream &os, const Point &p) {
          return os << "Pt(" << p._key << ',' << p._value << ')';
      }
  };
  struct Rectangle {
     int _start, _dur, _height, _end;
     Rectangle() { _start = _dur = _height = _end = 0;}    
     Rectangle(int s, int e, int h) {
        _start = s;
        _end = e;
        _dur = e - s;
        _height = h;
     }
     int getStart() const noexcept  { return _start; }
     int getEnd() const noexcept    { return _end;}
     int getHeight() const noexcept { return _height;}
     bool contains(int t) const noexcept { return _start <= t && t < _end;}     
     friend std::ostream &operator<<(std::ostream &os, const Rectangle &r) {
        return os << "Rect[" << r._start << ',' << r._dur
                  << ',' << r._end << ",h:" << r._height << ']';
     }
  };
private:  
    Rectangle *_profile;
    std::size_t _psz;
public:
   typedef handle_ptr<Profile> Ptr;  
   Profile(const std::vector<Rectangle>& ar);
   ~Profile();
   int rectangleIndex(int t) const noexcept {
     for (auto i = 0u; i < _psz; i++) 
        if (_profile[i].contains(t))
           return i;     
     return -1;
   }
   int size() const noexcept { return _psz; }
   const Rectangle &get(int i) const noexcept { return _profile[i]; }
   void print() { std::cout << *this << "\n";}
   friend std::ostream &operator<<(std::ostream &os, const Profile &p) {
      os << "[ ";     
      for (auto i=0u;i<p._psz;i++)
         os << p._profile[i] << ',';      
      return os << "\b]";   
   }     
};  

#endif
