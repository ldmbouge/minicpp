//
//  mddnode.cpp
//  minicpp
//
//  Created by Waldy on 10/3/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

#include "mddnode.hpp"

MDDNode* MDDNodeFactory::makeNode(const MDDState& ms,int val,int domSize,int layer,int layerSize)
{
   return new (_mem) MDDNode(_lastID++,_mem,_trailer,ms.clone(_mem),domSize,layer,layerSize);
}

void MDDNodeFactory::returnNode(MDDNode* n)
{}


MDDNode::MDDNode(int nid,Storage::Ptr mem, Trailer::Ptr t,unsigned layer, int id)
   : pos(id),
     _nid(nid),
     _inQueue(t,None),
     _active(true),
     layer(layer),
     children(t,mem,2),
     parents(t,mem,2)
{}

MDDNode::MDDNode(int nid,Storage::Ptr mem, Trailer::Ptr t,const MDDState& state,
		 int dsz,unsigned layer, int id)
   : pos(id),
     _nid(nid),
     _inQueue(t,None),
     _active(true),
     layer(layer),
     children(t,mem,std::max(dsz,1)),
     parents(t,mem,std::max(dsz,1)),
     state(state)
{}


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
}

void MDDNode::hookChild(MDDEdge::Ptr arc,Storage::Ptr mem)
{
   auto at = parents.size();
   parents.push_back(arc,mem);
   arc->setParentPosition(parents.getTrail(),at);  // arc needs to know where it is.
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
   MDDEdge::Ptr e = new (mem) MDDEdge(this, child, v,
                                      (unsigned short)children.size(),
                                      (unsigned int)child->parents.size());
   children.push_back(e,mem);
   child->parents.push_back(e,mem);
}

double MDDSpec::splitPriority(const MDDNode& n) const
{
   double ttl = 0.0;
   for(const auto& sf : _onSplit)
      ttl += sf(n);
   return ttl;
}

void MDDEdge::remove(MDD* mdd)
{
   parent->removeChild(mdd,value,childPosition);
   child->removeParent(mdd,value,parentPosition);
}
