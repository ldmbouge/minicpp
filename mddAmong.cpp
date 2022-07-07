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

#include "mddConstraints.hpp"
#include "mddnode.hpp"
#include <limits.h>

namespace Factory {
   MDDCstrDesc::Ptr amongMDD(MDD::Ptr m, const Factory::Vecb& x, int lb, int ub,std::set<int> rawValues) {
      MDDSpec& mdd = m->getSpec();
      assert(rawValues.size()==1);
      int tv = *rawValues.cbegin();
      auto d = mdd.makeConstraintDescriptor(x,"amongMDD");
      const auto minC = mdd.downIntState(d,0,INT_MAX,MinFun);
      const auto maxC = mdd.downIntState(d,0,INT_MAX,MaxFun);
      const auto rem  = mdd.downIntState(d,(int)x.size(),INT_MAX,MaxFun);
      mdd.arcExist(d,[=] (const auto& parent,const auto& child,auto,const auto& val) {
         bool vinS = tv == val;
         return (parent.down[minC] + vinS <= ub) && ((parent.down[maxC] + vinS +  parent.down[rem] - 1) >= lb);
      });
      
      mdd.transitionDown2(d,minC,{minC},{},[minC,tv] (auto& out,const auto& parent,const auto& x, const auto& val) {
         bool allMembers = val.size()==1 && val.singleton() == tv;
         out[minC] = parent.down[minC] + allMembers;
      });
      mdd.transitionDown2(d,maxC,{maxC},{},[maxC,tv] (auto& out,const auto& parent,const auto& x, const auto& val) {
         bool oneMember = val.contains(tv);
         out[maxC] = parent.down[maxC] + oneMember;
      });
      mdd.transitionDown2(d,rem,{rem},{},[rem] (auto& out,const auto& parent,const auto& x,const auto& val) {
         out[rem] = parent.down[rem] - 1;
      });      
      mdd.splitOnLargest([](const auto& in) { return -(double)in.getNumParents();});
      return d;
   }

   MDDCstrDesc::Ptr amongMDD(MDD::Ptr m, const Factory::Veci& x, int lb, int ub, std::set<int> rawValues) {
      MDDSpec& mdd = m->getSpec();
      ValueSet values(rawValues);
      auto d = mdd.makeConstraintDescriptor(x,"amongMDD");
      const auto minC = mdd.downIntState(d,0,INT_MAX,MinFun);
      const auto maxC = mdd.downIntState(d,0,INT_MAX,MaxFun);
      const auto rem  = mdd.downIntState(d,(int)x.size(),INT_MAX,MaxFun);
      if (rawValues.size() == 1) {
         int tv = *rawValues.cbegin();
         mdd.arcExist(d,[=] (const auto& parent,const auto& child,auto, const auto& val)  {
            bool vinS = tv == val;
            return (parent.down[minC] + vinS <= ub) && ((parent.down[maxC] + vinS +  parent.down[rem] - 1) >= lb);
         });
      } else {
         mdd.arcExist(d,[=] (const auto& parent,const auto& child,auto, const auto& val)  {
            bool vinS = values.member(val);
            return (parent.down[minC] + vinS <= ub) && ((parent.down[maxC] + vinS +  parent.down[rem] - 1) >= lb);
         });
      }

      mdd.transitionDown2(d,minC,{minC},{},[minC,values] (auto& out,const auto& parent,const auto& x, const auto& val) {
         bool allMembers = true;
         for(int v : val) {
            allMembers &= values.member(v);
            if (!allMembers) break;
         }
         out[minC] = parent.down[minC] + allMembers;
      });
      mdd.transitionDown2(d,maxC,{maxC},{},[maxC,values] (auto& out,const auto& parent,const auto& x, const auto& val) {
         bool oneMember = false;
         for(int v : val) {
            oneMember = values.member(v);
            if (oneMember) break;
         }
         out[maxC] = parent.down[maxC] + oneMember;
      });
      mdd.transitionDown2(d,rem,{rem},{},[rem] (auto& out,const auto& parent,const auto& x,const auto& val) {
         out[rem] = parent.down[rem] - 1;
      });
      
      mdd.splitOnLargest([](const auto& in) { return -(double)in.getNumParents();});
      return d;
   }
  
  MDDCstrDesc::Ptr amongMDD2(MDD::Ptr m, const Factory::Veci& x, int lb, int ub, std::set<int> rawValues,MDDOpts opts) {
    MDDSpec& mdd = m->getSpec();
    ValueSet values(rawValues);
    auto d = mdd.makeConstraintDescriptor(x,"amongMDD");
    const auto L = mdd.downIntState(d,0,INT_MAX,MinFun, opts.cstrP);
    const auto U = mdd.downIntState(d,0,INT_MAX,MaxFun, opts.cstrP);
    const auto Lup = mdd.upIntState(d,0,INT_MAX,MinFun, opts.cstrP);
    const auto Uup = mdd.upIntState(d,0,INT_MAX,MaxFun, opts.cstrP);

    mdd.transitionDown2(d,L,{L},{},[L,values](auto& out,const auto& parent,const auto& x, const auto& val) {
       bool allMembers = true;
       for(int v : val) {
          allMembers &= values.member(v);
          if (!allMembers) break;
       }
       out[L] = parent.down[L] + allMembers;
    });
    mdd.transitionDown2(d,U,{U},{},[U,values] (auto& out,const auto& parent,const auto& x, const auto& val) {
       bool oneMember = false;
       for(int v : val) {
          oneMember = values.member(v);
          if (oneMember) break;
       }
       out[U] = parent.down[U] + oneMember;
    });

    mdd.transitionUp2(d,Lup,{Lup},{},[Lup,values] (auto& out,const auto& child,const auto& x, const auto& val) {
       bool allMembers = true;
       for(int v : val) {
          allMembers &= values.member(v);
          if (!allMembers) break;
       }
       out[Lup] = child.up[Lup] + allMembers;
    });
    mdd.transitionUp2(d,Uup,{Uup},{},[Uup,values] (auto& out,const auto& child,const auto& x, const auto& val) {
       bool oneMember = false;
       for(int v : val) {
          oneMember = values.member(v);
          if (oneMember) break;
       }
       out[Uup] = child.up[Uup] + oneMember;
    });

    mdd.arcExist(d,[=] (const auto& parent,const auto& child,const auto&, const auto& val) -> bool {
       return ((parent.down[U] + values.member(val) + child.up[Uup] >= lb) &&
               (parent.down[L] + values.member(val) + child.up[Lup] <= ub));
    });

    switch (opts.nodeP) {
      case 0:
        mdd.splitOnLargest([](const auto& in) {
          return in.getPosition();
        }, opts.cstrP);
        break;
      case 1:
        mdd.splitOnLargest([](const auto& in) {
          return -in.getPosition();
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
        mdd.splitOnLargest([L](const auto& in) {
          return in.getDownState()[L];
        }, opts.cstrP);
        break;
      case 5:
        mdd.splitOnLargest([L](const auto& in) {
          return -in.getDownState()[L];
        }, opts.cstrP);
        break;
      case 6:
        mdd.splitOnLargest([U](const auto& in) {
          return in.getDownState()[U];
        }, opts.cstrP);
        break;
      case 7:
        mdd.splitOnLargest([U](const auto& in) {
          return -in.getDownState()[U];
        }, opts.cstrP);
        break;
      case 8:
        mdd.splitOnLargest([L,U](const auto& in) {
          return in.getDownState()[U] - in.getDownState()[L];
        }, opts.cstrP);
        break;
      case 9:
        mdd.splitOnLargest([L,U](const auto& in) {
          return in.getDownState()[L] - in.getDownState()[U];
        }, opts.cstrP);
        break;
      case 10:
        mdd.splitOnLargest([lb,ub,L,Lup,U,Uup](const auto& in) {
           auto down = in.getDownState();
           auto up   = in.getUpState();
           return -((double)std::max(lb - (down[L] + up[Lup]),0) + (double)std::max((down[U] + up[Uup]) - ub,0));
        }, opts.cstrP);
        break;
      case 11:
        mdd.splitOnLargest([lb,ub,L,Lup,U,Uup](const auto& in) {
           auto down = in.getDownState();
           auto up   = in.getUpState();
           return (double)std::max(lb - (down[L] + up[Lup]),0) + (double)std::max((down[U] + up[Uup]) - ub,0);
        }, opts.cstrP);
      default:
        break;
    }
    switch(opts.candP) {
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
    switch (opts.appxEQMode) {
      case 0:
         mdd.equivalenceClassValue([=](const auto& down, const auto& up) -> int {
            return (lb - (down[L] + up[Lup]) > opts.eqThreshold) +
               2*(ub - (down[U] + up[Uup]) > opts.eqThreshold);
         }, opts.cstrP);
         break;
       default:
          break;
    }
    return d;
  }

  MDDCstrDesc::Ptr amongMDD2(MDD::Ptr m, const Factory::Vecb& x, int lb, int ub, std::set<int> rawValues,MDDOpts opts) {
    MDDSpec& mdd = m->getSpec();
    ValueSet values(rawValues);
    assert(rawValues.size()==1);
    int tv = *rawValues.cbegin();
    auto d = mdd.makeConstraintDescriptor(x,"amongMDD");
    const auto L = mdd.downIntState(d,0,INT_MAX,MinFun, opts.cstrP);
    const auto U = mdd.downIntState(d,0,INT_MAX,MaxFun, opts.cstrP);
    const auto Lup = mdd.upIntState(d,0,INT_MAX,MinFun, opts.cstrP);
    const auto Uup = mdd.upIntState(d,0,INT_MAX,MaxFun, opts.cstrP);
    
    mdd.transitionDown2(d,L,{L},{},[L,tv] (auto& out,const auto& parent,const auto& x, const auto& val) {
       bool allMembers = val.size() == 1 && val.singleton() == tv;
       out[L] = parent.down[L] + allMembers;
    });
    mdd.transitionDown2(d,U,{U},{},[U,tv] (auto& out,const auto& parent,const auto& x, const auto& val) {
       bool oneMember = val.contains(tv);
       out[U] = parent.down[U] + oneMember;
    });
    mdd.transitionUp2(d,Lup,{Lup},{},[Lup,tv] (auto& out,const auto& child,const auto& x, const auto& val) {
       bool allMembers = val.size() == 1 && val.singleton() == tv;
       out[Lup] = child.up[Lup] + allMembers;
    });
    mdd.transitionUp2(d,Uup,{Uup},{},[Uup,tv] (auto& out,const auto& child,const auto& x, const auto& val) {
       bool oneMember = val.contains(tv);
       out[Uup] = child.up[Uup] + oneMember;
    });

    mdd.arcExist(d,[tv,L,U,Lup,Uup,lb,ub] (const auto& parent,const auto& child,auto var, const auto& val) {
       const bool vinS = tv == val;
       return ((parent.down[U] + vinS + child.up[Uup] >= lb) &&
               (parent.down[L] + vinS + child.up[Lup] <= ub));
    });

    switch (opts.nodeP) {
      case 0:
        mdd.splitOnLargest([](const auto& in) {
          return in.getPosition();
        }, opts.cstrP);
        break;
      case 1:
        mdd.splitOnLargest([](const auto& in) {
          return -in.getPosition();
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
        mdd.splitOnLargest([L](const auto& in) {
          return in.getDownState()[L];
        }, opts.cstrP);
        break;
      case 5:
        mdd.splitOnLargest([L](const auto& in) {
          return -in.getDownState()[L];
        }, opts.cstrP);
        break;
      case 6:
        mdd.splitOnLargest([U](const auto& in) {
          return in.getDownState()[U];
        }, opts.cstrP);
        break;
      case 7:
        mdd.splitOnLargest([U](const auto& in) {
          return -in.getDownState()[U];
        }, opts.cstrP);
        break;
      case 8:
        mdd.splitOnLargest([L,U](const auto& in) {
          return in.getDownState()[U] - in.getDownState()[L];
        }, opts.cstrP);
        break;
      case 9:
        mdd.splitOnLargest([L,U](const auto& in) {
          return in.getDownState()[L] - in.getDownState()[U];
        }, opts.cstrP);
        break;
      case 10:
        mdd.splitOnLargest([lb,ub,L,Lup,U,Uup](const auto& in) {
          return -((double)std::max(lb - (in.getDownState()[L] + in.getUpState()[Lup]),0) +
                   (double)std::max((in.getDownState()[U] + in.getUpState()[Uup]) - ub,0));
        }, opts.cstrP);
        break;
      case 11:
        mdd.splitOnLargest([lb,ub,L,Lup,U,Uup](const auto& in) {
          return (double)std::max(lb - (in.getDownState()[L] + in.getUpState()[Lup]),0) +
            (double)std::max((in.getDownState()[U] + in.getUpState()[Uup]) - ub,0);
        }, opts.cstrP);
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
    switch (opts.appxEQMode) {
      case 0:
        mdd.equivalenceClassValue([d,lb,ub,L,Lup,U,Uup,opts](const auto& down, const auto& up) -> int {
          return (lb - (down[L] + up[Lup]) > opts.eqThreshold) +
             2*(ub - (down[U] + up[Uup]) > opts.eqThreshold);
        }, opts.cstrP);
        break;
      default:
        break;
    }
    return d;
  }

  MDDCstrDesc::Ptr upToOneMDD(MDD::Ptr m, const Factory::Vecb& x, std::set<int> rawValues,MDDOpts opts)
  {
    MDDSpec& mdd = m->getSpec();
    ValueSet values(rawValues);
    auto d = mdd.makeConstraintDescriptor(x,"upToOne");
    const auto L = mdd.downIntState(d,0,1,MinFun, opts.cstrP);
    const auto Lup = mdd.upIntState(d,0,1,MinFun, opts.cstrP);

    mdd.transitionDown2(d,L,{L},{},[L,values] (auto& out,const auto& parent,const auto& x, const auto& val) {
       bool allMembers = true;
       for(int v : val) {
          allMembers &= values.member(v);
          if (!allMembers) break;
       }
       out[L] = parent.down[L] + allMembers;
    });
      
    mdd.transitionUp2(d,Lup,{Lup},{},[Lup,values] (auto& out,const auto& child,const auto& x, const auto& val) {
      bool allMembers = true;
      for(int v : val) {
        allMembers &= values.member(v);
        if (!allMembers) break;
      }
      out[Lup] = child.up[Lup] + allMembers;
    });
      
    mdd.arcExist(d,[=] (const auto& parent,const auto& child,var<int>::Ptr var, const auto& val) -> bool {
      return (parent.down[L] + values.member(val) + child.up[Lup] <= 1);
    });

    switch (opts.nodeP) {
      case 0:
        mdd.splitOnLargest([](const auto& in) {
          return in.getPosition();
        }, opts.cstrP);
        break;
      case 1:
        mdd.splitOnLargest([](const auto& in) {
          return -in.getPosition();
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
        mdd.splitOnLargest([L](const auto& in) {
          return in.getDownState()[L];
        }, opts.cstrP);
        break;
      case 5:
        mdd.splitOnLargest([L](const auto& in) {
          return -in.getDownState()[L];
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
