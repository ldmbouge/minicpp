#ifndef __ACSTR_H
#define __ACSTR_H

#include <memory>
#include <iostream>
#include "handle.hpp"

class Constraint {
   bool _scheduled;
protected:
   virtual void print(std::ostream& os) const {}
public:
   typedef handle_ptr<Constraint> Ptr;
   Constraint() : _scheduled(false) {}
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
