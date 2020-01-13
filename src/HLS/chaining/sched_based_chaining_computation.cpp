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
 * @file sched_based_chaining_computation.cpp
 * @brief chaining computation starting from the results of the scheduling step
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */

#include "sched_based_chaining_computation.hpp"

#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"

/// STD include
#include "custom_map.hpp"
#include "custom_set.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "schedule.hpp"

/// HLS/chaining includes
#include "chaining_information.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

sched_based_chaining_computation::sched_based_chaining_computation(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager)
    : chaining(_Param, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::SCHED_CHAINING)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

sched_based_chaining_computation::~sched_based_chaining_computation() = default;

void sched_based_chaining_computation::Initialize()
{
   HLSFunctionStep::Initialize();
   HLS->chaining_information = ChainingInformationRef(new ChainingInformation(HLSMgr, funId));
   HLS->chaining_information->Initialize();
}

DesignFlowStep_Status sched_based_chaining_computation::InternalExec()
{
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   const OpGraphConstRef flow_graph = FB->CGetOpGraph(FunctionBehavior::FLSAODG);

   VertexIterator op, op_end;
   for(boost::tie(op, op_end) = boost::vertices(*flow_graph); op != op_end; ++op)
   {
      const auto current_starting_cycle = HLS->Rsch->get_cstep(*op);
      const auto current_ending_cycle = HLS->Rsch->get_cstep_end(*op);
      if(current_starting_cycle == current_ending_cycle)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Operations " + GET_NAME(flow_graph, *op) + " and " + GET_NAME(flow_graph, *op) + " are chained in");
         HLS->chaining_information->add_chained_vertices_in(*op, *op);
      }
      InEdgeIterator ei, ei_end;
      bool is_chained_test = false;
      for(boost::tie(ei, ei_end) = boost::in_edges(*op, *flow_graph); ei != ei_end; ei++)
      {
         vertex src = boost::source(*ei, *flow_graph);
         if(HLS->Rsch->get_cstep_end(src) == current_starting_cycle)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, std::string("Operations ") + GET_NAME(flow_graph, src) + " and " + GET_NAME(flow_graph, *op) + " are chained in");
            HLS->chaining_information->add_chained_vertices_in(*op, src);
            is_chained_test = true;
         }
      }
      if(is_chained_test)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, std::string("Operations ") + GET_NAME(flow_graph, *op) + " is chained with something");
         HLS->chaining_information->is_chained_with.insert(*op);
      }
      OutEdgeIterator eo, eo_end;
      for(boost::tie(eo, eo_end) = boost::out_edges(*op, *flow_graph); eo != eo_end; eo++)
      {
         vertex tgt = boost::target(*eo, *flow_graph);
         if(HLS->Rsch->get_cstep(tgt) == current_ending_cycle)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, std::string("Operations ") + GET_NAME(flow_graph, tgt) + " and " + GET_NAME(flow_graph, *op) + " are chained out");
            HLS->chaining_information->add_chained_vertices_out(*op, tgt);
         }
      }
   }
   return DesignFlowStep_Status::SUCCESS;
}
