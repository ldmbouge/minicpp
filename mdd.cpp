//
//  mdd.cpp
//  minicpp
//
//  Created by Waldy on 10/6/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

#include "mdd.hpp"
#include <unordered_map>

void pN(MDDNode* n)
{
   std::cout << n->getState() << std::endl;
}

MDD::MDD(CPSolver::Ptr cp)
   : Constraint(cp),
     cp(cp),
     trail(cp->getStateManager())
{
   mem = new Storage(trail);
   setPriority(Constraint::CLOW);
}

/*
  MDD::post() initializes the MDD and starts the build process of the diagram.
*/
void MDD::post()
{
   x = _mddspec.getVars();
   numVariables = x.size();
   layers = std::vector<TVec<MDDNode*>>(numVariables+1);
   for(int i = 0; i < numVariables+1; i++)
      layers[i] = TVec<MDDNode*>(trail,mem,32);

   supports = std::vector< std::vector<::trail<int>> >(numVariables, std::vector<::trail<int>>(0));

   //Create Supports for all values for each variable
   for(int i = 0; i < numVariables; i++){
      for(int v = x[i]->min(); v <= x[i]->max(); v++)
         supports[i].emplace_back(trail,0);
      oft.push_back(x[i]->min());
   }
   this->buildDiagram();
}

void MDD::propagate()
{
   while (!queue.empty()) {
      auto node = queue.front();
      queue.pop_front();
      removeNode(node);
   }
}

struct MDDStateHash {
   std::size_t operator()(MDDState* s)  const noexcept { return s->getHash();}
};

struct MDDStateEqual {
   bool operator()(const MDDState* s1,const MDDState* s2) const { return *s1 == *s2;}
};

/*
  MDD::buildDiagram builds the diagram with the MDD-based constraints specified in the root state.
*/
void MDD::buildDiagram(){
   // Generate Root and Sink Nodes for MDD
   _mddspec.layout();
   std::cout << _mddspec << std::endl;
   auto rootState = _mddspec.rootState(mem);

   sink = new (mem) MDDNode(mem, trail, (int) numVariables, 0);
   root = new (mem) MDDNode(mem, trail, rootState, x[0]->size(),0, 0);

   layers[0].push_back(root,mem);
   layers[numVariables].push_back(sink,mem);
  
   //std::cout << "Num Vars:" << numVariables << std::endl;
   std::unordered_map<MDDState*,MDDNode*,MDDStateHash,MDDStateEqual> umap(2999);
   for(int i = 0; i < numVariables; i++){
      //std::cout << "x[" << i << "] " << x[i] << std::endl;
      //std::cout << "layersize" << layers[i].size() << std::endl;
      int lsize = (int) layers[i+1].size();
      for(int v = x[i]->min(); v <= x[i]->max(); v++){
         if(!x[i]->contains(v)) continue;
         for(int pidx = 0; pidx < layers[i].size(); pidx++){
            MDDNode* parent = layers[i][pidx];
            MDDState state;
            bool     ok;
            std::tie(state,ok) = _mddspec.createState(mem,parent->getState(), x[i], v);
            if(ok) {
               if(i < numVariables - 1){
                  MDDNode* child = nullptr;
                  auto found = umap.find(&state);
                  if (found != umap.end()) {
                     child = found->second;
                  }
                  if(child == nullptr){
                     child = new (mem) MDDNode(mem, trail, state, x[i]->size(),i+1, lsize);
                     umap.insert({child->key(),child});
                     layers[i+1].push_back(child,mem);
                     lsize++;
                  }
                  parent->addArc(mem,child, v);
               } else
                  parent->addArc(mem,sink, v);
               addSupport(i, v);
            }
         }
         if (getSupport(i,v) == 0)
             x[i]->remove(v);
      }
      //std::cout << "UMAP[" << i << "] :" << umap.size() << std::endl;
      umap.clear();
   }
   for(auto i = 0; i < layers.size();i++) {
      auto& layer = layers[i];
      //for(auto node : layer) {
      for(int j = (int)layer.size() - 1;j >= 0;j--) {
         auto node = layer[j];
         if(i != numVariables && node->getNumChildren() < 1)
            removeNode(node);
         else if(i != 0 && node->getNumParents() < 1)
            removeNode(node);
      }
   }
   propagate();
   for(int i = 0; i < numVariables; i++){
      if (!x[i]->isBound()) {
         x[i]->propagateOnDomainChange(new (cp) MDDTrim(cp, this,i));
         x[i]->propagateOnDomainChange(this);
      }
   }
}

/*
  MDD::trimLayer(int layer) trims the nodes to remove arcs that are no longer consistent.
*/
void MDD::trimLayer(int layer)
{
   for(int i = (int) layers[layer].size() - 1; i >= 0; i--)
      layers[layer][i]->trim(this,x[layer]);
}

/*
  MDD::scheduleRemoval(MDDNode*) adds node to removal queue.
*/
void MDD::scheduleRemoval(MDDNode* node)
{
   queue.push_front(node);
}

void MDD::removeNode(MDDNode* node)
{
   if(node->isActive(this)){
      node->remove(this);
      //swap nodes in layer and decrement size of layer
      const int l      = node->getLayer();
      const int nodeID = node->getPosition();
      layers[l].remove(nodeID);
      node->setPosition((int)layers[l].size(),mem);
      layers[l][nodeID]->setPosition(nodeID,mem);
   }
}

int MDD::getSupport(int layer,int value) const
{
    return supports[layer][value - oft[layer]];
}
/*
  MDD::addSupport(int layer, int value) increments support for value at specific layer.
*/

void MDD::addSupport(int layer, int value)
{
   supports[layer][value - oft[layer]] = supports[layer][value - oft[layer]] + 1;
}

/*
  MDD::removeSupport(int layer, int value) decrements support for value at specific layer.
  If support for a value reaches 0, then value is removed from the domain.
*/

void MDD::removeSupport(int layer, int value)
{
   //assert(supports[layer][value - oft[layer]].value() > 0);
   int s = supports[layer][value - oft[layer]] = supports[layer][value - oft[layer]] - 1;
   if(s < 1)
      x[layer]->remove(value);
}

/*
  MDD::saveGraph() prints the current state of the MDD to stdout in dot format.
  Use a graphviz dot graph visualizer to create a graphical view of the diagram.
*/
void MDD::saveGraph()
{
   std::cout << "digraph MDD {" << std::endl;
   for(int l = 0; l < numVariables; l++){
      for(int i = 0; i < layers[l].size(); i++){
         if(!layers[l][i]->isActive(this)) continue;
         auto nc = layers[l][i]->getNumChildren();
         const auto& ch = layers[l][i]->getChildren();
         for(int j = 0; j < nc; j++){
            int count = ch[j]->getChild()->getPosition();
            assert(ch[j]->getParent() == layers[l][i]);
            if (l == 0)
               std::cout << "src" << " ->" << "\"" << *(layers[l+1][count]) <<"\"";
            else if(l+1 == numVariables)
               std::cout << "\"" << *(layers[l][i]) << "\" ->" << "sink";
            else
               std::cout << "\"" << *(layers[l][i]) << "\" ->"
                         << "\"" << *(layers[l+1][count]) << "\"";
            std::cout << " [ label=\"" << ch[j]->getValue() << "\" ];" << std::endl;

         }
      }
   }
   std::cout << "}" << std::endl;
}



MDDStats::MDDStats(MDD* mdd) : _mdd(mdd), _nbLayers(mdd->nbLayers()) {
   _width = std::make_pair (INT_MAX,0);
   _nbIEdges = std::make_pair (INT_MAX,0);
   _nbOEdges = std::make_pair (INT_MAX,0);
   for(auto& layer : mdd->getLayers()){
      _width.first = std::min(_width.first,(int)layer.size());
      _width.second = std::max(_width.second,(int)layer.size());
      for(int i = 1; i < layer.size()-1; i++){
         auto n = layer[i];
         size_t out = n->getNumChildren();
         size_t in = n->getNumParents();
         _nbIEdges.first = (_nbIEdges.first < in) ? _nbIEdges.first : in;
         _nbIEdges.second = (_nbIEdges.second > in) ? _nbIEdges.second : in;
         _nbOEdges.first = (_nbOEdges.first < out) ? _nbOEdges.first : out;
         _nbOEdges.second = (_nbOEdges.second > out) ? _nbOEdges.second : out;
      }
   }
}
