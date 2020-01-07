/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *              Copyright (C) 2004-2020 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/**
 * @file Lexer_utilities.hpp
 * @brief macro used to instantiate LEX based lexers.
 *
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef LEXER_UTILITIES_HPP
#define LEXER_UTILITIES_HPP

/// Autoheader include
#include "config_HAVE_FLEX_2_5_34_OR_GREATER.hpp"
#include "config_HAVE_FLEX_2_5_35_OR_GREATER.hpp"

#define yyalloc LN_CONCAT(yyalloc)
#define yyrealloc LN_CONCAT(yyrealloc)
#define yyfree LN_CONCAT(yyfree)

#if HAVE_FLEX_2_5_35_OR_GREATER

#include <cstddef>

#ifndef YY_TYPEDEF_YY_SIZE_T
#define YY_TYPEDEF_YY_SIZE_T
typedef size_t yy_size_t;
#endif

void yyfree(void* ptr);
void* yyrealloc(void*, yy_size_t);
void* yyalloc(yy_size_t);

#else

void yyfree(void* ptr);
void* yyrealloc(void* ptr, unsigned int size);
void* yyalloc(unsigned int size);

#endif

#define YY_DECL int LN_CONCAT(FlexLexer)::yylex()
#include <iosfwd>

#undef yyFlexLexer
#define yyFlexLexer LN_CONCAT(baseFlexLexer)
#include <FlexLexer.h>

#ifndef LCLASS_SPECIALIZED
struct LN_CONCAT(FlexLexer) : public yyFlexLexer
{
   LN_CONCAT(FlexLexer)(std::istream* argin = nullptr, std::ostream* argout = nullptr) : yyFlexLexer(argin, argout)
   {
   }
   ~LN_CONCAT(FlexLexer)()
   {
   }
   void yyerror(const char* msg)
   {
      LexerError(msg);
   }
   void LexerError(const char* msg)
   {
      std::cout << msg << " at line number |" << lineno() << "|\t";
      std::cout << "text is |" << YYText() << "|" << std::endl;
      throw "Parse Error";
   }
   int yywrap()
   {
      return 1;
   }
   YYSTYPE* lvalp;
   int yylex();
};
#endif

inline int yyFlexLexer::yylex()
{
   return 0;
}

#if HAVE_FLEX_2_5_34_OR_GREATER
inline int yyFlexLexer::yywrap()
{
   return 1;
}
#else
#define yywrap LN_CONCAT(yywrap)
extern "C" inline int yywrap()
{
   return 1;
}
#endif

#endif
