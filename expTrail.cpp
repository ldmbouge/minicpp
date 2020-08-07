#include "expTrail.hpp"
#include "explainer.hpp"
#include "literal.hpp"

ExpTrailer::ExpTrailer(ExpSolver* es, handle_ptr<Explainer> exp)
  : Trailer(), 
    _es(es), 
    _exp(exp)
  {}

ExpTrailer::~ExpTrailer()
{
   for (auto e : _lits)
      delete e.getLit();
}

void ExpTrailer::clearLitsTo(size_t to) {
   Literal* lp;
   while (_lits.back().trailSize() != to) {
      lp = _lits.back().getLit();
      delete lp;
      _lits.pop_back();
   }
}

void ExpTrailer::popNoExp()
{
   unsigned to;
   std::size_t mem;
   long node;
   std::tie(to,mem,node) = _tops.top();
   _tops.pop();
   while (_trail.size() != to) {
      Entry* entry = _trail.top();
      entry->restore();
      _trail.pop();
      entry->Entry::~Entry();
   }
   _btop = mem;
   clearLitsTo(to);
}

void ExpTrailer::popWithExp()
{
   _exp->setFailDepth(_es->getDepth());
   unsigned to;
   std::size_t mem;
   long node;
   std::tie(to,mem,node) = _tops.top();
   _tops.pop();
   size_t tsz = _lits.back().trailSize();
   while (_trail.size() != to) {
      if (_trail.size() == tsz) {
         _exp->checkLit(_lits.back().getLit());
         _lits.pop_back();
         tsz = _lits.back().trailSize();
      }
      Entry* entry = _trail.top();
      entry->restore();
      _trail.pop();
      entry->Entry::~Entry();
   }
   _btop = mem;
   _exp->clearNoGood();
}

void ExpTrailer::restoreState()
{
   if (_es->status() == Failure)
      popWithExp();
   else
      popNoExp();
}

void ExpTrailer::storeLit(Literal* lp)
{
   _lits.emplace_back(LitEntry(_trail.size(), lp));
}
