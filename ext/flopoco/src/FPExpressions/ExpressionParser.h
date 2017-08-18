/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

#ifndef YY_FLOPOCOEXPRESSION_MNT_EXTRA_FERRANDI_SOFTWARE_PANDA_GIT_TRANSACTION_HELPERS_PANDA_FRAMEWORK_TRUNK_PANDA2_PANDA_EXT_FLOPOCO_SRC_FPEXPRESSIONS_EXPRESSIONPARSER_H_INCLUDED
# define YY_FLOPOCOEXPRESSION_MNT_EXTRA_FERRANDI_SOFTWARE_PANDA_GIT_TRANSACTION_HELPERS_PANDA_FRAMEWORK_TRUNK_PANDA2_PANDA_EXT_FLOPOCO_SRC_FPEXPRESSIONS_EXPRESSIONPARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int FlopocoExpressiondebug;
#endif
/* "%code requires" blocks.  */
#line 1 "/mnt/extra/ferrandi/software/panda/git-transaction-helpers/panda-framework/trunk/panda2/panda/ext/./flopoco/src/FPExpressions/ExpressionParser.y" /* yacc.c:1909  */

	#include "ExpressionParserData.h"
	extern program* p;

#line 49 "/mnt/extra/ferrandi/software/panda/git-transaction-helpers/panda-framework/trunk/panda2/panda/ext/./flopoco/src/FPExpressions/ExpressionParser.h" /* yacc.c:1909  */

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    FPEXPRESSION_UNKNOWN = 258,
    FPEXPRESSION_PV = 259,
    FPEXPRESSION_OUTPUT = 260,
    FPEXPRESSION_LPAR = 261,
    FPEXPRESSION_RPAR = 262,
    FPEXPRESSION_SQR = 263,
    FPEXPRESSION_SQRT = 264,
    FPEXPRESSION_EXP = 265,
    FPEXPRESSION_LOG = 266,
    FPEXPRESSION_PLUS = 267,
    FPEXPRESSION_MINUS = 268,
    FPEXPRESSION_TIMES = 269,
    FPEXPRESSION_DIV = 270,
    FPEXPRESSION_EQUALS = 271,
    FPEXPRESSION_FPNUMBER = 272,
    FPEXPRESSION_VARIABLE = 273
  };
#endif
/* Tokens.  */
#define FPEXPRESSION_UNKNOWN 258
#define FPEXPRESSION_PV 259
#define FPEXPRESSION_OUTPUT 260
#define FPEXPRESSION_LPAR 261
#define FPEXPRESSION_RPAR 262
#define FPEXPRESSION_SQR 263
#define FPEXPRESSION_SQRT 264
#define FPEXPRESSION_EXP 265
#define FPEXPRESSION_LOG 266
#define FPEXPRESSION_PLUS 267
#define FPEXPRESSION_MINUS 268
#define FPEXPRESSION_TIMES 269
#define FPEXPRESSION_DIV 270
#define FPEXPRESSION_EQUALS 271
#define FPEXPRESSION_FPNUMBER 272
#define FPEXPRESSION_VARIABLE 273

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 17 "/mnt/extra/ferrandi/software/panda/git-transaction-helpers/panda-framework/trunk/panda2/panda/ext/./flopoco/src/FPExpressions/ExpressionParser.y" /* yacc.c:1909  */

	char c_type;
	int i_type;
	double d_type;
	char* s_type;
	node* thisNode;         // each node is either a leaf or an assignement
	nodeList* thisNodeList; //list containing the assignments
	varList*  thisVarList; //list containing the assignments
	program* theProgram; //assignment list + output variables

#line 108 "/mnt/extra/ferrandi/software/panda/git-transaction-helpers/panda-framework/trunk/panda2/panda/ext/./flopoco/src/FPExpressions/ExpressionParser.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE FlopocoExpressionlval;

int FlopocoExpressionparse (void);

#endif /* !YY_FLOPOCOEXPRESSION_MNT_EXTRA_FERRANDI_SOFTWARE_PANDA_GIT_TRANSACTION_HELPERS_PANDA_FRAMEWORK_TRUNK_PANDA2_PANDA_EXT_FLOPOCO_SRC_FPEXPRESSIONS_EXPRESSIONPARSER_H_INCLUDED  */
