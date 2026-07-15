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
 *   Copyright (C) 2015-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
