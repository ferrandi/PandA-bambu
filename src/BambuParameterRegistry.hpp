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
 *   Copyright (C) 2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
