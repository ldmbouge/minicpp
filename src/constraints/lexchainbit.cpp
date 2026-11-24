#include <algorithm>
#include "constraints/lexchainbit.hpp"

LexChainBit::LexChainBit(std::vector<var<int>::Ptr> const& rows)
   : Constraint(rows.at(0)->getSolver()),
     _rows(rows),
     _nRows(static_cast<int>(rows.size())),
     _nCols(static_cast<int>(rows.at(0)->size())),
     _firstRow(_nCols),
     _rowLimit(_nRows)
{}

void LexChainBit::post()
{
   for (auto const& row : _rows)
      row->propagateOnBoundChange(this);
   propagate();
}

void LexChainBit::propagate()
{
   // Pruning a row can lower the first row of a column: repeat until nothing changes
   bool changed = true;
   while (changed) {
      changed = false;

      // First row that can take each column
      std::fill(_firstRow.begin(), _firstRow.end(), _nRows);
      for (int r = _nRows - 1; r >= 0; r -= 1) {
         auto const& row = _rows[r];
         int const maxCol = std::min(row->max(), _nCols - 1);
         for (int c = std::max(row->min(), 0); c <= maxCol; c += 1)
            if (row->containsBase(c))
               _firstRow[c] = r;
      }

      // Column c starts at row first(c) >= _firstRow[c], and the columns after it start later:
      // rows 0.._firstRow[c] cannot take a column after c. If no row can take c, it is empty,
      // and so are the columns after it.
      std::fill(_rowLimit.begin(), _rowLimit.end(), _nCols - 1);
      for (int c = 0; c < _nCols; c += 1) {
         int const r = _firstRow[c];
         int const limit = r < _nRows ? c : c - 1;
         int const lastRow = std::min(r, _nRows - 1);
         _rowLimit[lastRow] = std::min(_rowLimit[lastRow], limit);
      }
      for (int r = _nRows - 2; r >= 0; r -= 1)
         _rowLimit[r] = std::min(_rowLimit[r], _rowLimit[r + 1]);

      for (int r = 0; r < _nRows; r += 1) {
         if (_rows[r]->max() > _rowLimit[r]) {
            _rows[r]->removeAbove(_rowLimit[r]);
            changed = true;
         }
      }
   }
}
