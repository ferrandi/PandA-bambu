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
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
