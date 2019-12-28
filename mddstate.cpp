//
//  mddstate.cpp
//  minicpp
//
//  Created by Waldy on 10/28/19.
//  Copyright © 2019 Waldy. All rights reserved.
//

#include "mddstate.hpp"

void MDDState::append(Factory::Veci y){
    int size = (int) x.size();
    for(int i = 0; i < y.size(); i++){
        if(size < 1 || std::find(x.begin(), x.end(), y[i]) == x.end()){
            var<int>::Ptr temp = y[i];
            x.push_back(temp);
        }
    }
    std::cout << "size of x: " << x.size() << std::endl;
}

void MDDState::addArc(std::function<bool(std::vector<int>*, var<int>::Ptr, int)> a){
    auto b = arcLambda;
    if(arcLambda == nullptr){
        arcLambda = a;
    }
    else{
        arcLambda = [=] (std::vector<int>* p, var<int>::Ptr var, int val) -> bool {
            return a(p, var, val) && b(p, var, val);
        };
    }
}
void MDDState::addTransistion(std::function<int(std::vector<int>*, var<int>::Ptr, int)> t){
    transistionLambdas.push_back(t);

}
void MDDState::addRelaxation(std::function<int(std::vector<int>*,std::vector<int>*)> r){
    relaxationLambdas.push_back(r);

}
void MDDState::addSimilarity(std::function<double(std::vector<int>*,std::vector<int>*)> s){
    similarityLambdas.push_back(s);

}

std::function<bool(std::vector<int>*, var<int>::Ptr, int)> MDDState::getArcs(){
    return arcLambda;
}

std::vector<std::function<int(std::vector<int>*, var<int>::Ptr, int)>> MDDState::getTransistions(){
    return transistionLambdas;
}

std::vector<std::function<int(std::vector<int>*,std::vector<int>*)>> MDDState::getRelaxations(){
    return relaxationLambdas;
}
std::vector<std::function<double(std::vector<int>*, std::vector<int>*)>> MDDState::getSimilarities(){
    return similarityLambdas;
}

std::vector<int>* MDDState::createState(std::vector<int>* parent, var<int>::Ptr var, int v){
    if(arcLambda(parent, var, v)){
        std::vector<int>* result = new std::vector<int>(parent->size());
        for(int i = 0; i < parent->size(); i++){
            (*result)[i] = transistionLambdas[i](parent, var, v);
        }
        return result;
    }
    return nullptr;
}

