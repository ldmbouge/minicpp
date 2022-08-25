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
 *
 * Contributions by Waldemar Cruz, Rebecca Gentzel, Willem Jan Van Hoeve
 */

#include "mddConstraints.hpp"
#include "mddnode.hpp"
#include <limits.h>


namespace Factory {
   /**
    * Creates an MDD specification for the constraint |x0 - x1| = x2
    * @param m the MDD propagator to receive the constraint
    * @param vars the variables [x0,x1,x2]
    * @return a pointer to a descriptor for the created MDD description
    *
    * The constraint is described in the talk. 
    */
   MDDCstrDesc::Ptr absDiffMDD(MDD::Ptr m, const Factory::Veci& vars,MDDOpts opts) {
     MDDSpec& mdd = m->getSpec();
     assert(vars.size()==3);     
     // Filtering rules based the following constraint:  |vars[0]-vars[1]| = vars[2]
     // referred to below as |x-y| = z.
     auto d = mdd.makeConstraintDescriptor(vars,"absDiffMDD");
     const auto udom  = domRange(vars);
     const int minDom = udom.first;
     const int maxDom = udom.second;
     const int domSize = maxDom - minDom + 1;
    
    const auto xSome  = mdd.downBSState(d,udom.second - udom.first + 1,0,MinFun);
    const auto ySome  = mdd.downBSState(d,udom.second - udom.first + 1,0,MinFun);
    const auto N      = mdd.downIntState(d,0,INT_MAX,MinFun);        // layer index
    
    const auto ySomeUp = mdd.upBSState(d,udom.second - udom.first + 1,0,MinFun);
    const auto zSomeUp = mdd.upBSState(d,udom.second - udom.first + 1,0,MinFun);
    const auto NUp     = mdd.upIntState(d,0,INT_MAX,MinFun);        // layer index
    
    mdd.transitionDown(d,xSome,{xSome,N},{},[=](auto& out,const auto& p,auto,const auto& val)  noexcept {
      out[xSome] = p.down[xSome]; 
      if (p.down[N]==0) {
        auto sv = out[xSome];
        for(auto v : val)
          sv.set(v - minDom);
      }
    });
    mdd.transitionDown(d,ySome,{ySome,N},{},[=](auto& out,const auto& p,auto,const auto& val)  noexcept {
      out[ySome] = p.down[ySome];
      if (p.down[N]==1) {
        auto sv = out[ySome];
        for(auto v : val)
          sv.set(v - minDom);
      }
    });

    mdd.transitionDown(d,N,{N},{},[N](auto& out,const auto& p,auto,const auto&) noexcept     {
       out[N]   = p.down[N]+1;
    });
    mdd.transitionUp(d,NUp,{NUp},{},[NUp](auto& out,const auto& c,auto,const auto&) noexcept {
       out[NUp] = c.up[NUp]+1;
    });

    mdd.transitionUp(d,ySomeUp,{ySomeUp,NUp},{},[=](auto& out,const auto& c,auto, const auto& val) noexcept {
      out[ySomeUp] = c.up[ySomeUp];
      if (c.up[NUp]==1) {
        auto sv(out[ySomeUp]);
        for(auto v : val)
          sv.set(v - minDom);                                 
      }
    });
    mdd.transitionUp(d,zSomeUp,{zSomeUp,NUp},{},[=](auto& out,const auto& c,auto, const auto& val) noexcept  {
       out[zSomeUp] = c.up[zSomeUp];
       if (c.up[NUp]==0) {
         auto sv = out[zSomeUp];
         for(auto v : val)
           sv.set(v - minDom);                                 
       }
    });

    mdd.arcExist(d,[=](const auto& p,const auto& c,var<int>::Ptr, const auto& val)  noexcept {
       switch(p.down[N]) {
          case 0: { // filter x variable
             MDDBSValue yVals = c.up[ySomeUp],zVals = c.up[zSomeUp];
             for(auto yofs : yVals) {
               const auto i = yofs + minDom;
               if (i == val) continue;
               const int zval = std::abs(val-i);
               if (zval >= udom.first && zval <= udom.second && zVals.getBit(zval))
                 return true;
             }    
             return false;
          }break;
          case 1: { // filter y variable
             MDDBSValue xVals = p.down[xSome],zVals = c.up[zSomeUp];
             for(auto xofs : xVals) {
                const auto i = xofs + minDom;
                if (i == val) continue;
                const int zval = std::abs(i-val);
                if (zval >= udom.first && zval <= udom.second && zVals.getBit(zval))
                   return true;
             }    
             return false;
          }break;
          case 2: { // filter z variable
             MDDBSValue xVals = p.down[xSome],yVals = p.down[ySome];
             for(const auto xofs : xVals) {
                const auto i = xofs + minDom;
                if (val==0) continue;
                const int yval1 = i-val,yval2 = i+val;
                if ((yval1 >= udom.first && yval1 <= udom.second && yVals.getBit(yval1)) ||
                    (yval2 >= udom.first && yval2 <= udom.second && yVals.getBit(yval2)))
                   return true;
             }    
             return false;
          }break;
          default: return true;
       }
       return true;
    });
      
    mdd.addRelaxationDown(d,xSome,[xSome](auto& out,const auto& l,const auto& r)  noexcept     {
       out[xSome].setBinOR(l[xSome],r[xSome]);
    });
    mdd.addRelaxationDown(d,ySome,[ySome](auto& out,const auto& l,const auto& r)  noexcept     {
       out[ySome].setBinOR(l[ySome],r[ySome]);
    });
    mdd.addRelaxationUp(d,ySomeUp,[ySomeUp](auto& out,const auto& l,const auto& r)  noexcept     {
       out[ySomeUp].setBinOR(l[ySomeUp],r[ySomeUp]);
    });
    mdd.addRelaxationUp(d,zSomeUp,[zSomeUp](auto& out,const auto& l,const auto& r)  noexcept      {
       out[zSomeUp].setBinOR(l[zSomeUp],r[zSomeUp]);
    });
    switch (opts.appxEQMode) {
       case 1:
          mdd.equivalenceClassValue([=](const auto& down, const auto& up){
             MDDBSValue xVals = down[xSome], yVals = down[ySome];
             switch(down[N]) {
                case 1:
                   for (int i = domSize - 1; i >= 0; i--)
                      if (xVals.getBit(i)) return i;
                   break;
                case 2:
                   int minX = domSize, minY = domSize;
                   int maxX = 0, maxY = 0;
                   for (int i = domSize - 1; i >= 0; i--) {
                      if (xVals.getBit(i)) {
                         maxX = i;
                         break;
                      }
                   }
                   for (int i = domSize - 1; i >= 0; i--) {
                      if (yVals.getBit(i)) {
                         maxY = i;
                         break;
                      }
                   }
                   for (int i = 0; i < domSize; i++) {
                      if (xVals.getBit(i)) {
                         minX = i;
                         break;
                      }
                   }
                   for (int i = 0; i < domSize; i++) {
                      if (yVals.getBit(i)) {
                         minY = i;
                         break;
                      }
                   }
                   return std::max(std::abs(maxX - minY), std::abs(maxY - minX));
             }
             return 0;
          }, opts.cstrP);
          break;
       default:
          break;
    }
    return d;
  }
   /**
    * Convenience function to create |x0 - x1| = x2.
    * It uses an initializer_list and converts to an array to call the function above
    * that does the real work.
    */
   MDDCstrDesc::Ptr absDiffMDD(MDD::Ptr m,std::initializer_list<var<int>::Ptr> vars,MDDOpts opts) {
      CPSolver::Ptr cp = (*vars.begin())->getSolver();
      auto theVars = Factory::intVarArray(cp,vars.size(),[&vars](int i) {
         return std::data(vars)[i];
      });
      return absDiffMDD(m,theVars,opts);
   }
}
