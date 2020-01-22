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
#include "trailVec.hpp"

class MDDNode;

class MDDEdge {
public:
   template <class U> class TrailEntry : public Entry {
      U* _at;
      U  _old;
   public:
      TrailEntry(U* ptr) : _at(ptr),_old(*ptr) {}
      void restore() { *_at = _old;}
   };
   typedef handle_ptr<MDDEdge> Ptr;
   MDDEdge(MDDNode* parent, MDDNode* child, int value, unsigned short childPosition,unsigned short parentPosition)
      : value(value), parent(parent), child(child),
        childPosition(childPosition),
        parentPosition(parentPosition)
   {}
   void remove(MDD* mdd);
   int getValue() const                        { return value; }
   unsigned short getParentPosition() const    { return parentPosition;}
   unsigned int getChildPosition() const       { return childPosition;}
   void setParentPosition(Trailer::Ptr t,unsigned int pos)  {
      t->trail(new (t) TrailEntry<unsigned int>(&parentPosition));
      parentPosition = pos;
   }
   void setChildPosition(Trailer::Ptr t,unsigned short  pos)  {
      t->trail(new (t) TrailEntry<unsigned short>(&childPosition));
      childPosition = pos;
   }
   MDDNode* getChild() const       { return child;}
   MDDNode* getParent() const      { return parent;}
private:
   int value;
   MDDNode* parent;
   MDDNode* child;
   unsigned short childPosition;
   unsigned int parentPosition;
};

class MDDNode {
   template <class U> class TrailEntry : public Entry {
      U* _at;
      U  _old;
   public:
      TrailEntry(U* ptr) : _at(ptr),_old(*ptr) {}
      void restore() { *_at = _old;}
   };
public:
   MDDNode();
   MDDNode(Storage::Ptr mem, Trailer::Ptr t,int layer, int id);
   MDDNode(Storage::Ptr mem, Trailer::Ptr t,const MDDState& state,int dsz,int layer, int id);
   const auto& getChildren()           { return children;}
   std::size_t getNumChildren() const  { return children.size();}
   std::size_t getNumParents() const   { return parents.size();}
   bool disconnected() const           { return children.size() < 1 || parents.size() < 1;}
   void remove(MDD* mdd);
   void addArc(Storage::Ptr& mem,MDDNode* child, int v);
   void removeParent(MDD* mdd,int value,int pos);
   void removeChild(MDD* mdd,int value,int pos);
   void trim(MDD* mdd,var<int>::Ptr x);

   MDDState* key()            { return &state;}
   const MDDState& getState() { return state;}
   bool contains(int v);
   int getLayer() const      { return layer;}
   int getPosition() const   { return pos;}
   void setPosition(int p,Storage::Ptr mem) {
      auto t = children.getTrail();
      t->trail(new (t) TrailEntry<int>(&pos));
      pos = p;
      //this->pos = pos;
   }
   bool isActive(MDD* mdd) const { return pos < mdd->layerSize(layer);}
   void print(std::ostream& os)
   {
      os << "L[" << layer << "," << pos << "] " << state;
   }
private:
   int pos;
   const int layer;
   TVec<MDDEdge::Ptr,unsigned short> children;
   TVec<MDDEdge::Ptr,unsigned int>    parents;
   MDDState state;                     // Direct state embedding
};


inline std::ostream& operator<<(std::ostream& os,MDDNode& p)
{
   p.print(os);return os;
}

#endif /* MDDSTATE_HPP_ */
