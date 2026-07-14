#include <algorithm>
#include "cumulative.hpp"

void Cumulative::post() {
  for (auto const & v : _x)
    v->propagateOnBoundChange(this);
}

void Cumulative::calcRi() {
    _ri.clear();
    auto collectIfValid = [this](TimeInterval ti) { if (ti.start < ti.end) _ri.push_back(ti); };
    for (auto i = 0; i < _n; i += 1) {
        int const pi = _p[i];
        int const esti = _si[i].min;
        int const lsti = _si[i].max;
        int const lcti = lsti + pi;
        for (int j = 0; j < _n; j += 1) {
            if (_si[i].changed or _si[j].changed) {
                int const pj = _p[j];
                int const estj = _si[j].min;
                int const lstj = _si[j].max;
                int const ectj = estj + pj;
                int const lctj = lstj + pj;

                // Case 1
                collectIfValid({esti, ectj});
                collectIfValid({esti, lctj});
                collectIfValid({lsti, ectj});
                collectIfValid({lsti, lctj});

                // Case 2
                collectIfValid({esti, estj + lctj - esti});
                collectIfValid({esti, estj + lctj - lsti});
                collectIfValid({lsti, estj + lctj - esti});
                collectIfValid({lsti, estj + lctj - lsti});

                // Case 3
                collectIfValid({esti + lcti - ectj, ectj});
                collectIfValid({esti + lcti - ectj, lctj});
                collectIfValid({esti + lcti - lctj, ectj});
                collectIfValid({esti + lcti - lctj, lctj});
            }
        }
    }
}

void Cumulative::updateSi() {
    using namespace std;
    int const riSize = static_cast<int>(_ri.size());
    for (auto j = 0; j < riSize; j += 1) {
        auto const [t1,t2] = _ri[j];
        int w = 0;
        for (int i = 0; i < _n; i += 1) {
            int const hi = _h[i];
            int const pi = _p[i];
            int const esti = _x[i]->min(); // We could use _si, but the #propagations would not match GPU
            int const lsti = _x[i]->max(); // We could use _si, but the #propagations would not match GPU
            int const ls = max(0, min(esti + pi, t2) - max(esti, t1));
            int const rs = max(0, min(lsti + pi, t2) - max(lsti, t1));
            w += hi * min(ls, rs);
        }

        if (w <= _c * (t2 - t1))
            for (int i = 0; i < _n; i += 1){
                int const hi = _h[i];
                int const pi = _p[i];
                int const esti = _x[i]->min(); // We could use _si, but the #propagations would not match GPU
                int const lsti = _x[i]->max(); // We could use _si, but the #propagations would not match GPU
                int const ls = max(0, min(esti + pi, t2) - max(esti, t1));
                int const rs = max(0, min(lsti + pi, t2) - max(lsti, t1));
                int const avail = _c * (t2 - t1) - w + hi * min(ls, rs);
                if (avail < hi * ls)
                    _si[i].min = max(_si[i].min, t2 - (avail / hi));
                if (avail < hi * rs)
                    _si[i].max = min(_si[i].max, t1 + (avail / hi) - pi);
            }
        else
            failNow();
    }
}

void Cumulative::propagate() {
    for (auto i = 0; i < _n; i += 1)
        _si[i] = {_x[i]->changed(), _x[i]->min(), _x[i]->max()};
    calcRi();
    updateSi();
    for (auto i = 0; i < _n; i += 1)
        _x[i]->updateBounds(_si[i].min,_si[i].max);
}
