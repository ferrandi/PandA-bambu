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
 * @file token_interface.hpp
 * @brief A simple interface to token object of the raw files.
 *
 * Interface specification to token objects.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 */
#ifndef TOKEN_INTEFACE_HPP
#define TOKEN_INTEFACE_HPP

#include "custom_map.hpp"
#include <cstring>
#include <string>

#define TOKEN_EXPR(tok, bison_tok, name) tok,
enum class IRVocabularyTokenTypes_TokenEnum
{
   FIRST_TOKEN = -1,
#include "IRVocabularyTokens.def"
   LAST_TOKEN
};
#undef TOKEN_EXPR

struct irVocabularyTokenTypes
{
   static const char* tokenNames[];
   static const int bisontokens[];

   /// Map between bison token and token_interface token
   std::map<int, IRVocabularyTokenTypes_TokenEnum> from_bisontoken_map;
   int check_tokens(const char* tok) const;
   IRVocabularyTokenTypes_TokenEnum bison2token(int bison) const;
   irVocabularyTokenTypes();

 private:
   struct ltstr
   {
      bool operator()(const char* s1, const char* s2) const
      {
         return strcmp(s1, s2) < 0;
      }
   };
   std::map<const char*, int, ltstr> token_map;
};

/**
 * Return the name associated with the token.
 * @param i is the token coding.
 */
const std::string TI_getTokenName(const IRVocabularyTokenTypes_TokenEnum i);

/**
 * Macro which writes on an output stream a token.
 */
#define WRITE_TOKEN(os, token) os << " " << TI_getTokenName(IRVocabularyTokenTypes_TokenEnum::token)

/**
 * Second version of WRITE_TOKEN. Used in attr.cpp
 */
#define WRITE_TOKEN2(os, token) os << " " << TI_getTokenName(token)

/**
 * Macro used to convert a token symbol into a irVocabularyTokenTypes
 */
#define TOK(token) (IRVocabularyTokenTypes_TokenEnum::token)

/**
 * Macro used to convert a token symbol into the corresponding string
 */
#define STOK(token) TI_getTokenName(IRVocabularyTokenTypes_TokenEnum::token)

/**
 * Macro used to convert an int token symbol into the corresponding string
 */
#define STOK2(token) TI_getTokenName(token)

#endif
