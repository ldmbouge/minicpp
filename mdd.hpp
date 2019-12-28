//
//  mdd.hpp
//  minicpp
//
//  Created by Waldy on 10/6/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

#ifndef mdd_hpp
#define mdd_hpp


#include "mddnode.hpp"
class MDDNode;
class MDD{
public:
    MDD(CPSolver::Ptr cp);
    MDD(CPSolver::Ptr cp, Factory::Veci intVarArray, bool reduced);
    void saveGraph();
    void post();
    MDDState getState() { return _mddspec; }
    void setState(MDDState s){ _mddspec = s; }
    void trimLayer(int layer);
    void scheduleRemoval(MDDNode*);
    void startRemoval();
    void addSupport(int layer, int value);
    void removeSupport(int layer, int value);
    void removeNode(MDDNode* node);
private:
    void buildDiagram();
    Trailer::Ptr trail = nullptr;
    CPSolver::Ptr cp = nullptr;

    std::vector<var<int>::Ptr> x;
    std::vector<std::vector<MDDNode*>> layers;
    std::vector<::trail<int>> layerSize;
    std::deque<MDDNode*>* queue;
    std::vector<std::vector<::trail<int>>> supports;
    std::vector<int> oft;
    bool reduced = false;
    unsigned long numVariables;
    bool maximize;
    MDDNode* root = nullptr;
    MDDNode* sink = nullptr;
    var<int>::Ptr objective = nullptr;
    MDDState _mddspec;
};

class MDDTrim : public Constraint { //Trims layer when D(_var) changes.
    var<int>::Ptr _var;
    MDD* _mdd;
    int _layer;
public:
    MDDTrim(CPSolver::Ptr cp, var<int>::Ptr var, MDD* mdd, int layer): Constraint(cp), _var(var), _mdd(mdd), _layer(layer){};
    void post() override {};
    void propagate() override { _mdd->trimLayer(_layer);};
};

class MDDRemoval : public Constraint { //Removes nodes in queue.
    var<int>::Ptr _var;
    MDD* _mdd;
public:
    MDDRemoval(CPSolver::Ptr cp, var<int>::Ptr var, MDD* mdd): Constraint(cp), _var(var), _mdd(mdd){};
    void post() override {};
    void propagate() override { _mdd->startRemoval();};
};


#endif /* mdd_hpp */
