//
//  RuntimeMonitor.hpp
//  minicpp
//
//  Created by zitoun on 12/17/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

#ifndef RuntimeMonitor_hpp
#define RuntimeMonitor_hpp

#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>

struct timeval timeval_subtract(struct timeval* x,struct timeval* y) {
   /* Perform the carry for the later subtraction by updating y. */
   if (x->tv_usec < y->tv_usec) {
      int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
      y->tv_usec -= 1000000 * nsec;
      y->tv_sec += nsec;
   }
   if (x->tv_usec - y->tv_usec > 1000000) {
      int nsec = (x->tv_usec - y->tv_usec) / 1000000;
      y->tv_usec += 1000000 * nsec;
      y->tv_sec -= nsec;
   }
   
   /* Compute the time remaining to wait.
    tv_usec is certainly positive. */
   struct timeval result;
   result.tv_sec = x->tv_sec - y->tv_sec;
   result.tv_usec = x->tv_usec - y->tv_usec;
   return result;
}

class RuntimeMonitor {
public:
   static long cputime();
   static long microseconds();
   static long wctime();
   static timeval now();
   static timeval  elapsedSince(timeval then);
};



#endif /* RuntimeMonitor_hpp */
