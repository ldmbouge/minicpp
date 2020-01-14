//
//  mddnode.cpp
//  minicpp
//
//  Created by Waldy on 10/3/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

#include "mddnode.hpp"

MDDNode::MDDNode(Storage::Ptr mem, Trailer::Ptr t,int layer, int id)
   : _active(t, true),
     pos(id),
     layer(layer),
     children(t,mem,2),
     parents(t,mem,2)
{}

MDDNode::MDDNode(Storage::Ptr mem, Trailer::Ptr t,const MDDState& state,int dsz,int layer, int id)
   : _active(t, true),
     pos(id),
     layer(layer),
     children(t,mem,dsz),
     parents(t,mem,dsz),
     state(state)
{}

const TVec<MDDEdge::Ptr>& MDDNode::getChildren()  { return children; }

/*
  MDDNode::remove() removes all edges connected to MDDNode and de-activates node.
*/
void MDDNode::remove(MDD* mdd)
{
   this->setActive(false);
   for(int i = (int)children.size() - 1; i >= 0 ; i--)
      children.get(i)->remove(mdd);
   for(int i = (int)parents.size() - 1; i >=0 ; i--)
      parents.get(i)->remove(mdd);
}

/*
  MDDNode::removeChild(int child) removes child arc by index.
*/
void MDDNode::removeChild(MDD* mdd,int arc)
{
   mdd->removeSupport(this->layer, children.get(arc)->getValue());

   auto sz = children.remove(arc);
   if (sz) children.get(arc)->setChildPosition(children.getTrail(),arc);
   
   if(sz < 1 && layer == 0)
      failNow();
   if(sz < 1 && _active)
      mdd->scheduleRemoval(this);
}

/*
  MDDNode::removeParent(int parent) removes parent arc by index.
*/
void MDDNode::removeParent(MDD* mdd,int arc)
{
   auto sz = parents.remove(arc);
   if (sz) parents.get(arc)->setParentPosition(parents.getTrail(),arc);
   
   if(sz < 1 && layer==mdd->nbLayers())
      failNow();
   if(sz < 1 && _active)
      mdd->scheduleRemoval(this);
}

/*
  MDDNode::addArc(MDDNode* child, MDDNode* parent, int v){}
*/

void MDDNode::addArc(Storage::Ptr& mem,MDDNode* child, int v)
{
   MDDEdge::Ptr e = new (mem) MDDEdge(this, child, v,
                                      (unsigned short)children.size(),
                                      (unsigned short)child->parents.size());
   children.push_back(e);
   child->parents.push_back(e);
}

/*
  MDDNode::contains(int v) checks if value v is represented by an arc by node.
*/
bool MDDNode::contains(int v)
{
   for(int i = 0; i < this->children.size(); i++){
      if(children.get(i)->getValue() == v) return true;
   }
   return false;
}

/*
  MDDNode::trim() removes all arcs with values not in the domain of var.
*/
void MDDNode::trim(MDD* mdd,var<int>::Ptr x)
{
   if (_active) {
      for(int i = (int)children.size() - 1; i >= 0 ; i--){
         if(!x->contains(children.get(i)->getValue())){
            children.get(i)->remove(mdd);
         }
      }
   }
}

void MDDEdge::remove(MDD* mdd)
{
   parent->removeChild(mdd,childPosition);
   child->removeParent(mdd,parentPosition);
}
