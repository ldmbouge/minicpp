//
//  mddnode.cpp
//  minicpp
//
//  Created by Waldy on 10/3/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

#include "mddnode.hpp"

MDDNode::MDDNode(CPSolver::Ptr cp, Trailer::Ptr t, MDD* mdd, int layer, int id)
   : numChildren(t,0), numParents(t,0), _active(t, true), mdd(mdd), layer(layer), pos(id)
{
    isSink = false;
    isSource = false;
}
MDDNode::MDDNode(CPSolver::Ptr cp, Trailer::Ptr t, ::var<int>::Ptr var, MDDState::Ptr state, MDD* mdd, int layer, int id)
   : numChildren(t,0), numParents(t,0), var(var), state(state), _active(t, true), mdd(mdd), layer(layer), pos(id)
{
    isSink = false;
    isSource = false;
}

//void MDDNode::setNumChildren(int numChildren){ this->numChildren = numChildren; }
std::vector<MDDEdge*> MDDNode::getChildren(){ return children; }
std::vector<MDDEdge*> MDDNode::getParents(){ return parents; }

/*
 MDDNode::remove() removes all edges connected to MDDNode and de-activates node.
*/
void MDDNode::remove()
{
    this->setActive(false);
    for(int i = numChildren - 1; i >= 0 ; i--)
        children[i]->remove();        
    for(int i = numParents - 1; i >=0 ; i--)
        parents[i]->remove();       
}

/*
 MDDNode::removeChild(int child) removes child arc by index.
 */
void MDDNode::removeChild(int arc){
    mdd->removeSupport(this->layer, children[arc]->getValue());
    
    int lastpos = numChildren - 1;
    MDDEdge* temp = children[lastpos];
    children[arc]->setChildPosition(lastpos);
    temp->setChildPosition(arc);
    children[lastpos] = children[arc];
    children[arc] = temp;
    numChildren = numChildren - 1;

    if(numChildren < 1 && isSource)
       failNow();
        
    if(numChildren < 1 && isActive())
        mdd->scheduleRemoval(this);
}

/*
 MDDNode::removeParent(int parent) removes parent arc by index.
 */
void MDDNode::removeParent(int arc){
    
    int lastpos = numParents - 1;
    MDDEdge* temp = parents[lastpos];
    parents[arc]->setParentPosition(lastpos);
    temp->setParentPosition(arc);
    parents[lastpos] = parents[arc];
    parents[arc] = temp;
    
    numParents = numParents - 1;
    
    
    if(numParents < 1 && isSink) failNow();
    
    if(numParents < 1 && isActive())
        mdd->scheduleRemoval(this);
}

/*
 MDDNode::addArc(MDDNode* child, MDDNode* parent, int v){}
*/

void MDDNode::addArc(MDDNode* child, int v)
{
   MDDEdge* e = new MDDEdge(this, child, v, this->numChildren, child->numParents);
   this->children.push_back(e);
   this->numChildren = this->numChildren + 1;   
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
void MDDNode::trim()
{
   for(int i = this->numChildren - 1; i >= 0 ; i--){
      if(!var->contains(children[i]->getValue())){
         children[i]->remove();
      }
   }
}

