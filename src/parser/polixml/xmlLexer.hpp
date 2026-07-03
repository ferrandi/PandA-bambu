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
 * @file xmlLexer.hpp
 * @brief header file for LEX based lexer for the xml format.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef XMLLEXER_HPP
#define XMLLEXER_HPP

#define LN_CONCAT(name) Xml##name

#define LCLASS_SPECIALIZED

#include "Lexer_utilities.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "string_manipulation.hpp"

#include <utility>

extern int exit_code;

struct XmlFlexLexer : public yyFlexLexer
{
   /// The name of the parsed file/string
   const std::string name;

   XmlFlexLexer(const std::string& _name, std::istream* argin = nullptr, std::ostream* argout = nullptr)
       : yyFlexLexer(argin, argout), name(_name), keep(0)
   {
   }
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
