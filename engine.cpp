#include "engine.hpp"

Engine::Engine()
   : _ctx(new Context),
     _store(new Storage(_ctx))
{
}

Engine::~Engine()
{
}
