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
    const int minC = mdd.addDownState(d,0,INT_MAX,MinFun);
    const int maxC = mdd.addDownState(d,0,INT_MAX,MaxFun);
    const int rem  = mdd.addDownState(d,(int)x.size(),INT_MAX,MaxFun);
    mdd.arcExist(d,[minC,maxC,rem,tv,ub,lb] (const auto& parent,const auto& child,auto var,const auto& val) -> bool {
      bool vinS = tv == val;
      return (parent.down[minC] + vinS <= ub) && ((parent.down[maxC] + vinS +  parent.down[rem] - 1) >= lb);
    });

    mdd.transitionDown(d,minC,{minC},{},[minC,tv] (auto& out,const auto& parent,const auto& x, const auto& val) {
      bool allMembers = val.size()==1 && val.singleton() == tv;
      out.setInt(minC,parent.down[minC] + allMembers);
    });
    mdd.transitionDown(d,maxC,{maxC},{},[maxC,tv] (auto& out,const auto& parent,const auto& x, const auto& val) {
      bool oneMember = val.contains(tv);
      out.setInt(maxC,parent.down[maxC] + oneMember);
    });
    mdd.transitionDown(d,rem,{rem},{},[rem] (auto& out,const auto& parent,const auto& x,const auto& val) {
      out.setInt(rem,parent.down[rem] - 1);
    });      
    mdd.splitOnLargest([](const auto& in) { return -(double)in.getNumParents();});
    return d;
  }

  MDDCstrDesc::Ptr amongMDD(MDD::Ptr m, const Factory::Veci& x, int lb, int ub, std::set<int> rawValues) {
    MDDSpec& mdd = m->getSpec();
    ValueSet values(rawValues);
    auto d = mdd.makeConstraintDescriptor(x,"amongMDD");
    const int minC = mdd.addDownState(d,0,INT_MAX,MinFun);
    const int maxC = mdd.addDownState(d,0,INT_MAX,MaxFun);
    const int rem  = mdd.addDownState(d,(int)x.size(),INT_MAX,MaxFun);
    if (rawValues.size() == 1) {
      int tv = *rawValues.cbegin();
      mdd.arcExist(d,[minC,maxC,rem,tv,ub,lb] (const auto& parent,const auto& child,auto var, const auto& val) -> bool {
        bool vinS = tv == val;
        return (parent.down[minC] + vinS <= ub) && ((parent.down[maxC] + vinS +  parent.down[rem] - 1) >= lb);
      });
    } else {
      mdd.arcExist(d,[=] (const auto& parent,const auto& child,var<int>::Ptr var, const auto& val) -> bool {
        bool vinS = values.member(val);
        return (parent.down[minC] + vinS <= ub) && ((parent.down[maxC] + vinS +  parent.down[rem] - 1) >= lb);
      });
    }

    mdd.transitionDown(d,minC,{minC},{},[minC,values] (auto& out,const auto& parent,const auto& x, const auto& val) {
      bool allMembers = true;
      for(int v : val) {
        allMembers &= values.member(v);
        if (!allMembers) break;
      }
      out.setInt(minC,parent.down[minC] + allMembers);
    });
    mdd.transitionDown(d,maxC,{maxC},{},[maxC,values] (auto& out,const auto& parent,const auto& x, const auto& val) {
      bool oneMember = false;
      for(int v : val) {
        oneMember = values.member(v);
        if (oneMember) break;
      }
      out.setInt(maxC,parent.down[maxC] + oneMember);
    });
    mdd.transitionDown(d,rem,{rem},{},[rem] (auto& out,const auto& parent,const auto& x,const auto& val) {
      out.setInt(rem,parent.down[rem] - 1);
    });
      
    mdd.splitOnLargest([](const auto& in) { return -(double)in.getNumParents();});
    return d;
  }
  
  MDDCstrDesc::Ptr amongMDD2(MDD::Ptr m, const Factory::Veci& x, int lb, int ub, std::set<int> rawValues,MDDOpts opts) {
    MDDSpec& mdd = m->getSpec();
    ValueSet values(rawValues);
    auto d = mdd.makeConstraintDescriptor(x,"amongMDD");
    const int L = mdd.addDownState(d,0,INT_MAX,MinFun, opts.cstrP);
    const int U = mdd.addDownState(d,0,INT_MAX,MaxFun, opts.cstrP);
    const int Lup = mdd.addUpState(d,0,INT_MAX,MinFun, opts.cstrP);
    const int Uup = mdd.addUpState(d,0,INT_MAX,MaxFun, opts.cstrP);

    mdd.transitionDown(d,L,{L},{},[L,values](auto& out,const auto& parent,const auto& x, const auto& val) {
      bool allMembers = true;
      for(int v : val) {
        allMembers &= values.member(v);
        if (!allMembers) break;
      }
      out.set(L,parent.down.at(L) + allMembers);
    });
    mdd.transitionDown(d,U,{U},{},[U,values] (auto& out,const auto& parent,const auto& x, const auto& val) {
      bool oneMember = false;
      for(int v : val) {
        oneMember = values.member(v);
        if (oneMember) break;
      }
      out.set(U,parent.down.at(U) + oneMember);
    });

    mdd.transitionUp(d,Lup,{Lup},{},[Lup,values] (auto& out,const auto& child,const auto& x, const auto& val) {
      bool allMembers = true;
      for(int v : val) {
        allMembers &= values.member(v);
        if (!allMembers) break;
      }
      out.set(Lup,child.up.at(Lup) + allMembers);
    });
    mdd.transitionUp(d,Uup,{Uup},{},[Uup,values] (auto& out,const auto& child,const auto& x, const auto& val) {
      bool oneMember = false;
      for(int v : val) {
        oneMember = values.member(v);
        if (oneMember) break;
      }
      out.set(Uup,child.up.at(Uup) + oneMember);
    });

    mdd.arcExist(d,[=] (const auto& parent,const auto& child,var<int>::Ptr var, const auto& val) -> bool {
      return ((parent.down.at(U) + values.member(val) + child.up.at(Uup) >= lb) &&
              (parent.down.at(L) + values.member(val) + child.up.at(Lup) <= ub));
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
          return in.getDownState().at(L);
        }, opts.cstrP);
        break;
      case 5:
        mdd.splitOnLargest([L](const auto& in) {
          return -in.getDownState().at(L);
        }, opts.cstrP);
        break;
      case 6:
        mdd.splitOnLargest([U](const auto& in) {
          return in.getDownState().at(U);
        }, opts.cstrP);
        break;
      case 7:
        mdd.splitOnLargest([U](const auto& in) {
          return -in.getDownState().at(U);
        }, opts.cstrP);
        break;
      case 8:
        mdd.splitOnLargest([L,U](const auto& in) {
          return in.getDownState().at(U) - in.getDownState().at(L);
        }, opts.cstrP);
        break;
      case 9:
        mdd.splitOnLargest([L,U](const auto& in) {
          return in.getDownState().at(L) - in.getDownState().at(U);
        }, opts.cstrP);
        break;
      case 10:
        mdd.splitOnLargest([lb,ub,L,Lup,U,Uup](const auto& in) {
          return -((double)std::max(lb - (in.getDownState().at(L) + in.getUpState().at(Lup)),0) +
                   (double)std::max((in.getDownState().at(U) + in.getUpState().at(Uup)) - ub,0));
        }, opts.cstrP);
        break;
      case 11:
        mdd.splitOnLargest([lb,ub,L,Lup,U,Uup](const auto& in) {
          return (double)std::max(lb - (in.getDownState().at(L) + in.getUpState().at(Lup)),0) +
            (double)std::max((in.getDownState().at(U) + in.getUpState().at(Uup)) - ub,0);
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
        mdd.equivalenceClassValue([d,lb,ub,L,Lup,U,Uup,opts](const auto& down, const auto& up) -> int {
          return (lb - (down.at(L) + up.at(Lup)) > opts.eqThreshold) +
            2*(ub - (down.at(U) + up.at(Uup)) > opts.eqThreshold);
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
    const int L = mdd.addDownState(d,0,INT_MAX,MinFun, opts.cstrP);
    const int U = mdd.addDownState(d,0,INT_MAX,MaxFun, opts.cstrP);
    const int Lup = mdd.addUpState(d,0,INT_MAX,MinFun, opts.cstrP);
    const int Uup = mdd.addUpState(d,0,INT_MAX,MaxFun, opts.cstrP);

    mdd.transitionDown(d,L,{L},{},[L,tv] (auto& out,const auto& parent,const auto& x, const auto& val) {
      bool allMembers = val.size() == 1 && val.singleton() == tv;
      out.setInt(L,parent.down[L] + allMembers);
    });
    mdd.transitionDown(d,U,{U},{},[U,tv] (auto& out,const auto& parent,const auto& x, const auto& val) {
      bool oneMember = val.contains(tv);
      out.setInt(U,parent.down[U] + oneMember);
    });

    mdd.transitionUp(d,Lup,{Lup},{},[Lup,tv] (auto& out,const auto& child,const auto& x, const auto& val) {
      bool allMembers = val.size() == 1 && val.singleton() == tv;
      out.setInt(Lup,child.up[Lup] + allMembers);
    });
    mdd.transitionUp(d,Uup,{Uup},{},[Uup,tv] (auto& out,const auto& child,const auto& x, const auto& val) {
      bool oneMember = val.contains(tv);
      out.setInt(Uup,child.up[Uup] + oneMember);
    });

    mdd.arcExist(d,[tv,L,U,Lup,Uup,lb,ub] (const auto& parent,const auto& child,var<int>::Ptr var, const auto& val) -> bool {
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
          return in.getDownState().at(L);
        }, opts.cstrP);
        break;
      case 5:
        mdd.splitOnLargest([L](const auto& in) {
          return -in.getDownState().at(L);
        }, opts.cstrP);
        break;
      case 6:
        mdd.splitOnLargest([U](const auto& in) {
          return in.getDownState().at(U);
        }, opts.cstrP);
        break;
      case 7:
        mdd.splitOnLargest([U](const auto& in) {
          return -in.getDownState().at(U);
        }, opts.cstrP);
        break;
      case 8:
        mdd.splitOnLargest([L,U](const auto& in) {
          return in.getDownState().at(U) - in.getDownState().at(L);
        }, opts.cstrP);
        break;
      case 9:
        mdd.splitOnLargest([L,U](const auto& in) {
          return in.getDownState().at(L) - in.getDownState().at(U);
        }, opts.cstrP);
        break;
      case 10:
        mdd.splitOnLargest([lb,ub,L,Lup,U,Uup](const auto& in) {
          return -((double)std::max(lb - (in.getDownState().at(L) + in.getUpState().at(Lup)),0) +
                   (double)std::max((in.getDownState().at(U) + in.getUpState().at(Uup)) - ub,0));
        }, opts.cstrP);
        break;
      case 11:
        mdd.splitOnLargest([lb,ub,L,Lup,U,Uup](const auto& in) {
          return (double)std::max(lb - (in.getDownState().at(L) + in.getUpState().at(Lup)),0) +
            (double)std::max((in.getDownState().at(U) + in.getUpState().at(Uup)) - ub,0);
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
          return (lb - (down.at(L) + up.at(Lup)) > opts.eqThreshold) +
            2*(ub - (down.at(U) + up.at(Uup)) > opts.eqThreshold);
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
    const int L = mdd.addDownState(d,0,1,MinFun, opts.cstrP);
    const int Lup = mdd.addUpState(d,0,1,MinFun, opts.cstrP);

    mdd.transitionDown(d,L,{L},{},[L,values] (auto& out,const auto& parent,const auto& x, const auto& val) {
      bool allMembers = true;
      for(int v : val) {
        allMembers &= values.member(v);
        if (!allMembers) break;
      }
      out.set(L,parent.down.at(L) + allMembers);
    });
      
    mdd.transitionUp(d,Lup,{Lup},{},[Lup,values] (auto& out,const auto& child,const auto& x, const auto& val) {
      bool allMembers = true;
      for(int v : val) {
        allMembers &= values.member(v);
        if (!allMembers) break;
      }
      out.set(Lup,child.up.at(Lup) + allMembers);
    });
      
    mdd.arcExist(d,[=] (const auto& parent,const auto& child,var<int>::Ptr var, const auto& val) -> bool {
      return (parent.down.at(L) + values.member(val) + child.up.at(Lup) <= 1);
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
          return in.getDownState().at(L);
        }, opts.cstrP);
        break;
      case 5:
        mdd.splitOnLargest([L](const auto& in) {
          return -in.getDownState().at(L);
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
