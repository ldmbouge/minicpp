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

struct timeval timeval_subtract(struct timeval* x,struct timeval* y);

class RuntimeMonitor {
public:
   static long cputime();
   static long microseconds();
   static long wctime();
   static timeval now();
   static timeval  elapsedSince(timeval then);
};
#endif /* RuntimeMonitor_hpp */

