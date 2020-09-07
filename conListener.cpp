#include "conListener.hpp"

ClauseExpListener::ClauseExpListener(Clause::Ptr c)
  : _lis(c->getListener())
{
    _exp = c->_x[0]->getSolver()->getExpSolver()->getExplainer();
    c->setListener(this);
}

AllDiffACExpListener::AllDiffACExpListener(AllDifferentAC::Ptr c)
  : _lis(c->getListener())
{
    c->setListener(this);
    _exp = c->_x[0]->getSolver()->getExpSolver()->getExplainer();
}
