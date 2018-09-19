#ifndef __CONSTRAINT_H
#define __CONSTRAINT_H

#include <array>
#include <algorithm>
#include <iomanip>
#include <stdint.h>
#include "matrix.hpp"
#include "intvar.hpp"
#include "acstr.hpp"

class EQc : public Constraint { // x == c
    var<int>::Ptr _x;
    int           _c;
public:
   EQc(var<int>::Ptr& x,int c) : Constraint(x->getSolver()),_x(x),_c(c) {}
    void post() override;
};

class NEQc : public Constraint { // x != c
    var<int>::Ptr _x;
    int           _c;
public:
   NEQc(var<int>::Ptr& x,int c) : Constraint(x->getSolver()),_x(x),_c(c) {}
   void post() override;
};

class EQBinBC : public Constraint { // x == y + c
    var<int>::Ptr _x,_y;
    int _c;
public:
    EQBinBC(var<int>::Ptr& x,var<int>::Ptr& y,int c)
       : Constraint(x->getSolver()),_x(x),_y(y),_c(c) {}
    void post() override;
};

class NEQBinBC : public Constraint { // x != y + c
    var<int>::Ptr _x,_y;
    int _c;
    revList<Constraint::Ptr>::revNode* hdl[2];
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

class Sum : public Constraint { // s = Sum({x0,...,xk})
   std::vector<var<int>::Ptr> _x;
   trail<int>    _nUnBounds;
   trail<int>    _sumBounds;
   int _n;
   std::vector<int> _unBounds;
public:
   Sum(const Factory::Vecv& x,var<int>::Ptr s);
   Sum(const std::vector<var<int>::Ptr>& x,var<int>::Ptr s);
   void post() override;
   void propagate() override;
};

class AllDifferentBinary :public Constraint {
    std::vector<var<int>::Ptr> _x;
public:
   AllDifferentBinary(const Factory::Vecv& x);
   AllDifferentBinary(const std::vector<var<int>::Ptr>& x);
   void post() override;
};

class Circuit :public Constraint {
   std::vector<var<int>::Ptr>  _x;
   trail<int>* _dest;
   trail<int>* _orig;
   trail<int>* _lengthToDest;
   void bind(int i);
public:
   Circuit(const Factory::Vecv& x);
   Circuit(const std::vector<var<int>::Ptr>& x);
   void post() override;
};

class Minimize : public Objective {
    var<int>::Ptr _obj;
    int        _primal;
    void print(std::ostream& os) const;
public:
    Minimize(var<int>::Ptr& x);
    void tighten() override;
    int value() const override { return _obj->min();}
};

class Element2D : public Constraint {
    struct Triplet {
        int x,y,z;
        Triplet() : x(0),y(0),z(0) {}
        Triplet(int a,int b,int c) : x(a),y(b),z(c) {}
        Triplet(const Triplet& t) : x(t.x),y(t.y),z(t.z) {}
    };
        
    matrix<int,2> _matrix;
    var<int>::Ptr _x,_y,_z;
    int _n,_m;
    trail<int>* _nRowsSup;
    trail<int>* _nColsSup;
    trail<int> _low,_up;
    std::vector<Triplet> _xyz;
    void updateSupport(int lostPos);
public:
    Element2D(const matrix<int,2>& mat,var<int>::Ptr x,var<int>::Ptr y,var<int>::Ptr z);
    void post() override;;
    void propagate() override;
    void print(std::ostream& os) const override;
};

class Element1D : public Constraint {
   std::vector<int> _t;
   var<int>::Ptr _y;
   var<int>::Ptr _z;
public:
   Element1D(const std::vector<int>& array,var<int>::Ptr y,var<int>::Ptr z);
   void post() override;
};

namespace Factory {
    inline Constraint::Ptr equal(var<int>::Ptr x,var<int>::Ptr y,int c=0) {
        return new (x->getSolver()) EQBinBC(x,y,c);
    }
    inline Constraint::Ptr notEqual(var<int>::Ptr x,var<int>::Ptr y,int c=0) {
        return new (x->getSolver()) NEQBinBC(x,y,c);
    }
    inline Constraint::Ptr operator==(var<int>::Ptr x,int c) {
        return new (x->getSolver()) EQc(x,c);
    }
    inline Constraint::Ptr operator!=(var<int>::Ptr x,int c) {
        return new (x->getSolver()) NEQc(x,c);
    }
    inline Constraint::Ptr operator!=(var<int>::Ptr x,var<int>::Ptr y) {
        return Factory::notEqual(x,y,0);
    }
    inline Constraint::Ptr operator<=(var<int>::Ptr x,var<int>::Ptr y) {
        return new (x->getSolver()) LessOrEqual(x,y);
    }
    inline Constraint::Ptr operator>=(var<int>::Ptr x,var<int>::Ptr y) {
        return new (x->getSolver()) LessOrEqual(y,x);
    }
    inline Objective::Ptr minimize(var<int>::Ptr x) {
        return new Minimize(x);
    }
    inline var<bool>::Ptr isEqual(var<int>::Ptr x,const int c) {
        var<bool>::Ptr b = makeBoolVar(x->getSolver());
        try {
            x->getSolver()->post(new (x->getSolver()) IsEqual(b,x,c));
        } catch(Status s) {}
        return b;
    }
    template <class Vec> var<int>::Ptr sum(Vec& xs) {
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
    template <class Vec> Constraint::Ptr sum(const Vec& xs,var<int>::Ptr s) {
        return new (xs[0]->getSolver()) Sum(xs,s);
    }
    template <class Vec> Constraint::Ptr sum(const Vec& xs,int s) {
        auto sv = Factory::makeIntVar(xs[0]->getSolver(),s,s);
        return new (xs[0]->getSolver()) Sum(xs,sv);
    }
   template <class Vec> Constraint::Ptr allDifferent(const Vec& xs) {
      return new (xs[0]->getSolver()) AllDifferentBinary(xs);
   }
   template <class Vec>  Constraint::Ptr circuit(const Vec& xs) {
      return new (xs[0]->getSolver()) Circuit(xs);
   }
   inline var<int>::Ptr element(matrix<int,2>& d,var<int>::Ptr x,var<int>::Ptr y) {
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
   inline Constraint::Ptr element(const MSlice<int,2,1>& array,var<int>::Ptr y,var<int>::Ptr z) {
      std::vector<int> flat(array.size());
      for(int i=0;i < array.size();i++) 
         flat[i] = array[i];
      return new (y->getSolver()) Element1D(flat,y,z);
   }
   inline var<int>::Ptr element(const MSlice<int,2,1>& array,var<int>::Ptr y) {
      int min = INT32_MAX,max = INT32_MIN;
      std::vector<int> flat(array.size());
      for(int i=0;i < array.size();i++) {
         const int v = flat[i] = array[i];
         min = min < v ? min : v;
         max = max > v ? max : v;
      }
      auto z = makeIntVar(y->getSolver(),min,max);
      y->getSolver()->post(new (y->getSolver()) Element1D(flat,y,z));
      return z;
   }
};

void printCstr(Constraint::Ptr c); 

#endif
