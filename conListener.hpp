#ifndef __CON_LISTENER_H
#define __CON_LISTENER_H

#include "fail.hpp"
#include "explainer.hpp"
#include "constraint.hpp"
#include <vector>

class Literal;

class ConListener {
public:
    virtual void fail() { failNow();}
    virtual std::vector<Literal*> explain(Literal* lp) { return std::vector<Literal*>({lp});}
};

class NEQBoolExpListener : public ConListener {
    NEQBool::Ptr  _con;
    ConListener* _lis;
    Explainer::Ptr _exp;
public:
    NEQBoolExpListener(NEQBool::Ptr); 
    void fail() override;
    std::vector<Literal*> explain(Literal*) override;
};

class ClauseExpListener : public ConListener {
    Clause::Ptr  _con;
    ConListener* _lis;
    Explainer::Ptr _exp;
public:
    ClauseExpListener(Clause::Ptr); 
    void fail() override;
    std::vector<Literal*> explain(Literal*) override;
};

class AllDiffACExpListener : public ConListener {
    AllDifferentAC::Ptr  _con;
    ConListener* _lis;
    Explainer::Ptr _exp;
public:
    AllDiffACExpListener(AllDifferentAC::Ptr); 
    void fail() override;
};



#endif