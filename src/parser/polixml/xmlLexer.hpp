/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************            
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
