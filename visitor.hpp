#ifndef __VISITOR_H
#define __VISITOR_H

#include <iostream>

class Constraint;

class Visitor {
public:
    virtual void visitClause(Constraint* c) {}
    virtual void visitAllDifferentAC(Constraint* c) {}
};

class ExpVisitor : public Visitor {
public:
    void visitClause(Constraint* c) override; 
    void visitAllDifferentAC(Constraint* c) override;
};






#endif