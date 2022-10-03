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

#include "commandList.hpp"

static __thread struct CommandListPool* pool = nullptr;

CommandList* CommandList::grab() {
   _refCount++;
   return this;
}
void CommandList::letgo() {
   assert(_refCount > 0);
   if (--_refCount == 0) {
      _head = nullptr;
      _nodeID = -1;
      struct CommandListPool* pool = CommandList::instancePool();
      unsigned int next = (pool->_high + 1) % pool->_maxSize;
      if (next != pool->_low) {
         pool->_pool[pool->_high] = this;
         pool->_size++;
         pool->_high = next;
      }
   }
}
void CommandList::insert(ConstraintDesc::Ptr c) {
   assert(!_frozen);
   struct CommandNode* newNode = (struct CommandNode*)malloc(sizeof(struct CommandNode));
   newNode->_constraint = c;
   newNode->_next = _head;
   _head = newNode;
}
//void CommandList::setMemoryTo(unsigned int tail) {
//   assert(!_frozen);
//   _to = tail;
//}
//unsigned int CommandList::memoryFrom() {
//   return _from;
//}
//unsigned int CommandList::memoryTo() {
//   return _to;
//}
void CommandList::freeze() {
   _frozen = true;
}
bool CommandList::frozen() {
   return _frozen;
}
struct CommandNode* CommandList::head() {
   return _head;
}
unsigned int CommandList::length() {
   unsigned int num = 0;
   struct CommandNode* current = _head;
   while (current) {
      num++;
      current = current->_next;
   }
   return num;
}
long CommandList::nodeID() {
   return _nodeID;
}

CommandList* CommandList::newCommandList(unsigned int nodeID) {//, unsigned int from, unsigned int to) {
   CommandListPool* pool = CommandList::instancePool();
   CommandList* returnValue = nullptr;
   if (pool->_low == pool->_high) {
      returnValue = new CommandList(nodeID);//, from, to);
   } else {
      returnValue = pool->_pool[pool->_low];
      pool->_low = (pool->_low + 1) % pool->_maxSize;
      pool->_size--;
      returnValue->_refCount = 1;
      returnValue->_nodeID = nodeID;
//      returnValue->_from = from;
//      returnValue->_to = to;
      returnValue->_frozen = false;
   }
   return returnValue;
}
struct CommandListPool* CommandList::instancePool() {
   if (!pool) {
      pool = (struct CommandListPool*)malloc(sizeof(struct CommandListPool));
      pool->_low = pool->_high = pool->_size = 0;
      pool->_maxSize = 8192;
      pool->_pool = (CommandList**)malloc(sizeof(CommandList*)*pool->_maxSize);
   }
   return pool;
}
void CommandList::print() {
   std::cout << "  CommandList {\n";
   struct CommandNode* node = _head;
   while (node != nullptr) {
      std::cout << "    ";
      printCstrDesc(node->_constraint);
      node = node->_next;
   }
   std::cout << "  }\n";
}
bool operator==(const CommandList& left, const CommandList& right) {
   return left._nodeID == right._nodeID;// && left._from == right._from && left._to == right._to;
}

void CommandStack::pushList(unsigned int nodeID) {//, unsigned int memoryHead) {
   if (_size >= _maxSize) {
      _table = (CommandList**)realloc(_table, sizeof(CommandList*)*_maxSize*2);
      _maxSize <<= 1;
   }
   //assert(_size == 0 || memoryHead >= _table[_size-1]->memoryTo());
   _table[_size++] = CommandList::newCommandList(nodeID);//, memoryHead, memoryHead);
}
void CommandStack::pushCommandList(CommandList* list) {
   if (_size >= _maxSize) {
      _table = (CommandList**)realloc(_table, sizeof(CommandList*)*_maxSize*2);
      _maxSize <<= 1;
   }
//   assert(_size == 0 || list->memoryFrom() >= _table[_size-1]->memoryTo());
   _table[_size++] = list;
}
void CommandStack::addCommand(ConstraintDesc::Ptr constraint) {
   CommandList* commandList = _table[_size-1];
   if (commandList->frozen()) {
      _table[_size-1] = new CommandList(*commandList);
      commandList->letgo();
   }
   _table[_size-1]->insert(constraint);
}
//void CommandStack::setMemoryTail(unsigned int memoryTail) {
//   if (_size >= 1) {
//      if (_table[_size-1]->frozen()) {
//         CommandList* old = _table[_size - 1];
//         _table[_size - 1] = new CommandList(*old);
//         old->letgo();
//      }
//      _table[_size - 1]->setMemoryTo(memoryTail);
//   }
//}
CommandList* CommandStack::popList() {
   assert(_size > 0);
   return _table[--_size];
}
CommandList* CommandStack::peekAt(unsigned int index) {
   return _table[index];
}
int CommandStack::size() {
   return _size;
}
unsigned int CommandStack::sharedPrefixSize(CommandStack* other) {
   unsigned int i;
   unsigned int minSize = std::min(_size, other->size());
   for (i = 0; i < minSize && *(_table[i]) == *(other->peekAt(i)); i++);
   return i;
}
void CommandStack::print() {
   std::cout << "CommandStack {\n";
   for (int i = 0; i < _size; i++)
      _table[i]->print();
   std::cout << "}\n";
}

//MemoryTrail* Checkpoint::memoryTrail() {
//   return _memoryTrail;
//}
unsigned int Checkpoint::size() {
   return _path->size();
}
void Checkpoint::setNodeID(unsigned int nodeID) {
   _nodeID = nodeID;
}
unsigned int Checkpoint::nodeID() {
   return _nodeID;
}
void Checkpoint::setLevel(unsigned int level) {
   _level = level;
}
CommandStack* Checkpoint::commands() {
   return _path;
}
unsigned int Checkpoint::level() {
   return _level;
}
Checkpoint* Checkpoint::grab() {
   _refCount++;
   return this;
}
void Checkpoint::letgo() {
   assert(_refCount > 0);
//   if (--_refCount == 0) {
//      _memoryTrail->clear();
//   }
}
