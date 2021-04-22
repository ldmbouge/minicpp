#include "conListener.hpp"
#include "literal.hpp"
#include "explainer.hpp"
#include <iostream>

NEQBoolExpListener::NEQBoolExpListener(NEQBool::Ptr c)
  : _con(c),
    _lis(c->getListener())
{
    _exp = c->_b1->getSolver()->getExpSolver()->getExplainer();
    c->setListener(this);
}

void NEQBoolExpListener::fail()
{
    std::cout << "exp NEQBool fail called, but not implemented!\n";
    // std::vector<Literal*> nogood; 
    // for (auto v : _con->_x) 
    //    nogood.emplace_back(new Literal(v, EQ, 0, _con, v->getSolver()->getExpSolver()->getDepth()));
    // _exp->setNoGood(nogood);
    _lis->fail();
}

std::vector<Literal*> NEQBoolExpListener::explain(Literal* lp)
{
    if (lp->getRel() != EQ) 
        return std::vector<Literal*>({lp});
    Literal* rp;
    var<int>::Ptr vp = (lp->getVar() == _con->_b1) ? _con->_b2 : _con->_b1;
    Literal l = Literal(vp, EQ, 1 - lp->getVal(), _con->getListener(), 0);
    if ((rp = _exp->findLit(l)))
        return std::vector<Literal*>({rp});
    else assert(false);
}

ClauseExpListener::ClauseExpListener(Clause::Ptr c)
  : _con(c),
    _lis(c->getListener())
{
    _exp = c->_x[0].var()->getSolver()->getExpSolver()->getExplainer();
    c->setListener(this);
}

void ClauseExpListener::fail()
{
    std::cout << "exp clause fail called\n";
    std::vector<Literal*> nogood; 
    Literal* lp = nullptr;
    for (const auto& ce : _con->_x) {
        Literal l;
        if (ce.sense() == Clause::POS)
            l = Literal(ce.var(), EQ, 0, _con->getListener(), 0);
        else
            l = Literal(ce.var(), EQ, 1, _con->getListener(), 0);
        // if (l.getVar()->getId() == 13 && l.getRel() == EQ && l.getVal() == 0)
        //     std::cout << "fail is looking for < x13 == 0 > lit";
        lp = _exp->findLit(l);
        if (lp)
        // if ((lp = _exp->findLit(l)))
            nogood.push_back(lp);
        else assert(false);
    }
    _exp->setNoGood(nogood);
    std::cout << "setting nogood\n\t";
    _exp->printNoGood();
    _lis->fail();
}

std::vector<Literal*> ClauseExpListener::explain(Literal* lp)
{
    if ((lp->getRel() != EQ))
        assert(false); 
    std::vector<Literal*> reason; 
    Literal* rp;  // reason lit pointer
    Literal l;
    for (const auto& ce : _con->_x) {
        if (ce.var()->getId() != lp->getVar()->getId()) {
            if (ce.sense() == Clause::POS)
                l = Literal(ce.var(), EQ, 0, _con->getListener(), 0);
            else
                l = Literal(ce.var(), EQ, 1, _con->getListener(), 0);
            if ((rp = _exp->findLit(l)))
                reason.push_back(rp);
            else assert(false);
        }
    }
    return reason;
}

AllDiffACExpListener::AllDiffACExpListener(AllDifferentAC::Ptr c)
  : _con(c),
    _lis(c->getListener())
{
    c->setListener(this);
    _exp = c->_x[0]->getSolver()->getExpSolver()->getExplainer();
}

void AllDiffACExpListener::fail()
{
    // std::cout << "exp allDiff fail called\n";
    _lis->fail();
}
