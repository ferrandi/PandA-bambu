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
 *              Copyright (c) 2018-2020 Politecnico di Milano
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
 * @file c_initialization_flex_lexer.hpp
 * @brief header file for LEX based lexer for C initialization string.
 *
 * @author Lattuada Marco <marco.lattuada@polimi.it>
 *
 */
#ifndef C_INITIALIZATION_LEXER_HPP
#define C_INITIALIZATION_LEXER_HPP

#define LN_CONCAT(name) CInitialization##name

#define LCLASS_SPECIALIZED

/// superclass include
#include "Lexer_utilities.hpp"

/// STD include
#include <istream>

/// utility include
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(Parameter);

class CInitializationFlexLexer : public yyFlexLexer
{
 protected:
   /// The debug level
   const int debug_level;

 public:
   YYSTYPE* lvalp;
   int yylex();

   /**
    * Constructor
    */
   CInitializationFlexLexer(const ParameterConstRef parameters, std::istream* argin, std::ostream* argout);

   /**
    * Destructor
    */
   ~CInitializationFlexLexer();

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
};
typedef refcount<CInitializationFlexLexer> CInitializationFlexLexerRef;
#endif
