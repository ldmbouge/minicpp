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

#ifndef __CONTEXT_H
#define __CONTEXT_H

namespace Cont {
   class Cont;

#if defined(__x86_64__)
   struct Ctx64 {
      long rax;
      long rbx;
      long rcx;
      long rdx;
      long rdi;
      long rsi;
      long rbp;
      long rsp;
      long r8;
      long r9;
      long r10;
      long r11;
      long r12;
      long r13;
      long r14;
      long r15;
      long   rip;
      unsigned int   pad; // alignment padding.
      unsigned int   mxcsr;
      double xmm0[2];
      double xmm1[2];
      double xmm2[2];
      double xmm3[2];
      double xmm4[2];
      double xmm5[2];
      double xmm6[2];
      double xmm7[2];
      double xmm8[2];
      double xmm9[2];
      double xmm10[2];
      double xmm11[2];
      double xmm12[2];
      double xmm13[2];
      double xmm14[2];
      double xmm15[2];
      char   fpu[108];
   };
   __attribute__((noinline)) Cont* saveCtx(struct Ctx64* ctx,Cont* k);
   __attribute__((noinline)) Cont* restoreCtx(struct Ctx64* ctx,char* start,char* data,size_t length);
#else
#include <setjmp.h>
#endif
}

#endif
