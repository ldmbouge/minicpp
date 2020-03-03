#ifndef __MDDRELAX_H
#define __MDDRELAX_H

#include "mdd.hpp"
#include "trailable.hpp"
#include "mddnode.hpp"
#include <set>
#include <tuple>
#include <random>


struct MDDNodePtrOrder {
   bool operator()(const MDDNode* a,const MDDNode* b)  const noexcept {
      return a->getId() < b->getId();
   }
};

class MDDRelax : public MDD {
   const unsigned int _width;
   ::trail<unsigned> _lowest;
   std::mt19937 _rnG;
   std::vector<MDDState> _refs;
   const MDDState& pickReference(int layer,int layerSize) {
      std::uniform_int_distribution<int> sampler(0,layerSize-1);
      int dirIdx = sampler(_rnG);
      return layers[layer][dirIdx]->getState();
   }
   void rebuild();
   bool refreshNode(MDDNode* n,int l);
   std::set<MDDNode*,MDDNodePtrOrder> split(TVec<MDDNode*>& layer,int l);
   void spawn(std::set<MDDNode*,MDDNodePtrOrder>& delta,TVec<MDDNode*>& layer,unsigned int l);
   MDDNode* findSimilar(const std::multimap<float,MDDNode*>& layer,const MDDState& s,const MDDState& refDir);
   MDDNode* resetState(MDDNode* from,MDDNode* to,MDDState& s,int v,int l);
   void delState(MDDNode* state,int l);
public:
   MDDRelax(CPSolver::Ptr cp,int width = 32)
      : MDD(cp),_width(width),_lowest(cp->getStateManager(),0),
        _rnG(42)
   {}
   void trimDomains() override;
   void buildDiagram() override;
   void relaxLayer(int i);
   void propagate() override;
   void trimLayer(unsigned int layer) override;
   void debugGraph() override;
   const MDDState& ref(int l) const { return _refs[l];}
   void printRefs() {
      for(auto i=0u;i < numVariables;i++)
         std::cout << "R[" << i << "] = " << ref(i) << std::endl;
   }
};

#endif
