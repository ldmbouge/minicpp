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
#include <intvar.hpp>

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

    void Printer::printElem(ostream &out, AST::Node *ai, std::vector<var<int>::Ptr> *intVars, std::vector<var<bool>::Ptr> *boolVars) const
    {
        if (ai->isInt())
        {
            out << ai->getInt();
        }
        else if (ai->isBool())
        {
            out << (ai->getBool() ? "true" : "false");
        }
        else if (ai->isIntVar())
        {
            int val = intVars->at(ai->getIntVar())->min();
            out << val;
        }
        else if (ai->isBoolVar())
        {
            bool val = boolVars->at(ai->getBoolVar())->isTrue();
            out << (val ? "true" : "false");
        }
        else if (ai->isString()) {
            std::string s = ai->getString();
            for (unsigned int i=0; i<s.size(); i++) {
                if (s[i] == '\\' && i<s.size()-1) {
                    switch (s[i+1]) {
                        case 'n': out << "\n"; break;
                        case '\\': out << "\\"; break;
                        case 't': out << "\t"; break;
                        default: out << "\\" << s[i+1];
                    }
                    i++;
                } else {
                    out << s[i];
                }
            }
        }
    }

    void Printer::printElem(std::ostream& out, AST::Node* ai, const FlatZincModel& m) const
    {

        if (ai->isInt())
        {
            out << ai->getInt();
        }
        else if (ai->isIntVar())
        {
            out << ai->getIntVar();
        }
        else if (ai->isBoolVar())
        {
            // TODO: output actual variable
            out << ai->getBoolVar();
        }
        else if (ai->isSetVar())
        {
            // TODO: output actual variable
            out << ai->getSetVar();
        }
        else if (ai->isBool())
        {
            out << (ai->getBool() ? "true" : "false");
        } else if (ai->isSet())
        {
            AST::SetLit* s = ai->getSet();
            if (s->interval) {
                out << s->min << ".." << s->max;
            } else {
                out << "{";
                for (unsigned int i=0; i<s->s.size(); i++) {
                    out << s->s[i] << (i < s->s.size()-1 ? ", " : "}");
                }
            }
        }
        else if (ai->isString()) {
            std::string s = ai->getString();
            for (unsigned int i=0; i<s.size(); i++) {
                if (s[i] == '\\' && i<s.size()-1) {
                    switch (s[i+1]) {
                        case 'n': out << "\n"; break;
                        case '\\': out << "\\"; break;
                        case 't': out << "\t"; break;
                        default: out << "\\" << s[i+1];
                    }
                    i++;
                } else {
                    out << s[i];
                }
            }
        }
    }

    void
    Printer::print(std::ostream& out, const FlatZincModel& m) const {
        if (output == NULL)
            return;
        for (unsigned int i=0; i< output->a.size(); i++) {

            AST::Node* ai = output->a[i];
            if (ai->isArray())
            {
                AST::Array* aia = ai->getArray();
                int size = aia->a.size();
                out << "[";
                for (int j=0; j<size; j++)
                {
                    printElem(out,aia->a[j],m);
                    if (j<size-1)
                        out << ", ";
                }
                out << "]";
            }
            else
            {
                printElem(out,ai,m);
            }
        }
    }

    void Printer::print(ostream &out, std::vector<var<int>::Ptr> *intVars, std::vector<var<bool>::Ptr> *boolVars) const
    {
        for (unsigned int i=0; i< output->a.size(); i++)
        {

            AST::Node* ai = output->a[i];
            if (ai->isArray())
            {
                AST::Array* aia = ai->getArray();
                int size = aia->a.size();
                out << "[";
                for (int j=0; j<size; j++)
                {
                    printElem(out,aia->a[j], intVars, boolVars );
                    if (j<size-1)
                        out << ", ";
                }
                out << "]";
            }
            else
            {
                printElem(out,ai,intVars, boolVars );
            }
        }
    }

    Printer::~Printer(void)
    {
        delete output;
    }



}
