#include "explainer.hpp"
#include "constraint.hpp"
#include <string>
#include <iostream>

void Explainer::injectListeners() {
    for (auto x : _es->_cps->_iVars) {
        auto l = new (_es->getStore()) ExpListener(this,x);
        _listeners.push_back(l);
    }
}

void Explainer::bind(AVar::Ptr x, int a) {
    std::cout << "x_" << x->getId() << " == " << a << std::endl;
}

void Explainer::remove(AVar::Ptr x, int a) {
    std::cout << "x_" << x->getId() << " != " << a << std::endl;
}

void Explainer::changeMin(AVar::Ptr x, int newMin) {
    std::cout << "x_" << x->getId() << " >= " << newMin << std::endl;
}

void Explainer::changeMax(AVar::Ptr x, int newMax) {
    std::cout << "x_" << x->getId() << " <= " << newMax << std::endl;
}


void AllDiffExplainer::explain(var<int>::Ptr x, int val) {
    std::cout << "explaining " << x << " != " << val << std::endl;
    MaximumMatching& mm = _c->_mm;
    Graph& rg = _c->_rg;
    std::string s = "";
    _c->updateRange();
    _c->updateGraph();
    int nc = 0;
    int scc[_c->_nNodes];
    rg.SCC([&scc,&nc](int n,int nd[]) {
               for(int i=0;i < n;++i)
                   scc[nd[i]] = nc;
               ++nc;
           });
    int sccOfVal = scc[_c->valNode(val)];
    for (int i = 0; i < _c->_nVar; ++i) {
        if ((scc[i] == sccOfVal) || (val == _c->_match[i])) {
            for (int v = x->min(); v < x->max() + 1; ++v) {
                if (x->contains(v)) {
                    if (v != val)
                        s += "| var_" + std::to_string(i) + " != " + std::to_string(v) + " | ";
                }
            }
        }
    }
    std::cout << s << std::endl;
}
