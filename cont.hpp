/************************************************************************
 Mozilla Public License
 
 Copyright (c) 2012 NICTA, Laurent Michel and Pascal Van Hentenryck

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at http://mozilla.org/MPL/2.0/.

***********************************************************************/

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
   };
   void initContinuationLibrary(int *base);
   void shutdown();
   void letgo(Cont* c);
   char* getContBase();
};
