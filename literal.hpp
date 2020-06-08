#ifndef __LITERAL_H
#define __LITERAL_H

#include <memory>
#include <unordered_map>
#include "handle.hpp"
#include "intvar.hpp"
#include "trailable.hpp"
#include "solver.hpp"
#include "avar.hpp"
#include "acstr.hpp"
#include "trailList.hpp"
#include "domain.hpp"

enum LitRel {EQ, NEQ, LEQ, GEQ};

class Literal {
    var<int>::Ptr _x;
    LitRel _rel;
    int _c;
    Constraint::Ptr _cPtr;
    int _depth;
public:
    Literal(var<int>::Ptr x, LitRel rel, int c, Constraint::Ptr cPtr, int depth)
      : _x(x), _rel(rel), _c(c), _cPtr(cPtr), _depth(depth) {}
    void makeVar() {};
    void explain() {};  // TODO: delegate to cPtr->explain(this) when this is implemented and adjust return type
    friend class LitVar;
    // friend class LitHash;
};

// class LitHash {
// public:
//     size_t operator()(const Literal& l) const
//     { 
//         size_t k = ((0x0000ff & l._x->getId()) << 16) | ((0x0000ff & l._c) << 8)  | (0x0000ff & l._rel); 
//         return std::hash<size_t>()(k);
//     } 
// };

// typedef std::unordered_map<size_t, Literal, LitHash> LitHashSet;

class LitVar : public AVar {
    int _id;
    int _depth;
    int _c;
    CPSolver::Ptr _solver;
    var<int>::Ptr _x;
    LitRel _rel;
    trail<char> _val;
    void initVal();
    trailList<Constraint::Ptr> _onBindList;
protected:
    void setId(int id) override { _id = id;}
public:
    typedef handle_ptr<LitVar> Ptr;
    LitVar(const Literal& l)
      : _depth(l._depth),
        _c(l._c),
        _solver(l._x->getSolver()),
        _x(l._x),
        _rel(l._rel),
        _val(_x->getSolver()->getStateManager(), 0x02),
        _onBindList(_x->getSolver()->getStateManager(), _x->getStore())
    { initVal();
      _x->whenDomainChange([&] {updateVal();});
    }
    LitVar(const var<int>::Ptr& x, const int c, LitRel r) 
      : _depth(0), // depth is 0 because this constructor is used for literal variables initiated in model declaration, not during lcg
        _c(c), 
        _solver(x->getSolver()),
        _x(x),
        _rel(r),
        _val(x->getSolver()->getStateManager(), 0x02),
        _onBindList(_x->getSolver()->getStateManager(), _x->getStore())
        { initVal();
          _x->whenDomainChange([&] {updateVal();});
        }
    int getId() const override { return _id;}
    inline CPSolver::Ptr getSolver() const { return _solver;}
    inline bool isBound() const { return _val != 0x02;}
    inline bool isTrue() const { return _val == 0x01;}
    inline bool isFalse() const { return _val == 0x00;}
    void setTrue();
    void setFalse();
    void updateVal();
    void assign(bool b);
    virtual IntNotifier* getListener() const override { return nullptr;}
    virtual void setListener(IntNotifier*) override {}
    TLCNode* propagateOnBind(Constraint::Ptr c) { _onBindList.emplace_back(std::move(c));return nullptr;}
};

namespace Factory {
    LitVar::Ptr makeLitVarEQ(const var<int>::Ptr& x, const int c); 
    LitVar::Ptr makeLitVarNEQ(const var<int>::Ptr& x, const int c); 
    LitVar::Ptr makeLitVarLEQ(const var<int>::Ptr& x, const int c); 
    LitVar::Ptr makeLitVarGEQ(const var<int>::Ptr& x, const int c); 
}

#endif
