#include "controller.hpp"

DFSController::DFSController(Engine::Ptr ctx)
   : Controller(ctx)
{
   _exitK = nullptr;
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
