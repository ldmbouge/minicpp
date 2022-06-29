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
#include "mdd.hpp"

struct MDDOpts {
   int nodeP; // node priority
   int candP; // candidate priority
   int cstrP; // constraint priority
   int appxEQMode; // approximation equivalence mode
   int eqThreshold; // equivalence threshold
};

namespace Factory {
   template <typename Fun> inline lambdaMap toDict(int min, int max,Fun clo)
   {
      lambdaMap r;
      for(int i = min; i <= max; i++)
         r[i] = clo(i);
      return r;
   }
   template <typename Fun>
   lambdaMap toDict(int min, int max,std::vector<int>& p,Fun clo)
   {
      lambdaMap r;
      for(int i = min; i <= max; i++)
         r[p[i]] = clo(i,p[i]);
      return r;
   }
   inline TransDesc tDesc(std::initializer_list<int> sp1,std::initializer_list<int> sp2,lambdaTrans f) {
      return std::make_tuple<std::set<int>,std::set<int>,lambdaTrans>(sp1,sp2,std::move(f));
   }
   MDDCstrDesc::Ptr amongMDD(MDD::Ptr m, const Factory::Vecb& x, int lb, int ub,std::set<int> rawValues);
   MDDCstrDesc::Ptr amongMDD(MDD::Ptr m, const Factory::Veci& x, int lb, int ub, std::set<int> rawValues);
   MDDCstrDesc::Ptr amongMDD2(MDD::Ptr m, const Factory::Veci& x, int lb, int ub, std::set<int> rawValues,MDDOpts opts = {.eqThreshold = 3});
   MDDCstrDesc::Ptr amongMDD2(MDD::Ptr m, const Factory::Vecb& x, int lb, int ub, std::set<int> rawValues,MDDOpts opts = {.eqThreshold = 3});
   MDDCstrDesc::Ptr upToOneMDD(MDD::Ptr m, const Factory::Vecb& x, std::set<int> rawValues,MDDOpts opts = {.eqThreshold = 3});
   
   MDDCstrDesc::Ptr allDiffMDD(MDD::Ptr m, const Factory::Veci& vars,MDDOpts opts = {.cstrP = 0});
   MDDCstrDesc::Ptr allDiffMDD2(MDD::Ptr m, const Factory::Veci& vars,MDDOpts opts = {.eqThreshold = 4});
   
   MDDCstrDesc::Ptr seqMDD(MDD::Ptr m,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues);
   MDDCstrDesc::Ptr seqMDD2(MDD::Ptr m,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues);
   MDDCstrDesc::Ptr seqMDD3(MDD::Ptr m,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues);

   MDDCstrDesc::Ptr gccMDD(MDD::Ptr m,const Factory::Veci& vars,const std::map<int,int>& ub);
   MDDCstrDesc::Ptr gccMDD2(MDD::Ptr m,const Factory::Veci& vars, const std::map<int,int>& lb, const std::map<int,int>& ub);

   MDDCstrDesc::Ptr sum(MDD::Ptr m,std::initializer_list<var<int>::Ptr> vars,int lb, int ub);
   MDDCstrDesc::Ptr sum(MDD::Ptr m,std::initializer_list<var<int>::Ptr> vars,std::initializer_list<int> array, int lb, int ub);
   MDDCstrDesc::Ptr sum(MDD::Ptr m,const Factory::Veci& vars, const std::vector<int>& array, int lb, int ub);
   MDDCstrDesc::Ptr sum(MDD::Ptr m,const Factory::Vecb& vars, var<int>::Ptr z);
   MDDCstrDesc::Ptr sum(MDD::Ptr m,std::initializer_list<var<int>::Ptr> vars,var<int>::Ptr z);
   MDDCstrDesc::Ptr sum(MDD::Ptr m,const Factory::Veci& vars, var<int>::Ptr z);
   MDDCstrDesc::Ptr sum(MDD::Ptr m,const Factory::Veci& vars, const std::vector<int>& array, var<int>::Ptr z);
   MDDCstrDesc::Ptr sum(MDD::Ptr m,const Factory::Veci& vars, const std::vector<std::vector<int>>& matrix, var<int>::Ptr z);

   MDDCstrDesc::Ptr maxCutObjectiveMDD(MDD::Ptr m,const Factory::Vecb& vars,
                                       const std::vector<std::vector<int>>& weights,
                                       var<int>::Ptr z,MDDOpts opts = {});
   
   inline MDDCstrDesc::Ptr seqMDD2(MDD::Ptr m,const Factory::Vecb& vars, int len, int lb, int ub, std::set<int> rawValues) {
      Factory::Veci v2(vars.size(),Factory::alloci(vars[0]->getStore()));
      for(auto i=0u;i < vars.size();i++) v2[i] = vars[i];
      return seqMDD2(m,v2,len,lb,ub,rawValues);
   }
   inline MDDCstrDesc::Ptr seqMDD3(MDD::Ptr m,const Factory::Vecb& vars, int len, int lb, int ub, std::set<int> rawValues) {
      Factory::Veci v2(vars.size(),Factory::alloci(vars[0]->getStore()));
      for(auto i=0u;i < vars.size();i++) v2[i] = vars[i];
      return seqMDD3(m,v2,len,lb,ub,rawValues);
   }
}

#endif
