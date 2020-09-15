#ifndef __GLOBAL_CUT_QUEUE_H
#define __GLOBAL_CUT_QUEUE_H

#include "handle.hpp"
#include "constraint.hpp"
#include <vector>

class GlobalCut {
    Constraint::Ptr _con;
    int _depth;
public:
    GlobalCut(Constraint::Ptr cPtr, int d) : _con(cPtr), _depth(d) {}
    GlobalCut(GlobalCut&& other) : _con(other._con), _depth(other._depth) {}
    // ~GlobalCut() { _con.dealloc();}
    Constraint::Ptr getCon() { return _con;}
    int getDepth() { return _depth;}
};


class GlobalCutQueue {
    std::vector<GlobalCut> _cuts;
public:
    void addToQueue(Constraint::Ptr,int);
    std::vector<GlobalCut>& getCuts() { return _cuts;}
};


#endif