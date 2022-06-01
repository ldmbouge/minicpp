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

   void allDiffMDD(MDDSpec& mdd, const Factory::Veci& vars, int constraintPriority)
   {
      mdd.append(vars);
      auto d = mdd.makeConstraintDescriptor(vars,"allDiffMdd");
      auto udom = domRange(vars);
      int minDom = udom.first;
      const int n    = (int)vars.size();
      const int all  = mdd.addDownBSState(d,udom.second - udom.first + 1,0,External,constraintPriority);
      const int some = mdd.addDownBSState(d,udom.second - udom.first + 1,0,External,constraintPriority);
      const int len  = mdd.addDownState(d,0,vars.size(),MinFun,constraintPriority);
      const int allu = mdd.addUpBSState(d,udom.second - udom.first + 1,0,External,constraintPriority);
      const int someu = mdd.addUpBSState(d,udom.second - udom.first + 1,0,External,constraintPriority);

      mdd.transitionDown(all,{all},{},[minDom,all](auto& out,const auto& inDown,const auto& inCombined,const auto& var,const auto& val,bool up) noexcept {
                               out.setProp(all,inDown);
                               if (val.size()==1)
                                  out.getBS(all).set(val.singleton() - minDom);
                            });
      mdd.transitionDown(some,{some},{},[minDom,some](auto& out,const auto& inDown,const auto& inCombined,const auto& var,const auto& val,bool up) noexcept {
                                out.setProp(some,inDown);
                                MDDBSValue sv(out.getBS(some));
                                for(auto v : val)
                                   sv.set(v - minDom);
                            });
      mdd.transitionDown(len,{len},{},[len](auto& out,const auto& inDown,const auto& inCombined,const auto& var,const auto& val,bool up) noexcept {
                                      out.set(len,inDown[len] + 1);
                                   });
      mdd.transitionUp(allu,{allu},{},[minDom,allu](auto& out,const auto& inUp,const auto& inCombined,const auto& var,const auto& val,bool up) noexcept {
                               out.setProp(allu,inUp);
                               if (val.size()==1)
                                  out.getBS(allu).set(val.singleton() - minDom);
                            });
      mdd.transitionUp(someu,{someu},{},[minDom,someu](auto& out,const auto& inUp,const auto& inCombined,const auto& var,const auto& val,bool up) noexcept {
                                out.setProp(someu,inUp);
                                MDDBSValue sv(out.getBS(someu));
                                for(auto v : val)
                                   sv.set(v - minDom);
                             });

      mdd.addRelaxationDown(all,[all](auto& out,const auto& l,const auto& r) noexcept    {
                               out.getBS(all).setBinAND(l.getBS(all),r.getBS(all));
                            });
      mdd.addRelaxationDown(some,[some](auto& out,const auto& l,const auto& r) noexcept    {
                                out.getBS(some).setBinOR(l.getBS(some),r.getBS(some));
                            });
      mdd.addRelaxationUp(allu,[allu](auto& out,const auto& l,const auto& r)  noexcept   {
                               out.getBS(allu).setBinAND(l.getBS(allu),r.getBS(allu));
                            });
      mdd.addRelaxationUp(someu,[someu](auto& out,const auto& l,const auto& r)  noexcept   {
                                out.getBS(someu).setBinOR(l.getBS(someu),r.getBS(someu));
                            });

      mdd.arcExist(d,[minDom,some,all,len,someu,allu,n](const auto& pDown,const auto& pCombined,const auto& cUp,const auto& cCombined,const auto& var,const auto& val,bool up) noexcept -> bool  {
                      MDDBSValue sbs = pDown.getBS(some);
                      const int ofs = val - minDom;
                      const bool notOk = pDown.getBS(all).getBit(ofs) || (sbs.getBit(ofs) && sbs.cardinality() == pDown[len]);
                      if (notOk) return false;
                      bool upNotOk = false,mixNotOk = false;
                      if (up) {
                         MDDBSValue subs = cUp.getBS(someu);
                         upNotOk = cUp.getBS(allu).getBit(ofs) || (subs.getBit(ofs) && subs.cardinality() == n - pDown[len] - 1);
                         if (upNotOk) return false;
                         MDDBSValue both((char*)alloca(sizeof(unsigned long long)*subs.nbWords()),subs.nbWords());
                         both.setBinOR(subs,sbs).set(ofs);
                         mixNotOk = both.cardinality() < n;
                      }
                      return !mixNotOk;
                   });
      mdd.equivalenceClassValue([some,all,len](const auto& down, const auto& up) -> int {
          return (down.getBS(some).cardinality() - down.getBS(all).cardinality() < down[len]/2);
      }, constraintPriority);
   }
   void allDiffMDD2(MDDSpec& mdd, const Factory::Veci& vars, int nodePriority, int candidatePriority, int approxEquivMode, int equivalenceThreshold, int constraintPriority)
   {
      mdd.append(vars);
      auto d = mdd.makeConstraintDescriptor(vars,"allDiffMdd");
      auto udom = domRange(vars);
      int minDom = udom.first;
      int domSize = udom.second - udom.first + 1;
      const int n    = (int)vars.size();
      const int all  = mdd.addDownBSState(d,udom.second - udom.first + 1,0,External,constraintPriority);
      const int some = mdd.addDownBSState(d,udom.second - udom.first + 1,0,External,constraintPriority);
      const int numInSome = mdd.addDownState(d,0,vars.size(),External,constraintPriority);
      const int len  = mdd.addDownState(d,0,vars.size(),MinFun,constraintPriority);
      const int allu = mdd.addUpBSState(d,udom.second - udom.first + 1,0,External,constraintPriority);
      const int someu = mdd.addUpBSState(d,udom.second - udom.first + 1,0,External,constraintPriority);
      const int numInSomeUp = mdd.addUpState(d,0,vars.size(),External,constraintPriority);

      mdd.transitionDown(all,{all},{},[minDom,all](auto& out,const auto& inDown,const auto& inCombined,const auto& var,const auto& val,bool up) noexcept {
                               out.setProp(all,inDown);
                               if (val.size()==1)
                                  out.getBS(all).set(val.singleton() - minDom);
                            });
      mdd.transitionDown(some,{some},{},[minDom,some](auto& out,const auto& inDown,const auto& inCombined,const auto& var,const auto& val,bool up) noexcept {
                                out.setProp(some,inDown);
                                MDDBSValue sv(out.getBS(some));
                                for(auto v : val)
                                   sv.set(v - minDom);
                            });
      mdd.transitionDown(numInSome,{numInSome},{},[minDom,numInSome,some](auto& out,const auto& inDown,const auto& inCombined,const auto& var,const auto& val,bool up) noexcept {
                                      int num = inDown[numInSome];
                                      MDDBSValue sv(inDown.getBS(some));
                                      for(auto v : val)
                                          if (!sv.getBit(v - minDom)) num++;
                                      out.set(numInSome,num);
                                   });
      mdd.transitionDown(len,{len},{},[len](auto& out,const auto& inDown,const auto& inCombined,const auto& var,const auto& val,bool up) noexcept {
                                      out.set(len,inDown[len] + 1);
                                   });
      mdd.transitionUp(allu,{allu},{},[minDom,allu](auto& out,const auto& inUp,const auto& inCombined,const auto& var,const auto& val,bool up) noexcept {
                               out.setProp(allu,inUp);
                               if (val.size()==1)
                                  out.getBS(allu).set(val.singleton() - minDom);
                            });
      mdd.transitionUp(someu,{someu},{},[minDom,someu](auto& out,const auto& inUp,const auto& inCombined,const auto& var,const auto& val,bool up) noexcept {
                                out.setProp(someu,inUp);
                                MDDBSValue sv(out.getBS(someu));
                                for(auto v : val)
                                   sv.set(v - minDom);
                             });
      mdd.transitionUp(numInSomeUp,{numInSomeUp},{},[minDom,numInSomeUp,someu](auto& out,const auto& inUp,const auto& inCombined,const auto& var,const auto& val,bool up) noexcept {
                                      int num = inUp[numInSomeUp];
                                      MDDBSValue sv(inUp.getBS(someu));
                                      for(auto v : val)
                                          if (!sv.getBit(v - minDom)) num++;
                                      out.set(numInSomeUp,num);
                                   });

      mdd.addRelaxationDown(all,[all](auto& out,const auto& l,const auto& r) noexcept    {
                               out.getBS(all).setBinAND(l.getBS(all),r.getBS(all));
                            });
      mdd.addRelaxationDown(some,[some,numInSome](auto& out,const auto& l,const auto& r) noexcept    {
                                out.getBS(some).setBinOR(l.getBS(some),r.getBS(some));
                                out.set(numInSome,out.getBS(some).cardinality());
                            });
      mdd.addRelaxationDown(numInSome,[](auto& out,const auto& l,const auto& r)  noexcept   {
                            });
      mdd.addRelaxationUp(allu,[allu](auto& out,const auto& l,const auto& r)  noexcept   {
                               out.getBS(allu).setBinAND(l.getBS(allu),r.getBS(allu));
                            });
      mdd.addRelaxationUp(someu,[someu,numInSomeUp](auto& out,const auto& l,const auto& r)  noexcept   {
                                out.getBS(someu).setBinOR(l.getBS(someu),r.getBS(someu));
                                out.set(numInSomeUp,out.getBS(someu).cardinality());
                            });
      mdd.addRelaxationUp(numInSomeUp,[](auto& out,const auto& l,const auto& r)  noexcept   {
                            });

      mdd.arcExist(d,[minDom,some,all,numInSome,len,someu,allu,numInSomeUp,n](const auto& pDown,const auto& pCombined,const auto& cUp,const auto& cCombined,const auto& var,const auto& val,bool up) noexcept -> bool  {
                      MDDBSValue sbs = pDown.getBS(some);
                      const int ofs = val - minDom;
                      const bool notOk = pDown.getBS(all).getBit(ofs) || (sbs.getBit(ofs) && pDown[numInSome] == pDown[len]);
                      if (notOk) return false;
                      bool upNotOk = false,mixNotOk = false;
                      if (up) {
                         MDDBSValue subs = cUp.getBS(someu);
                         upNotOk = cUp.getBS(allu).getBit(ofs) || (subs.getBit(ofs) && cUp[numInSomeUp] == n - pDown[len] - 1);
                         if (upNotOk) return false;
                         MDDBSValue both((char*)alloca(sizeof(unsigned long long)*subs.nbWords()),subs.nbWords());
                         both.setBinOR(subs,sbs).set(ofs);
                         mixNotOk = both.cardinality() < n;
                      }
                      return !mixNotOk;
                   });
      int blockSize;
      int firstBlockMin, secondBlockMin, thirdBlockMin, fourthBlockMin;
      switch (approxEquivMode) {
         case 1:
            //Down only, ALL, 4 blocks
            blockSize = domSize/4;
            firstBlockMin = 0;
            secondBlockMin = firstBlockMin + blockSize;
            thirdBlockMin = secondBlockMin + blockSize;
            fourthBlockMin = thirdBlockMin + blockSize;
            mdd.equivalenceClassValue([all,firstBlockMin,secondBlockMin,thirdBlockMin,fourthBlockMin,domSize,equivalenceThreshold](const auto& down, const auto& up) -> int {
                int firstBlockCount = 0;
                int secondBlockCount = 0;
                int thirdBlockCount = 0;
                int fourthBlockCount = 0;
                MDDBSValue checkBits = down.getBS(all);
                int i = 0;
                for (i = firstBlockMin; i < secondBlockMin; i++)
                   if (checkBits.getBit(i)) firstBlockCount++;
                for (i = secondBlockMin; i < thirdBlockMin; i++)
                   if (checkBits.getBit(i)) secondBlockCount++;
                for (i = thirdBlockMin; i < fourthBlockMin; i++)
                   if (checkBits.getBit(i)) thirdBlockCount++;
                for (i = fourthBlockMin; i < domSize; i++)
                   if (checkBits.getBit(i)) fourthBlockCount++;
                return (firstBlockCount > equivalenceThreshold) +
                    2* (secondBlockCount > equivalenceThreshold) +
                    4* (thirdBlockCount > equivalenceThreshold) +
                    8* (fourthBlockCount > equivalenceThreshold);
            }, constraintPriority);
            break;
         default:
            break;
      }
      switch (nodePriority) {
         case 0:
            mdd.splitOnLargest([](const auto& in) {
               return in.getPosition();
                               }, constraintPriority);
            break;
         case 1:
            mdd.splitOnLargest([](const auto& in) {
               return in.getNumParents();
                               }, constraintPriority);
            break;
         case 2:
            mdd.splitOnLargest([](const auto& in) {
               return -in.getNumParents();
                               }, constraintPriority);
            break;
         case 3:
            mdd.splitOnLargest([](const auto& in) {
               return -in.getPosition();
                               }, constraintPriority);
            break;
         case 4:
            mdd.splitOnLargest([numInSome](const auto& in) {
               return in.getDownState()[numInSome];
                               }, constraintPriority);
            break;
         case 5:
            mdd.splitOnLargest([numInSome](const auto& in) {
               return -in.getDownState()[numInSome];
                               }, constraintPriority);
            break;
         default:
            break;
      }
      switch (candidatePriority) {
         case 0:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               return ((MDDEdge::Ptr*)arcs)[0]->getParent()->getPosition();
            }, constraintPriority);
            break;
         case 1:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               return numArcs;
            }, constraintPriority);
            break;
         case 2:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               return -numArcs;
            }, constraintPriority);
            break;
         case 3:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int minParentIndex = ((MDDEdge::Ptr*)arcs)[0]->getParent()->getPosition();
               for (int i = 1; i < numArcs; i++) {
                  minParentIndex = std::min(minParentIndex, ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition());
               }
               return minParentIndex;
            }, constraintPriority);
            break;
         case 4:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int maxParentIndex = ((MDDEdge::Ptr*)arcs)[0]->getParent()->getPosition();
               for (int i = 1; i < numArcs; i++) {
                  maxParentIndex = std::max(maxParentIndex, ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition());
               }
               return maxParentIndex;
            }, constraintPriority);
            break;
         case 5:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int sumParentIndex = 0;
               for (int i = 0; i < numArcs; i++) {
                  sumParentIndex += ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition();
               }
               return sumParentIndex;
            }, constraintPriority);
            break;
         case 6:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int sumParentIndex = 0;
               for (int i = 0; i < numArcs; i++) {
                  sumParentIndex += ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition();
               }
               return sumParentIndex/numArcs;
            }, constraintPriority);
            break;
         case 7:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int minParentIndex = ((MDDEdge::Ptr*)arcs)[0]->getParent()->getPosition();
               for (int i = 1; i < numArcs; i++) {
                  minParentIndex = std::min(minParentIndex, ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition());
               }
               return -minParentIndex;
            }, constraintPriority);
            break;
         case 8:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int maxParentIndex = ((MDDEdge::Ptr*)arcs)[0]->getParent()->getPosition();
               for (int i = 1; i < numArcs; i++) {
                  maxParentIndex = std::max(maxParentIndex, ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition());
               }
               return -maxParentIndex;
            }, constraintPriority);
            break;
         case 9:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int sumParentIndex = 0;
               for (int i = 0; i < numArcs; i++) {
                  sumParentIndex += ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition();
               }
               return -sumParentIndex;
            }, constraintPriority);
            break;
         case 10:
            mdd.candidateByLargest([](const auto& state, void* arcs, int numArcs) {
               int sumParentIndex = 0;
               for (int i = 0; i < numArcs; i++) {
                  sumParentIndex += ((MDDEdge::Ptr*)arcs)[i]->getParent()->getPosition();
               }
               return -sumParentIndex/numArcs;
            }, constraintPriority);
            break;
         default:
           break;
      }
   }
}
