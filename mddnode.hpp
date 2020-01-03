/*
 * mddstate.hpp
 *
 *  Created on: Sep 30, 2019
 *      Author: waldy
 */

#ifndef MDDNODE_HPP_
#define MDDNODE_HPP_

#include <deque>
#include "mddstate.hpp"
#include "mdd.hpp"
#include "trailable.hpp"

struct MDDEdge;
class MDD;
 

class MDDNode {    
public:
   MDDNode();
   MDDNode(CPSolver::Ptr cp, Trailer::Ptr t, MDD* mdd, int layer, int id);
   MDDNode(CPSolver::Ptr cp, Trailer::Ptr t, var<int>::Ptr var, MDDState::Ptr state, MDD* mdd, int layer, int id);

   void setIsSink(bool isSink)     { this->isSink = isSink;}
   bool getIsSink() const          { return isSink;}
   void setIsSource(bool isSource) { this->isSource = isSource;}
   bool getIsSource() const        { return isSource;}
   //void setNumChildren(int numChildren);
   std::vector<MDDEdge*> getChildren();
   std::vector<MDDEdge*> getParents();
   int getNumChildren() const { return numChildren;}
   int getNumParents() const  { return numParents;}

   void remove();
   void addArc(MDDNode* child, int v);
   void removeParent(int parent);
   void removeChild(int child);
   void trim();

   const MDDState::Ptr& getState() { return state;}
   bool contains(int v);
   bool isActive() const     {return _active;};

   int getLayer() const      {return layer;}
   int getPosition() const   { return pos;}
   void setPosition(int pos) { this->pos = pos;}
private:
   void setActive(bool b) {_active = b;};
   trail<bool> _active;
   var<int>::Ptr var;
   MDD*          mdd;
   bool isSink;
   bool isSource;
   int pos;
   const int layer;
   trail<int> numChildren;
   trail<int> numParents;
   std::vector<MDDEdge*> children;
   std::vector<MDDEdge*> parents;   
   MDDState::Ptr state;
};

class MDDEdge {
public:
   MDDEdge(MDDNode* parent, MDDNode* child, int value, int childPosition, int parentPosition)
      : value(value), parent(parent), child(child), childPosition(childPosition), parentPosition(parentPosition)
   {}
   void remove() {
      this->parent->removeChild(this->childPosition);
      this->child->removeParent(this->parentPosition);
   }
   int getValue() const            { return value; }
   int getParentPosition() const   { return parentPosition;}
   int getChildPosition() const    { return childPosition;}
   void setParentPosition(int pos) { parentPosition = pos;}
   void setChildPosition(int pos)  { childPosition = pos;}
   MDDNode* getChild() const       { return child;}
   MDDNode* getParent() const      { return parent;}
private:
   int value;
   int childPosition;
   int parentPosition;
   MDDNode* parent;
   MDDNode* child;
};


#endif /* MDDSTATE_HPP_ */

