#ifndef __VISITOR_H
#define __VISITOR_H

#include <iostream>
#include "handle.hpp"

class Constraint;
class Clause;
class AllDifferentAC;

class Visitor {
public:
    virtual void visitClause(handle_ptr<Clause> c) {} 
    virtual void visitAllDifferentAC(handle_ptr<AllDifferentAC> c) {}
};

class ExpVisitor : public Visitor {
public:
    void visitClause(handle_ptr<Clause> c) override; 
    void visitAllDifferentAC(handle_ptr<AllDifferentAC> c) override;
};






#endif