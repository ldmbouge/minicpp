#include "literal.hpp"

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% EQ

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

void LitVarEQ<trail<char>>::updateVal() {
    if (_x->contains(_c)) {
        if (_x->size() == 1) assign(true);
    }
    else assign(false);
}

void LitVarEQ<trail<char>>::assign(bool b) {
    b ? setTrue() : setFalse();
    for(auto& f : _onBindList)
        _x->getSolver()->schedule(f);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% NEQ

void LitVarNEQ<char>::initVal() {
    if (_x->contains(_c)) {
        if (_x->size() == 1) _val = 0x01;
        else _val = 0x02;
    }
    else _val = 0x00;
}

void LitVarNEQ<trail<char>>::initVal() {
    if (_x->contains(_c)) {
        if (_x->size() == 1) _val = 0x01;
        else _val = 0x02;
    }
    else _val = 0x00;
}

void LitVarNEQ<trail<char>>::setTrue() {
    if (isBound()) return;
    _val = 0x01;
    _x->remove(_c);
}

void LitVarNEQ<trail<char>>::setFalse() {
    if (isBound()) return;
    _val = 0x00;
    _x->assign(_c);
}

void LitVarNEQ<trail<char>>::updateVal() {
    if (_x->contains(_c)) {
        if (_x->size() == 1) assign(false);
    }
    else assign(true);
}

void LitVarNEQ<trail<char>>::assign(bool b) {
    b ? setTrue() : setFalse();
    for(auto& f : _onBindList)
        _x->getSolver()->schedule(f);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% GEQ

void LitVarGEQ<char>::initVal() {
    if (_x->contains(_c)) {
        if (_x->size() == 1) _val = 0x01;
        else _val = 0x02;
    }
    else _val = 0x00;
}

void LitVarGEQ<trail<char>>::initVal() {
    if (_x->min() >= _c) {
        _val = 0x01;
    }
    else if (_x->max() < _c) {
        _val = 0x00;
    }
    else 
        _val = 0x02;
}

void LitVarGEQ<trail<char>>::setTrue() {
    if (isBound()) return;
    _val = 0x01;
    _x->removeBelow(_c);
}

void LitVarGEQ<trail<char>>::setFalse() {
    if (isBound()) return;
    _val = 0x00;
    _x->removeAbove(_c-1);
}

void LitVarGEQ<trail<char>>::updateVal() {
    if (_x->min() >= _c) {
        assign(true);
    }
    else if (_x->max() < _c) {
        assign(false);
    }
}

void LitVarGEQ<trail<char>>::assign(bool b) {
    b ? setTrue() : setFalse();
    for(auto& f : _onBindList)
        _x->getSolver()->schedule(f);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% LEQ

void LitVarLEQ<char>::initVal() {
    if (_x->max() <= _c) {
        _val = 0x01;
    }
    else if (_x->min() > _c) {
        _val = 0x00;
    }
    else 
        _val = 0x02;
}

void LitVarLEQ<trail<char>>::initVal() {
    if (_x->contains(_c)) {
        if (_x->size() == 1) _val = 0x01;
        else _val = 0x02;
    }
    else _val = 0x00;
}

void LitVarLEQ<trail<char>>::setTrue() {
    if (isBound()) return;
    _val = 0x01;
    _x->removeAbove(_c);
}

void LitVarLEQ<trail<char>>::setFalse() {
    if (isBound()) return;
    _val = 0x00;
    _x->removeBelow(_c+1);
}

void LitVarLEQ<trail<char>>::updateVal() {
    if (_x->max() <= _c) {
        assign(true);
    }
    else if (_x->min() > _c) {
        assign(false);
    }
}

void LitVarLEQ<trail<char>>::assign(bool b) {
    b ? setTrue() : setFalse();
    for(auto& f : _onBindList)
        _x->getSolver()->schedule(f);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

namespace Factory {
    LitVarEQ<trail<char>>::Ptr makeLitVarEQ(const var<int>::Ptr& x, const int c) {
        auto rv = new (x->getSolver()->getStore()) LitVarEQ<trail<char>>(x,c);
        x->getSolver()->registerVar(rv);
        return rv;
    }
    LitVarNEQ<trail<char>>::Ptr makeLitVarNEQ(const var<int>::Ptr& x, const int c) {
        auto rv = new (x->getSolver()->getStore()) LitVarNEQ<trail<char>>(x,c);
        x->getSolver()->registerVar(rv);
        return rv;
    }
    LitVarGEQ<trail<char>>::Ptr makeLitVarGEQ(const var<int>::Ptr& x, const int c) {
        auto rv = new (x->getSolver()->getStore()) LitVarGEQ<trail<char>>(x,c);
        x->getSolver()->registerVar(rv);
        return rv;
    }
    LitVarLEQ<trail<char>>::Ptr makeLitVarLEQ(const var<int>::Ptr& x, const int c) {
        auto rv = new (x->getSolver()->getStore()) LitVarLEQ<trail<char>>(x,c);
        x->getSolver()->registerVar(rv);
        return rv;
    }
}
