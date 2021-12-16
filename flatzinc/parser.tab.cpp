/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */

#define YYPARSE_PARAM parm
#define YYLEX_PARAM static_cast<ParserState*>(parm)->yyscanner
#include "flatzinc.h"
#include "parser.h"
#include <iostream>
#include <fstream>
#include <sstream>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

using namespace std;

int yyparse(void*);
int yylex(YYSTYPE*, void* scanner);
//int yylex(YYSTYPE*);
int yylex_init (void** scanner);
int yylex_destroy (void* scanner);
int yyget_lineno (void* scanner);
void yyset_extra (void* user_defined ,void* yyscanner );

extern int yydebug;

using namespace FlatZinc;

void yyerror(void* parm, const char *str) {
  ParserState* pp = static_cast<ParserState*>(parm);
  pp->err << "Error: " << str
          << " in line no. " << yyget_lineno(pp->yyscanner)
          << std::endl;
  pp->hadError = true;
}

void yyassert(ParserState* pp, bool cond, const char* str)
{
  if (!cond) {
    pp->err << "Error: " << str
            << " in line no. " << yyget_lineno(pp->yyscanner)
            << std::endl;
    pp->hadError = true;
  }
}

/*
 * The symbol tables
 *
 */

AST::Node* getArrayElement(ParserState* pp, string id, unsigned int offset) {
  if (offset > 0) {
    vector<int> tmp;
    if (pp->intvararrays.get(id, tmp) && offset<=tmp.size())
      return new AST::IntVar(tmp[offset-1]);
    if (pp->boolvararrays.get(id, tmp) && offset<=tmp.size())
      return new AST::BoolVar(tmp[offset-1]);
    if (pp->setvararrays.get(id, tmp) && offset<=tmp.size())
      return new AST::SetVar(tmp[offset-1]);

    if (pp->intvalarrays.get(id, tmp) && offset<=tmp.size())
      return new AST::IntLit(tmp[offset-1]);
    if (pp->boolvalarrays.get(id, tmp) && offset<=tmp.size())
      return new AST::BoolLit(tmp[offset-1]);
    vector<AST::SetLit> tmpS;
    if (pp->setvalarrays.get(id, tmpS) && offset<=tmpS.size())
      return new AST::SetLit(tmpS[offset-1]);    
  }

  pp->err << "Error: array access to " << id << " invalid"
          << " in line no. "
          << yyget_lineno(pp->yyscanner) << std::endl;
  pp->hadError = true;
  return new AST::IntVar(0); // keep things consistent
}
AST::Node* getVarRefArg(ParserState* pp, string id, bool annotation = false) {
  int tmp;
  if (pp->intvarTable.get(id, tmp))
    return new AST::IntVar(tmp);
  if (pp->boolvarTable.get(id, tmp))
    return new AST::BoolVar(tmp);
  if (pp->setvarTable.get(id, tmp))
    return new AST::SetVar(tmp);
  if (annotation)
    return new AST::Atom(id);
  pp->err << "Error: undefined variable " << id
          << " in line no. "
          << yyget_lineno(pp->yyscanner) << std::endl;
  pp->hadError = true;
  return new AST::IntVar(0); // keep things consistent
}

void addDomainConstraint(ParserState* pp, std::string id, AST::Node* var,
                         Option<AST::SetLit* >& dom) {
  if (!dom())
    return;
  AST::Array* args = new AST::Array(2);
  args->a[0] = var;
  args->a[1] = dom.some();
  pp->domainConstraints.push_back(new ConExpr(id, args));
}

/*
 * Initialize the root gecode space
 *
 */

void initfg(ParserState* pp) {
  if (!pp->hadError)

  for (unsigned int i=0; i<pp->intvars.size(); i++) {
    if (!pp->hadError) {
      try {
        pp->fg->addIntVar(static_cast<IntVarSpec*>(pp->intvars[i].second));
      } catch (FlatZinc::Error& e) {
        yyerror(pp, e.toString().c_str());
      }
    }
    if (pp->intvars[i].first[0] != '[') {
      delete pp->intvars[i].second;
      pp->intvars[i].second = NULL;
    }
  }
  for (unsigned int i=0; i<pp->boolvars.size(); i++) {
    if (!pp->hadError) {
      try {
        pp->fg->addBoolVar(
          static_cast<BoolVarSpec*>(pp->boolvars[i].second));
      } catch (FlatZinc::Error& e) {
        yyerror(pp, e.toString().c_str());
      }
    }
    if (pp->boolvars[i].first[0] != '[') {
      delete pp->boolvars[i].second;
      pp->boolvars[i].second = NULL;
    }
  }
  /*
  for (unsigned int i=0; i<pp->setvars.size(); i++) {
    if (!pp->hadError) {
      try {
        pp->fg->newSetVar(static_cast<SetVarSpec*>(pp->setvars[i].second));
      } catch (FlatZinc::Error& e) {
        yyerror(pp, e.toString().c_str());
      }
    }      
    if (pp->setvars[i].first[0] != '[') {
      delete pp->setvars[i].second;
      pp->setvars[i].second = NULL;
    }
  }
  */
  for (unsigned int i=pp->domainConstraints.size(); i--;) {
    if (!pp->hadError) {
      try {
        assert(pp->domainConstraints[i]->args->a.size() == 2);
        pp->fg->addConstraint(*pp->domainConstraints[i], NULL);
        delete pp->domainConstraints[i];
      } catch (FlatZinc::Error& e) {
        yyerror(pp, e.toString().c_str());        
      }
    }
  }
}

/*
void fillPrinter(ParserState& pp, FlatZinc::Printer& p) {
  p.init(pp.getOutput());
}
*/

AST::Node* arrayOutput(AST::Call* ann) {
  AST::Array* a = NULL;
  
  if (ann->args->isArray()) {
    a = ann->args->getArray();
  } else {
    a = new AST::Array(ann->args);
  }
  
  std::ostringstream oss;
  
  oss << "array" << a->a.size() << "d(";
  for (unsigned int i=0; i<a->a.size(); i++) {
    AST::SetLit* s = a->a[i]->getSet();
    if (s->empty())
      oss << "{}, ";
    else if (s->interval)
      oss << s->min << ".." << s->max << ", ";
    else {
      oss << "{";
      for (unsigned int j=0; j<s->s.size(); j++) {
        oss << s->s[j];
        if (j<s->s.size()-1)
          oss << ",";
      }
      oss << "}, ";
    }
  }

  if (!ann->args->isArray()) {
    a->a[0] = NULL;
    delete a;
  }
  return new AST::String(oss.str());
}

/*
 * The main program
 *
 */

namespace FlatZinc {

  FlatZincModel* parse(const std::string& filename, std::ostream& err,
                       FlatZincModel* fzs) {
#ifdef HAVE_MMAP
    int fd;
    char* data;
    struct stat sbuf;
    fd = open(filename.c_str(), O_RDONLY);
    if (fd == -1) {
      err << "Cannot open file " << filename << endl;
      return NULL;
    }
    if (stat(filename.c_str(), &sbuf) == -1) {
      err << "Cannot stat file " << filename << endl;
      return NULL;      
    }
    data = (char*)mmap((caddr_t)0, sbuf.st_size, PROT_READ, MAP_SHARED, fd,0);
    if (data == (caddr_t)(-1)) {
      err << "Cannot mmap file " << filename << endl;
      return NULL;      
    }

    if (fzs == NULL) {
      fzs = new FlatZincModel();
    }
    ParserState pp(data, sbuf.st_size, err, fzs);
#else
    std::ifstream file;
    file.open(filename.c_str());
    if (!file.is_open()) {
      err << "Cannot open file " << filename << endl;
      return NULL;
    }
    std::string s = string(istreambuf_iterator<char>(file),
                           istreambuf_iterator<char>());
    if (fzs == NULL) {
      fzs = new FlatZincModel();
    }
    ParserState pp(s, err, fzs);
#endif
    yylex_init(&pp.yyscanner);
    yyset_extra(&pp, pp.yyscanner);
    // yydebug = 1;
    yyparse(&pp);
    pp.fg->output = pp.getOutput();

    if (pp.yyscanner)
      yylex_destroy(pp.yyscanner);
    return pp.hadError ? NULL : pp.fg;
  }

  FlatZincModel* parse(std::istream& is, std::ostream& err,
                       FlatZincModel* fzs) {
    std::string s = string(istreambuf_iterator<char>(is),
                           istreambuf_iterator<char>());

    if (fzs == NULL) {
      fzs = new FlatZincModel();
    }
    ParserState pp(s, err, fzs);
    yylex_init(&pp.yyscanner);
    yyset_extra(&pp, pp.yyscanner);
    // yydebug = 1;
    yyparse(&pp);
    pp.fg->output = pp.getOutput();
    
    if (pp.yyscanner)
      yylex_destroy(pp.yyscanner);
    return pp.hadError ? NULL : pp.fg;
  }

}



# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "parser.tab.hpp"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_FZ_INT_LIT = 3,                 /* FZ_INT_LIT  */
  YYSYMBOL_FZ_BOOL_LIT = 4,                /* FZ_BOOL_LIT  */
  YYSYMBOL_FZ_FLOAT_LIT = 5,               /* FZ_FLOAT_LIT  */
  YYSYMBOL_FZ_ID = 6,                      /* FZ_ID  */
  YYSYMBOL_FZ_U_ID = 7,                    /* FZ_U_ID  */
  YYSYMBOL_FZ_STRING_LIT = 8,              /* FZ_STRING_LIT  */
  YYSYMBOL_FZ_VAR = 9,                     /* FZ_VAR  */
  YYSYMBOL_FZ_PAR = 10,                    /* FZ_PAR  */
  YYSYMBOL_FZ_ANNOTATION = 11,             /* FZ_ANNOTATION  */
  YYSYMBOL_FZ_ANY = 12,                    /* FZ_ANY  */
  YYSYMBOL_FZ_ARRAY = 13,                  /* FZ_ARRAY  */
  YYSYMBOL_FZ_BOOL = 14,                   /* FZ_BOOL  */
  YYSYMBOL_FZ_CASE = 15,                   /* FZ_CASE  */
  YYSYMBOL_FZ_COLONCOLON = 16,             /* FZ_COLONCOLON  */
  YYSYMBOL_FZ_CONSTRAINT = 17,             /* FZ_CONSTRAINT  */
  YYSYMBOL_FZ_DEFAULT = 18,                /* FZ_DEFAULT  */
  YYSYMBOL_FZ_DOTDOT = 19,                 /* FZ_DOTDOT  */
  YYSYMBOL_FZ_ELSE = 20,                   /* FZ_ELSE  */
  YYSYMBOL_FZ_ELSEIF = 21,                 /* FZ_ELSEIF  */
  YYSYMBOL_FZ_ENDIF = 22,                  /* FZ_ENDIF  */
  YYSYMBOL_FZ_ENUM = 23,                   /* FZ_ENUM  */
  YYSYMBOL_FZ_FLOAT = 24,                  /* FZ_FLOAT  */
  YYSYMBOL_FZ_FUNCTION = 25,               /* FZ_FUNCTION  */
  YYSYMBOL_FZ_IF = 26,                     /* FZ_IF  */
  YYSYMBOL_FZ_INCLUDE = 27,                /* FZ_INCLUDE  */
  YYSYMBOL_FZ_INT = 28,                    /* FZ_INT  */
  YYSYMBOL_FZ_LET = 29,                    /* FZ_LET  */
  YYSYMBOL_FZ_MAXIMIZE = 30,               /* FZ_MAXIMIZE  */
  YYSYMBOL_FZ_MINIMIZE = 31,               /* FZ_MINIMIZE  */
  YYSYMBOL_FZ_OF = 32,                     /* FZ_OF  */
  YYSYMBOL_FZ_SATISFY = 33,                /* FZ_SATISFY  */
  YYSYMBOL_FZ_OUTPUT = 34,                 /* FZ_OUTPUT  */
  YYSYMBOL_FZ_PREDICATE = 35,              /* FZ_PREDICATE  */
  YYSYMBOL_FZ_RECORD = 36,                 /* FZ_RECORD  */
  YYSYMBOL_FZ_SET = 37,                    /* FZ_SET  */
  YYSYMBOL_FZ_SHOW = 38,                   /* FZ_SHOW  */
  YYSYMBOL_FZ_SHOWCOND = 39,               /* FZ_SHOWCOND  */
  YYSYMBOL_FZ_SOLVE = 40,                  /* FZ_SOLVE  */
  YYSYMBOL_FZ_STRING = 41,                 /* FZ_STRING  */
  YYSYMBOL_FZ_TEST = 42,                   /* FZ_TEST  */
  YYSYMBOL_FZ_THEN = 43,                   /* FZ_THEN  */
  YYSYMBOL_FZ_TUPLE = 44,                  /* FZ_TUPLE  */
  YYSYMBOL_FZ_TYPE = 45,                   /* FZ_TYPE  */
  YYSYMBOL_FZ_VARIANT_RECORD = 46,         /* FZ_VARIANT_RECORD  */
  YYSYMBOL_FZ_WHERE = 47,                  /* FZ_WHERE  */
  YYSYMBOL_48_ = 48,                       /* ';'  */
  YYSYMBOL_49_ = 49,                       /* '('  */
  YYSYMBOL_50_ = 50,                       /* ')'  */
  YYSYMBOL_51_ = 51,                       /* ','  */
  YYSYMBOL_52_ = 52,                       /* ':'  */
  YYSYMBOL_53_ = 53,                       /* '['  */
  YYSYMBOL_54_ = 54,                       /* ']'  */
  YYSYMBOL_55_ = 55,                       /* '='  */
  YYSYMBOL_56_ = 56,                       /* '{'  */
  YYSYMBOL_57_ = 57,                       /* '}'  */
  YYSYMBOL_YYACCEPT = 58,                  /* $accept  */
  YYSYMBOL_model = 59,                     /* model  */
  YYSYMBOL_preddecl_items = 60,            /* preddecl_items  */
  YYSYMBOL_preddecl_items_head = 61,       /* preddecl_items_head  */
  YYSYMBOL_vardecl_items = 62,             /* vardecl_items  */
  YYSYMBOL_vardecl_items_head = 63,        /* vardecl_items_head  */
  YYSYMBOL_constraint_items = 64,          /* constraint_items  */
  YYSYMBOL_constraint_items_head = 65,     /* constraint_items_head  */
  YYSYMBOL_preddecl_item = 66,             /* preddecl_item  */
  YYSYMBOL_pred_arg_list = 67,             /* pred_arg_list  */
  YYSYMBOL_pred_arg_list_head = 68,        /* pred_arg_list_head  */
  YYSYMBOL_pred_arg = 69,                  /* pred_arg  */
  YYSYMBOL_pred_arg_type = 70,             /* pred_arg_type  */
  YYSYMBOL_pred_arg_simple_type = 71,      /* pred_arg_simple_type  */
  YYSYMBOL_pred_array_init = 72,           /* pred_array_init  */
  YYSYMBOL_pred_array_init_arg = 73,       /* pred_array_init_arg  */
  YYSYMBOL_var_par_id = 74,                /* var_par_id  */
  YYSYMBOL_vardecl_item = 75,              /* vardecl_item  */
  YYSYMBOL_int_init = 76,                  /* int_init  */
  YYSYMBOL_int_init_list = 77,             /* int_init_list  */
  YYSYMBOL_int_init_list_head = 78,        /* int_init_list_head  */
  YYSYMBOL_list_tail = 79,                 /* list_tail  */
  YYSYMBOL_int_var_array_literal = 80,     /* int_var_array_literal  */
  YYSYMBOL_float_init = 81,                /* float_init  */
  YYSYMBOL_float_init_list = 82,           /* float_init_list  */
  YYSYMBOL_float_init_list_head = 83,      /* float_init_list_head  */
  YYSYMBOL_float_var_array_literal = 84,   /* float_var_array_literal  */
  YYSYMBOL_bool_init = 85,                 /* bool_init  */
  YYSYMBOL_bool_init_list = 86,            /* bool_init_list  */
  YYSYMBOL_bool_init_list_head = 87,       /* bool_init_list_head  */
  YYSYMBOL_bool_var_array_literal = 88,    /* bool_var_array_literal  */
  YYSYMBOL_set_init = 89,                  /* set_init  */
  YYSYMBOL_set_init_list = 90,             /* set_init_list  */
  YYSYMBOL_set_init_list_head = 91,        /* set_init_list_head  */
  YYSYMBOL_set_var_array_literal = 92,     /* set_var_array_literal  */
  YYSYMBOL_vardecl_int_var_array_init = 93, /* vardecl_int_var_array_init  */
  YYSYMBOL_vardecl_bool_var_array_init = 94, /* vardecl_bool_var_array_init  */
  YYSYMBOL_vardecl_float_var_array_init = 95, /* vardecl_float_var_array_init  */
  YYSYMBOL_vardecl_set_var_array_init = 96, /* vardecl_set_var_array_init  */
  YYSYMBOL_constraint_item = 97,           /* constraint_item  */
  YYSYMBOL_solve_item = 98,                /* solve_item  */
  YYSYMBOL_int_ti_expr_tail = 99,          /* int_ti_expr_tail  */
  YYSYMBOL_bool_ti_expr_tail = 100,        /* bool_ti_expr_tail  */
  YYSYMBOL_float_ti_expr_tail = 101,       /* float_ti_expr_tail  */
  YYSYMBOL_set_literal = 102,              /* set_literal  */
  YYSYMBOL_int_list = 103,                 /* int_list  */
  YYSYMBOL_int_list_head = 104,            /* int_list_head  */
  YYSYMBOL_bool_list = 105,                /* bool_list  */
  YYSYMBOL_bool_list_head = 106,           /* bool_list_head  */
  YYSYMBOL_float_list = 107,               /* float_list  */
  YYSYMBOL_float_list_head = 108,          /* float_list_head  */
  YYSYMBOL_set_literal_list = 109,         /* set_literal_list  */
  YYSYMBOL_set_literal_list_head = 110,    /* set_literal_list_head  */
  YYSYMBOL_flat_expr_list = 111,           /* flat_expr_list  */
  YYSYMBOL_flat_expr = 112,                /* flat_expr  */
  YYSYMBOL_non_array_expr_opt = 113,       /* non_array_expr_opt  */
  YYSYMBOL_non_array_expr = 114,           /* non_array_expr  */
  YYSYMBOL_non_array_expr_list = 115,      /* non_array_expr_list  */
  YYSYMBOL_non_array_expr_list_head = 116, /* non_array_expr_list_head  */
  YYSYMBOL_solve_expr = 117,               /* solve_expr  */
  YYSYMBOL_minmax = 118,                   /* minmax  */
  YYSYMBOL_annotations = 119,              /* annotations  */
  YYSYMBOL_annotations_head = 120,         /* annotations_head  */
  YYSYMBOL_annotation = 121,               /* annotation  */
  YYSYMBOL_annotation_list = 122,          /* annotation_list  */
  YYSYMBOL_annotation_expr = 123,          /* annotation_expr  */
  YYSYMBOL_ann_non_array_expr = 124        /* ann_non_array_expr  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if 1

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* 1 */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  7
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   337

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  58
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  67
/* YYNRULES -- Number of rules.  */
#define YYNRULES  156
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  336

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   302


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      49,    50,     2,     2,    51,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    52,    48,
       2,    55,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    53,     2,    54,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    56,     2,    57,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   433,   433,   435,   437,   440,   441,   445,   446,   450,
     451,   453,   455,   458,   459,   466,   468,   470,   473,   474,
     477,   480,   481,   482,   483,   486,   487,   488,   489,   492,
     493,   496,   497,   503,   503,   506,   535,   564,   569,   599,
     606,   613,   622,   681,   733,   740,   797,   810,   823,   830,
     844,   848,   863,   887,   888,   892,   894,   897,   897,   899,
     903,   905,   920,   944,   945,   949,   951,   955,   959,   961,
     976,  1000,  1001,  1005,  1007,  1010,  1013,  1015,  1030,  1054,
    1055,  1059,  1061,  1064,  1069,  1070,  1075,  1076,  1081,  1082,
    1087,  1088,  1092,  1106,  1119,  1141,  1143,  1145,  1151,  1153,
    1166,  1167,  1174,  1176,  1183,  1184,  1188,  1190,  1195,  1196,
    1200,  1202,  1207,  1208,  1212,  1214,  1219,  1220,  1224,  1226,
    1234,  1236,  1240,  1242,  1247,  1248,  1252,  1254,  1256,  1258,
    1260,  1309,  1323,  1324,  1328,  1330,  1338,  1349,  1371,  1372,
    1380,  1381,  1385,  1387,  1391,  1395,  1399,  1401,  1405,  1407,
    1411,  1413,  1415,  1417,  1419,  1462,  1473
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if 1
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "FZ_INT_LIT",
  "FZ_BOOL_LIT", "FZ_FLOAT_LIT", "FZ_ID", "FZ_U_ID", "FZ_STRING_LIT",
  "FZ_VAR", "FZ_PAR", "FZ_ANNOTATION", "FZ_ANY", "FZ_ARRAY", "FZ_BOOL",
  "FZ_CASE", "FZ_COLONCOLON", "FZ_CONSTRAINT", "FZ_DEFAULT", "FZ_DOTDOT",
  "FZ_ELSE", "FZ_ELSEIF", "FZ_ENDIF", "FZ_ENUM", "FZ_FLOAT", "FZ_FUNCTION",
  "FZ_IF", "FZ_INCLUDE", "FZ_INT", "FZ_LET", "FZ_MAXIMIZE", "FZ_MINIMIZE",
  "FZ_OF", "FZ_SATISFY", "FZ_OUTPUT", "FZ_PREDICATE", "FZ_RECORD",
  "FZ_SET", "FZ_SHOW", "FZ_SHOWCOND", "FZ_SOLVE", "FZ_STRING", "FZ_TEST",
  "FZ_THEN", "FZ_TUPLE", "FZ_TYPE", "FZ_VARIANT_RECORD", "FZ_WHERE", "';'",
  "'('", "')'", "','", "':'", "'['", "']'", "'='", "'{'", "'}'", "$accept",
  "model", "preddecl_items", "preddecl_items_head", "vardecl_items",
  "vardecl_items_head", "constraint_items", "constraint_items_head",
  "preddecl_item", "pred_arg_list", "pred_arg_list_head", "pred_arg",
  "pred_arg_type", "pred_arg_simple_type", "pred_array_init",
  "pred_array_init_arg", "var_par_id", "vardecl_item", "int_init",
  "int_init_list", "int_init_list_head", "list_tail",
  "int_var_array_literal", "float_init", "float_init_list",
  "float_init_list_head", "float_var_array_literal", "bool_init",
  "bool_init_list", "bool_init_list_head", "bool_var_array_literal",
  "set_init", "set_init_list", "set_init_list_head",
  "set_var_array_literal", "vardecl_int_var_array_init",
  "vardecl_bool_var_array_init", "vardecl_float_var_array_init",
  "vardecl_set_var_array_init", "constraint_item", "solve_item",
  "int_ti_expr_tail", "bool_ti_expr_tail", "float_ti_expr_tail",
  "set_literal", "int_list", "int_list_head", "bool_list",
  "bool_list_head", "float_list", "float_list_head", "set_literal_list",
  "set_literal_list_head", "flat_expr_list", "flat_expr",
  "non_array_expr_opt", "non_array_expr", "non_array_expr_list",
  "non_array_expr_list_head", "solve_expr", "minmax", "annotations",
  "annotations_head", "annotation", "annotation_list", "annotation_expr",
  "ann_non_array_expr", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-120)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
       0,    34,    46,   101,     0,     1,    12,  -120,    97,    -3,
      29,    33,    71,    96,   101,    68,    74,  -120,    50,   100,
    -120,  -120,  -120,    94,    64,    76,    78,    80,   136,    23,
      23,   112,   151,   119,    96,   113,   114,  -120,  -120,   144,
     110,  -120,  -120,   132,   163,   117,   118,  -120,   126,  -120,
    -120,   167,    24,  -120,  -120,  -120,   122,   120,   129,   131,
      23,    23,    23,   164,  -120,  -120,   168,   168,   133,   137,
     168,   139,   149,  -120,  -120,  -120,    15,    24,  -120,    50,
    -120,   186,  -120,   146,  -120,   198,  -120,   201,   150,   209,
     158,   168,   168,   168,   203,     9,   162,   202,   165,    23,
      51,    53,  -120,  -120,   200,  -120,   -15,  -120,  -120,  -120,
    -120,    23,  -120,  -120,  -120,  -120,  -120,   169,   169,   169,
     174,   204,  -120,  -120,   187,  -120,     9,   163,   182,  -120,
    -120,  -120,  -120,   170,     9,   170,   168,   204,  -120,  -120,
     170,   184,  -120,    44,  -120,  -120,  -120,  -120,  -120,    23,
     236,    15,   208,   168,   170,  -120,  -120,  -120,   210,   238,
       9,  -120,   -10,   189,    85,  -120,  -120,  -120,   190,  -120,
     193,   192,   170,   168,    51,   195,  -120,  -120,  -120,   109,
     169,  -120,    10,  -120,    58,     9,  -120,  -120,   196,   170,
    -120,   170,  -120,   197,  -120,  -120,   246,   144,  -120,  -120,
     188,   205,   206,   207,   220,  -120,  -120,  -120,  -120,  -120,
    -120,   199,  -120,   222,   211,   213,   214,    23,    23,    23,
     227,  -120,    24,    23,    23,    23,   168,   168,   168,   215,
     217,   168,   168,   168,   216,   218,   219,    23,    23,   223,
     224,   225,   228,   229,   233,   168,   168,   234,  -120,   239,
    -120,   240,  -120,   257,   265,   163,   241,   242,    19,  -120,
     148,  -120,   138,  -120,   221,   129,   237,   131,   245,   247,
     248,  -120,  -120,   251,  -120,   252,   226,  -120,   254,  -120,
     255,   243,  -120,   258,  -120,   256,   244,  -120,  -120,  -120,
    -120,  -120,    17,    95,  -120,   259,  -120,    19,  -120,   302,
    -120,   148,  -120,   305,  -120,   138,  -120,   204,  -120,   260,
     261,   262,  -120,   263,   267,  -120,   266,  -120,   268,  -120,
     269,  -120,  -120,    17,  -120,   310,  -120,    95,  -120,  -120,
    -120,  -120,  -120,   270,  -120,  -120
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       3,     0,     0,     7,     4,     0,     0,     1,     0,     0,
       0,     0,     0,    11,     8,     0,     0,     5,    16,     0,
      98,   100,    95,     0,   104,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    12,     0,     0,     9,     6,     0,
       0,    27,    28,     0,   104,     0,    57,    18,     0,    24,
      25,     0,     0,   106,   110,   114,     0,    57,    57,    57,
       0,     0,     0,     0,    33,    34,   140,   140,     0,     0,
     140,     0,     0,    13,    10,    23,     0,     0,    15,    58,
      17,     0,    97,     0,    96,    58,   105,    58,     0,    58,
       0,   140,   140,   140,     0,     0,     0,   141,     0,     0,
       0,     0,     2,    14,     0,    31,     0,    29,    26,    19,
      20,     0,   107,   111,    99,   115,   101,   124,   124,   124,
       0,   151,   150,   152,    33,   156,     0,   104,   154,   153,
     142,   145,   148,     0,     0,     0,   140,   127,   126,   128,
     132,   130,   129,     0,   120,   122,   139,   138,    93,     0,
       0,     0,     0,   140,     0,    35,    36,    37,     0,     0,
       0,   146,     0,     0,     0,    40,   143,    39,     0,   134,
       0,    57,     0,   140,     0,   136,    94,    32,    30,     0,
     124,   125,     0,   103,     0,     0,   149,   102,     0,     0,
     123,    58,   133,     0,    92,   121,     0,     0,    21,    38,
       0,     0,     0,     0,     0,   144,   147,   155,    41,   135,
     131,     0,    22,     0,     0,     0,     0,     0,     0,     0,
       0,   137,     0,     0,     0,     0,   140,   140,   140,     0,
       0,   140,   140,   140,     0,     0,     0,     0,     0,    84,
      86,    88,     0,     0,     0,   140,   140,     0,    42,     0,
      43,     0,    44,   108,   112,   104,     0,    90,    53,    85,
      71,    87,    63,    89,     0,    57,     0,    57,     0,     0,
       0,    45,    50,    51,    55,     0,    57,    68,    69,    73,
       0,    57,    60,    61,    65,     0,    57,    47,   109,    48,
     113,    46,   116,    79,    91,     0,    59,    58,    54,     0,
      75,    58,    72,     0,    67,    58,    64,     0,   118,     0,
      57,    77,    81,     0,    57,    76,     0,    56,     0,    74,
       0,    66,    49,    58,   117,     0,    83,    58,    80,    52,
      70,    62,   119,     0,    82,    78
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -120,  -120,  -120,  -120,  -120,  -120,  -120,  -120,   312,  -120,
    -120,   249,  -120,   -37,  -120,   175,   -29,   307,    22,  -120,
    -120,   -54,  -120,    20,  -120,  -120,  -120,    26,  -120,  -120,
    -120,     2,  -120,  -120,  -120,  -120,  -120,  -120,  -120,   296,
    -120,    -1,   134,   135,   -89,  -119,  -120,  -120,    79,  -120,
      77,  -120,  -120,  -120,   159,  -108,  -112,  -120,  -120,  -120,
    -120,    57,  -120,   -86,   176,  -120,   173
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     2,     3,     4,    13,    14,    33,    34,     5,    45,
      46,    47,    48,    49,   106,   107,   141,    15,   274,   275,
     276,    80,   259,   284,   285,   286,   263,   279,   280,   281,
     261,   312,   313,   314,   294,   248,   250,   252,   271,    35,
      71,    50,    26,    27,   142,    56,    57,   264,    58,   266,
      59,   309,   310,   143,   144,   155,   145,   170,   171,   176,
     149,    96,    97,   161,   162,   131,   132
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      66,    67,    75,    86,    88,    90,   129,    25,   163,   130,
     156,   157,   121,   122,   123,   124,    65,   125,   104,   200,
     307,   165,   272,   167,   201,    64,    65,    19,   169,    64,
      65,    91,    92,    93,   202,     1,   151,   129,   203,   152,
       6,   185,   181,   105,   186,   129,     7,   204,   166,    17,
      28,    83,    22,    19,   137,   138,   139,    64,    65,    39,
     193,    18,   126,    40,    41,   127,   128,    53,    54,    55,
     136,   129,   199,   127,    42,   129,   108,   208,    22,   209,
      44,    29,   153,   146,   147,    30,   148,    43,   121,   122,
     123,    64,    65,   125,   173,   174,   129,   128,   307,   206,
      19,    64,    65,    31,   140,   128,    44,   127,   205,   185,
       8,    20,    19,    32,     9,    10,    37,   192,   197,    51,
     175,    21,    38,    41,    98,    22,    52,   101,    60,    11,
      61,   128,    62,    42,    23,   128,   268,    22,    12,    63,
      68,   127,   198,   282,    64,    65,    43,    19,   117,   118,
     119,   127,   277,    24,    64,    65,   128,    69,    41,    70,
     212,    73,    74,    76,    77,    44,    53,    78,    42,    79,
      82,    85,    22,   137,   138,   139,    64,    65,    81,    84,
      87,    43,    89,    94,    95,    99,   100,   102,   226,   227,
     228,    19,   110,   168,   231,   232,   233,   103,   111,   214,
      44,   112,    20,   308,   315,   113,   120,   114,   245,   246,
     180,   288,    21,   290,   115,   116,    22,   133,   134,   150,
     135,   230,   298,   159,   154,   213,   127,   302,   158,   273,
     194,   278,   306,   283,   332,   164,   160,   172,   315,   177,
     179,   183,   182,   191,    24,   189,   187,   190,   196,   211,
     207,   210,   220,   221,   222,   229,   324,   217,   218,   219,
     328,    54,   316,   223,   311,   224,   225,   237,   273,   238,
      55,   242,   278,   243,   244,   287,   283,   297,   247,   249,
     251,   253,   254,   234,   235,   236,   255,   258,   239,   240,
     241,   289,   260,   262,   301,   305,   269,   270,   311,   291,
     292,   293,   256,   257,   295,   318,   296,   299,   320,   300,
     304,   303,   323,   333,   322,   325,    16,   326,   327,   317,
     329,    36,   330,   331,   335,   321,   178,   319,   109,   334,
      72,   267,   265,   195,   215,   216,   184,   188
};

static const yytype_int16 yycheck[] =
{
      29,    30,    39,    57,    58,    59,    95,     8,   127,    95,
     118,   119,     3,     4,     5,     6,     7,     8,     3,     9,
       3,   133,     3,   135,    14,     6,     7,     3,   140,     6,
       7,    60,    61,    62,    24,    35,    51,   126,    28,    54,
       6,    51,   154,    28,    54,   134,     0,    37,   134,    48,
      53,    52,    28,     3,     3,     4,     5,     6,     7,     9,
     172,    49,    53,    13,    14,    56,    95,     3,     4,     5,
      99,   160,   180,    56,    24,   164,    77,   189,    28,   191,
      56,    52,   111,    30,    31,    52,    33,    37,     3,     4,
       5,     6,     7,     8,    50,    51,   185,   126,     3,   185,
       3,     6,     7,    32,    53,   134,    56,    56,    50,    51,
       9,    14,     3,    17,    13,    14,    48,   171,     9,    19,
     149,    24,    48,    14,    67,    28,    32,    70,    52,    28,
      52,   160,    52,    24,    37,   164,   255,    28,    37,     3,
      28,    56,   179,     5,     6,     7,    37,     3,    91,    92,
      93,    56,     4,    56,     6,     7,   185,     6,    14,    40,
     197,    48,    48,    53,    32,    56,     3,    50,    24,    51,
       3,    51,    28,     3,     4,     5,     6,     7,    52,    57,
      51,    37,    51,    19,    16,    52,    49,    48,   217,   218,
     219,     3,     6,   136,   223,   224,   225,    48,    52,   200,
      56,     3,    14,   292,   293,     4,     3,    57,   237,   238,
     153,   265,    24,   267,     5,    57,    28,    55,    16,    19,
      55,   222,   276,    19,    55,    37,    56,   281,    54,   258,
     173,   260,   286,   262,   323,    53,    49,    53,   327,     3,
      32,     3,    32,    51,    56,    55,    57,    54,    53,     3,
      54,    54,    32,    54,    32,    28,   310,    52,    52,    52,
     314,     4,     3,    52,   293,    52,    52,    52,   297,    52,
       5,    55,   301,    55,    55,    54,   305,    51,    55,    55,
      55,    53,    53,   226,   227,   228,    53,    53,   231,   232,
     233,    54,    53,    53,    51,    51,    55,    55,   327,    54,
      53,    53,   245,   246,    53,     3,    54,    53,     3,    54,
      54,    53,    51,     3,    54,    53,     4,    54,    51,   297,
      54,    14,    54,    54,    54,   305,   151,   301,    79,   327,
      34,   254,   253,   174,   200,   200,   160,   164
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    35,    59,    60,    61,    66,     6,     0,     9,    13,
      14,    28,    37,    62,    63,    75,    66,    48,    49,     3,
      14,    24,    28,    37,    56,    99,   100,   101,    53,    52,
      52,    32,    17,    64,    65,    97,    75,    48,    48,     9,
      13,    14,    24,    37,    56,    67,    68,    69,    70,    71,
      99,    19,    32,     3,     4,     5,   103,   104,   106,   108,
      52,    52,    52,     3,     6,     7,    74,    74,    28,     6,
      40,    98,    97,    48,    48,    71,    53,    32,    50,    51,
      79,    52,     3,    99,    57,    51,    79,    51,    79,    51,
      79,    74,    74,    74,    19,    16,   119,   120,   119,    52,
      49,   119,    48,    48,     3,    28,    72,    73,    99,    69,
       6,    52,     3,     4,    57,     5,    57,   119,   119,   119,
       3,     3,     4,     5,     6,     8,    53,    56,    74,   102,
     121,   123,   124,    55,    16,    55,    74,     3,     4,     5,
      53,    74,   102,   111,   112,   114,    30,    31,    33,   118,
      19,    51,    54,    74,    55,   113,   113,   113,    54,    19,
      49,   121,   122,   103,    53,   114,   121,   114,   119,   114,
     115,   116,    53,    50,    51,    74,   117,     3,    73,    32,
     119,   114,    32,     3,   122,    51,    54,    57,   124,    55,
      54,    51,    79,   114,   119,   112,    53,     9,    71,   113,
       9,    14,    24,    28,    37,    50,   121,    54,   114,   114,
      54,     3,    71,    37,    99,   100,   101,    52,    52,    52,
      32,    54,    32,    52,    52,    52,    74,    74,    74,    28,
      99,    74,    74,    74,   119,   119,   119,    52,    52,   119,
     119,   119,    55,    55,    55,    74,    74,    55,    93,    55,
      94,    55,    95,    53,    53,    53,   119,   119,    53,    80,
      53,    88,    53,    84,   105,   106,   107,   108,   103,    55,
      55,    96,     3,    74,    76,    77,    78,     4,    74,    85,
      86,    87,     5,    74,    81,    82,    83,    54,    79,    54,
      79,    54,    53,    53,    92,    53,    54,    51,    79,    53,
      54,    51,    79,    53,    54,    51,    79,     3,   102,   109,
     110,    74,    89,    90,    91,   102,     3,    76,     3,    85,
       3,    81,    54,    51,    79,    53,    54,    51,    79,    54,
      54,    54,   102,     3,    89,    54
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    58,    59,    60,    60,    61,    61,    62,    62,    63,
      63,    64,    64,    65,    65,    66,    67,    67,    68,    68,
      69,    70,    70,    70,    70,    71,    71,    71,    71,    72,
      72,    73,    73,    74,    74,    75,    75,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    75,
      76,    76,    76,    77,    77,    78,    78,    79,    79,    80,
      81,    81,    81,    82,    82,    83,    83,    84,    85,    85,
      85,    86,    86,    87,    87,    88,    89,    89,    89,    90,
      90,    91,    91,    92,    93,    93,    94,    94,    95,    95,
      96,    96,    97,    98,    98,    99,    99,    99,   100,   100,
     101,   101,   102,   102,   103,   103,   104,   104,   105,   105,
     106,   106,   107,   107,   108,   108,   109,   109,   110,   110,
     111,   111,   112,   112,   113,   113,   114,   114,   114,   114,
     114,   114,   115,   115,   116,   116,   117,   117,   118,   118,
     119,   119,   120,   120,   121,   121,   122,   122,   123,   123,
     124,   124,   124,   124,   124,   124,   124
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     5,     0,     1,     2,     3,     0,     1,     2,
       3,     0,     1,     2,     3,     5,     0,     2,     1,     3,
       3,     6,     7,     2,     1,     1,     3,     1,     1,     1,
       3,     1,     3,     1,     1,     6,     6,     6,     8,     6,
       6,     8,    13,    13,    13,    15,    15,    15,    15,    17,
       1,     1,     4,     0,     2,     1,     3,     0,     1,     3,
       1,     1,     4,     0,     2,     1,     3,     3,     1,     1,
       4,     0,     2,     1,     3,     3,     1,     1,     4,     0,
       2,     1,     3,     3,     0,     2,     0,     2,     0,     2,
       0,     2,     6,     3,     4,     1,     3,     3,     1,     4,
       1,     4,     3,     3,     0,     2,     1,     3,     0,     2,
       1,     3,     0,     2,     1,     3,     0,     2,     1,     3,
       1,     3,     1,     3,     0,     2,     1,     1,     1,     1,
       1,     4,     0,     2,     1,     3,     1,     4,     1,     1,
       0,     1,     2,     3,     4,     1,     1,     3,     1,     3,
       1,     1,     1,     1,     1,     4,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (parm, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, parm); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, void *parm)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (parm);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, void *parm)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep, parm);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule, void *parm)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)], parm);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, parm); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


/* Context of a parse error.  */
typedef struct
{
  yy_state_t *yyssp;
  yysymbol_kind_t yytoken;
} yypcontext_t;

/* Put in YYARG at most YYARGN of the expected tokens given the
   current YYCTX, and return the number of tokens stored in YYARG.  If
   YYARG is null, return the number of expected tokens (guaranteed to
   be less than YYNTOKENS).  Return YYENOMEM on memory exhaustion.
   Return 0 if there are more than YYARGN expected tokens, yet fill
   YYARG up to YYARGN. */
static int
yypcontext_expected_tokens (const yypcontext_t *yyctx,
                            yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  int yyn = yypact[+*yyctx->yyssp];
  if (!yypact_value_is_default (yyn))
    {
      /* Start YYX at -YYN if negative to avoid negative indexes in
         YYCHECK.  In other words, skip the first -YYN actions for
         this state because they are default actions.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;
      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yyx;
      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
        if (yycheck[yyx + yyn] == yyx && yyx != YYSYMBOL_YYerror
            && !yytable_value_is_error (yytable[yyx + yyn]))
          {
            if (!yyarg)
              ++yycount;
            else if (yycount == yyargn)
              return 0;
            else
              yyarg[yycount++] = YY_CAST (yysymbol_kind_t, yyx);
          }
    }
  if (yyarg && yycount == 0 && 0 < yyargn)
    yyarg[0] = YYSYMBOL_YYEMPTY;
  return yycount;
}




#ifndef yystrlen
# if defined __GLIBC__ && defined _STRING_H
#  define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
# else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
# endif
#endif

#ifndef yystpcpy
# if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#  define yystpcpy stpcpy
# else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
# endif
#endif

#ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
      char const *yyp = yystr;
      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
#endif


static int
yy_syntax_error_arguments (const yypcontext_t *yyctx,
                           yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yyctx->yytoken != YYSYMBOL_YYEMPTY)
    {
      int yyn;
      if (yyarg)
        yyarg[yycount] = yyctx->yytoken;
      ++yycount;
      yyn = yypcontext_expected_tokens (yyctx,
                                        yyarg ? yyarg + 1 : yyarg, yyargn - 1);
      if (yyn == YYENOMEM)
        return YYENOMEM;
      else
        yycount += yyn;
    }
  return yycount;
}

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return -1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return YYENOMEM if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                const yypcontext_t *yyctx)
{
  enum { YYARGS_MAX = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  yysymbol_kind_t yyarg[YYARGS_MAX];
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* Actual size of YYARG. */
  int yycount = yy_syntax_error_arguments (yyctx, yyarg, YYARGS_MAX);
  if (yycount == YYENOMEM)
    return YYENOMEM;

  switch (yycount)
    {
#define YYCASE_(N, S)                       \
      case N:                               \
        yyformat = S;                       \
        break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

  /* Compute error message size.  Don't count the "%s"s, but reserve
     room for the terminator.  */
  yysize = yystrlen (yyformat) - 2 * yycount + 1;
  {
    int yyi;
    for (yyi = 0; yyi < yycount; ++yyi)
      {
        YYPTRDIFF_T yysize1
          = yysize + yytnamerr (YY_NULLPTR, yytname[yyarg[yyi]]);
        if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
          yysize = yysize1;
        else
          return YYENOMEM;
      }
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return -1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yytname[yyarg[yyi++]]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, void *parm)
{
  YY_USE (yyvaluep);
  YY_USE (parm);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (void *parm)
{
/* Lookahead token kind.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (&yylval, YYLEX_PARAM);
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 7: /* vardecl_items: %empty  */
      { initfg(static_cast<ParserState*>(parm)); }
    break;

  case 8: /* vardecl_items: vardecl_items_head  */
      { initfg(static_cast<ParserState*>(parm)); }
    break;

  case 35: /* vardecl_item: FZ_VAR int_ti_expr_tail ':' var_par_id annotations non_array_expr_opt  */
      {
        ParserState* pp = static_cast<ParserState*>(parm);
        bool print = (yyvsp[-1].argVec)->hasAtom("output_var");
        bool introduced = (yyvsp[-1].argVec)->hasAtom("var_is_introduced");
        pp->intvarTable.put((yyvsp[-2].sValue), pp->intvars.size());
        if (print) {
          pp->output(std::string((yyvsp[-2].sValue)), new AST::IntVar(pp->intvars.size()));
        }
        if ((yyvsp[0].oArg)()) {
          AST::Node* arg = (yyvsp[0].oArg).some();
          if (arg->isInt()) {
            pp->intvars.push_back(varspec((yyvsp[-2].sValue),
              new IntVarSpec(arg->getInt(),introduced)));
          } else if (arg->isIntVar()) {
            pp->intvars.push_back(varspec((yyvsp[-2].sValue),
              new IntVarSpec(Alias(arg->getIntVar()),introduced)));
          } else {
            yyassert(pp, false, "Invalid var int initializer.");
          }
          if (!pp->hadError)
            addDomainConstraint(pp, "int_in",
                                new AST::IntVar(pp->intvars.size()-1), (yyvsp[-4].oSet));
          delete arg;
        } else {
          pp->intvars.push_back(varspec((yyvsp[-2].sValue), new IntVarSpec((yyvsp[-4].oSet),introduced)));
        }
        delete (yyvsp[-1].argVec); free((yyvsp[-2].sValue));
      }
    break;

  case 36: /* vardecl_item: FZ_VAR bool_ti_expr_tail ':' var_par_id annotations non_array_expr_opt  */
      {
        ParserState* pp = static_cast<ParserState*>(parm);
        bool print = (yyvsp[-1].argVec)->hasAtom("output_var");
        bool introduced = (yyvsp[-1].argVec)->hasAtom("var_is_introduced");
        pp->boolvarTable.put((yyvsp[-2].sValue), pp->boolvars.size());
        if (print) {
          pp->output(std::string((yyvsp[-2].sValue)), new AST::BoolVar(pp->boolvars.size()));
        }
        if ((yyvsp[0].oArg)()) {
          AST::Node* arg = (yyvsp[0].oArg).some();
          if (arg->isBool()) {
            pp->boolvars.push_back(varspec((yyvsp[-2].sValue),
              new BoolVarSpec(arg->getBool(),introduced)));            
          } else if (arg->isBoolVar()) {
            pp->boolvars.push_back(varspec((yyvsp[-2].sValue),
              new BoolVarSpec(Alias(arg->getBoolVar()),introduced)));
          } else {
            yyassert(pp, false, "Invalid var bool initializer.");
          }
          if (!pp->hadError)
            addDomainConstraint(pp, "int_in",
                                new AST::BoolVar(pp->boolvars.size()-1), (yyvsp[-4].oSet));
          delete arg;
        } else {
          pp->boolvars.push_back(varspec((yyvsp[-2].sValue), new BoolVarSpec((yyvsp[-4].oSet),introduced)));
        }
        delete (yyvsp[-1].argVec); free((yyvsp[-2].sValue));
      }
    break;

  case 37: /* vardecl_item: FZ_VAR float_ti_expr_tail ':' var_par_id annotations non_array_expr_opt  */
      { ParserState* pp = static_cast<ParserState*>(parm);
        yyassert(pp, false, "Floats not supported.");
        delete (yyvsp[-1].argVec); free((yyvsp[-2].sValue));
      }
    break;

  case 38: /* vardecl_item: FZ_VAR FZ_SET FZ_OF int_ti_expr_tail ':' var_par_id annotations non_array_expr_opt  */
      { 
        ParserState* pp = static_cast<ParserState*>(parm);
        bool print = (yyvsp[-1].argVec)->hasAtom("output_var");
        bool introduced = (yyvsp[-1].argVec)->hasAtom("var_is_introduced");
        pp->setvarTable.put((yyvsp[-2].sValue), pp->setvars.size());
        if (print) {
          pp->output(std::string((yyvsp[-2].sValue)), new AST::SetVar(pp->setvars.size()));
        }
        if ((yyvsp[0].oArg)()) {
          AST::Node* arg = (yyvsp[0].oArg).some();
          if (arg->isSet()) {
            pp->setvars.push_back(varspec((yyvsp[-2].sValue),
              new SetVarSpec(arg->getSet(),introduced)));
          } else if (arg->isSetVar()) {
            pp->setvars.push_back(varspec((yyvsp[-2].sValue),
              new SetVarSpec(Alias(arg->getSetVar()),introduced)));
            delete arg;
          } else {
            yyassert(pp, false, "Invalid var set initializer.");
            delete arg;
          }
          if (!pp->hadError)
            addDomainConstraint(pp, "set_subset",
                                new AST::SetVar(pp->setvars.size()-1), (yyvsp[-4].oSet));
        } else {
          pp->setvars.push_back(varspec((yyvsp[-2].sValue), new SetVarSpec((yyvsp[-4].oSet),introduced)));
        }
        delete (yyvsp[-1].argVec); free((yyvsp[-2].sValue));
      }
    break;

  case 39: /* vardecl_item: FZ_INT ':' var_par_id annotations '=' non_array_expr  */
      {
        ParserState* pp = static_cast<ParserState*>(parm);
        yyassert(pp, (yyvsp[0].arg)->isInt(), "Invalid int initializer.");
        pp->intvals.put((yyvsp[-3].sValue), (yyvsp[0].arg)->getInt());
        delete (yyvsp[-2].argVec); free((yyvsp[-3].sValue));        
      }
    break;

  case 40: /* vardecl_item: FZ_BOOL ':' var_par_id annotations '=' non_array_expr  */
      {
        ParserState* pp = static_cast<ParserState*>(parm);
        yyassert(pp, (yyvsp[0].arg)->isBool(), "Invalid bool initializer.");
        pp->boolvals.put((yyvsp[-3].sValue), (yyvsp[0].arg)->getBool());
        delete (yyvsp[-2].argVec); free((yyvsp[-3].sValue));        
      }
    break;

  case 41: /* vardecl_item: FZ_SET FZ_OF FZ_INT ':' var_par_id annotations '=' non_array_expr  */
      {
        ParserState* pp = static_cast<ParserState*>(parm);
        yyassert(pp, (yyvsp[0].arg)->isSet(), "Invalid set initializer.");
        AST::SetLit* set = (yyvsp[0].arg)->getSet();
        pp->setvals.put((yyvsp[-3].sValue), *set);
        delete set;
        delete (yyvsp[-2].argVec); free((yyvsp[-3].sValue));        
      }
    break;

  case 42: /* vardecl_item: FZ_ARRAY '[' FZ_INT_LIT FZ_DOTDOT FZ_INT_LIT ']' FZ_OF FZ_VAR int_ti_expr_tail ':' var_par_id annotations vardecl_int_var_array_init  */
      {
        ParserState* pp = static_cast<ParserState*>(parm);
        yyassert(pp, (yyvsp[-10].iValue)==1, "Arrays must start at 1");
        if (!pp->hadError) {
          bool print = (yyvsp[-1].argVec)->hasCall("output_array");
          vector<int> vars((yyvsp[-8].iValue));
          if (!pp->hadError) {
            if ((yyvsp[0].oVarSpecVec)()) {
              vector<VarSpec*>* vsv = (yyvsp[0].oVarSpecVec).some();
              yyassert(pp, vsv->size() == static_cast<unsigned int>((yyvsp[-8].iValue)),
                       "Initializer size does not match array dimension");
              if (!pp->hadError) {
                for (int i=0; i<(yyvsp[-8].iValue); i++) {
                  IntVarSpec* ivsv = static_cast<IntVarSpec*>((*vsv)[i]);
                  if (ivsv->alias) {
                    vars[i] = ivsv->i;
                  } else {
                    vars[i] = pp->intvars.size();
                    pp->intvars.push_back(varspec((yyvsp[-2].sValue), ivsv));
                  }
                  if (!pp->hadError && (yyvsp[-4].oSet)()) {
                    Option<AST::SetLit*> opt =
                      Option<AST::SetLit*>::some(new AST::SetLit(*(yyvsp[-4].oSet).some()));                    
                    addDomainConstraint(pp, "int_in",
                                        new AST::IntVar(vars[i]),
                                        opt);
                  }
                }
              }
              delete vsv;
            } else {
              if ((yyvsp[-8].iValue)>0) {
                IntVarSpec* ispec = new IntVarSpec((yyvsp[-4].oSet),!print);
                string arrayname = "["; arrayname += (yyvsp[-2].sValue);
                for (int i=0; i<(yyvsp[-8].iValue)-1; i++) {
                  vars[i] = pp->intvars.size();
                  pp->intvars.push_back(varspec(arrayname, ispec));
                }
                vars[(yyvsp[-8].iValue)-1] = pp->intvars.size();
                pp->intvars.push_back(varspec((yyvsp[-2].sValue), ispec));
              }
            }
          }
          if (print) {
            AST::Array* a = new AST::Array();
            a->a.push_back(arrayOutput((yyvsp[-1].argVec)->getCall("output_array")));
            AST::Array* output = new AST::Array();
            for (int i=0; i<(yyvsp[-8].iValue); i++)
              output->a.push_back(new AST::IntVar(vars[i]));
            a->a.push_back(output);
            a->a.push_back(new AST::String(")"));
            pp->output(std::string((yyvsp[-2].sValue)), a);
          }
          pp->intvararrays.put((yyvsp[-2].sValue), vars);
        }
        delete (yyvsp[-1].argVec); free((yyvsp[-2].sValue));
      }
    break;

  case 43: /* vardecl_item: FZ_ARRAY '[' FZ_INT_LIT FZ_DOTDOT FZ_INT_LIT ']' FZ_OF FZ_VAR bool_ti_expr_tail ':' var_par_id annotations vardecl_bool_var_array_init  */
      {
        ParserState* pp = static_cast<ParserState*>(parm);
        bool print = (yyvsp[-1].argVec)->hasCall("output_array");
        yyassert(pp, (yyvsp[-10].iValue)==1, "Arrays must start at 1");
        if (!pp->hadError) {
          vector<int> vars((yyvsp[-8].iValue));
          if ((yyvsp[0].oVarSpecVec)()) {
            vector<VarSpec*>* vsv = (yyvsp[0].oVarSpecVec).some();
            yyassert(pp, vsv->size() == static_cast<unsigned int>((yyvsp[-8].iValue)),
                     "Initializer size does not match array dimension");
            if (!pp->hadError) {
              for (int i=0; i<(yyvsp[-8].iValue); i++) {
                BoolVarSpec* bvsv = static_cast<BoolVarSpec*>((*vsv)[i]);
                if (bvsv->alias)
                  vars[i] = bvsv->i;
                else {
                  vars[i] = pp->boolvars.size();
                  pp->boolvars.push_back(varspec((yyvsp[-2].sValue), (*vsv)[i]));
                }
                if (!pp->hadError && (yyvsp[-4].oSet)()) {
                  Option<AST::SetLit*> opt =
                    Option<AST::SetLit*>::some(new AST::SetLit(*(yyvsp[-4].oSet).some()));                    
                  addDomainConstraint(pp, "int_in",
                                      new AST::BoolVar(vars[i]),
                                      opt);
                }
              }
            }
            delete vsv;
          } else {
            for (int i=0; i<(yyvsp[-8].iValue); i++) {
              vars[i] = pp->boolvars.size();
              pp->boolvars.push_back(varspec((yyvsp[-2].sValue),
                                             new BoolVarSpec((yyvsp[-4].oSet),!print)));
            }          
          }
          if (print) {
            AST::Array* a = new AST::Array();
            a->a.push_back(arrayOutput((yyvsp[-1].argVec)->getCall("output_array")));
            AST::Array* output = new AST::Array();
            for (int i=0; i<(yyvsp[-8].iValue); i++)
              output->a.push_back(new AST::BoolVar(vars[i]));
            a->a.push_back(output);
            a->a.push_back(new AST::String(")"));
            pp->output(std::string((yyvsp[-2].sValue)), a);
          }
          pp->boolvararrays.put((yyvsp[-2].sValue), vars);
        }
        delete (yyvsp[-1].argVec); free((yyvsp[-2].sValue));
      }
    break;

  case 44: /* vardecl_item: FZ_ARRAY '[' FZ_INT_LIT FZ_DOTDOT FZ_INT_LIT ']' FZ_OF FZ_VAR float_ti_expr_tail ':' var_par_id annotations vardecl_float_var_array_init  */
      { 
        ParserState* pp = static_cast<ParserState*>(parm);
        yyassert(pp, false, "Floats not supported.");
        delete (yyvsp[-1].argVec); free((yyvsp[-2].sValue));
      }
    break;

  case 45: /* vardecl_item: FZ_ARRAY '[' FZ_INT_LIT FZ_DOTDOT FZ_INT_LIT ']' FZ_OF FZ_VAR FZ_SET FZ_OF int_ti_expr_tail ':' var_par_id annotations vardecl_set_var_array_init  */
      { 
        ParserState* pp = static_cast<ParserState*>(parm);
        bool print = (yyvsp[-1].argVec)->hasCall("output_array");
        yyassert(pp, (yyvsp[-12].iValue)==1, "Arrays must start at 1");
        if (!pp->hadError) {
          vector<int> vars((yyvsp[-10].iValue));
          if ((yyvsp[0].oVarSpecVec)()) {
            vector<VarSpec*>* vsv = (yyvsp[0].oVarSpecVec).some();
            yyassert(pp, vsv->size() == static_cast<unsigned int>((yyvsp[-10].iValue)),
                     "Initializer size does not match array dimension");
            if (!pp->hadError) {
              for (int i=0; i<(yyvsp[-10].iValue); i++) {
                SetVarSpec* svsv = static_cast<SetVarSpec*>((*vsv)[i]);
                if (svsv->alias)
                  vars[i] = svsv->i;
                else {
                  vars[i] = pp->setvars.size();
                  pp->setvars.push_back(varspec((yyvsp[-2].sValue), (*vsv)[i]));
                }
                if (!pp->hadError && (yyvsp[-4].oSet)()) {
                  Option<AST::SetLit*> opt =
                    Option<AST::SetLit*>::some(new AST::SetLit(*(yyvsp[-4].oSet).some()));                    
                  addDomainConstraint(pp, "set_subset",
                                      new AST::SetVar(vars[i]),
                                      opt);
                }
              }
            }
            delete vsv;
          } else {
            if ((yyvsp[-10].iValue)>0) {
              SetVarSpec* ispec = new SetVarSpec((yyvsp[-4].oSet),!print);
              string arrayname = "["; arrayname += (yyvsp[-2].sValue);
              for (int i=0; i<(yyvsp[-10].iValue)-1; i++) {
                vars[i] = pp->setvars.size();
                pp->setvars.push_back(varspec(arrayname, ispec));
              }
              vars[(yyvsp[-10].iValue)-1] = pp->setvars.size();
              pp->setvars.push_back(varspec((yyvsp[-2].sValue), ispec));
            }
          }
          if (print) {
            AST::Array* a = new AST::Array();
            a->a.push_back(arrayOutput((yyvsp[-1].argVec)->getCall("output_array")));
            AST::Array* output = new AST::Array();
            for (int i=0; i<(yyvsp[-10].iValue); i++)
              output->a.push_back(new AST::SetVar(vars[i]));
            a->a.push_back(output);
            a->a.push_back(new AST::String(")"));
            pp->output(std::string((yyvsp[-2].sValue)), a);
          }
          pp->setvararrays.put((yyvsp[-2].sValue), vars);
        }
        delete (yyvsp[-1].argVec); free((yyvsp[-2].sValue));
      }
    break;

  case 46: /* vardecl_item: FZ_ARRAY '[' FZ_INT_LIT FZ_DOTDOT FZ_INT_LIT ']' FZ_OF FZ_INT ':' var_par_id annotations '=' '[' int_list ']'  */
      {
        ParserState* pp = static_cast<ParserState*>(parm);
        yyassert(pp, (yyvsp[-12].iValue)==1, "Arrays must start at 1");
        yyassert(pp, (yyvsp[-1].setValue)->size() == static_cast<unsigned int>((yyvsp[-10].iValue)),
                 "Initializer size does not match array dimension");
        if (!pp->hadError)
          pp->intvalarrays.put((yyvsp[-5].sValue), *(yyvsp[-1].setValue));
        delete (yyvsp[-1].setValue);
        free((yyvsp[-5].sValue));
        delete (yyvsp[-4].argVec);
      }
    break;

  case 47: /* vardecl_item: FZ_ARRAY '[' FZ_INT_LIT FZ_DOTDOT FZ_INT_LIT ']' FZ_OF FZ_BOOL ':' var_par_id annotations '=' '[' bool_list ']'  */
      {
        ParserState* pp = static_cast<ParserState*>(parm);
        yyassert(pp, (yyvsp[-12].iValue)==1, "Arrays must start at 1");
        yyassert(pp, (yyvsp[-1].setValue)->size() == static_cast<unsigned int>((yyvsp[-10].iValue)),
                 "Initializer size does not match array dimension");
        if (!pp->hadError)
          pp->boolvalarrays.put((yyvsp[-5].sValue), *(yyvsp[-1].setValue));
        delete (yyvsp[-1].setValue);
        free((yyvsp[-5].sValue));
        delete (yyvsp[-4].argVec);
      }
    break;

  case 48: /* vardecl_item: FZ_ARRAY '[' FZ_INT_LIT FZ_DOTDOT FZ_INT_LIT ']' FZ_OF FZ_FLOAT ':' var_par_id annotations '=' '[' float_list ']'  */
      {
        ParserState* pp = static_cast<ParserState*>(parm);
        yyassert(pp, false, "Floats not supported.");
        delete (yyvsp[-4].argVec); free((yyvsp[-5].sValue));
      }
    break;

  case 49: /* vardecl_item: FZ_ARRAY '[' FZ_INT_LIT FZ_DOTDOT FZ_INT_LIT ']' FZ_OF FZ_SET FZ_OF FZ_INT ':' var_par_id annotations '=' '[' set_literal_list ']'  */
      {
        ParserState* pp = static_cast<ParserState*>(parm);
        yyassert(pp, (yyvsp[-14].iValue)==1, "Arrays must start at 1");
        yyassert(pp, (yyvsp[-1].setValueList)->size() == static_cast<unsigned int>((yyvsp[-12].iValue)),
                 "Initializer size does not match array dimension");
        if (!pp->hadError)
          pp->setvalarrays.put((yyvsp[-5].sValue), *(yyvsp[-1].setValueList));
        delete (yyvsp[-1].setValueList);
        delete (yyvsp[-4].argVec); free((yyvsp[-5].sValue));
      }
    break;

  case 50: /* int_init: FZ_INT_LIT  */
      { 
        (yyval.varSpec) = new IntVarSpec((yyvsp[0].iValue),false);
      }
    break;

  case 51: /* int_init: var_par_id  */
      { 
        int v = 0;
        ParserState* pp = static_cast<ParserState*>(parm);
        if (pp->intvarTable.get((yyvsp[0].sValue), v))
          (yyval.varSpec) = new IntVarSpec(Alias(v),false);
        else {
          pp->err << "Error: undefined identifier " << (yyvsp[0].sValue)
                  << " in line no. "
                  << yyget_lineno(pp->yyscanner) << std::endl;
          pp->hadError = true;
          (yyval.varSpec) = new IntVarSpec(0,false); // keep things consistent
        }
        free((yyvsp[0].sValue));
      }
    break;

  case 52: /* int_init: var_par_id '[' FZ_INT_LIT ']'  */
      { 
        vector<int> v;
        ParserState* pp = static_cast<ParserState*>(parm);
        if (pp->intvararrays.get((yyvsp[-3].sValue), v)) {
          yyassert(pp,static_cast<unsigned int>((yyvsp[-1].iValue)) > 0 && 
                      static_cast<unsigned int>((yyvsp[-1].iValue)) <= v.size(),
                   "array access out of bounds");
          if (!pp->hadError)
            (yyval.varSpec) = new IntVarSpec(Alias(v[(yyvsp[-1].iValue)-1]),false);
          else
            (yyval.varSpec) = new IntVarSpec(0,false); // keep things consistent
        } else {
          pp->err << "Error: undefined array identifier " << (yyvsp[-3].sValue)
                  << " in line no. "
                  << yyget_lineno(pp->yyscanner) << std::endl;
          pp->hadError = true;
          (yyval.varSpec) = new IntVarSpec(0,false); // keep things consistent
        }
        free((yyvsp[-3].sValue));
      }
    break;

  case 53: /* int_init_list: %empty  */
      { (yyval.varSpecVec) = new vector<VarSpec*>(0); }
    break;

  case 54: /* int_init_list: int_init_list_head list_tail  */
      { (yyval.varSpecVec) = (yyvsp[-1].varSpecVec); }
    break;

  case 55: /* int_init_list_head: int_init  */
      { (yyval.varSpecVec) = new vector<VarSpec*>(1); (*(yyval.varSpecVec))[0] = (yyvsp[0].varSpec); }
    break;

  case 56: /* int_init_list_head: int_init_list_head ',' int_init  */
      { (yyval.varSpecVec) = (yyvsp[-2].varSpecVec); (yyval.varSpecVec)->push_back((yyvsp[0].varSpec)); }
    break;

  case 59: /* int_var_array_literal: '[' int_init_list ']'  */
      { (yyval.varSpecVec) = (yyvsp[-1].varSpecVec); }
    break;

  case 60: /* float_init: FZ_FLOAT_LIT  */
      { (yyval.varSpec) = new FloatVarSpec((yyvsp[0].dValue),false); }
    break;

  case 61: /* float_init: var_par_id  */
      { 
        int v = 0;
        ParserState* pp = static_cast<ParserState*>(parm);
        if (pp->floatvarTable.get((yyvsp[0].sValue), v))
          (yyval.varSpec) = new FloatVarSpec(Alias(v),false);
        else {
          pp->err << "Error: undefined identifier " << (yyvsp[0].sValue)
                  << " in line no. "
                  << yyget_lineno(pp->yyscanner) << std::endl;
          pp->hadError = true;
          (yyval.varSpec) = new FloatVarSpec(0.0,false);
        }
        free((yyvsp[0].sValue));
      }
    break;

  case 62: /* float_init: var_par_id '[' FZ_INT_LIT ']'  */
      { 
        vector<int> v;
        ParserState* pp = static_cast<ParserState*>(parm);
        if (pp->floatvararrays.get((yyvsp[-3].sValue), v)) {
          yyassert(pp,static_cast<unsigned int>((yyvsp[-1].iValue)) > 0 && 
                      static_cast<unsigned int>((yyvsp[-1].iValue)) <= v.size(),
                   "array access out of bounds");
          if (!pp->hadError)
            (yyval.varSpec) = new FloatVarSpec(Alias(v[(yyvsp[-1].iValue)-1]),false);
          else
            (yyval.varSpec) = new FloatVarSpec(0.0,false);
        } else {
          pp->err << "Error: undefined array identifier " << (yyvsp[-3].sValue)
                  << " in line no. "
                  << yyget_lineno(pp->yyscanner) << std::endl;
          pp->hadError = true;
          (yyval.varSpec) = new FloatVarSpec(0.0,false);
        }
        free((yyvsp[-3].sValue));
      }
    break;

  case 63: /* float_init_list: %empty  */
      { (yyval.varSpecVec) = new vector<VarSpec*>(0); }
    break;

  case 64: /* float_init_list: float_init_list_head list_tail  */
      { (yyval.varSpecVec) = (yyvsp[-1].varSpecVec); }
    break;

  case 65: /* float_init_list_head: float_init  */
      { (yyval.varSpecVec) = new vector<VarSpec*>(1); (*(yyval.varSpecVec))[0] = (yyvsp[0].varSpec); }
    break;

  case 66: /* float_init_list_head: float_init_list_head ',' float_init  */
      { (yyval.varSpecVec) = (yyvsp[-2].varSpecVec); (yyval.varSpecVec)->push_back((yyvsp[0].varSpec)); }
    break;

  case 67: /* float_var_array_literal: '[' float_init_list ']'  */
      { (yyval.varSpecVec) = (yyvsp[-1].varSpecVec); }
    break;

  case 68: /* bool_init: FZ_BOOL_LIT  */
      { (yyval.varSpec) = new BoolVarSpec((yyvsp[0].iValue),false); }
    break;

  case 69: /* bool_init: var_par_id  */
      { 
        int v = 0;
        ParserState* pp = static_cast<ParserState*>(parm);
        if (pp->boolvarTable.get((yyvsp[0].sValue), v))
          (yyval.varSpec) = new BoolVarSpec(Alias(v),false);
        else {
          pp->err << "Error: undefined identifier " << (yyvsp[0].sValue)
                  << " in line no. "
                  << yyget_lineno(pp->yyscanner) << std::endl;
          pp->hadError = true;
          (yyval.varSpec) = new BoolVarSpec(false,false);
        }
        free((yyvsp[0].sValue));
      }
    break;

  case 70: /* bool_init: var_par_id '[' FZ_INT_LIT ']'  */
      { 
        vector<int> v;
        ParserState* pp = static_cast<ParserState*>(parm);
        if (pp->boolvararrays.get((yyvsp[-3].sValue), v)) {
          yyassert(pp,static_cast<unsigned int>((yyvsp[-1].iValue)) > 0 && 
                      static_cast<unsigned int>((yyvsp[-1].iValue)) <= v.size(),
                   "array access out of bounds");
          if (!pp->hadError)
            (yyval.varSpec) = new BoolVarSpec(Alias(v[(yyvsp[-1].iValue)-1]),false);
          else
            (yyval.varSpec) = new BoolVarSpec(false,false);
        } else {
          pp->err << "Error: undefined array identifier " << (yyvsp[-3].sValue)
                  << " in line no. "
                  << yyget_lineno(pp->yyscanner) << std::endl;
          pp->hadError = true;
          (yyval.varSpec) = new BoolVarSpec(false,false);
        }
        free((yyvsp[-3].sValue));
      }
    break;

  case 71: /* bool_init_list: %empty  */
      { (yyval.varSpecVec) = new vector<VarSpec*>(0); }
    break;

  case 72: /* bool_init_list: bool_init_list_head list_tail  */
      { (yyval.varSpecVec) = (yyvsp[-1].varSpecVec); }
    break;

  case 73: /* bool_init_list_head: bool_init  */
      { (yyval.varSpecVec) = new vector<VarSpec*>(1); (*(yyval.varSpecVec))[0] = (yyvsp[0].varSpec); }
    break;

  case 74: /* bool_init_list_head: bool_init_list_head ',' bool_init  */
      { (yyval.varSpecVec) = (yyvsp[-2].varSpecVec); (yyval.varSpecVec)->push_back((yyvsp[0].varSpec)); }
    break;

  case 75: /* bool_var_array_literal: '[' bool_init_list ']'  */
                                                { (yyval.varSpecVec) = (yyvsp[-1].varSpecVec); }
    break;

  case 76: /* set_init: set_literal  */
      { (yyval.varSpec) = new SetVarSpec((yyvsp[0].setLit),false); }
    break;

  case 77: /* set_init: var_par_id  */
      { 
        ParserState* pp = static_cast<ParserState*>(parm);
        int v = 0;
        if (pp->setvarTable.get((yyvsp[0].sValue), v))
          (yyval.varSpec) = new SetVarSpec(Alias(v),false);
        else {
          pp->err << "Error: undefined identifier " << (yyvsp[0].sValue)
                  << " in line no. "
                  << yyget_lineno(pp->yyscanner) << std::endl;
          pp->hadError = true;
          (yyval.varSpec) = new SetVarSpec(Alias(0),false);
        }
        free((yyvsp[0].sValue));
      }
    break;

  case 78: /* set_init: var_par_id '[' FZ_INT_LIT ']'  */
      { 
        vector<int> v;
        ParserState* pp = static_cast<ParserState*>(parm);
        if (pp->setvararrays.get((yyvsp[-3].sValue), v)) {
          yyassert(pp,static_cast<unsigned int>((yyvsp[-1].iValue)) > 0 && 
                      static_cast<unsigned int>((yyvsp[-1].iValue)) <= v.size(),
                   "array access out of bounds");
          if (!pp->hadError)
            (yyval.varSpec) = new SetVarSpec(Alias(v[(yyvsp[-1].iValue)-1]),false);
          else
            (yyval.varSpec) = new SetVarSpec(Alias(0),false);
        } else {
          pp->err << "Error: undefined array identifier " << (yyvsp[-3].sValue)
                  << " in line no. "
                  << yyget_lineno(pp->yyscanner) << std::endl;
          pp->hadError = true;
          (yyval.varSpec) = new SetVarSpec(Alias(0),false);
        }
        free((yyvsp[-3].sValue));
      }
    break;

  case 79: /* set_init_list: %empty  */
      { (yyval.varSpecVec) = new vector<VarSpec*>(0); }
    break;

  case 80: /* set_init_list: set_init_list_head list_tail  */
      { (yyval.varSpecVec) = (yyvsp[-1].varSpecVec); }
    break;

  case 81: /* set_init_list_head: set_init  */
      { (yyval.varSpecVec) = new vector<VarSpec*>(1); (*(yyval.varSpecVec))[0] = (yyvsp[0].varSpec); }
    break;

  case 82: /* set_init_list_head: set_init_list_head ',' set_init  */
      { (yyval.varSpecVec) = (yyvsp[-2].varSpecVec); (yyval.varSpecVec)->push_back((yyvsp[0].varSpec)); }
    break;

  case 83: /* set_var_array_literal: '[' set_init_list ']'  */
      { (yyval.varSpecVec) = (yyvsp[-1].varSpecVec); }
    break;

  case 84: /* vardecl_int_var_array_init: %empty  */
      { (yyval.oVarSpecVec) = Option<vector<VarSpec*>* >::none(); }
    break;

  case 85: /* vardecl_int_var_array_init: '=' int_var_array_literal  */
      { (yyval.oVarSpecVec) = Option<vector<VarSpec*>* >::some((yyvsp[0].varSpecVec)); }
    break;

  case 86: /* vardecl_bool_var_array_init: %empty  */
      { (yyval.oVarSpecVec) = Option<vector<VarSpec*>* >::none(); }
    break;

  case 87: /* vardecl_bool_var_array_init: '=' bool_var_array_literal  */
      { (yyval.oVarSpecVec) = Option<vector<VarSpec*>* >::some((yyvsp[0].varSpecVec)); }
    break;

  case 88: /* vardecl_float_var_array_init: %empty  */
      { (yyval.oVarSpecVec) = Option<vector<VarSpec*>* >::none(); }
    break;

  case 89: /* vardecl_float_var_array_init: '=' float_var_array_literal  */
      { (yyval.oVarSpecVec) = Option<vector<VarSpec*>* >::some((yyvsp[0].varSpecVec)); }
    break;

  case 90: /* vardecl_set_var_array_init: %empty  */
      { (yyval.oVarSpecVec) = Option<vector<VarSpec*>* >::none(); }
    break;

  case 91: /* vardecl_set_var_array_init: '=' set_var_array_literal  */
      { (yyval.oVarSpecVec) = Option<vector<VarSpec*>* >::some((yyvsp[0].varSpecVec)); }
    break;

  case 92: /* constraint_item: FZ_CONSTRAINT FZ_ID '(' flat_expr_list ')' annotations  */
      { 
        ConExpr c((yyvsp[-4].sValue), (yyvsp[-2].argVec));
        ParserState *pp = static_cast<ParserState*>(parm);
        if (!pp->hadError) {
          try {
            pp->fg->addConstraint(c, (yyvsp[0].argVec));
          } catch (FlatZinc::Error& e) {
            yyerror(pp, e.toString().c_str());
          }
        }
        delete (yyvsp[0].argVec); free((yyvsp[-4].sValue));
      }
    break;

  case 93: /* solve_item: FZ_SOLVE annotations FZ_SATISFY  */
      { 
        ParserState *pp = static_cast<ParserState*>(parm);
        if (!pp->hadError) {
          try {
            pp->fg->solve((yyvsp[-1].argVec));
          } catch (FlatZinc::Error& e) {
            yyerror(pp, e.toString().c_str());
          }
        } else {
          delete (yyvsp[-1].argVec);
        }
      }
    break;

  case 94: /* solve_item: FZ_SOLVE annotations minmax solve_expr  */
      { 
        ParserState *pp = static_cast<ParserState*>(parm);
        if (!pp->hadError) {
          try {
            if ((yyvsp[-1].bValue))
              pp->fg->minimize((yyvsp[0].iValue),(yyvsp[-2].argVec));
            else
              pp->fg->maximize((yyvsp[0].iValue),(yyvsp[-2].argVec));
          } catch (FlatZinc::Error& e) {
            yyerror(pp, e.toString().c_str());
          }
        } else {
          delete (yyvsp[-2].argVec);
        }
      }
    break;

  case 95: /* int_ti_expr_tail: FZ_INT  */
      { (yyval.oSet) = Option<AST::SetLit* >::none(); }
    break;

  case 96: /* int_ti_expr_tail: '{' int_list '}'  */
      { (yyval.oSet) = Option<AST::SetLit* >::some(new AST::SetLit(*(yyvsp[-1].setValue))); }
    break;

  case 97: /* int_ti_expr_tail: FZ_INT_LIT FZ_DOTDOT FZ_INT_LIT  */
      { 
        (yyval.oSet) = Option<AST::SetLit* >::some(new AST::SetLit((yyvsp[-2].iValue), (yyvsp[0].iValue)));
      }
    break;

  case 98: /* bool_ti_expr_tail: FZ_BOOL  */
      { (yyval.oSet) = Option<AST::SetLit* >::none(); }
    break;

  case 99: /* bool_ti_expr_tail: '{' bool_list_head list_tail '}'  */
      { bool haveTrue = false;
        bool haveFalse = false;
        for (int i=(yyvsp[-2].setValue)->size(); i--;) {
          haveTrue |= ((*(yyvsp[-2].setValue))[i] == 1);
          haveFalse |= ((*(yyvsp[-2].setValue))[i] == 0);
        }
        delete (yyvsp[-2].setValue);
        (yyval.oSet) = Option<AST::SetLit* >::some(
          new AST::SetLit(!haveFalse,haveTrue));
      }
    break;

  case 102: /* set_literal: '{' int_list '}'  */
      { (yyval.setLit) = new AST::SetLit(*(yyvsp[-1].setValue)); }
    break;

  case 103: /* set_literal: FZ_INT_LIT FZ_DOTDOT FZ_INT_LIT  */
      { (yyval.setLit) = new AST::SetLit((yyvsp[-2].iValue), (yyvsp[0].iValue)); }
    break;

  case 104: /* int_list: %empty  */
      { (yyval.setValue) = new vector<int>(0); }
    break;

  case 105: /* int_list: int_list_head list_tail  */
      { (yyval.setValue) = (yyvsp[-1].setValue); }
    break;

  case 106: /* int_list_head: FZ_INT_LIT  */
      { (yyval.setValue) = new vector<int>(1); (*(yyval.setValue))[0] = (yyvsp[0].iValue); }
    break;

  case 107: /* int_list_head: int_list_head ',' FZ_INT_LIT  */
      { (yyval.setValue) = (yyvsp[-2].setValue); (yyval.setValue)->push_back((yyvsp[0].iValue)); }
    break;

  case 108: /* bool_list: %empty  */
      { (yyval.setValue) = new vector<int>(0); }
    break;

  case 109: /* bool_list: bool_list_head list_tail  */
      { (yyval.setValue) = (yyvsp[-1].setValue); }
    break;

  case 110: /* bool_list_head: FZ_BOOL_LIT  */
      { (yyval.setValue) = new vector<int>(1); (*(yyval.setValue))[0] = (yyvsp[0].iValue); }
    break;

  case 111: /* bool_list_head: bool_list_head ',' FZ_BOOL_LIT  */
      { (yyval.setValue) = (yyvsp[-2].setValue); (yyval.setValue)->push_back((yyvsp[0].iValue)); }
    break;

  case 112: /* float_list: %empty  */
      { (yyval.floatSetValue) = new vector<double>(0); }
    break;

  case 113: /* float_list: float_list_head list_tail  */
      { (yyval.floatSetValue) = (yyvsp[-1].floatSetValue); }
    break;

  case 114: /* float_list_head: FZ_FLOAT_LIT  */
      { (yyval.floatSetValue) = new vector<double>(1); (*(yyval.floatSetValue))[0] = (yyvsp[0].dValue); }
    break;

  case 115: /* float_list_head: float_list_head ',' FZ_FLOAT_LIT  */
      { (yyval.floatSetValue) = (yyvsp[-2].floatSetValue); (yyval.floatSetValue)->push_back((yyvsp[0].dValue)); }
    break;

  case 116: /* set_literal_list: %empty  */
      { (yyval.setValueList) = new vector<AST::SetLit>(0); }
    break;

  case 117: /* set_literal_list: set_literal_list_head list_tail  */
      { (yyval.setValueList) = (yyvsp[-1].setValueList); }
    break;

  case 118: /* set_literal_list_head: set_literal  */
      { (yyval.setValueList) = new vector<AST::SetLit>(1); (*(yyval.setValueList))[0] = *(yyvsp[0].setLit); delete (yyvsp[0].setLit); }
    break;

  case 119: /* set_literal_list_head: set_literal_list_head ',' set_literal  */
      { (yyval.setValueList) = (yyvsp[-2].setValueList); (yyval.setValueList)->push_back(*(yyvsp[0].setLit)); delete (yyvsp[0].setLit); }
    break;

  case 120: /* flat_expr_list: flat_expr  */
      { (yyval.argVec) = new AST::Array((yyvsp[0].arg)); }
    break;

  case 121: /* flat_expr_list: flat_expr_list ',' flat_expr  */
      { (yyval.argVec) = (yyvsp[-2].argVec); (yyval.argVec)->append((yyvsp[0].arg)); }
    break;

  case 122: /* flat_expr: non_array_expr  */
      { (yyval.arg) = (yyvsp[0].arg); }
    break;

  case 123: /* flat_expr: '[' non_array_expr_list ']'  */
      { (yyval.arg) = (yyvsp[-1].argVec); }
    break;

  case 124: /* non_array_expr_opt: %empty  */
      { (yyval.oArg) = Option<AST::Node*>::none(); }
    break;

  case 125: /* non_array_expr_opt: '=' non_array_expr  */
      { (yyval.oArg) = Option<AST::Node*>::some((yyvsp[0].arg)); }
    break;

  case 126: /* non_array_expr: FZ_BOOL_LIT  */
      { (yyval.arg) = new AST::BoolLit((yyvsp[0].iValue)); }
    break;

  case 127: /* non_array_expr: FZ_INT_LIT  */
      { (yyval.arg) = new AST::IntLit((yyvsp[0].iValue)); }
    break;

  case 128: /* non_array_expr: FZ_FLOAT_LIT  */
      { (yyval.arg) = new AST::FloatLit((yyvsp[0].dValue)); }
    break;

  case 129: /* non_array_expr: set_literal  */
      { (yyval.arg) = (yyvsp[0].setLit); }
    break;

  case 130: /* non_array_expr: var_par_id  */
      { 
        vector<int> as;
        ParserState* pp = static_cast<ParserState*>(parm);
        if (pp->intvararrays.get((yyvsp[0].sValue), as)) {
          AST::Array *ia = new AST::Array(as.size());
          for (int i=as.size(); i--;)
            ia->a[i] = new AST::IntVar(as[i]);
          (yyval.arg) = ia;
        } else if (pp->boolvararrays.get((yyvsp[0].sValue), as)) {
          AST::Array *ia = new AST::Array(as.size());
          for (int i=as.size(); i--;)
            ia->a[i] = new AST::BoolVar(as[i]);
          (yyval.arg) = ia;
        } else if (pp->setvararrays.get((yyvsp[0].sValue), as)) {
          AST::Array *ia = new AST::Array(as.size());
          for (int i=as.size(); i--;)
            ia->a[i] = new AST::SetVar(as[i]);
          (yyval.arg) = ia;
        } else {
          std::vector<int> is;
          std::vector<AST::SetLit> isS;
          int ival = 0;
          bool bval = false;
          if (pp->intvalarrays.get((yyvsp[0].sValue), is)) {
            AST::Array *v = new AST::Array(is.size());
            for (int i=is.size(); i--;)
              v->a[i] = new AST::IntLit(is[i]);
            (yyval.arg) = v;
          } else if (pp->boolvalarrays.get((yyvsp[0].sValue), is)) {
            AST::Array *v = new AST::Array(is.size());
            for (int i=is.size(); i--;)
              v->a[i] = new AST::BoolLit(is[i]);
            (yyval.arg) = v;
          } else if (pp->setvalarrays.get((yyvsp[0].sValue), isS)) {
            AST::Array *v = new AST::Array(isS.size());
            for (int i=isS.size(); i--;)
              v->a[i] = new AST::SetLit(isS[i]);
            (yyval.arg) = v;            
          } else if (pp->intvals.get((yyvsp[0].sValue), ival)) {
            (yyval.arg) = new AST::IntLit(ival);
          } else if (pp->boolvals.get((yyvsp[0].sValue), bval)) {
            (yyval.arg) = new AST::BoolLit(bval);
          } else {
            (yyval.arg) = getVarRefArg(pp,(yyvsp[0].sValue));
          }
        }
        free((yyvsp[0].sValue));
      }
    break;

  case 131: /* non_array_expr: var_par_id '[' non_array_expr ']'  */
      { 
        ParserState* pp = static_cast<ParserState*>(parm);
        int i = -1;
        yyassert(pp, (yyvsp[-1].arg)->isInt(i), "Non-integer array index.");
        if (!pp->hadError)
          (yyval.arg) = getArrayElement(static_cast<ParserState*>(parm),(yyvsp[-3].sValue),i);
        else
          (yyval.arg) = new AST::IntLit(0); // keep things consistent
        free((yyvsp[-3].sValue));
      }
    break;

  case 132: /* non_array_expr_list: %empty  */
      { (yyval.argVec) = new AST::Array(0); }
    break;

  case 133: /* non_array_expr_list: non_array_expr_list_head list_tail  */
      { (yyval.argVec) = (yyvsp[-1].argVec); }
    break;

  case 134: /* non_array_expr_list_head: non_array_expr  */
      { (yyval.argVec) = new AST::Array((yyvsp[0].arg)); }
    break;

  case 135: /* non_array_expr_list_head: non_array_expr_list_head ',' non_array_expr  */
      { (yyval.argVec) = (yyvsp[-2].argVec); (yyval.argVec)->append((yyvsp[0].arg)); }
    break;

  case 136: /* solve_expr: var_par_id  */
      { 
        ParserState *pp = static_cast<ParserState*>(parm);
        if (!pp->intvarTable.get((yyvsp[0].sValue), (yyval.iValue))) {
          pp->err << "Error: unknown integer variable " << (yyvsp[0].sValue)
                  << " in line no. "
                  << yyget_lineno(pp->yyscanner) << std::endl;
          pp->hadError = true;
        }
        free((yyvsp[0].sValue));
      }
    break;

  case 137: /* solve_expr: var_par_id '[' FZ_INT_LIT ']'  */
      {
        vector<int> tmp;
        ParserState *pp = static_cast<ParserState*>(parm);
        if (!pp->intvararrays.get((yyvsp[-3].sValue), tmp)) {
          pp->err << "Error: unknown integer variable array " << (yyvsp[-3].sValue)
                  << " in line no. "
                  << yyget_lineno(pp->yyscanner) << std::endl;
          pp->hadError = true;
        }
        if ((yyvsp[-1].iValue) == 0 || static_cast<unsigned int>((yyvsp[-1].iValue)) > tmp.size()) {
          pp->err << "Error: array index out of bounds for array " << (yyvsp[-3].sValue)
                  << " in line no. "
                  << yyget_lineno(pp->yyscanner) << std::endl;
          pp->hadError = true;
        } else {
          (yyval.iValue) = tmp[(yyvsp[-1].iValue)-1];
        }
        free((yyvsp[-3].sValue));
      }
    break;

  case 140: /* annotations: %empty  */
      { (yyval.argVec) = NULL; }
    break;

  case 141: /* annotations: annotations_head  */
      { (yyval.argVec) = (yyvsp[0].argVec); }
    break;

  case 142: /* annotations_head: FZ_COLONCOLON annotation  */
      { (yyval.argVec) = new AST::Array((yyvsp[0].arg)); }
    break;

  case 143: /* annotations_head: annotations_head FZ_COLONCOLON annotation  */
      { (yyval.argVec) = (yyvsp[-2].argVec); (yyval.argVec)->append((yyvsp[0].arg)); }
    break;

  case 144: /* annotation: FZ_ID '(' annotation_list ')'  */
      { 
        (yyval.arg) = new AST::Call((yyvsp[-3].sValue), AST::extractSingleton((yyvsp[-1].arg))); free((yyvsp[-3].sValue));
      }
    break;

  case 145: /* annotation: annotation_expr  */
      { (yyval.arg) = (yyvsp[0].arg); }
    break;

  case 146: /* annotation_list: annotation  */
      { (yyval.arg) = new AST::Array((yyvsp[0].arg)); }
    break;

  case 147: /* annotation_list: annotation_list ',' annotation  */
      { (yyval.arg) = (yyvsp[-2].arg); (yyval.arg)->append((yyvsp[0].arg)); }
    break;

  case 148: /* annotation_expr: ann_non_array_expr  */
      { (yyval.arg) = (yyvsp[0].arg); }
    break;

  case 149: /* annotation_expr: '[' annotation_list ']'  */
      { (yyval.arg) = (yyvsp[-1].arg); }
    break;

  case 150: /* ann_non_array_expr: FZ_BOOL_LIT  */
      { (yyval.arg) = new AST::BoolLit((yyvsp[0].iValue)); }
    break;

  case 151: /* ann_non_array_expr: FZ_INT_LIT  */
      { (yyval.arg) = new AST::IntLit((yyvsp[0].iValue)); }
    break;

  case 152: /* ann_non_array_expr: FZ_FLOAT_LIT  */
      { (yyval.arg) = new AST::FloatLit((yyvsp[0].dValue)); }
    break;

  case 153: /* ann_non_array_expr: set_literal  */
      { (yyval.arg) = (yyvsp[0].setLit); }
    break;

  case 154: /* ann_non_array_expr: var_par_id  */
      { 
        vector<int> as;
        ParserState* pp = static_cast<ParserState*>(parm);
        if (pp->intvararrays.get((yyvsp[0].sValue), as)) {
          AST::Array *ia = new AST::Array(as.size());
          for (int i=as.size(); i--;)
            ia->a[i] = new AST::IntVar(as[i]);
          (yyval.arg) = ia;
        } else if (pp->boolvararrays.get((yyvsp[0].sValue), as)) {
          AST::Array *ia = new AST::Array(as.size());
          for (int i=as.size(); i--;)
            ia->a[i] = new AST::BoolVar(as[i]);
          (yyval.arg) = ia;
        } else if (pp->setvararrays.get((yyvsp[0].sValue), as)) {
          AST::Array *ia = new AST::Array(as.size());
          for (int i=as.size(); i--;)
            ia->a[i] = new AST::SetVar(as[i]);
          (yyval.arg) = ia;
        } else {
          std::vector<int> is;
          int ival = 0;
          bool bval = false;
          if (pp->intvalarrays.get((yyvsp[0].sValue), is)) {
            AST::Array *v = new AST::Array(is.size());
            for (int i=is.size(); i--;)
              v->a[i] = new AST::IntLit(is[i]);
            (yyval.arg) = v;
          } else if (pp->boolvalarrays.get((yyvsp[0].sValue), is)) {
            AST::Array *v = new AST::Array(is.size());
            for (int i=is.size(); i--;)
              v->a[i] = new AST::BoolLit(is[i]);
            (yyval.arg) = v;
          } else if (pp->intvals.get((yyvsp[0].sValue), ival)) {
            (yyval.arg) = new AST::IntLit(ival);
          } else if (pp->boolvals.get((yyvsp[0].sValue), bval)) {
            (yyval.arg) = new AST::BoolLit(bval);
          } else {
            (yyval.arg) = getVarRefArg(pp,(yyvsp[0].sValue),true);
          }
        }
        free((yyvsp[0].sValue));
      }
    break;

  case 155: /* ann_non_array_expr: var_par_id '[' ann_non_array_expr ']'  */
      { 
        ParserState* pp = static_cast<ParserState*>(parm);
        int i = -1;
        yyassert(pp, (yyvsp[-1].arg)->isInt(i), "Non-integer array index.");
        if (!pp->hadError)
          (yyval.arg) = getArrayElement(static_cast<ParserState*>(parm),(yyvsp[-3].sValue),i);
        else
          (yyval.arg) = new AST::IntLit(0); // keep things consistent
        free((yyvsp[-3].sValue));
      }
    break;

  case 156: /* ann_non_array_expr: FZ_STRING_LIT  */
      {
        (yyval.arg) = new AST::String((yyvsp[0].sValue));
        free((yyvsp[0].sValue));
      }
    break;



      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      {
        yypcontext_t yyctx
          = {yyssp, yytoken};
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == -1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *,
                             YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (yymsg)
              {
                yysyntax_error_status
                  = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
                yymsgp = yymsg;
              }
            else
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = YYENOMEM;
              }
          }
        yyerror (parm, yymsgp);
        if (yysyntax_error_status == YYENOMEM)
          YYNOMEM;
      }
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, parm);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, parm);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (parm, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, parm);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, parm);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
  return yyresult;
}

