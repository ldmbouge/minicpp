 //
//  mdd.cpp
//  minicpp
//
//  Created by Waldy on 10/6/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

#include "mdd.hpp"
#include "mddnode.hpp"
#include <unordered_map>
#include <climits>

void pN(MDDNode* n)
{
   std::cout << n->getState() << " [" << n->getNumChildren() << "]" << std::endl;
   for(auto& arc : n->getChildren()) {
      std::cout << '\t' << " - "  << arc->getValue() << " -> " 
                << arc->getChild()->getState() << " P:" << arc->getChild() << std::endl;
   }
}

void pS(const MDDState& s)
{
   std::cout << s << std::endl;
}

MDD::MDD(CPSolver::Ptr cp)
:  Constraint(cp),
   _lastNid(0),
   trail(cp->getStateManager()),
   cp(cp),
   _firstTime(trail,true)
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
   numVariables = (unsigned int) x.size();
   layers = std::vector<TVec<MDDNode*>>(numVariables+1);
   for(auto i = 0u; i < numVariables+1; i++)
      layers[i] = TVec<MDDNode*>(trail,mem,32);

   supports = std::vector< std::vector<::trail<int>> >(numVariables, std::vector<::trail<int>>(0));

   //Create Supports for all values for each variable
   for(auto i = 0u; i < numVariables; i++){
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

void MDD::buildNextLayer(unsigned int i)
{
   std::unordered_map<MDDState*,MDDNode*,MDDStateHash,MDDStateEqual> umap(2999);
   for(int v = x[i]->min(); v <= x[i]->max(); v++) {
      if(!x[i]->contains(v)) continue;
      for(auto pidx = 0u; pidx < layers[i].size(); pidx++) {
         MDDNode* parent = layers[i][pidx];
         MDDState state;
         bool     ok;
         std::tie(state,ok) = _mddspec.createState(mem,parent->getState(),i, x[i], v);
         if(ok) {
            if(i < numVariables - 1){
               auto found = umap.find(&state);
               MDDNode* child = nullptr;
               if(found == umap.end()){
                  child = new (mem) MDDNode(_lastNid++,mem, trail, state, x[i]->size(),i+1, (int)layers[i+1].size());
                  umap.insert({child->key(),child});
                  layers[i+1].push_back(child,mem);
               }  else child = found->second;
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
}

void MDD::trimDomains()
{
   for(auto i = 1u; i < numVariables;i++) {
      auto& layer = layers[i];
      for(int j = (int)layer.size() - 1;j >= 0;j--) {
         if(layer[j]->disconnected())
            removeNode(layer[j]);
      }
   }   
}

void MDD::hookupPropagators()
{
   for(auto i = 0u; i < numVariables; i++){
      if (!x[i]->isBound()) {
         x[i]->propagateOnDomainChange(new (cp) MDDTrim(cp, this,i));
         x[i]->propagateOnDomainChange(this);
      }
   }   
}

// Builds the diagram with the MDD-based constraints specified in the root state.
void MDD::buildDiagram()
{
   // Generate Root and Sink Nodes for MDD
   _mddspec.layout();
   std::cout << _mddspec << std::endl;
   auto rootState = _mddspec.rootState(mem);
   sink = new (mem) MDDNode(_lastNid++,mem, trail, (int) numVariables, 0);
   root = new (mem) MDDNode(_lastNid++,mem, trail, rootState, x[0]->size(),0, 0);
   layers[0].push_back(root,mem);
   layers[numVariables].push_back(sink,mem);

   for(auto i = 0u; i < numVariables; i++)
      buildNextLayer(i);   
   trimDomains();
   propagate();
   hookupPropagators();
}

/*
  MDD::trimLayer(int layer) trims the nodes to remove arcs that are no longer consistent.
*/
void MDD::trimLayer(unsigned int layer)
{
   if (_firstTime.fresh()) {
      _firstTime = false;
      queue.clear();
   }
   for(int i = (int) layers[layer].size() - 1; i >= 0; i--)
      layers[layer][i]->trim(this,x[layer]);
}

/*
  MDD::scheduleRemoval(MDDNode*) adds node to removal queue.
*/
void MDD::scheduleRemoval(MDDNode* node)
{
   if (_firstTime.fresh()) {
      _firstTime = false;
      queue.clear();
   }
   queue.push_front(node);
   assert(node->isActive());
   assert(layers[node->getLayer()][node->getPosition()] == node);
}

void MDD::removeNode(MDDNode* node)
{
   if(node->isActive()){
      assert(layers[node->getLayer()][node->getPosition()] == node);
      node->remove(this);
      node->deactivate();
      //swap nodes in layer and decrement size of layer
      const int l      = node->getLayer();
      const int nodeID = node->getPosition();
      layers[l].remove(nodeID);
      node->setPosition((int)layers[l].size(),mem);
      layers[l][nodeID]->setPosition(nodeID,mem);
      assert(node->getNumParents()==0);
      assert(node->getNumChildren()==0);
   }
}

/*
  MDD::removeSupport(int layer, int value) decrements support for value at specific layer.
  If support for a value reaches 0, then value is removed from the domain.
*/
void MDD::removeSupport(int layer, int value)
{
   int s = supports[layer][value - oft[layer]] -= 1;
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
   for(auto l = 0u; l < numVariables; l++){
      for(auto i = 0u; i < layers[l].size(); i++){
         if(!layers[l][i]->isActive()) continue;
         auto nc = layers[l][i]->getNumChildren();
         const auto& ch = layers[l][i]->getChildren();
         for(auto j = 0u; j < nc; j++){
            int count = ch[j]->getChild()->getPosition();
            assert(ch[j]->getParent() == layers[l][i]);
            if (l == 0)
               std::cout << "src" << " ->" << "\"" << *(layers[l+1][count]) <<"\"";
            else if(l+1 == numVariables)
               std::cout << "\"" << *(layers[l][i]) << "\" ->" << "sink";
            else {
               assert(layers[l+1][count] == ch[j]->getChild());
               std::cout << "\"" << *(layers[l][i]) << "\" ->"
                         << "\"" << *(layers[l+1][count]) << "\"";
            }
            std::cout << " [ label=\"" << ch[j]->getValue() << "\" ];" << std::endl;

         }
      }
   }
   std::cout << "}" << std::endl;
   for(auto l = 0u; l < numVariables; l++) {
      std::cout << "sup[" << l << "] = ";
      for(int v = x[l]->min();v <= x[l]->max();v++)
        std::cout << v << ":" << getSupport(l,v) << ',';
      std::cout << '\b' << std::endl;
   }
}

MDDStats::MDDStats(MDD* mdd) : _nbLayers((unsigned int)mdd->nbLayers()) {
   _width = std::make_pair (INT_MAX,0);
   _nbIEdges = std::make_pair (INT_MAX,0);
   _nbOEdges = std::make_pair (INT_MAX,0);
   for(auto& layer : mdd->getLayers()){
      _width.first = std::min(_width.first,(int)layer.size());
      _width.second = std::max(_width.second,(int)layer.size());
      for(auto i = 1u; i < layer.size()-1; i++){
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
