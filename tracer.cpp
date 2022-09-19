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

#include "tracer.hpp"
#include "fail.hpp"

Tracer::Tracer(Trailer::Ptr trail, MemoryTrail* memoryTrail)
   : _trail(trail), _memoryTrail(memoryTrail) { }
unsigned int Tracer::currentNode() {
   return _lastNodeID;
}
void Tracer::fail() {
   _lastNodeID++;
}
unsigned int Tracer::pushNode() {
//   assert(_commands->size() == _trailStack->size());
//   _trailStack->pushNode(_lastNodeID);
   _commands->pushList(_lastNodeID, _memoryTrail->trailSize());
   _trail->incMagic();
   _lastNodeID++;

//   assert(_commands->size() == _trailStack->size());
   _level += 1;
   return _lastNodeID - 1;
}
CommandList* Tracer::popNode() {
//   _trailStack->popNode();
   CommandList* list = _commands->popList();
   _trail->incMagic();
//   assert(_commands->size() == _trailStack->size());
   return list;
}
void Tracer::popToNode(unsigned int nodeID) {
//   _trailStack->popNode(nodeID);
   _trail->incMagic();
}
void Tracer::trust() {
   _level += 1;
}
unsigned int Tracer::level() {
   return _level;
}
void Tracer::reset() {
//   assert(_commands->size() == _trailStack->size());
//   while (!_trailStack->empty()) {
//      _trailStack->popNode();
      CommandList* list = _commands->popList();
      list->letgo();
//   }
   assert(_level == 0);
   pushNode();
}
Trailer::Ptr Tracer::trail() {
   return _trail;
}
MemoryTrail* Tracer::memoryTrail() {
   return _memoryTrail;
}
void Tracer::addCommand(ConstraintDesc::Ptr command) {
   _commands->addCommand(command);
}
Checkpoint* Tracer::captureCheckpoint() {
   _commands->setMemoryTail(_memoryTrail->trailSize());
   Checkpoint* checkpoint = new Checkpoint(_commands, _memoryTrail);
   checkpoint->setLevel(_level);
   checkpoint->setNodeID(pushNode());
   return checkpoint;
}
bool Tracer::restoreCheckpoint(Checkpoint* checkpoint, CPSolver* solver) {
//   assert(_commands->size() == _trailStack->size());
   CommandStack* restoreStack = checkpoint->commands();
   unsigned int currentSize = _commands->size();
   unsigned int restoreToSize = restoreStack->size();
   unsigned int i = _commands->sharedPrefixSize(restoreStack);
   while (i != currentSize--) {
//      _trailStack->pop();
      CommandList* list = _commands->popList();
//      assert(_commands->size() == _trailStack->size());
      list->letgo();
   }
   bool failed = false;
   if (failed) return false; //TODO: How do we do this here?

   _trail->incMagic();
   for (; i < restoreToSize; i++) {
//      assert(_commands->size() == _trailStack->size());
      CommandList* list = restoreStack->peekAt(i);
//      _trailStack->pushNode(list->nodeID());
      _memoryTrail->comply(checkpoint->memoryTrail(), list);
      _trail->incMagic();
      TRYFAIL
         struct CommandNode* node = list->head();
         while (node != nullptr) {
            solver->post(node->_constraint);
            node = node->_next;
         }
         _commands->pushCommandList(list->grab());
      ONFAIL
//         _trailStack->popNode();
         return false;
      ENDFAIL
   }
   _level = checkpoint->level();
//   return enforceObjective();
}
