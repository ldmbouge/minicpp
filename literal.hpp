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
    ConListener* _cLis;
    int _depth;
public:
    Literal() 
      : _x(nullptr), _rel(EQ), _c(0), _cLis(nullptr), _depth(0) {}
    Literal(var<int>::Ptr x, LitRel rel, int c, ConListener* cLis, int depth)
      : _x(x), _rel(rel), _c(c), _cLis(cLis), _depth(depth) {}
    Literal(const Literal& l)
      : _x(l._x), _rel(l._rel), _c(l._c), _cLis(l._cLis), _depth(l._depth) {}
    handle_ptr<LitVar> makeVar();
    std::vector<Literal*> explain();
    friend class LitVar;
    friend unsigned long litKey(const Literal&);
    bool operator==(const Literal& other) const;
    bool isValid() const;
    int getDepth() const { return _depth;}
    var<int>::Ptr getVar() const { return _x;}
    LitRel getRel() const { return _rel;}
    ConListener* getConLis() const { return _cLis;}
    int getVal() const { return _c;}
    void print(std::ostream& os) const {
      os << "<x_" << _x->getId() << LitRelString(_rel) << _c << " @ " << _depth << ">";
    }
    friend std::ostream& operator<<(std::ostream& os, const Literal& l) {
      l.print(os);
      return os;
    }
};

unsigned long litKey(const Literal&);
Literal litNegation(const Literal&);

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
    LitVar(const Literal&);
    LitVar(const Literal&&);
    LitVar(const var<int>::Ptr&, const int, LitRel);
    int getId() const override { return _id;}
    inline CPSolver::Ptr getSolver() const { return _solver;}
    inline bool isBound() { updateVal(); return _val != 0x02;}
    inline bool isTrue() { updateVal(); return _val == 0x01;}
    inline bool isFalse() { updateVal(); return _val == 0x00;}
    void setTrue();
    void setFalse();
    void updateVal();
    void assign(bool b);
    virtual IntNotifier* getListener() const override { return nullptr;}
    virtual void setListener(IntNotifier*) override {}
    TLCNode* propagateOnBind(Constraint::Ptr c) { _onBindList.emplace_back(std::move(c));return nullptr;}
    void print();
};

namespace Factory {
    LitVar::Ptr makeLitVar(const Literal&&); 
    LitVar::Ptr makeLitVarEQ(const var<int>::Ptr& x, const int c); 
    LitVar::Ptr makeLitVarNEQ(const var<int>::Ptr& x, const int c); 
    LitVar::Ptr makeLitVarLEQ(const var<int>::Ptr& x, const int c); 
    LitVar::Ptr makeLitVarGEQ(const var<int>::Ptr& x, const int c); 
}

#endif
