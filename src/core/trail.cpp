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
#include <assert.h>
#include <iostream>
#include "commandList.hpp"

#define TSIZE (1 << 28)

Trailer::Trailer()
    : _magic(-1)
{
   _block = (char*)malloc(TSIZE);
   _bsz = TSIZE;
   _btop = 0;
   _lastNode = 0;
   _enabled  = false;
}

Trailer::~Trailer()
{
   free(_block);
}

void Trailer::resize()
{
   std::cout  << "Trailer::resize to: " << (_bsz << 1) << "\n";
   char* nb = (char*)realloc(_block,_bsz << 1);
   if (nb != _block) {
      std::stack<Entry*> ns;
      while(!_trail.empty()) {
         Entry* e = _trail.top();
         Entry* ne = (Entry*)((char*)e - _block + nb);
         ne->relocate(nb - _block);
         ns.push(ne);
         _trail.pop();
      }
      while(!ns.empty()) {
         Entry* e = ns.top();
         _trail.push(e);
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
void Trailer::push(long nodeID)
{
    ++_magic;
    _tops.emplace(std::make_tuple(_trail.size(),_btop,nodeID));
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
   unsigned to;
   std::size_t mem;
   long node;
   std::tie(to,mem,node) = _tops.top();
   _tops.pop();
   while (_trail.size() != to) {
      assert(_trail.size() >= to);
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

//MemoryTrail::MemoryTrail()
//{
//}
//MemoryTrail::~MemoryTrail()
//{
//}
//void MemoryTrail::clear()
//{
//   return;
//}
//void MemoryTrail::saveState()
//{
//   return;
//}
//void MemoryTrail::restoreState()
//{
//   return;
//}
//void MemoryTrail::withNewState(const std::function<void(void)>& body)
//{
//   return;
//}
//unsigned int MemoryTrail::trailSize()
//{
//   return _trail.size();
//}
//void MemoryTrail::comply(MemoryTrail* other, CommandList* list)
//{
//   unsigned int from = list->memoryFrom();
//   unsigned int to = list->memoryTo();
//   for (auto i = from; i < to; i++) {
//      assert(other->at(i) != nullptr);
//      _trail.push_back(other->at(i));
//   }
//}
//void* MemoryTrail::at(unsigned int index) {
//   return _trail[index];
//}
