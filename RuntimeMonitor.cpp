//
//  RuntimeMonitor.cpp
//  minicpp
//
//  Created by zitoun on 12/17/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

#include "RuntimeMonitor.hpp"

namespace RuntimeMonitor {
   HRClock cputime()
   {
      return std::chrono::high_resolution_clock::now();
   }
   std::chrono::time_point<std::chrono::system_clock> wctime()
   {
      return std::chrono::system_clock::now();
   }
   HRClock now()
   {
      return std::chrono::high_resolution_clock::now();
   }
   double elapsedSince(HRClock then)
   {
      auto now  = std::chrono::high_resolution_clock::now();   
      auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - then);
      return diff.count();
   }
   long milli(HRClock s,HRClock e)
   {
      auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(e - s);
      return diff.count();
   }
}
