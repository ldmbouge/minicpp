#ifndef __ENGINE_H
#define __ENGINE_H

#include "handle.hpp"
#include "trail.hpp"
#include "store.hpp"

class Engine {
   Context::Ptr   _ctx;
   Storage::Ptr _store;
public:
   Engine();
   ~Engine();
   typedef handle_ptr<Engine> Ptr;
   auto& getContext()        { return _ctx;}
   auto& getStore()          { return _store;}
   int magic() const         { return _ctx->magic();}
   void incMagic()           { _ctx->incMagic();}
   long push()               { return _ctx->push();}
   void pop()                { _ctx->pop();}
   void popToNode(long node) { _ctx->popToNode(node);}
   void clear()              { _ctx->clear();}
};

#endif
