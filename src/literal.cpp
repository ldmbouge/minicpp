#include "literal.hpp"

namespace Factory {
    LitVarEQ<trail<char>>::Ptr makeLitVarEQ(const var<int>::Ptr& x, const int c) {
        auto rv = new (x->getStore()) LitVarEQ<trail<char>>(x,c);
        x->getSolver()->registerVar(rv);
        return rv;
    }
}