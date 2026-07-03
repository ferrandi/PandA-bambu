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
