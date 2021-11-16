/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Guido Tack <tack@gecode.org>
 *
 *  Contributing authors:
 *     Mikael Lagerkvist <lagerkvist@gmail.com>
 *
 *  Copyright:
 *     Guido Tack, 2007
 *     Mikael Lagerkvist, 2009
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

#include "registry.h"
#include "flatzinc.h"
#include <utility>

namespace FlatZinc
{

    Registry& registry(void)
    {
        static Registry r;
        return r;
    }

    void Registry::post(FlatZincModel& s, const ConExpr& ce, AST::Node* ann)
    {
        std::map<std::string,poster>::iterator i = r.find(ce.id);
        if (i == r.end())
        {
            throw FlatZinc::Error("Registry", std::string("Constraint ") + ce.id + " not found");
        }
        i->second(s, ce, ann);
    }

    void Registry::add(const std::string& id, poster p)
    {
        r[id] = p;
    }

    void Registry::parseConstsScope(FlatZincModel& s, AST::Node* n, Constraint& c)
    {
        if (not n->isArray())
        {
            parseConstsScopeElement(s, n, c);
        }
        else
        {
            std::vector<AST::Node*>* array = &n->getArray()->a;
            int arraySize = static_cast<int>(array->size());
            for(int i = 0; i < arraySize; i += 1)
            {
                parseConstsScopeElement(s, array->at(i), c);
            }
        }
    }

    void Registry::parseVarsScope(FlatZincModel& s, AST::Node* n, Constraint& c)
    {
        if (not n->isArray())
        {
            parseVarsScopeElement(s, n, c);
        }
        else
        {
            std::vector<AST::Node*>* array = &n->getArray()->a;
            int arraySize = static_cast<int>(array->size());
            for(int i = 0; i < arraySize; i += 1)
            {
                parseVarsScopeElement(s, array->at(i), c);
            }
        }
    }

    void Registry::parseConstsScopeElement(FlatZincModel& s, AST::Node* n, Constraint& c)
    {
        if(n->isBool())
        {
            c.consts.push_back(n->getBool());
        }
        else if (n->isInt())
        {
            c.consts.push_back(n->getInt());
        }
    }

    void Registry::parseVarsScopeElement(FlatZincModel& s, AST::Node* n, Constraint& c)
    {
        if (n->isBool() or n->isBoolVar())
        {
            c.vars.push_back(s.arg2BoolVar(n));
        }
        else if (n->isInt() or n->isIntVar())
        {
            c.vars.push_back(s.arg2IntVar(n));
        }
    }

    namespace
    {

        // Integer constraints
        void p_array_int_element(FlatZincModel& s, const ConExpr& ce, AST::Node* ann)
        {
            Constraint c;
            c.type = Constraint::Type::array_int_element;
            Registry::parseVarsScope(s, ce[0], c);
            Registry::parseConstsScope(s, ce[1], c);
            Registry::parseVarsScope(s, ce[2], c);
            s.constraints.push_back(c);
        }

        void p_array_var_int_element(FlatZincModel& s, const ConExpr& ce, AST::Node* ann)
        {
            Constraint c;
            c.type = Constraint::Type::array_var_int_element;
            Registry::parseVarsScope(s, ce[0], c);
            Registry::parseVarsScope(s, ce[1], c);
            Registry::parseVarsScope(s, ce[2], c);
            s.constraints.push_back(c);
        }

        void p_int_eq_reif(FlatZincModel& s, const ConExpr& ce, AST::Node* ann)
        {
            Constraint c;
            c.type = Constraint::Type::int_eq_reif;
            Registry::parseVarsScope(s, ce[0], c);
            Registry::parseVarsScope(s, ce[1], c);
            Registry::parseVarsScope(s, ce[2], c);
            s.constraints.push_back(c);
        }

        void p_int_lin_eq(FlatZincModel& s, const ConExpr& ce, AST::Node* ann)
        {
            Constraint c;
            c.type = Constraint::Type::int_lin_eq;
            Registry::parseConstsScope(s, ce[0], c);
            Registry::parseVarsScope(s, ce[1], c);
            Registry::parseConstsScope(s, ce[2], c);

            // Positive and negative coefficient counts
            int pos = 0;
            int neg = 0;
            for(int i = 0; i < c.vars.size(); i += 1)
            {
                if (c.consts[i] > 0)
                {
                    pos += 1;
                }
                else
                {
                    neg += 1;
                }
            }
            c.consts.push_back(pos);
            c.consts.push_back(neg);

            s.constraints.push_back(c);
        }

        void p_int_lin_eq_reif(FlatZincModel& s, const ConExpr& ce, AST::Node* ann)
        {
            Constraint c;
            c.type = Constraint::Type::int_lin_eq_reif;
            Registry::parseConstsScope(s, ce[0], c);
            Registry::parseVarsScope(s, ce[1], c);
            Registry::parseConstsScope(s, ce[2], c);
            Registry::parseVarsScope(s, ce[3], c);

            // Positive and negative coefficient counts
            int pos = 0;
            int neg = 0;
            for(int i = 0; i < c.vars.size() - 1; i += 1)
            {
                if (c.consts[i] > 0)
                {
                    pos += 1;
                }
                else
                {
                    neg += 1;
                }
            }
            c.consts.push_back(pos);
            c.consts.push_back(neg);

            s.constraints.push_back(c);
        }

        void p_int_lin_ne(FlatZincModel& s, const ConExpr& ce, AST::Node* ann)
        {
            Constraint c;
            c.type = Constraint::Type::int_lin_ne;
            Registry::parseConstsScope(s, ce[0], c);
            Registry::parseVarsScope(s, ce[1], c);
            Registry::parseConstsScope(s, ce[2], c);
            s.constraints.push_back(c);
        }

        void p_array_bool_or_reif(FlatZincModel& s, const ConExpr& ce, AST::Node* ann)
        {
            Constraint c;
            c.type = Constraint::Type::array_bool_or_reif;
            Registry::parseVarsScope(s, ce[0], c);
            Registry::parseVarsScope(s, ce[1], c);
            s.constraints.push_back(c);
        }

        void p_bool_clause(FlatZincModel& s, const ConExpr& ce, AST::Node* ann)
        {
            Constraint c;
            c.type = Constraint::Type::bool_clause;
            Registry::parseVarsScope(s, ce[0], c);
            Registry::parseVarsScope(s, ce[1], c);
            c.consts.push_back(ce[0]->getArray()->a.size());
            c.consts.push_back(ce[1]->getArray()->a.size());
            s.constraints.push_back(c);
        }

        class IntPoster
        {
            public:
            IntPoster()
            {
                registry().add("array_int_element", &p_array_int_element);
                registry().add("array_var_int_element", &p_array_var_int_element);
                registry().add("int_eq_reif", &p_int_eq_reif);
                registry().add("int_lin_eq", &p_int_lin_eq);
                registry().add("int_lin_eq_reif", &p_int_lin_eq_reif);
                registry().add("int_lin_ne", &p_int_lin_ne);
                registry().add("array_bool_or", &p_array_bool_or_reif);
                registry().add("bool_clause", &p_bool_clause);
            }
        };

        class SetPoster
        {
            public:
            SetPoster() {}
        };

        IntPoster __int_poster;
        SetPoster __set_poster;
    }

}