#ifndef __LITERAL_H
#define __LITERAL_H

#include <memory>
#include "handle.hpp"
#include "intvar.hpp"
#include "trailable.hpp"
#include "solver.hpp"

template<typename T> class LitVarEQ {};

template <>
class LitVarEQ<char> : public AVar {
    var<int>::Ptr _x;
    char _val;
    int _c;
    int _id;
protected:
    void setId(int id) override {_id = id;}
    int getId() const { return _id;}
public:
    LitVarEQ(const var<int>::Ptr& x, const int c) : _x(x), _val(0x02), _c(c) { setVal();}
    inline bool isBound() {return _val != 0x02;}
    inline bool isTrue() {return _val == 0x01;}
    inline bool isFalse() {return _val == 0x00;}
    void setVal();
};

void LitVarEQ<char>::setVal() {
    if (_x->contains(_c)) {
        if (_x->size() == 1) 
            _val = 0x01;
        else return;
    }
    else 
        _val = 0x00;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

class LitVarEQ<trail<char>>;

namespace Factory {
    handle_ptr<LitVarEQ<trail<char>>> makeLitVarEQ(const var<int>::Ptr& x, const int c);
}


template <>
class LitVarEQ<trail<char>> : public AVar {
    CPSolver::Ptr _cps;
    var<int>::Ptr _x;
    trail<char> _val;
    int _c;
    int _id;
    LitVarEQ(const var<int>::Ptr& x, const int c) 
      : _cps(x->getSolver()), _x(x), _val(x->getSolver()->getStateManager(), 0x02), _c(c) {update();}
protected:
    void setId(int id) override {_id = id;}
    int getId() const { return _id;}
public:
    typedef handle_ptr<LitVarEQ<trail<char>>> Ptr;
    inline bool isBound() {return _val != 0x02;}
    inline bool isTrue() {return _val == 0x01;}
    inline bool isFalse() {return _val == 0x00;}
    void setTrue();
    void setFalse();
    void update();
private:
    friend Ptr Factory::makeLitVarEQ(const var<int>::Ptr& x, const int c);
};

void LitVarEQ<trail<char>>::setTrue() {
    if (isBound()) return;
    _val = 0x01;
    _x->assign(_c);
}

void LitVarEQ<trail<char>>::setFalse() {
    if (isBound()) return;
    _val = 0x00;
    _x->remove(_c);
}

void LitVarEQ<trail<char>>::update() {
    if (isBound()) return;
    if (_x->contains(_c)) {
        if (_x->size() == 1) setTrue();
    }
    else setFalse();
}


#endif