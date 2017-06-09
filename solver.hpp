#ifndef __SOLVER_H
#define __SOLVER_H

#include <list>
#include <deque>
#include <functional>
#include <stdlib.h>

#include "handle.hpp"
#include "fail.hpp"
#include "engine.hpp"
#include "avar.hpp"
#include "acstr.hpp"
#include "cont.hpp"
#include "controller.hpp"
#include "reversible.hpp"


typedef std::reference_wrapper<std::function<void(void)>> Closure;
class Controller;

class CPSolver {
   Engine::Ptr                  _ctx;
   std::list<AVar::Ptr>       _iVars;
   std::list<Constraint::Ptr> _iCstr;
   std::deque<Closure>        _queue;
   bool                      _closed;
   long                  _afterClose;
   int                        _varId;
   int                          _nbc; // # choices
   int                          _nbf; // # fails
   int                          _nbs; // # solutions
   rev<Status>                   _cs;
   Controller*                 _ctrl;
public:
   template<typename T> friend class var;
   typedef handle_ptr<CPSolver> Ptr;
   CPSolver();
   ~CPSolver();
   Engine::Ptr context() { return _ctx;}
   void registerVar(AVar::Ptr avar);
   void schedule(std::function<void(void)>& cb) { _queue.emplace_back(cb);}
   Status status() const { return _cs;}
   Status propagate();
   Status add(Constraint::Ptr c);
   void close();
   void incrNbChoices() { _nbc += 1;}
   void incrNbSol()     { _nbs += 1;}
   void solveOne(std::function<void(void)> b);
   void solveAll(std::function<void(void)> b);
   template <class Body1,class Body2> void tryBin(Body1 left,Body2 right);
   //void tryBin(std::function<void(void)> left,std::function<void(void)> right);
   void fail();
   friend std::ostream& operator<<(std::ostream& os,const CPSolver& s) {
      return os << "CPSolver(" << &s << ")" << std::endl
                << "\t#choices   = " << s._nbc << std::endl
                << "\t#fail      = " << s._nbf << std::endl
                << "\t#solutions = " << s._nbs << std::endl;
   }
};

namespace Factory {
   inline CPSolver::Ptr makeSolver() { return handle_ptr<CPSolver>(new CPSolver);}
};

void* operator new  ( std::size_t count );

template <class Container,class FIt,class Body>
void withVarDo(Container& c,FIt it,Body b)
{
   if (it == c.end())
      return;
   auto& x = *it;
   b(x);
}

template <class Body1,class Body2>
void CPSolver::tryBin(Body1 left,Body2 right) 
{
   Cont::Cont* k = Cont::Cont::takeContinuation();
   if (k->nbCalls()==0) {
      _nbc++;
      _ctrl->addChoice(k);
      left();
   } else {
      Cont::letgo(k);
      _nbc++;
      _ctrl->trust();
      right();
   }
}

#endif
