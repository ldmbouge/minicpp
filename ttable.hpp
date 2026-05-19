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

#ifndef __TIMETABLE_HPP
#define __TIMETABLE_HPP

#include <handle.hpp>
#include <vector>
#include "profile.hpp"
#include "intvar.hpp"
#include "acstr.hpp"

class CumulativeTT : public Constraint {
   Factory::Veci _start;
   std::vector<int> _duration;
   Factory::Veci _end;
   std::vector<int> _demand;
   int _capa;
   bool _postMirror;
   Profile* buildProfile();
public:
  CumulativeTT(Factory::Veci &start,
               const std::vector<int> &dur,
               const std::vector<int> &reqs,
               int capa, bool postMirror = true)
      : Constraint(start[0]->getSolver()),
        _start(start.size(), Factory::alloci(start[0]->getStore())),
        _end(_start.size(), Factory::alloci(start[0]->getStore()))
   {
      using namespace Factory;
      int i  = 0;
      for(auto& xi : start) {
        _start[i] = xi;
        _end[i] = xi + dur[i];
        i++;        
      }
      _duration = dur;
      _demand = reqs;
      _capa = capa;
      _postMirror = postMirror;
   }
   void post();
   void propagate();
};



#endif
