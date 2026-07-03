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
 * @file BambuParameterRegistry.cpp
 * @brief Collect information about bambu parameters
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "BambuParameterRegistry.hpp"

#include <algorithm>
#include <map>
#include <mutex>

void RegisterBambuParameterRegistryShards();

namespace
{
   struct RegistryState
   {
      std::mutex mutex;
      std::map<std::string, PandaParameterInfo> entries;
   };

   RegistryState& GetRegistryState()
   {
      static RegistryState state;
      return state;
   }

   std::once_flag registry_once;

   void EnsureRegistryLoaded()
   {
      std::call_once(registry_once, []() { RegisterBambuParameterRegistryShards(); });
   }

   bool NormalizeAllowedValues(std::vector<std::string>& values)
   {
      if(values.empty())
      {
         return false;
      }
      auto before = values.size();
      std::sort(values.begin(), values.end());
      values.erase(std::unique(values.begin(), values.end()), values.end());
      return before != values.size();
   }

   bool MergeString(std::string& dst, const std::string& src)
   {
      if(src.empty())
      {
         return false;
      }
      if(dst.empty())
      {
         dst = src;
         return true;
      }
      if(dst != src && src < dst)
      {
         dst = src;
         return true;
      }
      return false;
   }

   bool MergeType(PandaParamType& dst, PandaParamType src)
   {
      if(src == PandaParamType::Unknown || dst == src)
      {
         return false;
      }
      if(dst == PandaParamType::Unknown || static_cast<int>(src) < static_cast<int>(dst))
      {
         dst = src;
         return true;
      }
      return false;
   }

   bool MergeAllowedValues(std::vector<std::string>& dst, const std::vector<std::string>& src)
   {
      if(src.empty())
      {
         return false;
      }
      std::size_t before = dst.size();
      dst.insert(dst.end(), src.begin(), src.end());
      NormalizeAllowedValues(dst);
      return before != dst.size();
   }

   bool MergeInfo(PandaParameterInfo& dst, const PandaParameterInfo& src)
   {
      bool changed = false;
      changed |= MergeType(dst.type, src.type);
      changed |= MergeString(dst.default_value, src.default_value);
      changed |= MergeString(dst.description, src.description);
      changed |= MergeString(dst.category, src.category);
      changed |= MergeString(dst.declared_in, src.declared_in);
      changed |= MergeAllowedValues(dst.allowed_values, src.allowed_values);
      return changed;
   }
} // namespace

const char* PandaParamTypeToString(PandaParamType type)
{
   switch(type)
   {
      case PandaParamType::Bool:
         return "Bool";
      case PandaParamType::Int:
         return "Int";
      case PandaParamType::UInt:
         return "UInt";
      case PandaParamType::Double:
         return "Double";
      case PandaParamType::String:
         return "String";
      case PandaParamType::Enum:
         return "Enum";
      case PandaParamType::Unknown:
         return "Unknown";
      default:
         return "Unknown";
   }
}

bool RegisterPandaParameter(PandaParameterInfo info)
{
   if(info.name.empty())
   {
      return false;
   }

   NormalizeAllowedValues(info.allowed_values);

   auto& state = GetRegistryState();
   std::lock_guard<std::mutex> lock(state.mutex);
   auto it = state.entries.find(info.name);
   if(it == state.entries.end())
   {
      state.entries.emplace(info.name, std::move(info));
      return true;
   }
   return MergeInfo(it->second, info);
}

const PandaParameterInfo* FindPandaParameter(std::string_view name)
{
   EnsureRegistryLoaded();
   auto& state = GetRegistryState();
   std::lock_guard<std::mutex> lock(state.mutex);
   auto it = state.entries.find(std::string(name));
   if(it == state.entries.end())
   {
      return nullptr;
   }
   return &it->second;
}

std::vector<PandaParameterInfo> ListPandaParameters()
{
   EnsureRegistryLoaded();
   auto& state = GetRegistryState();
   std::lock_guard<std::mutex> lock(state.mutex);
   std::vector<PandaParameterInfo> entries;
   entries.reserve(state.entries.size());
   for(const auto& entry : state.entries)
   {
      entries.push_back(entry.second);
   }
   std::sort(entries.begin(), entries.end(),
             [](const PandaParameterInfo& left, const PandaParameterInfo& right) { return left.name < right.name; });
   return entries;
}
