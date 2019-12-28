//
//  mddnode.cpp
//  minicpp
//
//  Created by Waldy on 10/3/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

#include "mddnode.hpp"

MDDNode::MDDNode(CPSolver::Ptr cp, Trailer::Ptr t, MDD* mdd, int layer, int id): trail(t), numChildren(t,0), numParents(t,0), _active(t, true), mdd(mdd), layer(layer), pos(id), id(id){
    isSink = false;
    isSource = false;
}
MDDNode::MDDNode(CPSolver::Ptr cp, Trailer::Ptr t, ::var<int>::Ptr var, std::vector<int>* state, MDD* mdd, int layer, int id): trail(t), numChildren(t,0), numParents(t,0), var(var), state(state), _active(t, true), mdd(mdd), layer(layer), pos(id), id(id){
    isSink = false;
    isSource = false;
}

void MDDNode::setIsSink(bool isSink){ this->isSink = isSink; }
bool MDDNode::getIsSink(){ return this->isSink; }
void MDDNode::setIsSource(bool isSource){ this->isSource = isSource; }
bool MDDNode::getIsSource(){ return this->isSource; }
//void MDDNode::setNumChildren(int numChildren){ this->numChildren = numChildren; }
std::vector<MDDEdge*> MDDNode::getChildren(){ return children; }
std::vector<MDDEdge*> MDDNode::getParents(){ return parents; }
int MDDNode::getNumChildren(){ return numChildren.value(); }
int MDDNode::getNumParents(){ return numParents.value(); }

/*
 MDDNode::remove() removes all edges connected to MDDNode and de-activates node.
*/
void MDDNode::remove(){
    this->setActive(false);

    for(int i = numChildren.value() - 1; i >= 0 ; i--){
        this->children[i]->remove();
    }
    
    for(int i = numParents.value() - 1; i >=0 ; i--){
        this->parents[i]->remove();
    }
    
}

/*
 MDDNode::removeChild(int child) removes child arc by index.
 */
void MDDNode::removeChild(int arc){
    mdd->removeSupport(this->layer, children[arc]->getValue());
    
    int lastpos = numChildren.value() - 1;
    MDDEdge* temp = children[lastpos];
    children[arc]->setChildPosition(lastpos);
    temp->setChildPosition(arc);
    children[lastpos] = children[arc];
    children[arc] = temp;
    numChildren = numChildren.value() - 1;

    if(numChildren.value() < 1 && isSource) failNow();
    
    
    if(numChildren.value() < 1 && isActive())
        mdd->scheduleRemoval(this);
}

/*
 MDDNode::removeParent(int parent) removes parent arc by index.
 */
void MDDNode::removeParent(int arc){
    
    int lastpos = numParents.value() - 1;
    MDDEdge* temp = parents[lastpos];
    parents[arc]->setParentPosition(lastpos);
    temp->setParentPosition(arc);
    parents[lastpos] = parents[arc];
    parents[arc] = temp;
    
    numParents = numParents.value() - 1;
    
    
    if(numParents.value() < 1 && isSink) failNow();
    
    if(numParents.value() < 1 && isActive())
        mdd->scheduleRemoval(this);
}

/*
 MDDNode::addArc(MDDNode* child, MDDNode* parent, int v){}
*/

void MDDNode::addArc(MDDNode* child, int v){
    MDDEdge* e = new MDDEdge(this, child, v, this->numChildren.value(), child->numParents.value());

    this->children.push_back(e);
    this->numChildren = this->numChildren.value() + 1;
    
    child->parents.push_back(e);
    child->numParents = child->numParents.value() + 1;
    
}

/*
 MDDNode::contains(int v) checks if value v is represented by an arc by node.
 */
bool MDDNode::contains(int v){
    for(int i = 0; i < this->numChildren.value(); i++){
        if(children[i]->getValue() == v) return true;
    }
    return false;
}

/*
 MDDNode::trim() removes all arcs with values not in the domain of var.
 */
void MDDNode::trim(){
    for(int i = this->numChildren.value() - 1; i >= 0 ; i--){
        if(!var->contains(children[i]->getValue())){
            children[i]->remove();
        }
    }
}

