#include "literal.hpp"

void LitVarEQ<char>::initVal() {
    if (_x->contains(_c)) {
        if (_x->size() == 1) _val = 0x01;
        else _val = 0x02;
    }
    else _val = 0x00;
}

void LitVarEQ<trail<char>>::initVal() {
    if (_x->contains(_c)) {
        if (_x->size() == 1) _val = 0x01;
        else _val = 0x02;
    }
    else _val = 0x00;
}

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

void LitVarEQ<trail<char>>::assign(bool b) {
    b ? setTrue() : setFalse();
    for(auto& f : _onBindList)
        _x->getSolver()->schedule(f);
}

namespace Factory {
    LitVarEQ<trail<char>>::Ptr makeLitVarEQ(const var<int>::Ptr& x, const int c) {
        auto rv = new (x->getSolver()->getStore()) LitVarEQ<trail<char>>(x,c);
        x->getSolver()->registerVar(rv);
        return rv;
    }
}
