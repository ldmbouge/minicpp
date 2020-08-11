#ifndef __LITERAL_H
#define __LITERAL_H

#include <memory>
#include <unordered_map>
#include <string>
#include <iostream>
#include "handle.hpp"
#include "intvar.hpp"
#include "trailable.hpp"
#include "solver.hpp"
#include "avar.hpp"
#include "acstr.hpp"
#include "trailList.hpp"
#include "domain.hpp"

enum LitRel {EQ, NEQ, LEQ, GEQ};

inline std::string LitRelString(const LitRel& rel) {
    switch(rel) {
      case  EQ : return " == ";
      case NEQ : return " != ";
      case LEQ : return " <= ";
      case GEQ : return " >= ";
    }
}

class LitVar;

class Literal {
    var<int>::Ptr _x;
    LitRel _rel;
    int _c;
    Constraint::Ptr _cPtr;
    int _depth;
public:
    Literal(var<int>::Ptr x, LitRel rel, int c, Constraint::Ptr cPtr, int depth)
      : _x(x), _rel(rel), _c(c), _cPtr(cPtr), _depth(depth) {}
    Literal(const Literal& l)
      : _x(l._x), _rel(l._rel), _c(l._c), _cPtr(l._cPtr), _depth(l._depth) {}
    handle_ptr<LitVar> makeVar();
    std::vector<Literal*> explain();
    friend class LitVar;
    friend unsigned long litKey(const Literal&);
    bool operator==(const Literal& other) const;
    bool isValid() const;
    int getDepth() { return _depth;}
    void print(std::ostream& os) const {
      os << "<x_" << _x->getId() << LitRelString(_rel) << _c << " @ " << _depth << ">";
    }
    friend std::ostream& operator<<(std::ostream& os, const Literal& l) {
      l.print(os);
      return os;
    }
};

unsigned long litKey(const Literal&);

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