#include "engine.hpp"

Engine::Engine()
   : _ctx(new Context),
     _store(new Storage(_ctx))
{
}

Engine::~Engine()
{
   std::cout << "Engine::~Engine" << std::endl;
   _ctx.dealloc();
   _store.dealloc();
}
