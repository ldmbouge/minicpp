#ifndef __LITERAL_H
#define __LITERAL_H

#include <memory>
#include "handle.hpp"
#include "intvar.hpp"
#include "trailable.hpp"
#include "solver.hpp"

class LitVar : public AVar {
protected:
    CPSolver::Ptr _solver;
    trailList<Constraint::Ptr> _onBindList;
   
public:
    LitVar(CPSolver::Ptr cps) : _solver(cps), _onBindList(cps->getStateManager(),cps->getStore()) {}
    virtual bool isBound() = 0;
    virtual bool isTrue() = 0;
    virtual bool isFalse() = 0;
    virtual void updateVal() = 0;
    virtual void setTrue() = 0;
    virtual void setFalse() = 0;
    virtual void assign(bool b) = 0;
    virtual TLCNode* propagateOnBind(Constraint::Ptr c) = 0;
    friend struct LitNotifier;
};

struct LitNotifier   {
    LitVar* theLit;
    LitNotifier(LitVar* x) : theLit(x) {}
    void bind() {
        for(auto& f : theLit->_onBindList)
            theLit->_solver->schedule(f);
    }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

template<typename T>
class LitVarEQ : public LitVar {
    var<int>::Ptr _x;
    int _c;
    int _id;
    T _val;
    // trailList<Constraint::Ptr> _onBindList;
    LitNotifier* _litNotifier;
protected:
    void setId(int id) override {_id = id;}
    int getId() const { return _id;}
public:
    typedef handle_ptr<LitVarEQ<T>> Ptr;
    // LitVarEQ(const var<int>::Ptr& x, const int c) : _x(x), _c(c), _val(0x02) { initVal();}
    LitVarEQ(const var<int>::Ptr& x, const int c, const Storage::Ptr& s)
      : LitVar(x->getSolver()), _x(x), _c(c), _val(x->getSolver()->getStateManager(), 0x02) { initVal();}
    inline bool isBound() override {return _val != 0x02;}
    inline bool isTrue() override {return _val == 0x01;}
    inline bool isFalse() override {return _val == 0x00;}
    void setTrue() override;
    void setFalse() override;
    void assign(bool b) override;
    void updateVal() override;
    void initVal();
    TLCNode* propagateOnBind(Constraint::Ptr c) { return _onBindList.emplace_back(std::move(c));}
};

template<typename T>
void LitVarEQ<T>::initVal() {
    if (_x->contains(_c)) {
        if (_x->size() == 1) _val = 0x01;
        else _val = 0x02;
    }
    else _val = 0x00;
}

template<typename T>
void LitVarEQ<T>::updateVal() {
    if (isBound()) return;
    if (_x->contains(_c)) {
        if (_x->size() == 1) setTrue();
    }
    else setFalse();
}

template<typename T>
void LitVarEQ<T>::setTrue() {
    if (isBound()) return;
    _val = 0x01;
    _x->assign(_c);
}

template<typename T>
void LitVarEQ<T>::setFalse() {
    if (isBound()) return;
    _val = 0x00;
    _x->remove(_c);
}

template<typename T>
void LitVarEQ<T>::assign(bool b) {
    b ? setTrue() : setFalse();
    _litNotifier->bind();
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

template<typename T>
class LitVarNEQ : public LitVar {
    var<int>::Ptr _x;
    int _c;
    int _id;
    T _val;
    // trailList<Constraint::Ptr> _onBindList;
    LitNotifier* _litNotifier;
protected:
    void setId(int id) override {_id = id;}
    int getId() const { return _id;}
public:
    typedef handle_ptr<LitVarNEQ<T>> Ptr;
    // LitVarNEQ(const var<int>::Ptr& x, const int c) : _x(x), _c(c), _val(0x02) { initVal();}
    LitVarNEQ(const var<int>::Ptr& x, const int c, const Storage::Ptr& s)
      : LitVar(x->getSolver()), _x(x), _c(c), _val(x->getSolver()->getStateManager(), 0x02) { initVal();}
    inline bool isBound() override {return _val != 0x02;}
    inline bool isTrue() override {return _val == 0x01;}
    inline bool isFalse() override {return _val == 0x00;}
    inline void setTrue() override;
    inline void setFalse() override;
    void assign(bool b) override;
    void updateVal() override;
    void initVal();
    TLCNode* propagateOnBind(Constraint::Ptr c) { return _onBindList.emplace_back(std::move(c));}
};

template<typename T>
void LitVarNEQ<T>::initVal() {
    if (_x->contains(_c)) {
        if (_x->size() == 1) _val = 0x00;
        else _val = 0x02;
    }
    else _val = 0x01;
}

template<typename T>
void LitVarNEQ<T>::updateVal() {
    if (isBound()) return;
    if (_x->contains(_c)) {
        if (_x->size() == 1) setFalse();
    }
    else setTrue();
}

template<typename T>
void LitVarNEQ<T>::setTrue() {
    if (isBound()) return;
    _val = 0x01;
    _x->remove(_c);
}

template<typename T>
void LitVarNEQ<T>::setFalse() {
    if (isBound()) return;
    _val = 0x00;
    _x->assign(_c);
}

template<typename T>
void LitVarNEQ<T>::assign(bool b) {
    b ? setTrue() : setFalse();
    _litNotifier->bind();
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

template<typename T>
class LitVarGEQ : public LitVar {
    var<int>::Ptr _x;
    int _c;
    int _id;
    T _val;
    // trailList<Constraint::Ptr> _onBindList;
    LitNotifier* _litNotifier;
protected:
    void setId(int id) override {_id = id;}
    int getId() const { return _id;}
public:
    typedef handle_ptr<LitVarGEQ<T>> Ptr;
    // LitVarGEQ(const var<int>::Ptr& x, const int c) : _x(x), _c(c), _val(0x02) { initVal();}
    LitVarGEQ(const var<int>::Ptr& x, const int c, const Storage::Ptr& s)
      : LitVar(x->getSolver()), _x(x), _c(c), _val(x->getSolver()->getStateManager(), 0x02) { initVal();}
    inline bool isBound() override {return _val != 0x02;}
    inline bool isTrue() override {return _val == 0x01;}
    inline bool isFalse() override {return _val == 0x00;}
    inline void setTrue() override;
    inline void setFalse() override;
    void assign(bool b) override;
    void updateVal() override;
    void initVal();
    TLCNode* propagateOnBind(Constraint::Ptr c) { return _onBindList.emplace_back(std::move(c));}
};

template<typename T>
void LitVarGEQ<T>::initVal() {
    if (_x->min() >= _c) {
        _val = 0x01;
    }
    else if (_x->max() < _c) {
        _val = 0x00;
    }
    else 
        _val = 0x02;
}

template<typename T>
void LitVarGEQ<T>::updateVal() {
    if (isBound()) return;
    if (_x->min() >= _c) {
        setTrue();
    }
    else if (_x->max() < _c) {
        setFalse();
    }
}

template<typename T>
void LitVarGEQ<T>::setTrue() {
    if (isBound()) return;
    _val = 0x01;
    _x->removeBelow(_c);
}

template<typename T>
void LitVarGEQ<T>::setFalse() {
    if (isBound()) return;
    _val = 0x00;
    _x->removeAbove(_c - 1);
}

template<typename T>
void LitVarGEQ<T>::assign(bool b) {
    b ? setTrue() : setFalse();
    _litNotifier->bind();
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

template<typename T>
class LitVarLEQ : public LitVar {
    var<int>::Ptr _x;
    int _c;
    int _id;
    T _val;
    // trailList<Constraint::Ptr> _onBindList;
    LitNotifier* _litNotifier;
protected:
    void setId(int id) override {_id = id;}
    int getId() const { return _id;}
public:
    typedef handle_ptr<LitVarLEQ<T>> Ptr;
    // LitVarLEQ(const var<int>::Ptr& x, const int c) : _x(x), _c(c), _val(0x02) { initVal();}
    LitVarLEQ(const var<int>::Ptr& x, const int c, const Storage::Ptr& s)
      : LitVar(x->getSolver()), _x(x), _c(c), _val(x->getSolver()->getStateManager(), 0x02) { initVal();}
    inline bool isBound() override {return _val != 0x02;}
    inline bool isTrue() override {return _val == 0x01;}
    inline bool isFalse() override {return _val == 0x00;}
    inline void setTrue() override;
    inline void setFalse() override;
    void assign(bool b) override;
    void updateVal() override;
    void initVal();
    TLCNode* propagateOnBind(Constraint::Ptr c) { return _onBindList.emplace_back(std::move(c));}
};

template<typename T>
void LitVarLEQ<T>::initVal() {
    if (_x->max() <= _c) {
        _val = 0x01;
    }
    else if (_x->min() > _c) {
        _val = 0x00;
    }
    else 
        _val = 0x02;
}

template<typename T>
void LitVarLEQ<T>::updateVal() {
    if (isBound()) return;
    if (_x->max() <= _c) {
        setTrue();
    }
    else if (_x->min() > _c) {
        setFalse();
    }
}

template<typename T>
void LitVarLEQ<T>::setTrue() {
    if (isBound()) return;
    _val = 0x01;
    _x->removeAbove(_c);
}

template<typename T>
void LitVarLEQ<T>::setFalse() {
    if (isBound()) return;
    _val = 0x00;
    _x->removeBelow(_c+1);
}

template<typename T>
void LitVarLEQ<T>::assign(bool b) {
    b ? setTrue() : setFalse();
    _litNotifier->bind();
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


namespace Factory {
    // LitVarEQ<char> tempLitVarEQ(const var<int>::Ptr& x, const int c) {
    //     return LitVarEQ<char>(x,c);
    // }
    LitVarEQ<trail<char>>::Ptr makeLitVarEQ(const var<int>::Ptr& x, const int c, const Storage::Ptr& s) {
        auto rv = new (s) LitVarEQ<trail<char>>(x,c,s);
        x->getSolver()->registerVar(rv);
        return rv;
    }
    // LitVarNEQ<char> tempLitVarNEQ(const var<int>::Ptr& x, const int c) {
    //     return LitVarNEQ<char>(x,c);
    // }
    LitVarNEQ<trail<char>>::Ptr makeLitVarNEQ(const var<int>::Ptr& x, const int c, const Storage::Ptr& s) {
        auto rv = new (s) LitVarNEQ<trail<char>>(x,c,s);
        x->getSolver()->registerVar(rv);
        return rv;
    }
    // LitVarGEQ<char> tempLitVarGEQ(const var<int>::Ptr& x, const int c) {
    //     return LitVarGEQ<char>(x,c);
    // }
    LitVarGEQ<trail<char>>::Ptr makeLitVarGEQ(const var<int>::Ptr& x, const int c, const Storage::Ptr& s) {
        auto rv = new (s) LitVarGEQ<trail<char>>(x,c,s);
        x->getSolver()->registerVar(rv);
        return rv;
    }
    // LitVarLEQ<char> tempLitVarLEQ(const var<int>::Ptr& x, const int c) {
    //     return LitVarLEQ<char>(x,c);
    // }
    LitVarLEQ<trail<char>>::Ptr makeLitVarLEQ(const var<int>::Ptr& x, const int c, const Storage::Ptr& s) {
        auto rv = new (s) LitVarLEQ<trail<char>>(x,c,s);
        x->getSolver()->registerVar(rv);
        return rv;
    }
}

#endif