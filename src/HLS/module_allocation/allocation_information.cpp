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
 * @file allocation_information.cpp
 * @brief This package is used by all HLS packages to manage resource constraints and characteristics.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "allocation_information.hpp"

#include "FSMInfo.hpp"
#include "Parameter.hpp"
#include "allocation.hpp"
#include "allocation_constants.hpp"
#include "area_estimation.hpp"
#include "area_info.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "math_function.hpp"
#include "memory.hpp"
#include "schedule.hpp"
#include "string_manipulation.hpp"
#include "structural_manager.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "time_info.hpp"
#include "typed_node_info.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <tuple>

const std::tuple<const std::vector<unsigned int>&, const std::vector<unsigned int>&>
AllocationInformation::InitializeDSPDB(const AllocationInformation* allocation_information)
{
   static std::vector<unsigned int> DSP_x_db;
   static std::vector<unsigned int> DSP_y_db;
   if(!(DSP_x_db.size() || DSP_y_db.size()))
   {
      /// initialize DSP x and y db
      const auto hls_d = allocation_information->hls_manager->get_HLS_device();
      std::string DSPs_x_sizes;
      const bool has_dsp_x_sizes =
          allocation_information->hls_manager->TryGetParameterFromParameterOrDevice<std::string>("DSPs_x_sizes", hls_d,
                                                                                                 DSPs_x_sizes);
      if(has_dsp_x_sizes)
      {
         std::string DSPs_y_sizes;
         const bool has_dsp_y_sizes =
             allocation_information->hls_manager->TryGetParameterFromParameterOrDevice<std::string>(
                 "DSPs_y_sizes", hls_d, DSPs_y_sizes);
         THROW_ASSERT(has_dsp_y_sizes, "device description is not complete");
         (void)(has_dsp_y_sizes);
         const auto DSPs_x_sizes_vec = string_to_container<std::vector<std::string>>(DSPs_x_sizes, ",");
         const auto DSPs_y_sizes_vec = string_to_container<std::vector<std::string>>(DSPs_y_sizes, ",");
         size_t n_elements = DSPs_x_sizes_vec.size();
         DSP_x_db.resize(n_elements);
         DSP_y_db.resize(n_elements);
         for(size_t index = 0; index < n_elements; ++index)
         {
            DSP_x_db[index] = static_cast<unsigned>(std::stoul(DSPs_x_sizes_vec[index]));
            DSP_y_db[index] = static_cast<unsigned>(std::stoul(DSPs_y_sizes_vec[index]));
         }
      }
   }
   return {DSP_x_db, DSP_y_db};
}

static const double epsilon = 0.000000001;

namespace
{
   unsigned int computeMuxTreeLevels(unsigned int mux_ins)
   {
      unsigned int levels = 0;
      unsigned long long covered_inputs = 1;
      while(covered_inputs < mux_ins)
      {
         covered_inputs <<= 1U;
         ++levels;
      }
      return levels;
   }
} // namespace

AllocationInformation::AllocationInformation(const HLS_managerRef _hls_manager, const unsigned int _function_index,
                                             const ParameterConstRef _parameters)
    : hls_manager(_hls_manager),
      parameters(_parameters),
      IRM(_hls_manager->get_ir_manager()),
      op_graph(hls_manager->CGetFunctionBehavior(_function_index)->GetOpGraph(FunctionBehavior::CFG)),
      behavioral_helper(_hls_manager->CGetFunctionBehavior(_function_index)->CGetBehavioralHelper()),
      hls(_hls_manager->get_HLS(_function_index)),
      HLS_C(hls->HLS_C),
      HLS_D(hls->HLS_D),
      function_index(_function_index),
      address_bitsize(_hls_manager->Rget_address_bitsize())
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

double AllocationInformation::time_m_execution_time(operation* op) const
{
   return op->time_m->get_execution_time() * time_multiplier;
}

double AllocationInformation::time_m_stage_period(operation* op) const
{
   return op->time_m->get_stage_period() * time_multiplier;
}

double AllocationInformation::lut_time_unit(unsigned int in_bits) const
{
   const auto max_lut_size = hls_manager->GetRequiredParameterFromParameterOrDevice<size_t>("max_lut_size", HLS_D);
   THROW_ASSERT(max_lut_size > 0, "Invalid parameter \"max_lut_size\": expected value > 0");
   THROW_ASSERT(in_bits > 0 && in_bits <= max_lut_size,
                "Unexpected lut input count " + STR(in_bits) + " for max_lut_size " + STR(max_lut_size));
   const technology_managerRef TM = HLS_D->get_technology_manager();
   technology_nodeRef f_unit_lut = TM->get_fu(LUT_NODE_STD, LIBRARY_STD_FU);
   THROW_ASSERT(f_unit_lut, "Library miss component: " + std::string(LUT_NODE_STD));
   auto* fu_lut = GetPointerS<functional_unit>(f_unit_lut);
   technology_nodeRef op_lut_node = fu_lut->get_operation("lut_node");
   auto* op_lut = GetPointerS<operation>(op_lut_node);
   return time_m_execution_time(op_lut) * static_cast<double>(in_bits) / static_cast<double>(max_lut_size);
}

double AllocationInformation::lut_area_unit(unsigned int in_bits) const
{
   const auto max_lut_size = hls_manager->GetRequiredParameterFromParameterOrDevice<size_t>("max_lut_size", HLS_D);
   THROW_ASSERT(max_lut_size > 0, "Invalid parameter \"max_lut_size\": expected value > 0");
   THROW_ASSERT(in_bits > 0 && in_bits <= max_lut_size,
                "Unexpected lut input count " + STR(in_bits) + " for max_lut_size " + STR(max_lut_size));
   const technology_managerRef TM = HLS_D->get_technology_manager();
   technology_nodeRef f_unit_lut = TM->get_fu(LUT_NODE_STD, LIBRARY_STD_FU);
   THROW_ASSERT(f_unit_lut, "Library miss component: " + std::string(LUT_NODE_STD));
   auto* fu_lut = GetPointerS<functional_unit>(f_unit_lut);
   const double base_area = area_estimation::get_lut_equivalent_area(HLS_D, fu_lut->area_m);
   return base_area * static_cast<double>(in_bits) / static_cast<double>(max_lut_size);
}

std::pair<std::string, std::string> AllocationInformation::get_fu_name(unsigned int id) const
{
   THROW_ASSERT(id_to_fu_names.find(id) != id_to_fu_names.end(), "Functional unit name not stored!");
   return id_to_fu_names.find(id)->second;
}

unsigned int AllocationInformation::get_number_fu_types() const
{
   return static_cast<unsigned int>(list_of_FU.size());
}

unsigned int AllocationInformation::get_number_fu(unsigned int fu_name) const
{
   THROW_ASSERT(fu_name < get_number_fu_types(), "functional unit id not meaningful");
   return tech_constraints[fu_name];
}

const CustomOrderedSet<unsigned int>& AllocationInformation::can_implement_set(OpGraph::vertex_descriptor v) const
{
   return can_implement_set(op_graph.CGetNodeInfo(v).GetNodeId());
}

const CustomOrderedSet<unsigned int>& AllocationInformation::can_implement_set(const unsigned int v) const
{
   const auto node_operation = [&]() -> std::string {
      if(v == ENTRY_ID)
      {
         return "Entry";
      }
      if(v == EXIT_ID)
      {
         return "Exit";
      }
      return GetPointerS<const node_stmt>(IRM->GetIRNode(v))->operation;
   }();
   const auto vtf_it = node_id_to_fus.find(std::pair<unsigned int, std::string>(v, node_operation));
   THROW_ASSERT(vtf_it != node_id_to_fus.end(), "unmapped operation " + IRM->GetIRNode(v)->ToString());
   return vtf_it->second;
}

bool AllocationInformation::CanImplementSetNotEmpty(const unsigned int v) const
{
   if(v == ENTRY_ID)
   {
      return true;
   }
   if(v == EXIT_ID)
   {
      return true;
   }
   const auto node_operation = GetPointerS<const node_stmt>(IRM->GetIRNode(v))->operation;
   return node_id_to_fus.find(std::pair<unsigned int, std::string>(v, node_operation)) != node_id_to_fus.end();
}

double AllocationInformation::get_execution_time(const unsigned int fu_name, OpGraph::vertex_descriptor v,
                                                 const OpGraph& g) const
{
   return get_execution_time(fu_name, g.CGetNodeInfo(v).GetNodeId());
}

double AllocationInformation::get_execution_time(const unsigned int fu_name, unsigned int v) const
{
   if(v == ENTRY_ID || v == EXIT_ID)
   {
      return 0.0;
   }
   THROW_ASSERT(can_implement_set(v).find(fu_name) != can_implement_set(v).end(),
                "This function (" + get_string_name(fu_name) + ") cannot implement the operation " + STR(v));
   if(!has_to_be_synthetized(fu_name))
   {
      return 0.0;
   }
   const auto operation_name = ir_helper::NormalizeTypename(GetPointerS<const node_stmt>(IRM->GetIRNode(v))->operation);
   const auto node_op = GetPointerS<functional_unit>(list_of_FU[fu_name])->get_operation(operation_name);
   THROW_ASSERT(GetPointerS<operation>(node_op)->time_m,
                "Timing information not specified for unit " + id_to_fu_names.find(fu_name)->second.first);
   double clock_budget = HLS_C->get_clock_period() * HLS_C->get_clock_period_resource_fraction();
   auto n_cycles = GetPointerS<operation>(node_op)->time_m->get_cycles();
   if(n_cycles)
   {
      const double stage_time = [&]() -> double {
         /// first check for component_timing_alias
         if(GetPointerS<functional_unit>(list_of_FU[fu_name])->component_timing_alias != "")
         {
            const auto& component_name = GetPointerS<functional_unit>(list_of_FU[fu_name])->component_timing_alias;
            const auto library = HLS_D->get_technology_manager()->get_library(component_name);
            const auto f_unit_alias = HLS_D->get_technology_manager()->get_fu(component_name, library);
            THROW_ASSERT(f_unit_alias, "Library miss component: " + component_name);
            const auto fu_alias = GetPointerS<functional_unit>(f_unit_alias);
            const auto op_alias_node = fu_alias->get_operation(operation_name);
            const auto op_alias = op_alias_node ? GetPointerS<operation>(op_alias_node) :
                                                  GetPointerS<operation>(fu_alias->get_operations().front());
            const auto ret = time_m_stage_period(op_alias);
            return ret;
         }
         else
         {
            return time_m_stage_period(GetPointerS<operation>(node_op));
         }
      }();
      if(stage_time < clock_budget && stage_time > 0)
      {
         return (n_cycles - 1) * clock_budget + stage_time;
      }
      else
      {
         double exec_time = get_execution_time_dsp_modified(fu_name, node_op);
         if(exec_time > (n_cycles - 1) * clock_budget && exec_time < n_cycles * clock_budget)
         {
            return exec_time;
         }
         else
         {
            return n_cycles * clock_budget;
         }
      }
   }
   /// DSP based components are underestimated when the RTL synthesis backend converts in LUTs, so we slightly increase
   /// the execution time
   if(GetPointerS<functional_unit>(list_of_FU[fu_name])->component_timing_alias != "")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Using alias");
      const auto& component_name = GetPointerS<functional_unit>(list_of_FU[fu_name])->component_timing_alias;
      const auto library = HLS_D->get_technology_manager()->get_library(component_name);
      const auto f_unit_alias = HLS_D->get_technology_manager()->get_fu(component_name, library);
      THROW_ASSERT(f_unit_alias, "Library miss component: " + component_name);
      const auto fu_alias = GetPointerS<functional_unit>(f_unit_alias);
      /// FIXME: here we are passing fu_name and not the index of the alias function which does not exists; however
      /// fu_name is used to identify if the operation is mapped on the DSP, so for non DSP operations works
      auto op_alias_node = fu_alias->get_operation(operation_name);
      op_alias_node = op_alias_node ? op_alias_node : fu_alias->get_operations().front();
      return get_execution_time_dsp_modified(fu_name, op_alias_node);
   }

   return get_execution_time_dsp_modified(fu_name, node_op);
}

double AllocationInformation::get_attribute_of_fu_per_op(OpGraph::vertex_descriptor v, const OpGraph& g,
                                                         Allocation_MinMax allocation_min_max,
                                                         AllocationInformation::op_target target) const
{
   unsigned int fu_name;
   bool flag;
   double res = get_attribute_of_fu_per_op(v, g, allocation_min_max, target, fu_name, flag);
   THROW_ASSERT(flag, "something wrong happened");
   return res;
}

double AllocationInformation::get_attribute_of_fu_per_op(OpGraph::vertex_descriptor v, const OpGraph& g,
                                                         Allocation_MinMax allocation_min_max,
                                                         AllocationInformation::op_target target, unsigned int& fu_name,
                                                         bool& flag, const updatecopy_HLS_constraints_functor* CF) const
{
   const unsigned int node_id = g.CGetNodeInfo(v).GetNodeId();
   const auto node_operation = [&]() -> std::string {
      if(node_id == ENTRY_ID)
      {
         return "Entry";
      }
      if(node_id == EXIT_ID)
      {
         return "Exit";
      }
      return GetPointerS<const node_stmt>(IRM->GetIRNode(node_id))->operation;
   }();
   const CustomOrderedSet<unsigned int>& fu_set =
       node_id_to_fus.find(std::pair<unsigned int, std::string>(node_id, node_operation))->second;

   const auto op_name = ir_helper::NormalizeTypename(g.CGetNodeInfo(v).GetOperation());
   const CustomOrderedSet<unsigned int>::const_iterator f_end = fu_set.end();
   auto f_i = fu_set.begin();
   flag = false;
   while(CF && f_i != f_end &&
         ((*CF)(*f_i) <= 0 || (binding.find(node_id) != binding.end() && binding.find(node_id)->second.second != *f_i)))
   {
      ++f_i;
   }
   if(f_i == f_end)
   {
      return -1.0;
   }
   flag = true;

   switch(target)
   {
      case initiation_time:
      {
         unsigned int temp(0u);
         fu_name = *f_i;
         if(!has_to_be_synthetized(fu_name))
         {
            return 1.0;
         }

         THROW_ASSERT(
             GetPointerS<operation>(GetPointerS<functional_unit>(list_of_FU[fu_name])->get_operation(op_name))->time_m,
             "Timing information not specified for operation " + op_name + " on unit " +
                 id_to_fu_names.find(fu_name)->second.first);
         auto int_value =
             GetPointerS<operation>(GetPointerS<functional_unit>(list_of_FU[fu_name])->get_operation(op_name))
                 ->time_m->get_initiation_time();

         if(binding.find(node_id) != binding.end() && binding.find(node_id)->second.second == fu_name)
         {
            return int_value;
         }
         ++f_i;

         for(; f_i != f_end; ++f_i)
         {
            if(CF && (*CF)(*f_i) <= 0)
            {
               continue;
            }
            switch(allocation_min_max)
            {
               case Allocation_MinMax::MAX:
                  THROW_ASSERT(
                      GetPointerS<operation>(GetPointerS<functional_unit>(list_of_FU[*f_i])->get_operation(op_name))
                          ->time_m,
                      "Timing information not specified for operation " + op_name + " on unit " +
                          id_to_fu_names.find(*f_i)->second.first);
                  temp = std::max(int_value, GetPointerS<operation>(
                                                 GetPointerS<functional_unit>(list_of_FU[*f_i])->get_operation(op_name))
                                                 ->time_m->get_initiation_time());
                  break;
               case Allocation_MinMax::MIN:
                  THROW_ASSERT(
                      GetPointerS<operation>(GetPointerS<functional_unit>(list_of_FU[*f_i])->get_operation(op_name))
                          ->time_m,
                      "Timing information not specified for operation " + op_name + " on unit " +
                          id_to_fu_names.find(*f_i)->second.first);
                  temp = std::min(int_value, GetPointerS<operation>(
                                                 GetPointerS<functional_unit>(list_of_FU[*f_i])->get_operation(op_name))
                                                 ->time_m->get_initiation_time());
                  break;
               default:
                  temp = 0u;
                  THROW_ERROR(std::string("Not supported AllocationInformation::op_performed"));
                  break;
            }
            if(temp != int_value)
            {
               fu_name = *f_i;
               int_value = temp;
            }
         }
         return int_value;
      }
      case execution_time:
      {
         double temp;
         fu_name = *f_i;
         if(!has_to_be_synthetized(fu_name))
         {
            return 0.0;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Get Execution time " + g.CGetNodeInfo(v).vertex_name);
         THROW_ASSERT(GetPointerS<functional_unit>(list_of_FU[fu_name]), "");
         THROW_ASSERT(GetPointerS<operation>(GetPointerS<functional_unit>(list_of_FU[fu_name])->get_operation(op_name)),
                      op_name + " not provided by " + list_of_FU[fu_name]->get_name());
         THROW_ASSERT(
             GetPointerS<operation>(GetPointerS<functional_unit>(list_of_FU[fu_name])->get_operation(op_name))->time_m,
             "Timing information not specified for operation " + op_name + " on unit " +
                 id_to_fu_names.find(fu_name)->second.first);
         double double_value = get_execution_time_dsp_modified(
             fu_name, GetPointerS<functional_unit>(list_of_FU[fu_name])->get_operation(op_name));
         if(binding.find(node_id) != binding.end() && binding.find(node_id)->second.second == fu_name)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Got Execution time: " + STR(double_value));
            return double_value;
         }
         ++f_i;
         for(; f_i != f_end; ++f_i)
         {
            if(CF && (*CF)(*f_i) <= 0)
            {
               continue;
            }
            switch(allocation_min_max)
            {
               case Allocation_MinMax::MAX:
                  THROW_ASSERT(
                      GetPointerS<operation>(GetPointerS<functional_unit>(list_of_FU[*f_i])->get_operation(op_name))
                          ->time_m,
                      "Timing information not specified for operation " + op_name + " on unit " +
                          id_to_fu_names.find(*f_i)->second.first);
                  temp = std::max(double_value,
                                  get_execution_time_dsp_modified(
                                      fu_name, GetPointerS<functional_unit>(list_of_FU[*f_i])->get_operation(op_name)));
                  break;
               case Allocation_MinMax::MIN:
                  THROW_ASSERT(
                      GetPointerS<operation>(GetPointerS<functional_unit>(list_of_FU[*f_i])->get_operation(op_name))
                          ->time_m,
                      "Timing information not specified for operation " + op_name + " on unit " +
                          id_to_fu_names.find(*f_i)->second.first);
                  temp = std::min(double_value,
                                  get_execution_time_dsp_modified(
                                      fu_name, GetPointerS<functional_unit>(list_of_FU[*f_i])->get_operation(op_name)));
                  break;
               default:
                  temp = 0;
                  THROW_ERROR(std::string("Not supported AllocationInformation::op_performed"));
                  break;
            }
            if(temp != double_value)
            {
               fu_name = *f_i;
               double_value = temp;
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Got Execution time: " + STR(double_value));
         return double_value;
      }
      case(power_consumption):
      default:
         THROW_ERROR(std::string("Not supported AllocationInformation::op_target"));
         break;
   }
   return -1.0;
}

unsigned int AllocationInformation::min_number_of_resources(OpGraph::vertex_descriptor v) const
{
   const auto node_id = op_graph.CGetNodeInfo(v).GetNodeId();
   if(node_id == ENTRY_ID)
   {
      return INFINITE_UINT;
   }
   if(node_id == EXIT_ID)
   {
      return INFINITE_UINT;
   }
   const auto operation = GetPointerS<const node_stmt>(IRM->GetIRNode(node_id))->operation;

   const CustomOrderedSet<unsigned int>& fu_set =
       node_id_to_fus.find(std::pair<unsigned int, std::string>(node_id, operation))->second;

   unsigned int min_num_res = INFINITE_UINT;
   const CustomOrderedSet<unsigned int>::const_iterator f_end = fu_set.end();

   for(auto f_i = fu_set.begin(); f_i != f_end; ++f_i)
   {
      unsigned int num_res = tech_constraints[*f_i];
      THROW_ASSERT(num_res != 0, "something wrong happened");
      min_num_res = min_num_res > num_res ? num_res : min_num_res;
   }
   return min_num_res;
}

double AllocationInformation::get_setup_hold_time() const
{
   return HLS_D->get_technology_manager()->CGetSetupHoldTime() * time_multiplier * setup_multiplier;
}

bool AllocationInformation::is_indirect_access_memory_unit(unsigned int fu_type) const
{
   technology_nodeRef current_fu = get_fu(fu_type);
   const auto& memory_ctrl_type = GetPointerS<functional_unit>(current_fu)->memory_ctrl_type;
   return memory_ctrl_type != "" && memory_ctrl_type != MEMORY_CTRL_TYPE_PROXY &&
          memory_ctrl_type != MEMORY_CTRL_TYPE_PROXYN && memory_ctrl_type != MEMORY_CTRL_TYPE_DPROXY &&
          memory_ctrl_type != MEMORY_CTRL_TYPE_DPROXYN && memory_ctrl_type != MEMORY_CTRL_TYPE_SPROXY &&
          memory_ctrl_type != MEMORY_CTRL_TYPE_SPROXYN;
}

double AllocationInformation::get_worst_execution_time(const unsigned int fu_name) const
{
   if(!has_to_be_synthetized(fu_name))
   {
      return 0.0;
   }
   const functional_unit::operation_vec node_ops = GetPointerS<functional_unit>(list_of_FU[fu_name])->get_operations();
   double max_value = 0.0;
   auto no_it_end = node_ops.end();
   for(auto no_it = node_ops.begin(); no_it != no_it_end; ++no_it)
   {
      max_value = std::max(max_value, get_execution_time_dsp_modified(fu_name, *no_it));
   }
   return max_value;
}

double AllocationInformation::get_area(const unsigned int fu_name) const
{
   THROW_ASSERT(fu_name < get_number_fu_types(), "functional unit id not meaningful");
   if(!has_to_be_synthetized(fu_name))
   {
      return 0.0;
   }
   area_infoRef a_m = GetPointerS<functional_unit>(list_of_FU[fu_name])->area_m;
   THROW_ASSERT(a_m, "Area information not specified for unit " + id_to_fu_names.find(fu_name)->second.first);
   return area_estimation::get_lut_equivalent_area(HLS_D, a_m);
}

double AllocationInformation::GetStatementArea(const unsigned int statement_index) const
{
   if(CanImplementSetNotEmpty(statement_index))
   {
      return get_area(GetFuType(statement_index));
   }

   const auto stmt = IRM->GetIRNode(statement_index);
   const auto stmt_kind = stmt->get_kind();
   if(stmt_kind == assign_stmt_K)
   {
      const auto ga = GetPointerS<const assign_stmt>(stmt);
      const auto op1_kind = ga->op1->get_kind();
      if(op1_kind == ssa_node_K || op1_kind == constant_int_val_node_K || op1_kind == nop_node_K ||
         op1_kind == concat_bit_node_K || op1_kind == extract_bit_node_K)
      {
         return 0.0;
      }
      else if((op1_kind == shr_node_K || op1_kind == shl_node_K) &&
              GetPointerS<const binary_node>(ga->op1)->op1->get_kind() == constant_int_val_node_K)
      {
         return 0.0;
      }
      else if(op1_kind == select_node_K)
      {
         THROW_ASSERT(ir_helper::Size(GetPointerS<const select_node>(ga->op1)->op0) == 1,
                      "Select node not allocated " + ga->op1->ToString());
         /// Computing time of select_node as time of select_node_FU - setup_time
         const auto data_bitsize = ir_helper::Size(ga->op0);
         const auto fu_prec = resize_1_8_pow2(data_bitsize);
         const auto op_area = mux_area_unit_raw(fu_prec);
         return op_area;
      }

      const auto data_bitsize = ir_helper::Size(ga->op0);
      const auto fu_prec = resize_1_8_pow2(data_bitsize);
      std::string fu_name;
      if(op1_kind == widen_mul_node_K || op1_kind == mul_node_K)
      {
         const auto in_prec = op1_kind == mul_node_K ? fu_prec : (fu_prec / 2);
         fu_name =
             ir_node::GetString(op1_kind) + "_FU_" + STR(in_prec) + "_" + STR(in_prec) + "_" + STR(fu_prec) + "_0";
      }
      else if(op1_kind == lut_node_K)
      {
         fu_name = ir_node::GetString(op1_kind) + "_FU";
      }
      else if(GetPointer<const unary_node>(ga->op1))
      {
         fu_name = ir_node::GetString(op1_kind) + "_FU_" + STR(fu_prec) + "_" + STR(fu_prec);
      }
      else if(GetPointer<const binary_node>(ga->op1))
      {
         fu_name = ir_node::GetString(op1_kind) + "_FU_" + STR(fu_prec) + "_" + STR(fu_prec) + "_" + STR(fu_prec);
      }
      else if(GetPointer<const ternary_node>(ga->op1))
      {
         fu_name = ir_node::GetString(op1_kind) + "_FU_" + STR(fu_prec) + "_" + STR(fu_prec) + "_" + STR(fu_prec) +
                   "_" + STR(fu_prec);
      }
      else
      {
         THROW_UNREACHABLE("Unhandled operation (" + ga->op1->get_kind_text() + ")" + STR(stmt));
      }
      const auto new_stmt_temp = HLS_D->get_technology_manager()->get_fu(fu_name, LIBRARY_STD_FU);
      THROW_ASSERT(new_stmt_temp, "Functional unit '" + fu_name + "' not found");
      const auto new_stmt_fu = GetPointerS<const functional_unit>(new_stmt_temp);
      return area_estimation::get_lut_equivalent_area(HLS_D, new_stmt_fu->area_m);
   }
   else if(stmt_kind == multi_way_if_stmt_K || stmt_kind == return_stmt_K)
   {
      return 0.0;
   }
   THROW_UNREACHABLE(STR(statement_index) + " - " + STR(stmt));
   return 0.0;
}

double AllocationInformation::get_DSPs(const unsigned int fu_name) const
{
   THROW_ASSERT(fu_name < get_number_fu_types(), "functional unit id not meaningful");
   if(!has_to_be_synthetized(fu_name))
   {
      return 0.0;
   }
   area_infoRef a_m = GetPointerS<functional_unit>(list_of_FU[fu_name])->area_m;
   THROW_ASSERT(a_m, "Area information not specified for unit " + id_to_fu_names.find(fu_name)->second.first);
   if(a_m)
   {
      return a_m->resource_or_default(area_info::DSP);
   }
   else
   {
      return 0;
   }
}

unsigned int AllocationInformation::get_initiation_time(const unsigned int fu_name, OpGraph::vertex_descriptor v) const
{
   return get_initiation_time(fu_name, op_graph.CGetNodeInfo(v).GetNodeId());
}

unsigned int AllocationInformation::get_initiation_time(const unsigned int fu_name,
                                                        const unsigned int statement_index) const
{
   if(statement_index == ENTRY_ID || statement_index == EXIT_ID)
   {
      return 0u;
   }
   const auto operation_name = GetPointerS<const node_stmt>(IRM->GetIRNode(statement_index))->operation;
   THROW_ASSERT(can_implement_set(statement_index).find(fu_name) != can_implement_set(statement_index).end(),
                "This function (" + get_string_name(fu_name) + ") cannot implement the operation " + operation_name);
   if(!has_to_be_synthetized(fu_name))
   {
      return 0u;
   }
   technology_nodeRef node_op =
       GetPointerS<functional_unit>(list_of_FU[fu_name])->get_operation(ir_helper::NormalizeTypename(operation_name));
   THROW_ASSERT(GetPointerS<operation>(node_op)->time_m,
                "Timing information not specified for unit " + id_to_fu_names.find(fu_name)->second.first);
   return GetPointerS<operation>(node_op)->time_m->get_initiation_time();
}

bool AllocationInformation::is_operation_bounded(const OpGraph& g, OpGraph::vertex_descriptor op,
                                                 unsigned int fu_type) const
{
   const technology_nodeRef node = get_fu(fu_type);
   const auto op_string = ir_helper::NormalizeTypename(g.CGetNodeInfo(op).GetOperation());
   const functional_unit* fu = GetPointerS<functional_unit>(node);
   const technology_nodeRef op_node = fu->get_operation(op_string);
   THROW_ASSERT(GetPointer<operation>(op_node), "Op node is not an operation");
   return GetPointerS<operation>(op_node)->is_bounded();
}

bool AllocationInformation::is_operation_bounded(const unsigned int index, unsigned int fu_type) const
{
   const technology_nodeRef node = get_fu(fu_type);
   const auto op_string = ir_helper::NormalizeTypename(GetPointerS<const node_stmt>(IRM->GetIRNode(index))->operation);
   const functional_unit* fu = GetPointerS<functional_unit>(node);
   const technology_nodeRef op_node = fu->get_operation(op_string);
   THROW_ASSERT(op_node, get_fu_name(fu_type).first + " cannot execute " + op_string);
   THROW_ASSERT(GetPointer<operation>(op_node), "Op node is not an operation: " + op_string);
   return GetPointerS<operation>(op_node)->is_bounded();
}

bool AllocationInformation::is_operation_PI_registered(const OpGraph& g, OpGraph::vertex_descriptor op,
                                                       unsigned int fu_type) const
{
   const technology_nodeRef node = get_fu(fu_type);
   const auto op_string = ir_helper::NormalizeTypename(g.CGetNodeInfo(op).GetOperation());
   const functional_unit* fu = GetPointerS<functional_unit>(node);
   const technology_nodeRef op_node = fu->get_operation(op_string);
   THROW_ASSERT(GetPointer<operation>(op_node), "Op node is not an operation");
   return GetPointerS<operation>(op_node)->is_primary_inputs_registered();
}

bool AllocationInformation::is_operation_PI_registered(const unsigned int index, unsigned int fu_type) const
{
   const technology_nodeRef node = get_fu(fu_type);
   const auto op_string = ir_helper::NormalizeTypename(GetPointerS<const node_stmt>(IRM->GetIRNode(index))->operation);
   const functional_unit* fu = GetPointerS<functional_unit>(node);
   const technology_nodeRef op_node = fu->get_operation(op_string);
   THROW_ASSERT(GetPointer<operation>(op_node), "Op node is not an operation");
   return GetPointerS<operation>(op_node)->is_primary_inputs_registered();
}

bool AllocationInformation::is_operation_PI_registered(const unsigned int index) const
{
   if(CanImplementSetNotEmpty(index))
   {
      return is_operation_PI_registered(index, GetFuType(index));
   }
   return false;
}

bool AllocationInformation::is_operation_bounded(const unsigned int index) const
{
   if(CanImplementSetNotEmpty(index))
   {
      return is_operation_bounded(index, GetFuType(index));
   }
   const auto tn = IRM->GetIRNode(index);

   /// currently all the operations introduced after the allocation has been performed are bounded
   if(tn->get_kind() == assign_stmt_K)
   {
#if HAVE_ASSERTS
      const auto right_kind = GetPointerS<const assign_stmt>(tn)->op1->get_kind();
#endif
      /// currently all the operations introduced after the allocation has been performed are bounded
      // BEAWARE: when adding operations here, check they are correctly handled by GetTimeLatency and GetCycleLatency
      THROW_ASSERT(ir_helper::IsConstant(GetPointerS<const assign_stmt>(tn)->op1) || right_kind == ssa_node_K ||
                       right_kind == select_node_K || right_kind == nop_node_K || right_kind == concat_bit_node_K ||
                       right_kind == extract_bit_node_K || right_kind == lut_node_K || right_kind == not_node_K ||
                       right_kind == neg_node_K || right_kind == xor_node_K || right_kind == or_node_K ||
                       right_kind == and_node_K || right_kind == shl_node_K || right_kind == shr_node_K ||
                       right_kind == widen_mul_node_K || right_kind == mul_node_K || right_kind == add_node_K ||
                       right_kind == sub_node_K || right_kind == ternary_add_node_K || right_kind == eq_node_K ||
                       right_kind == ne_node_K || right_kind == lt_node_K || right_kind == le_node_K ||
                       right_kind == gt_node_K || right_kind == ge_node_K || right_kind == ternary_sa_node_K ||
                       right_kind == ternary_as_node_K || right_kind == ternary_ss_node_K,
                   "Unexpected right part: " + ir_node::GetString(right_kind));
      return true;
   }
   if(tn->get_kind() == nop_stmt_K)
   {
      return true;
   }
   if(tn->get_kind() == phi_stmt_K)
   {
      return true;
   }
   THROW_ERROR("Unexpected operation in AllocationInformation::is_operation_bounded: " + tn->get_kind_text());
   return false;
}

bool AllocationInformation::is_dual_port_memory(unsigned int fu_type) const
{
   const auto current_fu = get_fu(fu_type);
   const auto& memory_type = GetPointerS<functional_unit>(current_fu)->memory_type;
   const auto& memory_ctrl_type = GetPointerS<functional_unit>(current_fu)->memory_ctrl_type;
   return memory_type == MEMORY_TYPE_ASYNCHRONOUS || memory_type == MEMORY_TYPE_SYNCHRONOUS_SDS ||
          memory_ctrl_type == MEMORY_CTRL_TYPE_DPROXY || memory_ctrl_type == MEMORY_CTRL_TYPE_DPROXYN ||
          memory_ctrl_type == MEMORY_CTRL_TYPE_SPROXY || memory_ctrl_type == MEMORY_CTRL_TYPE_SPROXYN;
}

bool AllocationInformation::is_direct_access_memory_unit(unsigned int fu_type) const
{
   const auto current_fu = get_fu(fu_type);
   const auto& memory_type = GetPointerS<functional_unit>(current_fu)->memory_type;
   const auto& memory_ctrl_type = GetPointerS<functional_unit>(current_fu)->memory_ctrl_type;
   return memory_type != "" || memory_ctrl_type == MEMORY_CTRL_TYPE_PROXY ||
          memory_ctrl_type == MEMORY_CTRL_TYPE_PROXYN || memory_ctrl_type == MEMORY_CTRL_TYPE_DPROXY ||
          memory_ctrl_type == MEMORY_CTRL_TYPE_DPROXYN || memory_ctrl_type == MEMORY_CTRL_TYPE_SPROXY ||
          memory_ctrl_type == MEMORY_CTRL_TYPE_SPROXYN;
}

bool AllocationInformation::is_direct_proxy_memory_unit(unsigned int fu_type) const
{
   const auto current_fu = get_fu(fu_type);
   const auto& memory_ctrl_type = GetPointerS<functional_unit>(current_fu)->memory_ctrl_type;
   return memory_ctrl_type == MEMORY_CTRL_TYPE_PROXY || memory_ctrl_type == MEMORY_CTRL_TYPE_PROXYN ||
          memory_ctrl_type == MEMORY_CTRL_TYPE_DPROXY || memory_ctrl_type == MEMORY_CTRL_TYPE_DPROXYN ||
          memory_ctrl_type == MEMORY_CTRL_TYPE_SPROXY || memory_ctrl_type == MEMORY_CTRL_TYPE_SPROXYN;
}

bool AllocationInformation::is_memory_unit(const unsigned int fu_name) const
{
   THROW_ASSERT(fu_name < get_number_fu_types(), "functional unit id not meaningful");
   return memory_units.find(fu_name) != memory_units.end();
}

bool AllocationInformation::is_proxy_unit(const unsigned int fu_name) const
{
   THROW_ASSERT(fu_name < get_number_fu_types(), "functional unit id not meaningful");
   return is_proxy_function_unit(fu_name) || is_proxy_wrapped_unit(fu_name);
}

bool AllocationInformation::is_proxy_function_unit(const unsigned int fu_name) const
{
   THROW_ASSERT(fu_name < get_number_fu_types(), "functional unit id not meaningful");
   return proxy_function_units.find(fu_name) != proxy_function_units.end();
}

bool AllocationInformation::is_proxy_wrapped_unit(const unsigned int fu_name) const
{
   THROW_ASSERT(fu_name < get_number_fu_types(), "functional unit id not meaningful");
   return proxy_wrapped_units.find(fu_name) != proxy_wrapped_units.end();
}

bool AllocationInformation::is_vertex_bounded(const unsigned int fu_name) const
{
   THROW_ASSERT(fu_name < get_number_fu_types(), "functional unit id not meaningful");
   return is_vertex_bounded_rel.find(fu_name) != is_vertex_bounded_rel.end();
}

bool AllocationInformation::is_vertex_bounded_with(OpGraph::vertex_descriptor v, unsigned int& fu_name) const
{
   return is_vertex_bounded_with(op_graph.CGetNodeInfo(v).GetNodeId(), fu_name);
}

bool AllocationInformation::is_vertex_bounded_with(const unsigned int v, unsigned int& fu_name) const
{
   if(binding.find(v) == binding.end())
   {
      return false;
   }
   else
   {
      /// If this codition is true, the operation changed type from last time it was performed allocation; we do not
      /// invalidate binding since this function is const
      if(v != ENTRY_ID && v != EXIT_ID &&
         GetPointerS<const node_stmt>(IRM->GetIRNode(v))->operation != binding.find(v)->second.first)
      {
         return false;
      }
      fu_name = binding.find(v)->second.second;
      return true;
   }
}

bool AllocationInformation::is_artificial_fu(const unsigned int fu_name) const
{
   THROW_ASSERT(fu_name < get_number_fu_types(), "functional unit id not meaningful");
   const auto& fu_string_name = list_of_FU[fu_name]->get_name();
   if(fu_string_name == ASSIGN_UNSIGNED_STD || fu_string_name == ASSIGN_SIGNED_STD ||
      fu_string_name == ASSIGN_REAL_STD || !has_to_be_synthetized(fu_name))
   {
      return true;
   }
   else
   {
      return false;
   }
}

unsigned int AllocationInformation::get_memory_var(const unsigned int fu_name) const
{
   THROW_ASSERT(is_memory_unit(fu_name), "functional unit id not meaningful");
   return memory_units.find(fu_name)->second;
}

std::map<unsigned int, unsigned int> AllocationInformation::get_memory_units() const
{
   return memory_units;
}

const std::map<unsigned int, unsigned int>& AllocationInformation::get_proxy_memory_units() const
{
   return proxy_memory_units;
}

unsigned int AllocationInformation::get_proxy_memory_var(const unsigned int fu_name) const
{
   THROW_ASSERT(proxy_memory_units.find(fu_name) != proxy_memory_units.end(), "functional unit id not meaningful");
   return proxy_memory_units.find(fu_name)->second;
}

const std::map<unsigned int, std::string>& AllocationInformation::get_proxy_function_units() const
{
   return proxy_function_units;
}

const std::map<unsigned int, std::string>& AllocationInformation::get_proxy_wrapped_units() const
{
   return proxy_wrapped_units;
}

bool AllocationInformation::has_to_be_synthetized(const unsigned int fu_name) const
{
   THROW_ASSERT(fu_name < get_number_fu_types(), "functional unit id not meaningful");
   const auto& fu_string_name = list_of_FU[fu_name]->get_name();
   if(fu_string_name == RETURN_STMT_STD || fu_string_name == ENTRY_STD || fu_string_name == EXIT_STD ||
      fu_string_name == NOP_STD || fu_string_name == PHI_STMT_STD || fu_string_name == NOP_STMT_STD)
   {
      return false;
   }
   else
   {
      return true;
   }
}

double AllocationInformation::get_stage_period(const unsigned int fu_name, OpGraph::vertex_descriptor v,
                                               const OpGraph& g) const
{
   return get_stage_period(fu_name, g.CGetNodeInfo(v).GetNodeId());
}

double AllocationInformation::get_stage_period(const unsigned int fu_name, const unsigned int v) const
{
   if(v == ENTRY_ID || v == EXIT_ID)
   {
      return 0.0;
   }
   const auto& operation_t = GetPointerS<const node_stmt>(IRM->GetIRNode(v))->operation;
   THROW_ASSERT(can_implement_set(v).find(fu_name) != can_implement_set(v).end(),
                "This function (" + get_string_name(fu_name) + ") cannot implement the operation " +
                    ir_helper::NormalizeTypename(operation_t));
   if(!has_to_be_synthetized(fu_name))
   {
      return 0.0;
   }
   technology_nodeRef node_op =
       GetPointerS<functional_unit>(list_of_FU[fu_name])->get_operation(ir_helper::NormalizeTypename(operation_t));
   THROW_ASSERT(GetPointerS<operation>(node_op)->time_m,
                "Timing information not specified for unit " + id_to_fu_names.find(fu_name)->second.first);
   /// DSP based components are underestimated when the RTL synthesis backend converts in LUTs, so we slightly increase
   /// the stage period first check for component_timing_alias
   if(GetPointerS<functional_unit>(list_of_FU[fu_name])->component_timing_alias != "")
   {
      const auto& component_name = GetPointerS<functional_unit>(list_of_FU[fu_name])->component_timing_alias;
      const auto& library = HLS_D->get_technology_manager()->get_library(component_name);
      technology_nodeRef f_unit_alias = HLS_D->get_technology_manager()->get_fu(component_name, library);
      THROW_ASSERT(f_unit_alias, "Library miss component: " + component_name);
      auto* fu_alias = GetPointerS<functional_unit>(f_unit_alias);
      technology_nodeRef op_alias_node = fu_alias->get_operation(operation_t);
      operation* op_alias = op_alias_node ? GetPointerS<operation>(op_alias_node) :
                                            GetPointerS<operation>(fu_alias->get_operations().front());
      return time_m_stage_period(op_alias);
   }
   else
   {
      THROW_ASSERT(GetPointer<operation>(node_op), "");
      return time_m_stage_period(GetPointerS<operation>(node_op));
   }
}

double AllocationInformation::estimate_mux_time(unsigned int fu_name) const
{
   auto fu_prec = get_prec(fu_name);
   fu_prec = resize_1_8_pow2(fu_prec);
   return mux_time_unit(fu_prec);
}

double AllocationInformation::estimate_muxNto1_delay(unsigned long long fu_prec, unsigned int mux_ins) const
{
   if(mux_ins < 2)
   {
      return 0;
   }
   fu_prec = resize_1_8_pow2(fu_prec);
   return computeMuxTreeLevels(mux_ins) * mux_time_unit_raw(fu_prec);
}

double AllocationInformation::estimate_muxNto1_area(unsigned long long fu_prec, unsigned int mux_ins) const
{
   if(mux_ins < 2)
   {
      return 0;
   }
   fu_prec = resize_1_8_pow2(fu_prec);
   const double ret = static_cast<double>(mux_ins - 1) * mux_area_unit_raw(fu_prec);
   THROW_ASSERT(ret != 0.0, "unexpected condition");
   return ret;
}

unsigned int AllocationInformation::get_cycles(const unsigned int fu_name, OpGraph::vertex_descriptor v,
                                               const OpGraph& g) const
{
   return get_cycles(fu_name, g.CGetNodeInfo(v).GetNodeId());
}

unsigned int AllocationInformation::get_cycles(const unsigned int fu_name, const unsigned int v) const
{
   if(v == ENTRY_ID || v == EXIT_ID)
   {
      return 0;
   }
   const auto& operation_t = GetPointerS<const node_stmt>(IRM->GetIRNode(v))->operation;
   THROW_ASSERT(can_implement_set(v).find(fu_name) != can_implement_set(v).end(),
                "This function (" + get_string_name(fu_name) + ") cannot implement the operation " +
                    ir_helper::NormalizeTypename(operation_t));
   if(!has_to_be_synthetized(fu_name))
   {
      return 0;
   }
   technology_nodeRef node_op = GetPointerS<functional_unit>(list_of_FU[fu_name])->get_operation(operation_t);
   THROW_ASSERT(GetPointer<operation>(node_op), id_to_fu_names.at(fu_name).first);
   THROW_ASSERT(GetPointerS<operation>(node_op)->time_m, "Timing information not specified for operation " +
                                                             node_op->get_name() + " on unit " +
                                                             id_to_fu_names.find(fu_name)->second.first);
   return GetPointerS<operation>(node_op)->time_m->get_cycles();
}

technology_nodeRef AllocationInformation::get_fu(unsigned int fu_name) const
{
   THROW_ASSERT(fu_name < get_number_fu_types(), "functional unit id " + STR(fu_name) + " is not meaningful");
   return list_of_FU[fu_name];
}

unsigned int AllocationInformation::get_number_channels(unsigned int fu_name) const
{
   if(nports_map.find(fu_name) == nports_map.end())
   {
      return 0;
   }
   else
   {
      return nports_map.find(fu_name)->second;
   }
}

/// ToBeCompleted
std::string AllocationInformation::get_string_name(unsigned int fu_name) const
{
   THROW_ASSERT(fu_name < get_number_fu_types(), "functional unit id not meaningful");
   return list_of_FU[fu_name]->get_name() + "_" + STR(fu_name);
}

bool AllocationInformation::can_implement(const unsigned int fu_id, OpGraph::vertex_descriptor v) const
{
   return can_implement_set(v).find(fu_id) != can_implement_set(v).end();
}

bool AllocationInformation::is_assign(const unsigned int fu_name) const
{
   THROW_ASSERT(fu_name < get_number_fu_types(), "functional unit id not meaningful");
   return list_of_FU[fu_name]->get_name() == ASSIGN_UNSIGNED_STD ||
          list_of_FU[fu_name]->get_name() == ASSIGN_SIGNED_STD || list_of_FU[fu_name]->get_name() == ASSIGN_REAL_STD;
}

bool AllocationInformation::is_return(const unsigned int fu_name) const
{
   THROW_ASSERT(fu_name < get_number_fu_types(), "functional unit id not meaningful");
   return list_of_FU[fu_name]->get_name() == RETURN_STMT_STD;
}

double AllocationInformation::get_execution_time_dsp_modified(const unsigned int fu_name,
                                                              const technology_nodeRef& node_op) const
{
   if(get_DSPs(fu_name) > 0)
   {
      THROW_ASSERT(GetPointer<operation>(node_op), "");
      return DSPs_margin * time_m_execution_time(GetPointerS<operation>(node_op));
   }
   else
   {
      return time_m_execution_time(GetPointerS<operation>(node_op));
   }
}

double AllocationInformation::get_stage_period_dsp_modified(const unsigned int fu_name,
                                                            const technology_nodeRef& node_op) const
{
   if(get_DSPs(fu_name) > 0)
   {
      return DSPs_margin_stage * time_m_stage_period(GetPointerS<operation>(node_op));
   }
   else
   {
      return time_m_stage_period(GetPointerS<operation>(node_op));
   }
}

double AllocationInformation::get_worst_stage_period(const unsigned int fu_name) const
{
   if(!has_to_be_synthetized(fu_name))
   {
      return 0.0;
   }
   const functional_unit::operation_vec node_ops = GetPointerS<functional_unit>(list_of_FU[fu_name])->get_operations();
   double max_value = 0.0;
   auto no_it_end = node_ops.end();
   for(auto no_it = node_ops.begin(); no_it != no_it_end; ++no_it)
   {
      max_value = std::max(max_value, get_stage_period_dsp_modified(fu_name, *no_it));
   }
   return max_value;
}

void AllocationInformation::set_number_channels(unsigned int fu_name, unsigned int n_ports)
{
   nports_map[fu_name] = n_ports;
}

unsigned int AllocationInformation::max_number_of_resources(OpGraph::vertex_descriptor v) const
{
   const auto node_id = op_graph.CGetNodeInfo(v).GetNodeId();
   if(node_id == ENTRY_ID)
   {
      return INFINITE_UINT;
   }
   if(node_id == EXIT_ID)
   {
      return INFINITE_UINT;
   }
   const auto operation = GetPointerS<const node_stmt>(IRM->GetIRNode(node_id))->operation;

   const CustomOrderedSet<unsigned int>& fu_set =
       node_id_to_fus.find(std::pair<unsigned int, std::string>(node_id, operation))->second;

   unsigned int tot_num_res = 0;
   const CustomOrderedSet<unsigned int>::const_iterator f_end = fu_set.end();

   for(auto f_i = fu_set.begin(); f_i != f_end; ++f_i)
   {
      auto num_res = tech_constraints[*f_i];
      THROW_ASSERT(num_res != 0, "something wrong happened");
      if(num_res == INFINITE_UINT)
      {
         return num_res;
      }
      else
      {
         tot_num_res += num_res;
      }
   }
   return tot_num_res;
}

unsigned int AllocationInformation::max_number_of_operations(unsigned int fu) const
{
   THROW_ASSERT(fu < get_number_fu_types(), "functional unit id not meaningful");
   THROW_ASSERT(fus_to_node_id.find(fu) != fus_to_node_id.end(),
                "no operation can be mapped on the given functional unit");
   return static_cast<unsigned int>(fus_to_node_id.find(fu)->second.size());
}

bool AllocationInformation::is_one_cycle_direct_access_memory_unit(unsigned int fu_type) const
{
   technology_nodeRef current_fu = get_fu(fu_type);
   return GetPointerS<functional_unit>(current_fu)->memory_type == MEMORY_TYPE_ASYNCHRONOUS ||
          GetPointerS<functional_unit>(current_fu)->memory_ctrl_type == MEMORY_CTRL_TYPE_DPROXY ||
          GetPointerS<functional_unit>(current_fu)->memory_ctrl_type == MEMORY_CTRL_TYPE_DPROXYN;
}

static const std::set<std::string> no_constant_ops = {"insertelement_node", "extractelement_node"};

void AllocationInformation::GetNodeTypePrec(OpGraph::vertex_descriptor node, const OpGraph& g,
                                            node_kind_prec_infoRef info, HLS_manager::io_binding_type& constant_id,
                                            bool is_constrained) const
{
   std::vector<HLS_manager::io_binding_type> vars_read = hls_manager->get_required_values(function_index, node);
   unsigned int first_valid_id = 0;
   unsigned int index = 0;
   constant_id = HLS_manager::io_binding_type(0, 0);
   if(vars_read.empty())
   {
      return;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Getting node type precision of " + g.CGetNodeInfo(node).vertex_name);
   std::string current_op = ir_helper::NormalizeTypename(g.CGetNodeInfo(node).GetOperation());

   bool is_a_pointer = false;
   ir_nodeConstRef type;
   bool is_second_constant = false;
   ir_nodeConstRef formal_parameter_type;
   unsigned long long max_size_in = 0;
   unsigned long long min_n_elements = 0;
   bool is_select_node_bool_test = false;
   for(auto itr = vars_read.begin(), end = vars_read.end(); itr != end; ++itr, ++index)
   {
      const auto id = std::get<0>(*itr);
      if(id && !first_valid_id)
      {
         first_valid_id = id;
      }
      if(current_op == "select_node" && id && !ir_helper::IsConstant(IRM->GetIRNode(id)))
      {
         if(ir_helper::Size(IRM->GetIRNode(id)) == 1)
         {
            is_select_node_bool_test = true;
         }
      }
      if((current_op == "select_node") && index != 0 && id)
      {
         first_valid_id = id;
      }
      if(current_op == "select_node")
      { /// no constant characterization for select_node
         is_second_constant = true;
      }
      if(id == 0 ||
         ((ir_helper::IsConstant(IRM->GetIRNode(id)) ||
           ir_helper::is_concat_or_node(IRM, g.CGetNodeInfo(node).GetNodeId())) &&
          !is_constrained && !is_second_constant && vars_read.size() != 1 && !no_constant_ops.count(current_op) &&
          (index == 1 || current_op != "lut_node" || current_op != "extract_bit_node")))
      {
         info->input_prec.push_back(0);
         info->real_input_nelem.push_back(0);
         info->base128_input_nelem.push_back(0);
         is_second_constant = true;
         constant_id = *itr;
         if(id)
         {
            const auto var_node = IRM->GetIRNode(id);
            type = ir_helper::CGetType(var_node);
            if(ir_helper::IsVectorType(type))
            {
               const auto element_type = ir_helper::CGetElements(type);
               const auto element_size = ir_helper::SizeAlloc(element_type);
               max_size_in = std::max(max_size_in, element_size);
               if(min_n_elements == 0 || ((128 / element_size) < min_n_elements))
               {
                  min_n_elements = 128 / element_size;
               }
            }
            else
            {
               max_size_in = std::max(max_size_in, ir_helper::Size(var_node));
            }
         }
      }
      else
      {
         const auto var_node = IRM->GetIRNode(id);
         type = ir_helper::CGetType(var_node);
         if(ir_helper::IsArrayType(type) || ir_helper::IsStructType(type))
         {
            info->input_prec.push_back(32);
            info->real_input_nelem.push_back(0);
            info->base128_input_nelem.push_back(0);
         }
         else
         {
            const auto& op_node = g.CGetNodeInfo(node).node;
            const auto form_par_type = ir_helper::GetFormalIth(op_node, index);
            const auto size_ir_var = ir_helper::Size(var_node);
            const auto size_form_par = form_par_type ? ir_helper::Size(form_par_type) : 0;
            const auto size_value = size_form_par ? size_form_par : size_ir_var;
            if(form_par_type && index == 0)
            {
               formal_parameter_type = form_par_type;
            }
            if(ir_helper::IsVectorType(type))
            {
               const auto element_type = ir_helper::CGetElements(type);
               const auto vector_size = ir_helper::SizeAlloc(type);
               const auto element_size = ir_helper::SizeAlloc(element_type);
               info->real_input_nelem.push_back(vector_size / element_size);
               info->base128_input_nelem.push_back(128 / element_size);
               info->input_prec.push_back(element_size);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Type is " + STR(type->index) + " " + STR(type) +
                                  " - Number of input elements (base128): " + STR(128 / element_size) +
                                  " - Number of real input elements: " + STR(vector_size / element_size) +
                                  " - Input precision: " + STR(element_size));
            }
            else
            {
               info->real_input_nelem.push_back(0);
               info->base128_input_nelem.push_back(0);
               info->input_prec.push_back(size_value);
            }
         }
      }
   }

   THROW_ASSERT(first_valid_id, "Unexpected pattern");
   if(formal_parameter_type)
   {
      type = formal_parameter_type;
      is_a_pointer = ir_helper::IsPointerType(type);
   }
   else
   {
      type = ir_helper::CGetType(IRM->GetIRNode(first_valid_id));
      is_a_pointer = ir_helper::IsPointerType(type);
   }
   if(is_a_pointer || ir_helper::IsArrayType(type) || ir_helper::IsStructType(type))
   {
      info->node_kind = "VECTOR_BOOL";
   }
   else if(ir_helper::IsSignedIntegerType(type))
   {
      info->node_kind = "INT";
   }
   else if(ir_helper::IsRealType(type))
   {
      info->node_kind = "REAL";
   }
   else if(ir_helper::IsUnsignedIntegerType(type))
   {
      info->node_kind = "UINT";
   }
   else if(ir_helper::IsBooleanType(type))
   {
      info->node_kind = "VECTOR_BOOL";
   }
   else if(ir_helper::IsVectorType(type))
   {
      const auto element_type = ir_helper::CGetElements(type);
      if(ir_helper::IsSignedIntegerType(element_type))
      {
         info->node_kind = "VECTOR_INT";
      }
      else if(ir_helper::IsUnsignedIntegerType(element_type))
      {
         info->node_kind = "VECTOR_UINT";
      }
      else if(ir_helper::IsRealType(element_type))
      {
         info->node_kind = "VECTOR_REAL";
      }
   }
   else
   {
      THROW_UNREACHABLE("not supported type: " + STR(type->index) + " - " + STR(type));
   }

   const auto max_size_in_true =
       std::max(max_size_in, *std::max_element(info->input_prec.begin(), info->input_prec.end()));
   for(const auto n_elements : info->base128_input_nelem)
   {
      if(n_elements && (min_n_elements == 0 || (n_elements < min_n_elements)))
      {
         min_n_elements = n_elements;
      }
   }
   /// Now we need to normalize the size to be compliant with the technology library assumptions
   if(is_select_node_bool_test)
   {
      info->is_single_bool_test_select_node = true;
   }

   max_size_in = resize_1_8_pow2(max_size_in_true);
   /// DSPs based components have to be managed in a different way
   if(current_op == "widen_mul_node" || current_op == "mul_node")
   {
      const auto nodeOutput_id = hls_manager->get_produced_value(function_index, node);
      const auto out_node = IRM->GetIRNode(nodeOutput_id);
      type = ir_helper::CGetType(out_node);
      if(ir_helper::IsVectorType(type))
      {
         const auto element_type = ir_helper::CGetElements(type);
         const auto element_size = ir_helper::SizeAlloc(element_type);
         const auto output_size = ir_helper::SizeAlloc(out_node);
         info->real_output_nelem = output_size / element_size;
         info->base128_output_nelem = 128 / element_size;
         info->output_prec = element_size;
         info->input_prec[0] = max_size_in;
         info->input_prec[1] = max_size_in;
      }
      else
      {
         THROW_ASSERT(info->input_prec.size() == 2, "unexpected number of inputs");
         const auto output_size_true = ir_helper::Size(out_node);
         if(output_size_true < info->input_prec[0])
         {
            info->input_prec[0] = output_size_true;
         }
         if(output_size_true < info->input_prec[1])
         {
            info->input_prec[1] = output_size_true;
         }
         if(info->input_prec[0] > info->input_prec[1])
         {
            std::swap(info->input_prec[0], info->input_prec[1]);
         }
         bool resized = false;

         const auto resized_second_index = resize_1_8_pow2(info->input_prec[1]);
         /// After first match we exit to prevent matching with larger mults
         for(size_t ind = 0; ind < DSP_y_db.size() && !resized; ind++)
         {
            const auto y_dsp_size = DSP_y_db[ind];
            const auto resized_y_dsp_size = resize_1_8_pow2(y_dsp_size);
            if(info->input_prec[1] < y_dsp_size && resized_y_dsp_size == resized_second_index)
            {
               if(info->input_prec[0] < DSP_x_db[ind])
               {
                  resized = true;
                  info->input_prec[1] = y_dsp_size;
                  info->input_prec[0] = DSP_x_db[ind];
               }
            }
         }
         if(!resized)
         {
            max_size_in = std::max(info->input_prec[0], info->input_prec[1]);
            max_size_in = resize_1_8_pow2(max_size_in);
            info->input_prec[0] = max_size_in;
            info->input_prec[1] = max_size_in;
            info->output_prec = max_size_in;
         }
         else
         {
            if(resize_1_8_pow2(output_size_true) < max_size_in)
            {
               max_size_in = resize_1_8_pow2(output_size_true);
            }
            info->output_prec = max_size_in;
         }
         if(current_op == "widen_mul_node")
         {
            info->output_prec = info->input_prec[0] + info->input_prec[1];
         }
         info->real_output_nelem = info->base128_output_nelem = 0;
      }
   }
   else if(starts_with(current_op, "itofp_node_") || starts_with(current_op, "fptoi_node_"))
   {
      /// ad hoc correction for itofp_node conversion
      if(starts_with(current_op, "itofp_node_") && max_size_in < 32)
      {
         max_size_in = 32;
      }
      auto nodeOutput_id = hls_manager->get_produced_value(function_index, node);
      if(nodeOutput_id)
      {
         const auto out_node = IRM->GetIRNode(nodeOutput_id);
         type = ir_helper::CGetType(out_node);
         if(ir_helper::IsArrayType(type) || ir_helper::IsStructType(type))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Output precision is 32");
            info->output_prec = 32;
         }
         else
         {
            info->output_prec = resize_1_8_pow2(ir_helper::Size(out_node));
            if(ir_helper::IsVectorType(type))
            {
               const auto element_type = ir_helper::CGetElements(type);
               const auto element_size = ir_helper::SizeAlloc(element_type);
               info->output_prec = ir_helper::SizeAlloc(out_node);
               info->base128_output_nelem = 128 / element_size;
               info->real_output_nelem = info->output_prec / element_size;
               info->output_prec = element_size;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Number of output elements (base128): " + STR(info->base128_output_nelem) +
                                  " - Number of real output elements: " + STR(info->real_output_nelem) +
                                  " - Output precision: " + STR(info->output_prec));
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Output is not a vector");
               info->real_output_nelem = 0;
               info->base128_output_nelem = 0;
            }
         }

         /// ad hoc correction for fptoi_node
         if(starts_with(current_op, "fptoi_node_") && info->output_prec < 32)
         {
            info->output_prec = 32;
         }
      }
   }
   else if(current_op == "add_node" || current_op == "sub_node" || current_op == "gep_node" ||
           current_op == "ternary_add_node" || current_op == "ternary_as_node" || current_op == "ternary_sa_node" ||
           current_op == "ternary_ss_node" || current_op == "neg_node" || current_op == "and_node" ||
           current_op == "or_node" || current_op == "xor_node" || current_op == "not_node" ||
           current_op == "concat_bit_node" || current_op == "select_node")
   {
      auto nodeOutput_id = hls_manager->get_produced_value(function_index, node);
      THROW_ASSERT(nodeOutput_id, "unexpected condition");
      const auto out_node = IRM->GetIRNode(nodeOutput_id);
      type = ir_helper::CGetType(out_node);
      auto out_prec = ir_helper::Size(out_node);
      if(ir_helper::IsVectorType(type))
      {
         const auto element_type = ir_helper::CGetElements(type);
         const auto element_size = ir_helper::SizeAlloc(element_type);
         out_prec = ir_helper::SizeAlloc(out_node);
         info->real_output_nelem = out_prec / element_size;
         info->base128_output_nelem = 128 / element_size;
         info->output_prec = element_size;
      }
      else
      {
         if(current_op == "add_node" || current_op == "sub_node" || current_op == "gep_node" ||
            current_op == "ternary_add_node" || current_op == "ternary_as_node" || current_op == "ternary_sa_node" ||
            current_op == "ternary_ss_node" || current_op == "neg_node")
         {
            if(out_prec == 9 || out_prec == 17 || out_prec == 33)
            {
               --out_prec;
               max_size_in = out_prec;
            }
         }
         else if(current_op == "and_node" || current_op == "or_node" || current_op == "xor_node" ||
                 current_op == "not_node" || current_op == "concat_bit_node")
         {
            /// timing does not change for these operations
            out_prec = std::min(out_prec, 64ull);
         }
         info->output_prec = resize_1_8_pow2(out_prec);
         info->real_output_nelem = 0;
         info->base128_output_nelem = 0;
      }
      if(current_op == "select_node" && max_size_in > 64)
      {
         max_size_in = 64;
      }

      if(info->output_prec >= max_size_in)
      {
         info->output_prec = max_size_in;
         info->base128_output_nelem = min_n_elements;
         info->real_output_nelem = min_n_elements;
      }
      else
      {
         max_size_in = info->output_prec;
         min_n_elements = info->base128_output_nelem;
         /// NOT really managed real_output_nelem
      }
   }
   else if(current_op == "shl_node")
   {
      auto nodeOutput_id = hls_manager->get_produced_value(function_index, node);
      THROW_ASSERT(nodeOutput_id, "unexpected condition");
      const auto out_node = IRM->GetIRNode(nodeOutput_id);
      type = ir_helper::CGetType(out_node);
      info->output_prec = resize_1_8_pow2(ir_helper::Size(out_node));
      if(ir_helper::IsVectorType(type))
      {
         const auto element_type = ir_helper::CGetElements(type);
         const auto element_size = ir_helper::SizeAlloc(element_type);
         info->output_prec = ir_helper::SizeAlloc(out_node);
         info->real_output_nelem = info->output_prec / element_size;
         info->base128_output_nelem = 128 / element_size;
         info->output_prec = element_size;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Type is " + STR(type->index) + " " + STR(type) +
                            " - Number of output elements (base128): " + STR(info->base128_output_nelem) +
                            " - Number of real output elements: " + STR(info->real_output_nelem) +
                            " - Output precision: " + STR(info->output_prec));
      }
      else
      {
         if(is_second_constant && info->output_prec > 64)
         {
            info->output_prec = 64;
            max_size_in = 64;
         }
         info->real_output_nelem = 0;
         info->base128_output_nelem = 0;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Output is not a vector");
      }
      if(info->output_prec >= max_size_in)
      {
         max_size_in = info->output_prec;
         min_n_elements = info->base128_output_nelem;
         /// NOT really managed real_output_nelem
      }
      else
      {
         info->output_prec = max_size_in;
         info->base128_output_nelem = min_n_elements;
         info->real_output_nelem = min_n_elements;
      }
   }
   else if(current_op == "shr_node")
   {
      if(max_size_in > 64)
      {
         if(!is_second_constant)
         {
            THROW_WARNING(
                "A bad estimation of the timing of the shr_node operator will happen. This may occur when a "
                "non-constant bit reference of a long ac_type is used. Unrolling such a part may fix the issue.");
         }
         max_size_in = 64;
      }
      info->output_prec = max_size_in;
      info->base128_output_nelem = min_n_elements;
      info->real_output_nelem = min_n_elements;
   }
   else if(max_size_in > 64)
   {
      max_size_in = 64;
      info->output_prec = 64;
      info->base128_output_nelem = min_n_elements;
      info->real_output_nelem = min_n_elements;
   }
   else
   {
      info->output_prec = max_size_in;
      info->base128_output_nelem = min_n_elements;
      info->real_output_nelem = min_n_elements;
   }
   size_t n_inputs = info->input_prec.size();
   if(current_op != "widen_mul_node" && current_op != "mul_node")
   {
      for(unsigned int i = 0; i < n_inputs; ++i)
      {
         if(info->input_prec[i] != 0)
         {
            info->input_prec[i] = max_size_in;
         }
      }
   }
   for(auto& n_elements : info->base128_input_nelem)
   {
      if(n_elements)
      {
         n_elements = min_n_elements;
      }
   }

   /// fix for the shufflevector_node operation
   if(current_op == "shufflevector_node")
   {
      if(info->input_prec[2] == 0)
      {
         std::swap(info->input_prec[2], info->input_prec[1]);
         std::swap(info->base128_input_nelem[2], info->base128_input_nelem[1]);
         std::swap(info->real_input_nelem[2], info->real_input_nelem[1]);
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "<--Got node type precision of " + g.CGetNodeInfo(node).vertex_name);
}

unsigned int updatecopy_HLS_constraints_functor::operator()(const unsigned int name) const
{
   return tech[name];
}

void updatecopy_HLS_constraints_functor::update(const unsigned int name, int delta)
{
   if(tech[name] == INFINITE_UINT)
   {
      return;
   }
   tech[name] = static_cast<unsigned int>(static_cast<int>(tech[name]) + delta);
}

updatecopy_HLS_constraints_functor::updatecopy_HLS_constraints_functor(
    const AllocationInformationRef allocation_information)
    : tech(allocation_information->tech_constraints)
{
}

unsigned long long AllocationInformation::get_prec(const unsigned int fu_name) const
{
   THROW_ASSERT(fu_name < get_number_fu_types(), "functional unit id not meaningful");
   THROW_ASSERT(precision_map.find(fu_name) != precision_map.end(), "missing the precision of " + STR(fu_name));
   return precision_map.find(fu_name)->second != 0 ? precision_map.find(fu_name)->second : 32;
}

double AllocationInformation::mux_time_unit(unsigned long long fu_prec) const
{
   return estimate_muxNto1_delay(fu_prec, 2);
}

double AllocationInformation::mux_time_unit_raw(unsigned long long fu_prec) const
{
   const auto vendor = hls_manager->GetParameterFromParameterOrDeviceOrDefault<std::string>("vendor", HLS_D, "");
   const auto family = hls_manager->GetParameterFromParameterOrDeviceOrDefault<std::string>("family", HLS_D, "");
   const bool is_openroad_target = vendor == "Generic" || family.find("yosysOpenROAD") != std::string::npos;
   if(!is_openroad_target)
   {
      return lut_time_unit(3) * mux_time_multiplier;
   }

   const technology_managerRef TM = HLS_D->get_technology_manager();
   technology_nodeRef f_unit_mux =
       TM->get_fu(MUX2_GATE_STD + STR("_1_") + STR(fu_prec) + "_" + STR(fu_prec) + "_" + STR(fu_prec), LIBRARY_STD_FU);
   THROW_ASSERT(f_unit_mux, "Library miss component: " + std::string(MUX2_GATE_STD) + STR("_1_") + STR(fu_prec) + "_" +
                                STR(fu_prec) + "_" + STR(fu_prec));
   auto* fu_br = GetPointerS<functional_unit>(f_unit_mux);
   auto* op_mux = GetPointerS<operation>(fu_br->get_operation(MUX2_GATE_STD));
   double mux_delay = time_m_execution_time(op_mux) - get_setup_hold_time();
   if(mux_delay <= 0.0)
   {
      mux_delay = get_setup_hold_time() / 2;
   }
   return mux_delay;
}

void AllocationInformation::print(std::ostream& os) const
{
   auto fu_end = list_of_FU.end();
   unsigned int index = 0;
   for(auto fu = list_of_FU.begin(); fu != fu_end; ++fu)
   {
      os << index << " ";
      index++;
      (*fu)->print(os);
   }
   if(!node_id_to_fus.empty())
   {
      os << "Op_name relation with functional unit name and operations.\n";
      for(const auto& node_id : node_id_to_fus)
      {
         for(const auto fu : node_id.second)
         {
            os << "  [" << STR(node_id.first.first) << ", <" << list_of_FU[fu]->get_name() << ">]" << std::endl;
         }
      }
   }
}

#ifndef NDEBUG
void AllocationInformation::print_allocated_resources() const
{
   if(debug_level >= DEBUG_LEVEL_VERBOSE)
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "\nDumping the list of all the fixed bindings FU <-> node");
      for(const auto& bind : binding)
      {
         if(bind.first == ENTRY_ID || bind.first == EXIT_ID)
         {
            continue;
         }
         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  Vertex " + STR(bind.first));
         PRINT_DBG_MEX(
             DEBUG_LEVEL_PEDANTIC, debug_level,
             "    Corresponding operation: " +
                 ir_helper::NormalizeTypename(GetPointerS<const node_stmt>(IRM->GetIRNode(bind.first))->operation) +
                 "(" + STR(bind.second.second) + ")");
         auto* fu = GetPointerS<functional_unit>(list_of_FU[bind.second.second]);
         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "    Vertex bound to: " + fu->get_name());
      }

      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Dumping the list of all the possible bindings FU <-> node");
      for(const auto& bind : node_id_to_fus)
      {
         if(bind.first.first == ENTRY_ID || bind.first.first == EXIT_ID || bind.first.first)
         {
            continue;
         }
         PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                       "  Vertex " + STR(bind.first.first) + "(" +
                           GetPointerS<const node_stmt>(IRM->GetIRNode(bind.first.first))->operation + ")");
         PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "    Operation can be implemented by the following FUs:");
         for(const auto fu_id : bind.second)
         {
            auto* fu = GetPointerS<functional_unit>(list_of_FU[fu_id]);
            PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                          "      FU name: " + fu->get_name() + "(" + STR(fu_id) + ")");
         }
      }
   }
}
#endif

technology_nodeRef AllocationInformation::get_fu(const std::string& fu_name, const HLS_managerConstRef hls_manager)
{
   const auto TM = hls_manager->get_HLS_device()->get_technology_manager();
   std::string library_name = TM->get_library(fu_name);
   if(library_name == "")
   {
      return technology_nodeRef();
   }
   return TM->get_fu(fu_name, library_name);
}

unsigned int AllocationInformation::GetCycleLatency(OpGraph::vertex_descriptor operationID) const
{
   return GetCycleLatency(op_graph.CGetNodeInfo(operationID).GetNodeId());
}

unsigned int AllocationInformation::GetCycleLatency(const unsigned int operationID) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Get cycle latency of " + ((operationID != ENTRY_ID && operationID != EXIT_ID) ?
                                                    STR(IRM->GetIRNode(operationID)) :
                                                    "Entry/Exit"));
   if(CanImplementSetNotEmpty(operationID))
   {
      const auto actual_latency = get_cycles(GetFuType(operationID), operationID);
      const auto ret_value = actual_latency != 0 ? actual_latency : 1;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Latency of allocation fu is " + STR(ret_value));
      return ret_value;
   }

   THROW_ASSERT(operationID != ENTRY_ID && operationID != EXIT_ID, "Entry or exit not allocated");
   const auto tn = IRM->GetIRNode(operationID);
   if(tn->get_kind() == assign_stmt_K)
   {
      const auto ga = GetPointerS<const assign_stmt>(tn);
      const auto right_kind = ga->op1->get_kind();
      if(right_kind == widen_mul_node_K || right_kind == mul_node_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Latency of not allocated fu is 1: possibly inaccurate");
         const auto data_bitsize = ir_helper::Size(ga->op0);
         const auto fu_prec = resize_1_8_pow2(data_bitsize);
         const auto in_prec = right_kind == mul_node_K ? fu_prec : (fu_prec / 2);
         const auto fu_name =
             ir_node::GetString(right_kind) + "_FU_" + STR(in_prec) + "_" + STR(in_prec) + "_" + STR(fu_prec) + "_0";
         const auto new_stmt_temp = HLS_D->get_technology_manager()->get_fu(fu_name, LIBRARY_STD_FU);
         THROW_ASSERT(new_stmt_temp, "Functional unit '" + fu_name + "' not found");
         const auto new_stmt_fu = GetPointerS<const functional_unit>(new_stmt_temp);
         const auto new_stmt_op_temp = new_stmt_fu->get_operation(ir_node::GetString(right_kind));
         const auto new_stmt_op = GetPointerS<operation>(new_stmt_op_temp);
         return new_stmt_op->time_m->get_cycles();
      }
      else if(right_kind == call_node_K)
      {
         return 0;
      }
      else if(right_kind == ssa_node_K || right_kind == constant_int_val_node_K || right_kind == select_node_K ||
              right_kind == nop_node_K || right_kind == addr_node_K || right_kind == lut_node_K ||
              right_kind == extract_bit_node_K || right_kind == concat_bit_node_K || right_kind == not_node_K ||
              right_kind == neg_node_K || right_kind == and_node_K || right_kind == or_node_K ||
              right_kind == xor_node_K || right_kind == shr_node_K || right_kind == shl_node_K ||
              right_kind == add_node_K || right_kind == gep_node_K || right_kind == sub_node_K ||
              right_kind == eq_node_K || right_kind == ne_node_K || right_kind == lt_node_K ||
              right_kind == le_node_K || right_kind == gt_node_K || right_kind == ge_node_K ||
              right_kind == ternary_add_node_K || right_kind == ternary_sa_node_K || right_kind == ternary_as_node_K ||
              right_kind == ternary_ss_node_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Latency of not allocated fu is 1");
         return 1;
      }
      THROW_UNREACHABLE("Unsupported right part (" + ir_node::GetString(right_kind) + ") of assign_stmt " +
                        ga->ToString());
   }
   else if(tn->get_kind() == multi_way_if_stmt_K || tn->get_kind() == phi_stmt_K || tn->get_kind() == nop_stmt_K ||
           tn->get_kind() == return_stmt_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Latency of not allocated fu is 1");
      return 1;
   }

   return 0;
}

std::pair<double, double> AllocationInformation::GetTimeLatency(OpGraph::vertex_descriptor operationID,
                                                                const unsigned int functional_unit,
                                                                const unsigned int stage) const
{
   return GetTimeLatency(op_graph.CGetNodeInfo(operationID).GetNodeId(), functional_unit, stage);
}

std::pair<double, double> AllocationInformation::GetTimeLatency(const unsigned int operation_index,
                                                                const unsigned int functional_unit_type,
                                                                const unsigned int stage) const
{
   if(operation_index == ENTRY_ID || operation_index == EXIT_ID)
   {
      return std::pair<double, double>(0.0, 0.0);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing time latency of " + STR(operation_index));

   const unsigned int time_operation_index = [&]() -> unsigned int {
      if(operation_index == ENTRY_ID || operation_index == EXIT_ID)
      {
         return operation_index;
      }
      if(CanImplementSetNotEmpty(operation_index))
      {
         return operation_index;
      }
      return operation_index;
   }();
   /// For the intermediate stage of multi-cycle the latency is the clock cycle
   const auto num_cycles = GetCycleLatency(time_operation_index);
   if(stage > 0 && stage < num_cycles - 1)
   {
      const double ret_value = HLS_C->get_clock_period_resource_fraction() * HLS_C->get_clock_period();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Time is " + STR(ret_value) + "," + STR(ret_value));
      return std::pair<double, double>(ret_value, ret_value);
   }

   if(CanImplementSetNotEmpty(time_operation_index))
   {
      unsigned int fu_type;
      if(functional_unit_type != fu_binding::UNKNOWN)
      {
         fu_type = functional_unit_type;
      }
      else
      {
         fu_type = GetFuType(time_operation_index);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Functional unit name is " + get_fu_name(fu_type).first);
      double connection_contribute = 0;
      /// The operation execution  time
      double actual_execution_time = get_execution_time(fu_type, time_operation_index);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Initial execution time " + STR(actual_execution_time));
      const auto n_ins = [&]() -> unsigned {
         unsigned res = 0;
         auto tn = IRM->GetIRNode(time_operation_index);
         if(tn->get_kind() == assign_stmt_K && GetPointerS<const assign_stmt>(tn)->op1->get_kind() == lut_node_K)
         {
            const auto le = GetPointerS<lut_node>(GetPointerS<const assign_stmt>(tn)->op1);
            if(le->op8)
            {
               res = 8;
            }
            else if(le->op7)
            {
               res = 7;
            }
            else if(le->op6)
            {
               res = 6;
            }
            else if(le->op5)
            {
               res = 5;
            }
            else if(le->op4)
            {
               res = 4;
            }
            else if(le->op3)
            {
               res = 3;
            }
            else if(le->op2)
            {
               res = 2;
            }
            else if(le->op1)
            {
               res = 1;
            }
            else
            {
               THROW_ERROR("unexpected condition");
            }
         }
         return res;
      }();
      double initial_execution_time =
          actual_execution_time -
          get_correction_time(fu_type, GetPointerS<const node_stmt>(IRM->GetIRNode(time_operation_index))->operation,
                              n_ins);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Initial corrected execution time " + STR(initial_execution_time));
      double op_execution_time = initial_execution_time;
      if(op_execution_time <= 0.0)
      {
         op_execution_time = epsilon;
      }

      /// The stage period
      double actual_stage_period;
      actual_stage_period = get_stage_period(fu_type, time_operation_index);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---actual_stage_period=" + STR(actual_stage_period));
      double initial_stage_period = 0.0;
      if(get_initiation_time(fu_type, time_operation_index) > 0)
      {
         if(actual_stage_period > HLS_C->get_clock_period_resource_fraction() * HLS_C->get_clock_period())
         {
            actual_stage_period = HLS_C->get_clock_period_resource_fraction() * HLS_C->get_clock_period();
         }
         initial_stage_period =
             actual_stage_period -
             get_correction_time(fu_type, GetPointerS<const node_stmt>(IRM->GetIRNode(time_operation_index))->operation,
                                 n_ins);
      }
      double stage_period = initial_stage_period;

      THROW_ASSERT(get_initiation_time(fu_type, time_operation_index) == 0 || stage_period > 0.0,
                   "unexpected condition: " + get_fu_name(fu_type).first + " Initiation time " +
                       STR(get_initiation_time(fu_type, time_operation_index)) + " Stage period " + STR(stage_period));

      if(stage_period > 0)
      {
         stage_period += connection_contribute;
      }
      else
      {
         op_execution_time += connection_contribute;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--Time is " + STR(op_execution_time) + "," + STR(stage_period));
      return std::make_pair(op_execution_time, stage_period);
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Not yet available: building time model");
      THROW_ASSERT(time_operation_index != ENTRY_ID && time_operation_index != EXIT_ID, "Entry or exit not allocated");
      const auto op_stmt = IRM->GetIRNode(time_operation_index);
      const auto op_stmt_kind = op_stmt->get_kind();
      if(op_stmt_kind == assign_stmt_K)
      {
         const auto ga = GetPointerS<const assign_stmt>(op_stmt);
         const auto op1_kind = ga->op1->get_kind();
         if(op1_kind == ssa_node_K || op1_kind == constant_int_val_node_K || op1_kind == nop_node_K ||
            op1_kind == addr_node_K || op1_kind == concat_bit_node_K || op1_kind == extract_bit_node_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Time is 0.0,0.0");
            return std::make_pair(0.0, 0.0);
         }
         else if((op1_kind == shr_node_K || op1_kind == shl_node_K) &&
                 GetPointerS<const binary_node>(ga->op1)->op1->get_kind() == constant_int_val_node_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Time is 0.0,0.0");
            return std::make_pair(0.0, 0.0);
         }
         else if(op1_kind == select_node_K)
         {
            THROW_ASSERT(ir_helper::Size(GetPointerS<const ternary_node>(ga->op1)->op0) == 1,
                         "Cond expr not allocated " + ga->op1->ToString());
            const auto data_bitsize = ir_helper::Size(ga->op0);
            const auto fu_prec = resize_1_8_pow2(data_bitsize);
            const auto op_execution_time = mux_time_unit_raw(fu_prec);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "<--Time is select time (precision is " + STR(fu_prec) + ") " + STR(op_execution_time) +
                               ",0.0");
            return std::make_pair(op_execution_time, 0.0);
         }

         const auto data_bitsize = ir_helper::Size(ga->op0);
         const auto fu_prec = resize_1_8_pow2(data_bitsize);
         std::string fu_name;
         if(op1_kind == widen_mul_node_K || op1_kind == mul_node_K)
         {
            const auto in_prec = op1_kind == mul_node_K ? fu_prec : (fu_prec / 2);
            fu_name =
                ir_node::GetString(op1_kind) + "_FU_" + STR(in_prec) + "_" + STR(in_prec) + "_" + STR(fu_prec) + "_0";
            const auto new_stmt_temp = HLS_D->get_technology_manager()->get_fu(fu_name, LIBRARY_STD_FU);
            THROW_ASSERT(new_stmt_temp, "Functional unit '" + fu_name + "' not found");
            const auto new_stmt_fu = GetPointerS<const functional_unit>(new_stmt_temp);
            const auto new_stmt_op_temp = new_stmt_fu->get_operation(ir_node::GetString(op1_kind));
            const auto new_stmt_op = GetPointerS<operation>(new_stmt_op_temp);
            auto op_execution_time = time_m_execution_time(new_stmt_op);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Uncorrected execution time is " + STR(op_execution_time));
            op_execution_time = op_execution_time - get_setup_hold_time();
            double actual_stage_period;
            actual_stage_period = time_m_stage_period(new_stmt_op);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---actual_stage_period=" + STR(actual_stage_period));
            double initial_stage_period = 0.0;
            if(new_stmt_op->time_m->get_initiation_time() > 0)
            {
               if(actual_stage_period > HLS_C->get_clock_period_resource_fraction() * HLS_C->get_clock_period())
               {
                  actual_stage_period = HLS_C->get_clock_period_resource_fraction() * HLS_C->get_clock_period();
               }
               initial_stage_period = actual_stage_period - get_setup_hold_time();
            }
            double stage_period = initial_stage_period;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "<--Time is " + STR(op_execution_time) + "," + STR(stage_period));
            return std::make_pair(op_execution_time, stage_period);
         }
         else if(op1_kind == lut_node_K)
         {
            fu_name = ir_node::GetString(op1_kind) + "_FU";
         }
         else if(GetPointer<const unary_node>(ga->op1))
         {
            fu_name = ir_node::GetString(op1_kind) + "_FU_" + STR(fu_prec) + "_" + STR(fu_prec);
         }
         else if(GetPointer<const binary_node>(ga->op1))
         {
            fu_name = ir_node::GetString(op1_kind) + "_FU_" + STR(fu_prec) + "_" + STR(fu_prec) + "_" + STR(fu_prec);
         }
         else if(GetPointer<const ternary_node>(ga->op1))
         {
            fu_name = ir_node::GetString(op1_kind) + "_FU_" + STR(fu_prec) + "_" + STR(fu_prec) + "_" + STR(fu_prec) +
                      "_" + STR(fu_prec);
         }
         else
         {
            THROW_UNREACHABLE("Latency of " + op_stmt->ToString() + " cannot be computed");
         }
         const auto new_stmt_temp = HLS_D->get_technology_manager()->get_fu(fu_name, LIBRARY_STD_FU);
         THROW_ASSERT(new_stmt_temp, "Functional unit '" + fu_name + "' not found");
         const auto new_stmt_fu = GetPointerS<const functional_unit>(new_stmt_temp);
         const auto new_stmt_op_temp = new_stmt_fu->get_operation(ir_node::GetString(op1_kind));
         const auto new_stmt_op = GetPointerS<operation>(new_stmt_op_temp);
         auto op_execution_time = time_m_execution_time(new_stmt_op);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Uncorrected execution time is " + STR(op_execution_time));
         op_execution_time = op_execution_time - get_setup_hold_time();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Time is " + STR(op_execution_time) + ",0.0");
         return std::make_pair(op_execution_time, 0.0);
      }
      else if(op_stmt_kind == multi_way_if_stmt_K)
      {
         auto controller_delay = estimate_controller_delay_fb();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Time is " + STR(controller_delay) + ",0.0");
         return std::make_pair(controller_delay, 0.0);
      }
      else if(op_stmt_kind == phi_stmt_K || op_stmt_kind == nop_stmt_K || op_stmt_kind == return_stmt_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Time is 0.0,0.0");
         return std::make_pair(0.0, 0.0);
      }
      THROW_UNREACHABLE("Latency of " + op_stmt->ToString() + " cannot be computed");
      return std::make_pair(0.0, 0.0);
   }
}

double AllocationInformation::GetPhiConnectionLatency(const unsigned int statement_index) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Computing phi connection delay of " + STR(statement_index));
   /// Checking for output phi
   const auto phi_in_degree = [&]() -> size_t {
      size_t ret_value = 0;
      if(statement_index == ENTRY_ID || statement_index == EXIT_ID)
      {
         return 0;
      }
      const auto tn = IRM->GetIRNode(statement_index);
      if(tn->get_kind() != assign_stmt_K)
      {
         return 0;
      }
      if(tn->get_kind() == assign_stmt_K && GetPointerS<const assign_stmt>(tn)->op0->get_kind() != ssa_node_K)
      {
         return 0;
      }
      const auto sn = GetPointerS<const ssa_node>(GetPointerS<const assign_stmt>(tn)->op0);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing uses of " + sn->ToString());
      for(const auto& use : sn->CGetUseStmts())
      {
         const auto target = use.first;
         if(target->get_kind() == phi_stmt_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Phi: " + target->ToString());
            const auto gp = GetPointerS<const phi_stmt>(target);
            CustomOrderedSet<unsigned int> phi_inputs;
            for(const auto& def_edge : gp->CGetDefEdgesList())
            {
               if(def_edge.first->index && !behavioral_helper->is_a_constant(def_edge.first->index))
               {
                  phi_inputs.insert(def_edge.first->index);
               }
            }
            auto curr_in_degree = phi_inputs.size();
            if(curr_in_degree > 4)
            {
               curr_in_degree = 4;
            }
            ret_value = std::max(ret_value, curr_in_degree);
         }
      }
      return ret_value;
   }();
   if(phi_in_degree == 0)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Delay is 0.0");
      return 0.0;
   }
   const auto statement = IRM->GetIRNode(statement_index);
   THROW_ASSERT(statement->get_kind() == assign_stmt_K, statement->ToString());
   const auto sn = GetPointerS<const assign_stmt>(statement)->op0;
   THROW_ASSERT(sn, "");
   const auto precision = resize_1_8_pow2(ir_helper::Size(sn));
   const auto mux_time = estimate_muxNto1_delay(precision, static_cast<unsigned int>(phi_in_degree));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "<--Delay (" + STR(phi_in_degree) + " with " + STR(precision) + " bits) is " + STR(mux_time));
   return mux_time;
}

unsigned int AllocationInformation::GetFuType(OpGraph::vertex_descriptor operation) const
{
   return GetFuType(op_graph.CGetNodeInfo(operation).GetNodeId());
}

unsigned int AllocationInformation::GetFuType(const unsigned int operation) const
{
   unsigned int fu_type = 0;
   if(not is_vertex_bounded_with(operation, fu_type))
   {
      const CustomOrderedSet<unsigned int>& fu_set = can_implement_set(operation);
      if(fu_set.size() > 1)
      {
         for(const auto fu : fu_set)
         {
            INDENT_OUT_MEX(0, 0, get_fu_name(fu).first);
         }
         THROW_UNREACHABLE("Multiple fus not supported: " + STR(IRM->GetIRNode(operation)));
      }
      else
      {
         return *(fu_set.begin());
      }
   }
   return fu_type;
}

double AllocationInformation::mux_area_unit_raw(unsigned long long fu_prec) const
{
   const auto vendor = hls_manager->GetParameterFromParameterOrDeviceOrDefault<std::string>("vendor", HLS_D, "");
   const auto family = hls_manager->GetParameterFromParameterOrDeviceOrDefault<std::string>("family", HLS_D, "");
   const bool is_openroad_target = vendor == "Generic" || family.find("yosysOpenROAD") != std::string::npos;
   if(!is_openroad_target)
   {
      fu_prec = resize_1_8_pow2(fu_prec);
      return static_cast<double>(fu_prec) * lut_area_unit(3);
   }

   const technology_managerRef TM = HLS_D->get_technology_manager();
   technology_nodeRef f_unit_mux =
       TM->get_fu(MUX2_GATE_STD + STR("_1_") + STR(fu_prec) + "_" + STR(fu_prec) + "_" + STR(fu_prec), LIBRARY_STD_FU);
   THROW_ASSERT(f_unit_mux, "Library miss component: " + std::string(MUX2_GATE_STD) + STR("_1_") + STR(fu_prec) + "_" +
                                STR(fu_prec) + "_" + STR(fu_prec));
   auto* fu_mux = GetPointerS<functional_unit>(f_unit_mux);
   double area = area_estimation::get_lut_equivalent_area(HLS_D, fu_mux->area_m);
   if(area > 0.0)
   {
      area -= 1.0;
   }
   return area;
}

double AllocationInformation::estimate_mux_area(unsigned int fu_name) const
{
   auto fu_prec = get_prec(fu_name);
   fu_prec = resize_1_8_pow2(fu_prec);
   return estimate_muxNto1_area(fu_prec, 2);
}

double AllocationInformation::estimate_controller_delay_fb() const
{
   return 0.5 * EstimateControllerDelay();
}

double AllocationInformation::EstimateControllerDelay() const
{
   const double states_number_normalization = parameters->IsParameter("StatesNumberNormalization") ?
                                                  parameters->GetParameter<double>("StatesNumberNormalization") :
                                                  NUM_CST_allocation_default_states_number_normalization;
   if(not parameters->getOption<bool>(OPT_estimate_logic_and_connections))
   {
      return 0.0;
   }
   size_t n_states =
       hls_manager->CGetFunctionBehavior(function_index)->GetBBGraph(FunctionBehavior::BB).num_vertices() +
       get_n_complex_operations();
   double n_states_factor = static_cast<double>(n_states) / NUM_CST_allocation_default_states_number_normalization_BB;
   if(hls->fsm_info &&
      hls->fsm_info->getNumberOfStates(hls_manager->CGetFunctionBehavior(function_index)->is_function_pipelined()))
   {
      n_states =
          hls->fsm_info->getNumberOfStates(hls_manager->CGetFunctionBehavior(function_index)->is_function_pipelined());
      if(n_states == 1)
      {
         return 0.0;
      }
      n_states_factor = static_cast<double>(n_states) / states_number_normalization;
   }
   unsigned int fu_prec = 16;
   const technology_managerRef TM = HLS_D->get_technology_manager();
   technology_nodeRef f_unit =
       TM->get_fu(MULTIPLIER_STD + std::string("_") + STR(fu_prec) + "_" + STR(fu_prec) + "_" + STR(fu_prec) + "_0",
                  LIBRARY_STD_FU);
   THROW_ASSERT(f_unit, "Library miss component: " + std::string(MULTIPLIER_STD) + std::string("_") + STR(fu_prec) +
                            "_" + STR(fu_prec) + "_" + STR(fu_prec) + "_0");
   auto* fu = GetPointerS<functional_unit>(f_unit);
   technology_nodeRef op_node = fu->get_operation("mul_node");
   auto* op = GetPointerS<operation>(op_node);
   double delay = time_m_execution_time(op);
   delay = delay * controller_delay_multiplier *
           ((1 - exp(-n_states_factor)) +
            n_states_factor / NUM_CST_allocation_default_states_number_normalization_linear_factor);
   if(delay < 1.5 * get_setup_hold_time())
   {
      delay = 1.5 * get_setup_hold_time();
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---Controller delay is " + STR(delay) + " while n_states is " + STR(n_states));
   return delay;
}

std::string AllocationInformation::get_latency_string(const std::string& lat) const
{
   if(lat == "2")
   {
      return std::string("");
   }
   else if(lat == "3")
   {
      return std::string("_3");
   }
   else if(lat == "4")
   {
      return std::string("_4");
   }
   else
   {
      THROW_ERROR("unexpected BRAM latency:" + lat);
   }
   return "";
}

#define ARRAY_CORRECTION 0
double AllocationInformation::get_correction_time(unsigned int fu, const std::string& operation_name,
                                                  unsigned int n_ins) const
{
   double res_value = get_setup_hold_time();
   technology_nodeRef current_fu = get_fu(fu);
   const auto& memory_type = GetPointerS<functional_unit>(current_fu)->memory_type;
   const auto& memory_ctrl_type = GetPointerS<functional_unit>(current_fu)->memory_ctrl_type;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Computing correction time of '" + operation_name + "'" +
                      (memory_type != "" ? "(" + memory_type + ")" : "") +
                      (memory_ctrl_type != "" ? "(" + memory_ctrl_type + ")" : ""));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Setup-Hold-time: " + STR(res_value));
   unsigned long long elmt_bitsize = 0;
   bool is_read_only_correction = false;
   bool is_proxied_correction = false;
   bool is_a_proxy = false;
   bool is_private_correction = false;
   bool is_single_variable = false;
   auto single_var_lambda = [&](unsigned var) -> bool {
      auto tnode = ir_helper::CGetType(IRM->GetIRNode(var));
      return !(ir_helper::IsArrayEquivType(tnode) || ir_helper::IsStructType(tnode));
   };

   if(memory_type == MEMORY_TYPE_SYNCHRONOUS_UNALIGNED)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Applying memory correction for MEMORY_TYPE_SYNCHRONOUS_UNALIGNED");
      unsigned var = get_memory_var(fu);
      if(!hls_manager->Rmem->is_a_proxied_variable(var))
      {
         is_proxied_correction = true;
      }
      else if(hls_manager->Rmem->is_private_memory(var))
      {
         is_private_correction = true;
      }
      if(hls_manager->Rmem->is_read_only_variable(var))
      {
         is_read_only_correction = true;
      }
      is_single_variable = single_var_lambda(var);

      elmt_bitsize = hls_manager->Rmem->get_bram_bitsize();

#if ARRAY_CORRECTION
      auto var_type = ir_helper::CGetType(IRM->GetIRNode(var));
      if(ir_helper::IsArrayEquivType(var_type))
      {
         auto dims = ir_helper::GetArrayDimensions(var_type);
         unsigned int n_not_power_of_two = 0;
         for(auto idx : dims)
            if(idx & (idx - 1))
               ++n_not_power_of_two;
         if(dims.size() > 1 && n_not_power_of_two > 0)
         {
            const technology_managerRef TM = HLS_D->get_technology_manager();
            auto bus_addr_bitsize = resize_1_8_pow2(address_bitsize);
            technology_nodeRef f_unit =
                TM->get_fu(ADDER_STD + std::string("_" + STR(bus_addr_bitsize) + "_" + STR(bus_addr_bitsize) + "_" +
                                                   STR(bus_addr_bitsize)),
                           LIBRARY_STD_FU);
            THROW_ASSERT(f_unit, "Library miss component: " + std::string(ADDER_STD) +
                                     std::string("_" + STR(bus_addr_bitsize) + "_" + STR(bus_addr_bitsize) + "_" +
                                                 STR(bus_addr_bitsize)));
            functional_unit* Fu = GetPointerS<functional_unit>(f_unit);
            technology_nodeRef op_node = Fu->get_operation("add_node");
            operation* op = GetPointerS<operation>(op_node);
            double delay = time_m_execution_time(op) - get_setup_hold_time();
            unsigned int n_levels = 0;
            for(; dims.size() >= (1u << n_levels); ++n_levels)
               ;
            res_value -= (n_levels - 1) * delay;
         }
      }
#endif
   }
   else if(memory_type == MEMORY_TYPE_ASYNCHRONOUS)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Applying memory correction for MEMORY_TYPE_ASYNCHRONOUS");
      unsigned var = get_memory_var(fu);
      if(!hls_manager->Rmem->is_a_proxied_variable(var))
      {
         is_proxied_correction = true;
      }
      if(hls_manager->Rmem->is_read_only_variable(var))
      {
         is_read_only_correction = true;
      }
      is_single_variable = single_var_lambda(var);

      const auto type_node = ir_helper::CGetType(IRM->GetIRNode(var));
      elmt_bitsize = ir_helper::AccessedMaximumBitsize(type_node, 1);
#if ARRAY_CORRECTION
      if(ir_helper::IsArrayEquivType(type_node))
      {
         const auto dims = ir_helper::GetArrayDimensions(type_node);
         unsigned int n_not_power_of_two = 0;
         for(auto idx : dims)
            if(idx & (idx - 1))
               ++n_not_power_of_two;
         if((dims.size() > 1 && n_not_power_of_two > 0))
         {
            const technology_managerRef TM = HLS_D->get_technology_manager();
            auto bus_addr_bitsize = resize_1_8_pow2(address_bitsize);
            technology_nodeRef f_unit =
                TM->get_fu(ADDER_STD + std::string("_" + STR(bus_addr_bitsize) + "_" + STR(bus_addr_bitsize) + "_" +
                                                   STR(bus_addr_bitsize)),
                           LIBRARY_STD_FU);
            THROW_ASSERT(f_unit, "Library miss component: " + std::string(ADDER_STD) +
                                     std::string("_" + STR(bus_addr_bitsize) + "_" + STR(bus_addr_bitsize) + "_" +
                                                 STR(bus_addr_bitsize)));
            functional_unit* Fu = GetPointerS<functional_unit>(f_unit);
            technology_nodeRef op_node = Fu->get_operation("add_node");
            operation* op = GetPointerS<operation>(op_node);
            double delay = time_m_execution_time(op) - get_setup_hold_time();
            unsigned int n_levels = 0;
            for(; dims.size() >= (1u << n_levels); ++n_levels)
               ;
            res_value -= (n_levels - 1) * delay;
         }
      }
#endif
   }
   else if(memory_type == MEMORY_TYPE_SYNCHRONOUS_SDS || memory_type == MEMORY_TYPE_SYNCHRONOUS_SDS1 ||
           memory_type == MEMORY_TYPE_SYNCHRONOUS_SDS_BUS || memory_type == MEMORY_TYPE_SYNCHRONOUS_SDS_BUS1)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Applying memory correction for MEMORY_TYPE_SYNCHRONOUS_SDS, MEMORY_TYPE_SYNCHRONOUS_SDS1 and "
                     "MEMORY_TYPE_SYNCHRONOUS_SDS_BUS/MEMORY_TYPE_SYNCHRONOUS_SDS_BUS1");
      unsigned var = get_memory_var(fu);
      is_single_variable = single_var_lambda(var);

      const auto type_node = ir_helper::CGetType(IRM->GetIRNode(var));
      elmt_bitsize = ir_helper::AccessedMaximumBitsize(type_node, 1);
#if ARRAY_CORRECTION
      if(ir_helper::IsArrayEquivType(type_node))
      {
         const auto dims = ir_helper::GetArrayDimensions(type_node);
         unsigned int n_not_power_of_two = 0;
         for(auto idx : dims)
            if(idx & (idx - 1))
               ++n_not_power_of_two;
         if((dims.size() > 1 && n_not_power_of_two > 0))
         {
            const technology_managerRef TM = HLS_D->get_technology_manager();
            auto bus_addr_bitsize = resize_1_8_pow2(address_bitsize);
            technology_nodeRef f_unit =
                TM->get_fu(ADDER_STD + std::string("_" + STR(bus_addr_bitsize) + "_" + STR(bus_addr_bitsize) + "_" +
                                                   STR(bus_addr_bitsize)),
                           LIBRARY_STD_FU);
            THROW_ASSERT(f_unit, "Library miss component: " + std::string(ADDER_STD) +
                                     std::string("_" + STR(bus_addr_bitsize) + "_" + STR(bus_addr_bitsize) + "_" +
                                                 STR(bus_addr_bitsize)));
            functional_unit* Fu = GetPointerS<functional_unit>(f_unit);
            technology_nodeRef op_node = Fu->get_operation("add_node");
            operation* op = GetPointerS<operation>(op_node);
            double delay = time_m_execution_time(op) - get_setup_hold_time();
            unsigned int n_levels = 0;
            for(; dims.size() >= (1u << n_levels); ++n_levels)
               ;
            res_value -= (n_levels - 1) * delay;
         }
      }
#endif
   }
   else if(memory_ctrl_type == MEMORY_CTRL_TYPE_PROXY || memory_ctrl_type == MEMORY_CTRL_TYPE_PROXYN ||
           memory_ctrl_type == MEMORY_CTRL_TYPE_DPROXY || memory_ctrl_type == MEMORY_CTRL_TYPE_DPROXYN ||
           memory_ctrl_type == MEMORY_CTRL_TYPE_SPROXY || memory_ctrl_type == MEMORY_CTRL_TYPE_SPROXYN)
   {
      is_a_proxy = true;
      unsigned var = proxy_memory_units.find(fu)->second;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Applying memory correction for PROXY for var:" + STR(var));
      if(hls_manager->Rmem->is_read_only_variable(var))
      {
         is_read_only_correction = true;
      }
      is_single_variable = single_var_lambda(var);

      auto* fu_cur = GetPointerS<functional_unit>(current_fu);
      technology_nodeRef op_cur_node = fu_cur->get_operation(operation_name);
      std::string latency_postfix =
          (memory_ctrl_type == MEMORY_CTRL_TYPE_DPROXY || memory_ctrl_type == MEMORY_CTRL_TYPE_DPROXYN) ?
              "" :
              get_latency_string(fu_cur->bram_load_latency);

      auto* op_cur = GetPointerS<operation>(op_cur_node);
      double cur_exec_time =
          op_cur->time_m->get_initiation_time() != 0u ? time_m_stage_period(op_cur) : time_m_execution_time(op_cur);
      double cur_exec_delta;
      technology_nodeRef f_unit_sds;
      if(hls_manager->Rmem->is_sds_var(var))
      {
         if(memory_ctrl_type == MEMORY_CTRL_TYPE_PROXY || memory_ctrl_type == MEMORY_CTRL_TYPE_DPROXY ||
            memory_ctrl_type == MEMORY_CTRL_TYPE_SPROXY)
         {
            if(hls_manager->Rmem->is_private_memory(var))
            {
               if(memory_ctrl_type == MEMORY_CTRL_TYPE_DPROXY)
               {
                  f_unit_sds = HLS_D->get_technology_manager()->get_fu(ARRAY_1D_STD_DISTRAM_SDS, LIBRARY_STD_FU);
               }
               else
               {
                  const auto private_sds_fu_name =
                      hls_manager->UseSinglePortSdsMemory() ? ARRAY_1D_STD_BRAM_SDS1 : ARRAY_1D_STD_BRAM_SDS;
                  f_unit_sds =
                      HLS_D->get_technology_manager()->get_fu(private_sds_fu_name + latency_postfix, LIBRARY_STD_FU);
               }
            }
            else
            {
               const auto public_sds_bus_fu_name =
                   hls_manager->UseSinglePortSdsMemory() ? ARRAY_1D_STD_BRAM_SDS_BUS1 : ARRAY_1D_STD_BRAM_SDS_BUS;
               f_unit_sds =
                   HLS_D->get_technology_manager()->get_fu(public_sds_bus_fu_name + latency_postfix, LIBRARY_STD_FU);
            }
         }
         else
         {
            if(hls_manager->Rmem->is_private_memory(var))
            {
               if(memory_ctrl_type == MEMORY_CTRL_TYPE_DPROXYN)
               {
                  f_unit_sds = HLS_D->get_technology_manager()->get_fu(ARRAY_1D_STD_DISTRAM_NN_SDS, LIBRARY_STD_FU);
               }
               else
               {
                  f_unit_sds = HLS_D->get_technology_manager()->get_fu(ARRAY_1D_STD_BRAM_NN_SDS + latency_postfix,
                                                                       LIBRARY_STD_FU);
               }
            }
            else
            {
               f_unit_sds = HLS_D->get_technology_manager()->get_fu(ARRAY_1D_STD_BRAM_NN_SDS_BUS + latency_postfix,
                                                                    LIBRARY_STD_FU);
            }
         }

         const auto type_node = ir_helper::CGetType(IRM->GetIRNode(var));
         elmt_bitsize = ir_helper::AccessedMaximumBitsize(type_node, 1);
      }
      else
      {
         f_unit_sds = HLS_D->get_technology_manager()->get_fu(ARRAY_1D_STD_BRAM_NN + latency_postfix, LIBRARY_STD_FU);
         if(hls_manager->Rmem->is_private_memory(var))
         {
            is_private_correction = true;
         }
         elmt_bitsize = hls_manager->Rmem->get_bram_bitsize();
      }
      THROW_ASSERT(f_unit_sds, "Library miss component");
      auto* fu_sds = GetPointerS<functional_unit>(f_unit_sds);
      technology_nodeRef op_sds_node = fu_sds->get_operation(operation_name);
      auto* op_sds = GetPointerS<operation>(op_sds_node);
      double cur_sds_exec_time =
          op_sds->time_m->get_initiation_time() != 0u ? time_m_stage_period(op_sds) : time_m_execution_time(op_sds);
      cur_exec_delta = cur_exec_time - cur_sds_exec_time;
      res_value = res_value + cur_exec_delta;

#if ARRAY_CORRECTION
      const auto type_node = ir_helper::CGetType(IRM->GetIRNode(var));
      if(ir_helper::IsArrayEquivType(type_node))
      {
         const auto dims = ir_helper::GetArrayDimensions(type_node);
         unsigned int n_not_power_of_two = 0;
         for(auto idx : dims)
            if(idx & (idx - 1))
               ++n_not_power_of_two;
         if(dims.size() > 1 && n_not_power_of_two > 0)
         {
            const technology_managerRef TM = HLS_D->get_technology_manager();
            auto bus_addr_bitsize = resize_1_8_pow2(address_bitsize);
            technology_nodeRef f_unit =
                TM->get_fu(ADDER_STD + std::string("_" + STR(bus_addr_bitsize) + "_" + STR(bus_addr_bitsize) + "_" +
                                                   STR(bus_addr_bitsize)),
                           LIBRARY_STD_FU);
            functional_unit* Fu = GetPointerS<functional_unit>(f_unit);
            technology_nodeRef op_node = Fu->get_operation("add_node");
            operation* op = GetPointerS<operation>(op_node);
            double delay = time_m_execution_time(op) - get_setup_hold_time();
            unsigned int n_levels = 0;
            for(; dims.size() >= (1u << n_levels); ++n_levels)
               ;
            res_value -= (n_levels - 1) * delay;
         }
      }
#endif
   }
   else if(memory_ctrl_type == MEMORY_CTRL_TYPE_D00)
   {
      elmt_bitsize = hls_manager->Rmem->get_bram_bitsize();
   }
   else if(is_single_bool_test_select_node_units(fu))
   {
      auto prec = get_prec(fu);
      auto fu_prec = resize_1_8_pow2(prec);
      if(fu_prec > 1)
      {
         const technology_managerRef TM = HLS_D->get_technology_manager();
         technology_nodeRef f_unit_ce = TM->get_fu(get_fu_name(fu).first, LIBRARY_STD_FU);
         auto* fu_ce = GetPointerS<functional_unit>(f_unit_ce);
         technology_nodeRef op_ce_node = fu_ce->get_operation("select_node");
         auto* op_ce = GetPointerS<operation>(op_ce_node);
         double setup_time = get_setup_hold_time();
         double ce_delay = time_m_execution_time(op_ce) - setup_time;
         double correction = ce_delay - mux_time_unit_raw(fu_prec);
         if(correction < 0.0)
         {
            correction = 0.0;
         }
         res_value = res_value + correction;
      }
   }
   else if(is_simple_gep_node(fu))
   {
      const technology_managerRef TM = HLS_D->get_technology_manager();
      technology_nodeRef f_unit_ce = TM->get_fu(get_fu_name(fu).first, LIBRARY_STD_FU);
      auto* fu_ce = GetPointerS<functional_unit>(f_unit_ce);
      technology_nodeRef op_ce_node = fu_ce->get_operation(operation_name);
      auto* op_ce = GetPointerS<operation>(op_ce_node);
      double setup_time = get_setup_hold_time();
      double ce_delay = time_m_execution_time(op_ce) - setup_time;
      double correction = ce_delay;
      res_value = res_value + correction;
   }
   else if(operation_name == "lut_node")
   {
      // std::cerr << "get_correction_time " << operation_name << " - " << n_ins << "\n";
      const auto max_lut_size = hls_manager->GetRequiredParameterFromParameterOrDevice<size_t>("max_lut_size", HLS_D);
      THROW_ASSERT(max_lut_size > 0, "Invalid parameter \"max_lut_size\": expected value > 0");
      const technology_managerRef TM = HLS_D->get_technology_manager();
      technology_nodeRef f_unit_lut = TM->get_fu(LUT_NODE_STD, LIBRARY_STD_FU);
      auto* fu_lut = GetPointerS<functional_unit>(f_unit_lut);
      technology_nodeRef op_lut_node = fu_lut->get_operation(operation_name);
      auto* op_lut = GetPointerS<operation>(op_lut_node);
      double setup_time = get_setup_hold_time();
      double lut_delay = time_m_execution_time(op_lut) - setup_time;
      res_value = res_value + lut_delay;
      if(n_ins > max_lut_size)
      {
         THROW_ERROR("unexpected condition");
      }
      else
      {
         auto delta_delay = (lut_delay * 1.) / static_cast<double>(max_lut_size);
         // std::cerr << "correction value = " << (max_lut_size-n_ins)*delta_delay << "\n";
         res_value = res_value - static_cast<double>(n_ins) * delta_delay;
      }
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---Correction value after first correction " + STR(res_value));
   double bus_multiplier = 0;
   if(elmt_bitsize == 128)
   {
      bus_multiplier = -1.0;
   }
   else if(elmt_bitsize == 64)
   {
      bus_multiplier = -0.5;
   }
   else if(elmt_bitsize == 32)
   {
      bus_multiplier = 0;
   }
   else if(elmt_bitsize == 16)
   {
      bus_multiplier = +0;
   }
   else if(elmt_bitsize == 8)
   {
      bus_multiplier = +0;
   }
   res_value = res_value + bus_multiplier * (get_setup_hold_time() / time_multiplier);
   if(is_read_only_correction)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Applying read only correction");
      res_value = res_value + memory_correction_coefficient * 0.5 * (get_setup_hold_time() / time_multiplier);
   }
   if(is_proxied_correction)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Applying proxy correction");
      res_value =
          res_value + memory_correction_coefficient * (estimate_mux_time(fu) / (mux_time_multiplier * time_multiplier));
   }
   if(is_private_correction)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Applying private correction");
      res_value =
          res_value + memory_correction_coefficient * (estimate_mux_time(fu) / (mux_time_multiplier * time_multiplier));
   }
   if(is_single_variable)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Applying single variable correction");
      const technology_managerRef TM = HLS_D->get_technology_manager();
      auto fname = get_fu_name(fu).first;
      technology_nodeRef f_unit_sv = TM->get_fu(fname, TM->get_library(fname));
      auto* fu_sv = GetPointerS<functional_unit>(f_unit_sv);
      technology_nodeRef op_sv_node = fu_sv->get_operation(operation_name);
      auto* op_sv = GetPointerS<operation>(op_sv_node);
      double setup_time = get_setup_hold_time();
      double cur_sv_exec_time =
          op_sv->time_m->get_initiation_time() != 0u ? time_m_stage_period(op_sv) : time_m_execution_time(op_sv);
      if(is_a_proxy || is_proxied_correction)
      {
         res_value = cur_sv_exec_time - setup_time;
      }
      else
      {
         double sv_delay = cur_sv_exec_time - 2 * setup_time;
         double correction = sv_delay;
         res_value = res_value + correction;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Correction is " + STR(res_value));
   return res_value;
}

double AllocationInformation::estimate_call_delay() const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Estimating call delay");
   double clock_budget = HLS_C->get_clock_period_resource_fraction() * HLS_C->get_clock_period();
   double scheduling_mux_margins = parameters->getOption<double>(OPT_scheduling_mux_margins) * mux_time_unit(32);
   auto dfp_P =
       parameters->isOption(OPT_disable_function_proxy) && parameters->getOption<bool>(OPT_disable_function_proxy);
   double call_delay;
   if(!dfp_P)
   {
      call_delay = clock_budget;
   }
   else
   {
      call_delay = clock_budget;
      INDENT_DBG_MEX(
          DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
          "---Minimum slack " +
              STR(minimumSlack > 0.0 && minimumSlack != std::numeric_limits<double>::max() ? minimumSlack : 0));
      call_delay -= minimumSlack > 0.0 && minimumSlack != std::numeric_limits<double>::max() ? minimumSlack : 0;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Call delay without slack " + STR(call_delay));
   if(call_delay < 0.0)
   {
      call_delay = get_setup_hold_time();
   }
   auto ctrl_delay = EstimateControllerDelay();
   if(call_delay < ctrl_delay)
   {
      call_delay = ctrl_delay;
   }
   /// Check if the operation mapped on this fu is bounded
   std::string function_name = behavioral_helper->GetFunctionName();
   auto module_name = hls->top->get_circ()->get_typeRef()->id_type;
   auto* fu = GetPointerS<functional_unit>(HLS_D->get_technology_manager()->get_fu(module_name, WORK_LIBRARY));
   auto* op = GetPointerS<operation>(fu->get_operation(function_name));
   if(not op->bounded)
   {
      /// Add delay due to multiplexer in front of the input; the multiplexer has as input the actual input used in
      /// first clock cycle and the registered input used in the following cycles
      call_delay += EstimateControllerDelay();
   }
   if(call_delay >= clock_budget - scheduling_mux_margins)
   {
      call_delay = clock_budget - scheduling_mux_margins;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Estimated call delay " + STR(call_delay));
   return call_delay;
}

double AllocationInformation::compute_normalized_area(unsigned int fu_s1) const
{
   double mux_area = estimate_mux_area(fu_s1);
   double resource_area =
       is_single_bool_test_select_node_units(fu_s1) ? (mux_area > 1 ? (mux_area - 1) : 0) : get_area(fu_s1);
   if(resource_area > mux_area && resource_area - mux_area < 4)
   {
      resource_area = mux_area;
   }
   const auto fu_name = list_of_FU[fu_s1]->get_name();
   if(parameters->IsParameter("no-share-max") && parameters->GetParameter<int>("no-share-max") &&
      (fu_name.find("max_node_FU_") != std::string::npos || fu_name.find("min_node_FU_") != std::string::npos))
   {
      resource_area = 0.0;
   }
   return (resource_area / mux_area);
}

unsigned int AllocationInformation::get_n_complex_operations() const
{
   return n_complex_operations;
}

std::string AllocationInformation::extract_bambu_provided_name(unsigned long long prec_in, unsigned long long prec_out,
                                                               const HLS_managerConstRef hls_manager,
                                                               technology_nodeRef& current_fu)
{
   std::string unit_name;
   if(prec_in == 32 && prec_out == 64)
   {
      unit_name = SF_FFDATA_CONVERTER_32_64_STD;
   }
   else if(prec_in == 64 && prec_out == 32)
   {
      unit_name = SF_FFDATA_CONVERTER_64_32_STD;
   }
   else
   {
      THROW_ERROR("not supported float to float conversion: " + STR(prec_in) + " " + STR(prec_out));
   }
   current_fu = get_fu(unit_name, hls_manager);
   return unit_name;
}

bool AllocationInformation::has_constant_in(unsigned int fu_name) const
{
   if(!has_to_be_synthetized(fu_name))
   {
      return false;
   }
   return GetPointerS<functional_unit>(list_of_FU[fu_name])->characterizing_constant_value != "";
}

bool AllocationInformation::is_proxy_memory_unit(const unsigned int fu_name) const
{
   THROW_ASSERT(fu_name < get_number_fu_types(), "functional unit id not meaningful");
   return proxy_memory_units.find(fu_name) != proxy_memory_units.end();
}

bool AllocationInformation::is_readonly_memory_unit(const unsigned int fu_name) const
{
   THROW_ASSERT(fu_name < get_number_fu_types(), "functional unit id not meaningful");
   return (is_memory_unit(fu_name) && hls_manager->Rmem->is_read_only_variable(get_memory_var(fu_name))) ||
          (is_proxy_memory_unit(fu_name) && hls_manager->Rmem->is_read_only_variable(get_proxy_memory_var(fu_name)));
}

bool AllocationInformation::is_single_bool_test_select_node_units(const unsigned int fu_name) const
{
   THROW_ASSERT(fu_name < get_number_fu_types(), "functional unit id not meaningful");
   return single_bool_test_select_node_units.find(fu_name) != single_bool_test_select_node_units.end();
}

bool AllocationInformation::is_simple_gep_node(const unsigned int fu_name) const
{
   THROW_ASSERT(fu_name < get_number_fu_types(), "functional unit id not meaningful");
   return simple_gep_node.find(fu_name) != simple_gep_node.end();
}

unsigned int AllocationInformation::get_worst_number_of_cycles(const unsigned int fu_name) const
{
   if(!has_to_be_synthetized(fu_name))
   {
      return 0;
   }
   const functional_unit::operation_vec node_ops = GetPointerS<functional_unit>(list_of_FU[fu_name])->get_operations();
   unsigned int max_value = 0;
   auto no_it_end = node_ops.end();
   for(auto no_it = node_ops.begin(); no_it != no_it_end; ++no_it)
   {
      max_value = std::max(max_value, GetPointerS<operation>(*no_it)->time_m->get_cycles());
   }
   return max_value;
}

double AllocationInformation::GetClockPeriodMargin() const
{
   auto clock_period = HLS_C->get_clock_period();
   auto clock_period_resource_fraction = HLS_C->get_clock_period_resource_fraction();
   auto scheduling_mux_margins = parameters->getOption<double>(OPT_scheduling_mux_margins) * mux_time_unit(32);
   auto setup_hold_time = get_setup_hold_time();

   return clock_period - ((clock_period * clock_period_resource_fraction) - scheduling_mux_margins - setup_hold_time);
}

double AllocationInformation::GetConnectionTime(OpGraph::vertex_descriptor first_operation,
                                                OpGraph::vertex_descriptor second_operation, bool readP) const
{
   const auto first_operation_index = op_graph.CGetNodeInfo(first_operation).GetNodeId();
   const auto second_operation_index = second_operation ? op_graph.CGetNodeInfo(second_operation).GetNodeId() : 0;
   return GetConnectionTime(first_operation_index, second_operation_index, readP);
}

double AllocationInformation::GetConnectionTime(const unsigned int first_operation, const unsigned int second_operation,
                                                bool readP) const
{
   const auto key = std::make_pair(first_operation, second_operation);
   const auto store_connection_time = [&](double value) {
      if(!readP)
      {
         connection_times[key] = value;
      }
      return value;
   };
   const auto compute_end_delay = [&]() -> double {
      if(first_operation == ENTRY_ID || first_operation == EXIT_ID)
      {
         return store_connection_time(0.0);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Get end delay of " + STR(first_operation));
      double end_delay = 0.0;
      const auto first_operation_kind = IRM->GetIRNode(first_operation)->get_kind();
      if(first_operation_kind == multi_way_if_stmt_K)
      {
         end_delay = estimate_controller_delay_fb();
      }
      else
      {
         const auto phi_delay = GetPhiConnectionLatency(first_operation);
         if(phi_delay > end_delay)
         {
            end_delay = phi_delay;
         }
         const auto to_dsp_register_delay = GetToDspRegisterDelay(first_operation);
         if(to_dsp_register_delay > end_delay)
         {
            end_delay = to_dsp_register_delay;
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--Got end delay of " + STR(first_operation) + ": " + STR(end_delay));
      return store_connection_time(end_delay);
   };

   const auto cached_connection = connection_times.find(key);
   if(cached_connection != connection_times.end())
   {
      return cached_connection->second;
   }

   if(!parameters->getOption<bool>(OPT_estimate_logic_and_connections))
   {
      return 0;
   }
   if(second_operation == 0)
   {
      return compute_end_delay();
   }
   if(is_operation_PI_registered(second_operation))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Computing overall connection time " + STR(first_operation) + "-->" + STR(second_operation) +
                         " Second operation has registered inputs");
      const auto second_operation_tn = IRM->GetIRNode(second_operation);
      const auto second_operation_name = GetPointerS<const node_stmt>(second_operation_tn)->operation;
      const auto called_function = IRM->GetFunction(second_operation_name);
      THROW_ASSERT(called_function, STR(second_operation_tn) + " has registered inputs but it is not a call");
      const auto called_hls = hls_manager->get_HLS(called_function->index);
      const auto called_sites_number = called_hls->call_sites_number;

      double mux_delay = 0.0;
      unsigned int n_levels = 0;
      for(; called_sites_number > (1u << n_levels); ++n_levels)
      {
         ;
      }
      mux_delay = (n_levels * mux_time_unit(32));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--Got connection time " + STR(first_operation) + "-->" + STR(second_operation) + ": " +
                         STR(mux_delay));
      return store_connection_time(mux_delay);
   }
   else if(first_operation != ENTRY_ID && first_operation != EXIT_ID && second_operation != ENTRY_ID &&
           second_operation != EXIT_ID &&
           (behavioral_helper->IsLut(first_operation) || behavioral_helper->IsLut(second_operation)))
   {
      return store_connection_time(0);
   }
   else
   {
      double connection_time = 0.0;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Computing overall connection time " + STR(first_operation) + "-->" + STR(second_operation));
      const bool is_load_store =
          behavioral_helper->IsLoad(second_operation) || behavioral_helper->IsStore(second_operation);
      if(is_load_store)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Computing connection time for load and store" + STR(first_operation) + "-->" +
                            STR(second_operation));
         const auto fu_type = GetFuType(second_operation);
         bool is_array = is_direct_access_memory_unit(fu_type);
         unsigned var =
             is_array ? (is_memory_unit(fu_type) ? get_memory_var(fu_type) : get_proxy_memory_var(fu_type)) : 0;
         auto nchannels = get_number_channels(fu_type);
         if(var && hls_manager->Rmem->get_maximum_references(var) > (2 * nchannels))
         {
            if(nchannels == 0)
            {
               THROW_ERROR("nchannels should be different than zero");
            }
            const auto ret = estimate_muxNto1_delay(
                get_prec(fu_type),
                static_cast<unsigned int>(hls_manager->Rmem->get_maximum_references(var)) / (2 * nchannels));
            connection_time += ret;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Computed connection time for load and store " + STR(first_operation) + "-->" +
                            STR(second_operation) + ": 0.0");
      }
      if(first_operation != ENTRY_ID)
      {
         const auto first_operation_tn = IRM->GetIRNode(first_operation);
         const bool is_first_load = behavioral_helper->IsLoad(first_operation);
         if(is_first_load)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "-->Computing connection time of load " + STR(first_operation) + "-->" +
                               STR(second_operation));
            const auto fu_type = GetFuType(first_operation);
            bool is_array = is_direct_access_memory_unit(fu_type);
            unsigned var =
                is_array ? (is_memory_unit(fu_type) ? get_memory_var(fu_type) : get_proxy_memory_var(fu_type)) : 0;
            auto nchannels = get_number_channels(fu_type);
            if(var && hls_manager->Rmem->get_maximum_loads(var) > (nchannels))
            {
               if(nchannels == 0)
               {
                  THROW_ERROR("nchannels should be different than zero");
               }
               auto ret = estimate_muxNto1_delay(get_prec(fu_type),
                                                 static_cast<unsigned int>(hls_manager->Rmem->get_maximum_loads(var)) /
                                                     (nchannels));
               if(ret > (2.5 * get_setup_hold_time()))
               {
                  ret = 2.5 * get_setup_hold_time();
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Computed connection time of load " + STR(first_operation) + "-->" +
                                  STR(second_operation) + ": " + STR(ret) + " var=" + STR(var));
               connection_time += ret;
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Computed connection time of load " + STR(first_operation) + "-->" +
                                  STR(second_operation) + ": 0.0");
            }
         }
         else if(GetPointerS<const node_stmt>(first_operation_tn)->operation != "STORE")
         {
            if(CanImplementSetNotEmpty(first_operation))
            {
               const auto fu_type = GetFuType(first_operation);
               const auto n_resources = get_number_fu(fu_type);
               if(n_resources != INFINITE_UINT)
               {
                  auto ret = estimate_muxNto1_delay(get_prec(fu_type), max_number_of_operations(fu_type) / n_resources);
                  if(ret != 0.0)
                  {
                     connection_time += ret;
                  }
               }
            }
         }
      }
      if(first_operation != ENTRY_ID && IRM->GetIRNode(first_operation)->get_kind() == assign_stmt_K)
      {
         const auto first_operation_tn = IRM->GetIRNode(first_operation);
         const auto ga = GetPointerS<const assign_stmt>(first_operation_tn);
         const auto ne = GetPointer<const nop_node>(ga->op1);
         if(ne)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing connection time due to conversion");
            const auto bool_input_ne = ir_helper::IsSignedIntegerType(ne->op);
            // cppcheck-suppress variableScope
            double fo_correction = 0.0;
            // cppcheck-suppress variableScope
            size_t fanout = 0;
            if(bool_input_ne)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Not expr with signed input in right part");
               const auto output_sn =
                   GetPointer<const ssa_node>(GetPointerS<const assign_stmt>(first_operation_tn)->op0);
               const auto input_sn = GetPointer<const ssa_node>(ne->op);
               if(output_sn && input_sn && ir_helper::Size(ga->op0) > ir_helper::Size(ne->op))
               {
                  fanout = (ir_helper::Size(ga->op0) - ir_helper::Size(ne->op) + 1) * output_sn->CGetNumberUses();
                  fo_correction = fanout_coefficient * get_setup_hold_time() * static_cast<double>(fanout);
                  if(fo_correction < connection_offset)
                  {
                     fo_correction = connection_offset;
                  }
                  else if(fo_correction > 1.1 * (connection_offset + get_setup_hold_time()))
                  {
                     fo_correction = 1.1 * (connection_offset + get_setup_hold_time());
                  }
                  connection_time += fo_correction;
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "<--Computed connection time due to conversion " + STR(first_operation) + "-->" +
                               STR(second_operation) + "(fanout " + STR(fanout) + ") : " + STR(fo_correction));
         }
      }
      if(CanImplementSetNotEmpty(first_operation) && get_DSPs(GetFuType(first_operation)) != 0.0)
      {
         connection_time += output_DSP_connection_time;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Connection time due to DSP connection " + STR(output_DSP_connection_time));
      }
      if(first_operation != ENTRY_ID && IRM->GetIRNode(first_operation)->get_kind() == assign_stmt_K)
      {
         const auto first_operation_tn = IRM->GetIRNode(first_operation);
         const auto op1_kind = GetPointerS<const assign_stmt>(first_operation_tn)->op1->get_kind();
         if(op1_kind == add_node_K || op1_kind == sub_node_K || op1_kind == ternary_add_node_K ||
            op1_kind == ternary_as_node_K || op1_kind == ternary_sa_node_K || op1_kind == ternary_ss_node_K ||
            op1_kind == eq_node_K || op1_kind == ne_node_K || op1_kind == gt_node_K || op1_kind == ge_node_K ||
            op1_kind == lt_node_K || op1_kind == le_node_K || op1_kind == gep_node_K)
         {
            const bool adding_connection = [&]() -> bool {
               const auto second_delay = GetTimeLatency(second_operation, fu_binding::UNKNOWN);
               if(second_delay.first > epsilon)
               {
                  return true;
               }
               const auto first_bb_index = GetPointerS<const assign_stmt>(IRM->GetIRNode(first_operation))->bb_index;
               const auto zero_distance_operations = GetZeroDistanceOperations(second_operation);
               for(const auto zero_distance_operation : zero_distance_operations)
               {
                  if(GetPointerS<const node_stmt>(IRM->GetIRNode(zero_distance_operation))->bb_index == first_bb_index)
                  {
                     const auto other_delay = GetTimeLatency(zero_distance_operation, fu_binding::UNKNOWN);
                     if(other_delay.first > epsilon || other_delay.second > epsilon)
                     {
                        return true;
                     }
                  }
               }
               return false;
            }();
            if(adding_connection)
            {
               connection_time += output_carry_connection_time;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Connection time due to carry connection " + STR(output_carry_connection_time));
            }
         }
      }
      if(!CanBeMerged(first_operation, second_operation))
      {
         connection_time = std::max(connection_time, connection_offset);
      }

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--Computed overall connection time " + STR(first_operation) + "-->" + STR(second_operation) +
                         ": " + STR(connection_time));
      return store_connection_time(connection_time);
   }
}

bool AllocationInformation::can_be_asynchronous_ram(ir_managerConstRef TM, unsigned int var, unsigned int threshold,
                                                    bool is_read_only_variable, unsigned channel_number)
{
   ir_nodeRef var_node = TM->GetIRNode(var);
   auto var_bitsize = ir_helper::Size(var_node);
   const auto hls_d = hls_manager->get_HLS_device();
   if(is_read_only_variable)
   {
      threshold = 32 * threshold;
   }
   else if(channel_number > 1)
   {
      threshold = hls_manager->GetParameterFromParameterOrDeviceOrDefault<unsigned int>("max_distram_nn_size", hls_d,
                                                                                        threshold);
   }
   if(var_node->get_kind() == variable_val_node_K)
   {
      const auto vd = GetPointerS<const variable_val_node>(var_node);
      const auto array_type_node = ir_helper::CGetType(var_node);
      if(GetPointer<const array_ty_node>(array_type_node))
      {
         std::vector<unsigned long long> dims;
         unsigned long long elts_size;
         ir_helper::get_array_dim_and_bitsize(TM, array_type_node->index, dims, elts_size);
         unsigned long long meaningful_bits = 0;
         if(vd->bit_values.size() != 0)
         {
            for(auto bit_el : vd->bit_values)
            {
               if(bit_el == 'U')
               {
                  ++meaningful_bits;
               }
            }
         }
         else
         {
            meaningful_bits = elts_size;
         }
         if(elts_size == 0)
         {
            THROW_ERROR("elts_size cannot be equal to zero");
         }
         if(meaningful_bits != elts_size)
         {
            auto real_bitsize = (var_bitsize / elts_size) * meaningful_bits;
            return (real_bitsize <= threshold) || (((var_bitsize / elts_size) <= 64) && channel_number == 1);
         }
         else
         {
            return (var_bitsize <= threshold) || (((var_bitsize / elts_size) <= 64) && channel_number == 1);
         }
      }
      else
      {
         return var_bitsize <= threshold;
      }
   }
   else
   {
      return var_bitsize <= threshold;
   }
}

bool AllocationInformation::IsVariableExecutionTime(const unsigned int) const
{
#if 1
   return false;
#else
   if(operation == ENTRY_ID || operation == EXIT_ID)
   {
      return false;
   }
   else if(GetPointerS<const node_stmt>(IRM->GetIRNode(operation))->operation == LOAD)
   {
      return true;
   }
   else if(CanImplementSetNotEmpty(operation))
   {
      for(const auto candidate_functional_unit : can_implement_set(operation))
      {
         if(get_DSPs(candidate_functional_unit))
         {
            return true;
         }
      }
   }
   return false;
#endif
}

unsigned int AllocationInformation::op_et_to_cycles(double et, double clock_period) const
{
   return static_cast<unsigned int>(ceil(et / clock_period));
}

bool AllocationInformation::CanBeMerged(const unsigned int first_operation, const unsigned int second_operation) const
{
   if(first_operation == ENTRY_ID || second_operation == EXIT_ID)
   {
      return true;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Checking if " + STR(IRM->GetIRNode(first_operation)) + " can be fused with " +
                      STR(IRM->GetIRNode(second_operation)));
   //   const auto first_delay = GetTimeLatency(first_operation, fu_binding::UNKNOWN);
   const auto second_delay = GetTimeLatency(second_operation, fu_binding::UNKNOWN);
   if(/*(first_delay.first <= epsilon and first_delay.second <= epsilon) || */ (second_delay.first <= epsilon &&
                                                                                second_delay.second <= epsilon))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes because one of the operations has zero delay");
      return true;
   }
   const auto ga0 = GetPointer<const assign_stmt>(IRM->GetIRNode(first_operation));
   const auto ga1 = GetPointer<const assign_stmt>(IRM->GetIRNode(second_operation));

   if(ga0 && ir_helper::Size(ga0->op0) == 1 && ga1 && ir_helper::Size(ga1->op1) == 1 &&
      (!CanImplementSetNotEmpty(second_operation) || get_DSPs(GetFuType(second_operation)) == 0.0))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes because single bit");
      return true;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No");
   return false;
}

bool AllocationInformation::CanBeChained(OpGraph::vertex_descriptor first_statement,
                                         OpGraph::vertex_descriptor second_statement) const
{
   const auto first_statement_index = op_graph.CGetNodeInfo(first_statement).GetNodeId();
   const auto second_statement_index = op_graph.CGetNodeInfo(second_statement).GetNodeId();
   const auto ret = CanBeChained(first_statement_index, second_statement_index);
   return ret;
}

bool AllocationInformation::CanBeChained(const unsigned int first_statement_index,
                                         const unsigned int second_statement_index) const
{
   if(first_statement_index == ENTRY_ID || first_statement_index == EXIT_ID || second_statement_index == ENTRY_ID ||
      second_statement_index == EXIT_ID)
   {
      return true;
   }
   const auto first_ir_node = IRM->GetIRNode(first_statement_index);
   const auto second_ir_node = IRM->GetIRNode(second_statement_index);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Checking if(" + STR(second_statement_index) + ") " + STR(second_ir_node) +
                      " can be chained with (" + STR(first_statement_index) + ") " + STR(first_ir_node));
   auto first_type = first_ir_node->get_kind();
   if(first_type == nop_stmt_K || second_ir_node->get_kind() == nop_stmt_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes because one is a nop_stmt");
      return true;
   }

   auto first_store = behavioral_helper->IsStore(first_statement_index);
   if(first_store)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because first is a store");
      return false;
   }
   if(not is_operation_bounded(first_statement_index))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because first is unbounded");
      return false;
   }
   auto second_load = behavioral_helper->IsLoad(second_statement_index);
   /// Load/Store from distributed memory cannot be chained with non-zero delay operations
   if(second_load &&
      GetTimeLatency(
          first_statement_index,
          CanImplementSetNotEmpty(first_statement_index) ? GetFuType(first_statement_index) : fu_binding::UNKNOWN, 0)
              .first > 0.001 &&
      is_one_cycle_direct_access_memory_unit(GetFuType(second_statement_index)) &&
      (!is_readonly_memory_unit(GetFuType(second_statement_index)) ||
       (!parameters->isOption(OPT_rom_duplication) || !parameters->getOption<bool>(OPT_rom_duplication))) &&
      ((hls_manager->Rmem->get_maximum_references(is_memory_unit(GetFuType(second_statement_index)) ?
                                                      get_memory_var(GetFuType(second_statement_index)) :
                                                      get_proxy_memory_var(GetFuType(second_statement_index)))) >
       get_number_channels(GetFuType(second_statement_index))))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because second is a load from distributed memory");
      return false;
   }
   auto second_store = behavioral_helper->IsStore(second_statement_index);

   if(first_store && !(!is_operation_bounded(second_statement_index)) &&
      is_operation_PI_registered(second_statement_index, GetFuType(second_statement_index)))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      return false;
   }
   /// Load and store from bus cannot be chained (if param is enabled)
   if(parameters->IsParameter("bus-no-chain") && parameters->GetParameter<int>("bus-no-chain") == 1 &&
      ((CanImplementSetNotEmpty(first_statement_index) &&
        is_indirect_access_memory_unit(GetFuType(first_statement_index))) ||
       (CanImplementSetNotEmpty(second_statement_index) &&
        is_indirect_access_memory_unit(GetFuType(second_statement_index)))))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--No because one of the operations is an access through bus");
      return false;
   }
   if(parameters->IsParameter("load-store-no-chain") && parameters->GetParameter<int>("load-store-no-chain") == 1 &&
      (behavioral_helper->IsLoad(first_statement_index) || second_load || first_store || second_store))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--No because one of the operations is a load or a store");
      return false;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
   return true;
}

void AllocationInformation::Initialize()
{
   controller_delay_multiplier =
       hls_manager->GetParameterFromParameterOrDeviceOrDefault<double>("controller_delay_multiplier", HLS_D, 1.0);
   setup_multiplier = hls_manager->GetParameterFromParameterOrDeviceOrDefault<double>("setup_multiplier", HLS_D, 1.0);
   time_multiplier = hls_manager->GetParameterFromParameterOrDeviceOrDefault<double>("time_multiplier", HLS_D, 1.0);
   mux_time_multiplier =
       hls_manager->GetParameterFromParameterOrDeviceOrDefault<double>("mux_time_multiplier", HLS_D, 1.0);
   memory_correction_coefficient =
       hls_manager->GetParameterFromParameterOrDeviceOrDefault<double>("memory_correction_coefficient", HLS_D, 0.7);

   double relative_connection_offset = 0.0;
   const bool has_relative_connection_offset = hls_manager->TryGetParameterFromParameterOrDevice<double>(
       "RelativeConnectionOffset", HLS_D, relative_connection_offset);
   double absolute_connection_offset = 0.0;
   const bool has_connection_offset =
       hls_manager->TryGetParameterFromParameterOrDevice<double>("ConnectionOffset", HLS_D, absolute_connection_offset);

   connection_offset = parameters->IsParameter("ConnectionOffset") ?
                           parameters->GetParameter<double>("ConnectionOffset") :
                       parameters->IsParameter("RelativeConnectionOffset") ?
                           parameters->GetParameter<double>("RelativeConnectionOffset") * get_setup_hold_time() :
                       has_relative_connection_offset ? relative_connection_offset * get_setup_hold_time() :
                       has_connection_offset          ? absolute_connection_offset :
                                                        NUM_CST_allocation_default_connection_offset;

   double output_dsp_connection_ratio = 0.0;
   const bool has_output_dsp_connection_ratio = hls_manager->TryGetParameterFromParameterOrDevice<double>(
       "OutputDSPConnectionRatio", HLS_D, output_dsp_connection_ratio);
   double output_carry_connection_ratio = 0.0;
   const bool has_output_carry_connection_ratio = hls_manager->TryGetParameterFromParameterOrDevice<double>(
       "OutputCarryConnectionRatio", HLS_D, output_carry_connection_ratio);

   output_DSP_connection_time =
       parameters->IsParameter("OutputDSPConnectionRatio") ?
           parameters->GetParameter<double>("OutputDSPConnectionRatio") * get_setup_hold_time() :
       has_output_dsp_connection_ratio ? output_dsp_connection_ratio * get_setup_hold_time() :
                                         NUM_CST_allocation_default_output_DSP_connection_ratio * get_setup_hold_time();
   output_carry_connection_time =
       parameters->IsParameter("OutputCarryConnectionRatio") ?
           parameters->GetParameter<double>("OutputCarryConnectionRatio") * get_setup_hold_time() :
       has_output_carry_connection_ratio ?
           output_carry_connection_ratio * get_setup_hold_time() :
           NUM_CST_allocation_default_output_carry_connection_ratio * get_setup_hold_time();
   fanout_coefficient = parameters->IsParameter("FanOutCoefficient") ?
                            parameters->GetParameter<double>("FanOutCoefficient") :
                            NUM_CST_allocation_default_fanout_coefficent;

   double device_dsps_margin = 0.0;
   const bool has_device_dsps_margin =
       hls_manager->TryGetParameterFromParameterOrDevice<double>("DSPs_margin", HLS_D, device_dsps_margin);
   double device_dsps_margin_stage = 0.0;
   const bool has_device_dsps_margin_stage =
       hls_manager->TryGetParameterFromParameterOrDevice<double>("DSPs_margin_stage", HLS_D, device_dsps_margin_stage);
   double device_dsp_allocation_coefficient = 0.0;
   const bool has_device_dsp_allocation_coefficient = hls_manager->TryGetParameterFromParameterOrDevice<double>(
       "DSP_allocation_coefficient", HLS_D, device_dsp_allocation_coefficient);

   DSPs_margin = has_device_dsps_margin && parameters->getOption<double>(OPT_DSP_margin_combinational) == 1.0 ?
                     device_dsps_margin :
                     parameters->getOption<double>(OPT_DSP_margin_combinational);
   DSPs_margin_stage = has_device_dsps_margin_stage && parameters->getOption<double>(OPT_DSP_margin_pipelined) == 1.0 ?
                           device_dsps_margin_stage :
                           parameters->getOption<double>(OPT_DSP_margin_pipelined);
   DSP_allocation_coefficient =
       has_device_dsp_allocation_coefficient && parameters->getOption<double>(OPT_DSP_allocation_coefficient) == 1.0 ?
           device_dsp_allocation_coefficient :
           parameters->getOption<double>(OPT_DSP_allocation_coefficient);
   minimumSlack = std::numeric_limits<double>::max();
   n_complex_operations = 0;
   id_to_fu_names.clear();
   is_vertex_bounded_rel.clear();
   list_of_FU.clear();
   memory_units.clear();
   nports_map.clear();
   precision_map.clear();
   proxy_function_units.clear();
   proxy_memory_units.clear();
   proxy_wrapped_units.clear();
   tech_constraints.clear();
   node_id_to_fus.clear();
   fus_to_node_id.clear();
   binding.clear();
   vars_to_memory_units.clear();
   precomputed_pipeline_unit.clear();
   single_bool_test_select_node_units.clear();
   simple_gep_node.clear();
   connection_times.clear();
   std::tie(DSP_x_db, DSP_y_db) = InitializeDSPDB(this);
}

double AllocationInformation::GetToDspRegisterDelay(const unsigned int statement_index) const
{
   if(statement_index == ENTRY_ID || statement_index == EXIT_ID)
   {
      return 0.0;
   }
   if(CanImplementSetNotEmpty(statement_index) && get_DSPs(GetFuType(statement_index)) != 0.0)
   {
      return 0.0;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking path to DSP register");
   double ret = 0.0;
   const auto zero_distance_operations = GetZeroDistanceOperations(statement_index);
   const auto statement_bb_index = GetPointerS<const node_stmt>(IRM->GetIRNode(statement_index))->bb_index;
   const auto tn = IRM->GetIRNode(statement_index);
   const bool is_carry = [&]() -> bool {
      if(tn->get_kind() != assign_stmt_K)
      {
         return false;
      }
      const auto op1_kind = GetPointerS<const assign_stmt>(tn)->op1->get_kind();
      if(op1_kind == add_node_K || op1_kind == sub_node_K || op1_kind == ternary_add_node_K ||
         op1_kind == ternary_as_node_K || op1_kind == ternary_sa_node_K || op1_kind == ternary_ss_node_K ||
         op1_kind == eq_node_K || op1_kind == ne_node_K || op1_kind == gt_node_K || op1_kind == ge_node_K ||
         op1_kind == lt_node_K || op1_kind == le_node_K || op1_kind == gep_node_K)
      {
         return true;
      }
      else
      {
         return false;
      }
   }();
   for(const auto zero_distance_operation : zero_distance_operations)
   {
      if(CanImplementSetNotEmpty(zero_distance_operation) && get_DSPs(GetFuType(zero_distance_operation)) != 0.0)
      {
         const auto zero_distance_operation_bb_index =
             GetPointerS<const node_stmt>(IRM->GetIRNode(zero_distance_operation))->bb_index;
         auto to_dsp_register_delay =
             (parameters->IsParameter("ToDSPRegisterDelay") ? parameters->GetParameter<double>("ToDSPRegisterDelay") :
                                                              0.6) *
             get_setup_hold_time();
         /// Add further delay if operations are faraway
         if(statement_bb_index != zero_distance_operation_bb_index)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---" + STR(zero_distance_operation) + " mapped on DSP on different BB");
            to_dsp_register_delay += 2 * ((parameters->IsParameter("ToDSPRegisterDelay") ?
                                               parameters->GetParameter<double>("ToDSPRegisterDelay") :
                                               0.6) *
                                          get_setup_hold_time());
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---" + STR(zero_distance_operation) + " mapped on DSP on same BB");
         }
         if(is_carry)
         {
            to_dsp_register_delay += output_carry_connection_time;
         }
         if(to_dsp_register_delay > ret)
         {
            ret = to_dsp_register_delay;
         }
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---" + STR(zero_distance_operation) + " not mapped on DSP");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked path to DSP register: " + STR(ret));
   return ret;
}

CustomSet<unsigned int> AllocationInformation::GetZeroDistanceOperations(const unsigned int statement_index) const
{
   const auto bb_version = hls_manager->CGetFunctionBehavior(function_index)->GetBBVersion();
   if(zero_distance_ops_bb_version.find(statement_index) != zero_distance_ops_bb_version.end() &&
      zero_distance_ops_bb_version.find(statement_index)->second == bb_version)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Get Zero Distance Operations of " + STR(statement_index) + " - Using cached values");
      return zero_distance_ops.find(statement_index)->second;
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Computing Zero Distance Operations of " + STR(statement_index));
      zero_distance_ops[statement_index].clear();
      zero_distance_ops_bb_version[statement_index] = bb_version;
      CustomSet<unsigned int> to_be_analyzed_ops;
      CustomSet<unsigned int> already_analyzed;
      to_be_analyzed_ops.insert(statement_index);
      while(to_be_analyzed_ops.size())
      {
         const auto current_tn_index = *(to_be_analyzed_ops.begin());
         to_be_analyzed_ops.erase(to_be_analyzed_ops.begin());
         already_analyzed.insert(current_tn_index);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Considering " + STR(IRM->GetIRNode(current_tn_index)));
         const auto current_ga = GetPointer<const assign_stmt>(IRM->GetIRNode(current_tn_index));
         if(!current_ga)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not continuing since not assign_stmt ");
            continue;
         }
         const auto current_sn = GetPointer<const ssa_node>(current_ga->op0);
         if(!current_sn)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not continuing since not ssa");
            continue;
         }
         if(current_tn_index != statement_index)
         {
            if(GetCycleLatency(statement_index) > 1)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not continuing since multi-cycle");
               continue;
            }
            if(GetTimeLatency(
                   current_tn_index,
                   CanImplementSetNotEmpty(current_tn_index) ? GetFuType(current_tn_index) : fu_binding::UNKNOWN, 0)
                   .first > 0.001)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not continuing since not zero delay");
               continue;
            }
         }
         for(const auto& use_stmt : current_sn->CGetUseStmts())
         {
            const auto use_stmt_index = use_stmt.first->index;
            if(already_analyzed.find(use_stmt_index) != already_analyzed.end())
            {
               continue;
            }
            to_be_analyzed_ops.insert(use_stmt_index);
            zero_distance_ops[statement_index].insert(use_stmt_index);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Considered " + STR(IRM->GetIRNode(current_tn_index)));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--Computed Zero Distance Operations of " + STR(statement_index));
      return zero_distance_ops[statement_index];
   }
}

void node_kind_prec_info::print(std::ostream& os) const
{
   os << "node_kind: " << node_kind << "\n";
   os << "node_kind: " << node_kind << "\n";
   for(auto el : input_prec)
   {
      os << el << " ";
   }
   os << "\n";
   for(auto el : base128_input_nelem)
   {
      os << el << " ";
   }
   os << "\n";
   for(auto el : real_input_nelem)
   {
      os << el << " ";
   }
   os << "\n";
   os << "output_prec: " << output_prec << "\n";
   os << "base128_output_nelem: " << base128_output_nelem << "\n";
   os << "real_output_nelem: " << real_output_nelem << "\n";
   os << "is_single_bool_test_select_node: " << (is_single_bool_test_select_node ? "T" : "F") << "\n";
   os << "is_simple_gep_node: " << (is_single_bool_test_select_node ? "T" : "F") << "\n";
}
