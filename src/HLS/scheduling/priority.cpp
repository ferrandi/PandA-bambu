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
 * @file priority.cpp
 * @brief set of classes used to define different priority schemes.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 * $Locker:  $
 * $State: Exp $
 *
 */

#include "priority.hpp"
#include "ASLAP.hpp"

#include "op_graph.hpp"

#include "schedule.hpp"

priority_static_mobility::priority_static_mobility(const ASLAPRef& aslap)
{
   const ScheduleConstRef asap = aslap->CGetASAP();
   const ScheduleConstRef alap = aslap->CGetALAP();

   const OpGraphConstRef data = aslap->CGetOpGraph();
   VertexIterator vIt, vEnd;
   for(boost::tie(vIt, vEnd) = boost::vertices(*data); vIt != vEnd; vIt++)
   {
      if(!asap->is_scheduled(*vIt))
         continue;
      operator[](*vIt) = from_strongtype_cast<int>(-alap->get_cstep(*vIt).second + asap->get_cstep(*vIt).second); /// Note that usually high priority in list based means low mobility.
   }
}

priority_dynamic_mobility::priority_dynamic_mobility(const ASLAPRef& aslap, const OpVertexSet& _ready_nodes, unsigned int _ctrl_step_multiplier) : ready_nodes(_ready_nodes), ctrl_step_multiplier(_ctrl_step_multiplier)
{
   const ScheduleConstRef asap = aslap->CGetASAP();
   const ScheduleConstRef alap = aslap->CGetALAP();

   const OpGraphConstRef data = aslap->CGetOpGraph();
   VertexIterator vIt, vEnd;
   for(boost::tie(vIt, vEnd) = boost::vertices(*data); vIt != vEnd; vIt++)
   {
      if(!asap->is_scheduled(*vIt))
         continue;
      operator[](*vIt) = from_strongtype_cast<int>(-alap->get_cstep(*vIt).second + asap->get_cstep(*vIt).second); /// Note that usually high priority in list based means low mobility.
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
      return false;
}

priority_fixed::priority_fixed(const CustomUnorderedMapUnstable<vertex, int>& priority_value)
{
   auto it_end = priority_value.end();
   for(auto it = priority_value.begin(); it != it_end; ++it)
      operator[](it->first) = it->second;
}
