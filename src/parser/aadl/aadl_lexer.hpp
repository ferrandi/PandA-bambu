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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
 * @file aadl_lexer.hpp
 * @brief header file for LEX based lexer for aadl file.
 *
 * @author Lattuada Marco <marco.lattuada@polimi.it>
 *
 */
#ifndef AADL_LEXER_HPP
#define AADL_LEXER_HPP

#define LN_CONCAT(name) Aadl##name

#define LCLASS_SPECIALIZED

#include "Lexer_utilities.hpp"

#include <istream>

class AadlFlexLexer : public yyFlexLexer
{
 protected:
   /// The debug level
   const int debug_level;

 public:
   /// Skip all tokens but end
   bool skip;

   YYSTYPE* lvalp;
   int yylex() override;

   /**
    * Constructor
    */
   AadlFlexLexer(const ParameterConstRef parameters, std::istream* argin, std::ostream* argout);

   /**
    * Destructor
    */
   ~AadlFlexLexer() override;

   void yyerror(const char* msg)
   {
      LexerError(msg);
   }

   void LexerError(const char* msg) override
   {
      std::cout << msg << " at line number |" << lineno() << "|\t";
      std::cout << "text is |" << YYText() << "|" << std::endl;
      throw "Parse Error";
   }

   int yywrap() override
   {
      return 1;
   }
};
typedef refcount<AadlFlexLexer> AadlFlexLexerRef;
#endif
