#ifndef __SEARCH_H
#define __SEARCH_H

#include "solver.hpp"
#include <vector>
#include <initializer_list>
#include <functional>

class Branches {
    std::vector<std::function<void(void)>> _alts;
public:
    Branches(std::initializer_list<std::function<void(void)>> alts)
    {
        _alts.insert(_alts.begin(),alts.begin(),alts.end());
    }
    std::vector<std::function<void(void)>>::iterator begin() { return _alts.begin();}
    std::vector<std::function<void(void)>>::iterator end()   { return _alts.end();}
    size_t size() const { return _alts.size();}
};

class Chooser {
    std::function<Branches(void)> _sel;
public:
    Chooser(std::function<Branches(void)> sel) : _sel(sel) {}
    Branches operator()() { return _sel();}
};

void dfsAll(CPSolver::Ptr cps,Chooser& c,std::function<void(void)> onSol);

template <class B0,class B1> inline Branches operator|(B0 b0,B1 b1) {
    return Branches({b0,b1});
}

template<class Container,typename Cond,typename Fun,typename Do>
inline Branches selectMin(Container& c,Cond test,Fun f,Do block) {
    auto from = c.begin(),to  = c.end();
    auto min = to;
    int ds = 0x7fffffff;
    for(;from != to;++from) {
        auto fv = f(*from);
        if (test(*from) && fv < ds) {
            min = from;
            ds = fv;
        }
    }
    if (min != to) {
        return block(*min);
    } else
        return Branches({});
}

#endif
