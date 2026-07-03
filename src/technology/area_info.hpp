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
 * @file area_info.hpp
 * @brief Collect information about resource area
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef AREA_INFO_HPP
#define AREA_INFO_HPP

#include "refcount.hpp"

#include <array>
#include <iostream>
#include <map>
#include <string>
#include <string_view>

#define FOR_EACH_RESOURCE_TYPE(X) \
   X(REGISTERS)                   \
   X(SLICE)                       \
   X(SLICE_LUTS)                  \
   X(LUT)                         \
   X(ALMS)                        \
   X(LOGIC_ELEMENTS)              \
   X(FUNCTIONAL_ELEMENTS)         \
   X(LOGIC_AREA)                  \
   X(DSP)                         \
   X(BRAM)                        \
   X(DRAM)                        \
   X(POWER)                       \
   X(URAM)                        \
   X(AREA)

struct area_info
{
   enum resource_type
   {
#define RESOURCE_TYPE_ENUMERATOR(name) name,
      FOR_EACH_RESOURCE_TYPE(RESOURCE_TYPE_ENUMERATOR)
#undef RESOURCE_TYPE_ENUMERATOR
          ERROR
   };

   static constexpr auto namedResourceTypes = std::array{
#define RESOURCE_TYPE_NAME(name) std::string_view{#name},
       FOR_EACH_RESOURCE_TYPE(RESOURCE_TYPE_NAME)
#undef RESOURCE_TYPE_NAME
   };

   static const std::string& to_string(enum resource_type);

   static resource_type to_resource_type(const std::string& val);

   std::map<enum resource_type, double> resources{{AREA, 1.0}};

   double resource_or_default(enum resource_type id, double alt = 0.0)
   {
      if(const auto it = resources.find(id); it != resources.end())
      {
         return it->second;
      }
      return alt;
   }

   void print(std::ostream& os) const;
};
using area_infoRef = refcount<area_info>;

#undef FOR_EACH_RESOURCE_TYPE

#endif
