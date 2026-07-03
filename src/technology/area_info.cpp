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
 *              Copyright (C) 2023-2026 Politecnico di Milano
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
 * @file area_info.cpp
 * @brief Collect information about resource area
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "area_info.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace
{
   template <std::size_t N, std::size_t... I>
   std::array<std::string, N> make_owned_names_impl(const std::array<std::string_view, N>& names,
                                                    std::index_sequence<I...>)
   {
      return {std::string{names[I]}...};
   }

   template <std::size_t N>
   std::array<std::string, N> make_owned_names(const std::array<std::string_view, N>& names)
   {
      return make_owned_names_impl(names, std::make_index_sequence<N>{});
   }

   template <std::size_t N>
   const std::string& enum_to_string(const std::size_t index, const std::array<std::string, N>& names,
                                     const char* const error_message)
   {
      if(index < names.size())
      {
         return names[index];
      }
      throw std::out_of_range(error_message);
   }

   template <typename Enum, std::size_t N>
   Enum string_to_enum(const std::string& value, const std::array<std::string_view, N>& names, const Enum error_value)
   {
      if(const auto it = std::find(names.begin(), names.end(), value); it != names.end())
      {
         return static_cast<Enum>(it - names.begin());
      }
      return error_value;
   }
} // namespace

void area_info::print(std::ostream& os) const
{
   if(auto luts = resources.find(resource_type::LUT); luts != resources.end())
   {
      os << "LUTs: " << luts->second << std::endl;
   }
}

const std::string& area_info::to_string(enum area_info::resource_type v)
{
   static const auto resource_strings = make_owned_names(namedResourceTypes);

   return enum_to_string(static_cast<std::size_t>(v), resource_strings, "Invalid area_info::resource_type value");
}

enum area_info::resource_type area_info::to_resource_type(const std::string& v)
{
   return string_to_enum(v, namedResourceTypes, resource_type::ERROR);
}
