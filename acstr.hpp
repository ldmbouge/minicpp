#ifndef __ACSTR_H
#define __ACSTR_H

#include <memory>
#include <iostream>
#include "handle.hpp"

class Constraint {
protected:
   virtual void print(std::ostream& os) const {}
public:
   typedef handle_ptr<Constraint> Ptr;
   Constraint() {}
   virtual ~Constraint() {}
   virtual void post() = 0;
   virtual void propagate() {}
};

class Objective : public Constraint {
public:
   typedef handle_ptr<Objective> Ptr;
   virtual void tighten() = 0;
};

#endif
