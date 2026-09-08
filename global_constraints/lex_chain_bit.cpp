#include "global_constraints/lex_chain_bit.hpp"

#include <algorithm>

void LexChainBit::post()
{
    for (auto const & row : _rows)
    {
        row->propagateOnBoundChange(this);
    }
};

void LexChainBit::propagate()
{
    std::fill(colsUpperbound.begin(), colsUpperbound.end(), 0);

    for (int rIdx = nRows - 1; rIdx >= 0; rIdx -= 1)
    {
        auto const & row = _rows.at(rIdx);
        int const minCol = row->min();
        int const maxCol = row->max();
        for (int cIdx = minCol; cIdx <= maxCol; cIdx += 1)
        {
            if (row->containsBase(cIdx))
            {
                colsUpperbound.at(cIdx) = rIdx;
            }
        }
    }

    for (int cIdx = nCols - 1; cIdx > 0; cIdx -= 1)
    {
        int rIdx = colsUpperbound.at(cIdx);
        if (rIdx >= 1)
        {
            _rows.at(rIdx - 1)->removeAbove(cIdx);
        }
    }
}
