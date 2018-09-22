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

#ifndef __CONTROLLER_H
#define __CONTROLLER_H

#include "trail.hpp"
#include <stack>

namespace Cont {
   class Cont;
};

class Controller {
protected:
   Trailer::Ptr _ctx;
public:
   Controller(Trailer::Ptr ctx);
   virtual ~Controller() {}
   virtual void addChoice(Cont::Cont* k) = 0;
   virtual void fail() = 0;
   virtual void trust() = 0;
   virtual void start(Cont::Cont* k) = 0;
   virtual void exit() = 0;
   virtual void clear() = 0;
};

class DFSController :public Controller {
   std::stack<Cont::Cont*> _cf;
   Cont::Cont* _exitK;
public:
   DFSController(Trailer::Ptr ctx);
   ~DFSController();
   void start(Cont::Cont* k);
   void addChoice(Cont::Cont* k);
   void trust();
   void fail();
   void exit();
   void clear();
};

#endif
