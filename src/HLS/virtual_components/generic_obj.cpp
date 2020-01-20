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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
 * @file generic_obj.cpp
 * @brief Base class for all resources into datapath
 *
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "generic_obj.hpp"

/// utility include
#include "exceptions.hpp"

#if !HAVE_UNORDERED
GenericObjSorter::GenericObjSorter() = default;

bool GenericObjSorter::operator()(const generic_objRef& x, const generic_objRef& y) const
{
   if(x == y)
   {
      return false;
   }
   THROW_ASSERT(x->get_string() != y->get_string() or x->get_string().find("CONSTANT") != std::string::npos, x->get_string());
   return x->get_string() < y->get_string();
}

GenericObjUnsignedIntSorter::GenericObjUnsignedIntSorter() = default;

bool GenericObjUnsignedIntSorter::operator()(const std::pair<generic_objRef, int>& x, const std::pair<generic_objRef, int>& y) const
{
   if(x.first == y.first)
   {
      return x.second < y.second;
   }
   THROW_ASSERT(x.first->get_string() != y.first->get_string(), x.first->get_string());
   return x.first->get_string() < y.first->get_string();
}

#endif

bool generic_obj::operator<(const generic_obj& other) const
{
   return get_string() < other.get_string();
}
