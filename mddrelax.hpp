#ifndef __MDDRELAX_H
#define __MDDRELAX_H

#include "mdd.hpp"
#include "trailable.hpp"
#include <set>
#include <tuple>

class MDDRelax : public MDD {
   const int _width;
   ::trail<int> _lowest;
   void rebuild();
   bool refreshNode(MDDNode* n,int l);
   std::set<MDDNode*> split(TVec<MDDNode*>& layer,int l);
   void spawn(std::set<MDDNode*>& delta,TVec<MDDNode*>& layer,int l);
   std::tuple<MDDNode*,double> findSimilar(std::vector<MDDNode*>& list,const MDDState& s);
   MDDNode* findSimilar(const std::map<float,MDDNode*>& layer,const MDDState& s,const MDDState& refDir);
   MDDNode* resetState(MDDNode* from,MDDNode* to,MDDState& s,int v,int l);
   void delState(MDDNode* state,int l);
public:
   MDDRelax(CPSolver::Ptr cp,int width = 32)
      : MDD(cp),_width(width),_lowest(cp->getStateManager(),0)
   {}
   void buildDiagram() override;
   void relaxLayer(int i);
   void propagate() override;
   void trimLayer(int layer) override;
};

#endif
