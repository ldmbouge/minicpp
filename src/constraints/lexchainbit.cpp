#include <algorithm>
#include "constraints/lexchainbit.hpp"

LexChainBit::LexChainBit(std::vector<var<int>::Ptr> const& rows)
   : Constraint(rows.at(0)->getSolver()),
     _rows(rows),
     _nRows(static_cast<int>(rows.size())),
     _nCols(static_cast<int>(rows.at(0)->size())),
     _colsUpperbound(_nCols, 0)
{}

void LexChainBit::post()
{
   for (auto const& row : _rows)
      row->propagateOnBoundChange(this);
}

void LexChainBit::propagate()
{
   std::fill(_colsUpperbound.begin(), _colsUpperbound.end(), 0);

   // First row that can take each column
   for (int r = _nRows - 1; r >= 0; r -= 1) {
      auto const& row = _rows[r];
      int const minCol = row->min();
      int const maxCol = row->max();
      for (int c = minCol; c <= maxCol; c += 1)
         if (row->containsBase(c))
            _colsUpperbound[c] = r;
   }

   // Rows before that one cannot take a later column
   for (int c = _nCols - 1; c > 0; c -= 1) {
      int const r = _colsUpperbound[c];
      if (r >= 1)
         _rows[r - 1]->removeAbove(c);
   }
}
