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

#ifndef __CONT_H
#define __CONT_H

typedef unsigned int ORUInt;
typedef int ORInt;

#include <stdlib.h>
#include "context.hpp"

namespace Cont {
   class Cont {
#if defined(__x86_64__)
      struct Ctx64   _target __attribute__ ((aligned(16)));
#else
      jmp_buf _target;
#endif
      size_t _length;
      char* _start;
      char* _data;
      int _used;
      ORInt _cnt;

      ORInt _field;
      void* _fieldId;
      static Cont* newCont();
   public:
      Cont();
      ~Cont();
      void saveStack(size_t len,void* s);
      void call(); 
      ORInt nbCalls() const { return _used;}
      Cont* grab();
      static Cont* takeContinuation();
      friend void letgo(Cont* c);
      friend void shutdown();
   };
   void initContinuationLibrary(int *base);
   void shutdown();
   void letgo(Cont* c);
   char* getContBase();
};

#endif
