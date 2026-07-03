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
 * @file easy_module_binding.cpp
 * @brief Class implementation of the partial module binding based on simple conditions.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "easy_module_binding.hpp"

#include "Parameter.hpp"
#include "allocation_information.hpp"
#include "behavioral_helper.hpp"
#include "cpu_time.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_node.hpp"

#include <iosfwd>
#include <string>
#include <tuple>

easy_module_binding::easy_module_binding(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr,
                                         unsigned int _funId, const DesignFlowManager& _design_flow_manager)
    : HLSFunctionStep(_Param, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::EASY_MODULE_BINDING)
{
   debug_level = _Param->get_class_debug_level(GET_CLASS(*this));
}

void easy_module_binding::Initialize()
{
   HLSFunctionStep::Initialize();
   if(!HLS->Rfu)
   {
      HLS->Rfu = fu_bindingRef(fu_binding::create_fu_binding(HLSMgr, funId, parameters));
   }
}

HLS_step::HLSRelationships
easy_module_binding::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::DOMINATOR_ALLOCATION, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::WHOLE_APPLICATION));
         if(HLSMgr->get_HLS(funId))
         {
            ret.insert(std::make_tuple(HLSMgr->get_HLS(funId)->chaining_algorithm, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

DesignFlowStep_Status easy_module_binding::InternalExec()
{
   long step_time = 0;
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      START_TIME(step_time);
   }
   const auto TM = HLSMgr->get_ir_manager();
   // resource binding and allocation  info
   fu_binding& fu = *(HLS->Rfu);
   const auto allocation_information = HLS->allocation_information;
   // pointer to a Control, Data dependence and antidependence graph graph
   const auto FB = HLSMgr->CGetFunctionBehavior(funId);
   const auto sdg = FB->GetOpGraph(FunctionBehavior::SDG);

   unsigned int fu_unit;
   /// compute unshared resources
   std::map<unsigned int, unsigned int> n_shared_fu;
   for(const auto operation : sdg.CGetOperations())
   {
      const auto id = sdg.CGetNodeInfo(operation).GetNodeId();
      if(id == ENTRY_ID || id == EXIT_ID)
      {
         continue;
      }
      fu_unit = fu.get_assign(operation);
      if(allocation_information->is_vertex_bounded(fu_unit))
      {
         continue;
      }
      if(n_shared_fu.find(fu_unit) == n_shared_fu.end())
      {
         n_shared_fu[fu_unit] = 1;
      }
      else
      {
         n_shared_fu[fu_unit] = 1 + n_shared_fu[fu_unit];
      }
   }
   if(output_level >= OUTPUT_LEVEL_MINIMUM && output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  "-->Easy binding information for function " + FB->CGetBehavioralHelper()->GetFunctionName() + ":");
   /// check easy binding and compute the list of vertices for which a sharing is possible

   CustomOrderedSet<OpGraph::vertex_descriptor> easy_bound_vertices;
   for(const auto op : sdg.CGetOperations())
   {
      if(fu.get_index(op) != INFINITE_UINT)
      {
         continue;
      }
      fu_unit = fu.get_assign(op);
      if(allocation_information->is_vertex_bounded(fu_unit) ||
         (allocation_information->is_memory_unit(fu_unit) &&
          (!allocation_information->is_readonly_memory_unit(fu_unit) ||
           (!allocation_information->is_one_cycle_direct_access_memory_unit(fu_unit) &&
            (!parameters->isOption(OPT_rom_duplication) || !parameters->getOption<bool>(OPT_rom_duplication)))) &&
          allocation_information->get_number_channels(fu_unit) == 1) ||
         n_shared_fu.find(fu_unit)->second == 1)
      {
         fu.bind(op, fu_unit, 0);
         easy_bound_vertices.insert(op);
         const auto node_info = sdg.CGetNodeInfo(op);
         if(node_info.GetNodeId())
         {
            INDENT_OUT_MEX(
                OUTPUT_LEVEL_VERY_PEDANTIC, output_level,
                "---" + node_info.vertex_name + "(" +
                    (node_info.GetNodeId() == ENTRY_ID ?
                         "ENTRY" :
                         (node_info.GetNodeId() == EXIT_ID ? "EXIT" :
                                                             TM->GetIRNode(node_info.GetNodeId())->ToString())) +
                    ") bound to " + allocation_information->get_fu_name(fu_unit).first + "(0)");
         }
      }
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  "---Bound operations:" + STR(easy_bound_vertices.size()) + "/" + STR(boost::num_vertices(sdg)));

   if(output_level >= OUTPUT_LEVEL_MINIMUM && output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      STOP_TIME(step_time);
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "Time to perform easy binding: " + print_cpu_time(step_time) + " seconds");
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   }
   return DesignFlowStep_Status::SUCCESS;
}
