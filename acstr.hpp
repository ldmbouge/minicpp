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

#ifndef __ACSTR_H
#define __ACSTR_H

#include "handle.hpp"
#include "trailable.hpp"

class CPSolver;
/**
 * @brief This is an abstract constraint.
 * 
 * The class supports the API to schedule (or even prioritize) constraints.
 * Subclasses take care of the actual propagation work specific to the combinatorial
 * structures they represent.
 */
class Constraint {
private:
   bool     _scheduled;
   unsigned char _prio;
   trail<bool> _active;
public:
   /**
    * @brief Low priority level for the propagator (2 levels only)
    */
   static const unsigned char CLOW  = 0;
   /**
    * @brief High priority level for the propagator (2 levels only)
    */
   static const unsigned char CHIGH = 1;
   /**
    * @brief Smart pointer type to an abstract Constraint 
    */
   typedef handle_ptr<Constraint> Ptr;
   /**
    * Constructor. Allocates a constraint on the given solver
    * @param cp the solver that will own this new constraint
    */
   Constraint(handle_ptr<CPSolver> cp);
   virtual ~Constraint() {}
   /**
    * @brief post API called when a constraint is added
    */
   virtual void post() = 0;
   /**
    * @brief propagate API called when an event (AC3) occurs on a variable in the scope of this constraint
    */
   virtual void propagate() {}
   /**
    * @brief Printing. Convenience printing API
    * @param os the stream the constraint is to be printed to
    */
   virtual void print(std::ostream& os) const {}
   /**
    * Sets the priority of the constraint to a new value from \f$\{0,1\}\f$
    * @param p the new priority
    */
   void setPriority(unsigned char p) { _prio = p;}
   /**
    * Retrieves the priority currently assigned to this constraint.
    * @return the assigned priority (either 0 or 1)
    */
   unsigned char getPriority() const { return _prio;}
   /**
    * Tags the constraint as currently scheduled (it is in a propagation queue)
    * @param s is true if the constraint is scheduled. False otherwise.
    */
   void setScheduled(bool s) { _scheduled = s;}
   /**
    * Retrieves the current `scheduled` flag for this constraint.
    * @return true if and only if the constraint is scheduled.
    */    
   bool isScheduled() const  { return _scheduled;}
   /**
    * Change the `active` flag of the constraint. Inactive constraints are skipped during propagation.
    * @param a the active flag to be set.
    */
   void setActive(bool a)    { _active = a;}
   /**
    * Retrieves the `active` flag of the constraint.
    * @return true if and only if the constraint is currently active.
    */
   bool isActive() const     { return _active;}
};

class ConstraintDesc {
public:
   typedef handle_ptr<ConstraintDesc> Ptr;
   ConstraintDesc();
   virtual Constraint* create() = 0;
};

/**
 * @brief This is an abstract Objective function. It is subclassed for `min` and `max`.
 *
 */
class Objective {
public:
   typedef handle_ptr<Objective> Ptr;
   /**
    * API called to report that the objective function has been tightened (better incumbent).
    */
   virtual void tighten() = 0;
   /**
    * Queries and return the current value of the (integer) objective function..
    * @return the current value of the objective.
    */
   virtual int value() const = 0;
};;

#endif
