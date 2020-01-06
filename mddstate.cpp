//
//  mddstate.cpp
//  minicpp
//
//  Created by Waldy on 10/28/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

#include "mddstate.hpp"
#include <algorithm>

void MDDSpec::append(Factory::Veci y){
    int size = (int) x.size();
    for(int i = 0; i < y.size(); i++){
       if(size < 1 || std::find(x.begin(), x.end(), y[i]) == x.end()){
          var<int>::Ptr temp = y[i];
          x.push_back(temp);
       }
    }
    std::cout << "size of x: " << x.size() << std::endl;
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
void MDDSpec::addTransistion(std::function<int(const MDDState::Ptr&, var<int>::Ptr, int)> t){
    transistionLambdas.push_back(t);

}
void MDDSpec::addRelaxation(std::function<int(MDDState::Ptr,MDDState::Ptr)> r){
    relaxationLambdas.push_back(r);

}
void MDDSpec::addSimilarity(std::function<double(MDDState::Ptr,MDDState::Ptr)> s){
    similarityLambdas.push_back(s);

}

MDDState::Ptr MDDSpec::createState(const MDDState::Ptr& parent, var<int>::Ptr var, int v){
    if(arcLambda(parent, var, v)){
       auto size = parent->size();
       auto result = std::make_shared<MDDState>(size);
       for(int i = 0; i < size; i++) 
          result->set(i,transistionLambdas[i](parent,var,v));
       result->hash();
       return result;
    }
    return nullptr;
}

std::pair<int,int> domRange(Factory::Veci& vars)
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
