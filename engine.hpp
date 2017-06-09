#ifndef __ENGINE_H
#define __ENGINE_H

#include <memory>
#include <stack>
#include <stdlib.h>
#include <iostream>
#include <tuple>

class Entry {
public:
   virtual void restore() = 0;
};

class Engine {
   std::stack<Entry*>      _trail;
   std::stack<std::tuple<int,std::size_t,long>>  _tops;
   mutable int             _magic;
   long  _lastNode;
   char* _block;
   std::size_t  _bsz;
   std::size_t  _btop;
public:
   Engine();
   ~Engine();
   void trail(Entry* e) { _trail.push(e);}
   typedef std::shared_ptr<Engine> Ptr;
   int magic() const { return _magic;}
   void incMagic() { _magic++;}
   long push();
   void pop();
   void popToNode(long node);
   void clear();
   friend void* operator new(std::size_t sz,Engine::Ptr& e);
};

inline void* operator new(std::size_t sz,Engine::Ptr& e) {
   char* ptr = e->_block + e->_btop;
   e->_btop += sz;
   return ptr;
}

#endif
