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
    virtual TLCNode* propagateOnBind(Constraint::Ptr c) { return NULL;}
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% EQ

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
        { initVal();
          _x->whenDomainChange([&] {updateVal();});
        }
    LitVarEQ<trail<char>>(const LitVarEQ<char>& l)
      : _solver(l.getVar()->getSolver()),
        _x(l.getVar()), 
        _c(l.getC()), 
        _val(_x->getSolver()->getStateManager(), l.getVal()), 
        _onBindList(_x->getSolver()->getStateManager(), _x->getStore())
        { _x->whenDomainChange([&] {updateVal();});}
    LitVarEQ<trail<char>>(const LitVarEQ<char>&& l)
      : _solver(l.getVar()->getSolver()),
        _x(std::move(l._x)), 
        _c(std::move(l._c)), 
        _val(_x->getSolver()->getStateManager(),std::move(l._val)), 
        _onBindList(_x->getSolver()->getStateManager(), _x->getStore())
        { _x->whenDomainChange([&] {updateVal();});}
    inline CPSolver::Ptr getSolver() const override { return _solver;}
    inline bool isBound() const override { return _val != 0x02;}
    inline bool isTrue() const override { return _val == 0x01;}
    inline bool isFalse() const override { return _val == 0x00;}
    void setTrue() override;
    void setFalse() override;
    void updateVal() override;
    void assign(bool b) override;
    void makePerm() override {}
   TLCNode* propagateOnBind(Constraint::Ptr c) override { _onBindList.emplace_back(std::move(c));return nullptr;}
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% NEQ

template<typename T> class LitVarNEQ {};
template<>
class LitVarNEQ<char> : public LitVar {
    var<int>::Ptr _x;
    int _c;
    char _val;
    void initVal();
public:
    LitVarNEQ<char>(const var<int>::Ptr& x, const int c) : _x(x), _c(c), _val(0x02) { initVal();}
    inline bool isBound() const override { return _val != 0x02;}
    inline bool isTrue() const override { return _val == 0x01;}
    inline bool isFalse() const override { return _val == 0x00;}
    inline var<int>::Ptr getVar() const { return _x;}
    inline int getC() const { return _c;}
    inline char getVal() const { return _val;}
    void makePerm() override {}
    friend class LitVarNEQ<trail<char>>;
};

template<>
class LitVarNEQ<trail<char>> : public LitVar {
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
    typedef handle_ptr<LitVarNEQ<trail<char>>> Ptr;
    LitVarNEQ<trail<char>>(const var<int>::Ptr& x, const int c) 
      : _solver(x->getSolver()),
        _x(x), 
        _c(c), 
        _val(x->getSolver()->getStateManager(), 0x02),
        _onBindList(_x->getSolver()->getStateManager(), _x->getStore())
        { initVal();
          _x->whenDomainChange([&] {updateVal();});
        }
    LitVarNEQ<trail<char>>(const LitVarNEQ<char>& l)
      : _x(l.getVar()), 
        _c(l.getC()), 
        _val(_x->getSolver()->getStateManager(), l.getVal()), 
        _onBindList(_x->getSolver()->getStateManager(), _x->getStore())
        { _x->whenDomainChange([&] {updateVal();});}
    LitVarNEQ<trail<char>>(const LitVarNEQ<char>&& l)
      : _x(std::move(l._x)), 
        _c(std::move(l._c)), 
        _val(_x->getSolver()->getStateManager(),std::move(l._val)), 
        _onBindList(_x->getSolver()->getStateManager(), _x->getStore())
        { _x->whenDomainChange([&] {updateVal();});}
    inline CPSolver::Ptr getSolver() const override { return _solver;}
    inline bool isBound() const override { return _val != 0x02;}
    inline bool isTrue() const override { return _val == 0x01;}
    inline bool isFalse() const override { return _val == 0x00;}
    void setTrue() override;
    void setFalse() override;
    void updateVal() override;
    void assign(bool b) override;
    void makePerm() override {}
    TLCNode* propagateOnBind(Constraint::Ptr c) override { _onBindList.emplace_back(std::move(c));return nullptr;}
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% GEQ

template<typename T> class LitVarGEQ {};
template<>
class LitVarGEQ<char> : public LitVar {
    var<int>::Ptr _x;
    int _c;
    char _val;
    void initVal();
public:
    LitVarGEQ<char>(const var<int>::Ptr& x, const int c) : _x(x), _c(c), _val(0x02) { initVal();}
    inline bool isBound() const override { return _val != 0x02;}
    inline bool isTrue() const override { return _val == 0x01;}
    inline bool isFalse() const override { return _val == 0x00;}
    inline var<int>::Ptr getVar() const { return _x;}
    inline int getC() const { return _c;}
    inline char getVal() const { return _val;}
    void makePerm() override {}
    friend class LitVarGEQ<trail<char>>;
};

template<>
class LitVarGEQ<trail<char>> : public LitVar {
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
    typedef handle_ptr<LitVarGEQ<trail<char>>> Ptr;
    LitVarGEQ<trail<char>>(const var<int>::Ptr& x, const int c) 
      : _solver(x->getSolver()),
        _x(x), 
        _c(c), 
        _val(x->getSolver()->getStateManager(), 0x02),
        _onBindList(_x->getSolver()->getStateManager(), _x->getStore())
        { initVal();
          _x->whenDomainChange([&] {updateVal();});
        }
    LitVarGEQ<trail<char>>(const LitVarGEQ<char>& l)
      : _x(l.getVar()), 
        _c(l.getC()), 
        _val(_x->getSolver()->getStateManager(), l.getVal()), 
        _onBindList(_x->getSolver()->getStateManager(), _x->getStore())
        { _x->whenDomainChange([&] {updateVal();});}
    LitVarGEQ<trail<char>>(const LitVarGEQ<char>&& l)
      : _x(std::move(l._x)), 
        _c(std::move(l._c)), 
        _val(_x->getSolver()->getStateManager(),std::move(l._val)), 
        _onBindList(_x->getSolver()->getStateManager(), _x->getStore())
        { _x->whenDomainChange([&] {updateVal();});}
    inline CPSolver::Ptr getSolver() const override { return _solver;}
    inline bool isBound() const override { return _val != 0x02;}
    inline bool isTrue() const override { return _val == 0x01;}
    inline bool isFalse() const override { return _val == 0x00;}
    void setTrue() override;
    void setFalse() override;
    void updateVal() override;
    void assign(bool b) override;
    void makePerm() override {}
    TLCNode* propagateOnBind(Constraint::Ptr c) override { _onBindList.emplace_back(std::move(c));return nullptr;}
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% LEQ

template<typename T> class LitVarLEQ {};
template<>
class LitVarLEQ<char> : public LitVar {
    var<int>::Ptr _x;
    int _c;
    char _val;
    void initVal();
public:
    LitVarLEQ<char>(const var<int>::Ptr& x, const int c) : _x(x), _c(c), _val(0x02) { initVal();}
    inline bool isBound() const override { return _val != 0x02;}
    inline bool isTrue() const override { return _val == 0x01;}
    inline bool isFalse() const override { return _val == 0x00;}
    inline var<int>::Ptr getVar() const { return _x;}
    inline int getC() const { return _c;}
    inline char getVal() const { return _val;}
    void makePerm() override {}
    friend class LitVarLEQ<trail<char>>;
};

template<>
class LitVarLEQ<trail<char>> : public LitVar {
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
    typedef handle_ptr<LitVarLEQ<trail<char>>> Ptr;
    LitVarLEQ<trail<char>>(const var<int>::Ptr& x, const int c) 
      : _solver(x->getSolver()),
        _x(x), 
        _c(c), 
        _val(x->getSolver()->getStateManager(), 0x02),
        _onBindList(_x->getSolver()->getStateManager(), _x->getStore())
        { initVal();
          _x->whenDomainChange([&] {updateVal();});
        }
    LitVarLEQ<trail<char>>(const LitVarLEQ<char>& l)
      : _x(l.getVar()), 
        _c(l.getC()), 
        _val(_x->getSolver()->getStateManager(), l.getVal()), 
        _onBindList(_x->getSolver()->getStateManager(), _x->getStore())
        { _x->whenDomainChange([&] {updateVal();});}
    LitVarLEQ<trail<char>>(const LitVarLEQ<char>&& l)
      : _x(std::move(l._x)), 
        _c(std::move(l._c)), 
        _val(_x->getSolver()->getStateManager(),std::move(l._val)), 
        _onBindList(_x->getSolver()->getStateManager(), _x->getStore())
        { _x->whenDomainChange([&] {updateVal();});}
    inline CPSolver::Ptr getSolver() const override { return _solver;}
    inline bool isBound() const override { return _val != 0x02;}
    inline bool isTrue() const override { return _val == 0x01;}
    inline bool isFalse() const override { return _val == 0x00;}
    void setTrue() override;
    void setFalse() override;
    void updateVal() override;
    void assign(bool b) override;
    void makePerm() override {}
    TLCNode* propagateOnBind(Constraint::Ptr c) override { _onBindList.emplace_back(std::move(c));return nullptr;}
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

namespace Factory {
    LitVarEQ<trail<char>>::Ptr makeLitVarEQ(const var<int>::Ptr& x, const int c); 
    LitVarNEQ<trail<char>>::Ptr makeLitVarNEQ(const var<int>::Ptr& x, const int c); 
    LitVarGEQ<trail<char>>::Ptr makeLitVarGEQ(const var<int>::Ptr& x, const int c); 
    LitVarLEQ<trail<char>>::Ptr makeLitVarLEQ(const var<int>::Ptr& x, const int c); 
}

#endif
