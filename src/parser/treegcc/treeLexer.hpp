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
 * @file treeLexer.hpp
 * @brief header file for LEX based lexer for GCC raw files.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef TREELEXER_HPP
#define TREELEXER_HPP

#define LN_CONCAT(name) Tree##name

#define LCLASS_SPECIALIZED

#include "Lexer_utilities.hpp"

/// Tree include
#include "token_interface.hpp"

/// Utility include
#include "exceptions.hpp"

class treeVocabularyTokenTypes;

struct TreeFlexLexer : public yyFlexLexer
{
   YYSTYPE* lvalp;
   int yylex() override;

   TreeFlexLexer(std::istream* argin, std::ostream* argout);

   ~TreeFlexLexer() override;

   void yyerror(const char* msg)
   {
      LexerError(msg);
   }

   void LexerError(const char* msg) override
   {
      std::cout << msg << " at line number |" << lineno() << "|\t";
      std::cout << "text is |" << YYText() << "|" << std::endl;
      THROW_ERROR("Parse error");
   }

   int yywrap() override
   {
      return 1;
   }

   treeVocabularyTokenTypes* tokens;

   TreeVocabularyTokenTypes_TokenEnum bison2token(int) const;
};

#endif
