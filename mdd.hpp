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

#ifndef mdd_hpp
#define mdd_hpp

#include "trailVec.hpp"
#include "acstr.hpp"
#include "solver.hpp"
#include "mddstate.hpp"

class MDDNode;
class MDDEdge;
class MDDNodeFactory;

class MDD  : public Constraint {
public:
   typedef handle_ptr<MDD> Ptr;
   void saveGraph();
   virtual void debugGraph() {}
   void post() override;
   void post(MDDCstrDesc::Ptr cDesc);
   template <class Stub> void post(const Stub& s) {
      post(s.execute(this));
   }
   MDDSpec& getSpec()      { return _mddspec; }
   CPSolver::Ptr getSolver() const { return cp;}
   virtual void trimLayer(unsigned int layer);
   void scheduleRemoval(MDDNode*);
   int getSupport(int layer,int value) const { return supports[layer][value - oft[layer]];}
   void addSupport(int layer, int value)     { supports[layer][value - oft[layer]] += 1;}
   void delSupport(int layer, int value)     { supports[layer][value - oft[layer]] -= 1;}
   void removeSupport(int layer, int value);
   virtual void removeNode(MDDNode* node);
   void propagate() override;
   virtual void refreshAll() {}
   std::size_t usage() const { return mem->usage();}
   unsigned long nbLayers() const { return numVariables;}
   const std::vector<TVec<MDDNode*>>& getLayers() const {return layers;}
   unsigned long layerSize(const int layer) const { return layers[layer].size();}
   virtual void removeArc(int outL,int inL,MDDEdge* arc) {}
   unsigned long layerAbove(var<int>::Ptr theVar) const;
   unsigned long layerBelow(var<int>::Ptr theVar) const;
   MDD(CPSolver::Ptr cp);
protected:
   virtual bool trimDomains();
   void hookupPropagators();
   virtual void buildNextLayer(unsigned int i);
   virtual void buildDiagram();
   void addNodeToLayer(int layer,MDDNode* n,int forValue);
   int _lastNid;
   Trailer::Ptr trail;
   CPSolver::Ptr cp;
   Storage::Ptr mem;
   std::vector<var<int>::Ptr> x;
   std::vector<var<int>::Ptr> z;
   std::vector<TVec<MDDNode*>> layers;
   std::deque<MDDNode*> queue;
   ::trail<bool> _firstTime;
   std::vector<std::vector<::trail<int>>> supports;
   std::vector<int> oft;
   unsigned numVariables;
   MDDNode* root = nullptr;
   MDDNode* sink = nullptr;
   var<int>::Ptr objective = nullptr;
   MDDSpec _mddspec;
   bool    _posting;
   MDDNodeFactory* _nf;
   MDDStateFactory* _sf;
};

int bestValue(const MDD* m,var<int>::Ptr theVar);
int optProperty(const MDD* m,int p,var<int>::Ptr theVar);

class MDDTrim : public Constraint { //Trims layer when D(_var) changes.
   MDD* _mdd;
   unsigned int _layer;
public:
   MDDTrim(CPSolver::Ptr cp,MDD* mdd,unsigned int layer): Constraint(cp), _mdd(mdd), _layer(layer){}
   void post() override {}
   void propagate() override { _mdd->trimLayer(_layer);}
};

class MDDStats {
   //MDD* _mdd;
   unsigned _nbLayers;
   std::pair<int,int> _width;
   std::pair<std::size_t,std::size_t> _nbIEdges;
   std::pair<std::size_t,std::size_t> _nbOEdges;
public :
   MDDStats(MDD* mdd);
   friend inline std::ostream& operator<<(std::ostream& os,const MDDStats& s)
   {
      os << "#nbLayers:" << s._nbLayers << " ";
      os << "#w:[" << s._width.first << "," << s._width.second << "] ";
      os << "#in:[" << s._nbIEdges.first << "," << s._nbIEdges.second << "] ";
      os << "#out:[" << s._nbOEdges.first << "," << s._nbOEdges.second << "] ";
      return os << std::endl;
   }
};


#endif /* mdd_hpp */
