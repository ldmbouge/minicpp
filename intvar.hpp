#ifndef __INTVAR_H
#define __INTVAR_H

#include <iostream>
#include <vector>
#include "handle.hpp"
#include "avar.hpp"
#include "solver.hpp"
#include "BitDomain.hpp"

template<typename T> class var {};

template<>
class var<int> :public AVar, public IntNotifier {
    handle_ptr<CPSolver>    _solver;
    BitDomain::Ptr             _dom;
    int                         _id;
    revList<std::function<void(void)>> _onBindList;
    revList<std::function<void(void)>> _onBoundsList;
protected:
    void setId(int id) override { _id = id;}
public:
    typedef handle_ptr<var<int>> Ptr;
    var<int>(CPSolver::Ptr& cps,int min,int max);
    ~var<int>();
    int getMin() const { return _dom->getMin();}
    int getMax() const { return _dom->getMax();}
    int getSize() const { return _dom->getSize();}
    bool isBound() const { return _dom->isBound();}
    bool contains(int v) const { return _dom->member(v);}

    void bind(int v);
    void remove(int v);
    void updateMin(int newMin);
    void updateMax(int newMax);
    void updateBounds(int newMin,int newMax);
    
    void bindEvt() override;
    void domEvt(int sz)  override;
    void updateMinEvt(int sz) override;
    void updateMaxEvt(int sz) override;

    auto whenBind(std::function<void(void)>&& f)         { return _onBindList.emplace_back(std::move(f));}
    auto whenChangeBounds(std::function<void(void)>&& f) { return _onBoundsList.emplace_back(std::move(f));}
    
    friend std::ostream& operator<<(std::ostream& os,const var<int>& x) {
        if (x.getSize() == 1)
            os << x.getMin();
        else
	  os << "x_" << x._id << '(' << *x._dom << ')';
	//os << "\n\tonBIND  :" << x._onBindList << std::endl;
	//os << "\tonBOUNDS:" << x._onBoundsList << std::endl;
	return os;
    }
};

inline std::ostream& operator<<(std::ostream& os,const var<int>::Ptr& xp) {
    return os << *xp;
}

template <class T> inline std::ostream& operator<<(std::ostream& os,const std::vector<T>& v) {
    os << '[';
    for(auto& e : v)
        os << e << ',';
    return os << '\b' << ']';
}

namespace Factory {
    inline var<int>::Ptr makeIntVar(CPSolver::Ptr cps,int min,int max);
    std::vector<var<int>::Ptr> intVarArray(CPSolver::Ptr cps,int sz,int min,int max);
};

template<class ForwardIt> ForwardIt min_dom(ForwardIt first, ForwardIt last)
{
    if (first == last) return last;

    int ds = 0x7fffffff;
    ForwardIt smallest = last;
    for (; first != last; ++first) {
        auto fsz = (*first)->getSize();
        if (fsz > 1 && fsz < ds) {
            smallest = first;
            ds = fsz;
        }
    }
    return smallest;
}

template<class Container> auto min_dom(Container& c) {
    return min_dom(c.begin(),c.end());
}

#endif
