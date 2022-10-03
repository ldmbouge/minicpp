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

#ifndef commandList_hpp
#define commandList_hpp

#include "constraint.hpp"

struct CommandNode {
  ConstraintDesc::Ptr _constraint;
  struct CommandNode* _next;
};

class CommandList;

struct CommandListPool {
  unsigned int _low;
  unsigned int _high;
  unsigned int _maxSize;
  unsigned int _size;
  CommandList** _pool;
};

class CommandList {
  struct CommandNode* _head;
  long _nodeID;
//  unsigned int     _from;
//  unsigned int       _to;
  unsigned int _refCount;
  bool  _frozen;
public:
  CommandList(long nodeID) : //, unsigned int from, unsigned int to) :
    _head(nullptr), _nodeID(nodeID), /*_from(from), _to(to),*/ _refCount(1), _frozen(false) { }
  CommandList(const CommandList& c) :
    _head(c._head), _nodeID(c._nodeID), /*_from(c._from), _to(c._to),*/ _refCount(c._refCount), _frozen(false) { }
  CommandList* grab();
  void letgo();
  void insert(ConstraintDesc::Ptr c);
//  void setMemoryTo(unsigned int tail);
//  unsigned int memoryFrom();
//  unsigned int memoryTo();
  void freeze();
  bool frozen();
  struct CommandNode* head();
  unsigned int length();
  long nodeID();
  void print();
  static CommandList* newCommandList(unsigned int nodeID);//, unsigned int from, unsigned int to);
  static struct CommandListPool* instancePool();
  friend bool operator==(const CommandList& left, const CommandList& right);
};

class CommandStack {
  CommandList** _table;
  int _size;
  int _maxSize;
public:
  CommandStack(unsigned int maxSize) :
    _table((CommandList**)malloc(sizeof(CommandList*)*maxSize)), _size(0), _maxSize(maxSize) { }
  void pushList(unsigned int nodeID);//, unsigned int memoryHead);
  void pushCommandList(CommandList* list);
  void addCommand(ConstraintDesc::Ptr constraint);
  void setMemoryTail(unsigned int memoryTail);
  CommandList* popList();
  CommandList* peekAt(unsigned int index);
  int size();
  unsigned int sharedPrefixSize(CommandStack* other);
  void print();
};

class Checkpoint {
  CommandStack* _path;
  unsigned int _nodeID;
  unsigned int _refCount;
  //MemoryTrail* _memoryTrail;
  unsigned int _level;
public:
  Checkpoint(CommandStack* commands) : //, MemoryTrail* memoryTrail) :
    _nodeID(-1), _refCount(1)/*, _memoryTrail(memoryTrail)*/, _level(-1) {
    _path = new CommandStack(64);
    unsigned int stackSize = commands->size();
    for (unsigned int i = 0; i < stackSize; i++) {
//      assert(i == 0 || commands->peekAt(i)->memoryFrom() >= commands->peekAt(i-1)->memoryTo());
      CommandList* list = commands->peekAt(i);
      list->grab();
      list->freeze();
      _path->pushCommandList(list);
    }
  }
  //MemoryTrail* memoryTrail();
  unsigned int size();
  void setNodeID(unsigned int nodeID);
  unsigned int nodeID();
  void setLevel(unsigned int level);
  unsigned int level();
  CommandStack* commands();
  Checkpoint* grab();
  void letgo();
};

#endif
