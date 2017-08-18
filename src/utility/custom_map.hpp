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
 *              Copyright (c) 2004-2017 Politecnico di Milano
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

///Autoheader include
#include "config_HAVE_UNORDERED.hpp"

///STL include
#if HAVE_UNORDERED
#include <unordered_map>
#else
#include <map>
#endif

///Utility include
#include <boost/lexical_cast.hpp>
#include "exceptions.hpp"

#if HAVE_UNORDERED
template <typename T, typename U>
class CustomMap : public std::unordered_map<T, U>
{
   public:
      std::pair<typename CustomMap<T, U>::iterator, bool> OverwriteInsert(const typename CustomMap<T,U>::value_type& val)
      {
         if(this->find(val.first) == this->end())
         {
            return std::unordered_map<T, U>::insert(val);
         }
         else
         {
            this->find(val.first)->second = val.second;
            return std::pair<typename CustomMap<T, U>::iterator, bool>(this->find(val.first), true);
         }
      }
};
#else
template <typename T, typename U>
class CustomMap : public std::map<T, U>
{
   public:
      std::pair<typename CustomMap<T, U>::iterator, bool> OverwriteInsert(const typename CustomMap<T,U>::value_type& val)
      {
         if(this->find(val.first) == this->end())
         {
            return std::map<T, U>::insert(val);
         }
         else
         {
            this->find(val.first)->second = val.second;
            return std::pair<typename CustomMap<T, U>::iterator, bool>(this->find(val.first), true);
         }
      }
};
#endif
#endif
