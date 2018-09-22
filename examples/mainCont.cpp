/*
 * mini-cp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License  v3
 * as published by the Free Software Foundation.
 *
 * mini-cp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY.
 * See the GNU Lesser General Public License  for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with mini-cp. If not, see http://www.gnu.org/licenses/lgpl-3.0.en.html
 *
 * Copyright (c)  2018. by Laurent Michel, Pierre Schaus, Pascal Van Hentenryck
 */

#include <iostream>
#include <iomanip>
#include <functional>

#include "cont.hpp"


static Cont::Cont* theCont = nullptr;
long fact(long n) {
   if (n==0) {
       if (theCont)
           Cont::letgo(theCont);
       theCont = Cont::Cont::takeContinuation();
       return 1;
   } else return n * fact(n-1);      
}

std::function<int(int)> makeClosure() 
{
   int w = 0;
   auto f = [=](long a) mutable {  // that is a mutable closure. The w in the closure will change with each call!
      long rv = fact(a+w);
      w++;
      std::cout << "w is " << w << std::endl;
      return rv;
   };
   std::function<long(long)> g(f);
   for(int i=0;i<10;i++)
      g(4);

   std::cout << "Original w = " << w << std::endl;

   return g;
}

int main(int argc,char* argv[])
{
    using namespace std;
    Cont::initContinuationLibrary(&argc);

    auto f = makeClosure(); 

    std::cout << "closure: " << f(5) << std::endl;
    static volatile int nb = 0;
    if (nb++ < 10)
        theCont->call();
    else Cont::letgo(theCont);
    Cont::shutdown();
    return 0;
}
