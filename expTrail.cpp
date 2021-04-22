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
   // check to see if this needs to delete lits in database too
   for (auto e : _lits)
      delete e.getLit();
}

void ExpTrailer::clearLitsTo(size_t to) {
   Literal* lp;
   // std::cout << "++++++++++++++++++++++++++++++++\n";
   // std::cout << "clearing lits to " << to << "\n";
   // printLitDatabase();
   while ( _lits.size() > 0 && _lits.back().trailSize() > to) {
      lp = _lits.back().getLit();
      removeLit(lp);
      delete lp;
      _lits.pop_back();
   }
   // std::cout << "++++++++++++++++++++++++++++++++\n";
   // std::cout << "after clearing\n";
   // printLitDatabase();
   // std::cout << "++++++++++++++++++++++++++++++++\n";
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
    std::cout << "PopNoExp done\n";
    printLitDatabase();
}

void ExpTrailer::popWithExp()
{
    std::cout << "start popWithExp\n";
    printLitDatabase(); 
   _exp->setFailDepth(_es->getDepth());
   unsigned to;
   std::size_t mem;
   long node;
   Literal* lp;
   std::tie(to,mem,node) = _tops.top();
   _tops.pop();
   size_t tsz = 0;
   if (_lits.size() > 0)
      tsz = _lits.back().trailSize();
   while (_trail.size() != to) {
      if (_trail.size() == tsz) {
         lp = _lits.back().getLit();
         removeLit(lp);
         _lits.pop_back();
         std::cout << "checking " << *lp << "\n";
         _exp->checkLit(lp);
         if (_lits.size() > 0)
            tsz = _lits.back().trailSize();
         else
            tsz = 0;
      }
      else {
         Entry* entry = _trail.top();
         entry->restore();
         _trail.pop();
         entry->Entry::~Entry();
      }
   }
   _btop = mem;
   _exp->clearNoGood();
    std::cout << "PopWithExp done\n";
    printLitDatabase();
    if (_exp->backjumping()) 
       backJumpTo(_exp->backjumpDepth()); 
}

void ExpTrailer::restoreState()
{
   if (_es->status() == Failure)
      try {
          popWithExp();
      } catch (Status e) {
          throw e;
      }
   else
      popNoExp();
}

void ExpTrailer::storeLit(Literal* lp)
{
    // if (lp->getVar()->getId() == 7 && lp->getRel() == NEQ && lp->getVal() == 1)
    //     std::cout << "storing < x7 != 1 > lit\n";
   _lits.emplace_back(LitEntry(_trail.size(), lp));
   _litDatabase.insert( {litKey(*lp), lp} );
}

void ExpTrailer::removeLit(Literal* lp)
{
   // std::cout << "trying to remove literal " << *lp << "\n";
   auto it = _litDatabase.find(litKey(*lp));
   if (it != _litDatabase.end()) {
       auto lp = it->second;
    //    if (lp->getVar()->getId() == 7 && lp->getRel() == NEQ && lp->getVal() == 1)
    //         std::cout << "removing < x7 != 1 > lit\n";
      _litDatabase.erase(it);
   }
   else
      std::cout << "did not find lit while removing it\n";
}

Literal* ExpTrailer::findLit(Literal& l)
{
   auto it = _litDatabase.find(litKey(l));
   if (it == _litDatabase.end()) {
      std::cout << "failed finding " << l << " in lit database\n";
      printLitDatabase();
      return nullptr;
    //   assert(false);
   }
   return it->second;
}

void ExpTrailer::printLitDatabase()
{
   std::cout << "lit database" << '\n';
   for (auto it : _litDatabase)
      std::cout << '\t' << *(it.second) << '\n';
}

void ExpTrailer::saveState() 
{
   push();
//    _exp->checkCuts();
}
