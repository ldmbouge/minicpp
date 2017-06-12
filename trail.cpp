#include "trail.hpp"

Context::Context()
    : _magic(-1)
{
   _block = (char*)malloc(1<<24);
   _bsz = (1 << 24);
   _btop = 0;
   _lastNode = 0;
}

Context::~Context()
{
   free(_block);
}

long Context::push()
{
    ++_magic;
    long rv = ++_lastNode;
    _tops.emplace(std::make_tuple(_trail.size(),_btop,rv));
    return rv;
}
void Context::popToNode(long node)
{
  while (true) {
    int to;
    std::size_t mem;
    long nodeId;
    std::tie(to,mem,nodeId) = _tops.top();
    pop();
    if (nodeId == node)
      break;
  }  
}

void Context::pop()
{
   int to;
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
}

void Context::clear()
{
  while (_tops.size() > 0) 
    pop();  
}
