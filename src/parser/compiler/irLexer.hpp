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
 * @file irLexer.hpp
 * @brief header file for LEX based lexer for raw files.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef IRLEXER_HPP
#define IRLEXER_HPP

#define LN_CONCAT(name) IR##name

#define LCLASS_SPECIALIZED

#include "Lexer_utilities.hpp"
#include "exceptions.hpp"
#include "token_interface.hpp"

class irVocabularyTokenTypes;

struct IRFlexLexer : public yyFlexLexer
{
   YYSTYPE* lvalp;
   int yylex() override;

   IRFlexLexer(std::istream* argin, std::ostream* argout);

   ~IRFlexLexer() override;

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

   irVocabularyTokenTypes* tokens;

   IRVocabularyTokenTypes_TokenEnum bison2token(int) const;
};

#endif
