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

#pragma once

#include <iostream>
#include <map>
#include <cassert>
#include <varitf.hpp>
#include <intvar.hpp>

#include "conexpr.h"
#include "ast.h"
#include "varspec.h"
#include <utils.hpp>

namespace FlatZinc 
{
    struct IntVar
    {
        int min;
        int max;
        std::vector<int> values;

        friend std::ostream& operator<<(std::ostream& os, IntVar& int_var)
        {
            os << "{";
            if(int_var.values.empty())
            {
               os << int_var.min << ",...," << int_var.max;
            }
            else
            {
                printVector(os, int_var.values);
            }
            os << "}";
            return os;
        };
    };

    struct BoolVar
    {
        enum State
            {
                Unassigned,
                True,
                False
            };
        State state;

        friend std::ostream& operator<<(std::ostream& os, BoolVar& bool_var)
        {
            os << "{";
            if(bool_var.state == State::Unassigned)
            {
                os << "false, true";
            }
            else
            {
                os << (bool_var.state == State::True ? "true" : "false");
            }
            os << "}";
            return os;
        };
    };

    struct Constraint
    {
        enum Type
        {
            //Builtins
            array_int_element,
            array_int_maximum,
            array_int_minimum,
            array_var_int_element,
            int_abs,
            int_div,
            int_eq,
            int_eq_reif,
            int_le,
            int_le_reif,
            int_lin_eq,
            int_lin_eq_reif,
            int_lin_le,
            int_lin_le_reif,
            int_lin_ne,
            int_lin_ne_reif,
            int_lt,
            int_lt_reif,
            int_max,
            int_min,
            int_mod,
            int_ne,
            int_ne_reif,
            int_plus,
            int_pow,
            int_times,
            array_bool_and_reif,
            array_bool_element,
            array_bool_or_reif,
            array_bool_xor,
            array_var_bool_element,
            bool2int,
            bool_and_reif,
            bool_clause,
            bool_eq,
            bool_eq_reif,
            bool_le,
            bool_le_reif,
            bool_lin_eq,
            bool_lin_le,
            bool_lt,
            bool_lt_reif,
            bool_not,
            bool_or_reif,
            bool_xor,
            bool_xor_reif,
            //Implications
            int_eq_imp,
            int_le_imp,
            int_lin_eq_imp,
            int_lin_le_imp,
            int_lin_ne_imp,
            array_bool_and_imp,
            array_bool_or_imp,
            bool_and_imp,
            bool_eq_imp,
            bool_le_imp,
            bool_lt_imp,
            bool_or_imp,
            bool_xor_imp,
            //Globals
            all_different
        };
        Type type;
        std::vector<int> vars;
        std::vector<int> consts;

        static char const * const type2str[];

        friend std::ostream& operator<<(std::ostream& os, Constraint& constraint)
        {
            os << type2str[constraint.type];
            os << " [";
            printVector(os, constraint.vars);
            os << "] | [";
            printVector(os, constraint.consts);
            os << "]";
            return os;
        };
    };

    struct SearchHeuristic
    {
        enum Type
        {
            boolean,
            integer
        };
        enum VariableSelection
            {
                first_fail,
                input_order,
                smallest,
                largest
            };
        enum ValueSelection
            {
                indomain_min,
                indomain_max,
                indomain_split
            };


        SearchHeuristic();

        template<typename Container,typename Vec>
        SearchHeuristic(Type type,const Container& vars,const Vec& rv,
                        VariableSelection defaultVar = VariableSelection::first_fail,
                        ValueSelection defaultVal = ValueSelection::indomain_min)
          : type(type),
            decision_variables(), 
            variable_selection(defaultVar),
            value_selection(defaultVal)
        { 
          auto inserter = std::back_inserter(decision_variables);
          for(const auto& i : vars) {
              auto xi = rv[i];
              if (!xi->isBound()) 
                *inserter = i;
          }      
          //printVector(std::cout, decision_variables);              
        }

        Type type;
        std::vector<int> decision_variables;
        VariableSelection variable_selection;
        ValueSelection value_selection;

        static VariableSelection str2varSel(std::string const & str);
        static ValueSelection str2valSel(std::string const & str);
    };

    struct Method
    {
        enum Type
            {
                Maximization,
                Minimization,
                Satisfaction
            };
        Type type;
    };

    class FlatZincModel
    {
        public:
        std::vector<IntVar> int_vars;
        std::vector<BoolVar> bool_vars;
        std::vector<Constraint> constraints;
        std::vector<SearchHeuristic> search_combinator;
        int objective_variable;
        Method method;
        AST::Array* output;

        FlatZincModel();

        void addIntVar(IntVarSpec* vs);
        void addBoolVar(BoolVarSpec* vs);
        int arg2IntVar(AST::Node* n);
        int arg2BoolVar(AST::Node* n);
        void addConstraint(const ConExpr& ce, AST::Node* annotation);

        void solve(AST::Array* annotation);
        void minimize(int var, AST::Array* annotation);
        void maximize(int var, AST::Array* annotation);
        void flatSolveAnnotation(AST::Array* ann, std::vector<AST::Node*>& flat_ann);
        void parseSolveAnnotation(AST::Array* ann);

        void print(std::ostream& out, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) const;
        void printElem(std::ostream& out, AST::Node* ai,  std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) const;
    };

    class Error 
    {
        private:
            const std::string msg;
        public:
            Error(std::string const & where, std::string const & what);
            std::string const & toString() const;
    };

    FlatZincModel* parse(std::string const & fileName, std::ostream& err = std::cerr, FlatZincModel* fzs = nullptr);
}
