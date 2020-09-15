#include "globalCutQueue.hpp"


void GlobalCutQueue::addToQueue(Constraint::Ptr cPtr, int d)
{
    _cuts.emplace_back(GlobalCut(cPtr,d));
}
