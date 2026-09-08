#pragma once

#include <vector>

#include "constraint.hpp"
#include "intvar.hpp"

// Symmetry breaking for interchangeable "bins": a value can only be used by a
// row if some earlier row already uses the previous value.
class LexChainBit : public Constraint
{
    protected:
        std::vector<var<int>::Ptr> _rows;
        int const nRows;
        int const nCols;
        std::vector<int> colsUpperbound;

    public:
        template<typename Vars>
        explicit LexChainBit(Vars const & rows) :
            Constraint(rows[0]->getSolver()),
            _rows(rows.begin(), rows.end()),
            nRows(static_cast<int>(_rows.size())),
            nCols(static_cast<int>(_rows.at(0)->size())),
            colsUpperbound(nCols, 0)
        {}
        void post() override;
        void propagate() override;
};
