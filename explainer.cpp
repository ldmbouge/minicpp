#include "explainer.hpp"
#include "constraint.hpp"
#include <string>
#include <iostream>

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
            for (int v = x->min(); v < x->max() + 1; ++v) {  // TODO: add domain iterator for variables - this does not work if domain has holes
                if (v != val)
                    s += "| var_" + std::to_string(i) + " != " + std::to_string(v) + " | ";
            }
        }
    }
    std::cout << s << std::endl;
}
