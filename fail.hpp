#ifndef __FAIL_H
#define __FAIL_H

enum Status { Failure,Success,Suspend };

static inline void failNow()
{
    throw Failure;
}

#endif
