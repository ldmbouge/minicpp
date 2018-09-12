#ifndef __CONSTRAINT_H
#define __CONSTRAINT_H

#include "intvar.hpp"
#include "acstr.hpp"

class EQc : public Constraint { // x == c
    var<int>::Ptr _x;
    int           _c;
public:
    EQc(var<int>::Ptr& x,int c) : _x(x),_c(c) {}
    void post() override;
};

class NEQc : public Constraint { // x != c
    var<int>::Ptr _x;
    int           _c;
public:
    NEQc(var<int>::Ptr& x,int c) : _x(x),_c(c) {}
    void post() override;
};

class EQBinBC : public Constraint { // x == y + c
    var<int>::Ptr _x,_y;
    int _c;
public:
    EQBinBC(var<int>::Ptr& x,var<int>::Ptr& y,int c) : _x(x),_y(y),_c(c) {}
    void post() override;
};

class NEQBinBC : public Constraint { // x != y + c
    var<int>::Ptr _x,_y;
    int _c;
    revList<Constraint::Ptr>::revNode* hdl[2];
    void print(std::ostream& os) const override;
public:
    NEQBinBC(var<int>::Ptr& x,var<int>::Ptr& y,int c) : _x(x),_y(y),_c(c) {}
    void post() override;
};

class EQBinDC : public Constraint { // x == y + c
    var<int>::Ptr _x,_y;
    int _c;
public:
    EQBinDC(var<int>::Ptr& x,var<int>::Ptr& y,int c) : _x(x),_y(y),_c(c) {}
    void post() override;
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
    inline Constraint::Ptr makeEQBinBC(var<int>::Ptr x,var<int>::Ptr y,int c) {
        return new (x->getSolver()) EQBinBC(x,y,c);
    }
    inline Constraint::Ptr makeNEQBinBC(var<int>::Ptr x,var<int>::Ptr y,int c) {
        return new (x->getSolver()) NEQBinBC(x,y,c);
    }
    inline Constraint::Ptr operator==(var<int>::Ptr x,int c) {
        return new (x->getSolver()) EQc(x,c);
    }
    inline Constraint::Ptr operator!=(var<int>::Ptr x,int c) {
        return new (x->getSolver()) NEQc(x,c);
    }
    inline Constraint::Ptr operator!=(var<int>::Ptr x,var<int>::Ptr y) {
        return Factory::makeNEQBinBC(x,y,0);
    }
    inline Objective::Ptr minimize(var<int>::Ptr x) {
        return new Minimize(x);
    }
};

#endif
