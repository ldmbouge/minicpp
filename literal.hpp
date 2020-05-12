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

enum LitRel {EQ, NEQ, LEQ, GEQ};

class Literal {
    var<int>::Ptr _x;
    LitRel _rel;
    int _c;
    Constraint::Ptr _cPtr;
    int _depth;
public:
    void makeVar() {};
    void explain() {};  // TODO: delegate to cPtr->explain(this) when this is implemented and adjust return type
    friend class LitVar;
    friend class LitHash;
};

class LitHash {
public:
    size_t operator()(const Literal& l) const
    { 
        return (size_t) l._x; 
    } 
};

class LitVar : public AVar {
    int _id;
    CPSolver::Ptr _solver;
    var<int>::Ptr _x;
    LitRel _rel;
    int _c;
    trail<char> _val;
    int _depth;
    void initVal();
    trailList<Constraint::Ptr> _onBindList;
protected:
    void setId(int id) override { _id = id;}
    int getId() const { return _id;}
public:
    typedef handle_ptr<LitVar> Ptr;
    LitVar(const Literal& l)
      : _solver(l._x->getSolver()),
        _x(l._x),
        _c(l._c),
        _depth(l._depth),
        _rel(l._rel),
        _val(_x->getSolver()->getStateManager(), 0x02),
        _onBindList(_x->getSolver()->getStateManager(), _x->getStore())
    { initVal();
      _x->whenDomainChange([&] {updateVal();});
    }
    LitVar(const var<int>::Ptr& x, const int c, LitRel r) 
      : _solver(x->getSolver()),
        _x(x), 
        _c(c), 
        _depth(0), // depth is 0 because this constructor is used for literal variables initiated in model declaration, not during lcg
        _rel(r),
        _val(x->getSolver()->getStateManager(), 0x02),
        _onBindList(_x->getSolver()->getStateManager(), _x->getStore())
        { initVal();
          _x->whenDomainChange([&] {updateVal();});
        }
    inline CPSolver::Ptr getSolver() const { return _solver;}
    inline bool isBound() const { return _val != 0x02;}
    inline bool isTrue() const { return _val == 0x01;}
    inline bool isFalse() const { return _val == 0x00;}
    void setTrue();
    void setFalse();
    void updateVal();
    void assign(bool b);
    TLCNode* propagateOnBind(Constraint::Ptr c) { _onBindList.emplace_back(std::move(c));return nullptr;}
};

namespace Factory {
    LitVar::Ptr makeLitVarEQ(const var<int>::Ptr& x, const int c); 
    LitVar::Ptr makeLitVarNEQ(const var<int>::Ptr& x, const int c); 
    LitVar::Ptr makeLitVarLEQ(const var<int>::Ptr& x, const int c); 
    LitVar::Ptr makeLitVarGEQ(const var<int>::Ptr& x, const int c); 
}

#endif
