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

struct MDDEdge;
class MDD;

class MDDNode {
    
public:
    MDDNode();
    MDDNode(CPSolver::Ptr cp, Trailer::Ptr t, MDD* mdd, int layer, int id);
    MDDNode(CPSolver::Ptr cp, Trailer::Ptr t, var<int>::Ptr var, std::vector<int>* state, MDD* mdd, int layer, int id);

    void setIsSink(bool isSink);
    bool getIsSink();
    void setIsSource(bool isSource);
    bool getIsSource();
    //void setNumChildren(int numChildren);
    std::vector<MDDEdge*> getChildren();
    std::vector<MDDEdge*> getParents();
    int getNumChildren();
    int getNumParents();

    void remove();
    void addArc(MDDNode* child, int v);
    void removeParent(int parent);
    void removeChild(int child);
    void trim();

    std::vector<int>* getState() {return state;}
    bool contains(int v);
    bool isActive(){return _active;};

    int getLayer() {return layer;}
    int getPosition() {return pos;}
    void setPosition(int pos) {this->pos = pos;}
    int getID() {return id;}
private:
    void setActive(bool b) {_active = b;};
    trail<bool> _active;
    Trailer::Ptr trail;
    var<int>::Ptr var;
    CPSolver::Ptr cp;
    MDD*          mdd;
    bool isSink;
    bool isSource;
    int pos;
    const int id;
    const int layer;
    ::trail<int> numChildren;
    ::trail<int> numParents;
    std::vector<MDDEdge*> children;
    std::vector<MDDEdge*> parents;
    
    std::vector<int>* state;
};

class MDDEdge {
public:
    MDDEdge(MDDNode* parent, MDDNode* child, int value, int childPosition, int parentPosition) : value(value), parent(parent), child(child), childPosition(childPosition), parentPosition(parentPosition){};
    void remove() {
        this->parent->removeChild(this->childPosition);
        this->child->removeParent(this->parentPosition);
    }
    int getValue() { return value; }
    int getParentPosition() {return parentPosition;}
    int getChildPosition() {return childPosition;}
    void setParentPosition(int pos) {parentPosition = pos;}
    void setChildPosition(int pos) {childPosition = pos;}
    MDDNode* getChild() {return child;}
    MDDNode* getParent() {return parent;}
private:
    int value;
    int childPosition;
    int parentPosition;
    MDDNode* parent;
    MDDNode* child;
};


#endif /* MDDSTATE_HPP_ */

