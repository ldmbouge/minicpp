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

#include "cont.hpp"
#include "context.hpp"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <iomanip>

typedef struct  {
    ORUInt low;
    ORUInt high;
    ORUInt sz;
    ORUInt nbCont;
    Cont::Cont** pool;
} ContPool;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"

// [ldm] The routine is meant to operate over 32-bit words (4-bytes at a time) or 64-bit wide 
// datum. dest / src must be increased by the data item size.
static inline void fastmemcpy(register ORUInt* dest,register ORUInt* src,register size_t len)
{
    while (len) {
        *dest++ = *src++;
        len -= sizeof(ORUInt);
    }
}

#pragma clang diagnostic pop

namespace Cont {
    Cont::Cont() {
        _used   = 0;
        _start  = 0;
        _data   = 0;
        _length = 0;
        _field  = 0;
        _fieldId = nullptr;
        _cnt    = 1;
    }
    Cont::~Cont() {
        free(_data);
    }
    void Cont::saveStack(size_t len,void* s) 
    {
        if (_length!=len) {
            if (_length!=0) free(_data);
            _data = (char*)malloc(len);
        }
        fastmemcpy((ORUInt*)_data,(ORUInt*)s,len);
        _length = len;
        _start  = (char*)s;
    }
    void Cont::call() 
    {
#if defined(__x86_64__)
        struct Ctx64* ctx = &_target;
        ctx->rax = (long)this;
        restoreCtx(ctx,_start,_data,_length);
#else
        _used++;
        _longjmp(_target,(long)self); // dot not save signal mask --> overhead   
#endif
    }
    Cont* Cont::takeContinuation()
    {
        Cont* k = newCont();
#if defined(__x86_64__)
        struct Ctx64* ctx = &k->_target;
        Cont* resume = saveCtx(ctx,k);
        if (resume != 0) {
            resume->_used++;
            return resume;      
        } else return k;
#else
        int len = getContBase() - (char*)&k;
        k->saveStack(len,&k);
        register Cont* jmpval = (Cont*)_setjmp(k->_target);   
        if (jmpval != 0) {
            fastmemcpy(jmpval->_start,(ORUInt*)jmpval->_data,jmpval->_length);
            return jmpval;
        } else 
            return k;   
#endif
    }

  static __thread ContPool* pool = nullptr;
    inline static ContPool* instancePool() {
        if (!pool) {
            pool = new ContPool;
            pool->low = pool->high = pool->nbCont = 0;
            pool->sz = 1000;
            pool->pool = new Cont*[pool->sz];
        }
        return pool;
    }
  inline static void freePool() {
    delete[] pool->pool;
    delete pool;
    pool = nullptr;
  }
    Cont* Cont::newCont()
    {
        ContPool* pool = instancePool();
        Cont* rv = nullptr;
        if (pool->low == pool->high) {
            pool->nbCont += 1;
            rv = new Cont;
        } else {
            rv = pool->pool[pool->low];
            pool->low = (pool->low+1) % pool->sz;
        }   
        rv->_used   = 0;
        rv->_start  = 0;
        rv->_cnt    = 1;
        return rv;
    }
    Cont* Cont::grab() { ++_cnt;return this;}

    void letgo(Cont* c)
    {
        assert(c->_cnt > 0);
        if (--c->_cnt == 0) {
            ContPool* pool = instancePool();
            ORUInt next = (pool->high + 1) % pool->sz;
            if (next == pool->low) {
                free(c->_data);
                pool->nbCont -= 1;
                delete c;
                return;
            }
            pool->pool[pool->high] = c;
            pool->high = next;
        }
    }

    void shutdown()
    {
        ContPool* pool = instancePool();
        if (pool) {
            ORInt nb=0;
            for(ORInt k=pool->low;k != pool->high;) {
                delete pool->pool[k];
                k = (k+1) % pool->sz;
                nb++;
            }
            pool->low = pool->high = 0;
            std::cout << "released " << nb << " continuations out of "
		      << pool->nbCont << "..." << std::endl;
	    freePool();
        }	
    }
}
