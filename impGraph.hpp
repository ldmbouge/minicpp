#ifndef __IMPGRAPH_H
#define __IMPGRAPH_H

#include <vector>
#include "literal.hpp"
#include "handle.hpp"
#include "hashtable.hpp"
#include "store.hpp"

class IGNode {
    Literal* _l;
    std::vector<Literal*> _reason;
public:
    IGNode(Literal* l) : _l(l) {}
    bool operator==(const IGNode& other) { return _l == other._l;}
    void addToReason(Literal* lp) { _reason.push_back(lp);}
    void explain() {}
};

class Explainer;

class ImpGraph {
    typedef Hashtable<unsigned int, IGNode, std::hash<unsigned int>, std::equal_to<unsigned int>> IGHashTable;
    Explainer* _exp;
    IGHashTable _ht;
    // IGNode*  _failNode;
    Literal* findLit(Literal l);  // TODO
public:
    typedef handle_ptr<ImpGraph> Ptr;
    ImpGraph(Explainer* exp, Pool::Ptr pool) : _exp(exp), _ht(pool, 100) {}
    void addNode(Literal* lp);
    void firstUIP(){}
};
















#endif