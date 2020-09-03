#include "conListener.hpp"
#include "constraint.hpp"

ExpConListener::ExpConListener(Constraint* c)
  : ConListener(c),
    _lis(c->getListener())
{
    c->setListener(this);
}

ClauseExpListener::ClauseExpListener(Constraint* c)
  : ExpConListener(c)
{}

AllDiffACExpListener::AllDiffACExpListener(Constraint* c)
  : ExpConListener(c)
{}
