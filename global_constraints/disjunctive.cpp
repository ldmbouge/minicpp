// Port of the mandatory-task path of Objective-CP's CPTaskDisjunctive.
// Copyright (c) 2015 NICTA, Andreas Schutt and Pascal Van Hentenryck (MPL 2.0).
// See disjunctive.hpp for the overview.

#include <algorithm>
#include <climits>
#include "disjunctive.hpp"
#include "fail.hpp"

static constexpr int MININT = INT_MIN;
static constexpr int MAXINT = INT_MAX;

static inline int leftChild(int i)  { return (i << 1) + 1; }
static inline int rightChild(int i) { return (i << 1) + 2; }
static inline int parent(int i)     { return (i - 1) >> 1; }

static int depthOf(int x) {
   int d = 0;
   while (x >>= 1) d++;
   return d;
}

/*******************************************************************************
 Setup
 ******************************************************************************/

void Disjunctive::post()
{
   if (_m <= 1) return;   // nothing to do: at most one task occupies the resource
   _est.resize(_m); _lst.resize(_m); _ect.resize(_m); _lct.resize(_m); _dur.resize(_m);
   _newEst.resize(_m); _newLct.resize(_m);
   _mapEst.resize(_m); _mapLct.resize(_m);
   _byEst.resize(_m); _byEct.resize(_m); _byLst.resize(_m); _byLct.resize(_m);
   for (int k = 0; k < _m; k++) {
      _byEst[k] = _byEct[k] = _byLst[k] = _byLct[k] = k;
      _dur[k] = _p[_task[k]];
   }
   _tsize  = 2 * _m - 1;
   _tdepth = depthOf(_tsize);
   _theta.resize(_tsize);
   _lambda.resize(_tsize);
   for (int k = 0; k < _m; k++)
      _x[_task[k]]->propagateOnBoundChange(this);
   propagate();
}

void Disjunctive::readData()
{
   for (int k = 0; k < _m; k++) {
      auto const& s = _x[_task[k]];
      _est[k] = s->min();
      _lst[k] = s->max();
      _ect[k] = _est[k] + _dur[k];
      _lct[k] = _lst[k] + _dur[k];
      _newEst[k] = _est[k];
      _newLct[k] = _lct[k];
   }
}

// Stable insertion sort of a persistent permutation (cheap when nearly sorted).
template <typename Key>
static void isort(std::vector<int>& a, Key key)
{
   for (int i = 1; i < (int)a.size(); i++) {
      int const v  = a[i];
      int const kv = key(v);
      int j = i - 1;
      while (j >= 0 && key(a[j]) > kv) { a[j + 1] = a[j]; j--; }
      a[j + 1] = v;
   }
}

void Disjunctive::sortAll()
{
   isort(_byEst, [this](int k) { return _est[k]; });
   isort(_byEct, [this](int k) { return _ect[k]; });
   isort(_byLst, [this](int k) { return _lst[k]; });
   isort(_byLct, [this](int k) { return _lct[k]; });
}

// The tree has 2m-1 nodes. Leaves read left to right are: the whole deepest
// level, then the remaining leaves of the level above. Rank r in 'order' is
// mapped to the r-th leaf in that left-to-right order.
void Disjunctive::initIndexMap(std::vector<int> const& order, std::vector<int>& map)
{
   int tt   = 0;
   int leaf = (1 << _tdepth) - 1;
   for (; leaf < _tsize; tt++, leaf++)
      map[order[tt]] = leaf;
   leaf = parent(leaf);
   for (; tt < _m; tt++, leaf++)
      map[order[tt]] = leaf;
}

// Inverse of initIndexMap: leaf index -> rank in the order used to build it.
int Disjunctive::leafToRank(int leaf) const
{
   int const offset = (1 << _tdepth) - 1;
   return offset <= leaf ? leaf - offset : leaf + _m - offset;
}

void Disjunctive::applyBounds()
{
   for (int k = 0; k < _m; k++)
      if (_newEst[k] > _est[k] || _newLct[k] < _lct[k])
         _x[_task[k]]->updateBounds(_newEst[k], _newLct[k] - _dur[k]);
}

/*******************************************************************************
 Theta and Theta-Lambda trees
 ******************************************************************************/

void Disjunctive::initTheta(int time)
{
   for (auto& n : _theta) n = {0, time};
}

void Disjunctive::initLambda(int time)
{
   for (auto& n : _lambda) n = {0, time};
}

void Disjunctive::insertThetaEct(int idx, int length, int time)
{
   _theta[idx] = {length, time};
   while (idx > 0) {
      idx = parent(idx);
      auto const& l = _theta[leftChild(idx)];
      auto const& r = _theta[rightChild(idx)];
      _theta[idx].length = l.length + r.length;
      _theta[idx].time   = std::max(r.time, l.time + r.length);
   }
}

void Disjunctive::insertThetaLst(int idx, int length, int time)
{
   _theta[idx] = {length, time};
   while (idx > 0) {
      idx = parent(idx);
      auto const& l = _theta[leftChild(idx)];
      auto const& r = _theta[rightChild(idx)];
      _theta[idx].length = l.length + r.length;
      _theta[idx].time   = std::min(l.time, r.time - l.length);
   }
}

void Disjunctive::insertLambdaEct(int idx, int length, int time)
{
   _lambda[idx] = {length, time};
   while (idx > 0) {
      idx = parent(idx);
      int const l = leftChild(idx), r = rightChild(idx);
      _lambda[idx].gLength = std::max(_lambda[l].gLength + _theta[r].length,
                                      _theta[l].length + _lambda[r].gLength);
      _lambda[idx].gTime   = std::max(_lambda[r].gTime,
                                      std::max(_theta[l].time + _lambda[r].gLength,
                                               _lambda[l].gTime + _theta[r].length));
   }
}

void Disjunctive::insertLambdaLst(int idx, int length, int time)
{
   _lambda[idx] = {length, time};
   while (idx > 0) {
      idx = parent(idx);
      int const l = leftChild(idx), r = rightChild(idx);
      _lambda[idx].gLength = std::max(_lambda[l].gLength + _theta[r].length,
                                      _theta[l].length + _lambda[r].gLength);
      _lambda[idx].gTime   = std::min(_lambda[l].gTime,
                                      std::min(_theta[r].time - _lambda[l].gLength,
                                               _lambda[r].gTime - _theta[l].length));
   }
}

// (Theta, Lambda) = (all tasks, {}) keyed on ect.
void Disjunctive::initThetaLambdaEct()
{
   for (int k = 0; k < _m; k++)
      _theta[_mapEst[k]] = {_dur[k], _ect[k]};
   for (int p = _tsize - _m - 1; p >= 0; p--) {
      auto const& l = _theta[leftChild(p)];
      auto const& r = _theta[rightChild(p)];
      _theta[p] = {l.length + r.length, std::max(r.time, l.time + r.length)};
   }
   for (int i = 0; i < _tsize; i++)
      _lambda[i] = {_theta[i].length, _theta[i].time};
}

// (Theta, Lambda) = (all tasks, {}) keyed on lst.
void Disjunctive::initThetaLambdaLst()
{
   for (int k = 0; k < _m; k++)
      _theta[_mapLct[k]] = {_dur[k], _lst[k]};
   for (int p = _tsize - _m - 1; p >= 0; p--) {
      auto const& l = _theta[leftChild(p)];
      auto const& r = _theta[rightChild(p)];
      _theta[p] = {l.length + r.length, std::min(l.time, r.time - l.length)};
   }
   for (int i = 0; i < _tsize; i++)
      _lambda[i] = {_theta[i].length, _theta[i].time};
}

// Leaf (grey task) responsible for ECT(Theta, Lambda).
int Disjunctive::responsibleLambdaEct() const
{
   int p = 0;
   bool gLength = false;
   for (;;) {
      int const l = leftChild(p), r = rightChild(p);
      if (l >= _tsize) break;
      if (gLength)
         p = (_lambda[p].gLength == _lambda[l].gLength + _theta[r].length) ? l : r;
      else if (_lambda[p].gTime == _lambda[r].gTime)
         p = r;
      else if (_lambda[p].gTime == _theta[l].time + _lambda[r].gLength) {
         p = r;
         gLength = true;
      } else
         p = l;
   }
   return p;
}

// Leaf (grey task) responsible for LST(Theta, Lambda).
int Disjunctive::responsibleLambdaLst() const
{
   int p = 0;
   bool gLength = false;
   for (;;) {
      int const l = leftChild(p), r = rightChild(p);
      if (l >= _tsize) break;
      if (gLength)
         p = (_lambda[p].gLength == _lambda[l].gLength + _theta[r].length) ? l : r;
      else if (_lambda[p].gTime == _lambda[l].gTime)
         p = l;
      else if (_lambda[p].gTime == _theta[r].time - _lambda[l].gLength) {
         p = l;
         gLength = true;
      } else
         p = r;
   }
   return p;
}

/*******************************************************************************
 Filtering algorithms
 ******************************************************************************/

// Overload check: for every prefix of tasks by lct, ECT(Theta) <= lct.
void Disjunctive::overloadCheck()
{
   initTheta(MININT);
   for (int k : _byLct) {
      insertThetaEct(_mapEst[k], _dur[k], _ect[k]);
      if (_theta[0].time > _lct[k])
         failNow();
   }
}

// Detectable precedences, raising est.
bool Disjunctive::dprecEst()
{
   bool update = false;
   initTheta(MININT);
   int jj = 0;
   for (int i : _byEct) {
      while (jj < _m && _ect[i] > _lst[_byLst[jj]]) {
         int const j = _byLst[jj++];
         insertThetaEct(_mapEst[j], _dur[j], _ect[j]);
      }
      int ectT;
      if (_ect[i] > _lst[i]) {        // i is in the tree: exclude it
         insertThetaEct(_mapEst[i], 0, MININT);
         ectT = _theta[0].time;
         insertThetaEct(_mapEst[i], _dur[i], _ect[i]);
      } else
         ectT = _theta[0].time;
      if (ectT > _newEst[i]) {
         _newEst[i] = ectT;
         update = true;
      }
   }
   return update;
}

// Detectable precedences, lowering lct.
bool Disjunctive::dprecLct()
{
   bool update = false;
   initTheta(MAXINT);
   int jj = _m - 1;
   for (int ii = _m - 1; ii >= 0; ii--) {
      int const i = _byLst[ii];
      while (jj >= 0 && _lst[i] < _ect[_byEct[jj]]) {
         int const j = _byEct[jj--];
         insertThetaLst(_mapLct[j], _dur[j], _lst[j]);
      }
      int lstT;
      if (_lst[i] < _ect[i]) {        // i is in the tree: exclude it
         insertThetaLst(_mapLct[i], 0, MAXINT);
         lstT = _theta[0].time;
         insertThetaLst(_mapLct[i], _dur[i], _lst[i]);
      } else
         lstT = _theta[0].time;
      if (lstT < _newLct[i]) {
         _newLct[i] = lstT;
         update = true;
      }
   }
   return update;
}

// Not-first, raising est.
bool Disjunctive::nfnlEst()
{
   bool update = false;
   initTheta(MAXINT);
   int jj = _m - 1;
   int last = -1;   // last task inserted in the tree
   for (int ii = _m - 1; ii >= 0; ii--) {
      int const i = _byEst[ii];
      while (jj >= 0 && _est[i] < _ect[_byEct[jj]]) {
         int const j = _byEct[jj--];
         if (last >= 0 && _theta[0].time < _ect[j] && _ect[last] > _newEst[j]) {
            _newEst[j] = _ect[last];
            update = true;
         }
         insertThetaLst(_mapLct[j], _dur[j], _lst[j]);
         last = j;
      }
      // i is in the tree since est[i] < ect[i]
      insertThetaLst(_mapLct[i], 0, MAXINT);
      int const lstT = _theta[0].time;
      insertThetaLst(_mapLct[i], _dur[i], _lst[i]);
      if (lstT < _ect[i] && _newEst[i] < _ect[last]) {
         _newEst[i] = _ect[last];
         update = true;
      }
   }
   return update;
}

// Not-last, lowering lct.
bool Disjunctive::nfnlLct()
{
   bool update = false;
   initTheta(MININT);
   int jj = 0;
   int last = -1;   // last task inserted in the tree
   for (int i : _byLct) {
      while (jj < _m && _lct[i] > _lst[_byLst[jj]]) {
         int const j = _byLst[jj++];
         if (last >= 0 && _theta[0].time > _lst[j] && _lst[last] < _newLct[j]) {
            _newLct[j] = _lst[last];
            update = true;
         }
         insertThetaEct(_mapEst[j], _dur[j], _ect[j]);
         last = j;
      }
      // i is in the tree since lst[i] < lct[i]
      insertThetaEct(_mapEst[i], 0, MININT);
      int const ectT = _theta[0].time;
      insertThetaEct(_mapEst[i], _dur[i], _ect[i]);
      if (ectT > _lst[i] && _newLct[i] > _lst[last]) {
         _newLct[i] = _lst[last];
         update = true;
      }
   }
   return update;
}

// Edge finding, raising est.
bool Disjunctive::efEst()
{
   bool update = false;
   initThetaLambdaEct();
   int jj = _m - 1;
   int j  = _byLct[jj];
   while (jj > 0) {
      // Move j from Theta (white) to Lambda (grey)
      insertThetaEct(_mapEst[j], 0, MININT);
      insertLambdaEct(_mapEst[j], _dur[j], _ect[j]);
      j = _byLct[--jj];
      while (_lambda[0].gTime > _lct[j]) {
         int const leaf = responsibleLambdaEct();
         int const i    = _byEst[leafToRank(leaf)];
         if (_theta[0].time > _newEst[i]) {
            _newEst[i] = _theta[0].time;
            update = true;
         }
         insertLambdaEct(_mapEst[i], 0, MININT);
      }
   }
   return update;
}

// Edge finding, lowering lct.
bool Disjunctive::efLct()
{
   bool update = false;
   initThetaLambdaLst();
   int jj = 0;
   int j  = _byEst[jj];
   while (jj < _m - 1) {
      // Move j from Theta (white) to Lambda (grey)
      insertThetaLst(_mapLct[j], 0, MAXINT);
      insertLambdaLst(_mapLct[j], _dur[j], _lst[j]);
      j = _byEst[++jj];
      while (_lambda[0].gTime < _est[j]) {
         int const leaf = responsibleLambdaLst();
         int const i    = _byLct[leafToRank(leaf)];
         if (_theta[0].time < _newLct[i]) {
            _newLct[i] = _theta[0].time;
            update = true;
         }
         insertLambdaLst(_mapLct[i], 0, MAXINT);
      }
   }
   return update;
}

/*******************************************************************************
 Main propagation loop
 ******************************************************************************/

// One pass, as in the original: the first rule family that tightens a bound
// applies its updates and returns. Changing the start variables reschedules
// this constraint, so the solver drives it to a fixpoint.
void Disjunctive::propagate()
{
   if (_m <= 1) return;
   readData();
   sortAll();
   initIndexMap(_byEst, _mapEst);
   overloadCheck();
   initIndexMap(_byLct, _mapLct);

   bool update = false;
   if (_opt.dprec) {
      update |= dprecEst();
      update |= dprecLct();
      if (update) { applyBounds(); return; }
   }
   if (_opt.nfnl) {
      update |= nfnlEst();
      update |= nfnlLct();
      if (update) { applyBounds(); return; }
   }
   if (_opt.ef) {
      update |= efEst();
      update |= efLct();
      if (update) { applyBounds(); return; }
   }
}
