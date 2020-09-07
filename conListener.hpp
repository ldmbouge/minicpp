#ifndef __CON_LISTENER_H
#define __CON_LISTENER_H

#include "fail.hpp"
#include "literal.hpp"
#include "explainer.hpp"
#include "constraint.hpp"
#include <iostream>


class ConListener {
public:
    virtual void fail() { std::cout << "fail called\n"; failNow();}
};

class ClauseExpListener : public ConListener {
    Clause::Ptr  _con;
    ConListener* _lis;
    Explainer::Ptr _exp;
public:
    ClauseExpListener(Clause::Ptr); 
    void fail() override { std::cout << "clause exp fail called\n"; _lis->fail();}
};

class AllDiffACExpListener : public ConListener {
    AllDifferentAC::Ptr  _con;
    ConListener* _lis;
    Explainer::Ptr _exp;
public:
    AllDiffACExpListener(AllDifferentAC::Ptr); 
    void fail() override { std::cout << "allDiffAC exp fail called\n"; _lis->fail();}
};



#endif