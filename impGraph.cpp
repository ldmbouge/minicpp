#include "impGraph.hpp"
#include "handle.hpp"

Literal* ImpGraph::findLit(Literal l)
{
    return nullptr;
}

void ImpGraph::addNode(Literal* lp)
{
    // make IGNode
    IGNode n = IGNode(lp);
    // get reason for literal
    std::vector<Literal*> reason = lp->explain();
    // add reason literals to ig and add edges to them from current node
    for (Literal* lPtr : reason) {
        // check for IGNode with this literal in ht. if not there, create the IGNode
        // IGNode temp(nullptr);
        // if ( !_ht.get(litKey(*lPtr), temp) ) {
        //     IGNode temp = IGNode(lPtr);
        //     _ht.insert(litKey(*lPtr), temp);
        // }
        n.addToReason(lPtr);
    }
    // insert IGNode into ht after reason is added (since ht makes a copy of the IGNode upon insertion)
    _ht.insert(litKey(*lp), n);
}
