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
 * @file xmlLexer.hpp
 * @brief header file for LEX based lexer for the xml format.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef XMLLEXER_HPP
#define XMLLEXER_HPP

#define LN_CONCAT(name) Xml##name

#define LCLASS_SPECIALIZED

#include <utility>

#include "Lexer_utilities.hpp"

/// utility include
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "exceptions.hpp"
#include "string_manipulation.hpp" // for STR

extern int exit_code;

struct XmlFlexLexer : public yyFlexLexer
{
   /// The name of the parsed file/string
   const std::string name;

   XmlFlexLexer(std::string _name, std::istream* argin = nullptr, std::ostream* argout = nullptr) : yyFlexLexer(argin, argout), name(std::move(_name)), keep(0)
   {
   }
   ~XmlFlexLexer() override = default;
   void yyerror(const char* msg)
   {
      LexerError(msg);
   }
   void LexerError(const char* msg) override
   {
      INDENT_OUT_MEX(0, 0, STR(msg) + " at line number |" + STR(lineno()) + "|\ttext is |" + STR(YYText()) + "|");
      exit_code = EXIT_FAILURE;
      THROW_ERROR("Error in parsing xml: " + name);
   }
   int yywrap() override
   {
      return 1;
   }
   /// To store start condition
   int keep;
   YYSTYPE* lvalp;
   int yylex() override;
};

#endif
