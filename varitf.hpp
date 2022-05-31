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
 *
 * Contributions by Waldemar Cruz, Rebecca Gentzel, Willem Jan Van Hoeve
 */

#ifndef __VARITF_H
#define __VARITF_H

#include "avar.hpp"
#include "store.hpp"
#include "solver.hpp"
#include "trailList.hpp"

/**
 * @brief Basic CP variable with a domain of arbitrary integers.
 *
 * The class inherits from the _abstract variable_ AVar and provides
 * the abstract API of the variable. The *specializations*  of this
 * template correspond to different implementations such as an actual
 * variable (`IntVarImpl`), or _views_ (e.g., Additive, multiplicative or flipping).
 */
template<> class var<int> : public AVar {
   friend class Storage;
private:
   int _id;
protected:
   /**
    * Saves the new identifier into the `_id` instance variable.
    * @param id the new identifier
    */ 
   void setId(int id) override { _id = id;}
public:
   /**
    * Looks up the  variable idendifier (signed 32 bit value, non-negative)
    * @return the variable identifier.
    */
   int getId() const noexcept { return _id;}
   /**
    * Ptr is the type for smart pointers to solver allocated variables.
    * This is the only type one should use to manipulate variabes (all
    * variables must be solver allocated via the Factory.)
    */
   typedef handle_ptr<var<int>> Ptr;
   /**
    * Retrieves the storage manager for this variable
    * @return a smart pointer to the memory manager.
    */
   virtual Storage::Ptr getStore() = 0;
   /**
    * Retrieves the solver that owns this variable.
    * @return a smart pointer to the solver owning this variable.
    */
   virtual CPSolver::Ptr getSolver() = 0;
   /**
    * Returns the smallest value present in the domain.
    * @return the smallest value
    */
   virtual int min() const  = 0;
   /**
    * Returns the largest value present in the domain.
    * @return the largest value
    */
   virtual int max() const  = 0;
   /**
    * Returns the size of the domain for this variable.
    * A size of 1 states that the variable is instantiated.
    * @return the domain size
    */
   virtual int size() const = 0;
   /**
    * Reports whether the variable is bound (domain size is 1)
    * @return true if and only if the variable is bound
    */
   virtual bool isBound() const = 0;
   /**
    * Reports whether a variable still appears in the domain.
    * @param v the value to test
    * @return true if and only if v is in the domain
    * @see size()
    */
   virtual bool contains(int v) const = 0;
   /**
    * Binds the variable to the given value. Namely, the domain
    * changes to contain only the specified value. All others are
    * removed
    * @param v the value to bind to the variable
    */
   virtual void assign(int v) = 0;
   /**
    * Removes a value from the domain of the variable. If the value
    * is present in the domain, the domain sizes drop by 1. Note that
    * this operation can raise a failure in case the domain becomes
    * empty.
    * @param v the value to remove
    */
   virtual void remove(int v) = 0;
   /**
    * Imposes a new minimum to the domain of this variable. This implies
    * the removal of every value below that new minimum. Note that
    * this operation can raise a failure in case the domain becomes
    * empty.
    * @param newMin is the new minimum
    */
   virtual void removeBelow(int newMin) = 0;
   /**
    * Imposes a new maximum to the domain of this variable. This implies
    * the removal of every value above that new maximum. Note that
    * this operation can raise a failure in case the domain becomes
    * empty.
    * @param newMax is the new maximum.
    */
   virtual void removeAbove(int newMax) = 0;
   /**
    * Simultaneously update the smallest and largest value in the domain.
    * This is equivalent to calling `removeMin(int)` and `removeMax(int)`
    * back to back. Note that
    * this operation can raise a failure in case the domain becomes
    * empty.
    * @param newMin is the new minimum
    * @param newMax is the new maximum
    */
   virtual void updateBounds(int newMin,int newMax) = 0;

   /**
    * Observer. This method registers a new observer with the variable
    * that will be notified whenever the domain of the variable gets bound
    * to a specific value. Note that the observer is a lambda that takes
    * no inputs and produces no outputs. It can access the variable and
    * query it to determine the actual value of the variable.
    * @param f is the observing lambda
    * @return adding an observer returns a pointer to an abstract datatype
    * that one can use to later *remove* the observer. This abstract value
    * has a single method to `detach` it from the variable is is attached to.
    * the primary use of this ability is to implement *watched literals*.
    */
   virtual TLCNode* whenBind(std::function<void(void)>&& f) = 0;
   /**
    * Observer. This method registers a new observer with the variable
    * that will be notified whenever the domain of the variable is updated 
    * and one of its bounds (`min` or `max`) change.
    * Note that the observer is a lambda that takes
    * no inputs and produces no outputs. It can access the variable and
    * query it to determine the actual value of the variable.
    * @param f is the observing lambda
    * @return adding an observer returns a pointer to an abstract datatype
    * that one can use to later *remove* the observer. This abstract value
    * has a single method to `detach` it from the variable is is attached to.
    * the primary use of this ability is to implement *watched literals*.
    */
   virtual TLCNode* whenBoundsChange(std::function<void(void)>&& f) = 0;
   /**
    * Observer. This method registers a new observer with the variable
    * that will be notified whenever the domain of the variable changes 
    * and a value is deleted anywhere in the domain.
    * Note that the observer is a lambda that takes
    * no inputs and produces no outputs. It can access the variable and
    * query it to determine the actual value of the variable.
    * @param f is the observing lambda
    * @return adding an observer returns a pointer to an abstract datatype
    * that one can use to later *remove* the observer. This abstract value
    * has a single method to `detach` it from the variable is is attached to.
    * the primary use of this ability is to implement *watched literals*.
    */
   virtual TLCNode* whenDomainChange(std::function<void(void)>&& f) = 0;
   /**
    * Call back. This method register the `propagate` method of the
    * specified constraint as a callback in case the domain of the variable
    * gets bound. It can be implemented in term of a call to
    * ```
    * return this->whenBind([]() { c->propagate();});
    * ```
    * @param c the constraint (propagator) to invoke the callback on.
    * @return Adding a callback amounts to adding an observer. Therefore
    * the method returns the same abstract value from the Observer method.
    * @see whenBind(std::function<void(void)>&& f)
    */ 
   virtual TLCNode* propagateOnBind(Constraint::Ptr c)          = 0;
   /**
    * Call back. This method register the `propagate` method of the
    * specified constraint as a callback in case at least one bound
    * of the domain of the variable
    * gets updated. It can be implemented in term of a call to
    * ```
    * return this->whenBoundsChange([]() { c->propagate();});
    * ```
    * @param c the constraint (propagator) to invoke the callback on.
    * @return Adding a callback amounts to adding an observer. Therefore
    * the method returns the same abstract value from the Observer method.
    * @see whenBoundsChange(std::function<void(void)>&& f)
    * @see Constraint
    */ 
   virtual TLCNode* propagateOnBoundChange(Constraint::Ptr c)   = 0;
   /**
    * Call back. This method register the `propagate` method of the
    * specified constraint as a callback in case the domain of the variable
    * gets updated. It can be implemented in term of a call to
    * ```
    * return this->whenDomainChange([]() { c->propagate();});
    * ```
    * @param c the constraint (propagator) to invoke the callback on.
    * @return Adding a callback amounts to adding an observer. Therefore
    * the method returns the same abstract value from the Observer method.
    * @see whenDomainChange(std::function<void(void)>&& f)
    * @see Constraint
    */ 
   virtual TLCNode* propagateOnDomainChange(Constraint::Ptr c ) = 0;
   /** 
    * Polymorphic variable printing.
    * This method gets used by the output operator on `const var<int>&`
    * @param os the stream to write to
    * @return the stream written to
    */ 
   virtual std::ostream& print(std::ostream& os) const = 0;
   friend std::ostream& operator<<(std::ostream& os,const var<int>& x) {
      return x.print(os);
   }
};


#endif
