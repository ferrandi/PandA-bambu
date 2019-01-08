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
 *              Copyright (C) 2004-2019 Politecnico di Milano
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

#include <algorithm> // for set_difference
#include <iterator>  // for inserter

/// STL include
#if HAVE_UNORDERED
#include <unordered_set>
#else
#include <set>
#endif

#if HAVE_UNORDERED
template <typename T>
class CustomSet : public std::unordered_set<T>
{
 public:
   void operator+=(const CustomSet& other)
   {
      typename CustomSet<T>::const_iterator other_element, other_element_end = other.end();
      for(other_element = other.begin(); other_element != other_element_end; ++other_element)
      {
         this->insert(*other_element);
      }
   }

   void operator-=(const CustomSet& other)
   {
      typename CustomSet<T>::const_iterator other_element, other_element_end = other.end();
      for(other_element = other.begin(); other_element != other_element_end; ++other_element)
      {
         this->erase(*other_element);
      }
   }

   CustomSet operator-(const CustomSet& other) const
   {
      CustomSet return_value = *this;
      return_value -= other;
      return return_value;
   }

   CustomSet Intersect(const CustomSet& other) const
   {
      CustomSet return_value;
      typename CustomSet<T>::const_iterator other_element, other_element_end = other.end();
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
#else
template <typename T>
class CustomSet : public std::set<T>
{
 public:
   void operator+=(const CustomSet& other)
   {
      typename CustomSet<T>::const_iterator other_element, other_element_end = other.end();
      for(other_element = other.begin(); other_element != other_element_end; ++other_element)
      {
         this->insert(*other_element);
      }
   }

   void operator-=(const CustomSet& other)
   {
      typename CustomSet<T>::const_iterator other_element, other_element_end = other.end();
      for(other_element = other.begin(); other_element != other_element_end; ++other_element)
      {
         this->erase(*other_element);
      }
   }

   CustomSet operator-(const CustomSet& other) const
   {
      CustomSet return_value;
      std::set_difference(this->begin(), this->end(), other.begin(), other.end(), std::inserter(return_value, return_value.begin()));
      return return_value;
   }

   CustomSet Intersect(const CustomSet& other) const
   {
      CustomSet return_value;
      typename CustomSet<T>::const_iterator other_element, other_element_end = other.end();
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
#endif
#endif
