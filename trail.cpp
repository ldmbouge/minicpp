#include "trail.hpp"

Trailer::Trailer()
    : _magic(-1)
{
   _block = (char*)malloc(1<<24);
   _bsz = (1 << 24);
   _btop = 0;
   _lastNode = 0;
}

Trailer::~Trailer()
{
   free(_block);
}

long Trailer::push()
{
    ++_magic;
    long rv = ++_lastNode;
    _tops.emplace(std::make_tuple(_trail.size(),_btop,rv));
    return rv;
}
void Trailer::popToNode(long node)
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

void Trailer::pop()
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

void Trailer::clear()
{
  while (_tops.size() > 0) 
    pop();  
}

void Trailer::saveState() 
{
   push();
}
void Trailer::restoreState() 
{
   pop();
}
void Trailer::withNewState(const std::function<void(void)>& body) 
{
   int lvl = push();
   body();
   popToNode(lvl);
}
