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

class MDDNode;
class MDD;

class MDDEdge {
public:
   typedef handle_ptr<MDDEdge> Ptr;
   MDDEdge(MDDNode* parent, MDDNode* child, int value, int childPosition, int parentPosition)
      : value(value), parent(parent), child(child), childPosition(childPosition), parentPosition(parentPosition)
   {}
   void remove();
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


class MDDNode {    
public:
   MDDNode();
   MDDNode(CPSolver::Ptr cp, Trailer::Ptr t,MDD* mdd, int layer, int id);
   MDDNode(CPSolver::Ptr cp, Trailer::Ptr t,MDDState::Ptr state, MDD* mdd, int layer, int id);

   void setIsSink(bool isSink)     { this->isSink = isSink;}
   bool getIsSink() const          { return isSink;}
   void setIsSource(bool isSource) { this->isSource = isSource;}
   bool getIsSource() const        { return isSource;}
   //void setNumChildren(int numChildren);
   std::vector<MDDEdge::Ptr> getChildren();
   std::vector<MDDEdge::Ptr> getParents();
   int getNumChildren() const { return numChildren;}
   int getNumParents() const  { return numParents;}

   void remove();
   void addArc(Storage::Ptr& mem,MDDNode* child, int v);
   void removeParent(int parent);
   void removeChild(int child);
   void trim(var<int>::Ptr x);

   const MDDState::Ptr& getState() { return state;}
   bool contains(int v);
   int getLayer() const      {return layer;}
   int getPosition() const   { return pos;}
   void setPosition(int pos) { this->pos = pos;}
   bool isActive() const     { return _active;}
private:
   void setActive(bool b) { _active = b;}
   trail<bool> _active;
   MDD*          mdd;
   bool isSink;
   bool isSource;
   int pos;
   const int layer;
   trail<int> numChildren;
   trail<int> numParents;
   std::vector<MDDEdge::Ptr> children;
   std::vector<MDDEdge::Ptr> parents;   
   MDDState::Ptr state;
};

#endif /* MDDSTATE_HPP_ */

