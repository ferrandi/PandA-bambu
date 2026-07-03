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
 * @file token_interface.cpp
 * @brief Implementation of interface to token objects.
 *
 * Interface implementation to token objects.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "token_interface.hpp"

#include "exceptions.hpp"
#include "refcount.hpp"

#include <cstring>
#include <fstream>
#include <iosfwd>

#include "config_HAVE_BISON_2_7_OR_GREATER.hpp"

/// NOTE: forward declarations of BisonParserData and IRFlexLexer are in this point since they will be used in
/// irParser.h
REF_FORWARD_DECL(BisonParserData);
REF_FORWARD_DECL(IRFlexLexer);

/// IR parser include
#if HAVE_BISON_2_7_OR_GREATER
#include "irParser.hpp"
#else
#include "irParser.h"
#endif

#define NO_TOKEN (-1)

#define TOKEN_EXPR(tok, bison_tok, name) bison_tok,
const int irVocabularyTokenTypes::bisontokens[] = {
#include "IRVocabularyTokens.def"
};
#undef TOKEN_EXPR

#define TOKEN_EXPR(tok, bison_tok, name) name,
const char* irVocabularyTokenTypes::tokenNames[] = {
#include "IRVocabularyTokens.def"
};
#undef TOKEN_EXPR

const std::string TI_getTokenName(const IRVocabularyTokenTypes_TokenEnum i)
{
   const char* tmp = irVocabularyTokenTypes::tokenNames[static_cast<unsigned int>(i)];
   return std::string(tmp);
}

int irVocabularyTokenTypes::check_tokens(const char* tok) const
{
   auto el = token_map.find(tok);
   if(el == token_map.end())
   {
      return -1;
   }
   else
   {
      return bisontokens[el->second];
   }
}
IRVocabularyTokenTypes_TokenEnum irVocabularyTokenTypes::bison2token(int bison) const
{
   auto el = from_bisontoken_map.find(bison);
   THROW_ASSERT(el != from_bisontoken_map.end(), "Token " + std::to_string(bison) + " does not exist");
   return el->second;
}
irVocabularyTokenTypes::irVocabularyTokenTypes()
{
   for(int i = static_cast<int>(IRVocabularyTokenTypes_TokenEnum::FIRST_TOKEN) + 1;
       i < static_cast<int>(IRVocabularyTokenTypes_TokenEnum::LAST_TOKEN); i++)
   {
      token_map[tokenNames[i]] = i;
      from_bisontoken_map[bisontokens[i]] = static_cast<IRVocabularyTokenTypes_TokenEnum>(i);
   }
}
