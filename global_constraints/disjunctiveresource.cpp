#include <algorithm>
#include <climits>
#include "disjunctiveresource.hpp"
#include "scheduling.hpp"

void DisjunctiveResource::add(var<int>::Ptr start, int duration)
{
   assert(!_posted && "tasks must be added before the resource is posted");
   _start.push_back(start);
   _dur.push_back(duration);
}

void DisjunctiveResource::post()
{
   assert(!_posted);
   _posted = true;
   int const n = size();
   if (n == 0) return;
   for (int k = 0; k <= n; k++)
      _succ.push_back(Factory::makeIntVar(_cp, 1, n + 1));
   _cp->post(Factory::disjunctive(_start, _dur), false);
   _cp->post(Factory::taskSequence(_start, _dur, _succ), false);
}

int DisjunctiveResource::frontier() const
{
   int const sink = size() + 1;
   int k = 0;
   for (int steps = 0; steps <= size(); steps++) {
      if (!_succ[k]->isBound()) return k;
      k = _succ[k]->min();
      if (k == sink) return -1;
   }
   return -1;   // a cycle; the circuit constraint rules this out
}

bool DisjunctiveResource::isSequenced() const
{
   return size() == 0 || frontier() < 0;
}

int DisjunctiveResource::globalSlack() const
{
   int estMin = INT_MAX, lctMax = INT_MIN, len = 0;
   for (int k = 0; k < size(); k++) {
      if (_dur[k] <= 0) continue;
      if (lct(k) - est(k) - _dur[k] > 0) {   // unfixed
         estMin = std::min(estMin, est(k));
         lctMax = std::max(lctMax, lct(k));
      }
      len += _dur[k];
   }
   return lctMax - estMin - len;
}

int DisjunctiveResource::localSlack() const
{
   int const n = size();
   std::vector<int> e(n), l(n), byEst(n), byLct(n);
   for (int k = 0; k < n; k++) {
      e[k] = est(k);
      l[k] = lct(k);
      byEst[k] = byLct[k] = k;
   }
   std::stable_sort(byEst.begin(), byEst.end(), [&](int a, int b) { return e[a] < e[b]; });
   std::stable_sort(byLct.begin(), byLct.end(), [&](int a, int b) { return l[a] < l[b]; });
   auto relevant = [&](int k) { return _dur[k] > 0; };
   auto unfixed  = [&](int k) { return l[k] - e[k] - _dur[k] > 0; };

   int slack = INT_MAX;
   int jjPrev = 0, jjLast = n - 1;
   for (int jj = n - 1; jj >= 0; jj--)
      if (relevant(byLct[jj]) && unfixed(byLct[jj])) { jjLast = jj; break; }
   for (int ii = 0; ii < n; ii++) {
      int const i = byEst[ii];
      if (!(relevant(i) && unfixed(i))) continue;
      int const estMin = e[i];
      bool first = true;
      int len = 0;
      for (int jj = jjPrev; jj <= jjLast; jj++) {
         int const j = byLct[jj];
         if (!(relevant(j) && estMin < l[j])) continue;
         if (first) { jjPrev = jj; first = false; }
         if (estMin <= e[j]) len += _dur[j];
         if (unfixed(j)) slack = std::min(slack, l[j] - len);
      }
   }
   return slack;
}
