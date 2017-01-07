#ifndef __SOLVER_H
#define __SOLVER_H

#include <list>
#include <deque>
#include <functional>

#include "fail.hpp"
#include "engine.hpp"
#include "avar.hpp"
#include "acstr.hpp"

typedef std::reference_wrapper<std::function<void(void)>> Closure;

class CPSolver {
    Engine::Ptr                  _ctx;
    std::list<AVar::Ptr>       _iVars;
    std::list<Constraint::Ptr> _iCstr;
    std::deque<Closure>        _queue;
    bool                      _closed;
    int                        _varId;
    int                          _nbc; // # choices
    int                          _nbf; // # fails
    int                          _nbs; // # solutions
public:
    template<typename T> friend class var;
    typedef std::shared_ptr<CPSolver> Ptr;
    CPSolver();
    Engine::Ptr context() { return _ctx;}
    void registerVar(AVar::Ptr avar);
    void schedule(std::function<void(void)>& cb) { _queue.emplace_back(std::ref(cb));}
    Status propagate();
    Status add(Constraint::Ptr c);
    void close();
    void incrNbChoices() { _nbc += 1;}
    void incrNbSol()     { _nbs += 1;}
    friend std::ostream& operator<<(std::ostream& os,const CPSolver& s) {
        return os << "CPSolver(" << &s << ")" << std::endl
                  << "\t#choices   = " << s._nbc << std::endl
                  << "\t#fail      = " << s._nbf << std::endl
                  << "\t#solutions = " << s._nbs << std::endl;
    }
};

namespace Factory {
    inline CPSolver::Ptr makeSolver() { return std::make_shared<CPSolver>();}
};

#endif
