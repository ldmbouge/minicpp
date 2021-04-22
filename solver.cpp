/*
 * mini-cp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License  v3
 * as published by the Free Software Foundation.
 *
 * mini-cp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY.
 * See the GNU Lesser General Public License  for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with mini-cp. If not, see http://www.gnu.org/licenses/lgpl-3.0.en.html
 *
 * Copyright (c)  2018. by Laurent Michel, Pierre Schaus, Pascal Van Hentenryck
 */

#include "solver.hpp"
#include "cont.hpp"
#include "controller.hpp"
#include <assert.h>
#include <iostream>
#include <iomanip>
#include <typeindex>

CPSolver::CPSolver()
    : _sm(new Trailer),
      _store(new Storage(_sm)),
      _currCon(nullptr),
      _es(nullptr)
{
    _varId  = 1;
    _nbc = _nbf = _nbs = 0;
}

CPSolver::CPSolver(Trailer::Ptr t)
    : _sm(t),
      _store(new Storage(_sm)),
      _currCon(nullptr),
      _es(nullptr)
{
    _varId  = 1;
    _nbc = _nbf = _nbs = 0;
}

CPSolver::~CPSolver()
{
   _iVars.clear();
   _store.dealloc();
   _sm.dealloc();
   std::cout << "CPSolver::~CPSolver(" << this << ")" << std::endl;
   //Cont::shutdown();
}

void CPSolver::post(Constraint::Ptr c,bool enforceFixPoint)
{
    if (!c) return;
    _cons.push_back(c);
    _currCon = c;
    c->post();
    if (enforceFixPoint) 
        fixpoint();    
}

void CPSolver::registerVar(AVar::Ptr avar)
{
   avar->setId(_varId++);
   _iVars.push_back(avar);
}

std::vector<handle_ptr<var<int>>> CPSolver::intVars()
{
   std::vector<handle_ptr<var<int>>> res;
   var<int>* p;
   for(auto v : _iVars){
      if ((p = v->getIntVar())) 
         res.push_back(p);
   }
   return res;
}

void CPSolver::notifyFixpoint()
{
   for(auto& body : _onFix)
      body();
}   

void CPSolver::fixpoint()
{
   try {
      notifyFixpoint();
      while (!_queue.empty()) {
         auto c = _queue.deQueue();
         c->setScheduled(false);
         if (c->isActive()) {
            _currCon = c;
            c->propagate();
         }
      }
      assert(_queue.size() == 0);
      _status = Suspend;
   } catch(Status x) {
      _status = x;  // should always be a Failure status
      while (!_queue.empty()) {
         _queue.deQueue()->setScheduled(false);
      }
      //_queue.clear();
      assert(_queue.size() == 0);
      _nbf += 1;
      throw x;
   }
}

// [LDM] This is for debugging purposes. Don't include when using valgrind

/*
#if defined(__APPLE__)
void* operator new  ( std::size_t count )
{
   return malloc(count);
}
#endif
*/
