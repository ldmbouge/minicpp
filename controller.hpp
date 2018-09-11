#ifndef __CONTROLLER_H
#define __CONTROLLER_H

#include "cont.hpp"
#include "trail.hpp"
#include <stack>

class Controller {
protected:
   Trailer::Ptr _ctx;
public:
   Controller(Trailer::Ptr ctx) : _ctx(ctx) {}
   virtual ~Controller() {}
   virtual void addChoice(Cont::Cont* k) = 0;
   virtual void fail() = 0;
   virtual void trust() = 0;
   virtual void start(Cont::Cont* k) = 0;
   virtual void exit() = 0;
   virtual void clear() = 0;
};

class DFSController :public Controller {
   std::stack<Cont::Cont*> _cf;
   Cont::Cont* _exitK;
public:
   DFSController(Trailer::Ptr ctx);
  ~DFSController();
   void start(Cont::Cont* k);
   void addChoice(Cont::Cont* k);
   void trust();
   void fail();
   void exit();
  void clear();
};

#endif
