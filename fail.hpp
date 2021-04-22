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

#ifndef __FAIL_H
#define __FAIL_H

#include <cassert>

enum StatusType { Success,Failure,Suspend,BackJump };

enum FailExpl { EQL, RM, UB, LB };  // equal; removed; upper bound; lower bound

struct Status {
    unsigned int _code;
    Status(unsigned int t, unsigned int d=0) : _code( (d << 2) | t) {}
    Status(Status& s) : _code(s._code) {}
    Status() : _code(2) {}
    Status type() {
        switch (0b11 & _code) {
            case 0: return Success;
            case 1: return Failure;
            case 2: return Suspend;
            case 3: return BackJump;
            default: assert(false);
        }
    }
    unsigned int depth() { return (_code >> 2);}
    bool operator==(StatusType s) { return (0b11 & _code) == s;}
};

static inline void failNow()
{
    // throw Failure;
    throw Status(1);
}

static inline void backJumpTo(unsigned int depth)
{
    throw Status(3, depth);
}

#endif
