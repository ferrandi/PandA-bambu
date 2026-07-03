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
 * @file priority.cpp
 * @brief set of classes used to define different priority schemes.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "priority.hpp"

#include "ASLAP.hpp"
#include "op_graph.hpp"
#include "schedule.hpp"

priority_static_mobility::priority_static_mobility(const ASLAP& aslap)
{
   const auto& asap = aslap.CGetASAP();
   const auto& alap = aslap.CGetALAP();

   const auto& data = aslap.CGetOpGraph();
   for(const auto v : data.vertices())
   {
      if(!asap.is_scheduled(v))
      {
         continue;
      }
      /// Note that usually high priority in list based means low mobility.
      operator[](v) = static_cast<int>(asap.get_cstep(v).second - alap.get_cstep(v).second);
   }
}

priority_dynamic_mobility::priority_dynamic_mobility(const ASLAP& aslap, const OpVertexSet& _ready_nodes,
                                                     unsigned int _ctrl_step_multiplier)
    : ready_nodes(_ready_nodes), ctrl_step_multiplier(_ctrl_step_multiplier)
{
   const auto& asap = aslap.CGetASAP();
   const auto& alap = aslap.CGetALAP();

   const auto& data = aslap.CGetOpGraph();
   for(const auto v : data.vertices())
   {
      if(!asap.is_scheduled(v))
      {
         continue;
      }
      /// Note that usually high priority in list based means low mobility.
      operator[](v) = static_cast<int>(asap.get_cstep(v).second - alap.get_cstep(v).second);
   }
}

bool priority_dynamic_mobility::update()
{
   auto it_end = ready_nodes.end();
   auto it = ready_nodes.begin();
   if(it != it_end)
   {
      while(it != it_end)
      {
         operator[](*it) = operator[](*it) + static_cast<int>(ctrl_step_multiplier); /// increase priority
         it++;
      }
      return true;
   }
   else
   {
      return false;
   }
}

priority_fixed::priority_fixed(const CustomUnorderedMapUnstable<OpGraph::vertex_descriptor, int>& priority_value)
{
   for(const auto& i : priority_value)
   {
      operator[](i.first) = i.second;
   }
}
