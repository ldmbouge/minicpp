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

/**
 * Struct to convey options for MDD LTS.
 */
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

   /**
    * @brief Posting stub.
    *
    * This is for internal use only. It is meant to avoid repeating the MDD argument twice on the same line of code.
    * It uses let-polymorphism to bundle arguments in a short-lived object and _feed_ the factory function which does the real work.
    * There is one such small stub for each factory function.
    */
   struct AMStub0 {
      const Factory::Veci& _vars;
      int _lb,_ub;
      std::set<int> _rawValues;
      MDDCstrDesc::Ptr execute(MDD::Ptr m) const { return amongMDD(m,_vars,_lb,_ub,_rawValues);}      
   };
   struct AMStub1 {
      const Factory::Vecb& _vars;
      int _lb,_ub;
      std::set<int> _rawValues;
      MDDCstrDesc::Ptr execute(MDD::Ptr m) const { return amongMDD(m,_vars,_lb,_ub,_rawValues);}      
   };
   struct AMStub2 {
      const Factory::Veci& _vars;
      int _lb,_ub;
      std::set<int>   _rawValues;
      MDDOpts              _opts;
      MDDCstrDesc::Ptr execute(MDD::Ptr m) const { return amongMDD2(m,_vars,_lb,_ub,_rawValues,_opts);}    
   };
   struct AMStub3 {
      const Factory::Vecb& _vars;
      int _lb,_ub;
      std::set<int>   _rawValues;
      MDDOpts              _opts;
      MDDCstrDesc::Ptr execute(MDD::Ptr m) const { return amongMDD2(m,_vars,_lb,_ub,_rawValues,_opts);}    
   };
   struct AMStub4 {
      const Factory::Vecb& _vars;
      std::set<int>   _rawValues;
      MDDOpts              _opts;
      MDDCstrDesc::Ptr execute(MDD::Ptr m) const { return upToOneMDD(m,_vars,_rawValues,_opts);}    
   };
   inline AMStub0 amongMDD(const Factory::Veci& vars,int lb,int ub,std::set<int> rawValues) 
   {
      return AMStub0 {vars,lb,ub,rawValues};
   }
   inline AMStub1 amongMDD(const Factory::Vecb& vars,int lb,int ub,std::set<int> rawValues) 
   {
      return AMStub1 {vars,lb,ub,rawValues};
   }
   inline AMStub2 amongMDD2(const Factory::Veci& vars,int lb,int ub,std::set<int> rawValues,MDDOpts opts = {.eqThreshold = 3}) 
   {
      return AMStub2 {vars,lb,ub,rawValues, opts};
   }
   inline AMStub3 amongMDD2(const Factory::Vecb& vars,int lb,int ub,std::set<int> rawValues,MDDOpts opts = {.eqThreshold = 3}) 
   {
      return AMStub3 {vars,lb,ub,rawValues, opts};
   }
   inline AMStub4 upToOneMDD(const Factory::Vecb& vars,std::set<int> rawValues,MDDOpts opts = {.eqThreshold = 3}) 
   {
      return AMStub4 {vars,rawValues, opts};
   }
   
   MDDCstrDesc::Ptr allDiffMDD(MDD::Ptr m, const Factory::Veci& vars,MDDOpts opts = {.cstrP = 0});
   MDDCstrDesc::Ptr allDiffMDD2(MDD::Ptr m, const Factory::Veci& vars,MDDOpts opts = {.eqThreshold = 4});

   /**
    * @brief Posting stub.
    *
    * This is for internal use only. It is meant to avoid repeating the MDD argument twice on the same line of code.
    * It uses let-polymorphism to bundle arguments in a short-lived object and _feed_ the factory function which does the real work.
    * There is one such small stub for each factory function.
    */
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

   
   MDDCstrDesc::Ptr seqMDD(MDD::Ptr m,const Factory::Veci& vars,int len, int lb, int ub, std::set<int> rawValues);
   MDDCstrDesc::Ptr seqMDD2(MDD::Ptr m,const Factory::Veci& vars,int len, int lb, int ub, std::set<int> rawValues);
   MDDCstrDesc::Ptr seqMDD3(MDD::Ptr m,const Factory::Veci& vars,int len, int lb, int ub, std::set<int> rawValues);
   /**
    * Convenience type meant to capture a pointer to the sequence factory function
    */
   using seqFact = MDDCstrDesc::Ptr(*)(MDD::Ptr,const Factory::Veci&,int,int,int,std::set<int>);
   /**
    * @brief Posting stub.
    *
    * This is for internal use only. It is meant to avoid repeating the MDD argument twice on the same line of code.
    * It uses let-polymorphism to bundle arguments in a short-lived object and _feed_ the factory function which does the real work.
    * There is one such small stub for each factory function.
    */
   template <typename Fun> struct SQStub1 {
      const Factory::Veci& _vars;
      int           _len,_lb,_ub;
      std::set<int>   _rawValues;
      Fun             _fun;
      MDDCstrDesc::Ptr execute(MDD::Ptr m) const { return _fun(m,_vars,_len,_lb,_ub,_rawValues);}
   };
   inline SQStub1<seqFact> seqMDD(const Factory::Veci& vars,int len, int lb, int ub, std::set<int> rawValues)
   {
      return SQStub1<seqFact> {vars,len,lb,ub,rawValues,seqMDD};
   }
   inline SQStub1<seqFact> seqMDD2(const Factory::Veci& vars,int len, int lb, int ub, std::set<int> rawValues)
   {
      return SQStub1<seqFact> {vars,len,lb,ub,rawValues,seqMDD2};
   }
   inline SQStub1<seqFact> seqMDD3(const Factory::Veci& vars,int len, int lb, int ub, std::set<int> rawValues)
   {
      return SQStub1<seqFact> {vars,len,lb,ub,rawValues,seqMDD3};
   }

   MDDCstrDesc::Ptr atMostMDD(MDD::Ptr m,const Factory::Veci& vars,const std::map<int,int>& ub);
   MDDCstrDesc::Ptr atMostMDD2(MDD::Ptr m,const Factory::Veci& vars,const std::map<int,int>& ub);
   
   MDDCstrDesc::Ptr gccMDD(MDD::Ptr m,const Factory::Veci& vars,const std::map<int,int>& ub);
   MDDCstrDesc::Ptr gccMDD2(MDD::Ptr m,const Factory::Veci& vars,const std::map<int,int>& lb, const std::map<int,int>& ub);

   /**
    * @brief Posting stub.
    *
    * This is for internal use only. It is meant to avoid repeating the MDD argument twice on the same line of code.
    * It uses let-polymorphism to bundle arguments in a short-lived object and _feed_ the factory function which does the real work.
    * There is one such small stub for each factory function.
    */
   struct GCCStub1 {
      const Factory::Veci&   _vars;
      const std::map<int,int>& _ub;      
      MDDCstrDesc::Ptr execute(MDD::Ptr m) const { return gccMDD(m,_vars,_ub);}
   };
   inline GCCStub1 gccMDD(const Factory::Veci& vars,const std::map<int,int>& ub)
   {
      return GCCStub1 {vars,ub};
   }
   struct GCCStub2 {
      const Factory::Veci&   _vars;
      const std::map<int,int>& _lb,_ub;      
      MDDCstrDesc::Ptr execute(MDD::Ptr m) const { return gccMDD2(m,_vars,_lb,_ub);}
   };
   inline GCCStub2 gccMDD2(const Factory::Veci& vars,const std::map<int,int>& lb,const std::map<int,int>& ub)
   {
      return GCCStub2 {vars,lb,ub};
   }
   
   

   MDDCstrDesc::Ptr sum(MDD::Ptr m,std::initializer_list<var<int>::Ptr> vars,int lb, int ub);
   MDDCstrDesc::Ptr sum(MDD::Ptr m,std::initializer_list<var<int>::Ptr> vars,std::initializer_list<int> array, int lb, int ub);
   MDDCstrDesc::Ptr sum(MDD::Ptr m,std::initializer_list<var<int>::Ptr> vars,var<int>::Ptr z);
   MDDCstrDesc::Ptr sum(MDD::Ptr m,std::vector<var<int>::Ptr> vars,int lb, int ub);
   MDDCstrDesc::Ptr sum(MDD::Ptr m,const Factory::Veci& vars, const std::vector<int>& array, int lb, int ub);
   MDDCstrDesc::Ptr sum(MDD::Ptr m,const Factory::Veci& vars, var<int>::Ptr z);
   MDDCstrDesc::Ptr sum(MDD::Ptr m,const Factory::Veci& vars, const std::vector<int>& array, var<int>::Ptr z);
   MDDCstrDesc::Ptr sum(MDD::Ptr m,const Factory::Veci& vars, const std::vector<std::vector<int>>& matrix, var<int>::Ptr z);
   MDDCstrDesc::Ptr sum(MDD::Ptr m,const Factory::Vecb& vars, var<int>::Ptr z);
   
   /**
    * @brief Posting stub.
    *
    * This is for internal use only. It is meant to avoid repeating the MDD argument twice on the same line of code.
    * It uses let-polymorphism to bundle arguments in a short-lived object and _feed_ the factory function which does the real work.
    * There is one such small stub for each factory function.
    */
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
   struct SumStub4 {
      const Factory::Veci& _vars;
      const std::vector<std::vector<int>>& _matrix;
      var<int>::Ptr _z;
      MDDCstrDesc::Ptr execute(MDD::Ptr m) const { return sum(m,_vars,_matrix,_z);}
   };
   inline SumStub4 sum(const Factory::Veci& vars, const std::vector<std::vector<int>>& matrix, var<int>::Ptr z)
   {
      return SumStub4 {vars,matrix,z};
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

   MDDCstrDesc::Ptr absDiffMDD(MDD::Ptr m, const Factory::Veci& vars,MDDOpts opts = {});
   MDDCstrDesc::Ptr absDiffMDD(MDD::Ptr m,std::initializer_list<var<int>::Ptr> vars,MDDOpts opts = {});
}

#endif
