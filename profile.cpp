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

#include <limits>
#include "profile.hpp"
#include <assert.h>

Profile::Profile(const std::vector<Rectangle> &ar) {
    _profile = new Rectangle[ar.size() * 2 + 2];
    _psz = 0;  
   std::vector<Point> point(ar.size() * 2 + 2);
   const auto sz = ar.size();
   auto i = 0u;
   for (const auto &p : ar) {
       point[i]      = Point { p._start,   p._height };
       point[i + sz] = Point { p._end  , - p._height };
      i++;      
   }
   point[2 * sz]     = Point { std::numeric_limits<int>::min(), 0 };
   point[2 * sz + 1] = Point { std::numeric_limits<int>::max(), 0 };
   std::sort(point.begin(), point.end(),
             [](const auto& p1,const auto& p2) { return p1._key < p2._key; });
  
   int sweepHeight = 0;
   int sweepTime = point[0]._key;
   for (const auto& [t,h] : point) {
     if (t != sweepTime) {
         assert(_psz < (ar.size() * 2 + 2));       
         _profile[_psz++] = Rectangle(sweepTime, t, sweepHeight);
         sweepTime = t;
     }
     sweepHeight += h;
   }
}

Profile::~Profile()
{
   delete []_profile;  
}
