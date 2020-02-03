#ifndef __EXPLAINER_H
#define __EXPLAINER_H

#include "handle.hpp"
// #include "constraint.hpp"
#include "intvar.hpp"

class AllDifferentAC;

class AllDiffExplainer {
    AllDifferentAC* _c;
public:
    typedef handle_ptr<AllDiffExplainer> Ptr;
    AllDiffExplainer(AllDifferentAC* c)
        : _c(c) {}
    ~AllDiffExplainer() {}
    void explain(var<int>::Ptr x, int val);
};


#endif