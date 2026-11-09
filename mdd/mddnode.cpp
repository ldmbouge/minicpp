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

#include "mddnode.hpp"
#include <algorithm>

MDDNodeFactory::MDDNodeFactory(Storage::Ptr mem,Trailer::Ptr trailer,int width)
   : _mem(mem),_trailer(trailer),_width(width),
     _lastID(trailer,0),
     _peakID(0),
     _pool(trailer,mem,2048)
{}


std::ostream& operator<<(std::ostream& os,enum Direction d)
{
   switch(d) {
      case Down: return os << "Down";
      case Up:   return os << "Up";
      case Bi:   return os << "Bi";
      case None: return os << "None";
      default: return os;
   }
}

MDDNode* MDDNodeFactory::makeNode(const MDDState& down,const MDDState& up,const MDDState& combined,int domSize,int layer,int layerSize)
{
   if (_pool.size() > 0) {
      MDDNode* n = _pool.pop_back();
      n->setDownState(down,_mem);
      n->setUpState(up,_mem);
      n->setCombinedState(combined,_mem);
      n->setPosition(layerSize,_mem);
      n->setLayer(layer,_mem);
      n->activate();
      return n;
   } else {
      MDDNode* retVal = new (_mem) MDDNode(_lastID++,_mem,_trailer,
                                           down.clone(_trailer,_mem),
                                           up.clone(_trailer,_mem),
                                           combined.clone(_trailer,_mem),
                                           domSize,layer,layerSize);
      _peakID = std::max(_peakID,_lastID.value());
      return retVal;
   }
}

void MDDNodeFactory::returnNode(MDDNode* n)
{
   _pool.push_back(n,_mem);
}

MDDNode::MDDNode(int nid,Storage::Ptr mem, Trailer::Ptr t,const MDDState& down, const MDDState& up, const MDDState& combined, 
		 int dsz,unsigned layer, int id)
   : pos(id),
     _nid(nid),
     _inQueue(t,None),
     _active(true),
     layer(layer),
     children(t,mem,std::max(dsz,1)),
     parents(t,mem,std::max(dsz,1)),
     downState(down),
     upState(up),
     combinedState(combined),
     _parentsChanged(t,false),
     _childrenChanged(t,false)
{
   _fq = _bq = nullptr;
}


bool MDDNode::unhookOutgoing(MDDEdge::Ptr arc)
{
   // Remove this outgoing arc from the children's list held in the receiving node. 
   assert(this == arc->getParent());
   int at = arc->getChildPosition();
   assert(at >= 0 && at < children.size());
   assert(children.get(at) == arc);
   children.remove(at);
   bool moreKids = children.size() > 0;
   if (moreKids)
      children.get(at)->setChildPosition(parents.getTrail(), at);
   _childrenChanged = true;
   return !moreKids;
}

bool MDDNode::unhookIncoming(MDDEdge::Ptr arc)
{
   // Remove this incoming arc from the parent's list held in the receiving node.
   assert(arc->getChild() == this);
   int at = arc->getParentPosition();
   assert(at >= 0 && at < (int)parents.size());
   assert(parents.get(at) == arc);
   parents.remove(at);
   bool moreParents = parents.size() > 0;
   if (moreParents)
      parents.get(at)->setParentPosition(parents.getTrail(), at);  // whoever was moved needs to know their position.
   _parentsChanged = true;
   return !moreParents;
}

void MDDNode::unhook(MDDEdge::Ptr arc)
{
   assert(this == arc->getParent());
   int at = arc->getChildPosition();
   assert(at >= 0 && at < (int)children.size());
   assert(children.get(at) == arc);
   children.remove(at);
   if (children.size() > 0)
      children.get(at)->setChildPosition(parents.getTrail(), at);
   auto childNode = arc->getChild();
   childNode->unhookChild(arc);
   _childrenChanged = true;
}

void MDDNode::unhookChild(MDDEdge::Ptr arc)
{
   assert(arc->getChild() == this);
   int at = arc->getParentPosition();
   assert(at >= 0 && at < (int)parents.size());
   assert(parents.get(at) == arc);
   parents.remove(at);
   if (parents.size() > 0)
      parents.get(at)->setParentPosition(parents.getTrail(), at);  // whoever was moved needs to know their position.
   _parentsChanged = true;
}

void MDDNode::hookChild(MDDEdge::Ptr arc,Storage::Ptr mem)
{
   auto at = parents.size();
   parents.push_back(arc,mem);
   arc->setParentPosition(parents.getTrail(),at);  // arc needs to know where it is.
   _parentsChanged = true;
}


/*
  MDDNode::remove() removes all edges connected to MDDNode and de-activates node.
*/
void MDDNode::remove(MDD* mdd)
{
   assert(!isActive());
   int layer = (int)getLayer();
   for(int i = (int)children.size() - 1; i >= 0 ; i--) {
      MDDEdge::Ptr arc = children.get(i);
      mdd->removeArc(layer,layer+1,arc.get());
      arc->remove(mdd);
   }
   for(int i = (int)parents.size() - 1; i >=0 ; i--) { // Would have to do something for up properties.
      MDDEdge::Ptr arc = parents.get(i);
      mdd->removeArc(layer-1,layer,arc.get());
      arc->remove(mdd);
   }
}

/*
  MDDNode::removeChild(int child) removes child arc by index.
*/
void MDDNode::removeChild(MDD* mdd,int value,int arc)
{
   _childrenChanged = true;
   mdd->removeSupport(layer,value);

   auto sz = children.remove(arc);
   if (sz) children.get(arc)->setChildPosition(children.getTrail(),arc);
   
   if(sz < 1 && layer == 0)
      failNow();
   if(sz < 1 && isActive())
      mdd->scheduleRemoval(this);
}

/*
  MDDNode::removeParent(int parent) removes parent arc by index.
*/
void MDDNode::removeParent(MDD* mdd,int value,int arc)
{
   _parentsChanged = true;
   auto sz = parents.remove(arc);
   if (sz) parents.get(arc)->setParentPosition(parents.getTrail(),arc);
   
   if(sz < 1 && layer==mdd->nbLayers())
      failNow();
   if(sz < 1 && isActive())
      mdd->scheduleRemoval(this);
}

/*
  MDDNode::addArc(MDDNode* child, MDDNode* parent, int v){}
*/

void MDDNode::addArc(Storage::Ptr& mem,MDDNode* child, int v)
{
   assert(isActive());
   _childrenChanged = true;
   child->setParentsChanged();
   MDDEdge::Ptr e = new (mem) MDDEdge(this, child, v,
                                      (unsigned short)children.size(),
                                      (unsigned int)child->parents.size());
   children.push_back(e,mem);
   child->parents.push_back(e,mem);
}

double MDDSpec::nodeSplitPriority(const MDDNode& n, int constraintPriority) const
{
   double ttl = 0.0;
   switch (_nodePriorityAggregateStrategy) {
      case 1:
         for(const auto& sf : _onSplitByPriorities[constraintPriority])
            ttl += sf(n);
         break;
      case 2:
         for(const auto& sf : _onSplitByPriorities[constraintPriority])
            ttl = std::min(ttl,sf(n));
         break;
      case 3:
         for(const auto& sf : _onSplitByPriorities[constraintPriority])
            ttl = std::max(ttl,sf(n));
         break;
      default:
         ttl = _onSplit[0](n);
   }
   return ttl;
}
double MDDSpec::candidateSplitPriority(const MDDState& state, void* arcs, int numArcs, int constraintPriority) const
{
   double ttl = 0.0;
   switch (_candidatePriorityAggregateStrategy) {
      case 1:
         for(const auto& sf : _candidateSplitByPriorities[constraintPriority])
            ttl += sf(state, arcs, numArcs);
         break;
      case 2:
         for(const auto& sf : _candidateSplitByPriorities[constraintPriority])
            ttl = std::min(ttl,sf(state, arcs, numArcs));
         break;
      case 3:
         for(const auto& sf : _candidateSplitByPriorities[constraintPriority])
            ttl = std::max(ttl,sf(state, arcs, numArcs));
         break;
      default:
         ttl = _candidateSplit[0](state, arcs, numArcs);
   }
   return ttl;
}

std::vector<int> MDDSpec::equivalenceValue(const MDDState& downState, const MDDState& upState, int constraintPriority)
{
   std::vector<int> equivalenceValues;
   for (auto ev : _equivalenceValueByPriorities[constraintPriority]) {
      equivalenceValues.emplace_back(ev(downState, upState));
   }
   //std::cout << equivalenceValues << std::endl;
   return equivalenceValues;
}

void MDDEdge::remove(MDD* mdd)
{
   parent->removeChild(mdd,value,childPosition);
   child->removeParent(mdd,value,parentPosition);
}
