//
//  RuntimeMonitor.cpp
//  minicpp
//
//  Created by zitoun on 12/17/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

#include "RuntimeMonitor.hpp"

#include <sys/time.h>
#include <sys/resource.h>

std::chrono::time_point<std::chrono::high_resolution_clock> RuntimeMonitor::cputime()
{
   return std::chrono::high_resolution_clock::now();
}
std::chrono::time_point<std::chrono::system_clock> RuntimeMonitor::wctime()
{
   return std::chrono::system_clock::now();
}
std::chrono::time_point<std::chrono::high_resolution_clock>  RuntimeMonitor::now()
{
   return std::chrono::high_resolution_clock::now();
}
double RuntimeMonitor::elapsedSince(std::chrono::time_point<std::chrono::high_resolution_clock> then)
{
   auto now  = std::chrono::high_resolution_clock::now();   
   auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - then);
   return diff.count();
}
long RuntimeMonitor::milli(HRClock s,HRClock e)
{
   auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(e - s);
   return diff.count();
}
