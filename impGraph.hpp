#ifndef __IMPGRAPH_H
#define __IMPGRAPH_H

#include <vector>
#include "literal.hpp"
#include "handle.hpp"

class ImpGraph {
    struct IGNode {
        Literal _l;
        std::vector<Literal*> _exp;
        void explain() {};
    };
    IGNode _root;
public:
    typedef handle_ptr<ImpGraph> Ptr;
    void build() {};
    void firstUIP(){};
};
















#endif