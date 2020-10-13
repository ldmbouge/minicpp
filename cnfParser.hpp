#ifndef __CNF_PARSER_H
#define __CNF_PARSER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <unordered_map>
#include "solver.hpp"
#include "intvar.hpp"


class CNFParser {
    CPSolver::Ptr _cps;
    std::vector<var<bool>::Ptr> _boolVars;
    std::unordered_map<int, var<bool>::Ptr> _boolVarMap;
    std::string _fname;
    int _curId;
    void createBoolVars(int n);
    void createClauseConstraint(std::vector<int> vars);
public:
    CNFParser(CPSolver::Ptr cps, std::string fname) : _cps(cps), _fname(fname), _curId(1) {}
    std::vector<var<bool>::Ptr> getBoolVars() { return _boolVars;}
    void parse();
};




#endif