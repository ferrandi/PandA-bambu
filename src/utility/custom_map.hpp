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
 * @file custom_map.hpp
 * @brief redefinition of map to manage ordered/unordered structures
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef CUSTOM_MAP_HPP
#define CUSTOM_MAP_HPP

#ifndef NO_ABSEIL_HASH
#define NO_ABSEIL_HASH 0
#endif

#include <absl/container/btree_map.h>
#include <absl/container/flat_hash_map.h>
#include <absl/container/node_hash_map.h>
#include <absl/hash/hash.h>

#include <map>
#include <unordered_map>

#include "config_HAVE_UNORDERED.hpp"

template <class T, class U, class Hash = std::hash<T>, class Eq = std::equal_to<T>,
          class Alloc = std::allocator<std::pair<const T, U>>>
using UnorderedMapStd = std::unordered_map<T, U, Hash, Eq, Alloc>;

template <typename _Key, typename _Tp, typename _Compare = std::less<_Key>,
          typename _Alloc = std::allocator<std::pair<const _Key, _Tp>>>
using OrderedMapStd = std::map<_Key, _Tp, _Compare, _Alloc>;

template <class Key, class Value, class Hash = absl::container_internal::hash_default_hash<Key>,
          class Eq = absl::container_internal::hash_default_eq<Key>,
          class Alloc = std::allocator<std::pair<const Key, Value>>>
using CustomUnorderedMapStable = absl::node_hash_map<Key, Value, Hash, Eq, Alloc>;

template <class K, class V, class Hash = absl::container_internal::hash_default_hash<K>,
          class Eq = absl::container_internal::hash_default_eq<K>,
          class Allocator = std::allocator<std::pair<const K, V>>>
using CustomUnorderedMapUnstable = absl::flat_hash_map<K, V, Hash, Eq, Allocator>;

template <class K, class V, class Hash = absl::container_internal::hash_default_hash<K>,
          class Eq = absl::container_internal::hash_default_eq<K>,
          class Allocator = std::allocator<std::pair<const K, V>>>
using CustomUnorderedMap = CustomUnorderedMapUnstable<K, V, Hash, Eq, Allocator>;

template <typename Key, typename Value, typename Compare = std::less<Key>,
          typename Alloc = std::allocator<std::pair<const Key, Value>>>
using CustomOrderedMap = absl::btree_map<Key, Value, Compare, Alloc>;

#if HAVE_UNORDERED
template <typename T, typename U>
using CustomMap = CustomUnorderedMap<T, U>;
#else
template <typename T, typename U>
using CustomMap = CustomOrderedMap<T, U>;
#endif

#endif
