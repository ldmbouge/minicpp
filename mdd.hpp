//
//  mdd.hpp
//  minicpp
//
//  Created by Waldy on 10/6/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

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
   MDD(CPSolver::Ptr cp);
   void saveGraph();
   virtual void debugGraph() {}
   void post() override;
   MDDSpec& getSpec()      { return _mddspec; }
   virtual void trimLayer(unsigned int layer);
   void scheduleRemoval(MDDNode*);
   int getSupport(int layer,int value) const { return supports[layer][value - oft[layer]];}
   void addSupport(int layer, int value)     { supports[layer][value - oft[layer]] += 1;} 
   void delSupport(int layer, int value)     { supports[layer][value - oft[layer]] -= 1;} 
   void removeSupport(int layer, int value);
   void removeNode(MDDNode* node);
   void propagate() override;
   virtual void refreshAll() {}
   std::size_t usage() const { return mem->usage();}
   unsigned long nbLayers() const { return numVariables;}
   std::vector<TVec<MDDNode*>>& getLayers() {return layers;}
   unsigned long layerSize(const int layer) {return layers[layer].size();}
   virtual void removeArc(int outL,int inL,MDDEdge* arc) {}
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

class MDDTrim : public Constraint { //Trims layer when D(_var) changes.
   MDD* _mdd;
   unsigned int _layer;
public:
   MDDTrim(CPSolver::Ptr cp, MDD* mdd,unsigned int layer): Constraint(cp), _mdd(mdd), _layer(layer){}
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
