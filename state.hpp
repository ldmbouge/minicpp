#ifndef __STATE_H
#define __STATE_H

#include <functional>
#include "handle.hpp"

class StateManager {
public:
   StateManager() {}
   virtual ~StateManager() {}
   typedef handle_ptr<StateManager> Ptr;
   virtual void saveState() = 0;
   virtual void restoreState() = 0;
   virtual void withNewState(const std::function<void(void)>& body) = 0;
};

#endif
