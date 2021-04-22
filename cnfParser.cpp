#include <iostream>
#include "cnfParser.hpp"
#include "constraint.hpp"

void CNFParser::createBoolVars(int n)
{
    for (int i = 0; i < n; ++i) {
        var<bool>::Ptr x = Factory::makeBoolVar(_cps);
        _boolVars.push_back(x);
        _boolVarMap.insert({_curId++,x});
    }
    return;
}

void CNFParser::createClauseConstraint(std::vector<int> vars)
{
    std::vector<var<bool>::Ptr> pos,neg;
    for (auto v : vars) {
        if (v > 0) {
            auto it = _boolVarMap.find(v);
            if (it == _boolVarMap.end())
                throw EXIT_FAILURE;
            pos.push_back(it->second);
        } else {
            auto it = _boolVarMap.find(-1*v);
            if (it == _boolVarMap.end())
                throw EXIT_FAILURE;
            neg.push_back(it->second);
        }
    }
    std::cout << "creating clause constraint:\n";
    std::cout << "\tpositives: ";
    for (auto& x : pos)
        std::cout << x->getId() << " ";
    std::cout << "\n";
    std::cout << "\tnegatives: ";
    for (auto& x : neg)
        std::cout << x->getId() << " ";
    std::cout << "\n";
    _cps->post(Factory::clause(_cps, pos, neg));
}

void CNFParser::parse()
{
    std::ifstream input(_fname);
    std::vector<int> clauseVars(0);
    std::string line;
    std::string word;
    int n = 0;
    int nbVars, nbCons;
    char c;
    for (;;) {
        // strip leading space
        while (std::isspace(c = input.peek())) 
            input.get();
        // peek at first char of line
        c = input.peek(); 
        // check for end of instance statement
        if (c == '%') 
            break;
        // skip comment lines
        if (c == 'c') {
            std::getline(input, line);
            continue;
        }
        // skip cnf declaration line
        if (c == 'p') {
            while (std::getline(input, word, ' ')) {
                if (word == "cnf") {
                    n = -1;
                    continue;
                }
                if (n == -1 && word != "") {
                    nbVars = std::stoi(word);
                    createBoolVars(nbVars);
                    n = -2;
                    continue;
                }
                if (n == -2 && word != "") {
                    nbCons = std::stoi(word);
                    std::getline(input, line);  // done on this line, so get to the beginning of the next line
                    break;
                }
            }
            continue;
        }
        // extract variables from clause line
        std::getline(input, line);
        std::stringstream ss(line);
        while (getline(ss, word, ' ')) {
            n = std::stoi(word);
            if (n != 0)
                clauseVars.push_back(n);
        }
        // make clause from variables found
        createClauseConstraint(clauseVars);
        // clear clauseVars for the next line
        clauseVars.clear();
    }
    return;
}



