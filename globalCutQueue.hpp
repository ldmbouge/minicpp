#ifndef __GLOBAL_CUT_QUEUE_H
#define __GLOBAL_CUT_QUEUE_H

#include "handle.hpp"
#include "constraint.hpp"
#include <vector>

class GlobalCut {
    Constraint::Ptr _con;
    int _depth;
    bool _posted;
public:
    GlobalCut(Constraint::Ptr cPtr, int d) : _con(cPtr), _depth(d), _posted(false) {}
    GlobalCut(const GlobalCut& other) : _con(other._con), _depth(other._depth), _posted(other._posted) {}
    // GlobalCut(const GlobalCut&& other) : _con(other._con), _depth(other._depth), _posted(other._posted) {}
    Constraint::Ptr getCon() const { return _con;}
    int getDepth() const { return _depth;}
    bool posted() const { return _posted;}
    void setCon(Constraint::Ptr c) { _con = c;}
    void setPosted(bool b) { _posted = b;}
};


class GlobalCutQueue {
    std::vector<GlobalCut> _cuts;
public:
    void addToQueue(Constraint::Ptr,int);
    std::vector<GlobalCut>& getCuts() { return _cuts;}
};


#endif