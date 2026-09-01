#pragma once

#include "intvar.hpp"
#include <varitf.hpp>

// References:
// - Constraint-Based Scheduling (ISBN: 978-1-4615-1479-4)
// - A New Characterization of Relevant Intervals for Energetic Reasoning (DOI: 10.1007/978-3-319-10428-7_22)

class Cumulative : public Constraint {
public:
    static constexpr int IntervalsPerActivityPair = 12;

    struct TimeInterval {
        int start;
        int end;
    };

    struct Domain {
        bool changed;
        int min;
        int max;
    };

private :
    int const _n;
    std::vector<var<int>::Ptr> _x;
    std::vector<int> const _p;
    std::vector<int> const _h;
    int const _c;
    std::vector<Domain> _si;
    std::vector<TimeInterval> _ri;
    void calcRi();
    void updateSi();

public:
    template<typename Vars>
    Cumulative(Vars& x, std::vector<int> const& p, std::vector<int> const& h, int c)
        : Constraint(x[0]->getSolver()), _n(x.size()), _x(x.begin(), x.end()), _p(p), _h(h), _c(c), _si(_n)
    {
        assert(p.size() == x.size());
        assert(h.size() == x.size());
        _ri.reserve(IntervalsPerActivityPair * _n * _n);
        setPriority(CLOW);
    }
    void post() override;
    void propagate() override;
};