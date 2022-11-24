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

Tracer::Tracer(Trailer::Ptr trail/*, MemoryTrail* memoryTrail*/)
   : _trail(trail)/*, _memoryTrail(memoryTrail)*/ {
   _commands = new CommandStack(32);
   _commands->pushList(_lastNodeID);
   _lastNodeID++;
   _level = 1;
}
unsigned int Tracer::currentNode() {
   return _lastNodeID;
}
void Tracer::fail() {
   _lastNodeID++;
}
unsigned int Tracer::pushNode() {
   _commands->pushList(_lastNodeID);/*, _memoryTrail->trailSize()*/
   _lastNodeID++;
   _trail->push();
   assert(_trail->depth() <= 1 || _commands->size() == _trail->depth());
   _level += 1;
   return _lastNodeID - 1;
}
CommandList* Tracer::popNode() {
   _trail->pop();
   _trail->incMagic();
   CommandList* list = _commands->popList();
   assert(_commands->size() == _trail->depth());
   return list;
}
void Tracer::trust() {
   _level += 1;
}
unsigned int Tracer::level() {
   return _level;
}
Trailer::Ptr Tracer::trail() {
   return _trail;
}
//MemoryTrail* Tracer::memoryTrail() {
//   return _memoryTrail;
//}
void Tracer::addCommand(ConstraintDesc::Ptr command) {
   _commands->addCommand(command);
}
std::shared_ptr<Checkpoint> Tracer::captureCheckpoint() {
   std::shared_ptr<Checkpoint> checkpoint (new Checkpoint(_commands));//, _memoryTrail);
   checkpoint->setLevel(_level);
   checkpoint->setNodeID(pushNode());
   return checkpoint;
}
bool Tracer::restoreCheckpoint(std::shared_ptr<Checkpoint> checkpoint, CPSemSolver::Ptr solver) {
   solver->startRestore();
   CommandStack* restoreStack = checkpoint->commands();
   unsigned int currentSize = _commands->size();
   unsigned int restoreToSize = restoreStack->size();
   unsigned int i = _commands->sharedPrefixSize(restoreStack);
   while (i != currentSize--) {
      _trail->pop();
      CommandList* list = _commands->popList();
      list->letgo();
   }

   _trail->incMagic();
   for (; i < restoreToSize; i++) {
      assert(_commands->size() == _trail->depth());
      CommandList* list = restoreStack->peekAt(i);
      _trail->push(list->nodeID());
      _commands->pushList(list->nodeID());
      TRYFAIL
         struct CommandNode* node = list->head();
         while (node != nullptr) {
            solver->post(node->_constraint);
            node = node->_next;
         }
      ONFAIL
         _trail->pop();
         _commands->popList();
         assert(_trail->depth() <= 1 || _commands->size() == _trail->depth());
         solver->endRestore();
         return false;
      ENDFAIL
   }
   _level = checkpoint->level();
   solver->endRestore();
   return true;
}
