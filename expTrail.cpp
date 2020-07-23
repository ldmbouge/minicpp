#include "expTrail.hpp"
#include "explainer.hpp"
#include "literal.hpp"

ExpTrailer::ExpTrailer(ExpSolver* es)
  : Trailer(), 
    _es(es), 
    _exp(es->getExplainer())
  {}

ExpTrailer::~ExpTrailer()
{
   for (auto e : _lits)
      delete e.getLit();
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
}

void ExpTrailer::popWithExp()
{
   unsigned to;
   std::size_t mem;
   long node;
   std::tie(to,mem,node) = _tops.top();
   _tops.pop();
   size_t tsz = _lits.back().trailSize();
   while (_trail.size() != to) {
      if (_trail.size() == tsz) {
         examineNextLit();
         _lits.pop_back();
         tsz = _lits.back().trailSize();
      }
      Entry* entry = _trail.top();
      entry->restore();
      _trail.pop();
      entry->Entry::~Entry();
   }
   _btop = mem;
}

void ExpTrailer::pop()
{
   if (_es->status() == Failure)
      popWithExp();
   else
      popNoExp();
}

void ExpTrailer::examineNextLit()
{
   Literal* lp = _lits.back().getLit();
   std::cout << "examining: " << *lp << "\n";
   delete lp;
}

void ExpTrailer::storeLit(Literal* lp)
{
   _lits.emplace_back(LitEntry(_trail.size(), lp));
}
