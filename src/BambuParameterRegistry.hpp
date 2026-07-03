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
 *              Copyright (C) 2026 Politecnico di Milano
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
 * @file BambuParameterRegistry.hpp
 * @brief Collect information about bambu parameters
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef BAMBU_PARAMETER_REGISTRY_HPP
#define BAMBU_PARAMETER_REGISTRY_HPP

#include <string>
#include <string_view>
#include <vector>

enum class PandaParamType
{
   Bool,
   Int,
   UInt,
   Double,
   String,
   Enum,
   Unknown
};

struct PandaParameterInfo
{
   std::string name;
   PandaParamType type;
   std::string default_value;
   std::string description;
   std::string category;
   std::string declared_in;
   std::vector<std::string> allowed_values;
};

bool RegisterPandaParameter(PandaParameterInfo info);
const PandaParameterInfo* FindPandaParameter(std::string_view name);
std::vector<PandaParameterInfo> ListPandaParameters();
const char* PandaParamTypeToString(PandaParamType type);

#define PANDA_STRINGIZE_DETAIL(x) #x
#define PANDA_STRINGIZE(x) PANDA_STRINGIZE_DETAIL(x)

#define PANDA_REGISTER_PARAMETER(NAME, TYPE, DEFAULT, DESC, CATEGORY)                                           \
   namespace                                                                                                    \
   {                                                                                                            \
      [[maybe_unused]] const bool _panda_param_reg_##__COUNTER__ = RegisterPandaParameter(                      \
          PandaParameterInfo{NAME, TYPE, DEFAULT, DESC, CATEGORY, __FILE__ ":" PANDA_STRINGIZE(__LINE__), {}}); \
   }

#endif
