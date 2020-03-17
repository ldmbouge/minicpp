#ifndef __EXPLAINER_H
#define __EXPLAINER_H

#include "handle.hpp"
#include "intvar.hpp"
// #include "constraint.hpp"

class AllDifferentAC;

class AllDiffExplainer {
    AllDifferentAC* _c;
public:
    typedef handle_ptr<AllDiffExplainer> Ptr;
    AllDiffExplainer(AllDifferentAC* c)
        : _c(c) {}
    ~AllDiffExplainer() {}
    void explain(Literal& l) {};
    void explain(var<int>::Ptr x, int val);
};


#endif