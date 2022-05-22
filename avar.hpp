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
/**
 * Abstract variable for the solver. It provides a naming capability
 * and an abstract pointer type.
 */
class AVar {
protected:
  /**
   * Set the identifier of the variable to a new value
   * @param id the new identifier
   */
    virtual void setId(int id) = 0;
    friend class CPSolver;
public:
  /**
   * Ptr is a smart pointer to a solver managed block of memory.
   */
    typedef handle_ptr<AVar> Ptr;
};

/**
 * `var<T>` is a templated type meant to represent variables with a domain `T`
 * @param `T` the type of values held in the domain.
 * @see var<int>, var<bool>
 */ 
template<typename T> class var {};

#endif
