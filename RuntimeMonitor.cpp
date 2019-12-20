//
//  RuntimeMonitor.cpp
//  minicpp
//
//  Created by zitoun on 12/17/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

#include "RuntimeMonitor.hpp"


long RuntimeMonitor::cputime()
{
   struct rusage r;
   getrusage(RUSAGE_SELF,&r);
   struct timeval t;
   t = r.ru_utime;
   return 1000 * t.tv_sec + t.tv_usec / 1000;
}
long RuntimeMonitor::microseconds()
{
   struct rusage r;
   getrusage(RUSAGE_SELF,&r);
   struct timeval t;
   t = r.ru_utime;
   return ((long)t.tv_usec) + (long)1000L * (long)t.tv_sec;
}
long RuntimeMonitor::wctime()
{
   struct timeval now;
   struct timezone tz;
   int st = gettimeofday(&now,&tz);
   if (st==0) {
      now.tv_sec -= (0xfff << 20);
      return 1000 * now.tv_sec + now.tv_usec/1000;
   }
   else return 0;
}
timeval RuntimeMonitor::now()
{
   struct rusage r;
   getrusage(RUSAGE_SELF,&r);
   return r.ru_utime;
}
timeval  RuntimeMonitor::elapsedSince(timeval then)
{
   struct rusage r;
   getrusage(RUSAGE_SELF,&r);
   return timeval_subtract(&r.ru_utime,&then);
}
