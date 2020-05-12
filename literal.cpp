#include "literal.hpp"

void LitVar::initVal() {
    switch(_rel) {
        case EQ:  if (_x->contains(_c)) {
                  if (_x->size() == 1) _val = 0x01;
                  else _val = 0x02;
                  }
                  else _val = 0x00;
                  return;
        case NEQ: if (_x->contains(_c)) {
                  if (_x->size() == 1) _val = 0x00;
                  else _val = 0x02;
                  }
                  else _val = 0x01;
                  return;
        case LEQ: if (_x->max() <= _c) {
                      _val = 0x01;
                  }
                  else if (_x->min() > _c) {
                      _val = 0x00;
                  }
                  else 
                      _val = 0x02;
                  return;
        case GEQ: if (_x->min() >= _c) {
                      _val = 0x01;
                  }
                  else if (_x->max() < _c) {
                      _val = 0x00;
                  }
                  else 
                      _val = 0x02;
                  return;
    }
}

void LitVar::setTrue() {
    if (isBound()) return;
    _val = 0x01;
    switch(_rel) {
        case EQ:  _x->assign(_c); return;
        case NEQ: _x->remove(_c); return;
        case LEQ: _x->removeAbove(_c); return;
        case GEQ: _x->removeBelow(_c); return;
    }
}

void LitVar::setFalse() {
    if (isBound()) return;
    _val = 0x00;
    switch(_rel) {
        case EQ:  _x->remove(_c); return;
        case NEQ: _x->assign(_c); return;
        case LEQ: _x->removeBelow(_c+1); return;
        case GEQ: _x->removeAbove(_c-1); return;        
    }
}

void LitVar::updateVal() {
    switch(_rel) {
        case EQ:  if (_x->contains(_c)) {
                  if (_x->size() == 1) assign(true);
                  }
                  else assign(false);
                  return;
        case NEQ: if (_x->contains(_c)) {
                  if (_x->size() == 1) assign(false);
                  }
                  else assign(true);
                  return;
        case LEQ: if (_x->max() <= _c) {
                      assign(true);
                  }
                  else if (_x->min() > _c) {
                      assign(false);
                  }
                  return;
        case GEQ: if (_x->min() >= _c) {
                      assign(true);
                  }
                  else if (_x->max() < _c) {
                      assign(false);
                  }
                  return;
    }
    return;
}

void LitVar::assign(bool b) {
    b ? setTrue() : setFalse();
    for(auto& f : _onBindList)
        _x->getSolver()->schedule(f);
}

namespace Factory {
    LitVar::Ptr makeLitVarEQ(const var<int>::Ptr& x, const int c) {
        auto rv = new (x->getSolver()->getStore()) LitVar(x,c,EQ);
        x->getSolver()->registerVar(rv);
        return rv;
    }
    LitVar::Ptr makeLitVarNEQ(const var<int>::Ptr& x, const int c) {
        auto rv = new (x->getSolver()->getStore()) LitVar(x,c,NEQ);
        x->getSolver()->registerVar(rv);
        return rv;
    }
    LitVar::Ptr makeLitVarLEQ(const var<int>::Ptr& x, const int c) {
        auto rv = new (x->getSolver()->getStore()) LitVar(x,c,LEQ);
        x->getSolver()->registerVar(rv);
        return rv;
    }
    LitVar::Ptr makeLitVarGEQ(const var<int>::Ptr& x, const int c) {
        auto rv = new (x->getSolver()->getStore()) LitVar(x,c,GEQ);
        x->getSolver()->registerVar(rv);
        return rv;
    }
}
