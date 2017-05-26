#ifndef __AVAR_H
#define __AVAR_H

#include <memory>
#include "handle.hpp"

class AVar {
protected:
    virtual void setId(int id) = 0;
    friend class CPSolver;
public:
    typedef handle_ptr<AVar> Ptr;
    AVar() {}
    virtual ~AVar() {}
};

#endif
