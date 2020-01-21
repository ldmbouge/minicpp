//
//  RuntimeMonitor.hpp
//  minicpp
//
//  Created by zitoun on 12/17/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

#ifndef RuntimeMonitor_hpp
#define RuntimeMonitor_hpp

#include <chrono>

class RuntimeMonitor {
public:
   typedef  std::chrono::time_point<std::chrono::high_resolution_clock> HRClock;
   static HRClock cputime();
   static std::chrono::time_point<std::chrono::system_clock> wctime();
   static HRClock now();
   static double elapsedSince(HRClock then);
   static long milli(HRClock s,HRClock e);
};
#endif /* RuntimeMonitor_hpp */

