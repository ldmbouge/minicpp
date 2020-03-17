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

#ifndef __ACSTR_H
#define __ACSTR_H

#include "handle.hpp"
#include "trailable.hpp"

class CPSolver;
class Literal;

class Constraint {
   bool _scheduled;
   trail<bool> _active;
public:
   typedef handle_ptr<Constraint> Ptr;
   Constraint(handle_ptr<CPSolver> cp);
   virtual ~Constraint() {}
   virtual void post() = 0;
   virtual void propagate() {}
   virtual void explain(Literal& l) {}
   virtual void print(std::ostream& os) const {}
   void setScheduled(bool s) { _scheduled = s;}
   bool isScheduled() const  { return _scheduled;}
   void setActive(bool a)    { _active = a;}
   bool isActive() const     { return _active;}
};

class Objective {
public:
   typedef handle_ptr<Objective> Ptr;
   virtual void tighten() = 0;
   virtual int value() const = 0;
};;

#endif
