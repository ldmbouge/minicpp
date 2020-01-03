//
//  mdd.cpp
//  minicpp
//
//  Created by Waldy on 10/6/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

#include "mdd.hpp"

MDD::MDD(CPSolver::Ptr cp): cp(cp), trail(cp->getStateManager()){}

MDD::MDD(CPSolver::Ptr cp, Factory::Veci iv, bool reduced)
   : cp(cp), reduced(reduced), trail(cp->getStateManager())
{
   for(int i = 0; i < iv.size(); i++)
      x.push_back(iv[i]);   
}
/*
  MDD::post() initializes the MDD and starts the build process of the diagram.
*/
void MDD::post()
{
   this->queue = new std::deque<MDDNode*>;
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

/*
  MDD::buildDiagram builds the diagram with the MDD-based constraints specified in the root state.
*/
void MDD::buildDiagram(){
   // Generate Root and Sink Nodes for MDD
   this->sink = new MDDNode(this->cp, this->trail, this, (int) numVariables, 0);
   this->root = new MDDNode(this->cp, this->trail, x[0], _mddspec.baseState, this, 0, 0);

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
      x[i]->propagateOnDomainChange(new MDDRemoval(cp, x[i], this));
      x[i]->propagateOnDomainChange(new MDDTrim(cp, x[i], this, i));

      int lsize = 0;
      for(int v = x[i]->min(); v <= x[i]->max(); v++){
         if(!x[i]->contains(v)) continue;
         for(int pidx = 0; pidx < layers[i].size(); pidx++){
            MDDNode* parent = layers[i][pidx];
            auto state = _mddspec.createState(parent->getState(), x[i], v);
            if(state != nullptr){                   
               if(i < numVariables - 1){
                  MDDNode* child = nullptr;
                  for(auto curKid : layers[i+1]) {
                     if (*state == *curKid->getState()) {
                        child = curKid;
                        break;
                     }
                  }
                  if(child == nullptr){
                     child = new MDDNode(this->cp, this->trail, x[i+1], state, this, i+1, lsize);
                     layers[i+1].push_back(child);
                     lsize++;
                  }                        
                  parent->addArc(child, v);
               } else
                  parent->addArc(sink, v);                
               this->addSupport(i, v);
            }
         }
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
   this->startRemoval();
}

/*
  MDD::trimLayer(int layer) trims the nodes to remove arcs that are no longer consistent.
*/

void MDD::trimLayer(int layer){
   for(int i = layerSize[layer].value() - 1; i >= 0; i--){
      if(layers[layer][i]->isActive())
         layers[layer][i]->trim();
   }
}

/*
  MDD::scheduleRemoval(MDDNode*) adds node to removal queue.
*/

void MDD::scheduleRemoval(MDDNode* node){
   queue->emplace_front(node);
}

/*
  MDD::startRemoval() removes all nodes waiting in queue.
*/
void MDD::startRemoval(){
   while (!queue->empty()) {
      auto node = queue->front();
      queue->pop_front();
      this->removeNode(node);
   }
}

void MDD::removeNode(MDDNode* node){
   node->remove();
   //swap nodes in layer and decrement size of layer
   int l = node->getLayer();
   int lsize = this->layerSize[l].value();
   int nodeID = node->getPosition();
   this->layers[l][nodeID] = this->layers[l][lsize - 1];
   this->layers[l][lsize - 1] = node;
   this->layerSize[l] = lsize - 1;
        
   this->layers[l][lsize - 1]->setPosition(lsize - 1);
   this->layers[l][nodeID]->setPosition(nodeID);
}

/*
  MDD::addSupport(int layer, int value) increments support for value at specific layer.
*/

void MDD::addSupport(int layer, int value){
   supports[layer][value - oft[layer]] = supports[layer][value - oft[layer]].value() + 1;
}

/*
  MDD::removeSupport(int layer, int value) decrements support for value at specific layer. 
  If support for a value reaches 0, then value is removed from the domain.
*/

void MDD::removeSupport(int layer, int value){
   //assert(supports[layer][value - oft[layer]].value() > 0);
    
   supports[layer][value - oft[layer]] = supports[layer][value - oft[layer]].value() - 1;
   if(supports[layer][value - oft[layer]].value() < 1){
      x[layer]->remove(value);
   }
}

/*
  MDD::saveGraph() prints the current state of the MDD to stdout in dot format. 
  Use a graphviz dot graph visualizer to create a graphical view of the diagram.
*/
void MDD::saveGraph(){    
   std::cout << "digraph MDD {" << std::endl;
   for(int l = 0; l < numVariables; l++){
      for(int i = 0; i < layers[l].size(); i++){
         if(!layers[l][i]->isActive()) continue;
         int nc = layers[l][i]->getNumChildren();
         auto ch = layers[l][i]->getChildren();
         for(int j = 0; j < nc; j++){
            int count = ch[j]->getChild()->getPosition();

            std::cout << "Layer_" << l << "_" << i << " ->" << "Layer_" << l+1 << "_"
                      << count << " [ label=\"" << ch[j]->getValue() << "\" ];" << std::endl;

         }
      }
   }
   std::cout << "}" << std::endl;
    
}
