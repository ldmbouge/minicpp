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
// #include "conListener.hpp"
#include <vector>

class CPSolver;
class Literal;
class Visitor;
class ConListener;

class Constraint {
private:
   bool     _scheduled;
   unsigned char _prio;
   trail<bool> _active;
protected:
   ConListener* _lis;
public:
   static const unsigned char CLOW  = 0;
   static const unsigned char CHIGH = 1;
   typedef handle_ptr<Constraint> Ptr;
   Constraint(handle_ptr<CPSolver> cp);
   virtual ~Constraint() {}
   virtual void post() = 0;
   virtual void propagate() {}
   virtual std::vector<Literal*> explain(Literal* lp) { return std::vector<Literal*>({lp});}
   virtual void print(std::ostream& os) const {}
   void setPriority(unsigned char p) { _prio = p;}
   unsigned char getPriority() const { return _prio;}
   void setScheduled(bool s) { _scheduled = s;}
   bool isScheduled() const  { return _scheduled;}
   void setActive(bool a)    { _active = a;}
   bool isActive() const     { return _active;}
   virtual void visit(Visitor& v) {}
   ConListener* getListener() { return _lis;}
   void setListener(ConListener* l) { _lis = l;}
};

class Objective {
public:
   typedef handle_ptr<Objective> Ptr;
   virtual void tighten() = 0;
   virtual int value() const = 0;
};

#endif
