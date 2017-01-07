#ifndef __AVAR_H
#define __AVAR_H

#include <memory>

class AVar {
protected:
    virtual void setId(int id) = 0;
    friend class CPSolver;
public:
    typedef std::shared_ptr<AVar> Ptr;
    AVar() {}
    virtual ~AVar() {}
};

#endif
