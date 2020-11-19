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

#ifndef __AVAR_H
#define __AVAR_H

#include <memory>
#include "handle.hpp"

struct IntNotifier;
template<typename T> class var;
template<> class var<int>;

class AVar {
protected:
    virtual void setId(int id) = 0;
    friend class CPSolver;
public:
    typedef handle_ptr<AVar> Ptr;
    AVar() {}
    virtual ~AVar() {}
    virtual var<int>* getIntVar() {return nullptr;}
    virtual int getId() const = 0;
    virtual IntNotifier* getListener() const = 0;
    virtual void setListener(IntNotifier*) = 0;
};


template<typename T> class var {};

#endif
