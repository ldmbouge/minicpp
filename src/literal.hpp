#ifndef __LITERAL_H
#define __LITERAL_H

#include <memory>
#include "handle.hpp"
#include "intvar.hpp"
#include "trailable.hpp"
#include "solver.hpp"
#include "avar.hpp"
#include "acstr.hpp"
#include "trailList.hpp"

class LitVar : public AVar {
public:
    typedef handle_ptr<LitVar> Ptr;
    virtual CPSolver::Ptr getSolver() const = 0;
    virtual bool isBound() const = 0;
    virtual bool isTrue() const = 0;
    virtual bool isFalse() const = 0;
    virtual void makePerm() {}
    virtual void updateVal() {}
    virtual void setTrue() {}
    virtual void setFalse() {}
    virtual void assign(bool b) {}
    virtual TLCNode* propagateOnBind(Constraint::Ptr c) {}
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

template<typename T> class LitVarEQ {};
template<>
class LitVarEQ<char> : public LitVar {
    var<int>::Ptr _x;
    int _c;
    char _val;
    void initVal();
public:
    LitVarEQ<char>(const var<int>::Ptr& x, const int c) : _x(x), _c(c), _val(0x02) { initVal();}
    inline bool isBound() const override { return _val != 0x02;}
    inline bool isTrue() const override { return _val == 0x01;}
    inline bool isFalse() const override { return _val == 0x00;}
    inline var<int>::Ptr getVar() const { return _x;}
    inline int getC() const { return _c;}
    inline char getVal() const { return _val;}
    void makePerm() override {}
    friend class LitVarEQ<trail<char>>;
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

template<>
class LitVarEQ<trail<char>> : public LitVar {
    CPSolver::Ptr _solver;
    var<int>::Ptr _x;
    int _c;
    int _id;
    trail<char> _val;
    void initVal();
    trailList<Constraint::Ptr> _onBindList;
protected:
    void setId(int id) override { _id = id;}
    int getId() const { return _id;}
public:
    typedef handle_ptr<LitVarEQ<trail<char>>> Ptr;
    LitVarEQ<trail<char>>(const var<int>::Ptr& x, const int c) 
      : _solver(x->getSolver()),
        _x(x), 
        _c(c), 
        _val(x->getSolver()->getStateManager(), 0x02),
        _onBindList(_x->getSolver()->getStateManager(), _x->getStore())
        { initVal();}
    LitVarEQ<trail<char>>(const LitVarEQ<char>& l)
      : _x(l.getVar()), 
        _c(l.getC()), 
        _val(_x->getSolver()->getStateManager(), l.getVal()), 
        _onBindList(_x->getSolver()->getStateManager(), _x->getStore())
        {}
    LitVarEQ<trail<char>>(const LitVarEQ<char>&& l)
      : _x(std::move(l._x)), 
        _c(std::move(l._c)), 
        _val(_x->getSolver()->getStateManager(),std::move(l._val)), 
        _onBindList(_x->getSolver()->getStateManager(), _x->getStore())
        {}
    inline CPSolver::Ptr getSolver() const override { return _solver;}
    inline bool isBound() const override { return _val != 0x02;}
    inline bool isTrue() const override { return _val == 0x01;}
    inline bool isFalse() const override { return _val == 0x00;}
    void setTrue() override;
    void setFalse() override;
    void assign(bool b) override;
    void makePerm() override {}
    TLCNode* propagateOnBind(Constraint::Ptr c) override { _onBindList.emplace_back(std::move(c));}
};

namespace Factory {
    LitVarEQ<trail<char>>::Ptr makeLitVarEQ(const var<int>::Ptr& x, const int c); 
}

#endif
