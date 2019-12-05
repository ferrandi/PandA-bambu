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
 * @file values_scheme.cpp
 * @brief Class implementation of values scheme for the storage value insertion phase
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
/// Header include
#include "values_scheme.hpp"

///. include
#include "Parameter.hpp"

/// HLS includes
#include "hls.hpp"
#include "hls_manager.hpp"

/// HLS/binding/storage_value_information includes
#include "storage_value_information_fsm.hpp"
#include "storage_value_information_pipeline.hpp"

/// HLS/liveness include
#include "liveness.hpp"

/// tree includes
#include "behavioral_helper.hpp"
#include "tree_helper.hpp"

/// utility include
#include "cpu_time.hpp"

/// include pipelining structure helper
#include "state_transition_graph.hpp"
#include "state_transition_graph_manager.hpp"

values_scheme::values_scheme(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager)
    : storage_value_insertion(_Param, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::VALUES_SCHEME_STORAGE_VALUE_INSERTION)
{
}

values_scheme::~values_scheme() = default;

void values_scheme::Initialize()
{
   HLSFunctionStep::Initialize();
   if(HLSMgr->CGetFunctionBehavior(HLS->functionId)->is_pipelining_enabled())
      HLS->storage_value_information = StorageValueInformationPipelineRef(new StorageValueInformationPipeline(HLSMgr, funId));
   else
      HLS->storage_value_information = StorageValueInformationFsmRef(new StorageValueInformationFsm(HLSMgr, funId));
   HLS->storage_value_information->Initialize();
}

DesignFlowStep_Status values_scheme::InternalExec()
{
   long step_time;
   START_TIME(step_time);
   THROW_ASSERT(HLS->Rliv, "Liveness analysis not yet computed");
   const tree_managerRef TreeM = HLSMgr->get_tree_manager();
   unsigned int i = 0;
   vertex current_vertex;
   vertex start_state = HLS->STG->get_entry_state();
   const std::list<vertex>& support = HLS->Rliv->get_support();
   last_intermediate_state fetch_previous(HLS->STG->GetStg(), HLSMgr->CGetFunctionBehavior(funId)->is_pipelining_enabled());
   next_unique_state get_next(HLS->STG->GetStg());

   const std::list<vertex>::const_iterator vEnd = support.end();
   for(auto vIt = support.begin(); vIt != vEnd; ++vIt)
   {
      // std::cerr << "current state for sv " << HLS->Rliv->get_name(*vIt) << std::endl;
      const CustomOrderedSet<unsigned int>& live = HLS->Rliv->get_live_in(*vIt);
      const CustomOrderedSet<unsigned int>::const_iterator k_end = live.end();
      for(auto k = live.begin(); k != k_end; ++k)
      {
         if(!HLS->storage_value_information->is_a_storage_value(*vIt, *k))
         {
            HLS->storage_value_information->set_storage_value_index(*vIt, *k, i);
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "In state " + HLS->Rliv->get_name(*vIt) + " variable " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(*k) + " is assigned to reg_" + std::to_string(i));
            HLS->storage_value_information->variable_index_map[i] = *k;
            i++;
         }
      }
      if(HLSMgr->CGetFunctionBehavior(HLS->functionId)->is_pipelining_enabled())
      {
         std::list<vertex> running_ops = HLS->STG->GetStg()->GetStateInfo(*vIt)->executing_operations;
         const std::list<vertex>::const_iterator opEnd = running_ops.end();
         vertex state_0 = get_next(start_state);
         for(auto opIt = running_ops.begin(); opIt != opEnd; ++opIt)
         {
            std::vector<HLS_manager::io_binding_type> inVars = HLSMgr->get_required_values(HLS->functionId, *opIt);
            for(unsigned int num = 0; num != inVars.size(); num++)
            {
               unsigned int var = std::get<0>(inVars[num]);
               if(tree_helper::is_parameter(TreeM, var))
               {
                  current_vertex = *vIt;
                  while(current_vertex != state_0)
                  {
                     if(!HLS->storage_value_information->is_a_storage_value(current_vertex, var))
                     {
                        HLS->storage_value_information->set_storage_value_index(current_vertex, var, i);
                        INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "In state " + HLS->Rliv->get_name(current_vertex) + " variable " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(var) + " is assigned to reg_" + std::to_string(i));
                        HLS->storage_value_information->variable_index_map[i] = var;
                        i++;
                     }
                     current_vertex = fetch_previous(start_state, current_vertex);
                  }
               }
            }
         }
      }
   }
   HLS->storage_value_information->number_of_storage_values = i;
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Storage Value Information of function " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->get_function_name() + ":");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Number of storage values inserted: " + std::to_string(i));
   STOP_TIME(step_time);
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "Time to compute storage value information: " + print_cpu_time(step_time) + " seconds");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   return DesignFlowStep_Status::SUCCESS;
}
