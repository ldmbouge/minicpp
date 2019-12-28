//
//  mddstate.hpp
//  minicpp
//
//  Created by Waldy on 10/28/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

#ifndef mddstate_hpp
#define mddstate_hpp

#include "intvar.hpp"
#include <set>


/*
enum Prop { vec_int , state_int };


union RType {
    void* vp;
    int i;
};

class MDDPropetry {
public:
    MDDPropetry() {}
    MDDPropetry(int v){ this->value.i = v; type = state_int;}
    MDDPropetry(std::vector<int> vp){
        std::vector<int>* v = new std::vector<int>(vp);
        this->value.vp = v;
        type = vec_int;
    }
    MDDPropetry(const MDDPropetry& p){
        if(p.type == vec_int){
            auto vp = static_cast<std::vector<int>*>(p.value.vp);
            std::vector<int>* v = new std::vector<int>(*vp);
            this->value.vp = v;
            this->type = vec_int;
        }
        if(p.type == state_int){
            this->value.i = p.value.i;
            this->type = state_int;
        }
    }

    RType getValue() { return value; }
    void setValue(int v) { this->value.i = v; }
private:
    RType value;
    Prop type;
};
*/


class MDDState {
public:
    MDDState():arcLambda(nullptr){ this->baseState = new std::vector<int>();};

    void addState(int s){ baseState->push_back(s); };
    void addArc(std::function<bool(std::vector<int>*, var<int>::Ptr, int)> a);
    void addTransistion(std::function<int(std::vector<int>*, var<int>::Ptr, int)> t);
    void addRelaxation(std::function<int(std::vector<int>*, std::vector<int>*)>);
    void addSimilarity(std::function<double(std::vector<int>*, std::vector<int>*)>);
    
    std::vector<int>* createState(std::vector<int>* state, var<int>::Ptr var, int v);
    
    std::function<bool(std::vector<int>*, var<int>::Ptr, int)> getArcs();
    std::vector<std::function<int(std::vector<int>*, var<int>::Ptr, int)>> getTransistions();
    std::vector<std::function<int(std::vector<int>*,std::vector<int>*)>> getRelaxations();
    std::vector<std::function<double(std::vector<int>*,std::vector<int>*)>> getSimilarities();
    
    void append(Factory::Veci x);
    std::vector<var<int>::Ptr> getVars(){ return x; }
    std::vector<int>* baseState;
private:
    std::vector<var<int>::Ptr> x;
    std::function<bool(std::vector<int>*, var<int>::Ptr, int)> arcLambda;
    std::vector<std::function<int(std::vector<int>*, var<int>::Ptr, int)>> transistionLambdas;
    std::vector<std::function<int(std::vector<int>*, std::vector<int>*)>> relaxationLambdas;
    std::vector<std::function<double(std::vector<int>*, std::vector<int>*)>> similarityLambdas;
};

inline bool isMember(int v, std::set<int> s) { return (s.find(v) != s.end());}
inline bool isMember(var<int>::Ptr v, std::set<var<int>::Ptr> s) { return (s.find(v) != s.end());}


namespace Factory {

    inline void amongMDD(MDDState& mdd, Factory::Veci x, int lb, int ub, std::set<int> values){
        int stateSize = (int) mdd.baseState->size();
        mdd.append(x);

        std::set<var<int>::Ptr> vars; for(auto e : x) vars.insert(e);

        int minC = 0 + stateSize, maxC = 1 + stateSize, rem = 2 + stateSize; //state idx
        
        mdd.addState(0); mdd.addState(0); mdd.addState((int) x.size());
        
        auto a = [=] (std::vector<int>* p, var<int>::Ptr var, int val) -> bool {
            return (p->at(minC) + isMember(val, values) <= ub) && ((p->at(maxC) + isMember(val, values) +  p->at(rem) - 1) >= lb);
        };
        
        mdd.addArc(a);
        
        mdd.addTransistion([=] (std::vector<int>* p, var<int>::Ptr var, int val) -> int { return p->at(minC) + isMember(val, values);});
        mdd.addTransistion([=] (std::vector<int>* p, var<int>::Ptr var, int val) -> int { return p->at(maxC) + isMember(val, values);});
        mdd.addTransistion([=] (std::vector<int>* p, var<int>::Ptr var, int val) -> int { return p->at(rem) - 1;});
        
        mdd.addRelaxation([=] (std::vector<int>* l, std::vector<int>* r) -> int { return std::min(l->at(minC), r->at(minC));});
        mdd.addRelaxation([=] (std::vector<int>* l, std::vector<int>* r) -> int { return std::max(l->at(maxC), r->at(maxC));});
        mdd.addRelaxation([=] (std::vector<int>* l, std::vector<int>* r) -> int { return l->at(rem);});

        mdd.addSimilarity([=] (std::vector<int>* l, std::vector<int>* r) -> double { return abs(l->at(minC) - r->at(minC)); });
        mdd.addSimilarity([=] (std::vector<int>* l, std::vector<int>* r) -> double { return abs(l->at(maxC) - r->at(maxC)); });
        mdd.addSimilarity([=] (std::vector<int>* l, std::vector<int>* r) -> double { return 0; });


    }
}

#endif /* mddstate_hpp */
