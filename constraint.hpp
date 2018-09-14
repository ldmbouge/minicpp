#ifndef __CONSTRAINT_H
#define __CONSTRAINT_H

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

class EQBinDC : public Constraint { // x == y + c
    var<int>::Ptr _x,_y;
    int _c;
public:
    EQBinDC(var<int>::Ptr& x,var<int>::Ptr& y,int c)
       : Constraint(x->getSolver()), _x(x),_y(y),_c(c) {}
    void post() override;
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
    trail<int>   _nUnBounds;
    trail<int>   _sumBounds;
    int _n;
    std::vector<int> _unBounds;
public:
    Sum(const Factory::Vecv& x,var<int>::Ptr s);
    void post() override;
    void propagate() override;
};

class Minimize : public Objective {
    var<int>::Ptr _obj;
    int        _primal;
    void print(std::ostream& os) const;
public:
    Minimize(var<int>::Ptr& x);
    void tighten() override;
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
    template <class Vec> Constraint::Ptr sum(Vec xs,var<int>::Ptr s) {
        return new (xs[0]->getSolver()) Sum(std::move(xs),s);
    }
    template <class Vec> Constraint::Ptr sum(Vec xs,int s) {
        auto sv = Factory::makeIntVar(xs[0]->getSolver(),s,s);
        return new (xs[0]->getSolver()) Sum(std::move(xs),sv);
    }
};

#endif
