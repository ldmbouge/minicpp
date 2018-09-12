#ifndef __ACSTR_H
#define __ACSTR_H

#include <memory>
#include <iostream>
#include "handle.hpp"
#include "trailable.hpp"

class CPSolver;

class Constraint {
   bool _scheduled;
   trail<bool> _active;
protected:
   virtual void print(std::ostream& os) const {}
public:
   typedef handle_ptr<Constraint> Ptr;
   Constraint(handle_ptr<CPSolver> cp);
   virtual ~Constraint() {}
   virtual void post() = 0;
   virtual void propagate() {}
   void setScheduled(bool s) { _scheduled = s;}
   bool isScheduled() const  { return _scheduled;}
};

class Objective {
public:
    typedef handle_ptr<Objective> Ptr;
    virtual void tighten() = 0;
};

#endif
