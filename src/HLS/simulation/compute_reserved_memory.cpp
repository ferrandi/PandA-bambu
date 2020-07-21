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
 *              Copyright (c) 2018-2020 Politecnico di Milano
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
 * @file compute_reserved_memory.cpp
 * @brief Specification of the functor used to compute size of objects starting from C initialization string
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "compute_reserved_memory.hpp"

/// STD include
#include <string>

/// tree includes
#include "tree_helper.hpp"
#include "tree_node.hpp"

/// utility include
#include "exceptions.hpp"

ComputeReservedMemory::ComputeReservedMemory(const tree_managerConstRef _TM, const tree_nodeConstRef _tn) : TM(_TM), tn(_tn), elements_number(1), depth_level(0)
{
}

unsigned int ComputeReservedMemory::GetReservedBytes() const
{
   const auto ptd_type = tree_helper::get_pointed_type(TM, tree_helper::get_type_index(TM, tn->index));
   auto reservedMem = elements_number * tree_helper::size(TM, ptd_type) / 8;
   return reservedMem ? reservedMem : 1;
}

void ComputeReservedMemory::CheckEnd()
{
   THROW_ASSERT(depth_level == 0, "");
}

void ComputeReservedMemory::GoDown()
{
   depth_level++;
}

void ComputeReservedMemory::GoNext()
{
   /// For compatibility with old initialization (without parentheses)
   if(depth_level == 0)
      elements_number++;
   if(depth_level == 1)
      elements_number++;
}

void ComputeReservedMemory::GoUp()
{
   THROW_ASSERT(depth_level > 0, "");
   depth_level--;
}

void ComputeReservedMemory::Process(const std::string&)
{
}
