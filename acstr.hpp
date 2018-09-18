#ifndef __ACSTR_H
#define __ACSTR_H

#include "handle.hpp"
#include "trailable.hpp"

class CPSolver;

class Constraint {
   bool _scheduled;
   trail<bool> _active;
public:
   typedef handle_ptr<Constraint> Ptr;
   Constraint(handle_ptr<CPSolver> cp);
   virtual ~Constraint() {}
   virtual void post() = 0;
   virtual void propagate() {}
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
