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

using namespace std;

namespace FlatZinc 
{
    char const * const Constraint::type2str[] =
        {
            "array_int_element",
            "array_var_int_element",
            "int_eq_reif",
            "int_lin_eq",
            "int_lin_eq_reif",
            "int_lin_ne",
            "array_bool_or_reif",
            "bool_clause"
        };

    FlatZincModel::FlatZincModel(void)
    {}

    void FlatZincModel::init(int intVars, int boolVars, int setVars)
    {
        int_vars = std::vector<IntVar>();
        bool_vars = std::vector<BoolVar>();
        constraints = std::vector<Constraint>();
    }

    void FlatZincModel::newIntVar(IntVarSpec* vs)
    {
        if (vs->alias)
        {
            int_vars.push_back(int_vars[vs->i]);
        }
        else if (vs->assigned)
        {
            IntVar v = {true, vs->i, vs->i};
            int_vars.push_back(v);
        }
        else
        {
            IntVar v;
            if (vs->domain.some()->interval)
            {
                v = {true, vs->domain.some()->min, vs->domain.some()->max};
            }
            else
            {
                v = {false, -1, 1, vs->domain.some()->s};
            }
            int_vars.push_back(v);
        }
    }

    void FlatZincModel::newBoolVar(BoolVarSpec* vs)
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

    void FlatZincModel::newSetVar(SetVarSpec* vs)
    {}

    void FlatZincModel::postConstraint(const ConExpr& ce, AST::Node* ann)
    {
        try
        {
            registry().post(*this, ce, ann);
        }
        catch (AST::TypeError& e)
        {
            throw FlatZinc::Error("Type error", e.what());
        }
    }

    void FlatZincModel::solve(AST::Array* ann) 
    {
        problem = Problem::Satisfaction;
    }

    void FlatZincModel::minimize(int var, AST::Array* ann) 
    {
        problem = Problem::Minimization;
        parseSearchAnnotation(ann);
        objective_variable = var;
    }

    void FlatZincModel::maximize(int var, AST::Array* ann) 
    {
        problem = Problem::Maximization;
        parseSearchAnnotation(ann);
        objective_variable = var;   
    }

    int FlatZincModel::arg2IntVar(AST::Node* n)
    {
        if (n->isIntVar())
        {
            return n->getIntVar();
        }
        else
        {
            newIntVar(new IntVarSpec(n->getInt(), true));
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
            newBoolVar(new BoolVarSpec(n->getBool(), true));
            return bool_vars.size() - 1;
        }
    }

    void FlatZincModel::print(std::ostream& out, Printer const & p) const
    {
        p.print(out, *this);
    }

    void FlatZincModel::parseSearchAnnotation(AST::Array* annotation)
    {
        AST::Call* call = annotation->getArray()->a[0]->getCall();
        AST::Array* args = call->getArgs(4);
        AST::Array* vars = args->a[0]->getArray();
        AST::Atom* varSel = args->a[1]->getAtom();
        AST::Atom* valSel = args->a[2]->getAtom();

        for(size_t i = 0; i < vars->a.size(); i += 1)
        {
            decisionVariables.push_back(vars->a[i]->getIntVar());
        }

        if(varSel->id == "first_fail")
        {
            variableSelection = VariableSelection::FirstFail;
        }
        else
        {
            printf("[ERROR] Unsupported variable selection %s\n", varSel->getString().c_str());
            exit(EXIT_FAILURE);
        }

        if(valSel->id== "indomain_min")
        {
            valueSelection = ValueSelection::IndomainMin;
        }
        else
        {
            printf("[ERROR] Unsupported value selection %s\n", valSel->getString().c_str());
            exit(EXIT_FAILURE);
        }
    }

    Error::Error(std::string const & where, std::string const & what) :
        msg(where + ": " + what) 
    {}

    std::string const & Error::toString() const 
    { 
        return msg; 
    }

    Printer::Printer() :
        output(nullptr)
    {}

    void Printer::init(AST::Array* output)
    {
        this->output = output;
    }

    void Printer::print(std::ostream& out, AST::Node* n, FlatZincModel const & m) const 
    {
        if (n->isInt()) 
        {
            out << n->getInt();
        }
        else if (n->isIntVar()) 
        {
            out << n->getIntVar();
        } 
        else if (n->isBool())
        {
            out << (n->getBool() ? "true" : "false");
        } 
        else if (n->isBoolVar()) 
        {
            out << n->getBoolVar();
        }      
    }

    void Printer::print(std::ostream& out, FlatZincModel const & m) const
    {
        if (output != nullptr)
        {
            for (unsigned int i = 0; i < output->a.size(); i++)
             {
                AST::Node* n = output->a[i];
                if (n->isArray())
                {
                    AST::Array* array = n->getArray();
                    int size = array->a.size();
                    out << "[";
                    print(out, array->a[0], m);
                    for (int j = 1; j < size; j += 1) 
                    {
                        out << ", ";
                        print(out, array->a[j], m);                            
                    }
                    out << "]";
                }
                else
                {
                    print(out, n, m);
                }
            }
        }
    }

    Printer::~Printer()
    {
        delete output;
    }
}
