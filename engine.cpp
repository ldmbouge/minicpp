#include "engine.hpp"

Engine::Engine()
    : _magic(-1)
{    
}

void Engine::push()
{
    ++_magic;
    _tops.emplace(_trail.size());
}
void Engine::pop()
{
    int to = _tops.top();
    _tops.pop();
    while (_trail.size() != to) {
        _trail.top()->restore();
        _trail.pop();
    }
}
