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
 *              Copyright (C) 2015-2026 Politecnico di Milano
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
 * @file hash_helper.hpp
 * @brief This file collects some hash functors
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef HASH_HELPER_HPP
#define HASH_HELPER_HPP

#include "custom_set.hpp"

#if NO_ABSEIL_HASH

#include <boost/functional/hash/hash.hpp>

/// Hash function for std::vector
namespace std
{
   template <typename T>
   struct hash<std::vector<T>>
   {
      std::size_t operator()(const std::vector<T>& val) const
      {
         return boost::hash_range<typename std::vector<T>::const_iterator>(val.begin(), val.end());
      }
   };
} // namespace std

/// Hash function for std::pair<T, U>
namespace std
{
   template <typename T, typename U>
   struct hash<std::pair<T, U>>
   {
      std::size_t operator()(const std::pair<T, U>& val) const
      {
         size_t hash_value = 0;
         boost::hash_combine(hash_value, val.first);
         boost::hash_combine(hash_value, val.second);
         return hash_value;
      }
   };
} // namespace std
#endif
#endif
