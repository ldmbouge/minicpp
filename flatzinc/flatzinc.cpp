/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Guido Tack <tack@gecode.org>
 *
 *  Copyright:
 *     Guido Tack, 2007
 *
 *  Last modified:
 *     $Date: 2010-07-02 19:18:43 +1000 (Fri, 02 Jul 2010) $ by $Author: tack $
 *     $Revision: 11149 $
 *
 *  This file is part of Gecode, the generic constraint
 *  development environment:
 *     http://www.gecode.org
 *
 *  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "flatzinc.h"
#include "registry.h"

#include <vector>
#include <string>
#include <cstring>
#include <intvar.hpp>
#include <search.hpp>
#include <utils.hpp>

using namespace std;

namespace FlatZinc 
{
    char const * const Constraint::type2str[] =
        {
            "array_int_element",
            "array_int_maximum",
            "array_int_minimum",
            "array_var_int_element",
            "int_abs",
            "int_div",
            "int_eq",
            "int_eq_reif",
            "int_le",
            "int_le_reif",
            "int_lin_eq",
            "int_lin_eq_reif",
            "int_lin_le",
            "int_lin_le_reif",
            "int_lin_ne",
            "int_lin_ne_reif",
            "int_lt",
            "int_lt_reif",
            "int_max",
            "int_min",
            "int_mod",
            "int_ne",
            "int_ne_reif",
            "int_plus",
            "int_pow",
            "int_times",
            "array_bool_and_reif",
            "array_bool_element",
            "array_bool_or_reif",
            "array_bool_xor",
            "array_var_bool_element",
            "bool2int",
            "bool_and_reif",
            "bool_clause",
            "bool_eq",
            "bool_eq_reif",
            "bool_le",
            "bool_le_reif",
            "bool_lin_eq",
            "bool_lin_le",
            "bool_lt",
            "bool_lt_reif",
            "bool_not",
            "bool_or_reif",
            "bool_xor",
            "bool_xor_reif"
        };


    SearchHeuristic::VariableSelection SearchHeuristic::str2varSel(std::string const & str)
    {
        if (str == TO_STRING(first_fail))
        {
            return first_fail;
        }
        else if (str == TO_STRING(input_order))
        {
            return input_order;
        }
        else if (str == TO_STRING(smallest))
        {
            return smallest;
        }
        else if (str == TO_STRING(largest))
        {
            return largest;
        }
        else
        {
            std::string error = "Unsupported variable selection: ";
            error += str;
            printError(error);
            exit(EXIT_FAILURE);
        }
    }

    SearchHeuristic::ValueSelection SearchHeuristic::str2valSel(std::string const & str)
    {
        if (str == TO_STRING(indomain_min))
        {
            return indomain_min;
        }
        else if (str == TO_STRING(indomain_max))
        {
            return indomain_max;
        }
        else if (str == TO_STRING(indomain_split))
        {
            return indomain_split;
        }
        else
        {
            std::string error = "Unsupported variable selection: ";
            error += str;
            printError(error);
            exit(EXIT_FAILURE);
        }
    }

    SearchHeuristic::SearchHeuristic()
    {}

    FlatZincModel::FlatZincModel(void) :
        int_vars(),
        bool_vars(),
        constraints()
    {}

    void FlatZincModel::addIntVar(IntVarSpec* vs)
    {
        if (vs->alias)
        {
            int_vars.push_back(int_vars[vs->i]);
        }
        else if (vs->assigned)
        {
            IntVar v = {vs->i, vs->i};
            int_vars.push_back(v);
        }
        else
        {
            IntVar v;
            if (vs->domain.some()->interval)
            {
                v = {vs->domain.some()->min, vs->domain.some()->max};
            }
            else
            {
                v = {0,0, vs->domain.some()->s};
            }
            int_vars.push_back(v);
        }
    }

    void FlatZincModel::addBoolVar(BoolVarSpec* vs)
    {
        if (vs->alias)
        {
            bool_vars.push_back(bool_vars[vs->i]);
        }
        else if (vs->assigned)
        {
            BoolVar v = {vs->i ? BoolVar::State::True : BoolVar::State::False};
            bool_vars.push_back(v);
        }
        else
        {
            BoolVar v = {BoolVar::State::Unassigned};
            bool_vars.push_back(v);
        }
    }

    void FlatZincModel::addConstraint(const ConExpr& ce, AST::Node* annotation)
    {
        try
        {
            registry().post(*this, ce, annotation);
        }
        catch (AST::TypeError& e)
        {
            throw FlatZinc::Error("Type error", e.what());
        }
    }

    int FlatZincModel::arg2IntVar(AST::Node* n)
    {
        if (n->isIntVar())
        {
            return n->getIntVar();
        }
        else
        {
            addIntVar(new IntVarSpec(n->getInt(), true));
            return int_vars.size() - 1;
        }
    }

    int FlatZincModel::arg2BoolVar(AST::Node* n)
    {
        if (n->isBoolVar())
        {
            return n->getBoolVar();
        }
        else
        {
            addBoolVar(new BoolVarSpec(n->getBool(), true));
            return bool_vars.size() - 1;
        }
    }

    void FlatZincModel::solve(AST::Array* ann) 
    {
        method.type = Method::Satisfaction;
        parseSolveAnnotation(ann);
    }

    void FlatZincModel::minimize(int var, AST::Array* ann) 
    {
        method.type = Method::Minimization;
        objective_variable = var;
        parseSolveAnnotation(ann);
    }

    void FlatZincModel::maximize(int var, AST::Array* ann) 
    {
        method.type = Method::Minimization;
        objective_variable = var;
        parseSolveAnnotation(ann);
    }

    void FlatZincModel::flatSolveAnnotation(AST::Array* ann, std::vector<AST::Node*>& flat_ann)
    {
        for (size_t i = 0; i < ann->a.size(); i+=1)
        {
            if (ann->a[i]->isCall("seq_search"))
            {
                AST::Call* c = ann->a[i]->getCall();
                if (c->args->isArray())
                {
                    flatSolveAnnotation(c->args->getArray(), flat_ann);
                }
                else
                {
                    flat_ann.push_back(c->args);
                }
            }
            else
            {
                flat_ann.push_back(ann->a[i]);
            }
        }
    }

    void FlatZincModel::parseSolveAnnotation(AST::Array *ann)
    {
        if (ann)
        {
            std::vector<AST::Node *> flat_ann;
            if (ann->isArray())
            {
                flatSolveAnnotation(ann->getArray(), flat_ann);
            }
            else
            {
                flat_ann.push_back(ann);
            }

            for (size_t i = 0; i < flat_ann.size(); i += 1)
            {
                AST::Call* call = flat_ann[i]->getCall("int_search");
                AST::Array* args = call->getArgs(4);
                AST::Array* vars = args->a[0]->getArray();
                AST::Atom* varSel = args->a[1]->getAtom();
                AST::Atom* valSel = args->a[2]->getAtom();

                SearchHeuristic sh;
                if (call->isCall("int_search"))
                {
                    sh.type = SearchHeuristic::Type::integer;
                    for (size_t i = 0; i < vars->a.size(); i += 1)
                    {
                        sh.decision_variables.push_back(vars->a[i]->getIntVar());
                    }
                }
                else if (call->isCall("bool_search"))
                {
                    sh.type = SearchHeuristic::Type::boolean;
                    for (size_t i = 0; i < vars->a.size(); i += 1)
                    {
                        sh.decision_variables.push_back(vars->a[i]->getBoolVar());
                    }
                }
                else
                {
                    std::string error = "Unsupported search : ";
                    error += call->id;
                    printError(error);
                    exit(EXIT_FAILURE);
                }
                sh.variable_selection = SearchHeuristic::str2varSel(varSel->id);
                sh.value_selection = SearchHeuristic::str2valSel(valSel->id);

                search_combinator.push_back(sh);
            }
        }
    }

    void FlatZincModel::print(ostream& out, vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) const
    {
        if (output != nullptr)
        {
            for (size_t i = 0; i < output->a.size(); i += 1)
            {
                AST::Node* ai = output->a[i];
                if (ai->isArray())
                {
                    AST::Array* aia = ai->getArray();
                    int size = aia->a.size();
                    out << "[";
                    for (int j = 0; j < size; j += 1)
                    {
                        printElem(out, aia->a[j], int_vars, bool_vars);
                        if (j < size - 1)
                        {
                            out << ", ";
                        }
                    }
                    out << "]";
                }
                else
                {
                    printElem(out, ai, int_vars, bool_vars);
                }
            }
        }
        else
        {
            std::string error = "Missing output annotation";
            printError(error);
            exit(EXIT_FAILURE);
        }
    }

    void FlatZincModel::printElem(ostream& out, AST::Node* ai, vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) const
    {
        if (ai->isInt())
        {
            out << ai->getInt();
        }
        if (ai->isBool())
        {
            out << (ai->getBool() ? "true" : "false");
        }
        else if (ai->isIntVar())
        {
            out << int_vars[ai->getIntVar()]->min();
        }
        else if (ai->isBoolVar())
        {
            out << (bool_vars[ai->getBoolVar()]->isTrue() ? "true" : "false");
        }
        else if (ai->isString())
        {
            std::string s = ai->getString();
            for (size_t i = 0; i < s.size(); i += 1)
            {
                if (s[i] == '\\' and i < s.size()-1)
                {
                    switch (s[i+1])
                    {
                        case 'n':
                            out << "\n";
                            break;
                        case '\\':
                            out << "\\";
                            break;
                        case 't':
                            out << "\t";
                            break;
                        default:
                            out << "\\" << s[i+1];
                    }

                    i += 1;
                }
                else
                {
                    out << s[i];
                }
            }
        }
    }

    Error::Error(std::string const & where, std::string const & what) :
        msg(where + ": " + what) 
    {}

    std::string const & Error::toString() const 
    { 
        return msg; 
    }
}
