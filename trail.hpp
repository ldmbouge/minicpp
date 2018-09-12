#ifndef __TRAIL_H
#define __TRAIL_H

#include <memory>
#include <stack>
#include <tuple>
#include "state.hpp"

class Entry {
public:
   virtual void restore() = 0;
};

class Trailer :public StateManager {
   std::stack<Entry*>      _trail;
   std::stack<std::tuple<int,std::size_t,long>>  _tops;
   mutable int             _magic;
   long  _lastNode;
   char* _block;
   std::size_t  _bsz;
   std::size_t  _btop;
public:
   Trailer();
   ~Trailer();
   void trail(Entry* e) { _trail.push(e);}
   typedef handle_ptr<Trailer> Ptr;
   int magic() const { return _magic;}
   void incMagic() { _magic++;}
   long push();
   void pop();
   void popToNode(long node);
   void clear();

   void saveState() override;
   void restoreState() override;
   void withNewState(const std::function<void(void)>& body) override;

   friend void* operator new(std::size_t sz,Trailer::Ptr& e);
};

inline void* operator new(std::size_t sz,Trailer::Ptr& e) {
   char* ptr = e->_block + e->_btop;
   e->_btop += sz;
   return ptr;
}


#endif
