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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file sdc_scheduling_base.cpp
 * @brief SDC scheduling base class
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "sdc_scheduling_base.hpp"

#include "basic_block.hpp"
#include "function_behavior.hpp"
#include "op_graph.hpp"

SDCSorter::SDCSorter(const FunctionBehaviorConstRef _function_behavior, const OpGraphConstRef _op_graph)
    : function_behavior(_function_behavior),
      op_graph(_op_graph),
      bb_index_map(_function_behavior->CGetBBGraph(FunctionBehavior::BB)->CGetBBGraphInfo()->bb_index_map)
{
}

bool SDCSorter::operator()(const vertex& x, const vertex& y) const
{
   const auto first_bb_index = op_graph->CGetOpNodeInfo(x)->bb_index;
   const auto second_bb_index = op_graph->CGetOpNodeInfo(y)->bb_index;
   if(first_bb_index != second_bb_index)
   {
      const auto first_bb_vertex = bb_index_map.at(first_bb_index);
      const auto second_bb_vertex = bb_index_map.at(second_bb_index);
      if(function_behavior->CheckBBReachability(first_bb_vertex, second_bb_vertex))
      {
         return true;
      }
      if(function_behavior->CheckBBReachability(second_bb_vertex, first_bb_vertex))
      {
         return false;
      }
   }
   if(x != y)
   {
      if(function_behavior->CheckReachability(x, y))
      {
         return true;
      }
      if(function_behavior->CheckReachability(y, x))
      {
         return false;
      }
   }
   return x < y;
}
