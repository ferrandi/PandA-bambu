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
 *              Copyright (C) 2004-2020 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * @file custom_map.hpp
 * @brief redefinition of map to manage ordered/unordered structures
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef CUSTOM_MAP_HPP
#define CUSTOM_MAP_HPP

/// Autoheader include
#include "config_HAVE_UNORDERED.hpp"

#include <map>
#include <unordered_map>

template <class T, class U, class Hash = std::hash<T>, class Eq = std::equal_to<T>, class Alloc = std::allocator<std::pair<const T, U>>>
using UnorderedMapStd = std::unordered_map<T, U, Hash, Eq, Alloc>;

template <typename _Key, typename _Tp, typename _Compare = std::less<_Key>, typename _Alloc = std::allocator<std::pair<const _Key, _Tp>>>
using OrderedMapStd = std::map<_Key, _Tp, _Compare, _Alloc>;

#if !defined(__clang__) && (__GNUC__ == 4 && __GNUC_MINOR__ <= 8)
#ifndef NO_ABSEIL_HASH
#define NO_ABSEIL_HASH 1
#endif

#include <utility> // for pair

template <class T, class U, class _Hash = std::hash<T>, class _Eq = std::equal_to<T>, class _Alloc = std::allocator<std::pair<const T, U>>>
using CustomUnorderedMap = UnorderedMapStd<T, U, _Hash, _Eq, _Alloc>;

template <class T, class U, class _Hash = std::hash<T>, class _Eq = std::equal_to<T>, class _Alloc = std::allocator<std::pair<const T, U>>>
using CustomUnorderedMapStable = UnorderedMapStd<T, U, _Hash, _Eq, _Alloc>;

template <class T, class U, class _Hash = std::hash<T>, class _Eq = std::equal_to<T>, class _Alloc = std::allocator<std::pair<const T, U>>>
using CustomUnorderedMapUnstable = UnorderedMapStd<T, U, _Hash, _Eq, _Alloc>;

template <typename _Key, typename _Tp, typename _Compare = std::less<_Key>, typename _Alloc = std::allocator<std::pair<const _Key, _Tp>>>
using CustomOrderedMap = OrderedMapStd<_Key, _Tp, _Compare, _Alloc>;

#if HAVE_UNORDERED
template <typename T, typename U>
using CustomMap = UnorderedMapStd<T, U>;
#else
template <typename T, typename U>
using CustomMap = OrderedMapStd<T, U>;
#endif

#else
#ifndef NO_ABSEIL_HASH
#define NO_ABSEIL_HASH 0
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
#pragma GCC diagnostic ignored "-Woverflow"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#if __GNUC__ == 4 && __GNUC_MINOR__ == 8
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wundef"
#pragma GCC diagnostic ignored "-Weffc++"
#ifndef __cpp_constexpr
#define __cpp_constexpr 200704
#endif
#endif
#else
#pragma GCC diagnostic warning "-Wsign-conversion"
#pragma GCC diagnostic warning "-Wconversion"
#pragma GCC diagnostic warning "-Wpedantic"
#pragma GCC diagnostic warning "-Wctor-dtor-privacy"
#pragma GCC diagnostic warning "-Woverflow"
#pragma GCC diagnostic warning "-Wzero-as-null-pointer-constant"
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wextra-semi"
#endif

#include "absl/container/btree_map.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/node_hash_map.h"
#include "absl/hash/hash.h"
#include <map>

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

template <class T, class U, class Hash = absl::container_internal::hash_default_hash<T>, class Eq = absl::container_internal::hash_default_eq<T>, class Alloc = std::allocator<std::pair<const T, U>>>
using CustomUnorderedMap = absl::flat_hash_map<T, U, Hash, Eq, Alloc>;

template <class T, class U, class Hash = absl::container_internal::hash_default_hash<T>, class Eq = absl::container_internal::hash_default_eq<T>, class Alloc = std::allocator<std::pair<const T, U>>>
using CustomUnorderedMapStable = absl::node_hash_map<T, U, Hash, Eq, Alloc>;

template <class T, class U, class Hash = absl::container_internal::hash_default_hash<T>, class Eq = absl::container_internal::hash_default_eq<T>, class Alloc = std::allocator<std::pair<const T, U>>>
using CustomUnorderedMapUnstable = absl::flat_hash_map<T, U, Hash, Eq, Alloc>;

template <typename T, typename U>
using CustomOrderedMap = absl::btree_map<T, U>;

#if HAVE_UNORDERED
template <typename T, typename U>
using CustomMap = CustomUnorderedMap<T, U>;
#else
template <typename T, typename U>
using CustomMap = CustomOrderedMap<T, U>;
#endif

#endif
#endif
