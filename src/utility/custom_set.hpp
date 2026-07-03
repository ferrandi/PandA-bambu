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
 * @file custom_set.hpp
 * @brief redefinition of set to manage ordered/unordered structures
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef CUSTOM_SET_HPP
#define CUSTOM_SET_HPP

#ifndef NO_ABSEIL_HASH
#define NO_ABSEIL_HASH 0
#endif

#include <absl/container/btree_set.h>
#include <absl/container/flat_hash_set.h>
#include <absl/container/node_hash_set.h>
#include <absl/hash/hash.h>

#include <set>
#include <unordered_set>

#include "config_HAVE_UNORDERED.hpp"

template <class _Value, class _Hash = std::hash<_Value>, class _Pred = std::equal_to<_Value>,
          class _Alloc = std::allocator<_Value>>
using UnorderedSetStd = std::unordered_set<_Value, _Hash, _Pred, _Alloc>;

template <typename Key, typename Compare = std::less<Key>, typename Alloc = std::allocator<Key>>
using OrderedSetStd = std::set<Key, Compare, Alloc>;

template <class T, class Hash = absl::container_internal::hash_default_hash<T>,
          class Eq = absl::container_internal::hash_default_eq<T>, class Alloc = std::allocator<T>>
using UnorderedSetStdStable = absl::node_hash_set<T, Hash, Eq, Alloc>;

template <class T, class Hash = absl::container_internal::hash_default_hash<T>,
          class Eq = absl::container_internal::hash_default_eq<T>, class Allocator = std::allocator<T>>
using CustomUnorderedSet = absl::flat_hash_set<T, Hash, Eq, Allocator>;

template <typename Key, typename Compare = std::less<Key>, typename Alloc = std::allocator<Key>>
using CustomOrderedSet = absl::btree_set<Key, Compare, Alloc>;

#if HAVE_UNORDERED
template <typename T>
using CustomSet = CustomUnorderedSet<T>;
#else
template <typename T>
using CustomSet = CustomOrderedSet<T>;
#endif

#endif
