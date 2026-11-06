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

#ifndef __CONSTRAINT_H
#define __CONSTRAINT_H

#include <set>
#include <array>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <cassert>
#include <stdint.h>
#include "matrix.hpp"
#include "intvar.hpp"
#include "acstr.hpp"
#include "global_constraints/sum.hpp"
#include "global_constraints/clause.hpp"
#include "global_constraints/alldifferent.hpp"
#include "global_constraints/circuit.hpp"
#include "global_constraints/element.hpp"
#include "global_constraints/maximum.hpp"

/**
 * @brief Equality constraint: x == c
 * @see Constraint::Ptr Factory::operator==(var<int>::Ptr x,const int c)
 */
class EQc : public Constraint { // x == c
   var<int>::Ptr _x;
   int           _c;
public:
   EQc(var<int>::Ptr x,int c) : Constraint(x->getSolver()),_x(x),_c(c) {}
   void post() override;
};

class EQcDesc : public ConstraintDesc {
   var<int>::Ptr _x;
   int           _c;
public:
   EQcDesc(var<int>::Ptr x,int c) : _x(x), _c(c) {}
   EQc* create() override;
   EQcDesc* clone() override;
   void print(std::ostream& os) const override;
};

class NEQc : public Constraint { // x != c
   var<int>::Ptr _x;
   int           _c;
public:
   NEQc(var<int>::Ptr x,int c) : Constraint(x->getSolver()),_x(x),_c(c) {}
   void post() override;
};

class NEQcDesc : public ConstraintDesc {
   var<int>::Ptr _x;
   int           _c;
public:
   NEQcDesc(var<int>::Ptr x,int c) : _x(x), _c(c) {}
   NEQc* create() override;
   NEQcDesc* clone() override;
   void print(std::ostream& os) const override;
};

class EQBinBC : public Constraint { // x == y + c
   var<int>::Ptr _x,_y;
   int _c;
public:
   EQBinBC(var<int>::Ptr x,var<int>::Ptr y,int c)
      : Constraint(x->getSolver()),_x(x),_y(y),_c(c) {}
   void post() override;
};


class EQTernBC : public Constraint { // x == y + z
  var<int>::Ptr _x,_y,_z;
public:
   EQTernBC(var<int>::Ptr x,var<int>::Ptr y,var<int>::Ptr z)
       : Constraint(x->getSolver()),_x(x),_y(y),_z(z) {}
   void post() override;
};

class EQTernBCbool : public Constraint { // x == y + b
  var<int>::Ptr _x,_y;
  var<bool>::Ptr _b;
public:
  EQTernBCbool(var<int>::Ptr x,var<int>::Ptr y,var<bool>::Ptr b)
    : Constraint(x->getSolver()),_x(x),_y(y),_b(b) {}
  void post() override;
};

class NEQBinBC : public Constraint { // x != y + c
   var<int>::Ptr _x,_y;
   int _c;
   trailList<Constraint::Ptr>::revNode* hdl[2];
   void print(std::ostream& os) const override;
public:
   NEQBinBC(var<int>::Ptr& x,var<int>::Ptr& y,int c)
      : Constraint(x->getSolver()), _x(x),_y(y),_c(c) {}
   void post() override;
};

class NEQBinBCLight : public Constraint { // x != y + c
   var<int>::Ptr _x,_y;
   int _c;
   void print(std::ostream& os) const override;
public:
   NEQBinBCLight(var<int>::Ptr& x,var<int>::Ptr& y,int c=0)
      : Constraint(x->getSolver()), _x(x),_y(y),_c(c) {}
   void post() override;
   void propagate() override;
};

class EQBinDC : public Constraint { // x == y + c
   var<int>::Ptr _x,_y;
   int _c;
public:
   EQBinDC(var<int>::Ptr& x,var<int>::Ptr& y,int c)
      : Constraint(x->getSolver()), _x(x),_y(y),_c(c) {}
   void post() override;
   void propagate() override;
};

class Conjunction :public Constraint { // z == x && y
   var<bool>::Ptr _z,_x,_y;
public:
   Conjunction(var<bool>::Ptr z,var<bool>::Ptr x,var<bool>::Ptr y)
      : Constraint(x->getSolver()),_z(z),_x(x),_y(y) {}
   void post() override;
   void propagate() override;
};

class LessOrEqual :public Constraint { // x <= y
   var<int>::Ptr _x,_y;
public:
   LessOrEqual(var<int>::Ptr x,var<int>::Ptr y)
      : Constraint(x->getSolver()),_x(x),_y(y) {}
   void post() override;
   void propagate() override;
};

class IsEqual : public Constraint { // b <=> x == c
   var<bool>::Ptr _b;
   var<int>::Ptr _x;
   int _c;
public:
   IsEqual(var<bool>::Ptr b,var<int>::Ptr x,int c)
      : Constraint(x->getSolver()),_b(b),_x(x),_c(c) {}
   void post() override;
   void propagate() override;
};

class XOR : public Constraint { // b <=> x xor y
   var<bool>::Ptr _b, _x, _y;
public:
   XOR(var<bool>::Ptr b,var<bool>::Ptr x,var<bool>::Ptr y)
      : Constraint(x->getSolver()),_b(b),_x(x),_y(y) {}
   void post() override;
   void propagate() override;
};

class IsMember : public Constraint { // b <=> x in S
   var<bool>::Ptr _b;
   var<int>::Ptr _x;
   std::set<int> _S;
public:
   IsMember(var<bool>::Ptr b,var<int>::Ptr x,std::set<int> S)
      : Constraint(x->getSolver()),_b(b),_x(x),_S(S) {}
   void post() override;
   void propagate() override;
};

class IsLessOrEqual : public Constraint { // b <=> x <= c
   var<bool>::Ptr _b;
   var<int>::Ptr _x;
   int _c;
public:
   IsLessOrEqual(var<bool>::Ptr b,var<int>::Ptr x,int c)
      : Constraint(x->getSolver()),_b(b),_x(x),_c(c) {}
   void post() override;
};

class IsLessOrEqualVar : public Constraint { // b <=> x <= y
   var<bool>::Ptr _b;
   var<int>::Ptr _x,_y;
public:
   IsLessOrEqualVar(var<bool>::Ptr b,var<int>::Ptr x,var<int>::Ptr y)
      : Constraint(x->getSolver()),_b(b),_x(x),_y(y) {}
   void post() override;
   void propagate() override;
};

class Minimize : public Objective {
   var<int>::Ptr _obj;
   int        _primal;
   int        _dual;
   void print(std::ostream& os) const;
public:
   Minimize(var<int>::Ptr& x);
   void tighten() override;
   int value() const override { return _obj->min();}
   bool betterThanPrimal(int value) const override { return value < _primal;}
   void foundPrimal(int primal) override;
   void setDual(int dual) override { _dual = dual; }
   int dual() const override { return _dual; }
   int primal() const override { return _primal; }
   double optimalityGap() const override { return static_cast<double>(_primal-_dual)/_primal; }
   bool isMin() const override { return true; }
};

class Maximize : public Objective {
   var<int>::Ptr _obj;
   int        _primal;
   int        _dual;
   void print(std::ostream& os) const;
public:
   Maximize(var<int>::Ptr& x);
   void tighten() override;
   int value() const override { return _obj->max();}
   bool betterThanPrimal(int value) const override { return value > _primal;}
   void foundPrimal(int primal) override;
   void setDual(int dual) override { _dual = dual; }
   int dual() const override { return _dual; }
   int primal() const override { return _primal; }
   double optimalityGap() const override { return static_cast<double>(_dual-_primal)/_dual; }
   bool isMin() const override { return false; }
};

class EQAbsDiffBC : public Constraint { // z == |x - y|
  var<int>::Ptr _z,_x,_y;
public:
  EQAbsDiffBC(var<int>::Ptr z,var<int>::Ptr x,var<int>::Ptr y)
       : Constraint(x->getSolver()),_z(z),_x(x),_y(y) {}
   void post() override;
};

class Absolute : public Constraint { // y == |x|
   var<int>::Ptr _x,_y;
public:
   Absolute(var<int>::Ptr x,var<int>::Ptr y)
      : Constraint(x->getSolver()),_x(x),_y(y) {}
   void post() override;
   void propagate() override;
};


class interval {
public:
  int min, max;
   interval(int min_,int max_) : min(min_),max(max_) {}
};

namespace Factory
{
   inline Constraint::Ptr equal(var<int>::Ptr x,var<int>::Ptr y,int c=0) {
      return new (x->getSolver()) EQBinBC(x,y,c);
   }
   inline Constraint::Ptr equal(var<int>::Ptr x,var<int>::Ptr y,var<int>::Ptr z) {
      return new (x->getSolver()) EQTernBC(x,y,z);
   }
   inline Constraint::Ptr equal(var<int>::Ptr x,var<int>::Ptr y,var<bool>::Ptr b) {
     return new (x->getSolver()) EQTernBCbool(x,y,b);
   }
   inline Constraint::Ptr notEqual(var<int>::Ptr x,var<int>::Ptr y,int c=0) {
      return new (x->getSolver()) NEQBinBC(x,y,c);
   }
   /**
    * Factory operator to create the constraint `x==c`
    * @param x the variable
    * @param c an integer constant
    * @return the constraint `x==c`
    * Note: this does not really allocate a constraint. It does the job inline on the variable.
    * @see EQc
    */
   inline ConstraintDesc::Ptr operator==(var<int>::Ptr x,const int c) {
      //auto cp = x->getSolver();
      //x->assign(c);
      //cp->fixpoint();
      //std::cout << "EQcDesc(" << x << "," << c << ")" << "\n";
      return new (x->getSolver()) EQcDesc(x,c);
   }
   /**
    * Factory operator to create the constraint `x!=c`
    * @param x the variable
    * @param c an integer constant
    * @return the constraint \f$x \neq c\f$
    * Note: this does not really allocate a constraint. It does the job inline on the variable.
    * @see NEQc
    */
   inline ConstraintDesc::Ptr operator!=(var<int>::Ptr x, const int c) {
      //auto cp = x->getSolver();
      //x->remove(c);
      //cp->fixpoint();
      return new (x->getSolver()) NEQcDesc(x,c);
      //return nullptr;
   }
   /**
    * Factory function to create the constraint \f$x \in S\f$
    * @param x the variable
    * @param S a set of integer constants
    * @return the constraint \f$x \in S\f$
    * Note: this does not really allocate a constraint. It does the job inline on the variable.
    */
   inline Constraint::Ptr inside(var<int>::Ptr x,std::set<int> S) {
      auto cp = x->getSolver();
      for(int v = x->min();v <= x->max();++v) {
         if (!x->contains(v)) continue;
         if (S.find(v) == S.end())
            x->remove(v);
      }
      cp->fixpoint();
      return nullptr;
   }
   /**
    * Factory function to create the constraint \f$x \notin S\f$
    * @param x the variable
    * @param S a set of integer constants
    * @return the constraint \f$x \notin S\f$
    * Note: this does not really allocate a constraint. It does the job inline on the variable.
    */
   inline Constraint::Ptr outside(var<int>::Ptr x,std::set<int> S) {
      auto cp = x->getSolver();
      for(int v : S) {
         if (x->contains(v))
            x->remove(v);
      }
      cp->fixpoint();
      return nullptr;
   }
   /**
    * Factory operator to create the constraint `x==c`
    * @param x the Boolean variable
    * @param c a Boolean constant
    * @return the constraint `x==c`
    * @see EQc
    */
   inline ConstraintDesc::Ptr operator==(var<bool>::Ptr x,const bool c) {
      return new (x->getSolver()) EQcDesc((var<int>::Ptr)x,c);
   }
   /**
    * Factory operator to create the constraint `x==c`
    * @param x the Boolean variable
    * @param c an integer constant (0 is false, anything else is true)
    * @return the constraint `x==c`
    * @see EQc
    */
   inline ConstraintDesc::Ptr operator==(var<bool>::Ptr x,const int c) {
      return new (x->getSolver()) EQcDesc((var<int>::Ptr)x,c);
   }
   /**
    * Factory operator to create the constraint `x==y`
    * @param x an integer variable
    * @param y an integer variable
    * @return the constraint `x==y`
    * @see EQBinBC
    * Note: this enforces bound consistency
    */
   inline Constraint::Ptr operator==(var<int>::Ptr x,var<int>::Ptr y) {
      return new (x->getSolver()) EQBinBC(x,y,0);
   }
   /**
    * Factory operator to create the constraint `x==y`
    * @param x a Boolean variable
    * @param y an integer variable
    * @return the constraint `x==y`
    * @see EQBinBC
    * Note: this enforces bound consistency
    */
   inline Constraint::Ptr operator==(var<bool>::Ptr x,var<int>::Ptr y) {
      return new (x->getSolver()) EQBinBC(x,y,0);
   }
   /**
    * Factory operator to create the constraint `x!=c`
    * @param x a Boolean variable
    * @param c a Boolean constant
    * @return the constraint \f$x \neq c\f$
    * @see NEQc
    */
   inline ConstraintDesc::Ptr operator!=(var<bool>::Ptr x,const bool c) {
      return new (x->getSolver()) NEQcDesc((var<int>::Ptr)x,c);
   }
   /**
    * Factory operator to create the constraint `x!=c`
    * @param x a Boolean variable
    * @param c an integer constant (0 is false, everything else is true)
    * @return the constraint \f$x \neq c\f$
    * @see NEQc
    */
   inline ConstraintDesc::Ptr operator!=(var<bool>::Ptr x,const int c) {
      return new (x->getSolver()) NEQcDesc((var<int>::Ptr)x,c);
   }
   /**
    * Factory operator to create the constraint `x!=y`
    * @param x an integer variable
    * @param y an integer variable
    * @return the constraint \f$x \neq y\f$
    * @see NEQBinBC
    */
   inline Constraint::Ptr operator!=(var<int>::Ptr x,var<int>::Ptr y) {
      return Factory::notEqual(x,y,0);
   }
   /**
    * Factory operator to create the constraint `x!=y`
    * @param x a Boolean variable
    * @param y a Boolean variable
    * @return the constraint \f$x \neq y\f$
    * @see NEQBinBC
    */
   inline Constraint::Ptr operator!=(var<bool>::Ptr x,var<bool>::Ptr y) {
      return Factory::notEqual(x,y,0);
   }
   /**
    * Factory operator to create the constraint \f$ x \leq y\f$
    * @param x an integer variable
    * @param y an integer variable
    * @return the constraint \f$x \leq y\f$
    * @see LessOrEqual
    */
   inline Constraint::Ptr operator<=(var<int>::Ptr x,var<int>::Ptr y) {
      return new (x->getSolver()) LessOrEqual(x,y);
   }
   /**
    * Factory operator to create the constraint \f$ x\geq y \f$
    * @param x an integer variable
    * @param y an integer variable
    * @return the constraint \f$x \geq y\f$
    * @see LessOrEqual
    */
   inline Constraint::Ptr operator>=(var<int>::Ptr x,var<int>::Ptr y) {
      return new (x->getSolver()) LessOrEqual(y,x);
   }
   /**
    * Factory operator to create the constraint x<y
    * @param x an integer variable
    * @param y an integer variable
    * @return the constraint \f$x < y\f$
    * @see LessOrEqual
    */
   inline Constraint::Ptr operator<(var<int>::Ptr x,var<int>::Ptr y) {
      return new (x->getSolver()) LessOrEqual(x,y-1);
   }
   /**
    * Factory operator to create the constraint x>y
    * @param x an integer variable
    * @param y an integer variable
    * @return the constraint \f$x > y\f$
    * @see LessOrEqual
    */
   inline Constraint::Ptr operator>(var<int>::Ptr x,var<int>::Ptr y) {
      return new (x->getSolver()) LessOrEqual(y,x-1);
   }
   /**
    * Factory function to create the constraint \f$x \leq c\f$
    * @param x an integer variable
    * @param c a constant integer
    * @return the constraint \f$x \leq c\f$
    * Note: this does not really allocate a constraint. It does the job inline on the variable directly.
    */
   inline Constraint::Ptr operator<=(var<int>::Ptr x,const int c) {
      auto cp = x->getSolver();
      x->removeAbove(c);
      cp->fixpoint();
      return nullptr;
   }
   /**
    * Factory function to create the constraint \f$x \geq c\f$
    * @param x an integer variable
    * @param c a constant integer
    * @return the constraint \f$x \geq c\f$
    * Note: this does not really allocate a constraint. It does the job inline on the variable directly.
    */
   inline Constraint::Ptr operator>=(var<int>::Ptr x,const int c) {
      auto cp = x->getSolver();
      x->removeBelow(c);
      cp->fixpoint();
      return nullptr;
   }
   /**
    * Factory function to create the constraint \f$x \leq c\f$
    * @param x a Boolean variable
    * @param c a constant integer
    * @return the constraint \f$x \leq c\f$
    * Note: this does not really allocate a constraint. It does the job inline on the variable directly.
    */
   inline Constraint::Ptr operator<=(var<bool>::Ptr x,const int c) {
        x->removeAbove(c);
        x->getSolver()->fixpoint();
        return nullptr;
    }
   /**
    * Factory function to create the constraint \f$x \geq c\f$
    * @param x a Boolean variable
    * @param c a constant integer
    * @return the constraint \f$x \geq c\f$
    * Note: this does not really allocate a constraint. It does the job inline on the variable directly.
    */
    inline Constraint::Ptr operator>=(var<bool>::Ptr x,const int c) {
        x->removeBelow(c);
        x->getSolver()->fixpoint();
        return nullptr;
    }
   /**
    * Factory function to create an objective function that minimizes variable `x`
    * @param x the variable to minimize
    * @return a pointer (handle) to the corresponding objective function
    * @see Minimize DFSearch::optimize
    */
   inline Objective::Ptr minimize(var<int>::Ptr x) {
      return new Minimize(x);
   }
   /**
    * Factory function to create an objective function that maximizes variable `x`
    * @param x the variable to maximize
    * @return a pointer (handle) to the corresponding objective function
    * @see Maximize DFSearch::optimize
    */
   inline Objective::Ptr maximize(var<int>::Ptr x) {
      return new Maximize(x);
   }
   /**
    * Factory operator that creates a fresh variable equal to the sum of two given integer variables.
    * @param x the first integer variable
    * @param y the second integer variable
    * @return a fresh variable `z` constrained to be equal to \f$x + y\f$
    */
   inline var<int>::Ptr operator+(var<int>::Ptr x,var<int>::Ptr y) { // x + y
      int min = x->min() + y->min();
      int max = x->max() + y->max();
      var<int>::Ptr z = makeIntVar(x->getSolver(),min,max);
      x->getSolver()->post(equal(z,x,y));
      return z;
   }
   /**
    * Factory operator that creates a fresh variable equal to the subtraction of two given integer variables.
    * @param x the first integer variable
    * @param y the second integer variable
    * @return a fresh variable `z` constrained to be equal to \f$x - y\f$
    */
   inline var<int>::Ptr operator-(var<int>::Ptr x,var<int>::Ptr y) { // x - y
      int min = x->min() - y->max();
      int max = x->max() - y->min();
      var<int>::Ptr z = makeIntVar(x->getSolver(),min,max);
      x->getSolver()->post(equal(x,z,y));
      return z;
   }
   /**
    * Factory operator that creates a fresh variable equal to the conjunction of two given Boolean variables.
    * @param x the first Boolean variable
    * @param y the second Boolean variable
    * @return a fresh variable `z` constrained to be equal to \f$x \wedge y\f$
    */
   inline var<bool>::Ptr operator*(var<bool>::Ptr x,var<bool>::Ptr y) { // x * y (bool) meaning x && y
      var<bool>::Ptr z = makeBoolVar(x->getSolver());
      x->getSolver()->post(new (x->getSolver()) Conjunction(z,x,y));
      return z;
   }
   /**
    * Factory operator that creates a fresh variable equal to the conjunction of two given Boolean variables.
    * @param x the first Boolean variable
    * @param y the second Boolean variable
    * @return a fresh variable `z` constrained to be equal to \f$x \wedge y\f$
    */
   inline var<bool>::Ptr operator&&(var<bool>::Ptr x,var<bool>::Ptr y) { // x * y (bool) meaning x && y
      var<bool>::Ptr z = makeBoolVar(x->getSolver());
      x->getSolver()->post(new (x->getSolver()) Conjunction(z,x,y));
      return z;
   }
   /**
    * Factory reification function that creates a fresh variable equal to the truth value of \f$x=c\f$
    * @param x an integer variable
    * @param c an integer constant
    * @return a fresh Boolean variable `z` constrained to \f$z \Leftrightarrow x = c\f$
    */
   inline var<bool>::Ptr isEqual(var<int>::Ptr x,const int c) {
      var<bool>::Ptr b = makeBoolVar(x->getSolver());
      TRYFAIL
         x->getSolver()->post(new (x->getSolver()) IsEqual(b,x,c));
      ONFAIL
      ENDFAIL
      return b;
   }
   /**
    * Factory reification function that creates a fresh variable equal to the truth value of \f$x \oplus y\f$
    * @param x an integer variable
    * @param y an integer variable
    * @return a fresh Boolean variable `z` constrained to \f$z \Leftrightarrow x \oplus y\f$
    */
   inline var<bool>::Ptr xOR(var<bool>::Ptr x,var<bool>::Ptr y) {
      var<bool>::Ptr z = makeBoolVar(x->getSolver());
      x->getSolver()->post(new (x->getSolver()) XOR(z, x, y));
      return z;
   }
   /**
    * Factory reification function that constraint variable `b` to be equal to the truth value of \f$x \in S\f$
    * @param b a Boolean variable
    * @param x an integer variable
    * @param S a set of integer constants
    * @return the constraint \f$b \Leftrightarrow x \in S\f$
    * @see IsMember
    */
   inline Constraint::Ptr isMember(var<bool>::Ptr b, var<int>::Ptr x, const std::set<int> S) {
     return new (x->getSolver()) IsMember(b,x,S);
   }
   /**
    * Factory reification function that creates a fresh variable equal to the truth value of \f$x \in S\f$
    * @param x an integer variable
    * @param S a set of integer constants
    * @return a fresh Boolean variable `z` constrained to \f$z \Leftrightarrow x \in S\f$
    * @see IsMember
    */
   inline var<bool>::Ptr isMember(var<int>::Ptr x,const std::set<int> S) {
      var<bool>::Ptr b = makeBoolVar(x->getSolver());
      TRYFAIL
         x->getSolver()->post(new (x->getSolver()) IsMember(b,x,S));
      ONFAIL
      ENDFAIL
      return b;
   }
   /**
    * Factory reification function that creates a fresh variable equal to the truth value of \f$x \leq c\f$
    * @param x an integer variable
    * @param c an integer constant
    * @return a fresh Boolean variable `z` constrained to \f$z \Leftrightarrow x \leq c\f$
    * @see IsLessOrEqual
    */
   inline var<bool>::Ptr isLessOrEqual(var<int>::Ptr x,const int c) {
      var<bool>::Ptr b = makeBoolVar(x->getSolver());
      TRYFAIL
         x->getSolver()->post(new (x->getSolver()) IsLessOrEqual(b,x,c));
      ONFAIL
      ENDFAIL
      return b;
   }
   /**
    * Factory reification function that creates a fresh variable equal to the truth value of \f$x \leq y\f$
    * @param x an integer variable
    * @param y an integer variable
    * @return a fresh Boolean variable `z` constrained to \f$z \Leftrightarrow x \leq y\f$
    * @see IsLessOrEqualVar
    */
   inline var<bool>::Ptr isLessOrEqual(var<int>::Ptr x, var<int>::Ptr y) {
      var<bool>::Ptr b = makeBoolVar(x->getSolver());
      TRYFAIL
         x->getSolver()->post(new (x->getSolver()) IsLessOrEqualVar(b, x, y));
      ONFAIL
      ENDFAIL
      return b;
   }
   /**
    * Factory reification function that creates a fresh variable equal to the truth value of \f$x < c\f$
    * @param x an integer variable
    * @param c an integer constant
    * @return a fresh Boolean variable `z` constrained to \f$z \Leftrightarrow x < c\f$
    * @see IsLessOrEqual
    */
   inline var<bool>::Ptr isLess(var<int>::Ptr x,const int c) {
      return isLessOrEqual(x,c - 1);
   }
   /**
    * Factory reification function that creates a fresh variable equal to the truth value of \f$x \geq c\f$
    * @param x an integer variable
    * @param c an integer constant
    * @return a fresh Boolean variable `z` constrained to \f$z \Leftrightarrow x \geq c\f$
    * @see IsLessOrEqual
    */
   inline var<bool>::Ptr isLargerOrEqual(var<int>::Ptr x,const int c) {
      return isLessOrEqual(- x,- c);
   }
   /**
    * Factory reification function that creates a fresh variable equal to the truth value of \f$x > c\f$
    * @param x an integer variable
    * @param c an integer constant
    * @return a fresh Boolean variable `z` constrained to \f$z \Leftrightarrow x > c\f$
    * @see IsLargerOrEqual
    */
   inline var<bool>::Ptr isLarger(var<int>::Ptr x,const int c) {
      return isLargerOrEqual(x , c + 1);
   }
   /**
    * Factory convenience function that creates a fresh variable `z` constrained to the sum of all input variables.
    * @param allVars a list of `n` var<int>::Ptr (or var<bool>::Ptr)
    * @return a fresh variable `z` such that \f$z = \sum_{i=0}^{n-1} allVars_i \f$
    * @see Sum
    */
   template <class T> var<int>::Ptr sum(std::initializer_list<T> allVars) {
      int sumMin = 0,sumMax = 0;
      for(T aVar : allVars) {
         sumMin += aVar->min();
         sumMax += aVar->max();
      }
      std::vector<T> vec = allVars;
      auto cp = vec[0]->getSolver();
      auto s = Factory::makeIntVar(cp,sumMin,sumMax);
      cp->post(new (cp) Sum(allVars,s));
      return s;      
   }
   /**
    * Factory convenience function that creates a fresh variable `z` constrained to the sum of entries in `xs`.
    * @param xs a vector of `n` var<int>::Ptr (or var<bool>::Ptr)
    * @return a fresh variable `z` such that \f$z = \sum_{i=0}^{n-1} xs_i \f$
    * @see Sum
    * Disabled for containers of numbers, which the plain ::sum in utils.hpp handles.
    */
   template <class Vec,
             std::enable_if_t<!std::is_arithmetic_v<std::decay_t<decltype(*std::begin(std::declval<Vec&>()))>>, int> = 0>
   var<int>::Ptr sum(Vec& xs) {
      int sumMin = 0,sumMax = 0;
      for(const auto& x : xs) {
         sumMin += x->min();
         sumMax += x->max();
      }
      auto cp = xs[0]->getSolver();
      auto s = Factory::makeIntVar(cp,sumMin,sumMax);
      cp->post(new (cp) Sum(xs,s));
      return s;
   }
   /**
    * Factory convenience function that creates a fresh variable `z` constrained to the sum of entries in `xs`.
    * @param cp the solver to own the fresh variable and constraint
    * @param xs a vector of `n` var<int>::Ptr (or var<bool>::Ptr)
    * @return a fresh variable `z` such that \f$z = \sum_{i=0}^{n-1} xs_i \f$
    * @see Sum   
    */
   template <class Vec> var<int>::Ptr sum(CPSolver::Ptr cp,Vec& xs) {
      int sumMin = 0,sumMax = 0;
      for(const auto& x : xs) {
         sumMin += x->min();
         sumMax += x->max();
      }
      auto s = Factory::makeIntVar(cp,sumMin,sumMax);
      if (xs.size() > 0)
         cp->post(new (cp) Sum(xs,s));
      return s;
   }
   /**
    * Factory convenience function that creates a fresh variable `z` constrained to the sum of scaled entries in `xs`.
    * @param xs a vector of `n` var<int>::Ptr (or var<bool>::Ptr)
    * @param allCoefs the coefficients to use to scale the entries of `xs`
    * @return a fresh variable `z` such that \f$z = \sum_{i=0}^{n-1} xs_i * allCoefs_i\f$
    * @see Sum   
    */
   template <class Vec> var<int>::Ptr sum(Vec& allVars,std::initializer_list<int> allCoefs) {
      assert(allVars.size()==allCoefs.size());
      Factory::Veci vec(allVars.size(),Factory::alloci(allVars[0]->getStore()));
      auto cbi = allCoefs.begin();
      for(unsigned int i=0;i < allVars.size();++i) {
         vec[i] = allVars[i] * *cbi;
         ++cbi;
      }
      return sum(vec);
   }   
   /**
    * Factory convenience function that creates a fresh variable `z` constrained to the sum of scaled entries in `xs`.
    * @param allVars a list of `n` var<int>::Ptr (or var<bool>::Ptr)
    * @param allCoefs a list of `n` coefficients to use to scale the entries of `allVars`
    * @return a fresh variable `z` such that \f$z = \sum_{i=0}^{n-1} allVars_i * allCoefs_i\f$
    * @see Sum   
    */
   template <class T> var<int>::Ptr sum(std::initializer_list<T> allVars,std::initializer_list<int> allCoefs) {
      assert(allVars.size()==allCoefs.size());
      std::vector<T> vec(allVars.size());
      auto cbi = allCoefs.begin();
      for(unsigned int i=0;i < allVars.size();++i) {
         vec[i] = allVars.data()[i] * *cbi;
         ++cbi;
      }
      return sum(vec);
   }   
   inline Constraint::Ptr sum(const Factory::Veci& xs,var<int>::Ptr s) {
      return new (xs[0]->getSolver()) Sum(xs,s);
   }
   inline Constraint::Ptr sum(const Factory::Vecb& xs,var<int>::Ptr s) {
      return new (xs[0]->getSolver()) Sum(xs,s);
   }
   inline Constraint::Ptr sum(const Factory::Veci& xs,int s) {
      auto sv = Factory::makeIntVar(xs[0]->getSolver(),s,s);
      return new (xs[0]->getSolver()) Sum(xs,sv);
   }
   inline Constraint::Ptr sum(const std::vector<var<int>::Ptr>& xs,int s) {
      auto sv = Factory::makeIntVar(xs[0]->getSolver(),s,s);
      return new (xs[0]->getSolver()) Sum(xs,sv);
   }
   inline Constraint::Ptr sum(const std::vector<var<int>::Ptr>& xs,var<int>::Ptr s) {
      return new (xs[0]->getSolver()) Sum(xs,s);
   }
   inline Constraint::Ptr sum(const std::vector<var<bool>::Ptr>& xs,var<int>::Ptr s) {
      return new (xs[0]->getSolver()) Sum(xs,s);
   }
   inline Constraint::Ptr sum(const Factory::Vecb& xs,int s) {
      return new (xs[0]->getSolver()) SumBool(xs,s);
   }
   inline Constraint::Ptr sum(const std::vector<var<bool>::Ptr>& xs,int s) {
      return new (xs[0]->getSolver()) SumBool(xs,s);
   }
   /**
    * Factory function that creates a constraint representing a Boolean clause over the variables in xs
    * @param xs a vector of `n` Boolean variables
    * @return a constraint representing \f$ \bigvee_{i=0}^n xs_i \f$
    * @see Clause
    */
   template <class Vec> Constraint::Ptr clause(const Vec& xs) {
      return new (xs[0]->getSolver()) Clause(xs);
   }
   inline Constraint::Ptr clause(std::initializer_list<var<bool>::Ptr> init) {
     return new ((*init.begin())->getSolver()) Clause(init);    
   }
   /**
    * Factory reification function that returns a constraint requiring `b` to be true if the clause over `xs` is true
    * @param b a Boolean variable
    * @param xs a vector of `n` Boolean variables
    * @return a constraint representing \f$b \Leftrightarrow \bigvee_{i=0}^n xs_i \f$
    * @see IsClause
    */
   template <class Vec> Constraint::Ptr isClause(var<bool>::Ptr b,const Vec& xs) {
      return new (b->getSolver()) IsClause(b,xs);
   }
   /**
    * Factory function that returns a Boolean variable representing \f$a \rightarrow b\f$
    * @param a a Boolean variable
    * @param b a Boolean variable
    * @return a fresh Boolean variable `z` such that \f$ z \Leftrightarrow a \rightarrow b\f$
    */    
   inline var<bool>::Ptr implies(var<bool>::Ptr a,var<bool>::Ptr b) { // a=>b is not(a) or b is (1-a)+b >= 1
      std::vector<var<int>::Ptr> left = {1- (var<int>::Ptr)a,b};
      return isLargerOrEqual(sum(left),1);
   }
   /**
    * Factory function that returns an allDifferent global constraint over the variables in `xs`
    * @param xs a vector of `n` variables
    * @return a global constraint enforcing \f$\forall i,j \in Range(xs) \mbox{ s.t. } i\neq j : x_i \neq x_j\f$
    * Note: The constraint enforces value consistency
    */

  template <class Vec> Constraint::Ptr allDifferent(const Vec& xs) {
      return new (xs[0]->getSolver()) AllDifferentBinary(xs);
   }

  inline Constraint::Ptr allDifferentv(const Factory::Veci& xs) {
    return new (xs[0]->getSolver()) AllDifferentBinary(xs);
  }

  /**
    * Factory function that returns an allDifferent global constraint over the variables in `xs`
    * @param xs a vector of `n` variables
    * @return a global constraint enforcing \f$\forall i,j \in Range(xs) \mbox{ s.t. } i\neq j : x_i \neq x_j\f$
    * Note: The constraint enforces domain consistency
    */
   template <class Vec> Constraint::Ptr allDifferentAC(const Vec& xs) {
      return new (xs[0]->getSolver()) AllDifferentAC(xs);
   }
   /**
    * Factory function that return a circuit constraint over the variables held in `xs`
    * @param xs a vector of `n` integer variables
    * @return a global constraint requiring that the value be all different in `xs` and form a single Hamiltonian
    * circuit visiting all vertices.
    * This is typically used with an array of integer variables `next` such that \f$next_i\f$
    * give the *next* place to visit after vertex \f$i\f$.
    * @see Circuit
    */
   template <class Vec>  Constraint::Ptr circuit(const Vec& xs) {
      return new (xs[0]->getSolver()) Circuit(xs);
   }
   /**
    * Factory function that returns a global constraint requiring \f$z = array[y]\f$
    * @param array a vector of `n` integer constants
    * @param y an integer variable to index into `array`
    * @param z an integer variable
    * @return a new constraint that imposes \f$z=array[y]\f$
    * Note: the constraint enforces domain consistency
    * @see Element1DDC
    */
   template <class Vec> Constraint::Ptr element(const Vec& array,var<int>::Ptr y,var<int>::Ptr z) {
      std::vector<int> flat(array.size());
      for(int i=0;i < (int)array.size();i++)
         flat[i] = array[i];
      return new (y->getSolver()) Element1DDC(flat,y,z);
   }
   /**
    * Factory function that returns a global constraint requiring \f$z = array[y]\f$
    * @param array a vector of `n` integer constants
    * @param y an integer variable to index into `array`
    * @param z an integer variable
    * @return a new constraint that imposes \f$z=array[y]\f$
    * Note: the constraint enforces bound consistency on `z`. Cheaper than `element` when
    * holes in `z` do not prune (e.g., a cost read through a successor variable).
    * @see Element1D
    */
   template <class Vec> Constraint::Ptr elementBC(const Vec& array,var<int>::Ptr y,var<int>::Ptr z) {
      std::vector<int> flat(array.size());
      for(int i=0;i < (int)array.size();i++)
         flat[i] = array[i];
      return new (y->getSolver()) Element1D(flat,y,z);
   }
   /**
    * Factory function that returns a global constraint requiring \f$z = xs[y]\f$
    * @param xs a vector of `n` integer variables
    * @param y an integer variable to index into `xs`
    * @param z an integer variable
    * @return a new constraint that imposes \f$z=xs[y]\f$
    * Note: the constraint enforces domain consistency
    * @see Element1DVar
    */
   template <class Vec> Constraint::Ptr elementVar(const Vec& xs,var<int>::Ptr y,var<int>::Ptr z) {
       std::vector<var<int>::Ptr> flat(xs.size());
       for(int i=0;i<xs.size();i++)
           flat[i] = xs[i];
       return new (y->getSolver()) Element1DVar(flat,y,z);
   }
   /**
    * Factory function that returns a fresh variable z constrained by \f$z = d[x][y]\f$
    * @param d a 2D matrix of integer constants
    * @param x an integer variable to index into `d`'s first dimension
    * @param y an integer variable to index into `d`'s second dimension
    * @return a fresh variable `z` constrained by \f$z=d[x][y]\f$
    * Note: the constraint enforces domain consistency
    * @see Element2D. This is ultimately rewritten as a table constraint
    */
   inline var<int>::Ptr element(Matrix<int,2>& d,var<int>::Ptr x,var<int>::Ptr y) {
      int min = INT32_MAX,max = INT32_MIN;
      for(int i=0;i<d.size(0);i++)
         for(int j=0;j < d.size(1);j++) {
            min = min < d[i][j] ? min : d[i][j];
            max = max > d[i][j] ? max : d[i][j];
         }
      auto z = makeIntVar(x->getSolver(),min,max);
      x->getSolver()->post(new (x->getSolver()) Element2D(d,x,y,z));
      return z;
   }
   /**
    * Factory function that constraints `z` to be \f$z = d[...][y]\f$
    * @param array a 1D slice (specific row) of a 2D matrix of integer constants
    * @param y an integer variable to index into `d`'s second dimension
    * @return a fresh variable `z` constrained by \f$z=d[...][y]\f$
    * Note: the constraint enforces bound consistency. Note that the slice was obtained
    * as a result of indexing the first dimension of  the 2D matrix with a constant (which produces a VMSlice)
    * @see Element1D
    */
   inline Constraint::Ptr element(const VMSlice<int,2,1>& array,var<int>::Ptr y,var<int>::Ptr z) {
      std::vector<int> flat(array.size());
      for(int i=0;i < array.size();i++)
         flat[i] = array[i];
      return new (y->getSolver()) Element1D(flat,y,z);
   }
   template <class Vec> inline var<int>::Ptr element(const Vec& array,var<int>::Ptr y) {
      int min = INT32_MAX,max = INT32_MIN;
      std::vector<int> flat(array.size());
      for(auto i=0u;i < array.size();i++) {
         const int v = flat[i] = array[i];
         min = min < v ? min : v;
         max = max > v ? max : v;
      }
      auto z = makeIntVar(y->getSolver(),min,max);
      y->getSolver()->post(new (y->getSolver()) Element1DDC(flat,y,z));
      return z;
   }
   template <class Vec> var<int>::Ptr elementVar(const Vec& xs,var<int>::Ptr y) {
      int min = INT32_MAX,max = INT32_MIN;
      std::vector<var<int>::Ptr> flat(xs.size());
      for(auto i=0u;i < xs.size();i++) {
         const auto& v = flat[i] = xs[i];
         min = min < v->min() ? min : v->min();
         max = max > v->max() ? max : v->max();
      }
      auto z = makeIntVar(y->getSolver(),min,max);
      y->getSolver()->post(new (y->getSolver()) Element1DVar(flat,y,z));
      return z;
   }
   /**
    * Factory function that produces a constraint \f$z = | x -y |\f$
    * @param z the result integer variable
    * @param x the first integer variable in the distance
    * @param y the second integer variabel in the distance
    * @return a new constraint enforcing \f$z = |x-y|\f$
    * @see EQAbsDiffBC. The constraint enforces bound consistency
    */
   inline Constraint::Ptr equalAbsDiff(var<int>::Ptr z,var<int>::Ptr x,var<int>::Ptr y) {
      return new (x->getSolver()) EQAbsDiffBC(z,x,y);
   }
   /**
    * Factory function that creates a fresh variable equal to \f$|x|\f$
    * @param x an integer variable
    * @return a fresh integer variable `y` constrained to \f$y = |x|\f$
    * @see Absolute
    */
   inline var<int>::Ptr abs(var<int>::Ptr x) {
      auto cp = x->getSolver();
      auto y = makeIntVar(cp,0,std::max(x->max(),-x->min()));
      TRYFAIL
         cp->post(new (cp) Absolute(x,y));
      ONFAIL
      ENDFAIL
      return y;
   }
   /**
    * Factory function that creates a fresh variable equal to the maximum of the input variables
    * @param xs a non-empty container of `n` var<int>::Ptr
    * @return a fresh integer variable `y` constrained to \f$y = \max_{i=0}^{n-1} xs_i\f$
    * @see Maximum
    */
   template <class Vec> var<int>::Ptr maximum(const Vec& xs) {
      assert(xs.size() > 0);
      auto cp = xs[0]->getSolver();
      int yMin = std::numeric_limits<int>::min(),yMax = std::numeric_limits<int>::min();
      for(auto& xi : xs) {
         yMin = std::max(yMin,xi->min());
         yMax = std::max(yMax,xi->max());
      }
      auto y = makeIntVar(cp,yMin,yMax);
      TRYFAIL
         cp->post(new (cp) Maximum(xs,y));
      ONFAIL
      ENDFAIL
      return y;
   }
};

/**
 * Helper function for debugging that calls the virtual method on `c` to print it out
 * @param c the constraint to print
 */
void printCstr(Constraint::Ptr c);
void printCstrDesc(ConstraintDesc::Ptr c);

#endif
