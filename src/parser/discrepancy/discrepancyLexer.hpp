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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * @author Michele Castellana <michele.castellana@mail.polimi.it>
 */

#ifndef DISCREPANCY_LEXER_HPP
#define DISCREPANCY_LEXER_HPP

#define LN_CONCAT(name) Discrepancy##name

#define LCLASS_SPECIALIZED

#include "Lexer_utilities.hpp"

#include "refcount.hpp"

struct DiscrepancyFlexLexer : public yyFlexLexer
{
   YYSTYPE* lvalp;
   int yylex() override;

   DiscrepancyFlexLexer(std::istream* argin, std::ostream* argout);

   ~DiscrepancyFlexLexer() override;

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

typedef refcount<DiscrepancyFlexLexer> DiscrepancyFlexLexerRef;
#endif
