#include "engine.hpp"

Engine::Engine()
    : _magic(-1)
{
   _block = (char*)malloc(1<<24);
   _bsz = (1 << 24);
   _btop = 0;
}

Engine::~Engine()
{
   free(_block);
}

void Engine::push()
{
    ++_magic;
    _tops.emplace(std::make_tuple(_trail.size(),_btop));
}
void Engine::pop()
{
   int to;
   std::size_t mem;
   std::tie(to,mem) = _tops.top();
   _tops.pop();
    while (_trail.size() != to) {
       Entry* entry = _trail.top();
       entry->restore();
       _trail.pop();
       entry->Entry::~Entry();
    }
    _btop = mem;
}
