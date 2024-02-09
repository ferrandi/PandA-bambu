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
 * @file reg_binding.cpp
 * @brief Class implementation of the register binding data structure.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "reg_binding.hpp"
#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "function_behavior.hpp"
#include "generic_obj.hpp"
#include "hls.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "liveness.hpp"
#include "omp_functions.hpp"
#include "reg_binding_cs.hpp"
#include "register_obj.hpp"
#include "state_transition_graph.hpp"
#include "state_transition_graph_manager.hpp"
#include "storage_value_information.hpp"
#include "structural_manager.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include <list>
#include <utility>

std::string reg_binding::reset_type;

reg_binding::reg_binding(const hlsRef& HLS_, const HLS_managerRef HLSMgr_)
    : debug(HLS_->debug_level),
      used_regs(0),
      HLS(HLS_),
      HLSMgr(HLSMgr_),
      all_regs_without_enable(false),
      FB(HLSMgr->CGetFunctionBehavior(HLS->functionId))
{
   if(reset_type.empty())
   {
      reset_type = HLSMgr->get_parameter()->getOption<std::string>(OPT_reset_type);
   }
}

reg_binding::~reg_binding() = default;

reg_bindingRef reg_binding::create_reg_binding(const hlsRef& HLS, const HLS_managerRef HLSMgr_)
{
   if(HLS->Param->isOption(OPT_context_switch))
   {
      auto omp_functions = GetPointer<OmpFunctions>(HLSMgr_->Rfuns);
      bool found = false;
      if(HLSMgr_->is_reading_writing_function(HLS->functionId) &&
         omp_functions->kernel_functions.find(HLS->functionId) != omp_functions->kernel_functions.end())
      {
         found = true;
      }
      if(HLSMgr_->is_reading_writing_function(HLS->functionId) &&
         omp_functions->parallelized_functions.find(HLS->functionId) != omp_functions->parallelized_functions.end())
      {
         found = true;
      }
      if(omp_functions->atomic_functions.find(HLS->functionId) != omp_functions->atomic_functions.end())
      {
         found = true;
      }
      if(found)
      {
         return reg_bindingRef(new reg_binding_cs(HLS, HLSMgr_));
      }
   }
   return reg_bindingRef(new reg_binding(HLS, HLSMgr_));
}

void reg_binding::print_el(const_iterator& it) const
{
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, HLS->output_level,
                  "---Storage Value: " + STR(it->first) + " for variable " +
                      FB->CGetBehavioralHelper()->PrintVariable(
                          HLS->storage_value_information->get_variable_index(it->first).first) +
                      " step " + STR(HLS->storage_value_information->get_variable_index(it->first).second) +
                      " stored into register " + it->second->get_string());
}

CustomOrderedSet<std::pair<unsigned int, unsigned int>> reg_binding::get_vars(const unsigned int& r) const
{
   CustomOrderedSet<std::pair<unsigned int, unsigned int>> vars;
   THROW_ASSERT(reg2storage_values.count(r) && reg2storage_values.at(r).size(),
                "at least a storage value has to be mapped on register r");

   for(const auto rs : reg2storage_values.at(r))
   {
      vars.insert(HLS->storage_value_information->get_variable_index(rs));
   }
   return vars;
}

unsigned long long reg_binding::compute_bitsize(unsigned int r)
{
   auto reg_vars = get_vars(r);
   auto max_bits = 0ull;
   for(auto reg_var : reg_vars)
   {
      structural_type_descriptor node_type0(reg_var.first, FB->CGetBehavioralHelper());
      auto node_size = STD_GET_SIZE(&node_type0);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, HLS->debug_level,
                    "- Analyzing node " + STR(reg_var.first) + "(" + STR(reg_var.second) + "), whose type is " +
                        node_type0.get_name() + " (size: " + STR(node_type0.size) +
                        ", vector_size: " + STR(node_type0.vector_size) + ")");
      max_bits = std::max(max_bits, node_size);
   }
   bitsize_map[r] = max_bits;
   return max_bits;
}

unsigned long long reg_binding::get_bitsize(unsigned int r) const
{
   THROW_ASSERT(bitsize_map.count(r), "register bitsize not computed");
   return bitsize_map.at(r);
}

void reg_binding::specialise_reg(structural_objectRef& reg, unsigned int r)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, HLS->debug_level, "Specializing " + reg->get_path() + ":");
   const auto reg_m = GetPointerS<module>(reg);
   const auto& in_type = reg_m->get_in_port(0)->get_typeRef();
   const auto& out_type = reg_m->get_out_port(0)->get_typeRef();
   const auto bits = compute_bitsize(r);
   const auto max_bits = std::max({bits, STD_GET_SIZE(in_type), STD_GET_SIZE(out_type)});
   const auto offset = static_cast<unsigned int>(reg_m->get_in_port(0U)->get_id() == CLOCK_PORT_NAME) +
                       static_cast<unsigned int>(reg_m->get_in_port(1U)->get_id() == RESET_PORT_NAME);

   if(STD_GET_SIZE(in_type) < max_bits)
   {
      reg_m->get_in_port(offset)->type_resize(max_bits); // in1
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, HLS->debug_level,
                    "- " + reg_m->get_in_port(offset)->get_path() + " -> " + in_type->get_name() +
                        " (size: " + STR(in_type->size) + ", vector_size: " + STR(in_type->vector_size) + ")");
   }
   if(STD_GET_SIZE(out_type) < max_bits)
   {
      reg_m->get_out_port(0)->type_resize(max_bits); // out1
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, HLS->debug_level,
                    "- " + reg_m->get_out_port(0)->get_path() + " -> " + out_type->get_name() +
                        " (size: " + STR(out_type->size) + ", vector_size: " + STR(out_type->vector_size) + ")");
   }
}

void reg_binding::compute_is_without_enable()
{
   std::map<std::pair<unsigned int, unsigned int>, unsigned int> n_in;
   std::map<std::pair<unsigned int, unsigned int>, unsigned int> n_out;
   for(const auto v : HLS->Rliv->get_support())
   {
      const auto dummy_offset = HLS->Rliv->is_a_dummy_state(v) ? 1U : 0U;
      const auto& live_in = HLS->Rliv->get_live_in(v);
      for(const auto& li : live_in)
      {
         if(n_in.find(li) == n_in.end())
         {
            n_in[li] = 1U + dummy_offset;
         }
         else
         {
            n_in[li] = n_in[li] + 1U + dummy_offset;
         }
      }
      const auto& live_out = HLS->Rliv->get_live_out(v);
      for(const auto& lo : live_out)
      {
         if(!n_out.count(lo))
         {
            n_out[lo] = 1 + dummy_offset;
            if(live_in.count(lo))
            {
               n_out[lo] = 2 + dummy_offset;
            }
         }
         else
         {
            n_out[lo] = n_out[lo] + 1 + dummy_offset;
         }
      }
   }

   for(auto i = 0U; i < get_used_regs(); i++)
   {
      const auto all_woe = [&]() {
         const auto& store_vars_set = get_vars(i);
         for(const auto& sv : store_vars_set)
         {
            if(n_in.find(sv) == n_in.end() || n_in.find(sv)->second != 1 || n_out.find(sv) == n_out.end() ||
               n_out.find(sv)->second != 1)
            {
               return false;
            }
         }
         return true;
      }();
      if(all_woe)
      {
         is_without_enable.insert(i);
      }
   }
}

void reg_binding::bind(unsigned int sv, unsigned int index)
{
   reverse_map[sv] = index;
   if(!unique_table.count(index))
   {
      unique_table[index] = generic_objRef(new register_obj(index));
   }
   auto i = this->find(sv);
   if(i == this->end())
   {
      this->insert(std::make_pair(sv, unique_table.at(index)));
   }
   else
   {
      i->second = unique_table.at(index);
   }
   reg2storage_values[index].insert(sv);
}

const register_obj& reg_binding::operator[](unsigned int v)
{
   THROW_ASSERT(count(v), "variable not preset");
   return *GetPointerS<register_obj>(at(v));
}

void reg_binding::add_to_SM(structural_objectRef clock_port, structural_objectRef reset_port)
{
   const auto& SM = HLS->datapath;
   const auto& circuit = SM->get_circ();

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug, "reg_binding::add_registers - Start");

   compute_is_without_enable();
   /// define boolean type for command signals
   all_regs_without_enable = get_used_regs() != 0;
   for(auto i = 0U; i < get_used_regs(); ++i)
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug, "Allocating register number: " + STR(i));
      generic_objRef regis = get(i);
      auto name = regis->get_string();
      const auto curr_is_is_without_enable = is_without_enable.count(i);
      all_regs_without_enable = all_regs_without_enable && curr_is_is_without_enable;
      const auto register_type_name = GetRegisterFUName(i);
      const auto library = HLS->HLS_D->get_technology_manager()->get_library(register_type_name);
      auto reg_mod = SM->add_module_from_technology_library(name, register_type_name, library, circuit,
                                                            HLS->HLS_D->get_technology_manager());
      specialise_reg(reg_mod, i);
      auto port_ck = reg_mod->find_member(CLOCK_PORT_NAME, port_o_K, reg_mod);
      THROW_ASSERT(port_ck, "Clock port missing from register.");
      SM->add_connection(clock_port, port_ck);
      auto port_rst = reg_mod->find_member(RESET_PORT_NAME, port_o_K, reg_mod);
      if(port_rst)
      {
         SM->add_connection(reset_port, port_rst);
      }
      regis->set_structural_obj(reg_mod);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug, "Register " + STR(i) + " successfully allocated");
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug, "reg_binding::add_registers - End");
   if(HLS->output_level >= OUTPUT_LEVEL_MINIMUM)
   {
      auto number_ff = 0ull;
      for(auto r = 0U; r < get_used_regs(); r++)
      {
         number_ff += get_bitsize(r);
      }
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, HLS->output_level,
                     "---Total number of flip-flops in function " + FB->CGetBehavioralHelper()->get_function_name() +
                         ": " + STR(number_ff));
   }
   if(all_regs_without_enable)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, HLS->output_level,
                     "---All registers are without enable: function pipelining may come for free");
   }
}

std::string reg_binding::GetRegisterFUName(unsigned int i)
{
   if(is_without_enable.count(i))
   {
      return register_STD;
   }
   else if(reset_type == "no")
   {
      return register_SE;
   }
   else if(reset_type == "sync")
   {
      return register_SRSE;
   }
   return register_SARSE;
}
