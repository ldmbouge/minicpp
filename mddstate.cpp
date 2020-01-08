//
//  mddstate.cpp
//  minicpp
//
//  Created by Waldy on 10/28/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

#include "mddstate.hpp"
#include <algorithm>

void MDDSpec::append(const Factory::Veci& y) {
    int size = (int) x.size();
    for(int i = 0; i < y.size(); i++){
       if(size < 1 || std::find(x.cbegin(), x.cend(), y[i]) == x.cend())
          x.push_back(y[i]);       
    }
    std::cout << "size of x: " << x.size() << std::endl;
}
void MDDSpec::addState(int s)
{
   baseState->addState(s);
   transistionLambdas.push_back(nullptr);
   relaxationLambdas.push_back(nullptr);
   similarityLambdas.push_back(nullptr);
}
void MDDSpec::addStates(int from, int to, std::function<int(int)> clo)
{
   for(int i = from; i <= to; i++)
      addState(clo(i));
}
void MDDSpec::addStates(std::initializer_list<int> inputs)
{
   for(auto& v : inputs)
      addState(v);
}
void MDDSpec::addArc(std::function<bool(const MDDState::Ptr&, var<int>::Ptr, int)> a){
    auto b = arcLambda;
    if(arcLambda == nullptr) {
       arcLambda = a;
    } else {
       arcLambda = [=] (const MDDState::Ptr& p, var<int>::Ptr var, int val) -> bool {
                      return a(p, var, val) && b(p, var, val);
      };
    }
}
void MDDSpec::addTransition(int p,std::function<int(const MDDState::Ptr&, var<int>::Ptr, int)> t)
{
    transistionLambdas[p] = t;
}
void MDDSpec::addRelaxation(int p,std::function<int(MDDState::Ptr,MDDState::Ptr)> r)
{
    relaxationLambdas[p] = r;
}
void MDDSpec::addSimilarity(int p,std::function<double(MDDState::Ptr,MDDState::Ptr)> s)
{
    similarityLambdas[p] = s;
}
void MDDSpec::addTransitions(lambdaMap& map)
{
     for(auto& kv : map)
        transistionLambdas[kv.first] = kv.second;
}
MDDState::Ptr MDDSpec::createState(Storage::Ptr& mem,const MDDState::Ptr& parent, var<int>::Ptr var, int v){
    if(arcLambda(parent, var, v)){
       auto size = parent->size();
       MDDState::Ptr result = new (mem) MDDState(size);
       for(int i = 0; i < size; i++) 
          result->set(i,transistionLambdas[i](parent,var,v));
       result->hash();
       return result;
    }
    return nullptr;
}

std::pair<int,int> domRange(const Factory::Veci& vars)
{
   std::pair<int,int> udom;
   udom.first = INT_MAX;
   udom.second = -INT_MAX;
   for(auto& x : vars){
      udom.first = (udom.first > x->min()) ? x->min() : udom.first;
      udom.second = (udom.second < x->max()) ? x->max() : udom.second;
   }
   return udom;
}
