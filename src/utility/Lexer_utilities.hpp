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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file Lexer_utilities.hpp
 * @brief macro used to instantiate LEX based lexers.
 *
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
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
using yy_size_t = size_t;
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

/** @cond DOXYGEN_IGNORE */
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
/** @endcond */

/** @cond DOXYGEN_IGNORE */
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
/** @endcond */

#endif
