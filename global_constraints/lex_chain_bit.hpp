#pragma once

#include <libminicpp/constraint.hpp>

class LexChainBit : public Constraint
{
    protected:
        std::vector<var<int>::Ptr> _rows;
        int const nRows;
        int const nCols;
        std::vector<int> colsUpperbound;

    public:
        LexChainBit(std::vector<var<int>::Ptr> const & rows);
        void post() override;
        void propagate() override;

};
