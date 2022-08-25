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

#include "mdd.hpp"
#include "mddnode.hpp"
#include <unordered_map>
#include <climits>

/**
 * Debugging support to print a node (pN)
 */
void pN(MDDNode* n)
{
   std::cout << "D:"
             << n->getDownState() << " U:"
             << n->getUpState()
             << " [#C:" << n->getNumChildren() << "]" << std::endl;
   for(auto& arc : n->getChildren()) {
      std::cout << '\t' << " - "  << arc->getValue() << " -> " 
                << arc->getChild()->getDownState()
                << " P:" << arc->getChild() << std::endl;
   }
}

/**
 * Debugging support to print a state (pS)
 */
void pS(const MDDState& s)
{
   std::cout << s << std::endl;
}
/**
 * Debugging support to print a property set (pP)
 */
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
   _mddspec.setConstraintPrioritySize(1);
   _nf = new (mem) MDDNodeFactory(mem,trail,std::numeric_limits<int>::max());
   _sf = nullptr;
}

void MDD::post(MDDCstrDesc::Ptr cDesc)
{
   getSpec();
}

/*
  MDD::post() initializes the MDD and starts the build process of the diagram.
*/
void MDD::post()
{
   _mddspec.varOrder();
   x = _mddspec.getVars();
   z = _mddspec.getGlobals();
   _sf = new (mem) MDDStateFactory(trail,&_mddspec);

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

unsigned long MDD::layerAbove(var<int>::Ptr theVar)
{
   for(auto i = 0u; i < numVariables+1; i++) 
      if (x[i] == theVar)
         return i;
   return 0;
}
unsigned long MDD::layerBelow(var<int>::Ptr theVar)
{
   for(auto i = 0u; i < numVariables+1; i++) 
      if (x[i] == theVar)
         return i+1;
   return numVariables;
}

int minCostDown(MDD* m,MDDNode* from,int depth)
{
   if (depth == 0 || from->getNumChildren() == 0)
      return 1;
   else {
      int bestCost = INT_MAX;
      for(auto& edge : from->getChildren()) {
         auto c = edge->getChild();
         int cInDeg = c->getNumParents();
         int recCost = minCostDown(m,c,depth-1);
         int dCost = cInDeg * recCost;
         if (dCost < bestCost)
            bestCost = dCost;
      }
      return bestCost;
   }
}

int bestValue(MDD* m,var<int>::Ptr theVar)
{
   auto& layers = m->getLayers();
   auto& layer = layers[m->layerAbove(theVar)];
   int bestFound = theVar->min();
   int bestFun = INT_MAX;
   for(auto& node : layer) {
      for(auto& edge : node->getChildren()) {
         int val = edge->getValue();
         auto c  = edge->getChild();
         int cInDeg = c->getNumParents();
         int down = minCostDown(m,c,2);
         int dCost = cInDeg * down;
         if (dCost < bestFun) {
            bestFun = dCost;
            bestFound = val;
         }
      }
   }
   return bestFound;
}

int optProperty(MDD* m,int p,var<int>::Ptr theVar)
{
   auto& layers = m->getLayers();
   auto& layer = layers[m->layerAbove(theVar)];
   int bestFound = -1;
   int bestFun = INT_MIN;
   for(auto& node : layer) {
      for(auto& edge : node->getChildren()) {
         int val = edge->getValue();
         auto c  = edge->getChild();
         auto pack = c->pack();
         auto theProp = pack.up.getSpec()->intPropUp(p);
         std::cout << pack.up << "\n";
         std::cout << "\tB/C:" << bestFun << "/" << pack.up[theProp] << "\n";
         if (pack.up[theProp] > bestFun) {
            bestFun = pack.up[theProp];
            bestFound = val;
         }
      }
   }
   return bestFound;
}

void MDD::propagate()
{
   while (!queue.empty()) {
      auto node = queue.front();
      queue.pop_front();
      if (node->isActive())
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
   MDDState downState(trail,&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSizeDown()),Down);
   MDDState upState(trail,&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSizeUp()),Up);
   MDDState combinedState(trail,&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSizeCombined()),Bi);
   for(int v = x[i]->min(); v <= x[i]->max(); v++) {
      if(!x[i]->contains(v)) continue;
      for(auto parent : layers[i]) { 
         if (!_mddspec.exist(parent->pack(),sink->pack(),x[i],v)) continue;
         if(i < numVariables - 1){
            _sf->createStateDown(downState,parent->pack(),i,x[i],MDDIntSet(v),false);
            auto found = umap.find(&downState);
            MDDNode* child = nullptr;
            if (found == umap.end()){
               child = _nf->makeNode(downState,upState,combinedState,x[i]->size(),i+1,(int)layers[i+1].size());
               umap.insert({child->key(),child});
               layers[i+1].push_back(child,mem);
            }  else {
               child = found->second;
            }
            parent->addArc(mem,child, v);
         } else {
            MDDState sinkState(sink->getDownState());
            _sf->createStateDown(downState, parent->pack(), i, x[i], MDDIntSet(v),false);
            if (sink->getNumParents() == 0) {
               sinkState.copyState(downState);
            } else {
               if (sinkState != downState) {
                  _mddspec.relaxationDown(sinkState, downState);
                  sinkState.relax();
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

   auto rootDownState = _mddspec.rootState(trail,mem);
   MDDState rootUpState(trail,&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSizeUp()),Up);
   MDDState rootCombinedState(trail,&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSizeCombined()),Bi);
   root = _nf->makeNode(rootDownState,rootUpState,rootCombinedState,x[0]->size(),0,0);
   layers[0].push_back(root,mem);
   _mddspec.updateNode(rootCombinedState,MDDPack(rootDownState,rootUpState,rootCombinedState));

   MDDState sinkDownState(trail,&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSizeDown()),Up);
   auto sinkUpState = _mddspec.sinkState(trail,mem);
   MDDState sinkCombinedState(trail,&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSizeCombined()),Bi);
   sink = _nf->makeNode(sinkDownState,sinkUpState,sinkCombinedState,0,(int)numVariables,0);
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
   std::string colors[2] = {"green","red"};
   std::cout << "digraph MDD {" << std::endl;
   std::cout << " node [style=filled gradientangle=270];\n"; 
   for(auto l = 0u; l <= numVariables; l++){
      for(auto i = 0u; i < layers[l].size(); i++){
         if(!layers[l][i]->isActive()) continue;
         auto n  = layers[l][i];
         auto nc = layers[l][i]->getNumChildren();
         const auto& ch = layers[l][i]->getChildren();
         bool dR = n->getDownState().isRelaxed();
         bool uR = n->getUpState().isRelaxed();         
         std::cout << "\"" << *(layers[l][i]) << "\" [color=\"" << colors[dR] << ":" << colors[uR] << "\"];\n";
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

