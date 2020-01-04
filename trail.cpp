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

void Trailer::resize()
{
   char* nb = (char*)realloc(_block,_bsz << 1);
   if (nb != _block) {
      std::stack<Entry*> ns;
      while(!_trail.empty()) {
         ns.push((Entry*)(((char*)_trail.top()) - _block + nb));
         _trail.pop();
      }
      while(!ns.empty()) {
         _trail.push(ns.top());
         ns.pop();
      }
   }
   _bsz <<= 1;
   _block = nb;
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
   long lvl = push();
   body();
   popToNode(lvl);
}
