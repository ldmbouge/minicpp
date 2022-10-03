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

#ifndef __TRACER_H
#define __TRACER_H

#include "commandList.hpp"
#include "solver.hpp"

class Tracer {
   Trailer::Ptr _trail;
   unsigned int _lastNodeID;
   CommandStack* _commands;
   unsigned int _level;
public:
   Tracer(Trailer::Ptr trail);//, MemoryTrail* memoryTrail);
   unsigned int currentNode();
   void fail();
   unsigned int pushNode();
   CommandList* popNode();
   void trust();
   unsigned int level();
   Trailer::Ptr trail();
   void addCommand(ConstraintDesc::Ptr command);
   std::shared_ptr<Checkpoint> captureCheckpoint();
   bool restoreCheckpoint(std::shared_ptr<Checkpoint> checkpoint, CPSemSolver::Ptr solver);
};

#endif
