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

#ifndef __MDDCONSTRAINTS_H
#define __MDDCONSTRAINTS_H

#include "mddstate.hpp"

namespace Factory {
   template <typename Fun> inline lambdaMap toDict(int min, int max,Fun clo)
   {
      lambdaMap r;
      for(int i = min; i <= max; i++)
         r[i] = clo(i);
      return r;
   }
   inline lambdaMap toDict(int min, int max,std::vector<int>& p,std::function<lambdaTrans(int,int)> clo)
   {
      lambdaMap r;
      for(int i = min; i <= max; i++)
         r[p[i]] = clo(i,p[i]);
      return r;
   }
   void amongMDD(MDDSpec& mdd, const Factory::Vecb& x, int lb, int ub,std::set<int> rawValues);
   void amongMDD(MDDSpec& mdd, const Factory::Veci& x, int lb, int ub, std::set<int> rawValues);
   void amongMDD2(MDDSpec& mdd, const Factory::Veci& x, int lb, int ub, std::set<int> rawValues);
   void allDiffMDD(MDDSpec& mdd, const Factory::Veci& vars);
   void seqMDD(MDDSpec& spec,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues);
   void seqMDD2(MDDSpec& spec,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues);
   void seqMDD3(MDDSpec& spec,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues);
   void gccMDD(MDDSpec& spec,const Factory::Veci& vars,const std::map<int,int>& ub);
   void gccMDD2(MDDSpec& spec,const Factory::Veci& vars, const std::map<int,int>& lb, const std::map<int,int>& ub);
   void sumMDD(MDDSpec& mdd, const Factory::Veci& vars, const std::vector<int>& array, int lb, int ub);
   void sumMDD(MDDSpec& mdd, const Factory::Veci& vars, const std::vector<int>& array, var<int>::Ptr z);
   void sumMDD(MDDSpec& mdd, const Factory::Veci& vars, const std::vector<std::vector<int>>& matrix, var<int>::Ptr z);
   // void absDiffMDD(MDDSpec& mdd, const Factory::Veci& vars);
   inline void seqMDD2(MDDSpec& spec,const Factory::Vecb& vars, int len, int lb, int ub, std::set<int> rawValues) {
      Factory::Veci v2(vars.size(),Factory::alloci(vars[0]->getStore()));
      for(auto i=0u;i < vars.size();i++) v2[i] = vars[i];
      seqMDD2(spec,v2,len,lb,ub,rawValues);
   }
   inline void seqMDD3(MDDSpec& spec,const Factory::Vecb& vars, int len, int lb, int ub, std::set<int> rawValues) {
      Factory::Veci v2(vars.size(),Factory::alloci(vars[0]->getStore()));
      for(auto i=0u;i < vars.size();i++) v2[i] = vars[i];
      seqMDD3(spec,v2,len,lb,ub,rawValues);
   }
}

#endif
