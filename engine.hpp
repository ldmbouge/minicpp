#ifndef __ENGINE_H
#define __ENGINE_H

#include <stack>

class Entry {
public:
    virtual void restore() = 0;
    typedef std::unique_ptr<Entry> Ptr;
};

class Engine {
    std::stack<Entry::Ptr>  _trail;
    std::stack<int>          _tops;
    mutable int             _magic;
public:
    Engine();
    void trail(Entry::Ptr&& e) { _trail.emplace(std::move(e));}
    typedef std::shared_ptr<Engine> Ptr;
    int magic() const { return _magic;}
    void push();
    void pop();
};
 
#endif
