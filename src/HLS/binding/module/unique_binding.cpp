/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file unique_binding.cpp
 * @brief Class implementation of a unique binding algorithm
 *
 * This class implements a unique algorithm for operation binding
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 */
#include "unique_binding.hpp"

#include "Parameter.hpp"
#include "allocation_information.hpp"
#include "constant_strings.hpp"
#include "dbgPrintHelper.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "hls.hpp"
#include "op_graph.hpp"
#include "schedule.hpp"
#include "utility.hpp"

unique_binding::unique_binding(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId,
                               const DesignFlowManager& _design_flow_manager)
    : fu_binding_creator(_Param, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::UNIQUE_MODULE_BINDING)
{
}

DesignFlowStep_Status unique_binding::InternalExec()
{
   const auto FB = HLSMgr->CGetFunctionBehavior(funId);
   const auto data = FB->GetOpGraph(FunctionBehavior::CFG);

   std::map<unsigned int, CustomOrderedSet<unsigned int>> black_list;
   std::map<unsigned int, std::list<std::pair<std::string, OpGraph::vertex_descriptor>>> fu_ops;
   for(const auto v : data.vertices())
   {
      unsigned int fu = HLS->Rfu->get_assign(v);
      if(HLS->Rfu->get_index(v) != INFINITE_UINT)
      {
         black_list[fu].insert(HLS->Rfu->get_index(v));
      }
      else if(HLS->allocation_information->is_vertex_bounded(fu) || HLS->allocation_information->is_memory_unit(fu))
      {
         HLS->Rfu->bind(v, fu, 0);
      }
      else
      {
         fu_ops[fu].push_back(std::make_pair(data.CGetNodeInfo(v).vertex_name, v));
      }
      black_list.insert(std::make_pair(fu, CustomOrderedSet<unsigned int>()));
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
   {
      HLS->Rsch->print(HLS->Rfu);
   }
   return DesignFlowStep_Status::SUCCESS;
}
