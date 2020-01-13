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
 * @file unique_binding.cpp
 * @brief Class implementation of a unique binding algorithm
 *
 * This class implements a unique algorithm for operation binding
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @version $Revision$
 * @date $Date$
 */
#include "unique_binding.hpp"

#include "graph.hpp"

#include "fu_binding.hpp"
#include "hls.hpp"
#include "schedule.hpp"

#include "Parameter.hpp"
#include "constant_strings.hpp"

#include "op_graph.hpp"

#include "utility.hpp"

/// HLS/module_allocation include
#include "allocation_information.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_

unique_binding::unique_binding(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager)
    : fu_binding_creator(_Param, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::UNIQUE_MODULE_BINDING)
{
}

unique_binding::~unique_binding() = default;

DesignFlowStep_Status unique_binding::InternalExec()
{
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   const OpGraphConstRef data = FB->CGetOpGraph(FunctionBehavior::CFG);

   VertexIterator vIt, vItEnd;
   std::map<unsigned int, CustomOrderedSet<unsigned int>> black_list;
   std::map<unsigned int, std::list<std::pair<std::string, vertex>>> fu_ops;
   for(boost::tie(vIt, vItEnd) = boost::vertices(*data); vIt != vItEnd; vIt++)
   {
      unsigned int fu = HLS->Rfu->get_assign(*vIt);
      if(HLS->Rfu->get_index(*vIt) != INFINITE_UINT)
         black_list[fu].insert(HLS->Rfu->get_index(*vIt));
      else if(HLS->allocation_information->is_vertex_bounded(fu) || HLS->allocation_information->is_memory_unit(fu))
         HLS->Rfu->bind(*vIt, fu, 0);
      else
         fu_ops[fu].push_back(std::make_pair(GET_NAME(data, *vIt), *vIt));
      if(black_list.find(fu) == black_list.end())
         black_list[fu] = CustomOrderedSet<unsigned int>();
   }
   for(auto& fu_op : fu_ops)
   {
      unsigned int fu = fu_op.first;
      fu_op.second.sort();
      for(auto& op : fu_op.second)
      {
         unsigned int idx = 0;
         while(black_list[fu].find(idx) != black_list[fu].end())
         {
            idx++;
         }
         HLS->Rfu->bind(op.second, fu, idx);
         black_list[fu].insert(idx);
      }
   }
   if(debug_level >= DEBUG_LEVEL_VERBOSE)
      HLS->Rsch->print(HLS->Rfu);
   return DesignFlowStep_Status::SUCCESS;
}
