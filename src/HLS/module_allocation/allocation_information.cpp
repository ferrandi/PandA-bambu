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
 * @file allocation_information.cpp
 * @brief This package is used by all HLS packages to manage resource constraints and characteristics.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "allocation_information.hpp"

#include "Parameter.hpp"
#include "allocation.hpp"
#include "allocation_constants.hpp"
#include "area_info.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "ext_tree_node.hpp"
#include "fu_binding.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"
#include "math_function.hpp"
#include "memory.hpp"
#include "schedule.hpp"
#include "state_transition_graph_manager.hpp"
#include "string_manipulation.hpp"
#include "structural_manager.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "time_info.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "typed_node_info.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <tuple>

const std::pair<const CustomMap<unsigned long long, CustomUnorderedMapStable<unsigned int, double>>&,
                const CustomMap<unsigned long long, CustomUnorderedMapStable<unsigned int, double>>&>
AllocationInformation::InitializeMuxDB(const AllocationInformationConstRef allocation_information)
{
   static CustomMap<unsigned long long, CustomUnorderedMapStable<unsigned int, double>> mux_timing_db;
   static CustomMap<unsigned long long, CustomUnorderedMapStable<unsigned int, double>> mux_area_db;
   if(mux_timing_db.empty() || mux_area_db.empty())
   {
      // const unsigned int debug_level = 0;
      // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Initializing mux databases");
      /// initialize mux DBs
      const technology_managerRef TM = allocation_information->hls_manager->get_HLS_device()->get_technology_manager();
      technology_nodeRef f_unit_mux = TM->get_fu(MUX_N_TO_1, LIBRARY_STD_FU);
      THROW_ASSERT(f_unit_mux, "Library miss component: " MUX_N_TO_1);
      const auto fu_br = GetPointerS<functional_unit_template>(f_unit_mux);
      const auto op_mux_node = GetPointerS<functional_unit>(fu_br->FU)->get_operation(MUX_N_TO_1);
      const auto op_mux = GetPointerS<operation>(op_mux_node);
      std::string temp_portsize_parameters = op_mux->portsize_parameters;
      std::vector<unsigned int> mux_precisions;
      mux_precisions.push_back(1);
      mux_precisions.push_back(8);
      mux_precisions.push_back(16);
      mux_precisions.push_back(32);
      mux_precisions.push_back(64);
      const auto parameters_split = string_to_container<std::vector<std::string>>(temp_portsize_parameters, "|");
      THROW_ASSERT(parameters_split.size() > 0, "unexpected portsize_parameter format");
      for(auto module_prec : mux_precisions)
      {
         for(auto& el_indx : parameters_split)
         {
            const auto parameters_pairs = string_to_container<std::vector<std::string>>(el_indx, ":");
            if(parameters_pairs[0] == "*")
            {
               temp_portsize_parameters = parameters_pairs[1];
               break;
            }
            else if(static_cast<unsigned>(std::stoul(parameters_pairs[0])) == module_prec)
            {
               temp_portsize_parameters = parameters_pairs[1];
               break;
            }
         }
         THROW_ASSERT(temp_portsize_parameters != "",
                      "expected some portsize0_parameters for the the template operation");
         const auto portsize_parameters = string_to_container<std::vector<std::string>>(temp_portsize_parameters, ",");
         for(const auto& n_inputs : portsize_parameters)
         {
            const technology_nodeRef fu_cur_obj =
                allocation_information->hls_manager->get_HLS_device()->get_technology_manager()->get_fu(
                    MUX_N_TO_1 "_" + STR(module_prec) + "_" + STR(module_prec) + "_" + STR(module_prec) + "_" +
                        n_inputs,
                    LIBRARY_STD_FU);
            if(fu_cur_obj)
            {
               const auto fu_cur = GetPointerS<functional_unit>(fu_cur_obj);
               area_infoRef a_m = fu_cur->area_m;
               auto cur_area = a_m->get_resource_value(area_info::SLICE_LUTS);
               if(cur_area == 0.0)
               {
                  cur_area = a_m->get_area_value();
               }
               auto n_inputs_value = static_cast<unsigned>(std::stoul(n_inputs));
               mux_area_db[module_prec][n_inputs_value] = cur_area;
               auto* fu_cur_operation = GetPointerS<operation>(fu_cur->get_operation(MUX_N_TO_1));
               mux_timing_db[module_prec][n_inputs_value] = fu_cur_operation->time_m->get_execution_time() *
                                                            allocation_information->time_multiplier *
                                                            allocation_information->mux_time_multiplier;
            }
         }
      }
#define MAX_MUX_N_INPUTS 65
      for(auto module_prec : mux_precisions)
      {
         if(mux_area_db.find(module_prec) == mux_area_db.end())
         {
            THROW_ASSERT(mux_timing_db.find(module_prec) == mux_timing_db.end(), "unexpected condition");
            for(unsigned int n_ins = 2; n_ins <= MAX_MUX_N_INPUTS; ++n_ins)
            {
               unsigned int n_levels;
               for(n_levels = 1; n_ins > (1ULL << n_levels); ++n_levels)
               {
                  ;
               }
               mux_area_db[module_prec][n_ins] = (n_ins - 1) * allocation_information->mux_area_unit_raw(module_prec);
               mux_timing_db[module_prec][n_ins] = n_levels *
                                                   (allocation_information->mux_time_unit_raw(module_prec) +
                                                    allocation_information->get_setup_hold_time()) *
                                                   allocation_information->mux_time_multiplier;
            }
         }
         else
         {
            THROW_ASSERT(mux_timing_db.find(module_prec) != mux_timing_db.end(), "unexpected condition");
            THROW_ASSERT(mux_area_db.find(module_prec)->second.find(2) != mux_area_db.find(module_prec)->second.end(),
                         "unexpected condition");
            THROW_ASSERT(mux_timing_db.find(module_prec)->second.find(2) !=
                             mux_timing_db.find(module_prec)->second.end(),
                         "unexpected condition");
            unsigned int prev_non_null = 2;
            for(unsigned int n_ins = 3; n_ins <= MAX_MUX_N_INPUTS; ++n_ins)
            {
               if(mux_area_db.find(module_prec)->second.find(n_ins) != mux_area_db.find(module_prec)->second.end())
               {
                  if(prev_non_null + 1 != n_ins)
                  {
                     for(; prev_non_null + 1 < n_ins; ++prev_non_null)
                     {
                        mux_area_db[module_prec][prev_non_null + 1] =
                            mux_area_db.find(module_prec)->second.find(prev_non_null)->second +
                            (mux_area_db.find(module_prec)->second.find(n_ins)->second -
                             mux_area_db.find(module_prec)->second.find(prev_non_null)->second) /
                                (n_ins - prev_non_null);
                        mux_timing_db[module_prec][prev_non_null + 1] =
                            mux_timing_db.find(module_prec)->second.find(prev_non_null)->second +
                            (mux_timing_db.find(module_prec)->second.find(n_ins)->second -
                             mux_timing_db.find(module_prec)->second.find(prev_non_null)->second) /
                                (n_ins - prev_non_null);
                     }
                  }
                  prev_non_null = n_ins;
               }
            }
         }
      }
      THROW_ASSERT(mux_area_db.find(64) != mux_area_db.end(), "unexpected condition");
      THROW_ASSERT(!mux_area_db.at(64).empty(), "unexpected condition");
      THROW_ASSERT(mux_timing_db.find(64) != mux_timing_db.end(), "unexpected condition");
      THROW_ASSERT(!mux_timing_db.at(64).empty(), "unexpected condition");
      mux_area_db[128].insert(mux_area_db.at(64).begin(), mux_area_db.at(64).end());
      mux_timing_db[128].insert(mux_timing_db.at(64).begin(), mux_timing_db.at(64).end());
      // THROW_WARNING(STR(mux_timing_db.size()));
      // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Initialized mux databases");
   }
   return std::pair<const CustomMap<unsigned long long, CustomUnorderedMapStable<unsigned int, double>>&,
                    const CustomMap<unsigned long long, CustomUnorderedMapStable<unsigned int, double>>&>(mux_timing_db,
                                                                                                          mux_area_db);
}

const std::tuple<const std::vector<unsigned int>&, const std::vector<unsigned int>&>
AllocationInformation::InitializeDSPDB(const AllocationInformationConstRef allocation_information)
{
   static std::vector<unsigned int> DSP_x_db;
   static std::vector<unsigned int> DSP_y_db;
   if(!(DSP_x_db.size() || DSP_y_db.size()))
   {
      /// initialize DSP x and y db
      const auto hls_d = allocation_information->hls_manager->get_HLS_device();
      if(hls_d->has_parameter("DSPs_x_sizes"))
      {
         THROW_ASSERT(hls_d->has_parameter("DSPs_y_sizes"), "device description is not complete");
         auto DSPs_x_sizes = hls_d->get_parameter<std::string>("DSPs_x_sizes");
         auto DSPs_y_sizes = hls_d->get_parameter<std::string>("DSPs_y_sizes");
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
   return std::tuple<const std::vector<unsigned int>&, const std::vector<unsigned int>&>(DSP_x_db, DSP_y_db);
}

static const double epsilon = 0.000000001;

AllocationInformation::AllocationInformation(const HLS_managerRef _hls_manager, const unsigned int _function_index,
                                             const ParameterConstRef _parameters)
    : HLSFunctionIR(_hls_manager, _function_index, _parameters), address_bitsize(_hls_manager->Rget_address_bitsize())
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

AllocationInformation::~AllocationInformation() = default;

double AllocationInformation::time_m_execution_time(operation* op) const
{
   return op->time_m->get_execution_time() * time_multiplier;
}

double AllocationInformation::time_m_stage_period(operation* op) const
{
   return op->time_m->get_stage_period() * time_multiplier;
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

const CustomOrderedSet<unsigned int>& AllocationInformation::can_implement_set(const vertex v) const
{
   return can_implement_set(op_graph->CGetOpNodeInfo(v)->GetNodeId());
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
      return GetPointerS<const gimple_node>(TreeM->GetTreeNode(v))->operation;
   }();
   const auto vtf_it = node_id_to_fus.find(std::pair<unsigned int, std::string>(v, node_operation));
   THROW_ASSERT(vtf_it != node_id_to_fus.end(), "unmapped operation " + TreeM->GetTreeNode(v)->ToString());
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
   const auto node_operation = GetPointerS<const gimple_node>(TreeM->GetTreeNode(v))->operation;
   return node_id_to_fus.find(std::pair<unsigned int, std::string>(v, node_operation)) != node_id_to_fus.end();
}

double AllocationInformation::get_execution_time(const unsigned int fu_name, const vertex v,
                                                 const OpGraphConstRef g) const
{
   return get_execution_time(fu_name, g->CGetOpNodeInfo(v)->GetNodeId());
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
   const auto operation_name =
       tree_helper::NormalizeTypename(GetPointerS<const gimple_node>(TreeM->GetTreeNode(v))->operation);
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

double AllocationInformation::get_attribute_of_fu_per_op(const vertex v, const OpGraphConstRef g,
                                                         Allocation_MinMax allocation_min_max,
                                                         AllocationInformation::op_target target) const
{
   unsigned int fu_name;
   bool flag;
   double res = get_attribute_of_fu_per_op(v, g, allocation_min_max, target, fu_name, flag);
   THROW_ASSERT(flag, "something wrong happened");
   return res;
}

double AllocationInformation::get_attribute_of_fu_per_op(const vertex v, const OpGraphConstRef g,
                                                         Allocation_MinMax allocation_min_max,
                                                         AllocationInformation::op_target target, unsigned int& fu_name,
                                                         bool& flag, const updatecopy_HLS_constraints_functor* CF) const
{
   const unsigned int node_id = g->CGetOpNodeInfo(v)->GetNodeId();
   const auto node_operation = [&]() -> std::string {
      if(node_id == ENTRY_ID)
      {
         return "Entry";
      }
      if(node_id == EXIT_ID)
      {
         return "Exit";
      }
      return GetPointerS<const gimple_node>(TreeM->GetTreeNode(node_id))->operation;
   }();
   const CustomOrderedSet<unsigned int>& fu_set =
       node_id_to_fus.find(std::pair<unsigned int, std::string>(node_id, node_operation))->second;

   const auto op_name = tree_helper::NormalizeTypename(g->CGetOpNodeInfo(v)->GetOperation());
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
         ControlStep temp(0u);
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
            return from_strongtype_cast<double>(int_value);
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
                  temp = ControlStep(0u);
                  THROW_ERROR(std::string("Not supported AllocationInformation::op_performed"));
                  break;
            }
            if(temp != int_value)
            {
               fu_name = *f_i;
               int_value = temp;
            }
         }
         return from_strongtype_cast<double>(int_value);
      }
      case execution_time:
      {
         double temp;
         fu_name = *f_i;
         if(!has_to_be_synthetized(fu_name))
         {
            return 0.0;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Get Execution time " + GET_NAME(g, v));
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

unsigned int AllocationInformation::min_number_of_resources(const vertex v) const
{
   const auto node_id = op_graph->CGetOpNodeInfo(v)->GetNodeId();
   if(node_id == ENTRY_ID)
   {
      return INFINITE_UINT;
   }
   if(node_id == EXIT_ID)
   {
      return INFINITE_UINT;
   }
   const auto operation = GetPointerS<const gimple_node>(TreeM->GetTreeNode(node_id))->operation;

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
   auto area = a_m->get_resource_value(area_info::SLICE_LUTS);
   if(area == 0.0)
   {
      area = a_m->get_area_value();
   }
   return area;
}

double AllocationInformation::GetStatementArea(const unsigned int statement_index) const
{
   if(CanImplementSetNotEmpty(statement_index))
   {
      return get_area(GetFuType(statement_index));
   }

   const auto stmt = TreeM->GetTreeNode(statement_index);
   const auto stmt_kind = stmt->get_kind();
   if(stmt_kind == gimple_assign_K)
   {
      const auto ga = GetPointerS<const gimple_assign>(stmt);
      const auto op1_kind = ga->op1->get_kind();
      if(op1_kind == ssa_name_K || op1_kind == integer_cst_K || op1_kind == convert_expr_K || op1_kind == nop_expr_K ||
         op1_kind == bit_ior_concat_expr_K || op1_kind == extract_bit_expr_K)
      {
         return 0.0;
      }
      else if((op1_kind == rshift_expr_K || op1_kind == lshift_expr_K) &&
              GetPointerS<const binary_expr>(ga->op1)->op1->get_kind() == integer_cst_K)
      {
         return 0.0;
      }
      else if(op1_kind == cond_expr_K || op1_kind == vec_cond_expr_K)
      {
         THROW_ASSERT(tree_helper::Size(GetPointerS<const cond_expr>(ga->op1)->op0) == 1,
                      "Cond expr not allocated " + ga->op1->ToString());
         /// Computing time of cond_expr as time of cond_expr_FU - setup_time
         const auto data_bitsize = tree_helper::Size(ga->op0);
         const auto fu_prec = resize_1_8_pow2(data_bitsize);
         const auto op_area = mux_area_unit_raw(fu_prec);
         return op_area;
      }

      const auto data_bitsize = tree_helper::Size(ga->op0);
      const auto fu_prec = resize_1_8_pow2(data_bitsize);
      std::string fu_name;
      if(op1_kind == widen_mult_expr_K || op1_kind == mult_expr_K)
      {
         const auto in_prec = op1_kind == mult_expr_K ? fu_prec : (fu_prec / 2);
         fu_name =
             tree_node::GetString(op1_kind) + "_FU_" + STR(in_prec) + "_" + STR(in_prec) + "_" + STR(fu_prec) + "_0";
      }
      else if(op1_kind == lut_expr_K)
      {
         fu_name = tree_node::GetString(op1_kind) + "_FU";
      }
      else if(GetPointer<const unary_expr>(ga->op1))
      {
         fu_name = tree_node::GetString(op1_kind) + "_FU_" + STR(fu_prec) + "_" + STR(fu_prec);
      }
      else if(GetPointer<const binary_expr>(ga->op1))
      {
         fu_name = tree_node::GetString(op1_kind) + "_FU_" + STR(fu_prec) + "_" + STR(fu_prec) + "_" + STR(fu_prec);
      }
      else if(GetPointer<const ternary_expr>(ga->op1))
      {
         fu_name = tree_node::GetString(op1_kind) + "_FU_" + STR(fu_prec) + "_" + STR(fu_prec) + "_" + STR(fu_prec) +
                   "_" + STR(fu_prec);
      }
      else
      {
         THROW_UNREACHABLE("Unhandled operation (" + ga->op1->get_kind_text() + ")" + STR(stmt));
      }
      const auto new_stmt_temp = HLS_D->get_technology_manager()->get_fu(fu_name, LIBRARY_STD_FU);
      THROW_ASSERT(new_stmt_temp, "Functional unit '" + fu_name + "' not found");
      const auto new_stmt_fu = GetPointerS<const functional_unit>(new_stmt_temp);
      return new_stmt_fu->area_m->get_area_value();
   }
   else if(stmt_kind == gimple_multi_way_if_K || stmt_kind == gimple_cond_K || stmt_kind == gimple_return_K)
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
      return a_m->get_resource_value(area_info::DSP);
   }
   else
   {
      return 0;
   }
}

ControlStep AllocationInformation::get_initiation_time(const unsigned int fu_name, const vertex v) const
{
   return get_initiation_time(fu_name, op_graph->CGetOpNodeInfo(v)->GetNodeId());
}

ControlStep AllocationInformation::get_initiation_time(const unsigned int fu_name,
                                                       const unsigned int statement_index) const
{
   if(statement_index == ENTRY_ID || statement_index == EXIT_ID)
   {
      return ControlStep(0u);
   }
   const auto operation_name = GetPointerS<const gimple_node>(TreeM->GetTreeNode(statement_index))->operation;
   THROW_ASSERT(can_implement_set(statement_index).find(fu_name) != can_implement_set(statement_index).end(),
                "This function (" + get_string_name(fu_name) + ") cannot implement the operation " + operation_name);
   if(!has_to_be_synthetized(fu_name))
   {
      return ControlStep(0u);
   }
   technology_nodeRef node_op =
       GetPointerS<functional_unit>(list_of_FU[fu_name])->get_operation(tree_helper::NormalizeTypename(operation_name));
   THROW_ASSERT(GetPointerS<operation>(node_op)->time_m,
                "Timing information not specified for unit " + id_to_fu_names.find(fu_name)->second.first);
   return GetPointerS<operation>(node_op)->time_m->get_initiation_time();
}

bool AllocationInformation::is_operation_bounded(const OpGraphConstRef g, const vertex& op, unsigned int fu_type) const
{
   const technology_nodeRef node = get_fu(fu_type);
   const auto op_string = tree_helper::NormalizeTypename(g->CGetOpNodeInfo(op)->GetOperation());
   const functional_unit* fu = GetPointerS<functional_unit>(node);
   const technology_nodeRef op_node = fu->get_operation(op_string);
   THROW_ASSERT(GetPointer<operation>(op_node), "Op node is not an operation");
   return GetPointerS<operation>(op_node)->is_bounded();
}

bool AllocationInformation::is_operation_bounded(const unsigned int index, unsigned int fu_type) const
{
   const technology_nodeRef node = get_fu(fu_type);
   const auto op_string =
       tree_helper::NormalizeTypename(GetPointerS<const gimple_node>(TreeM->GetTreeNode(index))->operation);
   const functional_unit* fu = GetPointerS<functional_unit>(node);
   const technology_nodeRef op_node = fu->get_operation(op_string);
   THROW_ASSERT(op_node, get_fu_name(fu_type).first + " cannot execute " + op_string);
   THROW_ASSERT(GetPointer<operation>(op_node), "Op node is not an operation: " + op_string);
   return GetPointerS<operation>(op_node)->is_bounded();
}

bool AllocationInformation::is_operation_PI_registered(const OpGraphConstRef g, const vertex& op,
                                                       unsigned int fu_type) const
{
   const technology_nodeRef node = get_fu(fu_type);
   const auto op_string = tree_helper::NormalizeTypename(g->CGetOpNodeInfo(op)->GetOperation());
   const functional_unit* fu = GetPointerS<functional_unit>(node);
   const technology_nodeRef op_node = fu->get_operation(op_string);
   THROW_ASSERT(GetPointer<operation>(op_node), "Op node is not an operation");
   return GetPointerS<operation>(op_node)->is_primary_inputs_registered();
}

bool AllocationInformation::is_operation_PI_registered(const unsigned int index, unsigned int fu_type) const
{
   const technology_nodeRef node = get_fu(fu_type);
   const auto op_string =
       tree_helper::NormalizeTypename(GetPointerS<const gimple_node>(TreeM->GetTreeNode(index))->operation);
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
   const auto tn = TreeM->GetTreeNode(index);

   /// currently all the operations introduced after the allocation has been performed are bounded
   if(tn->get_kind() == gimple_assign_K)
   {
#if HAVE_ASSERTS
      const auto right_kind = GetPointerS<const gimple_assign>(tn)->op1->get_kind();
#endif
      /// currently all the operations introduced after the allocation has been performed are bounded
      // BEAWARE: when adding operations here, check they are correctly handled by GetTimeLatency and GetCycleLatency
      THROW_ASSERT(tree_helper::IsConstant(GetPointerS<const gimple_assign>(tn)->op1) || right_kind == ssa_name_K ||
                       right_kind == cond_expr_K || right_kind == vec_cond_expr_K || right_kind == convert_expr_K ||
                       right_kind == nop_expr_K || right_kind == bit_ior_concat_expr_K ||
                       right_kind == extract_bit_expr_K || right_kind == lut_expr_K || right_kind == truth_not_expr_K ||
                       right_kind == bit_not_expr_K || right_kind == negate_expr_K || right_kind == bit_xor_expr_K ||
                       right_kind == bit_ior_expr_K || right_kind == bit_and_expr_K || right_kind == truth_and_expr_K ||
                       right_kind == truth_or_expr_K || right_kind == truth_xor_expr_K || right_kind == lshift_expr_K ||
                       right_kind == rshift_expr_K || right_kind == widen_mult_expr_K || right_kind == mult_expr_K ||
                       right_kind == plus_expr_K || right_kind == minus_expr_K || right_kind == ternary_plus_expr_K ||
                       right_kind == eq_expr_K || right_kind == ne_expr_K || right_kind == lt_expr_K ||
                       right_kind == le_expr_K || right_kind == gt_expr_K || right_kind == ge_expr_K ||
                       right_kind == ternary_mp_expr_K || right_kind == ternary_pm_expr_K ||
                       right_kind == ternary_mm_expr_K,
                   "Unexpected right part: " + tree_node::GetString(right_kind));
      return true;
   }
   if(tn->get_kind() == gimple_nop_K)
   {
      return true;
   }
   if(tn->get_kind() == gimple_phi_K)
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
   return memory_type == "ASYNCHRONOUS" || memory_type == "SYNCHRONOUS_SDS" ||
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

bool AllocationInformation::is_vertex_bounded_with(const vertex v, unsigned int& fu_name) const
{
   return is_vertex_bounded_with(op_graph->CGetOpNodeInfo(v)->GetNodeId(), fu_name);
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
         GetPointerS<const gimple_node>(TreeM->GetTreeNode(v))->operation != binding.find(v)->second.first)
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
   if(fu_string_name == GIMPLE_RETURN_STD || fu_string_name == ENTRY_STD || fu_string_name == EXIT_STD ||
      fu_string_name == NOP_STD || fu_string_name == GIMPLE_PHI_STD || fu_string_name == GIMPLE_ASM_STD ||
      fu_string_name == GIMPLE_LABEL_STD || fu_string_name == GIMPLE_GOTO_STD || fu_string_name == GIMPLE_NOP_STD ||
      fu_string_name == GIMPLE_PRAGMA_STD)
   {
      return false;
   }
   else
   {
      return true;
   }
}

double AllocationInformation::get_stage_period(const unsigned int fu_name, const vertex v,
                                               const OpGraphConstRef g) const
{
   return get_stage_period(fu_name, g->CGetOpNodeInfo(v)->GetNodeId());
}

double AllocationInformation::get_stage_period(const unsigned int fu_name, const unsigned int v) const
{
   if(v == ENTRY_ID || v == EXIT_ID)
   {
      return 0.0;
   }
   const auto& operation_t = GetPointerS<const gimple_node>(TreeM->GetTreeNode(v))->operation;
   THROW_ASSERT(can_implement_set(v).find(fu_name) != can_implement_set(v).end(),
                "This function (" + get_string_name(fu_name) + ") cannot implement the operation " +
                    tree_helper::NormalizeTypename(operation_t));
   if(!has_to_be_synthetized(fu_name))
   {
      return 0.0;
   }
   technology_nodeRef node_op =
       GetPointerS<functional_unit>(list_of_FU[fu_name])->get_operation(tree_helper::NormalizeTypename(operation_t));
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
   if(mux_ins > MAX_MUX_N_INPUTS)
   {
      return HLS_C->get_clock_period();
   }
   if(fu_prec > 128)
   {
      fu_prec = 128;
   }
   THROW_ASSERT(mux_timing_db.find(fu_prec) != mux_timing_db.end(),
                STR(fu_prec) + " not found in mux database of " + STR(mux_timing_db.size()) + " elements");
   while(mux_timing_db.find(fu_prec)->second.find(mux_ins) == mux_timing_db.find(fu_prec)->second.end() &&
         mux_ins <= MAX_MUX_N_INPUTS)
   {
      ++mux_ins;
   }
   THROW_ASSERT(mux_timing_db.find(fu_prec)->second.find(mux_ins) != mux_timing_db.find(fu_prec)->second.end(),
                "fu_prec:" + STR(fu_prec) + " mux_ins: " + STR(mux_ins));
   double ret = mux_timing_db.at(fu_prec).at(mux_ins) - get_setup_hold_time();
   // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---delay of MUX with " + STR(mux_ins) + " inputs and with
   // " + STR(fu_prec) + " bits: " + STR(ret));
   return ret;
}

double AllocationInformation::estimate_muxNto1_area(unsigned long long fu_prec, unsigned int mux_ins) const
{
   if(mux_ins < 2)
   {
      return 0;
   }
   fu_prec = resize_1_8_pow2(fu_prec);
   if(mux_ins > MAX_MUX_N_INPUTS)
   {
      return std::numeric_limits<double>::max();
   }
   while(mux_area_db.find(fu_prec)->second.find(mux_ins) == mux_area_db.find(fu_prec)->second.end() &&
         mux_ins <= MAX_MUX_N_INPUTS)
   {
      ++mux_ins;
   }
   double ret = mux_area_db.at(fu_prec).at(mux_ins);
   // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---area of MUX with " + STR(mux_ins) + " inputs and with "
   // + STR(fu_prec) + " bits: " + STR(ret));
   THROW_ASSERT(ret != 0.0, "unexpected condition");
   return ret;
}

unsigned int AllocationInformation::get_cycles(const unsigned int fu_name, const vertex v,
                                               const OpGraphConstRef g) const
{
   return get_cycles(fu_name, g->CGetOpNodeInfo(v)->GetNodeId());
}

unsigned int AllocationInformation::get_cycles(const unsigned int fu_name, const unsigned int v) const
{
   if(v == ENTRY_ID || v == EXIT_ID)
   {
      return 0;
   }
   const auto& operation_t = GetPointerS<const gimple_node>(TreeM->GetTreeNode(v))->operation;
   THROW_ASSERT(can_implement_set(v).find(fu_name) != can_implement_set(v).end(),
                "This function (" + get_string_name(fu_name) + ") cannot implement the operation " +
                    tree_helper::NormalizeTypename(operation_t));
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

bool AllocationInformation::can_implement(const unsigned int fu_id, const vertex v) const
{
   return can_implement_set(v).find(fu_id) != can_implement_set(v).end();
}

bool AllocationInformation::is_read_cond(const unsigned int fu_name) const
{
   THROW_ASSERT(fu_name < get_number_fu_types(), "functional unit id not meaningful");
   return list_of_FU[fu_name]->get_name() == READ_COND_STD;
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
   return list_of_FU[fu_name]->get_name() == GIMPLE_RETURN_STD;
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

unsigned int AllocationInformation::max_number_of_resources(const vertex v) const
{
   const auto node_id = op_graph->CGetOpNodeInfo(v)->GetNodeId();
   if(node_id == ENTRY_ID)
   {
      return INFINITE_UINT;
   }
   if(node_id == EXIT_ID)
   {
      return INFINITE_UINT;
   }
   const auto operation = GetPointerS<const gimple_node>(TreeM->GetTreeNode(node_id))->operation;

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

void AllocationInformation::GetNodeTypePrec(const vertex node, const OpGraphConstRef g, node_kind_prec_infoRef info,
                                            HLS_manager::io_binding_type& constant_id, bool is_constrained) const
{
   std::vector<HLS_manager::io_binding_type> vars_read = hls_manager->get_required_values(function_index, node);
   unsigned int first_valid_id = 0;
   unsigned int index = 0;
   constant_id = HLS_manager::io_binding_type(0, 0);
   if(vars_read.empty())
   {
      return;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Getting node type precision of " + GET_NAME(g, node));
   std::string current_op = tree_helper::NormalizeTypename(g->CGetOpNodeInfo(node)->GetOperation());

   bool is_a_pointer = false;
   tree_nodeConstRef type;
   bool is_second_constant = false;
   tree_nodeConstRef formal_parameter_type;
   unsigned long long max_size_in = 0;
   unsigned long long min_n_elements = 0;
   bool is_cond_expr_bool_test = false;
   for(auto itr = vars_read.begin(), end = vars_read.end(); itr != end; ++itr, ++index)
   {
      const auto id = std::get<0>(*itr);
      if(id && !first_valid_id)
      {
         first_valid_id = id;
      }
      if(current_op == "cond_expr" && id && !tree_helper::IsConstant(TreeM->GetTreeNode(id)))
      {
         if(tree_helper::Size(TreeM->GetTreeNode(id)) == 1)
         {
            is_cond_expr_bool_test = true;
         }
      }
      if((current_op == "cond_expr" || current_op == "vec_cond_expr") && index != 0 && id)
      {
         first_valid_id = id;
      }
      if(current_op == "cond_expr" || current_op == "vec_cond_expr")
      { /// no constant characterization for cond expr
         is_second_constant = true;
      }
      if(id == 0 || ((tree_helper::IsConstant(TreeM->GetTreeNode(id)) ||
                      tree_helper::is_concat_bit_ior_expr(TreeM, g->CGetOpNodeInfo(node)->GetNodeId())) &&
                     !is_constrained && !is_second_constant && vars_read.size() != 1 && current_op != "mult_expr" &&
                     current_op != "widen_mult_expr" && current_op != "insertelement_expr" &&
                     current_op != "extractelement_expr" &&
                     (index == 1 || current_op != "lut_expr" || current_op != "extract_bit_expr")))
      {
         info->input_prec.push_back(0);
         info->real_input_nelem.push_back(0);
         info->base128_input_nelem.push_back(0);
         is_second_constant = true;
         constant_id = *itr;
         if(id)
         {
            const auto var_node = TreeM->GetTreeNode(id);
            type = tree_helper::CGetType(var_node);
            if(tree_helper::IsVectorType(type))
            {
               const auto element_type = tree_helper::CGetElements(type);
               const auto element_size = tree_helper::SizeAlloc(element_type);
               max_size_in = std::max(max_size_in, element_size);
               if(min_n_elements == 0 || ((128 / element_size) < min_n_elements))
               {
                  min_n_elements = 128 / element_size;
               }
            }
            else
            {
               max_size_in = std::max(max_size_in, tree_helper::Size(var_node));
            }
         }
      }
      else
      {
         const auto var_node = TreeM->GetTreeNode(id);
         type = tree_helper::CGetType(var_node);
         if(tree_helper::IsArrayType(type) || tree_helper::IsStructType(type) ||
            tree_helper::IsUnionType(type) /*|| tree_helper::IsComplexType(type)*/)
         {
            info->input_prec.push_back(32);
            info->real_input_nelem.push_back(0);
            info->base128_input_nelem.push_back(0);
         }
         else
         {
            const auto& op_node = g->CGetOpNodeInfo(node)->node;
            const auto form_par_type = tree_helper::GetFormalIth(op_node, index);
            const auto size_tree_var = tree_helper::Size(var_node);
            const auto size_form_par = form_par_type ? tree_helper::Size(form_par_type) : 0;
            const auto size_value = size_form_par ? size_form_par : size_tree_var;
            if(form_par_type && index == 0)
            {
               formal_parameter_type = form_par_type;
            }
            if(tree_helper::IsVectorType(type))
            {
               const auto element_type = tree_helper::CGetElements(type);
               const auto vector_size = tree_helper::SizeAlloc(type);
               const auto element_size = tree_helper::SizeAlloc(element_type);
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
      is_a_pointer = tree_helper::IsPointerType(type);
   }
   else
   {
      type = tree_helper::CGetType(TreeM->GetTreeNode(first_valid_id));
      is_a_pointer = tree_helper::IsPointerType(type);
   }
   if(is_a_pointer || tree_helper::IsArrayType(type) || tree_helper::IsStructType(type) ||
      tree_helper::IsUnionType(type) || tree_helper::IsComplexType(type))
   {
      info->node_kind = "VECTOR_BOOL";
   }
   else if(tree_helper::IsSignedIntegerType(type))
   {
      info->node_kind = "INT";
   }
   else if(tree_helper::IsRealType(type))
   {
      info->node_kind = "REAL";
   }
   else if(tree_helper::IsUnsignedIntegerType(type))
   {
      info->node_kind = "UINT";
   }
   else if(tree_helper::IsBooleanType(type))
   {
      info->node_kind = "VECTOR_BOOL";
   }
   else if(tree_helper::IsVectorType(type))
   {
      const auto element_type = tree_helper::CGetElements(type);
      if(tree_helper::IsSignedIntegerType(element_type))
      {
         info->node_kind = "VECTOR_INT";
      }
      else if(tree_helper::IsUnsignedIntegerType(element_type))
      {
         info->node_kind = "VECTOR_UINT";
      }
      else if(tree_helper::IsRealType(element_type))
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
   if(is_cond_expr_bool_test)
   {
      info->is_single_bool_test_cond_expr = true;
   }
   // if(tree_helper::is_simple_pointer_plus_test(TreeM, g->CGetOpNodeInfo(node)->GetNodeId()))
   // info->is_simple_pointer_plus_expr = true;
   max_size_in = resize_1_8_pow2(max_size_in_true);
   /// DSPs based components have to be managed in a different way
   if(current_op == "widen_mult_expr" || current_op == "mult_expr")
   {
      const auto nodeOutput_id = hls_manager->get_produced_value(function_index, node);
      const auto out_node = TreeM->GetTreeNode(nodeOutput_id);
      type = tree_helper::CGetType(out_node);
      if(tree_helper::IsVectorType(type))
      {
         const auto element_type = tree_helper::CGetElements(type);
         const auto element_size = tree_helper::SizeAlloc(element_type);
         const auto output_size = tree_helper::SizeAlloc(out_node);
         info->real_output_nelem = output_size / element_size;
         info->base128_output_nelem = 128 / element_size;
         info->output_prec = element_size;
         info->input_prec[0] = max_size_in;
         info->input_prec[1] = max_size_in;
      }
      else
      {
         THROW_ASSERT(info->input_prec.size() == 2, "unexpected number of inputs");
         const auto output_size_true = tree_helper::Size(out_node);
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
         if(current_op == "widen_mult_expr")
         {
            info->output_prec = info->input_prec[0] + info->input_prec[1];
         }
         info->real_output_nelem = info->base128_output_nelem = 0;
      }
   }
   else if(starts_with(current_op, "float_expr_") || starts_with(current_op, "fix_trunc_expr_") ||
           current_op == "dot_prod_expr" || current_op == "widen_sum_expr" || current_op == "widen_mult_hi_expr" ||
           current_op == "widen_mult_lo_expr" || current_op == "vec_unpack_hi_expr" ||
           current_op == "vec_unpack_lo_expr")
   {
      /// ad hoc correction for float_expr conversion
      if(starts_with(current_op, "float_expr_") && max_size_in < 32)
      {
         max_size_in = 32;
      }
      auto nodeOutput_id = hls_manager->get_produced_value(function_index, node);
      if(nodeOutput_id)
      {
         const auto out_node = TreeM->GetTreeNode(nodeOutput_id);
         type = tree_helper::CGetType(out_node);
         if(tree_helper::IsArrayType(type) || tree_helper::IsStructType(type) ||
            tree_helper::IsUnionType(type) /*|| tree_helper::IsComplexType(type)*/)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Output precision is 32");
            info->output_prec = 32;
         }
         else
         {
            info->output_prec = resize_1_8_pow2(tree_helper::Size(out_node));
            if(tree_helper::IsVectorType(type))
            {
               const auto element_type = tree_helper::CGetElements(type);
               const auto element_size = tree_helper::SizeAlloc(element_type);
               info->output_prec = tree_helper::SizeAlloc(out_node);
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

         /// ad hoc correction for fix_trunc_expr
         if(starts_with(current_op, "fix_trunc_expr_") && info->output_prec < 32)
         {
            info->output_prec = 32;
         }
      }
      if(current_op == "dot_prod_expr")
      {
         max_size_in = info->output_prec / 2;
         min_n_elements = info->base128_output_nelem * 2;
      }
   }
   else if(current_op == "plus_expr" || current_op == "minus_expr" || current_op == "pointer_plus_expr" ||
           current_op == "ternary_plus_expr" || current_op == "ternary_pm_expr" || current_op == "ternary_mp_expr" ||
           current_op == "ternary_mm_expr" || current_op == "negate_expr" || current_op == "bit_and_expr" ||
           current_op == "bit_ior_expr" || current_op == "bit_xor_expr" || current_op == "bit_not_expr" ||
           current_op == "bit_ior_concat_expr" || current_op == "cond_expr" ||
           current_op == "vec_cond_expr" /// these ops never have info->output_prec > max_size_in
   )
   {
      auto nodeOutput_id = hls_manager->get_produced_value(function_index, node);
      THROW_ASSERT(nodeOutput_id, "unexpected condition");
      const auto out_node = TreeM->GetTreeNode(nodeOutput_id);
      type = tree_helper::CGetType(out_node);
      auto out_prec = tree_helper::Size(out_node);
      if(tree_helper::IsVectorType(type))
      {
         const auto element_type = tree_helper::CGetElements(type);
         const auto element_size = tree_helper::SizeAlloc(element_type);
         out_prec = tree_helper::SizeAlloc(out_node);
         info->real_output_nelem = out_prec / element_size;
         info->base128_output_nelem = 128 / element_size;
         info->output_prec = element_size;
      }
      else
      {
         if(current_op == "plus_expr" || current_op == "minus_expr" || current_op == "pointer_plus_expr" ||
            current_op == "ternary_plus_expr" || current_op == "ternary_pm_expr" || current_op == "ternary_mp_expr" ||
            current_op == "ternary_mm_expr" || current_op == "negate_expr")
         {
            if(out_prec == 9 || out_prec == 17 || out_prec == 33)
            {
               --out_prec;
               max_size_in = out_prec;
            }
         }
         else if(current_op == "bit_and_expr" || current_op == "bit_ior_expr" || current_op == "bit_xor_expr" ||
                 current_op == "bit_not_expr" || current_op == "bit_ior_concat_expr")
         {
            /// timing does not change for these operations
            out_prec = std::min(out_prec, 64ull);
         }
         info->output_prec = resize_1_8_pow2(out_prec);
         info->real_output_nelem = 0;
         info->base128_output_nelem = 0;
      }
      if(current_op == "cond_expr" && max_size_in > 64)
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
   else if(current_op == "lshift_expr")
   {
      auto nodeOutput_id = hls_manager->get_produced_value(function_index, node);
      THROW_ASSERT(nodeOutput_id, "unexpected condition");
      const auto out_node = TreeM->GetTreeNode(nodeOutput_id);
      type = tree_helper::CGetType(out_node);
      info->output_prec = resize_1_8_pow2(tree_helper::Size(out_node));
      if(tree_helper::IsVectorType(type))
      {
         const auto element_type = tree_helper::CGetElements(type);
         const auto element_size = tree_helper::SizeAlloc(element_type);
         info->output_prec = tree_helper::SizeAlloc(out_node);
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
   else if(current_op == "rshift_expr")
   {
      if(max_size_in > 64)
      {
         if(!is_second_constant)
         {
            THROW_WARNING(
                "A bad estimation of the timing of the rshift_expr operator will happen. This may occur when a "
                "non-constant bit reference of a long ac_type is used. Unrolling such a part may fix the issue.");
         }
         max_size_in = 64;
      }
      info->output_prec = max_size_in;
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
   if(current_op != "widen_mult_expr" && current_op != "mult_expr")
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

   /// fix for vec_perm_expr
   if(current_op == "vec_perm_expr")
   {
      if(info->input_prec[2] == 0)
      {
         std::swap(info->input_prec[2], info->input_prec[1]);
         std::swap(info->base128_input_nelem[2], info->base128_input_nelem[1]);
         std::swap(info->real_input_nelem[2], info->real_input_nelem[1]);
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Got node type precision of " + GET_NAME(g, node));
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
   const technology_managerRef TM = HLS_D->get_technology_manager();
   technology_nodeRef f_unit_mux =
       TM->get_fu(MUX_GATE_STD + STR("_1_") + STR(fu_prec) + "_" + STR(fu_prec) + "_" + STR(fu_prec), LIBRARY_STD_FU);
   THROW_ASSERT(f_unit_mux, "Library miss component: " + std::string(MUX_GATE_STD) + STR("_1_") + STR(fu_prec) + "_" +
                                STR(fu_prec) + "_" + STR(fu_prec));
   auto* fu_br = GetPointerS<functional_unit>(f_unit_mux);
   technology_nodeRef op_mux_node = fu_br->get_operation(MUX_GATE_STD);
   auto* op_mux = GetPointerS<operation>(op_mux_node);
   double mux_delay = time_m_execution_time(op_mux) - get_setup_hold_time();
   if(mux_delay <= 0.0)
   {
      mux_delay = get_setup_hold_time() / 2;
   }
   //#define MUX_MARGIN 1.3
   //   return mux_delay*ALLOCATION_MUX_MARGIN+get_setup_hold_time();
   //   return mux_delay+get_setup_hold_time()*ALLOCATION_MUX_MARGIN;
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
         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                       "    Corresponding operation: " +
                           tree_helper::NormalizeTypename(
                               GetPointerS<const gimple_node>(TreeM->GetTreeNode(bind.first))->operation) +
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
                           GetPointerS<const gimple_node>(TreeM->GetTreeNode(bind.first.first))->operation + ")");
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
   const HLS_deviceRef HLS_D = hls_manager->get_HLS_device();
   std::string library_name = HLS_D->get_technology_manager()->get_library(fu_name);
   if(library_name == "")
   {
      return technology_nodeRef();
   }
   return HLS_D->get_technology_manager()->get_fu(fu_name, library_name);
}

unsigned int AllocationInformation::GetCycleLatency(const vertex operationID) const
{
   return GetCycleLatency(op_graph->CGetOpNodeInfo(operationID)->GetNodeId());
}

unsigned int AllocationInformation::GetCycleLatency(const unsigned int operationID) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Get cycle latency of " + ((operationID != ENTRY_ID && operationID != EXIT_ID) ?
                                                    STR(TreeM->GetTreeNode(operationID)) :
                                                    "Entry/Exit"));
   if(CanImplementSetNotEmpty(operationID))
   {
      const auto actual_latency = get_cycles(GetFuType(operationID), operationID);
      const auto ret_value = actual_latency != 0 ? actual_latency : 1;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Latency of allocation fu is " + STR(ret_value));
      return ret_value;
   }

   THROW_ASSERT(operationID != ENTRY_ID && operationID != EXIT_ID, "Entry or exit not allocated");
   const auto tn = TreeM->GetTreeNode(operationID);
   if(tn->get_kind() == gimple_assign_K)
   {
      const auto ga = GetPointerS<const gimple_assign>(tn);
      const auto right_kind = ga->op1->get_kind();
      if(right_kind == widen_mult_expr_K || right_kind == mult_expr_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Latency of not allocated fu is 1: possibly inaccurate");
         const auto data_bitsize = tree_helper::Size(ga->op0);
         const auto fu_prec = resize_1_8_pow2(data_bitsize);
         const auto in_prec = right_kind == mult_expr_K ? fu_prec : (fu_prec / 2);
         const auto fu_name =
             tree_node::GetString(right_kind) + "_FU_" + STR(in_prec) + "_" + STR(in_prec) + "_" + STR(fu_prec) + "_0";
         const auto new_stmt_temp = HLS_D->get_technology_manager()->get_fu(fu_name, LIBRARY_STD_FU);
         THROW_ASSERT(new_stmt_temp, "Functional unit '" + fu_name + "' not found");
         const auto new_stmt_fu = GetPointerS<const functional_unit>(new_stmt_temp);
         const auto new_stmt_op_temp = new_stmt_fu->get_operation(tree_node::GetString(right_kind));
         const auto new_stmt_op = GetPointerS<operation>(new_stmt_op_temp);
         return new_stmt_op->time_m->get_cycles();
      }
      else if(right_kind == call_expr_K)
      {
         return 0;
      }
      else if(right_kind == ssa_name_K || right_kind == integer_cst_K || right_kind == cond_expr_K ||
              right_kind == vec_cond_expr_K || right_kind == nop_expr_K || right_kind == addr_expr_K ||
              right_kind == convert_expr_K || right_kind == lut_expr_K || right_kind == extract_bit_expr_K ||
              right_kind == bit_ior_concat_expr_K || right_kind == truth_not_expr_K || right_kind == bit_not_expr_K ||
              right_kind == negate_expr_K || right_kind == truth_and_expr_K || right_kind == truth_or_expr_K ||
              right_kind == truth_xor_expr_K || right_kind == bit_and_expr_K || right_kind == bit_ior_expr_K ||
              right_kind == bit_xor_expr_K || right_kind == rshift_expr_K || right_kind == lshift_expr_K ||
              right_kind == plus_expr_K || right_kind == pointer_plus_expr_K || right_kind == minus_expr_K ||
              right_kind == eq_expr_K || right_kind == ne_expr_K || right_kind == lt_expr_K ||
              right_kind == le_expr_K || right_kind == gt_expr_K || right_kind == ge_expr_K ||
              right_kind == ternary_plus_expr_K || right_kind == ternary_mp_expr_K || right_kind == ternary_pm_expr_K ||
              right_kind == ternary_mm_expr_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Latency of not allocated fu is 1");
         return 1;
      }
      THROW_UNREACHABLE("Unsupported right part (" + tree_node::GetString(right_kind) + ") of gimple assignment " +
                        ga->ToString());
   }
   else if(tn->get_kind() == gimple_multi_way_if_K || tn->get_kind() == gimple_cond_K ||
           tn->get_kind() == gimple_phi_K || tn->get_kind() == gimple_nop_K || tn->get_kind() == gimple_return_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Latency of not allocated fu is 1");
      return 1;
   }

   return 0;
}

std::pair<double, double> AllocationInformation::GetTimeLatency(const vertex operationID,
                                                                const unsigned int functional_unit,
                                                                const unsigned int stage) const
{
   return GetTimeLatency(op_graph->CGetOpNodeInfo(operationID)->GetNodeId(), functional_unit, stage);
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
         auto tn = TreeM->GetTreeNode(time_operation_index);
         if(tn->get_kind() == gimple_assign_K && GetPointerS<const gimple_assign>(tn)->op1->get_kind() == lut_expr_K)
         {
            const auto le = GetPointerS<lut_expr>(GetPointerS<const gimple_assign>(tn)->op1);
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
          get_correction_time(
              fu_type, GetPointerS<const gimple_node>(TreeM->GetTreeNode(time_operation_index))->operation, n_ins);
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
             get_correction_time(
                 fu_type, GetPointerS<const gimple_node>(TreeM->GetTreeNode(time_operation_index))->operation, n_ins);
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
      const auto op_stmt = TreeM->GetTreeNode(time_operation_index);
      const auto op_stmt_kind = op_stmt->get_kind();
      if(op_stmt_kind == gimple_assign_K)
      {
         const auto ga = GetPointerS<const gimple_assign>(op_stmt);
         const auto op1_kind = ga->op1->get_kind();
         if(op1_kind == ssa_name_K || op1_kind == integer_cst_K || op1_kind == convert_expr_K ||
            op1_kind == nop_expr_K || op1_kind == addr_expr_K || op1_kind == bit_ior_concat_expr_K ||
            op1_kind == extract_bit_expr_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Time is 0.0,0.0");
            return std::make_pair(0.0, 0.0);
         }
         else if((op1_kind == rshift_expr_K || op1_kind == lshift_expr_K) &&
                 GetPointerS<const binary_expr>(ga->op1)->op1->get_kind() == integer_cst_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Time is 0.0,0.0");
            return std::make_pair(0.0, 0.0);
         }
         else if(op1_kind == cond_expr_K || op1_kind == vec_cond_expr_K)
         {
            THROW_ASSERT(tree_helper::Size(GetPointerS<const ternary_expr>(ga->op1)->op0) == 1,
                         "Cond expr not allocated " + ga->op1->ToString());
            /// Computing time of cond_expr as time of cond_expr_FU - setup_time
            const auto data_bitsize = tree_helper::Size(ga->op0);
            const auto fu_prec = resize_1_8_pow2(data_bitsize);
            const auto op_execution_time = mux_time_unit(fu_prec);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "<--Time is mux time (precision is " + STR(fu_prec) + ") " + STR(op_execution_time) +
                               ",0.0");
            return std::make_pair(op_execution_time, 0.0);
         }

         const auto data_bitsize = tree_helper::Size(ga->op0);
         const auto fu_prec = resize_1_8_pow2(data_bitsize);
         std::string fu_name;
         if(op1_kind == widen_mult_expr_K || op1_kind == mult_expr_K)
         {
            const auto in_prec = op1_kind == mult_expr_K ? fu_prec : (fu_prec / 2);
            fu_name =
                tree_node::GetString(op1_kind) + "_FU_" + STR(in_prec) + "_" + STR(in_prec) + "_" + STR(fu_prec) + "_0";
            const auto new_stmt_temp = HLS_D->get_technology_manager()->get_fu(fu_name, LIBRARY_STD_FU);
            THROW_ASSERT(new_stmt_temp, "Functional unit '" + fu_name + "' not found");
            const auto new_stmt_fu = GetPointerS<const functional_unit>(new_stmt_temp);
            const auto new_stmt_op_temp = new_stmt_fu->get_operation(tree_node::GetString(op1_kind));
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
         else if(op1_kind == lut_expr_K)
         {
            fu_name = tree_node::GetString(op1_kind) + "_FU";
         }
         else if(GetPointer<const unary_expr>(ga->op1))
         {
            fu_name = tree_node::GetString(op1_kind) + "_FU_" + STR(fu_prec) + "_" + STR(fu_prec);
         }
         else if(GetPointer<const binary_expr>(ga->op1))
         {
            fu_name = tree_node::GetString(op1_kind) + "_FU_" + STR(fu_prec) + "_" + STR(fu_prec) + "_" + STR(fu_prec);
         }
         else if(GetPointer<const ternary_expr>(ga->op1))
         {
            fu_name = tree_node::GetString(op1_kind) + "_FU_" + STR(fu_prec) + "_" + STR(fu_prec) + "_" + STR(fu_prec) +
                      "_" + STR(fu_prec);
         }
         else
         {
            THROW_UNREACHABLE("Latency of " + op_stmt->ToString() + " cannot be computed");
         }
         const auto new_stmt_temp = HLS_D->get_technology_manager()->get_fu(fu_name, LIBRARY_STD_FU);
         THROW_ASSERT(new_stmt_temp, "Functional unit '" + fu_name + "' not found");
         const auto new_stmt_fu = GetPointerS<const functional_unit>(new_stmt_temp);
         const auto new_stmt_op_temp = new_stmt_fu->get_operation(tree_node::GetString(op1_kind));
         const auto new_stmt_op = GetPointerS<operation>(new_stmt_op_temp);
         auto op_execution_time = time_m_execution_time(new_stmt_op);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Uncorrected execution time is " + STR(op_execution_time));
         op_execution_time = op_execution_time - get_setup_hold_time();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Time is " + STR(op_execution_time) + ",0.0");
         return std::make_pair(op_execution_time, 0.0);
      }
      else if(op_stmt_kind == gimple_multi_way_if_K || op_stmt_kind == gimple_cond_K)
      {
         auto controller_delay = estimate_controller_delay_fb();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Time is " + STR(controller_delay) + ",0.0");
         return std::make_pair(controller_delay, 0.0);
      }
      else if(op_stmt_kind == gimple_phi_K || op_stmt_kind == gimple_nop_K || op_stmt_kind == gimple_return_K)
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
      const auto tn = TreeM->GetTreeNode(statement_index);
      if(tn->get_kind() != gimple_assign_K)
      {
         return 0;
      }
      if(tn->get_kind() == gimple_assign_K && GetPointerS<const gimple_assign>(tn)->op0->get_kind() != ssa_name_K)
      {
         return 0;
      }
      const auto sn = GetPointerS<const ssa_name>(GetPointerS<const gimple_assign>(tn)->op0);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing uses of " + sn->ToString());
      for(const auto& use : sn->CGetUseStmts())
      {
         const auto target = use.first;
         if(target->get_kind() == gimple_phi_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Phi: " + target->ToString());
            const auto gp = GetPointerS<const gimple_phi>(target);
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
   const auto statement = TreeM->GetTreeNode(statement_index);
   THROW_ASSERT(statement->get_kind() == gimple_assign_K, statement->ToString());
   const auto sn = GetPointerS<const gimple_assign>(statement)->op0;
   THROW_ASSERT(sn, "");
   const auto precision = resize_1_8_pow2(tree_helper::Size(sn));
   const auto mux_time = estimate_muxNto1_delay(precision, static_cast<unsigned int>(phi_in_degree));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "<--Delay (" + STR(phi_in_degree) + " with " + STR(precision) + " bits) is " + STR(mux_time));
   return mux_time;
}

double AllocationInformation::GetCondExprTimeLatency(const unsigned int operation_index) const
{
   const auto tn = TreeM->GetTreeNode(operation_index);
   THROW_ASSERT(tn->get_kind() == gimple_phi_K, "Tree node is " + STR(tn));
   /// Computing time of cond_expr as time of cond_expr_FU - setup_time
   /// In this way we are correctly estimating only phi with two inputs
   const auto type = tree_helper::CGetType(tn);
   const auto data_bitsize = tree_helper::Size(type);
   const auto fu_prec = resize_1_8_pow2(data_bitsize);
   return mux_time_unit(fu_prec);
}

unsigned int AllocationInformation::GetFuType(const vertex operation) const
{
   return GetFuType(op_graph->CGetOpNodeInfo(operation)->GetNodeId());
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
         THROW_UNREACHABLE("Multiple fus not supported: " + STR(TreeM->GetTreeNode(operation)));
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
   const technology_managerRef TM = HLS_D->get_technology_manager();
   technology_nodeRef f_unit_mux =
       TM->get_fu(MUX_GATE_STD + STR("_1_") + STR(fu_prec) + "_" + STR(fu_prec) + "_" + STR(fu_prec), LIBRARY_STD_FU);
   THROW_ASSERT(f_unit_mux, "Library miss component: " + std::string(MUX_GATE_STD) + STR("_1_") + STR(fu_prec) + "_" +
                                STR(fu_prec) + "_" + STR(fu_prec));
   auto* fu_mux = GetPointerS<functional_unit>(f_unit_mux);
   auto area = fu_mux->area_m->get_resource_value(area_info::SLICE_LUTS);
   if(area == 0.0)
   {
      area = fu_mux->area_m->get_area_value() - 1.0;
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
       boost::num_vertices(*hls_manager->CGetFunctionBehavior(function_index)->CGetBBGraph(FunctionBehavior::BB)) +
       get_n_complex_operations();
   double n_states_factor = static_cast<double>(n_states) / NUM_CST_allocation_default_states_number_normalization_BB;
   if(hls->STG && hls->STG->get_number_of_states())
   {
      n_states = hls->STG->get_number_of_states();
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
   technology_nodeRef op_node = fu->get_operation("mult_expr");
   auto* op = GetPointerS<operation>(op_node);
   double delay = time_m_execution_time(op);
   delay = delay * controller_delay_multiplier *
           ((1 - exp(-n_states_factor)) +
            n_states_factor / NUM_CST_allocation_default_states_number_normalization_linear_factor);
   if(delay < 2 * get_setup_hold_time())
   {
      delay = 2 * get_setup_hold_time();
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
      unsigned int type_index = tree_helper::get_type_index(TreeM, var);
      if(tree_helper::is_an_array(TreeM, type_index) || tree_helper::is_a_struct(TreeM, type_index) ||
         tree_helper::is_an_union(TreeM, type_index))
      {
         return false;
      }
      else
      {
         return true;
      }
   };

   if(memory_type == MEMORY_TYPE_SYNCHRONOUS_UNALIGNED)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Applying memory correction for MEMORY_TYPE_SYNCHRONOUS_UNALIGNED");
      unsigned var = get_memory_var(fu);
      if(!Rmem->is_a_proxied_variable(var))
      {
         is_proxied_correction = true;
      }
      else if(Rmem->is_private_memory(var))
      {
         is_private_correction = true;
      }
      if(Rmem->is_read_only_variable(var))
      {
         is_read_only_correction = true;
      }
      is_single_variable = single_var_lambda(var);

      elmt_bitsize = Rmem->get_bram_bitsize();

#if ARRAY_CORRECTION
      auto type_index = tree_helper::get_type_index(TreeM, var);
      if(tree_helper::is_an_array(TreeM, type_index))
      {
         std::vector<unsigned int> dims;
         tree_helper::get_array_dimensions(TreeM, type_index, dims);
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
            technology_nodeRef op_node = Fu->get_operation("plus_expr");
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
      if(!Rmem->is_a_proxied_variable(var))
      {
         is_proxied_correction = true;
      }
      if(Rmem->is_read_only_variable(var))
      {
         is_read_only_correction = true;
      }
      is_single_variable = single_var_lambda(var);

      const auto type_node = tree_helper::CGetType(TreeM->GetTreeNode(var));
      elmt_bitsize = tree_helper::AccessedMaximumBitsize(type_node, 1);
#if ARRAY_CORRECTION
      if(tree_helper::IsArrayEquivType(type_node))
      {
         const auto dims = tree_helper::GetArrayDimensions(type_node);
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
            technology_nodeRef op_node = Fu->get_operation("plus_expr");
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
   else if(memory_type == MEMORY_TYPE_SYNCHRONOUS_SDS || memory_type == MEMORY_TYPE_SYNCHRONOUS_SDS_BUS)
   {
      INDENT_DBG_MEX(
          DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
          "---Applying memory correction for MEMORY_TYPE_SYNCHRONOUS_SDS and MEMORY_TYPE_SYNCHRONOUS_SDS_BUS");
      unsigned var = get_memory_var(fu);
      is_single_variable = single_var_lambda(var);

      const auto type_node = tree_helper::CGetType(TreeM->GetTreeNode(var));
      elmt_bitsize = tree_helper::AccessedMaximumBitsize(type_node, 1);
#if ARRAY_CORRECTION
      if(tree_helper::IsArrayEquivType(type_node))
      {
         const auto dims = tree_helper::GetArrayDimensions(type_node);
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
            technology_nodeRef op_node = Fu->get_operation("plus_expr");
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
      if(Rmem->is_read_only_variable(var))
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
      if(Rmem->is_sds_var(var))
      {
         if(memory_ctrl_type == MEMORY_CTRL_TYPE_PROXY || memory_ctrl_type == MEMORY_CTRL_TYPE_DPROXY ||
            memory_ctrl_type == MEMORY_CTRL_TYPE_SPROXY)
         {
            if(Rmem->is_private_memory(var))
            {
               if(memory_ctrl_type == MEMORY_CTRL_TYPE_DPROXY)
               {
                  f_unit_sds = HLS_D->get_technology_manager()->get_fu(ARRAY_1D_STD_DISTRAM_SDS, LIBRARY_STD_FU);
               }
               else
               {
                  f_unit_sds =
                      HLS_D->get_technology_manager()->get_fu(ARRAY_1D_STD_BRAM_SDS + latency_postfix, LIBRARY_STD_FU);
               }
            }
            else
            {
               f_unit_sds =
                   HLS_D->get_technology_manager()->get_fu(ARRAY_1D_STD_BRAM_SDS_BUS + latency_postfix, LIBRARY_STD_FU);
            }
         }
         else
         {
            if(Rmem->is_private_memory(var))
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

         const auto type_node = tree_helper::CGetType(TreeM->GetTreeNode(var));
         elmt_bitsize = tree_helper::AccessedMaximumBitsize(type_node, 1);
      }
      else
      {
         f_unit_sds = HLS_D->get_technology_manager()->get_fu(ARRAY_1D_STD_BRAM_NN + latency_postfix, LIBRARY_STD_FU);
         if(Rmem->is_private_memory(var))
         {
            is_private_correction = true;
         }
         elmt_bitsize = Rmem->get_bram_bitsize();
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
      const auto type_node = tree_helper::CGetType(TreeM->GetTreeNode(var));
      if(tree_helper::IsArrayEquivType(type_node))
      {
         const auto dims = tree_helper::GetArrayDimensions(type_node);
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
            technology_nodeRef op_node = Fu->get_operation("plus_expr");
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
      elmt_bitsize = Rmem->get_bram_bitsize();
   }
   else if(is_single_bool_test_cond_expr_units(fu))
   {
      auto prec = get_prec(fu);
      auto fu_prec = resize_1_8_pow2(prec);
      if(fu_prec > 1)
      {
         const technology_managerRef TM = HLS_D->get_technology_manager();
         auto true_delay = [&]() -> double {
            technology_nodeRef f_unit_ce = TM->get_fu(COND_EXPR_STD "_1_1_1_1", LIBRARY_STD_FU);
            auto* fu_ce = GetPointerS<functional_unit>(f_unit_ce);
            technology_nodeRef op_ce_node = fu_ce->get_operation("cond_expr");
            auto* op_ce = GetPointerS<operation>(op_ce_node);
            double setup_time = get_setup_hold_time();
            return time_m_execution_time(op_ce) - setup_time;
         }();
         technology_nodeRef f_unit_ce = TM->get_fu(get_fu_name(fu).first, LIBRARY_STD_FU);
         auto* fu_ce = GetPointerS<functional_unit>(f_unit_ce);
         technology_nodeRef op_ce_node = fu_ce->get_operation("cond_expr");
         auto* op_ce = GetPointerS<operation>(op_ce_node);
         double setup_time = get_setup_hold_time();
         double ce_delay = time_m_execution_time(op_ce) - setup_time;
         double correction = ce_delay - true_delay;
         res_value = res_value + correction;
      }
   }
   else if(is_simple_pointer_plus_expr(fu))
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
   else if(operation_name == "lut_expr")
   {
      // std::cerr << "get_correction_time " << operation_name << " - " << n_ins << "\n";
      if(HLS_D->has_parameter("max_lut_size") && HLS_D->get_parameter<size_t>("max_lut_size") != 0)
      {
         const technology_managerRef TM = HLS_D->get_technology_manager();
         technology_nodeRef f_unit_lut = TM->get_fu(LUT_EXPR_STD, LIBRARY_STD_FU);
         auto* fu_lut = GetPointerS<functional_unit>(f_unit_lut);
         technology_nodeRef op_lut_node = fu_lut->get_operation(operation_name);
         auto* op_lut = GetPointerS<operation>(op_lut_node);
         double setup_time = get_setup_hold_time();
         double lut_delay = time_m_execution_time(op_lut) - setup_time;
         res_value = res_value + lut_delay;
         auto max_lut_size = HLS_D->get_parameter<size_t>("max_lut_size");
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
   std::string function_name = behavioral_helper->get_function_name();
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
       is_single_bool_test_cond_expr_units(fu_s1) ? (mux_area > 1 ? (mux_area - 1) : 0) : get_area(fu_s1);
   if(resource_area > mux_area && resource_area - mux_area < 4)
   {
      resource_area = mux_area;
   }
   const auto fu_name = list_of_FU[fu_s1]->get_name();
   if(parameters->IsParameter("no-share-max") && parameters->GetParameter<int>("no-share-max") &&
      (fu_name.find("max_expr_FU_") != std::string::npos || fu_name.find("min_expr_FU_") != std::string::npos))
   {
      resource_area = 0.0;
   }
   return (resource_area / mux_area) + 3 * get_DSPs(fu_s1);
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
   return (is_memory_unit(fu_name) && Rmem->is_read_only_variable(get_memory_var(fu_name))) ||
          (is_proxy_memory_unit(fu_name) && Rmem->is_read_only_variable(get_proxy_memory_var(fu_name)));
}

bool AllocationInformation::is_single_bool_test_cond_expr_units(const unsigned int fu_name) const
{
   THROW_ASSERT(fu_name < get_number_fu_types(), "functional unit id not meaningful");
   return single_bool_test_cond_expr_units.find(fu_name) != single_bool_test_cond_expr_units.end();
}

bool AllocationInformation::is_simple_pointer_plus_expr(const unsigned int fu_name) const
{
   THROW_ASSERT(fu_name < get_number_fu_types(), "functional unit id not meaningful");
   return simple_pointer_plus_expr.find(fu_name) != simple_pointer_plus_expr.end();
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

CustomSet<unsigned int> AllocationInformation::ComputeRoots(const unsigned int ssa, const AbsControlStep cs) const
{
   const auto bb_version = hls_manager->CGetFunctionBehavior(function_index)->GetBBVersion();
   if(ssa_bb_versions.find(ssa) != ssa_bb_versions.end() &&
      ssa_bb_versions.find(ssa)->second == std::pair<unsigned int, AbsControlStep>(bb_version, cs))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Compute roots - Using cached values of " + STR(ssa) + " at version " + STR(bb_version));
      return ssa_roots.find(ssa)->second;
   }
   else
   {
      const auto schedule = hls->Rsch;
      CustomSet<unsigned int> already_analyzed_ssas;
      CustomSet<unsigned int> ssa_to_be_analyzeds;
      CustomSet<unsigned int> roots;
      ssa_to_be_analyzeds.insert(ssa);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Computing roots of " + STR(ssa) + " at version " + STR(bb_version));
      while(ssa_to_be_analyzeds.size())
      {
         const auto current_tn_index = *(ssa_to_be_analyzeds.begin());
         ssa_to_be_analyzeds.erase(ssa_to_be_analyzeds.begin());
         if(already_analyzed_ssas.find(current_tn_index) != already_analyzed_ssas.end())
         {
            continue;
         }
         already_analyzed_ssas.insert(current_tn_index);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Considering " + STR(TreeM->GetTreeNode(current_tn_index)));
         const auto current_sn = GetPointer<const ssa_name>(TreeM->GetTreeNode(current_tn_index));
         if(not current_sn)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ignored since not ssa");
            continue;
         }
         const auto current_sn_def = current_sn->CGetDefStmt();
         if(cs.second == AbsControlStep::UNKNOWN &&
            cs.first != GetPointerS<const gimple_node>(current_sn_def)->bb_index)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ignored since defined in different basic block");
            continue;
         }
         const auto current_def_ga = GetPointer<const gimple_assign>(current_sn_def);
         if(not current_def_ga)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Adding as root " + current_sn->ToString());
            roots.insert(current_tn_index);
            continue;
         }
         const auto be = GetPointer<const binary_expr>(current_def_ga->op1);
         if(be &&
            (be->get_kind() == rshift_expr_K || be->get_kind() == lshift_expr_K || be->get_kind() == bit_and_expr_K))
         {
            if(be->op1->get_kind() != integer_cst_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Adding as root " + current_sn->ToString() +
                                  " which is defined in a shift by variable or in an and with a variable");
               roots.insert(current_tn_index);
               continue;
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Defined in a shift by constant or in an and with a constant");
               if(already_analyzed_ssas.find(be->op0->index) == already_analyzed_ssas.end())
               {
                  ssa_to_be_analyzeds.insert(be->op0->index);
               }
               continue;
            }
         }
         if(be && (be->get_kind() == gt_expr_K || be->get_kind() == ge_expr_K || be->get_kind() == lt_expr_K ||
                   be->get_kind() == le_expr_K || be->get_kind() == eq_expr_K || be->get_kind() == ne_expr_K ||
                   be->get_kind() == truth_and_expr_K || be->get_kind() == truth_or_expr_K ||
                   be->get_kind() == truth_xor_expr_K))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Defined in a comparison");
            if(already_analyzed_ssas.find(be->op0->index) == already_analyzed_ssas.end())
            {
               ssa_to_be_analyzeds.insert(be->op0->index);
            }
            if(already_analyzed_ssas.find(be->op1->index) == already_analyzed_ssas.end())
            {
               ssa_to_be_analyzeds.insert(be->op1->index);
            }
            continue;
         }
         const auto ue = GetPointer<const unary_expr>(current_def_ga->op1);
         if(ue && (ue->get_kind() == truth_not_expr_K || ue->get_kind() == nop_expr_K))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Defined in not or nop");
            if(already_analyzed_ssas.find(ue->op->index) == already_analyzed_ssas.end())
            {
               ssa_to_be_analyzeds.insert(ue->op->index);
            }
            continue;
         }
         const auto ce = GetPointer<const cond_expr>(current_def_ga->op1);
         if(ce)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Defined in cond expr");
            if(already_analyzed_ssas.find(ce->op0->index) == already_analyzed_ssas.end())
            {
               ssa_to_be_analyzeds.insert(ce->op0->index);
            }
            if(already_analyzed_ssas.find(ce->op1->index) == already_analyzed_ssas.end())
            {
               ssa_to_be_analyzeds.insert(ce->op1->index);
            }
            if(already_analyzed_ssas.find(ce->op2->index) == already_analyzed_ssas.end())
            {
               ssa_to_be_analyzeds.insert(ce->op2->index);
            }
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Adding as root " + current_sn->ToString());
         roots.insert(current_tn_index);
      }
      ssa_bb_versions[ssa] = std::pair<unsigned int, AbsControlStep>(bb_version, cs);
      ssa_roots[ssa] = roots;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--Computed roots of " + STR(ssa) + " at version " + STR(bb_version) + ": " + STR(roots.size()) +
                         " elements");
      return roots;
   }
}

CustomSet<unsigned int> AllocationInformation::ComputeDrivenCondExpr(const unsigned int ssa) const
{
   const auto bb_version = hls_manager->CGetFunctionBehavior(function_index)->GetBBVersion();
   if(cond_expr_bb_versions.find(ssa) != cond_expr_bb_versions.end() &&
      cond_expr_bb_versions.find(ssa)->second == bb_version)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Computing cond_exprs starting from " + STR(TreeM->GetTreeNode(ssa)) +
                         " - Using cached values");
      return ssa_cond_exprs.find(ssa)->second;
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Computing cond_exprs starting from " + STR(TreeM->GetTreeNode(ssa)));
      CustomSet<unsigned int> cond_expr_ga_indices;
      CustomSet<unsigned int> ssa_to_be_analyzeds;
      CustomSet<unsigned int> already_analyzed_ssas;
      ssa_to_be_analyzeds.insert(ssa);
      while(ssa_to_be_analyzeds.size())
      {
         const auto current_tn_index = *(ssa_to_be_analyzeds.begin());
         ssa_to_be_analyzeds.erase(ssa_to_be_analyzeds.begin());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Considering " + STR(TreeM->GetTreeNode(current_tn_index)));
         const auto current_sn = GetPointer<const ssa_name>(TreeM->GetTreeNode(current_tn_index));
         if(!current_sn)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ignored since not ssa");
            continue;
         }
         for(const auto& use_stmt : current_sn->CGetUseStmts())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering use in " + STR(use_stmt.first));
            if(use_stmt.first->get_kind() != gimple_assign_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipping since not gimple assignment");
               continue;
            }
            const auto current_use_ga = GetPointerS<const gimple_assign>(use_stmt.first);
            const auto be = GetPointer<const binary_expr>(current_use_ga->op1);
            if(be &&
               (be->get_kind() == rshift_expr_K || be->get_kind() == lshift_expr_K || be->get_kind() == bit_and_expr_K))
            {
               if(be->op1->get_kind() != integer_cst_K)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "<--Used in a shift by a variable or in an and with a variable");
                  continue;
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "<--Used in a shift by constant or in an and with a constant");
                  if(already_analyzed_ssas.find(current_use_ga->op0->index) == already_analyzed_ssas.end())
                  {
                     ssa_to_be_analyzeds.insert(current_use_ga->op0->index);
                  }
                  continue;
               }
            }
            if(be && (be->get_kind() == gt_expr_K || be->get_kind() == ge_expr_K || be->get_kind() == lt_expr_K ||
                      be->get_kind() == le_expr_K || be->get_kind() == eq_expr_K || be->get_kind() == ne_expr_K ||
                      be->get_kind() == truth_and_expr_K || be->get_kind() == truth_or_expr_K ||
                      be->get_kind() == truth_xor_expr_K))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Used in a comparison");
               if(already_analyzed_ssas.find(current_use_ga->op0->index) == already_analyzed_ssas.end())
               {
                  ssa_to_be_analyzeds.insert(current_use_ga->op0->index);
               }
               continue;
            }
            const auto ue = GetPointer<const unary_expr>(current_use_ga->op1);
            if(ue && ue->get_kind() == truth_not_expr_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Used in not");
               if(already_analyzed_ssas.find(current_use_ga->op0->index) == already_analyzed_ssas.end())
               {
                  ssa_to_be_analyzeds.insert(current_use_ga->op0->index);
               }
               continue;
            }
            if(ue && ue->get_kind() == nop_expr_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Used in nop");
               if(already_analyzed_ssas.find(current_use_ga->op0->index) == already_analyzed_ssas.end())
               {
                  ssa_to_be_analyzeds.insert(current_use_ga->op0->index);
               }
               continue;
            }
            if(current_use_ga->op1->get_kind() == cond_expr_K)
            {
               const auto ce = GetPointer<const cond_expr>(current_use_ga->op1);
               if(ce->op0->index != current_tn_index)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "<--Used as operand of a cond_expr, but not as condition");
                  continue;
               }
               cond_expr_ga_indices.insert(current_use_ga->index);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Adding cond expr " + STR(current_use_ga));
               continue;
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipping");
               continue;
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Considered " + STR(TreeM->GetTreeNode(current_tn_index)));
      }
      cond_expr_bb_versions[ssa] = bb_version;
      ssa_cond_exprs[ssa] = cond_expr_ga_indices;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--Computed cond_exprs starting from " + STR(TreeM->GetTreeNode(ssa)));
      return cond_expr_ga_indices;
   }
}

double AllocationInformation::GetConnectionTime(const vertex first_operation, const vertex second_operation,
                                                const AbsControlStep cs) const
{
   const auto first_operation_index = op_graph->CGetOpNodeInfo(first_operation)->GetNodeId();
   const auto second_operation_index = second_operation ? op_graph->CGetOpNodeInfo(second_operation)->GetNodeId() : 0;
   return GetConnectionTime(first_operation_index, second_operation_index, cs);
}

double AllocationInformation::GetConnectionTime(const unsigned int first_operation, const unsigned int second_operation,
                                                const AbsControlStep cs) const
{
   if(!parameters->getOption<bool>(OPT_estimate_logic_and_connections))
   {
      return 0;
   }
   if(second_operation == 0)
   {
      if(first_operation == ENTRY_ID || first_operation == EXIT_ID)
      {
         return 0.0;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Get end delay of " + STR(first_operation));
      double end_delay = 0.0;
      const auto first_operation_kind = TreeM->GetTreeNode(first_operation)->get_kind();
      if(first_operation_kind == gimple_multi_way_if_K || first_operation_kind == gimple_switch_K ||
         first_operation_kind == gimple_cond_K)
      {
         end_delay = estimate_controller_delay_fb();
      }
      else
      {
         double phi_delay = GetPhiConnectionLatency(first_operation);
         if(phi_delay > end_delay)
         {
            end_delay = phi_delay;
         }
         double to_dsp_register_delay = GetToDspRegisterDelay(first_operation);
         if(to_dsp_register_delay > end_delay)
         {
            end_delay = to_dsp_register_delay;
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--Got end delay of " + STR(first_operation) + ": " + STR(end_delay));
      return end_delay;
   }
   if(is_operation_PI_registered(second_operation))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Computing overall connection time " + STR(first_operation) + "-->" + STR(second_operation) +
                         " Second operation has registered inputs");
      const auto second_operation_tn = TreeM->GetTreeNode(second_operation);
      const auto second_operation_name = GetPointerS<const gimple_node>(second_operation_tn)->operation;
      const auto called_function = TreeM->GetFunction(second_operation_name);
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
      return mux_delay;
   }
   else if(first_operation != ENTRY_ID && first_operation != EXIT_ID && second_operation != ENTRY_ID &&
           second_operation != EXIT_ID &&
           (behavioral_helper->IsLut(first_operation) || behavioral_helper->IsLut(second_operation)))
   {
      return 0;
   }
   else
   {
      const auto second_operation_tn = TreeM->GetTreeNode(second_operation);
      tree_nodeRef cond_def;

      double connection_time = 0.0;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Computing overall connection time " + STR(first_operation) + "-->" + STR(second_operation));

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Computing connection time due to fanout " + STR(first_operation) + "-->" +
                         STR(second_operation));
      for(const auto& used_ssa : tree_helper::ComputeSsaUses(TreeM->GetTreeNode(second_operation)))
      {
         const auto used_ssa_sn = GetPointer<const ssa_name>(used_ssa.first);
         if(used_ssa_sn && used_ssa_sn->CGetDefStmt()->index == first_operation)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Transferred data is " + STR(used_ssa.first));
            const auto roots = ComputeRoots(used_ssa_sn->index, cs);
            if(roots.find(used_ssa_sn->index) != roots.end())
            {
               CustomSet<unsigned int> cond_expr_ga_indices;
               const auto this_cond_expr = ComputeDrivenCondExpr(used_ssa_sn->index);
               cond_expr_ga_indices.insert(this_cond_expr.begin(), this_cond_expr.end());

               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing fan out");
               size_t n_fo = 0;
               for(const auto cond_expr_ga_index : cond_expr_ga_indices)
               {
                  const auto current_ga = GetPointerS<const gimple_assign>(TreeM->GetTreeNode(cond_expr_ga_index));
                  const auto cond_def_sn = GetPointerS<const ssa_name>(current_ga->op0);
                  const auto local_fo = tree_helper::Size(current_ga->op0) * cond_def_sn->CGetNumberUses();
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Incrementing fan out of " + STR(local_fo) + " because of " +
                                     STR(TreeM->GetTreeNode(cond_expr_ga_index)));
                  n_fo += local_fo;
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Estimated FO of " + STR(first_operation) + "-->" + STR(second_operation) + " = " +
                                  STR(n_fo));
               if(n_fo)
               {
                  double fo_correction = fanout_coefficient * get_setup_hold_time() * static_cast<double>(n_fo);
                  if(fo_correction < connection_offset)
                  {
                     fo_correction = connection_offset;
                  }
                  else if(fo_correction > 1.1 * (connection_offset + get_setup_hold_time()))
                  {
                     fo_correction = 1.1 * (connection_offset + get_setup_hold_time());
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Computed connection time due to fanout " + STR(first_operation) + "-->" +
                                     STR(second_operation) + ": " + STR(fo_correction));
                  connection_time += fo_correction;
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Computed connection time due to fanout" + STR(first_operation) + "-->" +
                                     STR(second_operation) + ": 0.0");
               }
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Input data (coming from other operation) is " + STR(used_ssa.first));
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
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
         const auto first_operation_tn = TreeM->GetTreeNode(first_operation);
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
         else if(GetPointerS<const gimple_node>(first_operation_tn)->operation != "STORE")
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
      if(first_operation != ENTRY_ID && TreeM->GetTreeNode(first_operation)->get_kind() == gimple_assign_K)
      {
         const auto first_operation_tn = TreeM->GetTreeNode(first_operation);
         const auto ga = GetPointerS<const gimple_assign>(first_operation_tn);
         const auto ne = GetPointer<const nop_expr>(ga->op1);
         if(ne)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing connection time due to conversion");
            const auto bool_input_ne = tree_helper::is_int(TreeM, ne->op->index);
            // cppcheck-suppress variableScope
            double fo_correction = 0.0;
            // cppcheck-suppress variableScope
            size_t fanout = 0;
            if(bool_input_ne)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Not expr with signed input in right part");
               const auto output_sn =
                   GetPointer<const ssa_name>(GetPointerS<const gimple_assign>(first_operation_tn)->op0);
               const auto input_sn = GetPointer<const ssa_name>(ne->op);
               if(output_sn && input_sn && tree_helper::Size(ga->op0) > tree_helper::Size(ne->op))
               {
                  fanout = (tree_helper::Size(ga->op0) - tree_helper::Size(ne->op) + 1) * output_sn->CGetNumberUses();
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
      if(first_operation != ENTRY_ID && TreeM->GetTreeNode(first_operation)->get_kind() == gimple_assign_K)
      {
         const auto first_operation_tn = TreeM->GetTreeNode(first_operation);
         const auto op1_kind = GetPointerS<const gimple_assign>(first_operation_tn)->op1->get_kind();
         if(op1_kind == plus_expr_K || op1_kind == minus_expr_K || op1_kind == ternary_plus_expr_K ||
            op1_kind == ternary_pm_expr_K || op1_kind == ternary_mp_expr_K || op1_kind == ternary_mm_expr_K ||
            op1_kind == eq_expr_K || op1_kind == ne_expr_K || op1_kind == gt_expr_K || op1_kind == ge_expr_K ||
            op1_kind == lt_expr_K || op1_kind == le_expr_K || op1_kind == pointer_plus_expr_K)
         {
            const bool adding_connection = [&]() -> bool {
               const auto second_delay = GetTimeLatency(second_operation, fu_binding::UNKNOWN);
               if(second_delay.first > epsilon)
               {
                  return true;
               }
               const auto first_bb_index =
                   GetPointerS<const gimple_assign>(TreeM->GetTreeNode(first_operation))->bb_index;
               const auto zero_distance_operations = GetZeroDistanceOperations(second_operation);
               for(const auto zero_distance_operation : zero_distance_operations)
               {
                  if(GetPointerS<const gimple_node>(TreeM->GetTreeNode(zero_distance_operation))->bb_index ==
                     first_bb_index)
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
      return connection_time;
   }
}

bool AllocationInformation::can_be_asynchronous_ram(tree_managerConstRef TM, unsigned int var, unsigned int threshold,
                                                    bool is_read_only_variable, unsigned channel_number)
{
   tree_nodeRef var_node = TM->GetTreeNode(var);
   auto var_bitsize = tree_helper::Size(var_node);
   const auto hls_d = hls_manager->get_HLS_device();
   if(is_read_only_variable)
   {
      threshold = 32 * threshold;
   }
   else if(hls_d->has_parameter("max_distram_nn_size") && channel_number > 1)
   {
      threshold = hls_d->get_parameter<unsigned int>("max_distram_nn_size");
   }
   if(var_node->get_kind() == var_decl_K)
   {
      const auto vd = GetPointerS<const var_decl>(var_node);
      const auto array_type_node = tree_helper::CGetType(var_node);
      if(GetPointer<const array_type>(array_type_node))
      {
         std::vector<unsigned long long> dims;
         unsigned long long elts_size;
         tree_helper::get_array_dim_and_bitsize(TM, array_type_node->index, dims, elts_size);
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
   else if(GetPointerS<const gimple_node>(TreeM->GetTreeNode(operation))->operation == LOAD)
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

ControlStep AllocationInformation::op_et_to_cycles(double et, double clock_period) const
{
   return ControlStep(static_cast<unsigned int>(ceil(et / clock_period)));
}

bool AllocationInformation::CanBeMerged(const unsigned int first_operation, const unsigned int second_operation) const
{
   if(first_operation == ENTRY_ID || second_operation == EXIT_ID)
   {
      return true;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Checking if " + STR(TreeM->GetTreeNode(first_operation)) + " can be fused in " +
                      STR(TreeM->GetTreeNode(second_operation)));
   //   const auto first_delay = GetTimeLatency(first_operation, fu_binding::UNKNOWN);
   const auto second_delay = GetTimeLatency(second_operation, fu_binding::UNKNOWN);
   if(/*(first_delay.first <= epsilon and first_delay.second <= epsilon) || */ (second_delay.first <= epsilon &&
                                                                                second_delay.second <= epsilon))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes because one of the operations has zero delay");
      return true;
   }
   const auto ga0 = GetPointer<const gimple_assign>(TreeM->GetTreeNode(first_operation));
   const auto ga1 = GetPointer<const gimple_assign>(TreeM->GetTreeNode(second_operation));

   if(ga0 && tree_helper::Size(ga0->op0) == 1 && ga1 && tree_helper::Size(ga1->op1) == 1 &&
      (!CanImplementSetNotEmpty(second_operation) || get_DSPs(GetFuType(second_operation)) == 0.0))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes because single bit");
      return true;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No");
   return false;
}

bool AllocationInformation::CanBeChained(const vertex first_statement, const vertex second_statement) const
{
   const auto first_statement_index = op_graph->CGetOpNodeInfo(first_statement)->GetNodeId();
   const auto second_statement_index = op_graph->CGetOpNodeInfo(second_statement)->GetNodeId();
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
   const auto first_tree_node = TreeM->GetTreeNode(first_statement_index);
   const auto second_tree_node = TreeM->GetTreeNode(second_statement_index);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Checking if (" + STR(second_statement_index) + ") " + STR(second_tree_node) +
                      " can be chained with (" + STR(first_statement_index) + ") " + STR(first_tree_node));
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
   if(GetTimeLatency(
          first_statement_index,
          CanImplementSetNotEmpty(first_statement_index) ? GetFuType(first_statement_index) : fu_binding::UNKNOWN, 0)
              .first > 0.001 &&
      second_load && is_one_cycle_direct_access_memory_unit(GetFuType(second_statement_index)) &&
      (!is_readonly_memory_unit(GetFuType(second_statement_index)) ||
       (!parameters->isOption(OPT_rom_duplication) || !parameters->getOption<bool>(OPT_rom_duplication))) &&
      ((Rmem->get_maximum_references(is_memory_unit(GetFuType(second_statement_index)) ?
                                         get_memory_var(GetFuType(second_statement_index)) :
                                         get_proxy_memory_var(GetFuType(second_statement_index)))) >
       get_number_channels(GetFuType(second_statement_index))))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because second is a load from distributed memory");
      return false;
   }
   auto first_type = first_tree_node->get_kind();
   auto second_store = behavioral_helper->IsStore(second_statement_index);
   /// STORE cannot be executed in the same clock cycle of the condition which controls it
   if((first_type == gimple_cond_K || first_type == gimple_multi_way_if_K) && second_store)
   {
      INDENT_DBG_MEX(
          DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
          "<--No because stores cannot be executed in the same clock cycle of the condition which controls it");
      return false;
   }
   /// UNBOUNDED operations cannot be executed in the same clock cycle of the condition which controls it
   if((first_type == gimple_cond_K || first_type == gimple_multi_way_if_K) &&
      !is_operation_bounded(second_statement_index))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--No because unbounded operations cannot be executed in the same clock cycle of the condition "
                     "which controls it");
      return false;
   }
   /// labels cannot be executed in the same clock cycle of the condition which controls it
   if((first_type == gimple_cond_K || first_type == gimple_multi_way_if_K) &&
      (second_tree_node->get_kind() == gimple_label_K))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--No because labels and nops cannot be executed in the same clock cycle of the condition which "
                     "controls it");
      return false;
   }
   /// Operations with side effect cannot be executed in the same clock cycle of the control_step which controls them
   if((first_type == gimple_cond_K || first_type == gimple_multi_way_if_K) &&
      (GetPointerS<const gimple_node>(second_tree_node)->vdef))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--No because operations with side effect cannot be executed in the same clock cycle of the "
                     "condition which controls it");
      return false;
   }
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
   HLSIR::Initialize();
   op_graph = hls_manager->CGetFunctionBehavior(function_index)->CGetOpGraph(FunctionBehavior::CFG);
   HLS_C = hls->HLS_C;
   HLS_D = hls->HLS_D;
   behavioral_helper = hls_manager->CGetFunctionBehavior(function_index)->CGetBehavioralHelper();
   Rmem = hls_manager->Rmem;
   TreeM = hls_manager->get_tree_manager();
   connection_time_ratio =
       HLS_D->has_parameter("connection_time_ratio") ? HLS_D->get_parameter<double>("connection_time_ratio") : 1;
   controller_delay_multiplier = HLS_D->has_parameter("controller_delay_multiplier") ?
                                     HLS_D->get_parameter<double>("controller_delay_multiplier") :
                                     1;
   setup_multiplier = HLS_D->has_parameter("setup_multiplier") ? HLS_D->get_parameter<double>("setup_multiplier") : 1.0;
   time_multiplier = HLS_D->has_parameter("time_multiplier") ? HLS_D->get_parameter<double>("time_multiplier") : 1.0;
   mux_time_multiplier =
       HLS_D->has_parameter("mux_time_multiplier") ? HLS_D->get_parameter<double>("mux_time_multiplier") : 1.0;
   memory_correction_coefficient = HLS_D->has_parameter("memory_correction_coefficient") ?
                                       HLS_D->get_parameter<double>("memory_correction_coefficient") :
                                       0.7;

   connection_offset = parameters->IsParameter("ConnectionOffset") ?
                           parameters->GetParameter<double>("ConnectionOffset") :
                       parameters->IsParameter("RelativeConnectionOffset") ?
                           parameters->GetParameter<double>("RelativeConnectionOffset") * get_setup_hold_time() :
                       HLS_D->has_parameter("RelativeConnectionOffset") ?
                           HLS_D->get_parameter<double>("RelativeConnectionOffset") * get_setup_hold_time() :
                       HLS_D->has_parameter("ConnectionOffset") ? HLS_D->get_parameter<double>("ConnectionOffset") :
                                                                  NUM_CST_allocation_default_connection_offset;

   output_DSP_connection_time =
       parameters->IsParameter("OutputDSPConnectionRatio") ?
           parameters->GetParameter<double>("OutputDSPConnectionRatio") * get_setup_hold_time() :
       HLS_D->has_parameter("OutputDSPConnectionRatio") ?
           HLS_D->get_parameter<double>("OutputDSPConnectionRatio") * get_setup_hold_time() :
           NUM_CST_allocation_default_output_DSP_connection_ratio * get_setup_hold_time();
   output_carry_connection_time =
       parameters->IsParameter("OutputCarryConnectionRatio") ?
           parameters->GetParameter<double>("OutputCarryConnectionRatio") * get_setup_hold_time() :
       HLS_D->has_parameter("OutputCarryConnectionRatio") ?
           HLS_D->get_parameter<double>("OutputCarryConnectionRatio") * get_setup_hold_time() :
           NUM_CST_allocation_default_output_carry_connection_ratio * get_setup_hold_time();
   fanout_coefficient = parameters->IsParameter("FanOutCoefficient") ?
                            parameters->GetParameter<double>("FanOutCoefficient") :
                            NUM_CST_allocation_default_fanout_coefficent;
   max_fanout_size = parameters->IsParameter("MaxFanOutSize") ? parameters->GetParameter<size_t>("MaxFanOutSize") :
                                                                NUM_CST_allocation_default_max_fanout_size;
   DSPs_margin =
       HLS_D->has_parameter("DSPs_margin") && parameters->getOption<double>(OPT_DSP_margin_combinational) == 1.0 ?
           HLS_D->get_parameter<double>("DSPs_margin") :
           parameters->getOption<double>(OPT_DSP_margin_combinational);
   DSPs_margin_stage =
       HLS_D->has_parameter("DSPs_margin_stage") && parameters->getOption<double>(OPT_DSP_margin_pipelined) == 1.0 ?
           HLS_D->get_parameter<double>("DSPs_margin_stage") :
           parameters->getOption<double>(OPT_DSP_margin_pipelined);
   DSP_allocation_coefficient = HLS_D->has_parameter("DSP_allocation_coefficient") &&
                                        parameters->getOption<double>(OPT_DSP_allocation_coefficient) == 1.0 ?
                                    HLS_D->get_parameter<double>("DSP_allocation_coefficient") :
                                    parameters->getOption<double>(OPT_DSP_allocation_coefficient);
   minimumSlack = std::numeric_limits<double>::max();
   n_complex_operations = 0;
   mux_timing_db = InitializeMuxDB(AllocationInformationConstRef(this, null_deleter())).first;
   mux_area_db = InitializeMuxDB(AllocationInformationConstRef(this, null_deleter())).second;
   DSP_x_db = std::get<0>(InitializeDSPDB(AllocationInformationConstRef(this, null_deleter())));
   DSP_y_db = std::get<1>(InitializeDSPDB(AllocationInformationConstRef(this, null_deleter())));
}

void AllocationInformation::Clear()
{
   HLSIR::Clear();
   op_graph = OpGraphConstRef();
   HLS_C = HLS_constraintsConstRef();
   HLS_D = HLS_deviceConstRef();
   behavioral_helper = BehavioralHelperConstRef();
   Rmem = memoryConstRef();
   TreeM = tree_managerConstRef();

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
   memory_units_sizes.clear();
   vars_to_memory_units.clear();
   precomputed_pipeline_unit.clear();
   single_bool_test_cond_expr_units.clear();
   simple_pointer_plus_expr.clear();
   vars_to_memory_units.clear();
   precomputed_pipeline_unit.clear();
   single_bool_test_cond_expr_units.clear();
   simple_pointer_plus_expr.clear();
   ssa_roots.clear();
   ssa_bb_versions.clear();
   ssa_cond_exprs.clear();
   cond_expr_bb_versions.clear();
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
   const auto statement_bb_index = GetPointerS<const gimple_node>(TreeM->GetTreeNode(statement_index))->bb_index;
   const auto tn = TreeM->GetTreeNode(statement_index);
   const bool is_carry = [&]() -> bool {
      if(tn->get_kind() != gimple_assign_K)
      {
         return false;
      }
      const auto op1_kind = GetPointerS<const gimple_assign>(tn)->op1->get_kind();
      if(op1_kind == plus_expr_K || op1_kind == minus_expr_K || op1_kind == ternary_plus_expr_K ||
         op1_kind == ternary_pm_expr_K || op1_kind == ternary_mp_expr_K || op1_kind == ternary_mm_expr_K ||
         op1_kind == eq_expr_K || op1_kind == ne_expr_K || op1_kind == gt_expr_K || op1_kind == ge_expr_K ||
         op1_kind == lt_expr_K || op1_kind == le_expr_K || op1_kind == pointer_plus_expr_K)
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
             GetPointerS<const gimple_node>(TreeM->GetTreeNode(zero_distance_operation))->bb_index;
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
                        "-->Considering " + STR(TreeM->GetTreeNode(current_tn_index)));
         const auto current_ga = GetPointer<const gimple_assign>(TreeM->GetTreeNode(current_tn_index));
         if(!current_ga)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not continuing since not gimple_assign ");
            continue;
         }
         const auto current_sn = GetPointer<const ssa_name>(current_ga->op0);
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
                        "<--Considered " + STR(TreeM->GetTreeNode(current_tn_index)));
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
   os << "is_single_bool_test_cond_expr: " << (is_single_bool_test_cond_expr ? "T" : "F") << "\n";
   os << "is_simple_pointer_plus_expr: " << (is_single_bool_test_cond_expr ? "T" : "F") << "\n";
}
