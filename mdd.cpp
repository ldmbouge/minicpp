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

void pP(const MDDPropSet& p)
{
   std::cout << p << '\n';
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
   _posting = true;
   _nf = new (mem) MDDNodeFactory(mem,trail,std::numeric_limits<int>::max());
   _sf = new (mem) MDDStateFactory(&_mddspec);
}

/*
  MDD::post() initializes the MDD and starts the build process of the diagram.
*/
void MDD::post()
{
   _mddspec.varOrder();
   x = _mddspec.getVars();
   z = _mddspec.getGlobals();
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
   buildDiagram();
   _posting = false;
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
   std::size_t operator()(MDDState* s)  const noexcept { return s->hash();}
};

struct MDDStateEqual {
   bool operator()(const MDDState* s1,const MDDState* s2) const { return *s1 == *s2;}
};

void MDD::buildNextLayer(unsigned int i)
{
   std::unordered_map<MDDState*,MDDNode*,MDDStateHash,MDDStateEqual> umap(2999);
   MDDState state(&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSize()));
   for(int v = x[i]->min(); v <= x[i]->max(); v++) {
      if(!x[i]->contains(v)) continue;
      for(auto parent : layers[i]) { 
         if (!_mddspec.exist(parent->getState(),sink->getState(),x[i],v,false)) continue;
         if(i < numVariables - 1){
            _sf->createState(state,parent->getState(),i,x[i],MDDIntSet(v),false);
            auto found = umap.find(&state);
            MDDNode* child = nullptr;
            if (found == umap.end()){
               child = _nf->makeNode(state,x[i]->size(),i+1,(int)layers[i+1].size());
               umap.insert({child->key(),child});
               layers[i+1].push_back(child,mem);
            }  else {
               child = found->second;
            }
            parent->addArc(mem,child, v);
         } else {
            MDDState sinkState(sink->getState());
            _mddspec.copyStateUp(state,sink->getState());
            _sf->createState(state, parent->getState(), i, x[i], MDDIntSet(v),false);
            if (sink->getNumParents() == 0) {
               sinkState.copyState(state);
            } else {
               if (sinkState != state) {
                  _mddspec.relaxation(sinkState, state);
                  sinkState.relaxDown();
               }
            }
            parent->addArc(mem,sink, v);
         }
         addSupport(i, v);
      }
      if (getSupport(i,v) == 0)
         x[i]->remove(v);
   }
}

void MDD::addNodeToLayer(int layer,MDDNode* n,int forValue)
{
   n->activate();
   n->setPosition((int)layers[layer].size(),mem);
   layers[layer].push_back(n,mem);
   addSupport(layer-1,forValue);
}

bool MDD::trimDomains()
{
   bool changed = false;
   for(auto i = 1u; i < numVariables;i++) {
      const auto& layer = layers[i];
      for(int j = (int)layer.size() - 1;j >= 0;j--) {
         if(layer[j]->disconnected()) {
            removeNode(layer[j]);
            changed = true;
         }
      }
   }
   return changed;
}

void MDD::hookupPropagators()
{
   for(auto i = 0u; i < numVariables; ++i){
      if (!x[i]->isBound()) {
         x[i]->propagateOnDomainChange(new (cp) MDDTrim(cp, this,i));
         x[i]->propagateOnDomainChange(this);
      }
   }
   for(auto i= 0u;i < z.size();++i) {
      if (!z[i]->isBound()) {
         z[i]->whenDomainChange([this]() {
                                   refreshAll();
                                });
      }
   }
}

// Builds the diagram with the MDD-based constraints specified in the root state.
void MDD::buildDiagram()
{
   // Generate Root and Sink Nodes for MDD
   _mddspec.layout();
   _mddspec.compile();
   std::cout << _mddspec << std::endl;
   auto rootState = _mddspec.rootState(mem);
   auto sinkState = _mddspec.rootState(mem);
   sink = _nf->makeNode(sinkState,0,(int)numVariables,0);
   root = _nf->makeNode(rootState,x[0]->size(),0,0);
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
   auto var = x[layer];
   for(auto i = layers[layer].cbegin(); i != layers[layer].cend();i++) {
      auto& children = (*i)->getChildren();
      for(int i = (int)children.size() - 1; i >= 0 ; i--){
         auto arc = children.get(i);
         if(!var->contains(arc->getValue())) {
            arc->remove(this);
         }
      }   
   }
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
   assert(layers[node->getLayer()].get(node->getPosition()) == node);
}

void MDD::removeNode(MDDNode* node)
{
   if(node->isActive()){
      assert(layers[node->getLayer()].get(node->getPosition()) == node);
      node->deactivate();
      node->remove(this);
      //swap nodes in layer and decrement size of layer
      const int l      = node->getLayer();
      const int nodeID = node->getPosition();
      layers[l].remove(nodeID);
      node->setPosition((int)layers[l].size(),mem);
      layers[l].get(nodeID)->setPosition(nodeID,mem);
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
               std::cout << "\"" << *(layers[l][i]) << "\"" << " ->" << "\"" << *(layers[l+1][count]) <<"\"";
            else if(l+1 == numVariables)
               std::cout << "\"" << *(layers[l][i]) << "\" ->" << "\"" << *(layers[l+1][count]) << "\"";
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
      for(auto i = 0u; i < layer.size(); i++){
         auto n = layer[i];
         size_t out = n->getNumChildren();
         size_t in = n->getNumParents();
         if (n->getLayer() > 0) {
            _nbIEdges.first = (_nbIEdges.first < in) ? _nbIEdges.first : in;
            _nbIEdges.second = (_nbIEdges.second > in) ? _nbIEdges.second : in;
         }
         if (n->getLayer() < _nbLayers) {
            _nbOEdges.first = (_nbOEdges.first < out) ? _nbOEdges.first : out;
            _nbOEdges.second = (_nbOEdges.second > out) ? _nbOEdges.second : out;
         }
      }
   }
}
