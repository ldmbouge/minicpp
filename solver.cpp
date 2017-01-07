#include "solver.hpp"
#include <assert.h>
#include <iostream>
#include <iomanip>

CPSolver::CPSolver()
    : _ctx(new Engine)
{
    _closed = false;
    _varId  = 0;
    _nbc = _nbf = _nbs = 0;
}

Status CPSolver::add(Constraint::Ptr c)
{
    if (!_closed) {
        _iCstr.push_back(c);
        return Suspend;
    } else {
        try {
            c->post();
        } catch(Status x) {
            std::cout << "post raised: " << x << std::endl;
            return x;
        }
        return Suspend;
    }
}

void CPSolver::registerVar(AVar::Ptr avar) {
    avar->setId(_varId++);
    _iVars.push_back(avar);
}

void CPSolver::close()
{
    for(auto c : _iCstr)
        c->post();
    _closed = true;
}

Status CPSolver::propagate()
{
    try {
        while (!_queue.empty()) {
            auto cb = _queue.front();
            _queue.pop_front();
            cb();
        }
        assert(_queue.size() == 0);
        return Suspend;
    } catch(Status x) {
        _queue.clear();
        assert(_queue.size() == 0);
        _nbf += 1;
        return Failure;
    }
}

