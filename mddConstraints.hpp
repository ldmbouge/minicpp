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
   MDDCstrDesc::Ptr amongMDD(MDD::Ptr m, const Factory::Vecb& x, int lb, int ub,std::set<int> rawValues);
   MDDCstrDesc::Ptr amongMDD(MDD::Ptr m, const Factory::Veci& x, int lb, int ub, std::set<int> rawValues);
   MDDCstrDesc::Ptr amongMDD2(MDD::Ptr m, const Factory::Veci& x, int lb, int ub, std::set<int> rawValues,MDDOpts opts = {.eqThreshold = 3});
   MDDCstrDesc::Ptr amongMDD2(MDD::Ptr m, const Factory::Vecb& x, int lb, int ub, std::set<int> rawValues,MDDOpts opts = {.eqThreshold = 3});
   MDDCstrDesc::Ptr upToOneMDD(MDD::Ptr m, const Factory::Vecb& x, std::set<int> rawValues,MDDOpts opts = {.eqThreshold = 3});
   
   MDDCstrDesc::Ptr allDiffMDD(MDD::Ptr m, const Factory::Veci& vars,MDDOpts opts = {.cstrP = 0});
   MDDCstrDesc::Ptr allDiffMDD2(MDD::Ptr m, const Factory::Veci& vars,MDDOpts opts = {.eqThreshold = 4});

   struct ADStub {
      const Factory::Veci& _vars;
      MDDOpts              _opts;
      MDDCstrDesc::Ptr execute(MDD::Ptr m) const { return allDiffMDD(m,_vars,_opts);}
   };
   inline ADStub allDiffMDD(const Factory::Veci& vars,MDDOpts opts = {.cstrP = 0})
   {
      return ADStub {vars,opts};
   }
   struct ADStub2 {
      const Factory::Veci& _vars;
      MDDOpts              _opts;
      MDDCstrDesc::Ptr execute(MDD::Ptr m) const { return allDiffMDD2(m,_vars,_opts);}
   };
   inline ADStub2 allDiffMDD2(const Factory::Veci& vars,MDDOpts opts = {.eqThreshold = 4})
   {
      return ADStub2 {vars,opts};
   }

   
   MDDCstrDesc::Ptr seqMDD(MDD::Ptr m,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues);
   MDDCstrDesc::Ptr seqMDD2(MDD::Ptr m,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues);
   MDDCstrDesc::Ptr seqMDD3(MDD::Ptr m,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues);

   MDDCstrDesc::Ptr gccMDD(MDD::Ptr m,const Factory::Veci& vars,const std::map<int,int>& ub);
   MDDCstrDesc::Ptr gccMDD2(MDD::Ptr m,const Factory::Veci& vars, const std::map<int,int>& lb, const std::map<int,int>& ub);


   MDDCstrDesc::Ptr sum(MDD::Ptr m,std::initializer_list<var<int>::Ptr> vars,int lb, int ub);
   MDDCstrDesc::Ptr sum(MDD::Ptr m,std::initializer_list<var<int>::Ptr> vars,std::initializer_list<int> array, int lb, int ub);
   MDDCstrDesc::Ptr sum(MDD::Ptr m,std::initializer_list<var<int>::Ptr> vars,var<int>::Ptr z);
   MDDCstrDesc::Ptr sum(MDD::Ptr m,std::vector<var<int>::Ptr> vars,int lb, int ub);
   MDDCstrDesc::Ptr sum(MDD::Ptr m,const Factory::Veci& vars, const std::vector<int>& array, int lb, int ub);
   MDDCstrDesc::Ptr sum(MDD::Ptr m,const Factory::Veci& vars, var<int>::Ptr z);
   MDDCstrDesc::Ptr sum(MDD::Ptr m,const Factory::Veci& vars, const std::vector<int>& array, var<int>::Ptr z);
   MDDCstrDesc::Ptr sum(MDD::Ptr m,const Factory::Veci& vars, const std::vector<std::vector<int>>& matrix, var<int>::Ptr z);
   MDDCstrDesc::Ptr sum(MDD::Ptr m,const Factory::Vecb& vars, var<int>::Ptr z);
   
   struct SumStub {
      const Factory::Veci& _vars;
      const std::vector<int>& _a;
      var<int>::Ptr           _z;
      MDDCstrDesc::Ptr execute(MDD::Ptr m) const { return sum(m,_vars,_a,_z);}
   };
   inline SumStub sum(const Factory::Veci& vars,const std::vector<int>& a,var<int>::Ptr z)
   {
      return SumStub {vars,a,z};
   }
   
   struct SumStub2 {
      std::vector<var<int>::Ptr> _vars;
      int _lb;
      int _ub;
      MDDCstrDesc::Ptr execute(MDD::Ptr m) const { return sum(m,_vars,_lb,_ub);}
   };
   inline SumStub2 sum(std::initializer_list<var<int>::Ptr> vars,int lb,int ub)
   {
      return SumStub2 {std::vector<var<int>::Ptr> {vars} ,lb,ub};
   }
   struct SumStub3 {
      const Factory::Veci& _vars;
      std::vector<int> _c;
      int _lb;
      int _ub;
      MDDCstrDesc::Ptr execute(MDD::Ptr m) const { return sum(m,_vars,_c,_lb,_ub);}
   };
   inline SumStub3 sum(const Factory::Veci& vars,std::initializer_list<int> c,int lb,int ub)
   {
      return SumStub3 {vars,std::vector<int> {c},lb,ub};
   }
   
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
