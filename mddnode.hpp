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
      void restore() noexcept { *_at = _old;}
   };
   typedef handle_ptr<MDDEdge> Ptr;
   MDDEdge(MDDNode* parent, MDDNode* child, int value, unsigned short childPosition,unsigned int parentPosition)
      : value(value), parent(parent), child(child),
        childPosition(childPosition),
        parentPosition(parentPosition)
   {}
   void remove(MDD* mdd);
   int getValue() const  noexcept                       { return value; }
   unsigned short getParentPosition() const noexcept    { return parentPosition;}
   unsigned int getChildPosition() const noexcept       { return childPosition;}
   void setParentPosition(Trailer::Ptr t,unsigned int pos) noexcept {
      t->trail(new (t) TrailEntry<unsigned int>(&parentPosition));
      parentPosition = pos;
   }
   void setChildPosition(Trailer::Ptr t,unsigned short  pos) noexcept  {
      t->trail(new (t) TrailEntry<unsigned short>(&childPosition));
      childPosition = pos;
   }
   MDDNode* getChild() const noexcept   { return child;}
   MDDNode* getParent() const noexcept  { return parent;}
   void moveTo(MDDNode* n,Trailer::Ptr t,Storage::Ptr mem);
private:
   int value;
   MDDNode* parent;
   MDDNode* child;
   unsigned short childPosition;
   unsigned int parentPosition;
};

class MDDNodeFactory;
class MDDNode {
   friend class MDDNodeFactory;
   template <class U> class TrailEntry : public Entry {
      U* _at;
      U  _old;
   public:
      TrailEntry(U* ptr) : _at(ptr),_old(*ptr) {}
      void restore() noexcept { *_at = _old;}
   };
   MDDNode(int nid,Storage::Ptr mem, Trailer::Ptr t,const MDDState& state,int dsz,unsigned layer, int id);
public:
   const auto& getParents()  noexcept  { return parents;}
   const auto& getChildren() noexcept  { return children;}
   std::size_t getNumChildren() const  noexcept { return children.size();}
   std::size_t getNumParents() const   noexcept { return parents.size();}
   bool disconnected() const noexcept           { return children.size() < 1 || parents.size() < 1;}
   void clearChildren() noexcept { children.clear();}
   void clearParents()  noexcept { parents.clear();}
   void remove(MDD* mdd);
   void addArc(Storage::Ptr& mem,MDDNode* child, int v);
   void removeParent(MDD* mdd,int value,int pos);
   void removeChild(MDD* mdd,int value,int pos);
   bool unhookOutgoing(MDDEdge::Ptr arc); // returns true if the node is now child-less
   bool unhookIncoming(MDDEdge::Ptr arc); // returns true if the node is now orphaned
   void unhook(MDDEdge::Ptr arc);
   void unhookChild(MDDEdge::Ptr arc);
   void hookChild(MDDEdge::Ptr arc,Storage::Ptr mem);
   MDDState* key()            { return &state;}
   void setState(const MDDState& s,Storage::Ptr mem) {
      auto t = children.getTrail();
      state.assign(s,t,mem);
   }
   void setLayer(unsigned short l,Storage::Ptr mem) {
      auto t = children.getTrail();
      t->trail(new (t) TrailEntry<unsigned short>(&layer));
      layer = l;
   }
   const MDDState& getState() const noexcept { return state;}
   unsigned short getLayer() const noexcept  { return layer;}
   int getPosition() const noexcept          { return pos;}
   int getId() const noexcept                { return _nid;}
   void setPosition(int p,Storage::Ptr mem) {
      auto t = children.getTrail();
      t->trail(new (t) TrailEntry<int>(&pos));
      pos = p;
   }
   void clearQueue() const noexcept { _inQueue = None;}
   void enterQueue(enum Direction d) const noexcept { _inQueue = (enum Direction)(_inQueue | d);}
   void leaveQueue(enum Direction d) const noexcept { _inQueue = (enum Direction)(_inQueue & ~d);}
   bool inQueue(enum Direction d) const noexcept    { return (_inQueue & d)==d;}
   enum Direction curQueue() const noexcept { return _inQueue;}
   bool isActive() const noexcept { return _active;}
   void deactivate() {
      auto t = children.getTrail();
      t->trail(new (t) TrailEntry<bool>(&_active));
      _active = false;
   }
   void activate() {
      auto t = children.getTrail();
      t->trail(new (t) TrailEntry<bool>(&_active));
      _active = true;
      _inQueue = None;
   }
   void print(std::ostream& os) {
      os << "L[" << layer << "," << pos <<  "] " << state;
   }
   bool parentsChanged() const noexcept { return parents.changed();}
   bool childrenChanged() const noexcept { return children.changed();}
private:   
   int pos;
   int _nid;
   mutable trail<enum Direction> _inQueue;
   bool _active;
   unsigned short layer;
   TVec<MDDEdge::Ptr,unsigned short> children;
   TVec<MDDEdge::Ptr,unsigned int>    parents;
   MDDState state;                     // Direct state embedding
   friend class MDDEdge;
};


class MDDNodeFactory {
   Storage::Ptr       _mem;
   Trailer::Ptr   _trailer;
   int              _width;
   trail<int>      _lastID;
   int             _peakID;
   //int _lastID;
   TVec<MDDNode*> _pool;
public:
   MDDNodeFactory(Storage::Ptr mem,Trailer::Ptr trailer,int width);
   void setWidth(int w) noexcept { _width = w;}
   MDDNode* makeNode(const MDDState& ms,int domSize,int layer,int layerSize);
   void returnNode(MDDNode* n);
   int nbNodes() const noexcept { return _lastID;}
   int peakNodes() const noexcept { return _peakID;}
};

inline void MDDEdge::moveTo(MDDNode* n,Trailer::Ptr t,Storage::Ptr mem) 
{
   child->unhookChild(this);
   t->trail(new (t) TrailEntry<MDDNode*>(&child));
   child = n;
   child->hookChild(this,mem);
   assert(n->isActive());
   assert(getParent()->getChildren().get(getChildPosition()).get() == this);
   assert(getChild()->getParents().get(getParentPosition()).get() == this);
}


inline std::ostream& operator<<(std::ostream& os,MDDNode& p)
{
   p.print(os);return os;
}

#endif /* MDDSTATE_HPP_ */
