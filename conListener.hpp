#ifndef __CON_LISTENER_H
#define __CON_LISTENER_H

#include "fail.hpp"
#include "literal.hpp"
#include "explainer.hpp"
#include <iostream>

class Constraint;

class ConListener {
protected:
    Constraint* _con;
public:
    ConListener(Constraint* con) : _con(con) {}
    virtual void fail() { std::cout << "fail called\n"; failNow();}
};

class ExpConListener : public ConListener {
protected:
    ConListener* _lis;
public:
    ExpConListener(Constraint*);
};

class ClauseExpListener : public ExpConListener {
public:
    ClauseExpListener(Constraint*); 
    void fail() override { std::cout << "clause exp fail called\n"; _lis->fail();}
};

class AllDiffACExpListener : public ExpConListener {
public:
    AllDiffACExpListener(Constraint*); 
    void fail() override { std::cout << "allDiffAC exp fail called\n"; _lis->fail();}
};



#endif