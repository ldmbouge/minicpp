#ifndef __ACSTR_H
#define __ACSTR_H

#include <memory>
#include <iostream>


class Constraint {
protected:
    virtual void print(std::ostream& os) const {}
public:
    typedef std::shared_ptr<Constraint> Ptr;
    Constraint() {}
    virtual ~Constraint() {}
    virtual void post() = 0;
};

#endif
