//
//  mdd.cpp
//  minicpp
//
//  Created by Waldy on 10/6/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

#include "mdd.hpp"
#include <unordered_map>

MDD::MDD(CPSolver::Ptr cp)
   : Constraint(cp),
     cp(cp),
     trail(cp->getStateManager())
{}

MDD::MDD(CPSolver::Ptr cp, Factory::Veci iv, bool reduced)
   : Constraint(cp),
     cp(cp),
     reduced(reduced),
     trail(cp->getStateManager())
{
   for(int i = 0; i < iv.size(); i++)
      x.push_back(iv[i]);   
}
/*
  MDD::post() initializes the MDD and starts the build process of the diagram.
*/
void MDD::post()
{
   this->x = _mddspec.getVars();
   this->numVariables = x.size();
   this->layers = std::vector< std::vector<MDDNode*> > (numVariables+1, std::vector<MDDNode*>(0));
   for(int i = 0; i < numVariables+1; i++)
      layerSize.emplace_back(trail,0);  
   this->supports = std::vector< std::vector<::trail<int>> >(numVariables, std::vector<::trail<int>>(0));
    
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
   std::size_t operator()(MDDState::Ptr s)  const noexcept { return s->getHash();}
};

/*
  MDD::buildDiagram builds the diagram with the MDD-based constraints specified in the root state.
*/
void MDD::buildDiagram(){
   // Generate Root and Sink Nodes for MDD
   this->sink = new MDDNode(cp, trail, this, (int) numVariables, 0);
   this->root = new MDDNode(cp, trail, _mddspec.baseState, this, 0, 0);

   sink->setIsSink(true);
   root->setIsSource(true);

   layers[0].push_back(this->root);
   layers[numVariables].push_back(sink);

   this->layerSize[0] = 1;
   this->layerSize[numVariables] = 1;
   //std::cout << "Num Vars:" << numVariables << std::endl;
   for(int i = 0; i < numVariables; i++){
      //std::cout << "x[" << i << "] " << x[i] << std::endl;
      //std::cout << "layersize" << layers[i].size() << std::endl;
      std::unordered_map<MDDState::Ptr,MDDNode*,MDDStateHash> umap(101);
      int lsize = 0;
      for(int v = x[i]->min(); v <= x[i]->max(); v++){
         if(!x[i]->contains(v)) continue;
         for(int pidx = 0; pidx < layers[i].size(); pidx++){
            MDDNode* parent = layers[i][pidx];
            auto state = _mddspec.createState(parent->getState(), x[i], v);            
            if(state != nullptr){                   
               if(i < numVariables - 1){
                  MDDNode* child = nullptr;
                  auto found = umap.find(state);
                  if (found != umap.end()) {
                     child = found->second;
                  } 
                  if(child == nullptr){
                     child = new MDDNode(this->cp, this->trail, state, this, i+1, lsize);
                     umap.insert({state,child});
                     layers[i+1].push_back(child);
                     lsize++;
                  }                        
                  parent->addArc(child, v);
               } else
                  parent->addArc(sink, v);                
               addSupport(i, v);
            }
         }
         if (getSupport(i,v) == 0)
             x[i]->remove(v);   
      }
      if(i < numVariables - 1){
         this->layerSize[i+1] = lsize;
      }
   }
   for(auto &layer : layers){
      for(auto node : layer){
         if(!node->getIsSink() && node->getNumChildren() < 1) node->remove();
         if(!node->getIsSource() && node->getNumParents() < 1) node->remove();
      }
   }
   propagate();
   for(int i = 0; i < numVariables; i++){
      if (!x[i]->isBound()) {
         x[i]->propagateOnDomainChange(this);
         //x[i]->propagateOnDomainChange(new (cp) MDDRemoval(cp,this));
         x[i]->propagateOnDomainChange(new (cp) MDDTrim(cp, this,i));
      }
   }
}

/*
  MDD::trimLayer(int layer) trims the nodes to remove arcs that are no longer consistent.
*/
void MDD::trimLayer(int layer)
{
   for(int i = layerSize[layer] - 1; i >= 0; i--) 
      layers[layer][i]->trim(x[layer]);   
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
   node->remove();
   //swap nodes in layer and decrement size of layer
   int l = node->getLayer();
   int lsize = layerSize[l].value();
   int nodeID = node->getPosition();
   layers[l][nodeID] = layers[l][lsize - 1];
   layers[l][lsize - 1] = node;
   layerSize[l] = lsize - 1;        
   layers[l][lsize - 1]->setPosition(lsize - 1);
   layers[l][nodeID]->setPosition(nodeID);
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
   supports[layer][value - oft[layer]] = supports[layer][value - oft[layer]] - 1;
   if(supports[layer][value - oft[layer]] < 1)
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
         if(!layers[l][i]->isActive()) continue;
         int nc = layers[l][i]->getNumChildren();
         auto ch = layers[l][i]->getChildren();
         for(int j = 0; j < nc; j++){
            int count = ch[j]->getChild()->getPosition();
            if(ch[j]->getParent()->getIsSource())
               std::cout << "src" << " ->" << "\"L[" << l+1 << "," << count << "] " << *layers[l+1][count]->getState() <<"\"";
            else if(ch[j]->getChild()->getIsSink())
               std::cout << "\"L[" << l << "," << i << "] " << *layers[l][i]->getState() << "\" ->" << "sink";
            else
               std::cout << "\"L[" << l << "," << i << "] " << *layers[l][i]->getState() << "\" ->"
                         << "\"L[" << l+1 << "," << count << "] " << *layers[l+1][count]->getState() << "\"";
            std::cout << " [ label=\"" << ch[j]->getValue() << "\" ];" << std::endl;

         }
      }
   }
   std::cout << "}" << std::endl;    
}
