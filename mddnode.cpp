//
//  mddnode.cpp
//  minicpp
//
//  Created by Waldy on 10/3/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

#include "mddnode.hpp"

MDDNode::MDDNode(CPSolver::Ptr cp, Trailer::Ptr t,int layer, int id)
   : numChildren(t,0), numParents(t,0),
     _active(t, true),
     layer(layer), pos(id)
{}

MDDNode::MDDNode(CPSolver::Ptr cp, Trailer::Ptr t,const MDDState& state,int layer, int id)
   : numChildren(t,0), numParents(t,0), state(state),
     _active(t, true),
     layer(layer), pos(id)
{}
const std::vector<MDDEdge::Ptr>& MDDNode::getChildren()  { return children; }

/*
 MDDNode::remove() removes all edges connected to MDDNode and de-activates node.
*/
void MDDNode::remove(MDD* mdd)
{
    this->setActive(false);
    for(int i = numChildren - 1; i >= 0 ; i--)
        children[i]->remove(mdd);
    for(int i = numParents - 1; i >=0 ; i--)
        parents[i]->remove(mdd);
}

/*
 MDDNode::removeChild(int child) removes child arc by index.
 */
void MDDNode::removeChild(MDD* mdd,int arc)
{
    mdd->removeSupport(this->layer, children[arc]->getValue());

    int lastpos = numChildren - 1;
    MDDEdge::Ptr temp = children[lastpos];
    children[arc]->setChildPosition(lastpos);
    temp->setChildPosition(arc);
    children[lastpos] = children[arc];
    children[arc] = temp;
    numChildren = numChildren - 1;

    if(numChildren < 1 && layer == 0)
       failNow();

    if(numChildren < 1 && _active)
        mdd->scheduleRemoval(this);
}

/*
 MDDNode::removeParent(int parent) removes parent arc by index.
 */
void MDDNode::removeParent(MDD* mdd,int arc){

    int lastpos = numParents - 1;
    MDDEdge::Ptr temp = parents[lastpos];
    parents[arc]->setParentPosition(lastpos);
    temp->setParentPosition(arc);
    parents[lastpos] = parents[arc];
    parents[arc] = temp;

    numParents = numParents - 1;

    if(numParents < 1 && layer==mdd->nbLayers())
       failNow();
    if(numParents < 1 && _active)
        mdd->scheduleRemoval(this);
}

/*
 MDDNode::addArc(MDDNode* child, MDDNode* parent, int v){}
*/

void MDDNode::addArc(Storage::Ptr& mem,MDDNode* child, int v)
{
   MDDEdge::Ptr e = new (mem) MDDEdge(this, child, v, numChildren, child->numParents);
   children.push_back(e);
   numChildren = numChildren + 1;
   child->parents.push_back(e);
   child->numParents = child->numParents + 1;
}

/*
 MDDNode::contains(int v) checks if value v is represented by an arc by node.
 */
bool MDDNode::contains(int v)
{
   for(int i = 0; i < this->numChildren; i++){
      if(children[i]->getValue() == v) return true;
   }
   return false;
}

/*
 MDDNode::trim() removes all arcs with values not in the domain of var.
 */
void MDDNode::trim(MDD* mdd,var<int>::Ptr x)
{
   if (_active) {
      for(int i = this->numChildren - 1; i >= 0 ; i--){
         if(!x->contains(children[i]->getValue())){
            children[i]->remove(mdd);
         }
      }
   }
}

void MDDEdge::remove(MDD* mdd)
{
   parent->removeChild(mdd,childPosition);
   child->removeParent(mdd,parentPosition);
}
