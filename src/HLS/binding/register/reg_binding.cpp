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
#include "generic_obj.hpp"
#include "register_obj.hpp"

#include "behavioral_helper.hpp"
#include "function_behavior.hpp"

#include "FPGA_device.hpp"
#include "Parameter.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"
#include "liveness.hpp"
#include "omp_functions.hpp"
#include "reg_binding_cs.hpp"

#include "boost/lexical_cast.hpp"
#include "structural_manager.hpp"
#include "technology_manager.hpp"

/// HLS/binding/storage_value_information
#include "storage_value_information.hpp"

/// STL includes
#include "custom_set.hpp"
#include <list>
#include <utility>

/// technology/physical_library include
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "technology_node.hpp"

reg_binding::reg_binding(const hlsRef& HLS_, const HLS_managerRef HLSMgr_) : debug(HLS_->debug_level), used_regs(0), HLS(HLS_), HLSMgr(HLSMgr_), all_regs_without_enable(false)
{
}

reg_binding::~reg_binding() = default;

reg_bindingRef reg_binding::create_reg_binding(const hlsRef& HLS, const HLS_managerRef HLSMgr_)
{
   if(HLS->Param->isOption(OPT_context_switch))
   {
      auto omp_functions = GetPointer<OmpFunctions>(HLSMgr_->Rfuns);
      bool found = false;
      if(HLSMgr_->is_reading_writing_function(HLS->functionId) && omp_functions->kernel_functions.find(HLS->functionId) != omp_functions->kernel_functions.end())
         found = true;
      if(HLSMgr_->is_reading_writing_function(HLS->functionId) && omp_functions->parallelized_functions.find(HLS->functionId) != omp_functions->parallelized_functions.end())
         found = true;
      if(omp_functions->atomic_functions.find(HLS->functionId) != omp_functions->atomic_functions.end())
         found = true;
      if(found)
         return reg_bindingRef(new reg_binding_cs(HLS, HLSMgr_));
      else
         return reg_bindingRef(new reg_binding(HLS, HLSMgr_));
   }
   else
      return reg_bindingRef(new reg_binding(HLS, HLSMgr_));
}

void reg_binding::print_el(const_iterator& it) const
{
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, HLS->output_level,
                  "---Storage Value: " + STR(it->first) + " for variable " + HLSMgr->CGetFunctionBehavior(HLS->functionId)->CGetBehavioralHelper()->PrintVariable(HLS->storage_value_information->get_variable_index(it->first)) + " stored into register " +
                      it->second->get_string());
}

CustomOrderedSet<unsigned int> reg_binding::get_vars(const unsigned int& r) const
{
   CustomOrderedSet<unsigned int> vars;
   THROW_ASSERT(reg2storage_values.find(r) != reg2storage_values.end() && !reg2storage_values.find(r)->second.empty(), "at least a storage value has to be mapped on register r");

   auto rs_it_end = reg2storage_values.find(r)->second.end();
   for(auto rs_it = reg2storage_values.find(r)->second.begin(); rs_it != rs_it_end; ++rs_it)
      vars.insert(HLS->storage_value_information->get_variable_index(*rs_it));
   return vars;
}

unsigned int reg_binding::compute_bitsize(unsigned int r)
{
   CustomOrderedSet<unsigned int> reg_vars = get_vars(r);
   unsigned int max_bits = 0;
   for(unsigned int reg_var : reg_vars)
   {
      structural_type_descriptorRef node_type0 = structural_type_descriptorRef(new structural_type_descriptor(reg_var, HLSMgr->CGetFunctionBehavior(HLS->functionId)->CGetBehavioralHelper()));
      unsigned int node_size = STD_GET_SIZE(node_type0);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, HLS->debug_level, "- Analyzing node " + STR(reg_var) + ", whose type is " + node_type0->get_name() + " (size: " + STR(node_type0->size) + ", vector_size: " + STR(node_type0->vector_size) + ")");
      max_bits = max_bits < node_size ? node_size : max_bits;
   }
   bitsize_map[r] = max_bits;
   return max_bits;
}

unsigned int reg_binding::get_bitsize(unsigned int r) const
{
   THROW_ASSERT(bitsize_map.find(r) != bitsize_map.end(), "register bitsize not computed");
   return bitsize_map.find(r)->second;
}

void reg_binding::specialise_reg(structural_objectRef& reg, unsigned int r)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, HLS->debug_level, "Specializing " + reg->get_path() + ":");
   const structural_type_descriptorRef& in_type = GetPointer<module>(reg)->get_in_port(0)->get_typeRef();
   const structural_type_descriptorRef& out_type = GetPointer<module>(reg)->get_out_port(0)->get_typeRef();
   unsigned int max_bits = STD_GET_SIZE(in_type);
   max_bits = max_bits < STD_GET_SIZE(out_type) ? STD_GET_SIZE(out_type) : max_bits;
   unsigned int bits = compute_bitsize(r);
   max_bits = max_bits < bits ? bits : max_bits;
   unsigned int offset = 0;
   if(GetPointer<module>(reg)->get_in_port(0)->get_id() == CLOCK_PORT_NAME)
   {
      if(GetPointer<module>(reg)->get_in_port(1)->get_id() == RESET_PORT_NAME)
         offset = 2;
      else
         offset = 1;
   }
   if(STD_GET_SIZE(in_type) < max_bits)
   {
      GetPointer<module>(reg)->get_in_port(offset)->type_resize(max_bits); // in1
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, HLS->debug_level, "- " + GetPointer<module>(reg)->get_in_port(0)->get_path() + " -> " + in_type->get_name() + " (size: " + STR(in_type->size) + ", vector_size: " + STR(in_type->vector_size) + ")");
   }
   if(STD_GET_SIZE(out_type) < max_bits)
   {
      GetPointer<module>(reg)->get_out_port(0)->type_resize(max_bits); // out1
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, HLS->debug_level, "- " + GetPointer<module>(reg)->get_out_port(0)->get_path() + " -> " + out_type->get_name() + " (size: " + STR(out_type->size) + ", vector_size: " + STR(out_type->vector_size) + ")");
   }
}

void reg_binding::compute_is_without_enable()
{
   std::map<unsigned int, unsigned int> n_in;
   std::map<unsigned int, unsigned int> n_out;
   const std::list<vertex>& support_set = HLS->Rliv->get_support();
   const std::list<vertex>::const_iterator ss_it_end = support_set.end();
   for(auto ss_it = support_set.begin(); ss_it != ss_it_end; ++ss_it)
   {
      vertex v = *ss_it;
      unsigned int dummy_offset = HLS->Rliv->is_a_dummy_state(v) ? 1 : 0;
      const CustomOrderedSet<unsigned int>& LI = HLS->Rliv->get_live_in(v);
      const CustomOrderedSet<unsigned int>::const_iterator li_it_end = LI.end();
      for(auto li_it = LI.begin(); li_it != li_it_end; ++li_it)
      {
         if(n_in.find(*li_it) == n_in.end())
            n_in[*li_it] = 1 + dummy_offset;
         else
            n_in[*li_it] = n_in[*li_it] + 1 + dummy_offset;
      }
      const CustomOrderedSet<unsigned int>& LO = HLS->Rliv->get_live_out(v);
      const CustomOrderedSet<unsigned int>::const_iterator lo_it_end = LO.end();
      for(auto lo_it = LO.begin(); lo_it != lo_it_end; ++lo_it)
      {
         if(n_out.find(*lo_it) == n_out.end())
         {
            n_out[*lo_it] = 1 + dummy_offset;
            if(LI.find(*lo_it) != LI.end())
               n_out[*lo_it] = 2 + dummy_offset;
         }
         else
            n_out[*lo_it] = n_out[*lo_it] + 1 + dummy_offset;
      }
   }

   for(unsigned int i = 0; i < get_used_regs(); i++)
   {
      const CustomOrderedSet<unsigned int>& store_vars_set = get_vars(i);
      const CustomOrderedSet<unsigned int>::const_iterator svs_it_end = store_vars_set.end();
      bool all_woe = true;
      for(auto svs_it = store_vars_set.begin(); svs_it != svs_it_end && all_woe; ++svs_it)
      {
         if(n_in.find(*svs_it) == n_in.end() || n_out.find(*svs_it) == n_out.end())
            all_woe = false;
         if(n_in.find(*svs_it)->second != 1 || n_out.find(*svs_it)->second != 1)
            all_woe = false;
      }
      if(all_woe)
      {
         // std::cerr << "register STD " << i << std::endl;
         is_without_enable.insert(i);
      }
   }
}

void reg_binding::bind(unsigned int sv, unsigned int index)
{
   reverse_map[sv] = index;
   if(unique_table.find(index) == unique_table.end())
      unique_table[index] = generic_objRef(new register_obj(index));
   auto i = this->find(sv);
   if(i == this->end())
      this->insert(std::make_pair(sv, unique_table[index]));
   else
      i->second = unique_table[index];
   reg2storage_values[index].insert(sv);
}

const register_obj& reg_binding::operator[](unsigned int v)
{
   THROW_ASSERT(this->find(v) != this->end(), "variable not preset");
   return *GetPointer<register_obj>(this->find(v)->second);
}
void reg_binding::add_to_SM(structural_objectRef clock_port, structural_objectRef reset_port)
{
   const structural_managerRef& SM = HLS->datapath;

   const structural_objectRef& circuit = SM->get_circ();

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug, "reg_binding::add_registers - Start");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug, "Number of registers: " + std::to_string(get_used_regs()));

   compute_is_without_enable();
   /// define boolean type for command signals
   all_regs_without_enable = get_used_regs() != 0;
   for(unsigned int i = 0; i < get_used_regs(); i++)
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug, "Allocating register number: " + std::to_string(i));
      generic_objRef regis = get(i);
      std::string name = regis->get_string();
      bool curr_is_is_without_enable = is_without_enable.find(i) != is_without_enable.end();
      all_regs_without_enable = all_regs_without_enable && curr_is_is_without_enable;
      std::string register_type_name = CalculateRegisterName(i);
      std::string library = HLS->HLS_T->get_technology_manager()->get_library(register_type_name);
      structural_objectRef reg_mod = SM->add_module_from_technology_library(name, register_type_name, library, circuit, HLS->HLS_T->get_technology_manager());
      this->specialise_reg(reg_mod, i);
      structural_objectRef port_ck = reg_mod->find_member(CLOCK_PORT_NAME, port_o_K, reg_mod);
      SM->add_connection(clock_port, port_ck);
      structural_objectRef port_rst = reg_mod->find_member(RESET_PORT_NAME, port_o_K, reg_mod);
      if(port_rst != nullptr)
         SM->add_connection(reset_port, port_rst);
      regis->set_structural_obj(reg_mod);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug, "Register " + boost::lexical_cast<std::string>(i) + " successfully allocated");
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug, "reg_binding::add_registers - End");
   if(HLS->output_level >= OUTPUT_LEVEL_MINIMUM)
   {
      unsigned int number_ff = 0;
      for(unsigned int r = 0; r < get_used_regs(); r++)
      {
         number_ff += get_bitsize(r);
      }
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, HLS->output_level, "---Total number of flip-flops in function " + HLSMgr->CGetFunctionBehavior(HLS->functionId)->CGetBehavioralHelper()->get_function_name() + ": " + STR(number_ff));
   }
   if(all_regs_without_enable)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, HLS->output_level, "---All registers are without enable: function pipelining may come for free");
   }
}

std::string reg_binding::CalculateRegisterName(unsigned int i)
{
   std::string register_type_name;
   std::string synch_reset = HLS->Param->getOption<std::string>(OPT_sync_reset);
   if(is_without_enable.find(i) != is_without_enable.end())
      register_type_name = register_STD;
   else if(synch_reset == "no")
      register_type_name = register_SE;
   else if(synch_reset == "sync")
      register_type_name = register_SRSE;
   else
      register_type_name = register_SARSE;
   return register_type_name;
}
