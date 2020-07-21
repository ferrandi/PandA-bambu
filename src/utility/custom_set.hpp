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
 * @file custom_set.hpp
 * @brief redefinition of set to manage ordered/unordered structures
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef CUSTOM_SET_HPP
#define CUSTOM_SET_HPP

/// Autoheader include
#include "config_HAVE_UNORDERED.hpp"

#include <set>
#include <unordered_set>

template <class _Value, class _Hash = std::hash<_Value>, class _Pred = std::equal_to<_Value>, class _Alloc = std::allocator<_Value>>
using UnorderedSetStd = std::unordered_set<_Value, _Hash, _Pred, _Alloc>;

template <typename Key, typename Compare = std::less<Key>, typename Alloc = std::allocator<Key>>
using OrderedSetStd = std::set<Key, Compare, Alloc>;

#if !defined(__clang__) && (__GNUC__ == 4 && __GNUC_MINOR__ <= 8)
#ifndef NO_ABSEIL_HASH
#define NO_ABSEIL_HASH 1
#endif
#include <algorithm>

template <class _Value, class _Hash = std::hash<_Value>, class _Pred = std::equal_to<_Value>, class _Alloc = std::allocator<_Value>>
using UnorderedSetStdStable = UnorderedSetStd<_Value, _Hash, _Pred, _Alloc>;

template <class T, class _Hash = std::hash<T>, class _Pred = std::equal_to<T>, class _Alloc = std::allocator<T>>
class CustomUnorderedSet : public UnorderedSetStd<T, _Hash, _Pred, _Alloc>
{
 public:
   void operator+=(const CustomUnorderedSet& other)
   {
      typename CustomUnorderedSet<T>::const_iterator other_element, other_element_end = other.end();
      for(other_element = other.begin(); other_element != other_element_end; ++other_element)
      {
         this->insert(*other_element);
      }
   }

   void operator-=(const CustomUnorderedSet& other)
   {
      typename CustomUnorderedSet<T>::const_iterator other_element, other_element_end = other.end();
      for(other_element = other.begin(); other_element != other_element_end; ++other_element)
      {
         this->erase(*other_element);
      }
   }

   CustomUnorderedSet operator-(const CustomUnorderedSet& other) const
   {
      CustomUnorderedSet return_value = *this;
      return_value -= other;
      return return_value;
   }

   CustomUnorderedSet Intersect(const CustomUnorderedSet& other) const
   {
      CustomUnorderedSet return_value;
      typename CustomUnorderedSet<T>::const_iterator other_element, other_element_end = other.end();
      for(other_element = other.begin(); other_element != other_element_end; ++other_element)
      {
         if(this->find(*other_element) != this->end())
         {
            return_value.insert(*other_element);
         }
      }
      return return_value;
   }
};

template <typename Key, typename Compare = std::less<Key>, typename _Alloc = std::allocator<Key>>
class CustomOrderedSet : public OrderedSetStd<Key, Compare, _Alloc>
{
 public:
   void operator+=(const CustomOrderedSet& other)
   {
      typename CustomOrderedSet<Key>::const_iterator other_element, other_element_end = other.end();
      for(other_element = other.begin(); other_element != other_element_end; ++other_element)
      {
         this->insert(*other_element);
      }
   }

   void operator-=(const CustomOrderedSet& other)
   {
      typename CustomOrderedSet<Key>::const_iterator other_element, other_element_end = other.end();
      for(other_element = other.begin(); other_element != other_element_end; ++other_element)
      {
         this->erase(*other_element);
      }
   }

   CustomOrderedSet operator-(const CustomOrderedSet& other) const
   {
      CustomOrderedSet return_value;
      std::set_difference(this->begin(), this->end(), other.begin(), other.end(), std::inserter(return_value, return_value.begin()));
      return return_value;
   }

   CustomOrderedSet Intersect(const CustomOrderedSet& other) const
   {
      CustomOrderedSet return_value;
      typename CustomOrderedSet<Key>::const_iterator other_element, other_element_end = other.end();
      for(other_element = other.begin(); other_element != other_element_end; ++other_element)
      {
         if(this->find(*other_element) != this->end())
         {
            return_value.insert(*other_element);
         }
      }
      return return_value;
   }
};

#if HAVE_UNORDERED
template <typename T>
using CustomSet = CustomUnorderedSet<T>;
#else
template <typename T>
using CustomSet = CustomOrderedSet<T>;
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
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wextra-semi"
#endif

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_set.h"
#include "absl/container/node_hash_set.h"
#include "absl/hash/hash.h"
#include <set>

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

template <class _Value, class _Hash = std::hash<_Value>, class _Pred = std::equal_to<_Value>, class _Alloc = std::allocator<_Value>>
using UnorderedSetStdStable = absl::node_hash_set<_Value, _Hash, _Pred, _Alloc>;

template <class T, class _Hash = absl::container_internal::hash_default_hash<T>, class _Eq = absl::container_internal::hash_default_eq<T>, class _Alloc = std::allocator<T>>
class CustomUnorderedSet : public absl::flat_hash_set<T, _Hash, _Eq, _Alloc>
{
 public:
   void operator+=(const CustomUnorderedSet& other)
   {
      typename CustomUnorderedSet<T>::const_iterator other_element, other_element_end = other.end();
      for(other_element = other.begin(); other_element != other_element_end; ++other_element)
      {
         this->insert(*other_element);
      }
   }

   void operator-=(const CustomUnorderedSet& other)
   {
      typename CustomUnorderedSet<T>::const_iterator other_element, other_element_end = other.end();
      for(other_element = other.begin(); other_element != other_element_end; ++other_element)
      {
         this->erase(*other_element);
      }
   }

   CustomUnorderedSet operator-(const CustomUnorderedSet& other) const
   {
      CustomUnorderedSet return_value = *this;
      return_value -= other;
      return return_value;
   }

   CustomUnorderedSet Intersect(const CustomUnorderedSet& other) const
   {
      CustomUnorderedSet return_value;
      typename CustomUnorderedSet<T>::const_iterator other_element, other_element_end = other.end();
      for(other_element = other.begin(); other_element != other_element_end; ++other_element)
      {
         if(this->find(*other_element) != this->end())
         {
            return_value.insert(*other_element);
         }
      }
      return return_value;
   }
};

template <typename Key, typename Compare = std::less<Key>, typename Alloc = std::allocator<Key>>
class CustomOrderedSet : public absl::btree_set<Key, Compare, Alloc>
{
 public:
   void operator+=(const CustomOrderedSet& other)
   {
      typename CustomOrderedSet<Key>::const_iterator other_element, other_element_end = other.end();
      for(other_element = other.begin(); other_element != other_element_end; ++other_element)
      {
         this->insert(*other_element);
      }
   }

   void operator-=(const CustomOrderedSet& other)
   {
      typename CustomOrderedSet<Key>::const_iterator other_element, other_element_end = other.end();
      for(other_element = other.begin(); other_element != other_element_end; ++other_element)
      {
         this->erase(*other_element);
      }
   }

   CustomOrderedSet operator-(const CustomOrderedSet& other) const
   {
      CustomOrderedSet return_value;
      std::set_difference(this->begin(), this->end(), other.begin(), other.end(), std::inserter(return_value, return_value.begin()));
      return return_value;
   }

   CustomOrderedSet Intersect(const CustomOrderedSet& other) const
   {
      CustomOrderedSet return_value;
      typename CustomOrderedSet<Key>::const_iterator other_element, other_element_end = other.end();
      for(other_element = other.begin(); other_element != other_element_end; ++other_element)
      {
         if(this->find(*other_element) != this->end())
         {
            return_value.insert(*other_element);
         }
      }
      return return_value;
   }
};

#if HAVE_UNORDERED
template <typename T>
using CustomSet = CustomUnorderedSet<T>;
#else
template <typename T>
using CustomSet = CustomOrderedSet<T>;
#endif

#endif
#endif
