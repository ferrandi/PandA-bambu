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
 * @file easy_module_binding.cpp
 * @brief Class implementation of the partial module binding based on simple conditions.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "easy_module_binding.hpp"

#include "function_behavior.hpp"
#include "hls_manager.hpp"

#include "fu_binding.hpp"
#include "graph.hpp"
#include "op_graph.hpp"
#include "parallel_memory_fu_binding.hpp"

#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_node.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"

/// HLS include
#include "hls.hpp"

/// HLS/allocation_information include
#include "allocation_information.hpp"

#include "custom_map.hpp"
#include "custom_set.hpp"

/// tree include
#include "behavioral_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

/// utility include
#include "cpu_time.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

#include "custom_set.hpp"
#include <iosfwd>
#include <string>
#include <tuple>

easy_module_binding::easy_module_binding(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager)
    : HLSFunctionStep(_Param, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::EASY_MODULE_BINDING)
{
   debug_level = _Param->get_class_debug_level(GET_CLASS(*this));
}

easy_module_binding::~easy_module_binding() = default;

void easy_module_binding::Initialize()
{
   HLSFunctionStep::Initialize();
   if(not HLS->Rfu)
   {
      if(parameters->getOption<int>(OPT_memory_banks_number) > 1 && !parameters->isOption(OPT_context_switch))
      {
         HLS->Rfu = fu_bindingRef(new ParallelMemoryFuBinding(HLSMgr, funId, parameters));
      }
      else
      {
         HLS->Rfu = fu_bindingRef(fu_binding::create_fu_binding(HLSMgr, funId, parameters));
      }
   }
}

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> easy_module_binding::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::INITIALIZE_HLS, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         if(HLSMgr->get_HLS(funId))
         {
            ret.insert(std::make_tuple(HLSMgr->get_HLS(funId)->chaining_algorithm, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
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
      START_TIME(step_time);
   const auto TM = HLSMgr->get_tree_manager();
   // resource binding and allocation  info
   fu_binding& fu = *(HLS->Rfu);
   const AllocationInformationConstRef allocation_information = HLS->allocation_information;
   // pointer to a Control, Data dependence and antidependence graph graph
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   const OpGraphConstRef sdg = FB->CGetOpGraph(FunctionBehavior::SDG);

   unsigned int fu_unit;
   /// compute unshared resources
   std::map<unsigned int, unsigned int> n_shared_fu;
   for(const auto operation : sdg->CGetOperations())
   {
      const auto id = sdg->CGetOpNodeInfo(operation)->GetNodeId();
      if(id == ENTRY_ID or id == EXIT_ID)
         continue;
      fu_unit = fu.get_assign(operation);
      if(allocation_information->is_vertex_bounded(fu_unit))
         continue;
      if(n_shared_fu.find(fu_unit) == n_shared_fu.end())
         n_shared_fu[fu_unit] = 1;
      else
         n_shared_fu[fu_unit] = 1 + n_shared_fu[fu_unit];
   }
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Easy binding information for function " + FB->CGetBehavioralHelper()->get_function_name() + ":");
   /// check easy binding and compute the list of vertices for which a sharing is possible
   if(HLSMgr->GetFunctionBehavior(funId)->build_simple_pipeline())
   {
      std::set<vertex> bound_vertices;
      std::map<unsigned int, unsigned int> fu_instances;
      for(const auto op : sdg->CGetOperations())
      {
         if(fu.get_index(op) != INFINITE_UINT)
            continue;
         fu_unit = fu.get_assign(op);
         if(fu_instances.find(fu_unit) == fu_instances.end())
            fu_instances.insert(std::pair<unsigned int, unsigned int>(fu_unit, 0));
         fu.bind(op, fu_unit, fu_instances[fu_unit]);
         fu_instances[fu_unit]++;
         bound_vertices.insert(op);
         const auto node_id = sdg->CGetOpNodeInfo(op)->GetNodeId();
         if(node_id)
         {
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level,
                           "---" + GET_NAME(sdg, op) + "(" + (node_id == ENTRY_ID ? "ENTRY" : (node_id == EXIT_ID ? "EXIT" : TM->get_tree_node_const(node_id)->ToString())) + ") bound to " + allocation_information->get_fu_name(fu_unit).first + "(0)");
         }
      }
   }
   else
   {
      CustomOrderedSet<vertex> easy_bound_vertices;
      for(const auto op : sdg->CGetOperations())
      {
         if(fu.get_index(op) != INFINITE_UINT)
            continue;
         fu_unit = fu.get_assign(op);
         if(allocation_information->is_vertex_bounded(fu_unit) ||
            (allocation_information->is_memory_unit(fu_unit) &&
             (!allocation_information->is_readonly_memory_unit(fu_unit) || (!allocation_information->is_one_cycle_direct_access_memory_unit(fu_unit) && (!parameters->isOption(OPT_rom_duplication) || !parameters->getOption<bool>(OPT_rom_duplication)))) &&
             allocation_information->get_number_channels(fu_unit) == 1) ||
            n_shared_fu.find(fu_unit)->second == 1)
         {
            fu.bind(op, fu_unit, 0);
            easy_bound_vertices.insert(op);
            const auto node_id = sdg->CGetOpNodeInfo(op)->GetNodeId();
            if(node_id)
            {
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level,
                              "---" + GET_NAME(sdg, op) + "(" + (node_id == ENTRY_ID ? "ENTRY" : (node_id == EXIT_ID ? "EXIT" : TM->get_tree_node_const(node_id)->ToString())) + ") bound to " + allocation_information->get_fu_name(fu_unit).first + "(0)");
            }
         }
      }
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Bound operations:" + STR(easy_bound_vertices.size()) + "/" + STR(boost::num_vertices(*sdg)));
   }
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      STOP_TIME(step_time);
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "Time to perform easy binding: " + print_cpu_time(step_time) + " seconds");
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   return DesignFlowStep_Status::SUCCESS;
}
