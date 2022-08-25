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

   MDDCstrDesc::Ptr allDiffMDD(MDD::Ptr m, const Factory::Veci& vars,MDDOpts opts)
   {
      MDDSpec& mdd = m->getSpec();
      auto d = mdd.makeConstraintDescriptor(vars,"allDiffMdd");
      auto udom = domRange(vars);
      int minDom = udom.first;
      int domSize = udom.second - udom.first + 1;
      const int n    = (int)vars.size();
      const auto all   = mdd.downBSState(d,udom.second - udom.first + 1,0,External,opts.cstrP);
      const auto some  = mdd.downBSState(d,udom.second - udom.first + 1,0,External,opts.cstrP);
      const auto len   = mdd.downIntState(d,0,vars.size(),MinFun,opts.cstrP);
      const auto allu  = mdd.upBSState(d,udom.second - udom.first + 1,0,External,opts.cstrP);
      const auto someu = mdd.upBSState(d,udom.second - udom.first + 1,0,External,opts.cstrP);

      mdd.transitionDown(d,all,{all},{},[minDom,all](auto& out,const auto& in,const auto&,const auto& val) noexcept {
         out[all] = in.down[all];
         if (val.size()==1)
            out[all].set(val.singleton() - minDom);
      });
      mdd.transitionDown(d,some,{some},{},[minDom,some](auto& out,const auto& in,const auto&,const auto& val) noexcept {
         out[some] = in.down[some];
         MDDBSValue sv(out[some]);
         for(auto v : val)
            sv.set(v - minDom);
      });
      mdd.transitionDown(d,len,{len},{},[len](auto& out,const auto& in,const auto&,const auto& val) noexcept {
         out[len] = in.down[len] + 1;
      });
      mdd.transitionUp(d,allu,{allu},{},[minDom,allu](auto& out,const auto& in,const auto&,const auto& val) noexcept {
         out[allu] = in.up[allu];
         if (val.size()==1)
            out[allu].set(val.singleton() - minDom);
      });
      mdd.transitionUp(d,someu,{someu},{},[minDom,someu](auto& out,const auto& in,const auto&,const auto& val) noexcept {
         out[someu] = in.up[someu];
         MDDBSValue sv(out[someu]);
         for(auto v : val)
            sv.set(v - minDom);
      });

      mdd.addRelaxationDown(d,all,[all](auto& out,const auto& l,const auto& r) noexcept    {
         out[all].setBinAND(l[all],r[all]);
      });
      mdd.addRelaxationDown(d,some,[some](auto& out,const auto& l,const auto& r) noexcept  {
         out[some].setBinOR(l[some],r[some]);
      });
      mdd.addRelaxationUp(d,allu,[allu](auto& out,const auto& l,const auto& r)  noexcept   {
         out[allu].setBinAND(l[allu],r[allu]);
      });
      mdd.addRelaxationUp(d,someu,[someu](auto& out,const auto& l,const auto& r)  noexcept {
         out[someu].setBinOR(l[someu],r[someu]);
      });

      const int nbW = someu->size() >> 3;

      mdd.arcExist(d,[=](const auto& parent,const auto& child,const auto& var,const auto& val) noexcept  {
         const int ofs = val - minDom;
         if (parent.down[all].getBit(ofs))
            return false;
         MDDBSValue sbs = parent.down[some];         
         const bool notOk = sbs.getBit(ofs) && sbs.cardinality() == parent.down[len];
         if (notOk)
            return false;
         if (child.up[allu].getBit(ofs))
            return false;         
         MDDBSValue subs = child.up[someu];
         const bool upNotOk = subs.getBit(ofs) && subs.cardinality() == n - parent.down[len] - 1;
         if (upNotOk)
            return false;
         char tmp[64];
         char* tmpd = (nbW > 8) ?  (char*)alloca(sizeof(long long)*nbW) : tmp;
         MDDBSValue both(tmpd,nbW);
         both.setBinOR(subs,sbs).set(ofs);
         return both.cardinality() >= n;
      });

      int blockSize;
      int firstBlockMin, secondBlockMin, thirdBlockMin, fourthBlockMin;
      switch (opts.appxEQMode) {
         case 1:
            //Down only, ALL, 4 blocks
            blockSize = domSize/4;
            firstBlockMin = 0;
            secondBlockMin = firstBlockMin + blockSize;
            thirdBlockMin = secondBlockMin + blockSize;
            fourthBlockMin = thirdBlockMin + blockSize;
            mdd.equivalenceClassValue([=](const auto& down, const auto& up){
               int firstBlockCount = 0;
               int secondBlockCount = 0;
               int thirdBlockCount = 0;
               int fourthBlockCount = 0;
               MDDBSValue checkBits = down[all];
               int i = 0;
               for (i = firstBlockMin; i < secondBlockMin; i++)
                  if (checkBits.getBit(i)) firstBlockCount++;
               for (i = secondBlockMin; i < thirdBlockMin; i++)
                  if (checkBits.getBit(i)) secondBlockCount++;
               for (i = thirdBlockMin; i < fourthBlockMin; i++)
                  if (checkBits.getBit(i)) thirdBlockCount++;
               for (i = fourthBlockMin; i < domSize; i++)
                  if (checkBits.getBit(i)) fourthBlockCount++;
               return (firstBlockCount > opts.eqThreshold) +
                  2* (secondBlockCount > opts.eqThreshold) +
                  4* (thirdBlockCount > opts.eqThreshold) +
                  8* (fourthBlockCount > opts.eqThreshold);
            }, opts.cstrP);
            break;
          case 2:
            mdd.equivalenceClassValue([some,all,len](const auto& down, const auto& up) -> int {
               return (down[some].cardinality() - down[all].cardinality() < down[len]/2);
            }, opts.cstrP);
            break;
         default:
            break;
      }
      switch (opts.nodeP) {
         case 1:
            mdd.splitOnLargest([](const auto& in) {
               return in.getPosition();
            }, opts.cstrP);
            break;
         case 2:
            mdd.splitOnLargest([](const auto& in) {
               return in.getNumParents();
            }, opts.cstrP);
            break;
         case 3:
            mdd.splitOnLargest([](const auto& in) {
               return -in.getNumParents();
            }, opts.cstrP);
            break;
         case 4:
            mdd.splitOnLargest([](const auto& in) {
               return -in.getPosition();
            }, opts.cstrP);
            break;
         default:
            break;
      }
      switch (opts.candP) {
         case 1:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               return ((MDDEdge::Ptr*)arcs)[0]->getParent()->getPosition();
            }, opts.cstrP);
            break;
         case 2:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               return numArcs;
            }, opts.cstrP);
            break;
         case 3:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               return -numArcs;
            }, opts.cstrP);
            break;
         case 4:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int minParentIndex = ((MDDEdge::Ptr*)arcs)[0]->getParent()->getPosition();
               for (int i = 1; i < numArcs; i++) {
                  minParentIndex = std::min(minParentIndex, ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition());
               }
               return minParentIndex;
            }, opts.cstrP);
            break;
         case 5:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int maxParentIndex = ((MDDEdge::Ptr*)arcs)[0]->getParent()->getPosition();
               for (int i = 1; i < numArcs; i++) {
                  maxParentIndex = std::max(maxParentIndex, ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition());
               }
               return maxParentIndex;
            }, opts.cstrP);
            break;
         case 6:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int sumParentIndex = 0;
               for (int i = 0; i < numArcs; i++) {
                  sumParentIndex += ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition();
               }
               return sumParentIndex;
            }, opts.cstrP);
            break;
         case 7:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int sumParentIndex = 0;
               for (int i = 0; i < numArcs; i++) {
                  sumParentIndex += ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition();
               }
               return sumParentIndex/numArcs;
            }, opts.cstrP);
            break;
         case 8:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int minParentIndex = ((MDDEdge::Ptr*)arcs)[0]->getParent()->getPosition();
               for (int i = 1; i < numArcs; i++) {
                  minParentIndex = std::min(minParentIndex, ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition());
               }
               return -minParentIndex;
            }, opts.cstrP);
            break;
         case 9:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int maxParentIndex = ((MDDEdge::Ptr*)arcs)[0]->getParent()->getPosition();
               for (int i = 1; i < numArcs; i++) {
                  maxParentIndex = std::max(maxParentIndex, ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition());
               }
               return -maxParentIndex;
            }, opts.cstrP);
            break;
         case 10:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int sumParentIndex = 0;
               for (int i = 0; i < numArcs; i++) {
                  sumParentIndex += ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition();
               }
               return -sumParentIndex;
            }, opts.cstrP);
            break;
         case 11:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int sumParentIndex = 0;
               for (int i = 0; i < numArcs; i++) {
                  sumParentIndex += ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition();
               }
               return -sumParentIndex/numArcs;
            }, opts.cstrP);
            break;
         default:
            break;
      }
      return d;
   }
   MDDCstrDesc::Ptr allDiffMDD2(MDD::Ptr m, const Factory::Veci& vars,MDDOpts opts)
   {
      MDDSpec& mdd = m->getSpec();
      auto d = mdd.makeConstraintDescriptor(vars,"allDiffMdd");
      auto udom = domRange(vars);
      int minDom = udom.first;
      int domSize = udom.second - udom.first + 1;
      const auto n    = (int)vars.size();
      
      const auto all  = mdd.downBSState(d,udom.second - udom.first + 1,0,External,opts.cstrP);
      const auto some = mdd.downBSState(d,udom.second - udom.first + 1,0,External,opts.cstrP);
      const auto numInSome = mdd.downIntState(d,0,vars.size(),External,opts.cstrP);
      const auto len  = mdd.downIntState(d,0,vars.size(),MinFun,opts.cstrP);
      const auto allu = mdd.upBSState(d,udom.second - udom.first + 1,0,External,opts.cstrP);
      const auto someu = mdd.upBSState(d,udom.second - udom.first + 1,0,External,opts.cstrP);
      const auto numInSomeUp = mdd.upIntState(d,0,vars.size(),External,opts.cstrP);

      mdd.transitionDown(d,all,{all},{},[minDom,all](auto& out,const auto& in,const auto&,const auto& val) noexcept {
         out[all] = in.down[all];
         if (val.size()==1)
            out[all].set(val.singleton() - minDom);
      });
      mdd.transitionDown(d,some,{some},{},[minDom,some](auto& out,const auto& in,const auto&,const auto& val) noexcept {
         out[some] = in.down[some];
         MDDBSValue sv(out[some]);
         for(auto v : val)
            sv.set(v - minDom);
      });
      mdd.transitionDown(d,numInSome,{numInSome},{},[minDom,numInSome,some](auto& out,const auto& in,const auto&,const auto& val) noexcept {
         int num = in.down[numInSome];
         MDDBSValue sv(in.down[some]);
         for(auto v : val)
            if (!sv.getBit(v - minDom)) num++;
         out[numInSome] = num;
      });
      mdd.transitionDown(d,len,{len},{},[len](auto& out,const auto& in,const auto&,const auto& val) noexcept {
         out[len] = in.down[len] + 1;
      });
      mdd.transitionUp(d,allu,{allu},{},[minDom,allu](auto& out,const auto& in,const auto&,const auto& val) noexcept {
         out[allu] = in.up[allu];
         if (val.size()==1)
            out[allu].set(val.singleton() - minDom);
      });
      mdd.transitionUp(d,someu,{someu},{},[minDom,someu](auto& out,const auto& in,const auto&,const auto& val) noexcept {
         out[someu] = in.up[someu];
         MDDBSValue sv(out[someu]);
         for(auto v : val)
            sv.set(v - minDom);
      });
      mdd.transitionUp(d,numInSomeUp,{numInSomeUp},{},[minDom,numInSomeUp,someu](auto& out,const auto& in,const auto&,const auto& val) noexcept {
         int num = in.up[numInSomeUp];
         MDDBSValue sv(in.up[someu]);
         for(auto v : val)
            if (!sv.getBit(v - minDom)) num++;
         out[numInSomeUp] = num;
      });

      mdd.addRelaxationDown(d,all,[all](auto& out,const auto& l,const auto& r) noexcept    {
         out[all].setBinAND(l[all],r[all]);
      });
      mdd.addRelaxationDown(d,some,[some,numInSome](auto& out,const auto& l,const auto& r) noexcept    {
         out[some].setBinOR(l[some],r[some]);
         out[numInSome] = out[some].cardinality();
      });
      mdd.addRelaxationDown(d,numInSome,[](auto& out,const auto& l,const auto& r)  noexcept   {});
      mdd.addRelaxationUp(d,allu,[allu](auto& out,const auto& l,const auto& r)  noexcept   {
         out[allu].setBinAND(l[allu],r[allu]);
      });
      mdd.addRelaxationUp(d,someu,[someu,numInSomeUp](auto& out,const auto& l,const auto& r)  noexcept   {
         out[someu].setBinOR(l[someu],r[someu]);
         out[numInSomeUp] = out[someu].cardinality();
      });
      mdd.addRelaxationUp(d,numInSomeUp,[](auto& out,const auto& l,const auto& r)  noexcept   {});

      mdd.arcExist(d,[=](const auto& parent,const auto& child,const auto& var,const auto& val) noexcept {
         MDDBSValue sbs = parent.down[some];
         const int ofs = val - minDom;
         const bool notOk = parent.down[all].getBit(ofs) || (sbs.getBit(ofs) && parent.down[numInSome] == parent.down[len]);
         if (notOk) return false;
         bool upNotOk = false,mixNotOk = false;
         MDDBSValue subs = child.up[someu];
         upNotOk = child.up[allu].getBit(ofs) || (subs.getBit(ofs) && child.up[numInSomeUp] == n - parent.down[len] - 1);
         if (upNotOk) return false;
         MDDBSValue both((char*)alloca(sizeof(unsigned long long)*subs.nbWords()),subs.nbWords());
         both.setBinOR(subs,sbs).set(ofs);
         mixNotOk = both.cardinality() < n;
         return !mixNotOk;
      });
      int blockSize;
      int firstBlockMin, secondBlockMin, thirdBlockMin, fourthBlockMin;
      switch (opts.appxEQMode) {
         case 1:
            //Down only, ALL, 4 blocks
            blockSize = domSize/4;
            firstBlockMin = 0;
            secondBlockMin = firstBlockMin + blockSize;
            thirdBlockMin = secondBlockMin + blockSize;
            fourthBlockMin = thirdBlockMin + blockSize;
            mdd.equivalenceClassValue([=](const auto& down, const auto& up){
               int firstBlockCount = 0;
               int secondBlockCount = 0;
               int thirdBlockCount = 0;
               int fourthBlockCount = 0;
               MDDBSValue checkBits = down[all];
               int i = 0;
               for (i = firstBlockMin; i < secondBlockMin; i++)
                  if (checkBits.getBit(i)) firstBlockCount++;
               for (i = secondBlockMin; i < thirdBlockMin; i++)
                  if (checkBits.getBit(i)) secondBlockCount++;
               for (i = thirdBlockMin; i < fourthBlockMin; i++)
                  if (checkBits.getBit(i)) thirdBlockCount++;
               for (i = fourthBlockMin; i < domSize; i++)
                  if (checkBits.getBit(i)) fourthBlockCount++;
               return (firstBlockCount > opts.eqThreshold) +
                  2* (secondBlockCount > opts.eqThreshold) +
                  4* (thirdBlockCount > opts.eqThreshold) +
                  8* (fourthBlockCount > opts.eqThreshold);
            }, opts.cstrP);
            break;
         default:
            break;
      }
      switch (opts.nodeP) {
         case 0:
            mdd.splitOnLargest([](const auto& in) {
               return in.getPosition();
            }, opts.cstrP);
            break;
         case 1:
            mdd.splitOnLargest([](const auto& in) {
               return in.getNumParents();
            }, opts.cstrP);
            break;
         case 2:
            mdd.splitOnLargest([](const auto& in) {
               return -in.getNumParents();
            }, opts.cstrP);
            break;
         case 3:
            mdd.splitOnLargest([](const auto& in) {
               return -in.getPosition();
            }, opts.cstrP);
            break;
         case 4:
            mdd.splitOnLargest([numInSome](const auto& in) {
               return in.getDownState()[numInSome];
            }, opts.cstrP);
            break;
         case 5:
            mdd.splitOnLargest([numInSome](const auto& in) {
               return -in.getDownState()[numInSome];
            }, opts.cstrP);
            break;
         default:
            break;
      }
      switch (opts.candP) {
         case 0:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               return ((MDDEdge::Ptr*)arcs)[0]->getParent()->getPosition();
            }, opts.cstrP);
            break;
         case 1:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               return numArcs;
            }, opts.cstrP);
            break;
         case 2:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               return -numArcs;
            }, opts.cstrP);
            break;
         case 3:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int minParentIndex = ((MDDEdge::Ptr*)arcs)[0]->getParent()->getPosition();
               for (int i = 1; i < numArcs; i++) {
                  minParentIndex = std::min(minParentIndex, ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition());
               }
               return minParentIndex;
            }, opts.cstrP);
            break;
         case 4:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int maxParentIndex = ((MDDEdge::Ptr*)arcs)[0]->getParent()->getPosition();
               for (int i = 1; i < numArcs; i++) {
                  maxParentIndex = std::max(maxParentIndex, ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition());
               }
               return maxParentIndex;
            }, opts.cstrP);
            break;
         case 5:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int sumParentIndex = 0;
               for (int i = 0; i < numArcs; i++) {
                  sumParentIndex += ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition();
               }
               return sumParentIndex;
            }, opts.cstrP);
            break;
         case 6:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int sumParentIndex = 0;
               for (int i = 0; i < numArcs; i++) {
                  sumParentIndex += ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition();
               }
               return sumParentIndex/numArcs;
            }, opts.cstrP);
            break;
         case 7:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int minParentIndex = ((MDDEdge::Ptr*)arcs)[0]->getParent()->getPosition();
               for (int i = 1; i < numArcs; i++) {
                  minParentIndex = std::min(minParentIndex, ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition());
               }
               return -minParentIndex;
            }, opts.cstrP);
            break;
         case 8:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int maxParentIndex = ((MDDEdge::Ptr*)arcs)[0]->getParent()->getPosition();
               for (int i = 1; i < numArcs; i++) {
                  maxParentIndex = std::max(maxParentIndex, ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition());
               }
               return -maxParentIndex;
            }, opts.cstrP);
            break;
         case 9:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int sumParentIndex = 0;
               for (int i = 0; i < numArcs; i++) {
                  sumParentIndex += ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition();
               }
               return -sumParentIndex;
            }, opts.cstrP);
            break;
         case 10:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int sumParentIndex = 0;
               for (int i = 0; i < numArcs; i++) {
                  sumParentIndex += ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition();
               }
               return -sumParentIndex/numArcs;
            }, opts.cstrP);
            break;
         default:
            break;
      }
      return d;
   }
}
