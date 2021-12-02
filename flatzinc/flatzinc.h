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

#include "conexpr.h"
#include "ast.h"
#include "varspec.h"

namespace FlatZinc 
{
    struct IntVar
    {
        bool interval;
        int min;
        int max;
        std::vector<int> values;
    };

    struct BoolVar
    {
        enum State {Unassigned, True, False};
        State state;
    };

    struct Constraint
    {
        enum Type {
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
            //int_lt,
            //int_lt_reif,
            //int_max,
            //int_min,
            //int_mod,
            //int_ne,
            //int_ne_reif,
            //int_plus,
            //int_pow,
            //int_times,
            array_bool_or_reif,
            bool_clause,
        };

        Type type;
        std::vector<int> vars;
        std::vector<int> consts;

        static char const * const type2str[];
    };

    enum Problem
    {
        Satisfaction,
        Minimization,
        Maximization
    };

    char const * const problem2str[] =
        {
            "Satisfaction",
            "Minimization",
            "Maximization"
        };

    enum VariableSelection
    {
        FirstFail
    };
    char const * const varSel2str[] =
        {
            "FirstFail"
        };

    enum ValueSelection
    {
        IndomainMin
    };
    static char const * const valSel2str[] =
        {
            "IndomainMin"
        };

    class Printer;

    class FlatZincModel
    {
        public:
        std::vector<IntVar> int_vars;
        std::vector<BoolVar> bool_vars;
        std::vector<Constraint> constraints;
        Problem problem;
        std::vector<int> decisionVariables;
        VariableSelection variableSelection;
        ValueSelection valueSelection;
        int objective_variable;

        FlatZincModel();
        ~FlatZincModel();

        void init(int intVars, int boolVars, int setVars);

        void newIntVar(IntVarSpec* vs);
        void newBoolVar(BoolVarSpec* vs);
        void newSetVar(SetVarSpec* vs);
        void postConstraint(ConExpr const & ce, AST::Node* annotation);

        int arg2IntVar(AST::Node* n);
        int arg2BoolVar(AST::Node* n);

        void parseSearchAnnotation(AST::Array* annotation);

        void solve(AST::Array* annotation);
        void minimize(int var, AST::Array* annotation);
        void maximize(int var, AST::Array* annotation);

        void print(std::ostream& out, Printer const & p) const;
    };

    class Printer
    {
        private:
            AST::Array* output;

        public:
            Printer();
            void init(AST::Array* output);
            void print(std::ostream& out, FlatZincModel const & m) const;
            ~Printer();

        private:
            Printer(Printer const &);
            Printer& operator=(Printer const &);
            void print(std::ostream& out, AST::Node* n, FlatZincModel const & m) const;
    };

    class Error 
    {
        private:
            const std::string msg;
        public:
            Error(std::string const & where, std::string const & what);
            std::string const & toString() const;
    };

    FlatZincModel* parse(std::string const & fileName, Printer& p, std::ostream& err = std::cerr, FlatZincModel* fzs = nullptr);
    FlatZincModel* parse(std::istream& is, Printer& p, std::ostream& err = std::cerr, FlatZincModel* fzs = nullptr);
}
