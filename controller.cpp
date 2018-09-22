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

#include "controller.hpp"
#include "cont.hpp"
#include <iostream>

Controller::Controller(Trailer::Ptr ctx)
   : _ctx(ctx) {}

DFSController::DFSController(Trailer::Ptr ctx)
   : Controller(ctx)
{
   _exitK = nullptr;
}

DFSController::~DFSController()
{
  std::cout << "DFSController::~DFSController" << std::endl;
  clear();
  Cont::letgo(_exitK);
}

void DFSController::start(Cont::Cont* k)
{
   _exitK = k;
}

void DFSController::addChoice(Cont::Cont* k)
{
   _cf.push(k);
   _ctx->push();
}

void DFSController::trust()
{
   _ctx->incMagic();
}

void DFSController::fail()
{
   while (_cf.size() > 0) {
      auto k = _cf.top();
      _cf.pop();
      _ctx->pop();
      if (k)
         k->call();
   }
   _exitK->call();
}

void DFSController::exit()
{
   while (!_cf.empty()) {
      Cont::letgo(_cf.top());
      _cf.pop();
   }
   _exitK->call();
}

void DFSController::clear()
{
   while (!_cf.empty()) {
      Cont::letgo(_cf.top());
      _cf.pop();
   }
}
