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
 * @file fu_binding.cpp
 * @brief Class implementation of the functional-unit binding data structure.
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "fu_binding.hpp"
#include "fu_binding_cs.hpp"
#include "funit_obj.hpp"

#include "call_graph_manager.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"
#include "memory.hpp"
#include "memory_allocation.hpp"
#include "memory_cs.hpp"
#include "memory_symbol.hpp"
#include "omp_functions.hpp"
#include "reg_binding.hpp"

#include "functions.hpp"

#include "structural_manager.hpp"
#include "structural_objects.hpp"

#include "constant_strings.hpp"

#include "utility.hpp"

#include "behavioral_helper.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#include "conn_binding.hpp"

#include "library_manager.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"

#include <boost/algorithm/string/replace.hpp>

#include <fstream>

///. include
#include "Parameter.hpp"

/// design_flows/backend/ToHDL include
#include "language_writer.hpp"

/// HLS/module_allocation
#include "allocation_information.hpp"

/// STD include
#include <limits>
#include <string>

/// STL includes
#include "custom_set.hpp"
#include <algorithm>
#include <utility>
#include <vector>

/// utility include
#include "string_manipulation.hpp" // for GET_CLASS

const unsigned int fu_binding::UNKNOWN = std::numeric_limits<unsigned int>::max();

fu_binding::fu_binding(const HLS_managerConstRef _HLSMgr, const unsigned int _function_id, const ParameterConstRef _parameters)
    : allocation_information(_HLSMgr->get_HLS(_function_id)->allocation_information),
      TreeM(_HLSMgr->get_tree_manager()),
      op_graph(_HLSMgr->CGetFunctionBehavior(_function_id)->CGetOpGraph(FunctionBehavior::DFG)),
      parameters(_parameters),
      debug_level(_parameters->get_class_debug_level(GET_CLASS(*this))),
      has_resource_sharing_p(true)
{
}

fu_binding::fu_binding(const fu_binding& original)
    : unique_table(original.unique_table),
      operations(original.operations),
      op_binding(original.op_binding),
      allocation_information(original.allocation_information),
      TreeM(original.TreeM),
      op_graph(original.op_graph),
      parameters(original.parameters),
      debug_level(parameters->get_class_debug_level(GET_CLASS(*this))),
      has_resource_sharing_p(original.has_resource_sharing_p)
{
}

fu_binding::~fu_binding() = default;

fu_bindingRef fu_binding::create_fu_binding(const HLS_managerConstRef _HLSMgr, const unsigned int _function_id, const ParameterConstRef _parameters)
{
   if(_parameters->isOption(OPT_context_switch))
   {
      auto omp_functions = GetPointer<OmpFunctions>(_HLSMgr->Rfuns);
      bool found = false;
      if(omp_functions->kernel_functions.find(_function_id) != omp_functions->kernel_functions.end())
         found = true;
      if(omp_functions->hierarchical_functions.find(_function_id) != omp_functions->hierarchical_functions.end())
         found = true;
      if(omp_functions->parallelized_functions.find(_function_id) != omp_functions->parallelized_functions.end())
         found = true;
      if(omp_functions->atomic_functions.find(_function_id) != omp_functions->atomic_functions.end())
         found = true;
      if(found)
         return fu_bindingRef(new fu_binding_cs(_HLSMgr, _function_id, _parameters));
      else
         return fu_bindingRef(new fu_binding(_HLSMgr, _function_id, _parameters));
   }
   else
      return fu_bindingRef(new fu_binding(_HLSMgr, _function_id, _parameters));
}

void fu_binding::bind(const vertex& v, unsigned int unit, unsigned int index)
{
   if(unique_table.count(std::make_pair(unit, index)) == 0)
      unique_table[std::make_pair(unit, index)] = generic_objRef(new funit_obj(allocation_information->get_string_name(unit) + "_i" + STR(index), unit, index));
   const unsigned int statement_index = op_graph->CGetOpNodeInfo(v)->GetNodeId();
   op_binding[statement_index] = unique_table[std::make_pair(unit, index)];
   auto key = std::make_pair(unit, index);
   if(operations.find(key) == operations.end())
   {
      operations.insert(std::pair<std::pair<unsigned int, unsigned int>, OpVertexSet>(key, OpVertexSet(op_graph)));
   }
   operations.at(key).insert(v);
   if(index != INFINITE_UINT)
      update_allocation(unit, index + 1);
}

OpVertexSet fu_binding::get_operations(unsigned int unit, unsigned int index) const
{
   if(operations.find(std::make_pair(unit, index)) == operations.end())
   {
      return OpVertexSet(op_graph);
   }
   return operations.find(std::make_pair(unit, index))->second;
}

const funit_obj& fu_binding::operator[](const vertex& v)
{
   const auto statement_index = op_graph->CGetOpNodeInfo(v)->GetNodeId();
   THROW_ASSERT(op_binding.find(statement_index) != op_binding.end(), "vertex not preset");
   return *(GetPointer<funit_obj>(op_binding.find(statement_index)->second));
}

bool fu_binding::is_assigned(const vertex& v) const
{
   const auto statement_index = op_graph->CGetOpNodeInfo(v)->GetNodeId();
   return is_assigned(statement_index);
}

bool fu_binding::is_assigned(const unsigned int statement_index) const
{
   return op_binding.find(statement_index) != op_binding.end();
}

std::list<unsigned int> fu_binding::get_allocation_list() const
{
   std::list<unsigned int> allocation_list;
   for(const auto alloc : allocation_map)
   {
      if(alloc.second > 0)
         allocation_list.push_back(alloc.first);
   }
   return allocation_list;
}

void fu_binding::update_allocation(unsigned int unit, unsigned int number)
{
   if(number > allocation_map[unit])
      allocation_map[unit] = number;
}

unsigned int fu_binding::get_assign(vertex const& v) const
{
   const auto statement_index = op_graph->CGetOpNodeInfo(v)->GetNodeId();
   return get_assign(statement_index);
}

unsigned int fu_binding::get_assign(const unsigned int statement_index) const
{
   THROW_ASSERT(op_binding.find(statement_index) != op_binding.end(), "Operation " + TreeM->get_tree_node_const(statement_index)->ToString() + " not assigned");
   THROW_ASSERT(GetPointer<funit_obj>(op_binding.find(statement_index)->second), "");
   return GetPointer<funit_obj>(op_binding.find(statement_index)->second)->get_fu();
}

std::string fu_binding::get_fu_name(vertex const& v) const
{
   return allocation_information->get_string_name(get_assign(v));
}

unsigned int fu_binding::get_index(vertex const& v) const
{
   const auto statement_index = op_graph->CGetOpNodeInfo(v)->GetNodeId();
   return GetPointer<funit_obj>(op_binding.find(statement_index)->second)->get_index();
}

structural_objectRef fu_binding::add_gate(const HLS_managerRef HLSMgr, const hlsRef HLS, const technology_nodeRef fu, const std::string& name, const OpVertexSet& ops, structural_objectRef clock_port, structural_objectRef reset_port)
{
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(HLS->functionId);
   const OpGraphConstRef data = FB->CGetOpGraph(FunctionBehavior::CFG);
   const structural_managerRef SM = HLS->datapath;
   const structural_objectRef circuit = SM->get_circ();
   structural_objectRef curr_gate = structural_objectRef(new component_o(debug_level, circuit));
   /// creating structural_manager starting from technology_node
   structural_objectRef curr_lib_instance;
   if(GetPointer<functional_unit>(fu)->fu_template_name == "")
   {
      curr_lib_instance = GetPointer<functional_unit>(fu)->CM->get_circ();
   }
   else
   {
      std::string template_name = GetPointer<functional_unit>(fu)->fu_template_name;
      std::string library_name = HLS->HLS_T->get_technology_manager()->get_library(template_name);
      curr_lib_instance = GetPointer<functional_unit>(GetPointer<functional_unit_template>(HLS->HLS_T->get_technology_manager()->get_fu(template_name, library_name))->FU)->CM->get_circ();
   }
   THROW_ASSERT(curr_lib_instance, "structural description not provided: check the library given. Component: " + GetPointer<functional_unit>(fu)->functional_unit_name);

   curr_lib_instance->copy(curr_gate);
   if(ops.size() == 1)
   {
      curr_gate->set_id("fu_" + GET_NAME(data, *(ops.begin())));
   }
   else
   {
      THROW_ASSERT(not name.empty(), "cannot name the added gate if the name is empty");
      curr_gate->set_id(name);
   }

   /// connecting clock and reset ports, if any
   structural_objectRef port_ck = curr_gate->find_member(CLOCK_PORT_NAME, port_o_K, curr_gate);
   if(port_ck)
      SM->add_connection(clock_port, port_ck);
   structural_objectRef port_rst = curr_gate->find_member(RESET_PORT_NAME, port_o_K, curr_gate);
   if(port_rst)
      SM->add_connection(reset_port, port_rst);
   GetPointer<module>(circuit)->add_internal_object(curr_gate);

   return curr_gate;
}

void fu_binding::kill_proxy_memory_units(std::map<unsigned int, unsigned int>& memory_units, structural_objectRef curr_gate, std::map<unsigned int, std::list<structural_objectRef>>& var_call_sites_rel,
                                         std::map<unsigned int, unsigned int>& reverse_memory_units)
{
   /// compute the set of killing vars
   OrderedSetStd<unsigned int> killing_vars;
   const std::map<unsigned int, unsigned int>::const_iterator it_mu_end = memory_units.end();
   for(std::map<unsigned int, unsigned int>::const_iterator it_mu = memory_units.begin(); it_mu != it_mu_end; ++it_mu)
   {
      killing_vars.insert(it_mu->second);
      reverse_memory_units[it_mu->second] = it_mu->first;
   }
   for(auto kv : killing_vars)
   {
      structural_objectRef port_proxy_in1 = curr_gate->find_member("proxy_in1_" + STR(kv), port_o_K, curr_gate);
      if(port_proxy_in1)
      {
         var_call_sites_rel[kv].push_back(curr_gate);
         structural_objectRef port_proxy_in2 = curr_gate->find_member("proxy_in2_" + STR(kv), port_o_K, curr_gate);
         structural_objectRef port_proxy_in3 = curr_gate->find_member("proxy_in3_" + STR(kv), port_o_K, curr_gate);
         structural_objectRef port_proxy_out1 = curr_gate->find_member("proxy_out1_" + STR(kv), port_o_K, curr_gate);
         structural_objectRef port_proxy_sel_LOAD = curr_gate->find_member("proxy_sel_LOAD_" + STR(kv), port_o_K, curr_gate);
         structural_objectRef port_proxy_sel_STORE = curr_gate->find_member("proxy_sel_STORE_" + STR(kv), port_o_K, curr_gate);
         GetPointer<port_o>(port_proxy_in1)->set_is_memory(false);
         GetPointer<port_o>(port_proxy_in2)->set_is_memory(false);
         GetPointer<port_o>(port_proxy_in3)->set_is_memory(false);
         GetPointer<port_o>(port_proxy_out1)->set_is_memory(false);
         GetPointer<port_o>(port_proxy_sel_LOAD)->set_is_memory(false);
         GetPointer<port_o>(port_proxy_sel_STORE)->set_is_memory(false);
      }
   }
}

void fu_binding::kill_proxy_function_units(std::map<unsigned int, std::string>& wrapped_units, structural_objectRef curr_gate, std::map<std::string, std::list<structural_objectRef>>& fun_call_sites_rel,
                                           std::map<std::string, unsigned int>& reverse_wrapped_units)
{
   /// compute the set of killing functions
   OrderedSetStd<std::string> killing_funs;
   const std::map<unsigned int, std::string>::const_iterator it_mu_end = wrapped_units.end();
   for(std::map<unsigned int, std::string>::const_iterator it_mu = wrapped_units.begin(); it_mu != it_mu_end; ++it_mu)
   {
      killing_funs.insert(it_mu->second);
      reverse_wrapped_units[it_mu->second] = it_mu->first;
   }
   for(auto fun_name : killing_funs)
   {
      auto inPortSize = static_cast<unsigned int>(GetPointer<module>(curr_gate)->get_in_port_size());
      for(unsigned int currentPort = 0; currentPort < inPortSize; ++currentPort)
      {
         structural_objectRef curr_port = GetPointer<module>(curr_gate)->get_in_port(currentPort);
         if(!GetPointer<port_o>(curr_port)->get_is_memory())
            continue;
         std::string port_name = curr_port->get_id();
         if(boost::algorithm::starts_with(port_name, PROXY_PREFIX))
         {
            size_t found = port_name.rfind(fun_name);
            if(found != std::string::npos && found + fun_name.size() == port_name.size())
            {
               GetPointer<port_o>(curr_port)->set_is_memory(false);
               if(std::find(fun_call_sites_rel[fun_name].begin(), fun_call_sites_rel[fun_name].end(), curr_gate) == fun_call_sites_rel[fun_name].end())
                  fun_call_sites_rel[fun_name].push_back(curr_gate);
            }
         }
      }
      auto outPortSize = static_cast<unsigned int>(GetPointer<module>(curr_gate)->get_out_port_size());
      for(unsigned int currentPort = 0; currentPort < outPortSize; ++currentPort)
      {
         structural_objectRef curr_port = GetPointer<module>(curr_gate)->get_out_port(currentPort);
         if(!GetPointer<port_o>(curr_port)->get_is_memory())
            continue;
         std::string port_name = curr_port->get_id();
         size_t found = port_name.rfind(fun_name);
         if(found != std::string::npos && found + fun_name.size() == port_name.size())
         {
            GetPointer<port_o>(curr_port)->set_is_memory(false);
            if(std::find(fun_call_sites_rel[fun_name].begin(), fun_call_sites_rel[fun_name].end(), curr_gate) == fun_call_sites_rel[fun_name].end())
               fun_call_sites_rel[fun_name].push_back(curr_gate);
         }
      }
   }
}

void fu_binding::manage_killing_memory_proxies(std::map<unsigned int, structural_objectRef>& mem_obj, std::map<unsigned int, unsigned int>& reverse_memory_units, std::map<unsigned int, std::list<structural_objectRef>>& var_call_sites_rel,
                                               const structural_managerRef SM, const hlsRef HLS, unsigned int& _unique_id)
{
   const structural_objectRef circuit = SM->get_circ();
   const auto vcsr_it_end = var_call_sites_rel.end();
   for(auto vcsr_it = var_call_sites_rel.begin(); vcsr_it != vcsr_it_end; ++vcsr_it)
   {
      unsigned int var = vcsr_it->first;
      THROW_ASSERT(reverse_memory_units.find(var) != reverse_memory_units.end(), "var not found");
      unsigned int storage_fu_unit_id = reverse_memory_units.find(var)->second;
      THROW_ASSERT(mem_obj.find(storage_fu_unit_id) != mem_obj.end(), "storage_fu_unit not found: " + STR(storage_fu_unit_id));
      structural_objectRef storage_fu_unit = mem_obj.find(storage_fu_unit_id)->second;
      structural_objectRef storage_port_proxy_out1 = storage_fu_unit->find_member("proxy_out1", port_o_K, storage_fu_unit);
      THROW_ASSERT(storage_port_proxy_out1, "missing proxy_out1 port");
      structural_objectRef storage_port_proxy_out1_sign;
      const auto proxied_unit_it_end = vcsr_it->second.end();
      std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter> to_be_merged;
      structural_objectRef port_in1 = storage_fu_unit->find_member("proxy_in1", port_o_K, storage_fu_unit);
      THROW_ASSERT(port_in1, "missing port proxy_in1");
      structural_objectRef port_in2 = storage_fu_unit->find_member("proxy_in2", port_o_K, storage_fu_unit);
      THROW_ASSERT(port_in2, "missing port proxy_in2");
      structural_objectRef port_in3 = storage_fu_unit->find_member("proxy_in3", port_o_K, storage_fu_unit);
      THROW_ASSERT(port_in3, "missing port proxy_in3");
      structural_objectRef port_sel_LOAD = storage_fu_unit->find_member("proxy_sel_LOAD", port_o_K, storage_fu_unit);
      THROW_ASSERT(port_sel_LOAD, "missing port proxy_sel_LOAD");
      structural_objectRef port_sel_STORE = storage_fu_unit->find_member("proxy_sel_STORE", port_o_K, storage_fu_unit);
      THROW_ASSERT(port_sel_STORE, "missing port proxy_sel_STORE");

      for(auto proxied_unit_it = vcsr_it->second.begin(); proxied_unit_it != proxied_unit_it_end; ++proxied_unit_it)
      {
         structural_objectRef proxied_unit = *proxied_unit_it;
         structural_objectRef proxied_port_proxy_out1 = proxied_unit->find_member("proxy_out1_" + STR(var), port_o_K, proxied_unit);
         THROW_ASSERT(proxied_port_proxy_out1, "missing proxied proxy_out1 port");
         if(!storage_port_proxy_out1_sign)
         {
            if(storage_port_proxy_out1->get_kind() == port_vector_o_K)
               storage_port_proxy_out1_sign = SM->add_sign_vector(storage_port_proxy_out1->get_id() + "_" + STR(var), GetPointer<port_o>(storage_port_proxy_out1)->get_ports_size(), circuit, storage_port_proxy_out1->get_typeRef());
            else
               storage_port_proxy_out1_sign = SM->add_sign(storage_port_proxy_out1->get_id() + "_" + STR(var), circuit, storage_port_proxy_out1->get_typeRef());
            SM->add_connection(storage_port_proxy_out1_sign, storage_port_proxy_out1);
         }
         SM->add_connection(storage_port_proxy_out1_sign, proxied_port_proxy_out1);

         structural_objectRef port_proxy_in1 = proxied_unit->find_member("proxy_in1_" + STR(var), port_o_K, proxied_unit);
         if(std::find(to_be_merged[port_in1].begin(), to_be_merged[port_in1].end(), port_proxy_in1) == to_be_merged[port_in1].end())
            to_be_merged[port_in1].push_back(port_proxy_in1);
         structural_objectRef port_proxy_in2 = proxied_unit->find_member("proxy_in2_" + STR(var), port_o_K, proxied_unit);
         if(std::find(to_be_merged[port_in2].begin(), to_be_merged[port_in2].end(), port_proxy_in2) == to_be_merged[port_in2].end())
            to_be_merged[port_in2].push_back(port_proxy_in2);
         structural_objectRef port_proxy_in3 = proxied_unit->find_member("proxy_in3_" + STR(var), port_o_K, proxied_unit);
         if(std::find(to_be_merged[port_in3].begin(), to_be_merged[port_in3].end(), port_proxy_in3) == to_be_merged[port_in3].end())
            to_be_merged[port_in3].push_back(port_proxy_in3);
         structural_objectRef port_proxy_sel_LOAD = proxied_unit->find_member("proxy_sel_LOAD_" + STR(var), port_o_K, proxied_unit);
         if(std::find(to_be_merged[port_sel_LOAD].begin(), to_be_merged[port_sel_LOAD].end(), port_proxy_sel_LOAD) == to_be_merged[port_sel_LOAD].end())
            to_be_merged[port_sel_LOAD].push_back(port_proxy_sel_LOAD);
         structural_objectRef port_proxy_sel_STORE = proxied_unit->find_member("proxy_sel_STORE_" + STR(var), port_o_K, proxied_unit);
         if(std::find(to_be_merged[port_sel_STORE].begin(), to_be_merged[port_sel_STORE].end(), port_proxy_sel_STORE) == to_be_merged[port_sel_STORE].end())
            to_be_merged[port_sel_STORE].push_back(port_proxy_sel_STORE);
      }
      join_merge_split(SM, HLS, to_be_merged, circuit, _unique_id);
   }
}

void fu_binding::manage_killing_function_proxies(std::map<unsigned int, structural_objectRef>& fun_obj, std::map<std::string, unsigned int>& reverse_function_units, std::map<std::string, std::list<structural_objectRef>>& fun_call_sites_rel,
                                                 const structural_managerRef SM, const hlsRef HLS, unsigned int& _unique_id)
{
   const structural_objectRef circuit = SM->get_circ();
   const auto fcsr_it_end = fun_call_sites_rel.end();
   for(auto fcsr_it = fun_call_sites_rel.begin(); fcsr_it != fcsr_it_end; ++fcsr_it)
   {
      std::string fun = fcsr_it->first;
      THROW_ASSERT(reverse_function_units.find(fun) != reverse_function_units.end(), "fun not found");
      unsigned int wrapped_fu_unit_id = reverse_function_units.find(fun)->second;
      THROW_ASSERT(fun_obj.find(wrapped_fu_unit_id) != fun_obj.end(), "wrapped_fu_unit not found");
      structural_objectRef wrapped_fu_unit = fun_obj.find(wrapped_fu_unit_id)->second;
      std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter> to_be_merged;
      const auto proxied_unit_it_end = fcsr_it->second.end();

      auto inPortSize = static_cast<unsigned int>(GetPointer<module>(wrapped_fu_unit)->get_in_port_size());
      for(unsigned int currentPort = 0; currentPort < inPortSize; ++currentPort)
      {
         structural_objectRef curr_port = GetPointer<module>(wrapped_fu_unit)->get_in_port(currentPort);
         std::string port_name = curr_port->get_id();
         if(boost::algorithm::starts_with(port_name, PROXY_PREFIX))
         {
            for(auto proxied_unit_it = fcsr_it->second.begin(); proxied_unit_it != proxied_unit_it_end; ++proxied_unit_it)
            {
               structural_objectRef proxied_unit = *proxied_unit_it;
               structural_objectRef port_proxy_in_i = proxied_unit->find_member(port_name + "_" + fun, port_o_K, proxied_unit);
               if(port_proxy_in_i && std::find(to_be_merged[curr_port].begin(), to_be_merged[curr_port].end(), port_proxy_in_i) == to_be_merged[curr_port].end())
                  to_be_merged[curr_port].push_back(port_proxy_in_i);
            }
         }
      }
      auto outPortSize = static_cast<unsigned int>(GetPointer<module>(wrapped_fu_unit)->get_out_port_size());
      for(unsigned int currentPort = 0; currentPort < outPortSize; ++currentPort)
      {
         structural_objectRef wrapped_port_proxy_out_i = GetPointer<module>(wrapped_fu_unit)->get_out_port(currentPort);
         std::string port_name = wrapped_port_proxy_out_i->get_id();
         if(boost::algorithm::starts_with(port_name, PROXY_PREFIX))
         {
            structural_objectRef wrapped_port_proxy_out_i_sign;
            for(auto proxied_unit_it = fcsr_it->second.begin(); proxied_unit_it != proxied_unit_it_end; ++proxied_unit_it)
            {
               structural_objectRef proxied_unit = *proxied_unit_it;
               structural_objectRef proxied_port_proxy_out_i = proxied_unit->find_member(port_name + "_" + fun, port_o_K, proxied_unit);
               if(proxied_port_proxy_out_i)
               {
                  if(!wrapped_port_proxy_out_i_sign)
                  {
                     if(wrapped_port_proxy_out_i->get_kind() == port_vector_o_K)
                        wrapped_port_proxy_out_i_sign = SM->add_sign_vector(wrapped_port_proxy_out_i->get_id() + "_" + fun, GetPointer<port_o>(wrapped_port_proxy_out_i)->get_ports_size(), circuit, wrapped_port_proxy_out_i->get_typeRef());
                     else
                        wrapped_port_proxy_out_i_sign = SM->add_sign(wrapped_port_proxy_out_i->get_id() + "_" + fun, circuit, wrapped_port_proxy_out_i->get_typeRef());
                     SM->add_connection(wrapped_port_proxy_out_i_sign, wrapped_port_proxy_out_i);
                  }
                  SM->add_connection(wrapped_port_proxy_out_i_sign, proxied_port_proxy_out_i);
               }
            }
         }
      }
      join_merge_split(SM, HLS, to_be_merged, circuit, _unique_id);
   }
}

void fu_binding::add_to_SM(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef clock_port, structural_objectRef reset_port)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding functional units to circuit");
   const structural_managerRef SM = HLS->datapath;

   /// unique id identifier
   unsigned int unique_id = 0;

   /// initialize resource sharing to false
   has_resource_sharing_p = !HLS->Rreg->is_all_regs_without_enable(); // it assumes that HLS->Rreg->add_to_SM is called first and then HLS->Rfu->add_to_SM

   const structural_objectRef circuit = SM->get_circ();

   std::list<structural_objectRef> memory_modules;

   /// add the MEMCPY_STD component when parameters has to be copied into the local store
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(HLS->functionId);
   const auto& function_parameters = FB->CGetBehavioralHelper()->get_parameters();
   unsigned int sign_id = 0;
   structural_objectRef start_port = GetPointer<module>(circuit)->find_member(START_PORT_NAME, port_o_K, circuit);
   structural_objectRef done_port = GetPointer<module>(circuit)->find_member(DONE_PORT_NAME, port_o_K, circuit);
   structural_objectRef in_chain = start_port;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding parameter ports");
   for(const auto function_parameter : function_parameters)
   {
      if(HLSMgr->Rmem->is_parm_decl_copied(function_parameter) && !HLSMgr->Rmem->is_parm_decl_stored(function_parameter))
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Managing parameter copy: " + STR(function_parameter));
         const technology_nodeRef fu_lib_unit = HLS->HLS_T->get_technology_manager()->get_fu(MEMCPY_STD, WORK_LIBRARY);
         THROW_ASSERT(fu_lib_unit, "functional unit not available: check the library given. Component: " + std::string(MEMCPY_STD));
         structural_objectRef curr_gate = add_gate(HLSMgr, HLS, fu_lib_unit, "parameter_manager_" + STR(function_parameter), OpVertexSet(op_graph), clock_port, reset_port);
         conn_binding::direction_type direction = conn_binding::IN;
         generic_objRef port_obj = HLS->Rconn->get_port(function_parameter, direction);
         structural_objectRef in_par = port_obj->get_out_sign();
         structural_objectRef src = GetPointer<module>(curr_gate)->find_member("src", port_o_K, curr_gate);
         SM->add_connection(in_par, src);
         structural_objectRef dest = GetPointer<module>(curr_gate)->find_member("dest", port_o_K, curr_gate);

         structural_objectRef const_obj = SM->add_module_from_technology_library("memcpy_dest_" + HLSMgr->Rmem->get_symbol(function_parameter, HLS->functionId)->get_symbol_name(), CONSTANT_STD, LIBRARY_STD, circuit, HLS->HLS_T->get_technology_manager());
         const_obj->SetParameter("value", HLSMgr->Rmem->get_symbol(function_parameter, HLS->functionId)->get_symbol_name());
         std::string name = "out_const_memcpy_dest_" + HLSMgr->Rmem->get_symbol(function_parameter, HLS->functionId)->get_symbol_name();
         structural_objectRef dest_sign = SM->add_sign(name, circuit, dest->get_typeRef());
         structural_objectRef out_port = const_obj->find_member("out1", port_o_K, const_obj);
         // customize output port size
         out_port->type_resize(STD_GET_SIZE(dest->get_typeRef()));
         SM->add_connection(dest_sign, out_port);
         SM->add_connection(dest, dest_sign);
         structural_objectRef n = GetPointer<module>(curr_gate)->find_member("len", port_o_K, curr_gate);
         structural_objectRef n_obj = SM->add_constant("constant_len_" + STR(function_parameter), circuit, n->get_typeRef(), STR(tree_helper::size(TreeM, tree_helper::get_type_index(TreeM, function_parameter)) / 8));
         SM->add_connection(n, n_obj);
         THROW_ASSERT(in_chain, "missing in chain element");
         structural_objectRef start_obj = GetPointer<module>(curr_gate)->find_member(START_PORT_NAME, port_o_K, curr_gate);

         if(HLS->registered_inputs && in_chain == start_port)
         {
            technology_nodeRef delay_unit;
            std::string synch_reset = parameters->getOption<std::string>(OPT_sync_reset);
            if(synch_reset == "sync")
               delay_unit = HLS->HLS_T->get_technology_manager()->get_fu(flipflop_SR, LIBRARY_STD);
            else
               delay_unit = HLS->HLS_T->get_technology_manager()->get_fu(flipflop_AR, LIBRARY_STD);
            THROW_ASSERT(delay_unit, "");
            structural_objectRef delay_gate = add_gate(HLSMgr, HLS, delay_unit, "start_delayed_" + STR(function_parameter), OpVertexSet(op_graph), clock_port, reset_port);
            structural_objectRef sign = SM->add_sign(START_PORT_NAME + STR("_") + STR(sign_id), circuit, start_obj->get_typeRef());
            ++sign_id;
            SM->add_connection(sign, in_chain);
            SM->add_connection(sign, GetPointer<module>(delay_gate)->get_in_port(2));
         }

         if(in_chain == start_port)
            SM->add_connection(in_chain, start_obj);
         else
         {
            structural_objectRef sign = SM->add_sign(START_PORT_NAME + STR("_") + STR(sign_id), circuit, in_chain->get_typeRef());
            ++sign_id;
            SM->add_connection(sign, in_chain);
            SM->add_connection(sign, start_obj);
         }
         in_chain = GetPointer<module>(curr_gate)->find_member(DONE_PORT_NAME, port_o_K, curr_gate);
         manage_module_ports(HLSMgr, HLS, SM, curr_gate, 0);
         memory_modules.push_back(curr_gate);
      }
      else if(HLSMgr->Rmem->is_parm_decl_stored(function_parameter))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Managing parameter initialization: " + STR(function_parameter));
         unsigned int bus_data_bitsize = HLSMgr->Rmem->get_bus_data_bitsize();
         unsigned int bus_addr_bitsize = HLSMgr->get_address_bitsize();
         unsigned int bus_size_bitsize = HLSMgr->Rmem->get_bus_size_bitsize();
         unsigned int bus_tag_bitsize = 0;
         if(HLS->Param->isOption(OPT_context_switch))
            bus_tag_bitsize = GetPointer<memory_cs>(HLSMgr->Rmem)->get_bus_tag_bitsize();
         structural_objectRef curr_gate;
         bool is_multiport;
         size_t max_n_ports = HLS->Param->isOption(OPT_channels_number) ? parameters->getOption<unsigned int>(OPT_channels_number) : 0;
         if(parameters->getOption<MemoryAllocation_ChannelsType>(OPT_channels_type) == MemoryAllocation_ChannelsType::MEM_ACC_NN)
         {
            const technology_nodeRef fu_lib_unit = HLS->HLS_T->get_technology_manager()->get_fu(MEMSTORE_STDN, LIBRARY_STD_FU);
            THROW_ASSERT(fu_lib_unit, "functional unit not available: check the library given. Component: " + std::string(MEMSTORE_STDN));
            curr_gate = add_gate(HLSMgr, HLS, fu_lib_unit, "parameter_manager_" + STR(function_parameter), OpVertexSet(op_graph), clock_port, reset_port);
            is_multiport = true;
         }
         else
         {
            const technology_nodeRef fu_lib_unit = HLS->HLS_T->get_technology_manager()->get_fu(MEMSTORE_STD, LIBRARY_STD_FU);
            THROW_ASSERT(fu_lib_unit, "functional unit not available: check the library given. Component: " + std::string(MEMSTORE_STD));
            curr_gate = add_gate(HLSMgr, HLS, fu_lib_unit, "parameter_manager_" + STR(function_parameter), OpVertexSet(op_graph), clock_port, reset_port);
            is_multiport = false;
         }
         conn_binding::direction_type direction = conn_binding::IN;
         generic_objRef port_obj = HLS->Rconn->get_port(function_parameter, direction);
         structural_objectRef in_par = port_obj->get_out_sign();
         structural_objectRef data = GetPointer<module>(curr_gate)->find_member("data", port_o_K, curr_gate);
         data->type_resize(STD_GET_SIZE(in_par->get_typeRef()));
         SM->add_connection(in_par, data);

         structural_objectRef size = GetPointer<module>(curr_gate)->find_member("size", port_o_K, curr_gate);
         size->type_resize(STD_GET_SIZE(in_par->get_typeRef()));
         structural_objectRef size_const_obj = SM->add_module_from_technology_library("size_par_" + HLSMgr->Rmem->get_symbol(function_parameter, HLS->functionId)->get_symbol_name(), CONSTANT_STD, LIBRARY_STD, circuit, HLS->HLS_T->get_technology_manager());
         const std::string parameter_value = (static_cast<HDLWriter_Language>(parameters->getOption<unsigned int>(OPT_writer_language)) == HDLWriter_Language::VHDL) ?
                                                 std::string("\"") + NumberToBinaryString(STD_GET_SIZE(in_par->get_typeRef()), STD_GET_SIZE(in_par->get_typeRef())) + std::string("\"") :
                                                 STR(STD_GET_SIZE(in_par->get_typeRef()));
         size_const_obj->SetParameter("value", parameter_value);
         std::string size_name = "out_const_size_par_" + HLSMgr->Rmem->get_symbol(function_parameter, HLS->functionId)->get_symbol_name();
         structural_objectRef size_sign = SM->add_sign(size_name, circuit, size->get_typeRef());
         structural_objectRef size_out_port = size_const_obj->find_member("out1", port_o_K, size_const_obj);
         // customize output port size
         size_out_port->type_resize(STD_GET_SIZE(in_par->get_typeRef()));
         SM->add_connection(size_sign, size_out_port);
         SM->add_connection(size, size_sign);

         structural_objectRef addr = GetPointer<module>(curr_gate)->find_member("addr", port_o_K, curr_gate);
         addr->type_resize(bus_addr_bitsize);
         structural_objectRef const_obj = SM->add_module_from_technology_library("addr_par_" + HLSMgr->Rmem->get_symbol(function_parameter, HLS->functionId)->get_symbol_name(), CONSTANT_STD, LIBRARY_STD, circuit, HLS->HLS_T->get_technology_manager());
         const_obj->SetParameter("value", HLSMgr->Rmem->get_symbol(function_parameter, HLS->functionId)->get_symbol_name());
         std::string name = "out_const_addr_par_" + HLSMgr->Rmem->get_symbol(function_parameter, HLS->functionId)->get_symbol_name();
         structural_objectRef addr_sign = SM->add_sign(name, circuit, addr->get_typeRef());
         structural_objectRef out_port = const_obj->find_member("out1", port_o_K, const_obj);
         // customize output port size
         out_port->type_resize(bus_addr_bitsize);
         SM->add_connection(addr_sign, out_port);
         SM->add_connection(addr, addr_sign);

         THROW_ASSERT(in_chain, "missing in chain element");
         structural_objectRef start_obj = GetPointer<module>(curr_gate)->find_member(START_PORT_NAME, port_o_K, curr_gate);
         if(HLS->registered_inputs && in_chain == start_port)
         {
            technology_nodeRef delay_unit;
            std::string synch_reset = parameters->getOption<std::string>(OPT_sync_reset);
            if(synch_reset == "sync")
               delay_unit = HLS->HLS_T->get_technology_manager()->get_fu(flipflop_SR, LIBRARY_STD);
            else
               delay_unit = HLS->HLS_T->get_technology_manager()->get_fu(flipflop_AR, LIBRARY_STD);
            structural_objectRef delay_gate = add_gate(HLSMgr, HLS, delay_unit, "start_delayed_" + STR(function_parameter), OpVertexSet(op_graph), clock_port, reset_port);
            structural_objectRef sign = SM->add_sign(START_PORT_NAME + STR("_") + STR(sign_id), circuit, start_obj->get_typeRef());
            ++sign_id;
            SM->add_connection(sign, in_chain);
            SM->add_connection(sign, GetPointer<module>(delay_gate)->get_in_port(2));
            in_chain = GetPointer<module>(delay_gate)->get_out_port(0);
         }

         if(in_chain == start_port)
         {
            SM->add_connection(in_chain, start_obj);
         }
         else
         {
            structural_objectRef sign = SM->add_sign(START_PORT_NAME + STR("_") + STR(sign_id), circuit, in_chain->get_typeRef());
            ++sign_id;
            SM->add_connection(sign, in_chain);
            SM->add_connection(sign, start_obj);
         }
         in_chain = GetPointer<module>(curr_gate)->find_member(DONE_PORT_NAME, port_o_K, curr_gate);
         /// component specialization
         for(unsigned int i = 0; i < GetPointer<module>(curr_gate)->get_in_port_size(); i++)
         {
            structural_objectRef port = GetPointer<module>(curr_gate)->get_in_port(i);
            if(is_multiport && port->get_kind() == port_vector_o_K && GetPointer<port_o>(port)->get_ports_size() == 0)
               GetPointer<port_o>(port)->add_n_ports(static_cast<unsigned int>(max_n_ports), port);
            if(GetPointer<port_o>(port)->get_is_data_bus() || GetPointer<port_o>(port)->get_is_addr_bus() || GetPointer<port_o>(port)->get_is_size_bus() || GetPointer<port_o>(port)->get_is_tag_bus())
               port_o::resize_busport(bus_size_bitsize, bus_addr_bitsize, bus_data_bitsize, bus_tag_bitsize, port);
         }
         for(unsigned int i = 0; i < GetPointer<module>(curr_gate)->get_out_port_size(); i++)
         {
            structural_objectRef port = GetPointer<module>(curr_gate)->get_out_port(i);
            if(is_multiport && port->get_kind() == port_vector_o_K && GetPointer<port_o>(port)->get_ports_size() == 0)
               GetPointer<port_o>(port)->add_n_ports(static_cast<unsigned int>(max_n_ports), port);
            if(GetPointer<port_o>(port)->get_is_data_bus() || GetPointer<port_o>(port)->get_is_addr_bus() || GetPointer<port_o>(port)->get_is_size_bus() || GetPointer<port_o>(port)->get_is_tag_bus())
               port_o::resize_busport(bus_size_bitsize, bus_addr_bitsize, bus_data_bitsize, bus_tag_bitsize, port);
         }
         manage_module_ports(HLSMgr, HLS, SM, curr_gate, 0);
         memory_modules.push_back(curr_gate);
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added parameter ports");
   if(in_chain)
      SM->add_connection(in_chain, done_port);
   else
   {
      THROW_ASSERT(!done_port, "done port not connected");
   }

   if(HLS->control_flow_checker)
   {
      structural_objectRef controller_flow_circuit = HLS->control_flow_checker->get_circ();
      structural_objectRef curr_gate = structural_objectRef(new component_o(debug_level, circuit));
      controller_flow_circuit->copy(curr_gate);
      curr_gate->set_id("ControlFlowChecker_i");
      GetPointer<module>(circuit)->add_internal_object(curr_gate);
      structural_objectRef controller_flow_clock = curr_gate->find_member(CLOCK_PORT_NAME, port_o_K, curr_gate);
      SM->add_connection(controller_flow_clock, clock_port);
      structural_objectRef controller_flow_reset = curr_gate->find_member(RESET_PORT_NAME, port_o_K, curr_gate);
      SM->add_connection(controller_flow_reset, reset_port);
      structural_objectRef controller_flow_start = curr_gate->find_member(START_PORT_NAME, port_o_K, curr_gate);
      structural_objectRef start_CFC = SM->add_port(START_PORT_NAME_CFC, port_o::IN, circuit, structural_type_descriptorRef(new structural_type_descriptor("bool", 0)));
      SM->add_connection(start_CFC, controller_flow_start);
      structural_objectRef controller_flow_done = curr_gate->find_member(DONE_PORT_NAME, port_o_K, curr_gate);
      structural_objectRef done_CFC = SM->add_port(DONE_PORT_NAME_CFC, port_o::IN, circuit, structural_type_descriptorRef(new structural_type_descriptor("bool", 0)));
      SM->add_connection(done_CFC, controller_flow_done);
      structural_objectRef controller_flow_present_state = curr_gate->find_member(PRESENT_STATE_PORT_NAME, port_o_K, curr_gate);
      structural_objectRef controller_present_state = SM->add_port(PRESENT_STATE_PORT_NAME, port_o::IN, circuit, controller_flow_present_state->get_typeRef());
      SM->add_connection(controller_present_state, controller_flow_present_state);
      structural_objectRef controller_flow_next_state = curr_gate->find_member(NEXT_STATE_PORT_NAME, port_o_K, curr_gate);
      structural_objectRef controller_next_state = SM->add_port(NEXT_STATE_PORT_NAME, port_o::IN, circuit, controller_flow_next_state->get_typeRef());
      SM->add_connection(controller_next_state, controller_flow_next_state);
      memory_modules.push_back(curr_gate);
   }

   std::map<unsigned int, unsigned int> memory_units = allocation_information->get_memory_units();
   std::map<unsigned int, structural_objectRef> mem_obj;
   for(const auto& m : memory_units)
   {
      const unsigned int fu_type_id = m.first;
      unsigned int var = m.second;
      std::string name;
      std::string fun_unit_name = allocation_information->get_fu_name(fu_type_id).first;
      if(allocation_information->is_direct_access_memory_unit(fu_type_id))
      {
         name = "array_" + STR(var);
      }
      else
      {
         THROW_ERROR("Unit not currently supported: " + fun_unit_name);
      }
      const technology_nodeRef fu_lib_unit = allocation_information->get_fu(fu_type_id);
      THROW_ASSERT(fu_lib_unit, "functional unit not available: check the library given. Component: " + fun_unit_name);

      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Memory Unit: " + allocation_information->get_string_name(fu_type_id) + " for variable: " + HLSMgr->CGetFunctionBehavior(HLS->functionId)->CGetBehavioralHelper()->PrintVariable(var));
      std::string base_address = HLSMgr->Rmem->get_symbol(var, HLS->functionId)->get_symbol_name();
      unsigned int rangesize = HLSMgr->Rmem->get_rangesize(var);
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  - base address: " + STR(base_address));
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  - range size: " + STR(rangesize));
      unsigned int n_channels = allocation_information->get_number_channels(fu_type_id);
      unsigned int total_allocated = get_number(fu_type_id);
      unsigned int n_iterations = std::max(1u, total_allocated);
      for(unsigned int num = 0; num < n_iterations; num = num + n_channels)
      {
         OpVertexSet operations_set(op_graph);
         for(unsigned int channel_index = 0; channel_index < n_channels && (num + channel_index < total_allocated); ++channel_index)
         {
            auto key = std::make_pair(fu_type_id, num + channel_index);
            if(operations.find(key) != operations.end())
            {
               auto opset = operations.at(key);
               operations_set.insert(opset.begin(), opset.end());
            }
         }
         has_resource_sharing_p = has_resource_sharing_p || (operations_set.size() > 1);
         structural_objectRef curr_gate = add_gate(HLSMgr, HLS, fu_lib_unit, name + "_" + STR(num / n_channels), OpVertexSet(op_graph), clock_port, reset_port);
         specialise_fu(HLSMgr, HLS, curr_gate, fu_type_id, operations_set, var);
         std::string memory_type = GetPointer<functional_unit>(fu_lib_unit)->memory_type;
         std::string channels_type = GetPointer<functional_unit>(fu_lib_unit)->channels_type;
         specialize_memory_unit(HLSMgr, HLS, curr_gate, var, base_address, rangesize, false, memory_type == MEMORY_TYPE_SYNCHRONOUS_UNALIGNED && (channels_type == CHANNELS_TYPE_MEM_ACC_N1 || channels_type == CHANNELS_TYPE_MEM_ACC_NN),
                                HLS->Param->isOption(OPT_sparse_memory) && parameters->getOption<bool>(OPT_sparse_memory),
                                memory_type == MEMORY_TYPE_SYNCHRONOUS_SDS || memory_type == MEMORY_TYPE_SYNCHRONOUS_SDS_BUS || memory_type == MEMORY_TYPE_ASYNCHRONOUS);
         check_parametrization(curr_gate);
         mem_obj[fu_type_id] = curr_gate;
         for(unsigned int channel_index = 0; (channel_index < n_channels) && ((num + channel_index) < total_allocated); ++channel_index)
         {
            generic_objRef module_obj = get(fu_type_id, num + channel_index);
            module_obj->set_structural_obj(curr_gate);
         }
         if(!HLSMgr->Rmem->is_private_memory(var))
         {
            manage_module_ports(HLSMgr, HLS, SM, curr_gate, 0);
            memory_modules.push_back(curr_gate);
         }
      }
   }

   std::map<unsigned int, unsigned int> reverse_memory_units;
   std::map<std::string, unsigned int> reverse_function_units;
   std::map<unsigned int, std::list<structural_objectRef>> var_call_sites_rel;
   std::map<std::string, std::list<structural_objectRef>> fun_call_sites_rel;
   std::map<unsigned int, structural_objectRef> fun_obj;
   std::map<unsigned int, std::string> wrapped_units = allocation_information->get_proxy_wrapped_units();
   for(auto wu = wrapped_units.begin(); wu != wrapped_units.end(); ++wu)
   {
      has_resource_sharing_p = true;
      std::string fun_unit_name = allocation_information->get_fu_name(wu->first).first;
      const technology_nodeRef fu_lib_unit = allocation_information->get_fu(wu->first);
      THROW_ASSERT(fu_lib_unit, "functional unit not available: check the library given. Component: " + fun_unit_name);
      structural_objectRef curr_gate = add_gate(HLSMgr, HLS, fu_lib_unit, wu->second + "_instance", OpVertexSet(op_graph), clock_port, reset_port);
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Wrapped Unit: " + allocation_information->get_string_name(wu->first));
      const OpVertexSet& mapped_operations = get_operations(wu->first, 0);
      specialise_fu(HLSMgr, HLS, curr_gate, wu->first, mapped_operations, 0);
      check_parametrization(curr_gate);
      fun_obj[wu->first] = curr_gate;
      kill_proxy_memory_units(memory_units, curr_gate, var_call_sites_rel, reverse_memory_units);
      kill_proxy_function_units(wrapped_units, curr_gate, fun_call_sites_rel, reverse_function_units);
      bool added_memory_element = manage_module_ports(HLSMgr, HLS, SM, curr_gate, 0);
      if(added_memory_element)
         memory_modules.push_back(curr_gate);
      /// propagate memory parameters if contained into the module to be instantiated
      memory::propagate_memory_parameters(curr_gate, HLS->datapath);
   }

   const std::map<unsigned int, unsigned int>& proxy_memory_units = allocation_information->get_proxy_memory_units();
   const std::map<unsigned int, std::string>& proxy_function_units = allocation_information->get_proxy_function_units();

   std::list<std::pair<structural_objectRef, unsigned int>> proxy_memory_units_to_be_renamed_back;
   std::list<std::pair<structural_objectRef, std::string>> proxy_function_units_to_be_renamed_back;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Specializing functional units");

   for(auto i : this->get_allocation_list())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Functional Unit: " + allocation_information->get_string_name(i));
      if(allocation_information->is_return(i))
      {
         structural_objectRef obj = circuit->find_member(RETURN_PORT_NAME, port_o_K, circuit);
         generic_objRef module_obj = get(i, 0);
         module_obj->set_structural_obj(obj);
      }

      if(!allocation_information->has_to_be_synthetized(i))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--It has not to be synthesized");
         continue;
      }
      bool is_multichannel = allocation_information->get_number_channels(i) > 1;

      for(unsigned int num = 0; num < get_number(i); num++)
      {
         generic_objRef module_obj = get(i, num);
         THROW_ASSERT(module_obj, "module missing: " + allocation_information->get_fu_name(i).first + " instance " + STR(num));
         std::string name = module_obj->get_string();

         structural_objectRef curr_gate;
         if(allocation_information->is_memory_unit(i))
         {
            continue;
         }
         else if(wrapped_units.find(i) != wrapped_units.end())
         {
            curr_gate = fun_obj[i];
         }
         else if(is_multichannel && (num % allocation_information->get_number_channels(i)) != 0)
         {
            unsigned int n_channels = allocation_information->get_number_channels(i);
            generic_objRef true_module_obj = get(i, (num / n_channels) * n_channels);
            curr_gate = true_module_obj->get_structural_obj();
            const OpVertexSet& mapped_operations = get_operations(i, num);
            has_resource_sharing_p = has_resource_sharing_p || (mapped_operations.size() > 1);
            const unsigned int ar_var = allocation_information->is_proxy_memory_unit(i) ? allocation_information->get_proxy_memory_var(i) : 0;
            specialise_fu(HLSMgr, HLS, curr_gate, i, mapped_operations, ar_var);
            module_obj->set_structural_obj(curr_gate);
         }
         else
         {
            const technology_nodeRef fu_lib_unit = allocation_information->get_fu(i);
            const OpVertexSet& mapped_operations = get_operations(i, num);
            THROW_ASSERT(fu_lib_unit, "functional unit not available: check the library given. Component: " + allocation_information->get_fu_name(i).first);
            curr_gate = add_gate(HLSMgr, HLS, fu_lib_unit, name, allocation_information->is_direct_proxy_memory_unit(i) or allocation_information->is_indirect_access_memory_unit(i) ? OpVertexSet(op_graph) : mapped_operations, clock_port, reset_port);
            has_resource_sharing_p = has_resource_sharing_p || (mapped_operations.size() > 1);
            std::string current_op;
            if(mapped_operations.size())
               current_op = tree_helper::normalized_ID(op_graph->CGetOpNodeInfo(*(mapped_operations.begin()))->GetOperation());
            if(current_op == BUILTIN_WAIT_CALL)
            {
               has_resource_sharing_p = true;
               const vertex site = *mapped_operations.begin();
               unsigned int vertex_node_id = op_graph->CGetOpNodeInfo(site)->GetNodeId();

               memory_symbolRef callSiteMemorySym = HLSMgr->Rmem->get_symbol(vertex_node_id, HLS->functionId);
               memory::add_memory_parameter(HLS->datapath, callSiteMemorySym->get_symbol_name(), STR(callSiteMemorySym->get_address()));
            }
            const unsigned int ar_var = allocation_information->is_proxy_memory_unit(i) ? allocation_information->get_proxy_memory_var(i) : 0;
            specialise_fu(HLSMgr, HLS, curr_gate, i, mapped_operations, ar_var);
            check_parametrization(curr_gate);
            if(proxy_memory_units.find(i) != proxy_memory_units.end())
            {
               proxy_memory_units_to_be_renamed_back.push_back(std::make_pair(curr_gate, proxy_memory_units.find(i)->second));
               structural_objectRef port_proxy_in1 = curr_gate->find_member("proxy_in1", port_o_K, curr_gate);
               structural_objectRef port_proxy_in2 = curr_gate->find_member("proxy_in2", port_o_K, curr_gate);
               structural_objectRef port_proxy_in3 = curr_gate->find_member("proxy_in3", port_o_K, curr_gate);
               structural_objectRef port_sel_LOAD = curr_gate->find_member("proxy_sel_LOAD", port_o_K, curr_gate);
               structural_objectRef port_sel_STORE = curr_gate->find_member("proxy_sel_STORE", port_o_K, curr_gate);
               structural_objectRef port_proxy_out1 = curr_gate->find_member("proxy_out1", port_o_K, curr_gate);
               /// rename proxy ports
               std::string var_name = "_" + STR(proxy_memory_units.find(i)->second);
               port_proxy_in1->set_id(port_proxy_in1->get_id() + var_name);
               port_proxy_in2->set_id(port_proxy_in2->get_id() + var_name);
               port_proxy_in3->set_id(port_proxy_in3->get_id() + var_name);
               port_sel_LOAD->set_id(port_sel_LOAD->get_id() + var_name);
               port_sel_STORE->set_id(port_sel_STORE->get_id() + var_name);
               port_proxy_out1->set_id(port_proxy_out1->get_id() + var_name);
            }
            kill_proxy_memory_units(memory_units, curr_gate, var_call_sites_rel, reverse_memory_units);

            if(proxy_function_units.find(i) != proxy_function_units.end())
            {
               std::string fun_name = "_" + STR(proxy_function_units.find(i)->second);
               proxy_function_units_to_be_renamed_back.push_back(std::make_pair(curr_gate, proxy_function_units.find(i)->second));
               auto inPortSize = static_cast<unsigned int>(GetPointer<module>(curr_gate)->get_in_port_size());
               for(unsigned int currentPort = 0; currentPort < inPortSize; ++currentPort)
               {
                  structural_objectRef curr_port = GetPointer<module>(curr_gate)->get_in_port(currentPort);
                  if(!GetPointer<port_o>(curr_port)->get_is_memory())
                     continue;
                  std::string port_name = curr_port->get_id();
                  if(boost::algorithm::starts_with(port_name, PROXY_PREFIX))
                  {
                     GetPointer<port_o>(curr_port)->set_id(port_name + fun_name);
                  }
               }
               auto outPortSize = static_cast<unsigned int>(GetPointer<module>(curr_gate)->get_out_port_size());
               for(unsigned int currentPort = 0; currentPort < outPortSize; ++currentPort)
               {
                  structural_objectRef curr_port = GetPointer<module>(curr_gate)->get_out_port(currentPort);
                  if(!GetPointer<port_o>(curr_port)->get_is_memory())
                     continue;
                  std::string port_name = curr_port->get_id();
                  if(boost::algorithm::starts_with(port_name, PROXY_PREFIX))
                  {
                     GetPointer<port_o>(curr_port)->set_id(port_name + fun_name);
                  }
               }
            }
            kill_proxy_function_units(wrapped_units, curr_gate, fun_call_sites_rel, reverse_function_units);

            bool added_memory_element = manage_module_ports(HLSMgr, HLS, SM, curr_gate, num);
            if(added_memory_element && std::find(memory_modules.begin(), memory_modules.end(), curr_gate) == memory_modules.end())
               memory_modules.push_back(curr_gate);

            /// propagate memory parameters if contained into the module to be instantiated
            memory::propagate_memory_parameters(curr_gate, HLS->datapath);
         }
         module_obj->set_structural_obj(curr_gate);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Specialized functional units");
   if(!has_resource_sharing_p)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, HLS->output_level, "---Resources are not shared in function " + HLSMgr->CGetFunctionBehavior(HLS->functionId)->CGetBehavioralHelper()->get_function_name() + ": function pipelining may come for free");
   }

   const auto cg_man = HLSMgr->CGetCallGraphManager();
   const auto top_function_ids = cg_man->GetRootFunctions();
   if(top_function_ids.find(HLS->functionId) != top_function_ids.end() and cg_man->ExistsAddressedFunction())
   {
      CustomOrderedSet<unsigned int> addressed_functions = cg_man->GetAddressedFunctions();

      structural_objectRef constBitZero = SM->add_module_from_technology_library("constBitZero", CONSTANT_STD, LIBRARY_STD, circuit, HLS->HLS_T->get_technology_manager());
      structural_objectRef signBitZero = SM->add_sign("bitZero", circuit, circuit->find_member(CLOCK_PORT_NAME, port_o_K, circuit)->get_typeRef());
      SM->add_connection(signBitZero, constBitZero->find_member("out1", port_o_K, constBitZero));

      for(const auto Itr : addressed_functions)
      {
         std::string FUName = tree_helper::name_function(TreeM, Itr);

         if(HLSMgr->Rfuns->is_a_proxied_function(FUName))
            continue;

         structural_objectRef FU = SM->add_module_from_technology_library(FUName + "_i0", FUName, WORK_LIBRARY, circuit, HLS->HLS_T->get_technology_manager());

         if(std::find(memory_modules.begin(), memory_modules.end(), FU) == memory_modules.end())
            memory_modules.push_back(FU);

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Considering additional top: " + FUName + "@" + STR(Itr));
         if(HLSMgr->Rfuns->has_proxied_shared_functions(Itr))
         {
            CustomOrderedSet<std::string> proxied_shared_functions = HLSMgr->Rfuns->get_proxied_shared_functions(Itr);
            for(auto name : proxied_shared_functions)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---  proxied shared function: " + name);
            }
            kill_proxy_function_units(wrapped_units, FU, fun_call_sites_rel, reverse_function_units);
         }

         SM->add_connection(FU->find_member(START_PORT_NAME, port_o_K, FU), signBitZero);

         SM->add_connection(FU->find_member(CLOCK_PORT_NAME, port_o_K, FU), circuit->find_member(CLOCK_PORT_NAME, port_o_K, circuit));

         SM->add_connection(FU->find_member(RESET_PORT_NAME, port_o_K, FU), circuit->find_member(RESET_PORT_NAME, port_o_K, circuit));

         for(const auto additional_parameter : HLSMgr->CGetFunctionBehavior(Itr)->CGetBehavioralHelper()->get_parameters())
         {
            std::string parameterName = HLSMgr->CGetFunctionBehavior(Itr)->CGetBehavioralHelper()->PrintVariable(additional_parameter);

            structural_objectRef parameterPort = FU->find_member(parameterName, port_o_K, FU);
            structural_objectRef constZeroParam = SM->add_module_from_technology_library("zeroParam_" + FUName + "_" + parameterName, CONSTANT_STD, LIBRARY_STD, circuit, HLS->HLS_T->get_technology_manager());
            structural_objectRef constZeroOutPort = constZeroParam->find_member("out1", port_o_K, constZeroParam);
            const std::string parameter_value =
                (static_cast<HDLWriter_Language>(parameters->getOption<unsigned int>(OPT_writer_language)) == HDLWriter_Language::VHDL) ? std::string("\"") + NumberToBinaryString(0, STD_GET_SIZE(parameterPort->get_typeRef())) + std::string("\"") : "0";
            constZeroParam->SetParameter("value", parameter_value);

            constZeroOutPort->type_resize(STD_GET_SIZE(parameterPort->get_typeRef()));

            structural_objectRef signBitZeroParam = SM->add_sign("signZeroParam_" + FUName + "_" + parameterName, SM->get_circ(), constZeroOutPort->get_typeRef());

            SM->add_connection(parameterPort, signBitZeroParam);
            SM->add_connection(constZeroOutPort, signBitZeroParam);
         }
         manage_module_ports(HLSMgr, HLS, SM, FU, 0);
         memory::propagate_memory_parameters(FU, HLS->datapath);
      }
   }
   if(parameters->IsParameter("chained-memory-modules") && parameters->GetParameter<int>("chained-memory-modules") == 1)
      manage_memory_ports_chained(SM, memory_modules, circuit);
   else
      manage_memory_ports_parallel_chained(HLSMgr, SM, memory_modules, circuit, HLS, unique_id);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Managed memory ports");

   /// rename back all the memory proxies ports
   for(const auto& pmutbrb : proxy_memory_units_to_be_renamed_back)
   {
      structural_objectRef curr_gate = pmutbrb.first;
      THROW_ASSERT(curr_gate, "missing structural object");
      std::string var_name = "_" + STR(pmutbrb.second);
      structural_objectRef port_proxy_in1 = curr_gate->find_member("proxy_in1" + var_name, port_o_K, curr_gate);
      structural_objectRef port_proxy_in2 = curr_gate->find_member("proxy_in2" + var_name, port_o_K, curr_gate);
      structural_objectRef port_proxy_in3 = curr_gate->find_member("proxy_in3" + var_name, port_o_K, curr_gate);
      structural_objectRef port_sel_LOAD = curr_gate->find_member("proxy_sel_LOAD" + var_name, port_o_K, curr_gate);
      structural_objectRef port_sel_STORE = curr_gate->find_member("proxy_sel_STORE" + var_name, port_o_K, curr_gate);
      structural_objectRef port_proxy_out1 = curr_gate->find_member("proxy_out1" + var_name, port_o_K, curr_gate);
      port_proxy_in1->set_id("proxy_in1");
      port_proxy_in2->set_id("proxy_in2");
      port_proxy_in3->set_id("proxy_in3");
      port_sel_LOAD->set_id("proxy_sel_LOAD");
      port_sel_STORE->set_id("proxy_sel_STORE");
      port_proxy_out1->set_id("proxy_out1");
   }
   HLS->Rfu->manage_killing_memory_proxies(mem_obj, reverse_memory_units, var_call_sites_rel, SM, HLS, unique_id);

   /// rename back all the function proxies ports
   for(const auto& pfutbrb : proxy_function_units_to_be_renamed_back)
   {
      structural_objectRef curr_gate = pfutbrb.first;
      THROW_ASSERT(curr_gate, "missing structural object");
      std::string fun_name = "_" + STR(pfutbrb.second);
      auto inPortSize = static_cast<unsigned int>(GetPointer<module>(curr_gate)->get_in_port_size());
      for(unsigned int currentPort = 0; currentPort < inPortSize; ++currentPort)
      {
         structural_objectRef curr_port = GetPointer<module>(curr_gate)->get_in_port(currentPort);
         std::string port_name = curr_port->get_id();
         if(boost::algorithm::starts_with(port_name, PROXY_PREFIX))
         {
            size_t found = port_name.rfind(fun_name);
            if(found != std::string::npos)
            {
               std::string orig_port_name = port_name.substr(0, found);
               GetPointer<port_o>(curr_port)->set_id(orig_port_name);
            }
         }
      }
      auto outPortSize = static_cast<unsigned int>(GetPointer<module>(curr_gate)->get_out_port_size());
      for(unsigned int currentPort = 0; currentPort < outPortSize; ++currentPort)
      {
         structural_objectRef curr_port = GetPointer<module>(curr_gate)->get_out_port(currentPort);
         std::string port_name = curr_port->get_id();
         if(boost::algorithm::starts_with(port_name, PROXY_PREFIX))
         {
            size_t found = port_name.rfind(fun_name);
            if(found != std::string::npos)
            {
               std::string orig_port_name = port_name.substr(0, found);
               GetPointer<port_o>(curr_port)->set_id(orig_port_name);
            }
         }
      }
   }
   HLS->Rfu->manage_killing_function_proxies(fun_obj, reverse_function_units, fun_call_sites_rel, SM, HLS, unique_id);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added functional units to circuit");
}

void fu_binding::check_parametrization(structural_objectRef curr_gate)
{
   auto* fu_module = GetPointer<module>(curr_gate);
   NP_functionalityRef np = fu_module->get_NP_functionality();
   if(np)
   {
      std::vector<std::string> param;
      np->get_library_parameters(param);
      std::vector<std::string>::const_iterator it_end = param.end();
      for(std::vector<std::string>::const_iterator it = param.begin(); it != it_end; ++it)
         THROW_ASSERT(curr_gate->find_member(*it, port_o_K, curr_gate) || curr_gate->find_member(*it, port_vector_o_K, curr_gate) || curr_gate->ExistsParameter(*it), "parameter not yet specialized: " + *it + " for module " + GET_TYPE_NAME(curr_gate));
   }
}

bool fu_binding::manage_module_ports(const HLS_managerRef HLSMgr, const hlsRef HLS, const structural_managerRef SM, const structural_objectRef curr_gate, unsigned int num)
{
   const structural_objectRef circuit = SM->get_circ();
   /// creating extern IN port on datapath starting from extern ports on module
   bool added_memory_element = false;
   for(unsigned int j = 0; j < GetPointer<module>(curr_gate)->get_in_port_size(); j++)
   {
      structural_objectRef port_in = GetPointer<module>(curr_gate)->get_in_port(j);
      manage_extern_global_port(HLSMgr, HLS, SM, port_in, port_o::IN, circuit, num);
      if(GetPointer<port_o>(port_in)->get_is_memory())
      {
         added_memory_element = true;
      }
   }
   /// creating extern OUT port on datapath starting from extern ports on module
   for(unsigned int j = 0; j < GetPointer<module>(curr_gate)->get_out_port_size(); j++)
   {
      structural_objectRef port_out = GetPointer<module>(curr_gate)->get_out_port(j);
      manage_extern_global_port(HLSMgr, HLS, SM, port_out, port_o::OUT, circuit, num);
   }
   /// creating extern IO port on datapath starting from extern ports on module
   for(unsigned int j = 0; j < GetPointer<module>(curr_gate)->get_in_out_port_size(); j++)
   {
      structural_objectRef port_in_out = GetPointer<module>(curr_gate)->get_in_out_port(j);
      manage_extern_global_port(HLSMgr, HLS, SM, port_in_out, port_o::IO, circuit, num);
   }
   return added_memory_element;
}

void fu_binding::manage_memory_ports_chained(const structural_managerRef SM, const std::list<structural_objectRef>& memory_modules, const structural_objectRef circuit)
{
   std::map<std::string, structural_objectRef> from_ports;
   std::map<std::string, structural_objectRef> primary_outs;
   structural_objectRef cir_port;
   unsigned int sign_id = 0;
   for(const auto& memory_module : memory_modules)
   {
      for(unsigned int j = 0; j < GetPointer<module>(memory_module)->get_in_port_size(); j++)
      {
         structural_objectRef port_i = GetPointer<module>(memory_module)->get_in_port(j);
         if(GetPointer<port_o>(port_i)->get_is_memory() && (!GetPointer<port_o>(port_i)->get_is_global()) && (!GetPointer<port_o>(port_i)->get_is_extern()))
         {
            std::string port_name = GetPointer<port_o>(port_i)->get_id();
            THROW_ASSERT(port_name.substr(1, 3) == "in_", "Expected the \"?in_\" prefix:" + port_name);
            std::string key = port_name.substr(0, 1) + port_name.substr(4); /// trimmed away the "?in_"prefix
            cir_port = circuit->find_member(port_name, port_i->get_kind(), circuit);
            THROW_ASSERT(!cir_port || GetPointer<port_o>(cir_port), "should be a port or null");
            if(!cir_port)
            {
               if(port_i->get_kind() == port_vector_o_K)
                  cir_port = SM->add_port_vector(port_name, port_o::IN, GetPointer<port_o>(port_i)->get_ports_size(), circuit, port_i->get_typeRef());
               else
                  cir_port = SM->add_port(port_name, port_o::IN, circuit, port_i->get_typeRef());
               port_o::fix_port_properties(port_i, cir_port);
               SM->add_connection(cir_port, port_i);
            }
            else
            {
               THROW_ASSERT(from_ports.find(key) != from_ports.end(), "somewhere the signal " + key + " should be produced");
               structural_objectRef sign;
               if(port_i->get_kind() == port_vector_o_K)
                  sign = SM->add_sign_vector(key + "_" + std::to_string(sign_id), GetPointer<port_o>(port_i)->get_ports_size(), circuit, port_i->get_typeRef());
               else
                  sign = SM->add_sign(key + "_" + std::to_string(sign_id), circuit, port_i->get_typeRef());
               ++sign_id;
               SM->add_connection(sign, port_i);
               SM->add_connection(sign, from_ports.find(key)->second);
               from_ports[key] = structural_objectRef();
            }
         }
      }
      for(unsigned int j = 0; j < GetPointer<module>(memory_module)->get_out_port_size(); j++)
      {
         structural_objectRef port_i = GetPointer<module>(memory_module)->get_out_port(j);
         if(GetPointer<port_o>(port_i)->get_is_memory() && (!GetPointer<port_o>(port_i)->get_is_global()) && (!GetPointer<port_o>(port_i)->get_is_extern()))
         {
            std::string port_name = GetPointer<port_o>(port_i)->get_id();
            THROW_ASSERT(port_name.substr(1, 4) == "out_", "Expected the \"?out_\" prefix: " + port_name);
            std::string key = port_name.substr(0, 1) + port_name.substr(5); /// trimmed away the "?out_"prefix
            cir_port = circuit->find_member(port_name, port_i->get_kind(), circuit);
            THROW_ASSERT(!cir_port || GetPointer<port_o>(cir_port), "should be a port or null");
            if(!cir_port)
            {
               if(port_i->get_kind() == port_vector_o_K)
                  cir_port = SM->add_port_vector(port_name, port_o::OUT, GetPointer<port_o>(port_i)->get_ports_size(), circuit, port_i->get_typeRef());
               else
                  cir_port = SM->add_port(port_name, port_o::OUT, circuit, port_i->get_typeRef());
               port_o::fix_port_properties(port_i, cir_port);
               primary_outs[port_name] = cir_port;
            }
            from_ports[key] = port_i;
         }
      }
   }
   std::map<std::string, structural_objectRef>::const_iterator po_end = primary_outs.end();
   for(std::map<std::string, structural_objectRef>::const_iterator po = primary_outs.begin(); po != po_end; ++po)
   {
      THROW_ASSERT(from_ports.find(po->first.substr(0, 1) + po->first.substr(5)) != from_ports.end(), "Port source not present");
      SM->add_connection(po->second, from_ports.find(po->first.substr(0, 1) + po->first.substr(5))->second);
   }
}

void fu_binding::join_merge_split(const structural_managerRef SM, const hlsRef HLS, std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter>& primary_outs, const structural_objectRef circuit, unsigned int& _unique_id)
{
   std::string js_name = "join_signal";
   std::string js_library = HLS->HLS_T->get_technology_manager()->get_library(js_name);
   std::string ss_name = "split_signal";
   std::string ss_library = HLS->HLS_T->get_technology_manager()->get_library(ss_name);
   const auto po_end = primary_outs.end();
   for(auto po = primary_outs.begin(); po != po_end; ++po)
   {
      std::string bus_merger_res_name = "bus_merger";
      std::string bus_merger_inst_name = bus_merger_res_name + po->first->get_id() + STR(_unique_id) + "_";
      ++_unique_id;
      std::string bm_library = HLS->HLS_T->get_technology_manager()->get_library(bus_merger_res_name);
      structural_objectRef bus_merger_mod = SM->add_module_from_technology_library(bus_merger_inst_name, bus_merger_res_name, bm_library, circuit, HLS->HLS_T->get_technology_manager());

      structural_objectRef bm_in_port = GetPointer<module>(bus_merger_mod)->get_in_port(0);
      GetPointer<port_o>(bm_in_port)->add_n_ports(static_cast<unsigned int>(po->second.size()), bm_in_port);
      if(po->first->get_kind() == port_vector_o_K)
         port_o::resize_std_port(GetPointer<port_o>(po->first)->get_ports_size() * STD_GET_SIZE(po->first->get_typeRef()), 0, 0, bm_in_port);
      else
         port_o::resize_std_port(STD_GET_SIZE(po->first->get_typeRef()), 0, 0, bm_in_port);
      auto it_el = po->second.begin();
      for(unsigned int in_id = 0; in_id < po->second.size(); ++in_id, ++it_el)
      {
         structural_objectRef sign_in;
         if(po->first->get_kind() == port_vector_o_K)
         {
            sign_in = SM->add_sign_vector("sig_in_vector_" + bus_merger_inst_name + STR(in_id), GetPointer<port_o>(*it_el)->get_ports_size(), circuit, (*it_el)->get_typeRef());
            structural_objectRef js_mod = SM->add_module_from_technology_library(js_name + bus_merger_inst_name + STR(in_id), js_name, js_library, circuit, HLS->HLS_T->get_technology_manager());
            structural_objectRef js_in_port = GetPointer<module>(js_mod)->get_in_port(0);
            GetPointer<port_o>(js_in_port)->add_n_ports(static_cast<unsigned int>(GetPointer<port_o>(*it_el)->get_ports_size()), js_in_port);
            port_o::resize_std_port(STD_GET_SIZE((*it_el)->get_typeRef()), 0, 0, js_in_port);
            SM->add_connection(sign_in, *it_el);
            SM->add_connection(sign_in, js_in_port);
            structural_objectRef js_out_port = GetPointer<module>(js_mod)->get_out_port(0);
            port_o::resize_std_port(GetPointer<port_o>(po->first)->get_ports_size() * STD_GET_SIZE(po->first->get_typeRef()), 0, 0, js_out_port);
            structural_type_descriptorRef sig_type = structural_type_descriptorRef(new structural_type_descriptor);
            po->first->get_typeRef()->copy(sig_type);
            sign_in = SM->add_sign("sig_in_" + bus_merger_inst_name + STR(in_id), circuit, sig_type);
            if(sig_type->type == structural_type_descriptor::BOOL)
            {
               sig_type->type = structural_type_descriptor::VECTOR_BOOL;
               sign_in->type_resize(1, GetPointer<port_o>(po->first)->get_ports_size() * STD_GET_SIZE(po->first->get_typeRef()));
            }
            else
               sign_in->type_resize(GetPointer<port_o>(po->first)->get_ports_size() * STD_GET_SIZE(po->first->get_typeRef()));
            SM->add_connection(sign_in, js_out_port);
            SM->add_connection(sign_in, GetPointer<port_o>(bm_in_port)->get_port(in_id));
         }
         else
         {
            sign_in = SM->add_sign("sig_in_" + bus_merger_inst_name + STR(in_id), circuit, po->first->get_typeRef());
            SM->add_connection(sign_in, *it_el);
            SM->add_connection(sign_in, GetPointer<port_o>(bm_in_port)->get_port(in_id));
         }
      }
      structural_objectRef bm_out_port = GetPointer<module>(bus_merger_mod)->get_out_port(0);
      if(po->first->get_kind() == port_vector_o_K)
         port_o::resize_std_port(GetPointer<port_o>(po->first)->get_ports_size() * STD_GET_SIZE(po->first->get_typeRef()), 0, 0, bm_out_port);
      else
         port_o::resize_std_port(STD_GET_SIZE(po->first->get_typeRef()), 0, 0, bm_out_port);
      structural_objectRef sign_out;
      if(po->first->get_kind() == port_vector_o_K)
      {
         structural_objectRef ss_mod = SM->add_module_from_technology_library(ss_name + bus_merger_inst_name, ss_name, ss_library, circuit, HLS->HLS_T->get_technology_manager());
         structural_type_descriptorRef sig_type = structural_type_descriptorRef(new structural_type_descriptor);
         po->first->get_typeRef()->copy(sig_type);
         sign_out = SM->add_sign("sig_out_" + bus_merger_inst_name, circuit, sig_type);
         if(sig_type->type == structural_type_descriptor::BOOL)
         {
            sig_type->type = structural_type_descriptor::VECTOR_BOOL;
            sign_out->type_resize(1, GetPointer<port_o>(po->first)->get_ports_size() * STD_GET_SIZE(po->first->get_typeRef()));
         }
         else
            sign_out->type_resize(GetPointer<port_o>(po->first)->get_ports_size() * STD_GET_SIZE(po->first->get_typeRef()));
         SM->add_connection(sign_out, bm_out_port);
         structural_objectRef ss_in_port = GetPointer<module>(ss_mod)->get_in_port(0);
         port_o::resize_std_port(GetPointer<port_o>(po->first)->get_ports_size() * STD_GET_SIZE(po->first->get_typeRef()), 0, 0, ss_in_port);
         SM->add_connection(sign_out, ss_in_port);
         structural_objectRef ss_out_port = GetPointer<module>(ss_mod)->get_out_port(0);
         GetPointer<port_o>(ss_out_port)->add_n_ports(static_cast<unsigned int>(GetPointer<port_o>(po->first)->get_ports_size()), ss_out_port);
         port_o::resize_std_port(STD_GET_SIZE(po->first->get_typeRef()), 0, 0, ss_out_port);
         if(po->first->get_owner() != circuit)
         {
            structural_objectRef sign_out_vector = SM->add_sign_vector("sig_out_vector_" + bus_merger_inst_name, GetPointer<port_o>(po->first)->get_ports_size(), circuit, po->first->get_typeRef());
            SM->add_connection(ss_out_port, sign_out_vector);
            SM->add_connection(sign_out_vector, po->first);
         }
         else
            SM->add_connection(ss_out_port, po->first);
      }
      else
      {
         structural_type_descriptorRef sig_type = structural_type_descriptorRef(new structural_type_descriptor);
         bm_out_port->get_typeRef()->copy(sig_type);
         sign_out = SM->add_sign("sig_out_" + bus_merger_inst_name, circuit, sig_type);
         if(sig_type->type == structural_type_descriptor::BOOL)
         {
            sig_type->type = structural_type_descriptor::VECTOR_BOOL;
            sign_out->type_resize(1, STD_GET_SIZE(po->first->get_typeRef()));
         }
         else
         {
            sign_out->type_resize(STD_GET_SIZE(po->first->get_typeRef()));
         }
         SM->add_connection(sign_out, bm_out_port);
         SM->add_connection(sign_out, po->first);
      }
   }
}

bool jms_sorter::operator()(const structural_objectRef& a, const structural_objectRef& b) const
{
   return a->get_path() < b->get_path();
}

void fu_binding::manage_memory_ports_parallel_chained(const HLS_managerRef, const structural_managerRef SM, const std::list<structural_objectRef>& memory_modules, const structural_objectRef circuit, const hlsRef HLS, unsigned int& _unique_id)
{
   std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter> primary_outs;
   structural_objectRef cir_port;
   for(const auto& memory_module : memory_modules)
   {
      for(unsigned int j = 0; j < GetPointer<module>(memory_module)->get_in_port_size(); ++j)
      {
         structural_objectRef port_i = GetPointer<module>(memory_module)->get_in_port(j);
         if(GetPointer<port_o>(port_i)->get_is_memory() && (!GetPointer<port_o>(port_i)->get_is_global()) && (!GetPointer<port_o>(port_i)->get_is_extern()))
         {
            std::string port_name = GetPointer<port_o>(port_i)->get_id();
            cir_port = circuit->find_member(port_name, port_i->get_kind(), circuit);
            THROW_ASSERT(!cir_port || GetPointer<port_o>(cir_port), "should be a port or null");
            if(!cir_port)
            {
               if(port_i->get_kind() == port_vector_o_K)
                  cir_port = SM->add_port_vector(port_name, port_o::IN, GetPointer<port_o>(port_i)->get_ports_size(), circuit, port_i->get_typeRef());
               else
                  cir_port = SM->add_port(port_name, port_o::IN, circuit, port_i->get_typeRef());
               port_o::fix_port_properties(port_i, cir_port);
               SM->add_connection(cir_port, port_i);
            }
            else
            {
               SM->add_connection(cir_port, port_i);
            }
         }
      }
      for(unsigned int j = 0; j < GetPointer<module>(memory_module)->get_out_port_size(); j++)
      {
         structural_objectRef port_i = GetPointer<module>(memory_module)->get_out_port(j);
         if(GetPointer<port_o>(port_i)->get_is_memory() && (!GetPointer<port_o>(port_i)->get_is_global()) && (!GetPointer<port_o>(port_i)->get_is_extern()))
         {
            std::string port_name = GetPointer<port_o>(port_i)->get_id();
            cir_port = circuit->find_member(port_name, port_i->get_kind(), circuit);
            THROW_ASSERT(!cir_port || GetPointer<port_o>(cir_port), "should be a port or null");
            if(!cir_port)
            {
               if(port_i->get_kind() == port_vector_o_K)
                  cir_port = SM->add_port_vector(port_name, port_o::OUT, GetPointer<port_o>(port_i)->get_ports_size(), circuit, port_i->get_typeRef());
               else
                  cir_port = SM->add_port(port_name, port_o::OUT, circuit, port_i->get_typeRef());
               port_o::fix_port_properties(port_i, cir_port);
            }
            if(std::find(primary_outs[cir_port].begin(), primary_outs[cir_port].end(), port_i) == primary_outs[cir_port].end())
               primary_outs[cir_port].push_back(port_i);
         }
      }
   }
   join_merge_split(SM, HLS, primary_outs, circuit, _unique_id);
}

void fu_binding::manage_extern_global_port(const HLS_managerRef, const hlsRef, const structural_managerRef SM, structural_objectRef port_in, unsigned int _dir, structural_objectRef circuit, unsigned int num)
{
   auto dir = static_cast<port_o::port_direction>(_dir);
   if(GetPointer<port_o>(port_in)->get_is_extern())
   {
      structural_objectRef ext_port;
      if(GetPointer<port_o>(port_in)->get_is_global())
      {
         std::string port_name = GetPointer<port_o>(port_in)->get_id();
         ext_port = circuit->find_member(port_name, port_in->get_kind(), circuit);
         THROW_ASSERT(!ext_port || GetPointer<port_o>(ext_port), "should be a port or null");
         if(ext_port && GetPointer<port_o>(ext_port)->get_port_direction() != dir)
         {
            SM->change_port_direction(ext_port, dir, circuit);
            if(STD_GET_SIZE(ext_port->get_typeRef()) < STD_GET_SIZE(port_in->get_typeRef()))
               port_o::resize_std_port(STD_GET_SIZE(port_in->get_typeRef()), 0, 0, ext_port);
         }
         else if(!ext_port)
         {
            if(port_in->get_kind() == port_vector_o_K)
               ext_port = SM->add_port_vector(port_name, dir, GetPointer<port_o>(port_in)->get_ports_size(), circuit, port_in->get_typeRef());
            else
               ext_port = SM->add_port(port_name, dir, circuit, port_in->get_typeRef());
         }
      }
      else
      {
         if(port_in->get_kind() == port_vector_o_K)
            ext_port = SM->add_port_vector("ext_" + GetPointer<port_o>(port_in)->get_id() + "_" + std::to_string(num), dir, GetPointer<port_o>(port_in)->get_ports_size(), circuit, port_in->get_typeRef());
         else
            ext_port = SM->add_port("ext_" + GetPointer<port_o>(port_in)->get_id() + "_" + std::to_string(num), dir, circuit, port_in->get_typeRef());
      }
      port_o::fix_port_properties(port_in, ext_port);
      SM->add_connection(port_in, ext_port);
   }
   else if(GetPointer<port_o>(port_in)->get_is_global())
   {
      structural_objectRef ext_port;
      std::string port_name = GetPointer<port_o>(port_in)->get_id();
      ext_port = circuit->find_member(port_name, port_in->get_kind(), circuit);
      THROW_ASSERT(!ext_port || GetPointer<port_o>(ext_port), "should be a port or null");
      if(!ext_port)
      {
         if(port_in->get_kind() == port_vector_o_K)
            ext_port = SM->add_port_vector(port_name, dir, GetPointer<port_o>(port_in)->get_ports_size(), circuit, port_in->get_typeRef());
         else
            ext_port = SM->add_port(port_name, dir, circuit, port_in->get_typeRef());
         port_o::fix_port_properties(port_in, ext_port);
      }
      SM->add_connection(port_in, ext_port);
   }
   else if(GetPointer<port_o>(port_in)->get_port_interface() != port_o::port_interface::PI_DEFAULT)
   {
      structural_objectRef ext_port;
      std::string port_name = GetPointer<port_o>(port_in)->get_id();
      ext_port = circuit->find_member(port_name, port_in->get_kind(), circuit);
      THROW_ASSERT(!ext_port || GetPointer<port_o>(ext_port), "should be a port or null");
      if(ext_port && GetPointer<port_o>(ext_port)->get_port_direction() != dir)
      {
         SM->change_port_direction(ext_port, dir, circuit);
         if(STD_GET_SIZE(ext_port->get_typeRef()) < STD_GET_SIZE(port_in->get_typeRef()))
            port_o::resize_std_port(STD_GET_SIZE(port_in->get_typeRef()), 0, 0, ext_port);
      }
      else if(!ext_port)
      {
         if(port_in->get_kind() == port_vector_o_K)
            ext_port = SM->add_port_vector(port_name, dir, GetPointer<port_o>(port_in)->get_ports_size(), circuit, port_in->get_typeRef());
         else
            ext_port = SM->add_port(port_name, dir, circuit, port_in->get_typeRef());
      }
      port_o::fix_port_properties(port_in, ext_port);
      SM->add_connection(port_in, ext_port);
   }
}

tree_nodeRef getFunctionType(tree_nodeRef exp);
void fu_binding::specialise_fu(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef fu_obj, unsigned int fu, const OpVertexSet& mapped_operations, unsigned int ar)
{
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(HLS->functionId);
   unsigned int bus_data_bitsize = HLSMgr->Rmem->get_bus_data_bitsize();
   unsigned int bus_size_bitsize = HLSMgr->Rmem->get_bus_size_bitsize();
   unsigned int bus_addr_bitsize = HLSMgr->get_address_bitsize();
   unsigned int bus_tag_bitsize = 0;
   if(HLS->Param->isOption(OPT_context_switch))
      bus_tag_bitsize = GetPointer<memory_cs>(HLSMgr->Rmem)->get_bus_tag_bitsize();
   module* fu_module = GetPointer<module>(fu_obj);
   const technology_nodeRef fu_tech_obj = allocation_information->get_fu(fu);
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Specializing " + fu_obj->get_path() + " of type " + GET_TYPE_NAME(fu_obj));
   std::map<unsigned int, unsigned int> required_variables;
   std::map<unsigned int, unsigned int> num_elements;
   unsigned int n_out_elements = 0;
   unsigned int produced_variables = 1;
   bool is_multiport = allocation_information->get_number_channels(fu) > 1;
   size_t max_n_ports = is_multiport ? allocation_information->get_number_channels(fu) : 0;

   if(ar)
   {
      bool has_misaligned_indirect_ref = false;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Ar is true");
      unsigned int elmt_bitsize = 1;
      unsigned int type_index = tree_helper::get_type_index(TreeM, ar);
      tree_nodeRef type_node = TreeM->get_tree_node_const(type_index);
      tree_helper::accessed_greatest_bitsize(TreeM, type_node, type_index, elmt_bitsize);

      if(allocation_information->is_direct_access_memory_unit(fu))
      {
         required_variables[0] = elmt_bitsize;
         if(HLSMgr->Rmem->is_private_memory(ar))
            bus_data_bitsize = std::max(bus_data_bitsize, elmt_bitsize);
         required_variables[1] = bus_addr_bitsize;
         if(HLSMgr->Rmem->is_private_memory(ar))
         {
            for(; elmt_bitsize >= (1u << bus_size_bitsize); ++bus_size_bitsize)
            {
            }
         }
         required_variables[2] = bus_size_bitsize;
         produced_variables = elmt_bitsize;
      }
      else
         THROW_ERROR("Unit currently not supported: " + allocation_information->get_fu_name(fu).first);
      const OpGraphConstRef data = FB->CGetOpGraph(FunctionBehavior::CFG);
      for(auto mapped_operation : mapped_operations)
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  on BRAM = " + data->CGetOpNodeInfo(mapped_operation)->GetOperation() + " " + GET_NAME(data, mapped_operation));
         const std::vector<HLS_manager::io_binding_type>& vars = HLSMgr->get_required_values(HLS->functionId, mapped_operation);
         unsigned int out_var = HLSMgr->get_produced_value(HLS->functionId, mapped_operation);
         if(GET_TYPE(data, mapped_operation) & TYPE_STORE)
         {
            THROW_ASSERT(std::get<0>(vars[0]), "Expected a tree node in case of a value to store");
            required_variables[0] = std::max(required_variables[0], tree_helper::size(TreeM, tree_helper::get_type_index(TreeM, std::get<0>(vars[0]))));
            if(tree_helper::is_a_misaligned_vector(TreeM, std::get<0>(vars[0])))
               has_misaligned_indirect_ref = true;
         }
         else if(GET_TYPE(data, mapped_operation) & TYPE_LOAD)
         {
            THROW_ASSERT(out_var, "Expected a tree node in case of a value to load");
            produced_variables = std::max(produced_variables, tree_helper::size(TreeM, tree_helper::get_type_index(TreeM, out_var)));
            if(tree_helper::is_a_misaligned_vector(TreeM, out_var))
               has_misaligned_indirect_ref = true;
         }
      }
      unsigned int accessed_bitsize = std::max(required_variables[0], produced_variables);

      unsigned int bram_bitsize = HLSMgr->Rmem->get_bram_bitsize();
      bool unaligned_access_p = parameters->isOption(OPT_unaligned_access) && parameters->getOption<bool>(OPT_unaligned_access);
      if(unaligned_access_p || bram_bitsize != 8 || bus_data_bitsize != 8 || HLSMgr->Rmem->is_private_memory(ar))
      {
         if(has_misaligned_indirect_ref)
         {
            bram_bitsize = std::max(accessed_bitsize, bram_bitsize);
         }
         else if(accessed_bitsize / 2 > bram_bitsize)
         {
            while(accessed_bitsize / 2 > bram_bitsize)
               bram_bitsize *= 2;
         }
#if 1
         unsigned datasize = elmt_bitsize;
         /// Round up to the next highest power of 2
         datasize--;
         datasize |= datasize >> 1;
         datasize |= datasize >> 2;
         datasize |= datasize >> 4;
         datasize |= datasize >> 8;
         datasize |= datasize >> 16;
         datasize++;
         if(datasize <= HLSMgr->Rmem->get_maxbram_bitsize() && bram_bitsize < datasize)
            bram_bitsize = datasize;
#endif
      }
      if(bram_bitsize > HLSMgr->Rmem->get_maxbram_bitsize())
         THROW_ERROR("incorrect operation mapping on memory module");

      if(fu_module->ExistsParameter("BRAM_BITSIZE"))
      {
         fu_module->SetParameter("BRAM_BITSIZE", STR(bram_bitsize));
      }
      if(fu_module->ExistsParameter("BUS_PIPELINED"))
      {
         bool Has_extern_allocated_data = ((HLSMgr->Rmem->get_memory_address() - HLSMgr->base_address) > 0 and parameters->getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) != MemoryAllocation_Policy::EXT_PIPELINED_BRAM and
                                           parameters->getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) != MemoryAllocation_Policy::INTERN_UNALIGNED) or
                                          (HLSMgr->Rmem->has_unknown_addresses() and parameters->getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) != MemoryAllocation_Policy::ALL_BRAM and
                                           parameters->getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) != MemoryAllocation_Policy::EXT_PIPELINED_BRAM);
         if(Has_extern_allocated_data)
         {
            fu_module->SetParameter("BUS_PIPELINED", "0");
         }
         else
         {
            fu_module->SetParameter("BUS_PIPELINED", "1");
         }
      }
   }
   else
   {
      const OpGraphConstRef data = FB->CGetOpGraph(FunctionBehavior::CFG);

      for(auto mapped_operation : mapped_operations)
      {
         const std::vector<HLS_manager::io_binding_type>& vars = HLSMgr->get_required_values(HLS->functionId, mapped_operation);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Considering operation " + HLSMgr->get_tree_manager()->get_tree_node_const(data->CGetOpNodeInfo(mapped_operation)->GetNodeId())->ToString());
         unsigned int out_var = HLSMgr->get_produced_value(HLS->functionId, mapped_operation);
         auto* fun_unit = GetPointer<functional_unit>(fu_tech_obj);
         std::string memory_ctrl_type = fun_unit->memory_ctrl_type;

         if(memory_ctrl_type != "")
         {
            unsigned int mem_var_size_in = 1;
            unsigned int mem_var_size_out = 1;

            if(GET_TYPE(data, mapped_operation) & TYPE_STORE)
            {
               THROW_ASSERT(std::get<0>(vars[0]), "Expected a tree node in case of a value to store");
               mem_var_size_in = std::max(mem_var_size_in, tree_helper::size(HLSMgr->get_tree_manager(), tree_helper::get_type_index(HLSMgr->get_tree_manager(), std::get<0>(vars[0]))));
            }
            else if(GET_TYPE(data, mapped_operation) & TYPE_LOAD)
            {
               THROW_ASSERT(out_var, "Expected a tree node in case of a value to load");
               mem_var_size_out = std::max(mem_var_size_out, tree_helper::size(TreeM, tree_helper::get_type_index(TreeM, out_var)));
            }
            /// specializing MEMORY_STD ports
            if(required_variables.find(0) == required_variables.end())
               required_variables[0] = 0;
            required_variables[0] = std::max(required_variables[0], mem_var_size_in);
            required_variables[1] = bus_addr_bitsize;
            if(allocation_information->is_direct_access_memory_unit(fu))
            {
               bus_data_bitsize = std::max(bus_data_bitsize, std::max(mem_var_size_in, mem_var_size_out));
               for(; bus_data_bitsize >= (1u << bus_size_bitsize); ++bus_size_bitsize)
                  ;
            }
            required_variables[2] = bus_size_bitsize;
            produced_variables = std::max(produced_variables, mem_var_size_out);
         }
         else if(HLS->HLS_T->get_technology_manager()->get_library(allocation_information->get_fu_name(fu).first) != WORK_LIBRARY &&
                 HLS->HLS_T->get_technology_manager()->get_library(allocation_information->get_fu_name(fu).first) != PROXY_LIBRARY) // functions just synthesized shouldn't be customized
         {
            NP_functionalityRef np = fu_module->get_NP_functionality();
            bool is_flopoco = np && np->exist_NP_functionality(NP_functionality::FLOPOCO_PROVIDED);
            std::string op_name = data->CGetOpNodeInfo(mapped_operation)->GetOperation();
            bool is_float_expr = op_name.find(FLOAT_EXPR) != std::string::npos;
            for(unsigned int i = 0; i < vars.size(); i++)
            {
               unsigned int tree_var = std::get<0>(vars[i]);
               if(tree_var == 0)
                  continue;
               if(required_variables.find(i) == required_variables.end())
                  required_variables[i] = 0;
               if(tree_helper::is_a_vector(TreeM, tree_var))
               {
                  unsigned int type_index = tree_helper::get_type_index(TreeM, tree_var);
                  const unsigned int size = static_cast<unsigned int>(tree_helper::size(TreeM, type_index));
                  const unsigned int element_type = tree_helper::GetElements(TreeM, type_index);
                  const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TreeM, element_type));
                  required_variables[i] = std::max(required_variables[i], element_size);

                  if(num_elements.find(i) == num_elements.end())
                  {
                     num_elements[i] = size / element_size;
                  }
                  else
                  {
                     THROW_ASSERT(num_elements.find(i)->second == size / element_size, "Performed a wrong module allocation");
                  }
               }
               else
               {
                  unsigned int bitsize = tree_helper::size(TreeM, tree_var);
                  if(is_float_expr && is_flopoco)
                  {
                     if(bitsize < 32)
                        bitsize = 32;
                     else if(bitsize > 32 && bitsize < 64)
                        bitsize = 64;
                  }
                  required_variables[i] = std::max(required_variables[i], bitsize);
               }
            }
            if(np)
            {
               std::vector<std::string> param;
               np->get_library_parameters(param);
               std::vector<std::string>::const_iterator it_end = param.end();
               for(std::vector<std::string>::const_iterator it = param.begin(); it != it_end; ++it)
               {
                  if(*it == "LSB_PARAMETER" && op_name == "pointer_plus_expr")
                  {
                     unsigned int curr_LSB = 0;
                     unsigned int op0_tree_var = std::get<0>(vars[0]);
                     if(op0_tree_var)
                     {
                        unsigned int var = tree_helper::get_base_index(TreeM, op0_tree_var);
                        if(var && FB->is_variable_mem(var) && HLSMgr->Rmem->is_sds_var(var))
                        {
                           unsigned int value_bitsize = 1;
                           unsigned int type_index = tree_helper::get_type_index(TreeM, var);
                           tree_nodeRef type_node = TreeM->get_tree_node_const(type_index);
                           tree_helper::accessed_greatest_bitsize(TreeM, type_node, type_index, value_bitsize);
                           if(value_bitsize <= 8)
                              curr_LSB = 0;
                           else if(value_bitsize == 16)
                              curr_LSB = 1;
                           else if(value_bitsize == 32)
                              curr_LSB = 2;
                           else if(value_bitsize == 64)
                              curr_LSB = 3;
                           else if(value_bitsize == 128)
                              curr_LSB = 4;
                           else if(value_bitsize == 256)
                              curr_LSB = 5;
                           else
                              curr_LSB = 0;
                        }
                     }
                     auto op0 = TreeM->get_tree_node_const(op0_tree_var);
                     auto op1 = TreeM->get_tree_node_const(std::get<0>(vars[1]));
                     if(op0->get_kind() == ssa_name_K)
                     {
                        auto ssa_var0 = GetPointer<ssa_name>(op0);
                        if(!ssa_var0->bit_values.empty())
                        {
                           auto tailZeros = 0u;
                           const auto lengthBV = ssa_var0->bit_values.size();
                           while(lengthBV > tailZeros && ssa_var0->bit_values.at(lengthBV - 1 - tailZeros) == '0')
                              ++tailZeros;
                           if(tailZeros < curr_LSB)
                              curr_LSB = tailZeros;
                        }
                        else
                           curr_LSB = 0;
                     }
                     else
                        curr_LSB = 0;
                     if(op1->get_kind() == ssa_name_K)
                     {
                        auto ssa_var1 = GetPointer<ssa_name>(op1);
                        if(!ssa_var1->bit_values.empty())
                        {
                           auto tailZeros = 0u;
                           const auto lengthBV = ssa_var1->bit_values.size();
                           while(lengthBV > tailZeros && ssa_var1->bit_values.at(lengthBV - 1 - tailZeros) == '0')
                              ++tailZeros;
                           if(tailZeros < curr_LSB)
                              curr_LSB = tailZeros;
                        }
                        else
                           curr_LSB = 0;
                     }
                     else if(op1->get_kind() == integer_cst_K)
                     {
                        const integer_cst* int_const = GetPointer<integer_cst>(op1);
                        unsigned long long int offset_value = static_cast<unsigned long long int>(int_const->value);
                        if(offset_value)
                        {
                           auto tailZeros = 0u;
                           while((offset_value & (1ULL << tailZeros)) == 0)
                              ++tailZeros;
                           if(tailZeros < curr_LSB)
                              curr_LSB = tailZeros;
                        }
                     }
                     else
                        curr_LSB = 0;
                     if(fu_module->ExistsParameter("LSB_PARAMETER"))
                     {
                        int lsb_parameter = boost::lexical_cast<int>(fu_module->GetParameter("LSB_PARAMETER"));
                        if(lsb_parameter < 0)
                           lsb_parameter = static_cast<int>(curr_LSB);
                        else
                           lsb_parameter = std::min(lsb_parameter, static_cast<int>(curr_LSB));
                        fu_module->SetParameter("LSB_PARAMETER", STR(lsb_parameter));
                     }
                     else
                     {
                        fu_module->SetParameter("LSB_PARAMETER", STR(curr_LSB));
                     }
                  }
                  if(*it == "OFFSET_PARAMETER" && op_name == "bit_ior_concat_expr")
                  {
                     unsigned int index = data->CGetOpNodeInfo(mapped_operation)->GetNodeId();
                     const tree_nodeRef ga_node = TreeM->GetTreeNode(index);
                     const gimple_assign* ga = GetPointer<gimple_assign>(ga_node);
                     const bit_ior_concat_expr* ce = GetPointer<bit_ior_concat_expr>(GET_NODE(ga->op1));
                     const tree_nodeRef offset_node = GET_NODE(ce->op2);
                     const integer_cst* int_const = GetPointer<integer_cst>(offset_node);
                     unsigned long long int offset_value = static_cast<unsigned long long int>(int_const->value);
                     fu_module->SetParameter("OFFSET_PARAMETER", STR(offset_value));
                  }
                  if(*it == "unlock_address" && op_name == BUILTIN_WAIT_CALL)
                  {
                     unsigned int index = data->CGetOpNodeInfo(mapped_operation)->GetNodeId();
                     std::string parameterName = HLSMgr->Rmem->get_symbol(index, HLS->functionId)->get_symbol_name();
                     fu_module->SetParameter("unlock_address", parameterName);
                  }
                  if(*it == "MEMORY_INIT_file" && op_name == BUILTIN_WAIT_CALL)
                  {
                     unsigned int index = data->CGetOpNodeInfo(mapped_operation)->GetNodeId();
                     std::string parameterAddressFileName = "function_addresses_" + STR(index) + ".mem";
                     std::ofstream parameterAddressFile(parameterAddressFileName);

                     const tree_nodeRef call = TreeM->GetTreeNode(index);
                     tree_nodeRef calledFunction = GetPointer<gimple_call>(call)->args[0];
                     tree_nodeRef hasreturn_node = GetPointer<gimple_call>(call)->args[1];
                     long long int hasreturn_value = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(hasreturn_node)));
                     tree_nodeRef addrExpr = GET_NODE(calledFunction);
                     tree_nodeRef functionType = getFunctionType(addrExpr);
                     tree_nodeRef paramList = GetPointer<function_type>(functionType)->prms;
                     unsigned int count_param = 0;
                     unsigned int address = 0;
                     unsigned int alignment = HLSMgr->Rmem->get_parameter_alignment();
                     HLSMgr->Rmem->compute_next_base_address(address, index, alignment);
                     while(paramList)
                     {
                        tree_nodeRef elem = GET_NODE(paramList);
                        auto* node = GetPointer<tree_list>(elem);
                        paramList = node->chan;
                        if(GET_NODE(node->valu)->get_kind() != void_type_K)
                        {
                           std::string str_address = convert_to_binary(static_cast<unsigned long long int>(address), HLSMgr->get_address_bitsize());
                           parameterAddressFile << str_address << "\n";
                           HLSMgr->Rmem->compute_next_base_address(address, GET_INDEX_NODE(node->valu), alignment);
                           count_param++;
                        }
                     }
                     tree_nodeRef return_type = GetPointer<function_type>(functionType)->retn;
                     if(return_type && GET_NODE(return_type)->get_kind() != void_type_K && hasreturn_value)
                     {
                        std::string str_address = convert_to_binary(static_cast<unsigned long long int>(address), HLSMgr->get_address_bitsize());
                        parameterAddressFile << str_address << "\n";
                     }
                     parameterAddressFile.close();
                     fu_module->SetParameter("MEMORY_INIT_file", "\"\"" + parameterAddressFileName + "\"\"");
                  }
               }
            }
            if(out_var)
            {
               if(tree_helper::is_a_vector(TreeM, out_var))
               {
                  unsigned int type_index = tree_helper::get_type_index(TreeM, out_var);
                  const unsigned int size = static_cast<unsigned int>(tree_helper::size(TreeM, type_index));
                  const unsigned int element_type = tree_helper::GetElements(TreeM, type_index);
                  const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TreeM, element_type));
                  n_out_elements = size / element_size;
                  produced_variables = element_size;
               }
               else
               {
                  produced_variables = std::max(produced_variables, tree_helper::size(TreeM, out_var));
               }
               /// check for precision parameter
               if(np)
               {
                  std::vector<std::string> param;
                  np->get_library_parameters(param);
                  std::vector<std::string>::const_iterator it_end = param.end();
                  for(std::vector<std::string>::const_iterator it = param.begin(); it != it_end; ++it)
                  {
                     if(*it == "PRECISION")
                     {
                        unsigned int sizetype = tree_helper::size(TreeM, tree_helper::get_type_index(TreeM, out_var));
                        if(sizetype == 1)
                           sizetype = 8;
                        fu_module->SetParameter("PRECISION", STR(sizetype));
                     }
                  }
               }
            }
         }
      }
   }

   unsigned int offset = 0;
   bool is_multi_read_cond = allocation_information->get_fu_name(fu).first == MULTI_READ_COND_STD;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Resizing input ports");
   for(unsigned int i = 0; i < fu_module->get_in_port_size(); i++)
   {
      structural_objectRef port = fu_module->get_in_port(i);
      if(port->get_id() == CLOCK_PORT_NAME || port->get_id() == RESET_PORT_NAME || port->get_id() == START_PORT_NAME)
         ++offset;
      if(is_multiport && port->get_kind() == port_vector_o_K && GetPointer<port_o>(port)->get_ports_size() == 0)
         GetPointer<port_o>(port)->add_n_ports(static_cast<unsigned int>(max_n_ports), port);
      else if(is_multi_read_cond && port->get_kind() == port_vector_o_K && GetPointer<port_o>(port)->get_ports_size() == 0)
         GetPointer<port_o>(port)->add_n_ports(static_cast<unsigned int>(required_variables.size()), port);
      if(GetPointer<port_o>(port)->get_is_data_bus() || GetPointer<port_o>(port)->get_is_addr_bus() || GetPointer<port_o>(port)->get_is_size_bus() || GetPointer<port_o>(port)->get_is_tag_bus())
         port_o::resize_busport(bus_size_bitsize, bus_addr_bitsize, bus_data_bitsize, bus_tag_bitsize, port);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Resized input ports");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Resizing variables");
   for(auto l = required_variables.begin(); l != required_variables.end() && !is_multi_read_cond; ++l)
   {
      unsigned int bitsize_variable = l->second;
      structural_objectRef port = fu_module->get_in_port(l->first + offset);
      unsigned int n_elmts = 0;
      if(num_elements.find(l->first) != num_elements.end())
         n_elmts = num_elements.find(l->first)->second;
      port_o::resize_std_port(bitsize_variable, n_elmts, debug_level, port);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Resized variables");
   offset = 0;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Resizing output ports");
   for(unsigned int i = 0; i < fu_module->get_out_port_size(); i++)
   {
      structural_objectRef port = fu_module->get_out_port(i);
      if(port->get_id() == DONE_PORT_NAME)
         offset++;
      if(is_multiport && port->get_kind() == port_vector_o_K && GetPointer<port_o>(port)->get_ports_size() == 0)
         GetPointer<port_o>(port)->add_n_ports(static_cast<unsigned int>(max_n_ports), port);
      if(GetPointer<port_o>(port)->get_is_data_bus() || GetPointer<port_o>(port)->get_is_addr_bus() || GetPointer<port_o>(port)->get_is_size_bus() || GetPointer<port_o>(port)->get_is_tag_bus())
         port_o::resize_busport(bus_size_bitsize, bus_addr_bitsize, bus_data_bitsize, bus_tag_bitsize, port);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Resized output ports");
   if(offset < fu_module->get_out_port_size())
   {
      structural_objectRef port = fu_module->get_out_port(offset);
      if(port->get_typeRef()->type != structural_type_descriptor::BOOL)
      {
         if(is_multi_read_cond)
            port_o::resize_std_port(static_cast<unsigned int>(required_variables.size()), 0, debug_level, port);
         else
            port_o::resize_std_port(produced_variables, n_out_elements, debug_level, port);
      }
   }

   auto* fun_unit = GetPointer<functional_unit>(fu_tech_obj);
   if(fun_unit)
   {
      const functional_unit::operation_vec& Ops = fun_unit->get_operations();
      auto ops_end = Ops.end();
      for(auto ops = Ops.begin(); ops != ops_end; ++ops)
      {
         auto* curr_op = GetPointer<operation>(*ops);
         std::string pipe_parameters_str = curr_op->pipe_parameters;
         if(pipe_parameters_str != "")
         {
            fu_module->SetParameter(PIPE_PARAMETER, pipe_parameters_str);
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Specialized " + fu_obj->get_path());
}

void fu_binding::specialize_memory_unit(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef fu_obj, unsigned int ar, std::string& base_address, unsigned int rangesize, bool is_doubled, bool is_memory_splitted, bool is_sparse_memory,
                                        bool is_sds)
{
   auto* fu_module = GetPointer<module>(fu_obj);
   /// base address specialization
   fu_module->SetParameter("address_space_begin", STR(base_address));
   fu_module->SetParameter("address_space_rangesize", STR(rangesize));
   if(is_sparse_memory)
      fu_module->SetParameter("USE_SPARSE_MEMORY", "1");
   else
      fu_module->SetParameter("USE_SPARSE_MEMORY", "0");
   memory::add_memory_parameter(HLS->datapath, base_address, STR(HLSMgr->Rmem->get_base_address(ar, HLS->functionId)));

   long long int vec_size = 0;
   /// array ref initialization
   THROW_ASSERT(ar, "expected a real tree node index");
   std::string init_filename = "array_ref_" + std::to_string(ar) + ".mem";
   std::ofstream init_file_a((init_filename).c_str());
   std::ofstream init_file_b;
   if(is_memory_splitted)
      init_file_b.open(("0_" + init_filename).c_str());
   unsigned int elts_size;
   fill_array_ref_memory(init_file_a, init_file_b, ar, vec_size, elts_size, HLSMgr->Rmem, ((is_doubled ? 2 : 1) * boost::lexical_cast<unsigned int>(fu_module->GetParameter("BRAM_BITSIZE"))), is_memory_splitted, is_sds, fu_module);
   THROW_ASSERT(vec_size, "at least one element is expected");
   if(is_memory_splitted)
   {
      fu_module->SetParameter("MEMORY_INIT_file_a", "\"\"" + init_filename + "\"\"");
      fu_module->SetParameter("MEMORY_INIT_file_b", "\"\"0_" + init_filename + "\"\"");
   }
   else
      fu_module->SetParameter("MEMORY_INIT_file", "\"\"" + init_filename + "\"\"");

   /// specialize the number of elements in the array
   bool unaligned_access_p = parameters->isOption(OPT_unaligned_access) && parameters->getOption<bool>(OPT_unaligned_access);
   if(!unaligned_access_p && HLSMgr->Rmem->get_bram_bitsize() == 8 && HLSMgr->Rmem->get_bus_data_bitsize() == 8 && !HLSMgr->Rmem->is_private_memory(ar))
   {
      fu_module->SetParameter("n_elements", STR((vec_size * elts_size) / 8));
      fu_module->SetParameter("data_size", "8");
   }
   else
   {
      fu_module->SetParameter("n_elements", STR(vec_size));
      fu_module->SetParameter("data_size", STR(elts_size));
   }
   if(HLSMgr->Rmem->is_private_memory(ar))
      fu_module->SetParameter("PRIVATE_MEMORY", "1");
   else
      fu_module->SetParameter("PRIVATE_MEMORY", "0");
   if(HLSMgr->Rmem->is_read_only_variable(ar))
      fu_module->SetParameter("READ_ONLY_MEMORY", "1");
   else
      fu_module->SetParameter("READ_ONLY_MEMORY", "0");
}
#define CHANGE_SDS_MEMORY_LAYOUT 0

void fu_binding::fill_array_ref_memory(std::ostream& init_file_a, std::ostream& init_file_b, unsigned int ar, long long int& vec_size, unsigned int& elts_size, const memoryRef mem, unsigned int bram_bitsize, bool is_memory_splitted, bool is_sds,
                                       module* fu_module)
{
   unsigned int type_index;
   tree_nodeRef ar_node = TreeM->get_tree_node_const(ar);
   tree_nodeRef init_node;
   auto* vd = GetPointer<var_decl>(ar_node);
   if(vd && vd->init)
      init_node = GET_NODE(vd->init);
   else if(GetPointer<string_cst>(ar_node))
      init_node = ar_node;
   tree_nodeRef array_type_node = tree_helper::get_type_node(ar_node, type_index);
   unsigned int element_precision = 0;
   if(tree_helper::is_an_array(TreeM, type_index))
   {
      std::vector<unsigned int> dims;
      tree_helper::get_array_dim_and_bitsize(TreeM, type_index, dims, elts_size);
      THROW_ASSERT(dims.size(), "something of wrong happen");
      vec_size = 1;
      for(std::vector<unsigned int>::const_iterator it = dims.begin(); it != dims.end(); ++it)
         vec_size *= *it;
   }
   else if(GetPointer<integer_type>(array_type_node) || GetPointer<real_type>(array_type_node) || GetPointer<enumeral_type>(array_type_node) || GetPointer<pointer_type>(array_type_node) || GetPointer<reference_type>(array_type_node) ||
           GetPointer<record_type>(array_type_node) || GetPointer<union_type>(array_type_node) || GetPointer<complex_type>(array_type_node))
   {
      elts_size = tree_helper::size(TreeM, type_index);
      vec_size = 1;
   }
   else if(GetPointer<boolean_type>(array_type_node))
   {
      elts_size = 8;
      vec_size = 1;
   }
   else if(GetPointer<vector_type>(array_type_node))
   {
      elts_size = static_cast<unsigned int>(tree_helper::Size(array_type_node));
      const unsigned int element_type = tree_helper::GetElements(TreeM, type_index);
      element_precision = static_cast<unsigned int>(tree_helper::size(TreeM, element_type));
      vec_size = 1;
   }
   else
      THROW_ERROR("Type not supported: " + array_type_node->get_kind_text());
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---elts_size " + STR(elts_size));
   if(is_sds)
   {
      bram_bitsize = elts_size;
      bool unaligned_access_p = parameters->isOption(OPT_unaligned_access) && parameters->getOption<bool>(OPT_unaligned_access);
      if(!unaligned_access_p && mem->get_bram_bitsize() == 8 && mem->get_bus_data_bitsize() == 8 && !mem->is_private_memory(ar))
      {
         fu_module->SetParameter("BRAM_BITSIZE", "8");
         vec_size = (vec_size * elts_size) / 8;
         bram_bitsize = 8;
         elts_size = 8;
         is_sds = false;
      }
      else
         fu_module->SetParameter("BRAM_BITSIZE", STR(elts_size));
#if CHANGE_SDS_MEMORY_LAYOUT
      if(vd && vd->bit_values.size())
      {
         elts_size = element_precision = static_cast<unsigned int>(vd->bit_values.size());
      }
#endif
   }

   unsigned int nbyte_on_memory = bram_bitsize / 8;

   if(init_node && ((GetPointer<constructor>(init_node) && GetPointer<constructor>(init_node)->list_of_idx_valu.size()) || (GetPointer<string_cst>(init_node) && GetPointer<string_cst>(init_node)->strg != "") ||
                    (!GetPointer<constructor>(init_node) && !GetPointer<string_cst>(init_node))))
   {
      std::vector<std::string> init_string;
      write_init(TreeM, ar_node, init_node, init_string, mem, element_precision);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---element_precision " + STR(element_precision));
      if(is_sds && (element_precision == 0 || elts_size == element_precision))
      {
         THROW_ASSERT(!is_memory_splitted, "unexpected condition");
         for(auto init_value : init_string)
            init_file_a << init_value << std::endl;
      }
      else
      {
         std::vector<std::string> eightbit_string;
         std::string bits_offset = "";
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---nbyte_on_memory " + STR(nbyte_on_memory));
         for(unsigned int l = 0; l < init_string.size(); ++l)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---init_string.size() " + STR(init_string.size()));
            if(init_string[l].size() < 8 && init_string.size() == 1)
            {
               std::string res = init_string[l];
               while(res.size() < 8)
                  res = "0" + res;
               eightbit_string.push_back(res);
            }
            else
            {
               std::string local_binary_string;
               size_t local_data_bitsize;
               size_t data_bitsize = init_string[l].size();
               if(bits_offset.size())
               {
                  if(static_cast<int>(data_bitsize) - 8 + static_cast<int>(bits_offset.size()) >= 0)
                  {
                     local_data_bitsize = data_bitsize - (8 - bits_offset.size());
                     eightbit_string.push_back(init_string[l].substr(data_bitsize - (8 - bits_offset.size()), 8 - bits_offset.size()) + bits_offset);
                     local_binary_string = init_string[l].substr(0, local_data_bitsize);
                     bits_offset = "";
                  }
                  else
                  {
                     local_data_bitsize = 0;
                     bits_offset = init_string[l] + bits_offset;
                  }
               }
               else
               {
                  local_binary_string = init_string[l];
                  local_data_bitsize = data_bitsize;
               }
               for(unsigned int base_index = 0; base_index < local_data_bitsize; base_index = base_index + 8)
               {
                  if((static_cast<int>(local_data_bitsize) - 8 - static_cast<int>(base_index)) >= 0)
                     eightbit_string.push_back(local_binary_string.substr(local_data_bitsize - 8 - base_index, 8));
                  else
                     bits_offset = local_binary_string.substr(0, local_data_bitsize - base_index);
               }
            }
         }
         if(bits_offset.size())
         {
            std::string tail_padding;
            for(auto tail_padding_ind = bits_offset.size(); tail_padding_ind < 8; ++tail_padding_ind)
               tail_padding += "0";
            tail_padding = tail_padding + bits_offset;
            eightbit_string.push_back(tail_padding);
         }
         if(eightbit_string.size() % nbyte_on_memory != 0)
         {
            for(size_t l = eightbit_string.size() % nbyte_on_memory; l < nbyte_on_memory; ++l)
               eightbit_string.push_back("00000000");
         }
         if(static_cast<size_t>(tree_helper::Size(array_type_node) / 8) > eightbit_string.size())
         {
            size_t tail_bytes = static_cast<size_t>(tree_helper::Size(array_type_node) / 8) - eightbit_string.size();
            for(size_t l = 0; l < tail_bytes; ++l)
               eightbit_string.push_back("00000000");
         }

         std::string str_bit;
         bool is_even = true;
         unsigned int counter = 0;
         for(unsigned int l = 0; l < eightbit_string.size();)
         {
            str_bit = "";
            for(counter = 0; counter < nbyte_on_memory && l < eightbit_string.size(); counter++, l++)
               str_bit = eightbit_string[l] + str_bit;
            if(is_even || !is_memory_splitted)
               init_file_a << str_bit << std::endl;
            else
               init_file_b << str_bit << std::endl;
            is_even = !is_even;
         }
         if(!is_even && is_memory_splitted)
         {
            for(unsigned int l = 0; l < (nbyte_on_memory * 8); ++l)
               init_file_b << "0";
         }
      }
   }
   else
   {
      if(is_sds)
      {
         THROW_ASSERT(!is_memory_splitted, "unexpected condition");
         for(unsigned int i = 0; i < vec_size; ++i)
         {
            for(unsigned int j = 0; j < elts_size; ++j)
               init_file_a << "0";
            init_file_a << std::endl;
         }
      }
      else
      {
         unsigned int counter = 0;
         bool is_even = true;
         for(unsigned int i = 0; i < vec_size; ++i)
         {
            for(unsigned int j = 0; j < elts_size; ++j)
            {
               if(is_even || !is_memory_splitted)
                  init_file_a << "0";
               else
                  init_file_b << "0";
               counter++;
               if(counter % (nbyte_on_memory * 8) == 0)
               {
                  if(is_even || !is_memory_splitted)
                     init_file_a << std::endl;
                  else
                     init_file_b << std::endl;
                  is_even = !is_even;
               }
            }
         }
         if(counter % (nbyte_on_memory * 8) != 0)
         {
            for(unsigned int l = counter % (nbyte_on_memory * 8); l < (nbyte_on_memory * 8); ++l)
            {
               if(is_even || !is_memory_splitted)
                  init_file_a << "0";
               else
                  init_file_b << "0";
            }
            is_even = !is_even;
         }
         if(!is_even && is_memory_splitted)
         {
            for(unsigned int l = 0; l < (nbyte_on_memory * 8); ++l)
               init_file_b << "0";
         }
      }
   }
}

void fu_binding::write_init(const tree_managerConstRef TreeM, tree_nodeRef var_node, tree_nodeRef init_node, std::vector<std::string>& init_file, const memoryRef mem, unsigned int element_precision)
{
   std::string trimmed_value;

   switch(init_node->get_kind())
   {
      case real_cst_K:
      {
         unsigned int type_index;
         tree_helper::get_type_node(init_node, type_index);
         unsigned int precision = tree_helper::size(TreeM, type_index);
         auto* rc = GetPointer<real_cst>(init_node);
         std::string C_value = rc->valr;
         trimmed_value = convert_fp_to_string(C_value, precision);
         init_file.push_back(trimmed_value);
         break;
      }
      case integer_cst_K:
      {
         auto* ic = GetPointer<integer_cst>(init_node);
         auto ull_value = static_cast<unsigned long long int>(tree_helper::get_integer_cst_value(ic));
         trimmed_value = "";
         unsigned int type_index;
         tree_helper::get_type_node(init_node, type_index);
         unsigned int precision = std::max(8u, tree_helper::size(TreeM, type_index));
         THROW_ASSERT(precision, "expected a size greater than 0");
         if(element_precision)
            precision = std::min(precision, element_precision);
         for(unsigned int ind = 0; ind < precision; ind++)
            trimmed_value = trimmed_value + (((1LLU << (precision - ind - 1)) & ull_value) ? '1' : '0');
         init_file.push_back(trimmed_value);
         break;
      }
      case complex_cst_K:
      {
         unsigned int type_index;
         tree_helper::get_type_node(init_node, type_index);
         unsigned int precision = tree_helper::size(TreeM, type_index);
         auto* rp = GetPointer<real_cst>(GET_NODE(GetPointer<complex_cst>(init_node)->real));
         std::string trimmed_value_r;
         if(rp)
         {
            std::string C_value_r = rp->valr;
            trimmed_value_r = convert_fp_to_string(C_value_r, precision / 2);
         }
         else
         {
            auto* ic = GetPointer<integer_cst>(GET_NODE(GetPointer<complex_cst>(init_node)->real));
            THROW_ASSERT(ic, "expected an integer_cst");
            auto ull_value = static_cast<unsigned long long int>(tree_helper::get_integer_cst_value(ic));
            for(unsigned int ind = 0; ind < precision / 2; ind++)
               trimmed_value_r = trimmed_value_r + (((1LLU << (precision / 2 - ind - 1)) & ull_value) ? '1' : '0');
         }
         auto* ip = GetPointer<real_cst>(GET_NODE(GetPointer<complex_cst>(init_node)->imag));
         std::string trimmed_value_i;
         if(ip)
         {
            std::string C_value_i = ip->valr;
            trimmed_value_i = convert_fp_to_string(C_value_i, precision / 2);
         }
         else
         {
            auto* ic = GetPointer<integer_cst>(GET_NODE(GetPointer<complex_cst>(init_node)->imag));
            THROW_ASSERT(ic, "expected an integer_cst");
            auto ull_value = static_cast<unsigned long long int>(tree_helper::get_integer_cst_value(ic));
            for(unsigned int ind = 0; ind < precision / 2; ind++)
               trimmed_value_i = trimmed_value_i + (((1LLU << (precision / 2 - ind - 1)) & ull_value) ? '1' : '0');
         }
         trimmed_value = trimmed_value_i + trimmed_value_r;
         init_file.push_back(trimmed_value);
         break;
      }
      case constructor_K:
      {
         auto* co = GetPointer<constructor>(init_node);
         std::vector<std::pair<tree_nodeRef, tree_nodeRef>>::const_iterator i = co->list_of_idx_valu.begin();
         std::vector<std::pair<tree_nodeRef, tree_nodeRef>>::const_iterator vend = co->list_of_idx_valu.end();
         bool designated_initializers_used = false;
         bool is_struct = false;
         bool is_union = false;
         unsigned int union_size = 0;
         // unsigned int struct_or_union_align = 0;
         std::vector<tree_nodeRef>* field_list = nullptr;
         /// check if designated initializers are really used
         tree_nodeRef firstnode = (i != vend) ? co->list_of_idx_valu.begin()->first : tree_nodeRef();
         if(firstnode && GET_NODE(firstnode)->get_kind() == field_decl_K)
         {
            auto* fd = GetPointer<field_decl>(GET_NODE(firstnode));
            tree_nodeRef scpe = GET_NODE(fd->scpe);

            if(scpe->get_kind() == record_type_K)
            {
               field_list = &(GetPointer<record_type>(scpe)->list_of_flds);
               // struct_or_union_align = GetPointer<record_type>(scpe)->algn;
               is_struct = true;
            }
            else if(scpe->get_kind() == union_type_K)
            {
               field_list = &(GetPointer<union_type>(scpe)->list_of_flds);
               is_union = true;
               union_size = tree_helper::size(TreeM, GET_INDEX_NODE(fd->scpe));
            }
            else
               THROW_ERROR("expected a record_type or a union_type");
            std::vector<tree_nodeRef>::const_iterator flend = field_list->end();
            std::vector<tree_nodeRef>::const_iterator fli = field_list->begin();
            for(; fli != flend && i != vend; ++i, ++fli)
            {
               if(i->first && GET_INDEX_NODE(i->first) != GET_INDEX_NODE(*fli))
                  break;
            }
            if(fli != flend && is_struct)
               designated_initializers_used = true;
         }

         const auto main_element_precision = element_precision;
         if(designated_initializers_used)
         {
            THROW_ASSERT(field_list, "something of wrong happen");
            std::vector<tree_nodeRef>::const_iterator flend = field_list->end();
            std::vector<tree_nodeRef>::const_iterator fli = field_list->begin();
            std::vector<tree_nodeRef>::const_iterator inext;
            i = co->list_of_idx_valu.begin();
            for(; fli != flend; ++fli)
            {
               if(!GetPointer<field_decl>(GET_NODE(*fli)))
                  continue;
               inext = fli;
               ++inext;
               while(inext != flend && !GetPointer<field_decl>(GET_NODE(*inext)))
                  ++inext;

               if(GetPointer<field_decl>(GET_NODE(*fli))->is_bitfield())
               {
                  unsigned int size_type_index = tree_helper::get_type_index(TreeM, GET_INDEX_NODE(*fli));
                  // fix the element precision to pass to write_init
                  element_precision = tree_helper::size(TreeM, size_type_index);
               }

               if(i != vend && GET_INDEX_NODE(i->first) == GET_INDEX_NODE(*fli))
               {
                  write_init(TreeM, GET_NODE(i->first), GET_NODE(i->second), init_file, mem, element_precision);
                  ++i;
               }
               else
               {
                  write_init(TreeM, GET_NODE(*fli), GET_NODE(*fli), init_file, mem, element_precision);
               }

               if(GetPointer<field_decl>(GET_NODE(*fli))->is_bitfield())
               {
                  // reset the element_precision to the main value
                  element_precision = main_element_precision;
               }

               if(!GetPointer<field_decl>(GET_NODE(*fli))->is_bitfield())
               {
                  /// check if padding is needed
                  unsigned long long int nbits;
                  unsigned int type_index;
                  integer_cst* ic;
                  if(inext != flend)
                  {
                     tree_nodeRef idx_next = GET_NODE(*inext);
                     auto* idx_next_fd = GetPointer<field_decl>(idx_next);
                     ic = GetPointer<integer_cst>(GET_NODE(idx_next_fd->bpos));
                     nbits = static_cast<unsigned long long int>(tree_helper::get_integer_cst_value(ic));
                  }
                  else
                  {
                     type_index = GET_INDEX_NODE(co->type);
                     nbits = tree_helper::size(TreeM, type_index);
                  }
                  tree_nodeRef idx_curr = GET_NODE(*fli);
                  auto* idx_curr_fd = GetPointer<field_decl>(idx_curr);
                  tree_helper::get_type_node(idx_curr, type_index);
                  unsigned int field_decl_size = tree_helper::size(TreeM, type_index);
                  ic = GetPointer<integer_cst>(GET_NODE(idx_curr_fd->bpos));
                  nbits = nbits - static_cast<unsigned long long int>(tree_helper::get_integer_cst_value(ic));
                  nbits = nbits - field_decl_size;
                  if(nbits > 0)
                  {
                     /// add padding
                     std::string init_string;
                     for(unsigned int j = 0; j < nbits; ++j)
                        init_string += "0";
                     init_file.push_back(init_string);
                  }
               }
            }
         }
         else
         {
            std::vector<std::pair<tree_nodeRef, tree_nodeRef>>::const_iterator inext;
            for(i = co->list_of_idx_valu.begin(); i != vend; ++i)
            {
               if(is_struct and !GetPointer<field_decl>(GET_NODE(i->first)))
                  continue;
               inext = i;
               ++inext;
               while(inext != vend && is_struct && !GetPointer<field_decl>(GET_NODE(inext->first)))
                  ++inext;
               if(is_struct and GetPointer<field_decl>(GET_NODE(i->first))->is_bitfield())
               {
                  unsigned int size_type_index = tree_helper::get_type_index(TreeM, GET_INDEX_NODE(i->first));
                  // fix the element precision to pass to write_init
                  element_precision = tree_helper::size(TreeM, size_type_index);
               }
               write_init(TreeM, i->first ? GET_NODE(i->first) : tree_nodeRef(), GET_NODE(i->second), init_file, mem, element_precision);
               if(is_struct and GetPointer<field_decl>(GET_NODE(i->first))->is_bitfield())
               {
                  // reset the element_precision to the main value
                  element_precision = main_element_precision;
               }

               if(is_struct && !GetPointer<field_decl>(GET_NODE(i->first))->is_bitfield())
               {
                  /// check if padding is needed
                  unsigned long long int nbits;
                  unsigned int type_index;
                  integer_cst* ic;
                  if(inext != vend)
                  {
                     tree_nodeRef idx_next = GET_NODE(inext->first);
                     auto* idx_next_fd = GetPointer<field_decl>(idx_next);
                     ic = GetPointer<integer_cst>(GET_NODE(idx_next_fd->bpos));
                     nbits = static_cast<unsigned long long int>(tree_helper::get_integer_cst_value(ic));
                  }
                  else
                  {
                     type_index = GET_INDEX_NODE(co->type);
                     nbits = tree_helper::size(TreeM, type_index);
                  }
                  tree_nodeRef idx_curr = GET_NODE(i->first);
                  auto* idx_curr_fd = GetPointer<field_decl>(idx_curr);
                  tree_helper::get_type_node(idx_curr, type_index);
                  unsigned int field_decl_size = tree_helper::size(TreeM, type_index);
                  ic = GetPointer<integer_cst>(GET_NODE(idx_curr_fd->bpos));
                  nbits = nbits - static_cast<unsigned long long int>(tree_helper::get_integer_cst_value(ic));
                  nbits = nbits - field_decl_size;
                  if(nbits > 0)
                  {
                     /// add padding
                     std::string init_string;
                     for(unsigned int j = 0; j < nbits; ++j)
                        init_string += "0";
                     init_file.push_back(init_string);
                  }
               }
               else if(is_union)
               {
                  /// check if padding is needed
                  THROW_ASSERT(co->list_of_idx_valu.size() == 1, "just one initializer is possible");
                  unsigned int type_index;
                  tree_helper::get_type_node(GET_NODE(i->first), type_index);
                  unsigned int field_decl_size = tree_helper::size(TreeM, type_index);
                  if(field_decl_size != union_size)
                  {
                     /// add padding
                     unsigned int nbits = union_size - field_decl_size;
                     std::string init_string;
                     for(unsigned int j = 0; j < nbits; ++j)
                        init_string += "0";
                     init_file.push_back(init_string);
                  }
               }
            }
         }
         unsigned int type_index;
         tree_nodeRef type_n = tree_helper::get_type_node(var_node, type_index);
         if(GetPointer<array_type>(type_n))
         {
            unsigned int size_of_data;
            std::vector<unsigned int> dims;
            tree_helper::get_array_dim_and_bitsize(TreeM, type_index, dims, size_of_data);
            if(element_precision)
               size_of_data = std::min(size_of_data, element_precision);
            unsigned int num_elements = dims[0];
            std::string value;
            if(num_elements < co->list_of_idx_valu.size())
               THROW_ERROR("C description not supported: Array with undefined size or not correctly initialized " + STR(co->list_of_idx_valu.size()) + "-" + STR(num_elements));
            num_elements = num_elements - static_cast<unsigned int>(co->list_of_idx_valu.size());
            for(unsigned int l = 0; l < num_elements; l++)
            {
               value = "";
               for(unsigned int ii = 0; ii < size_of_data; ii++)
                  value += "0";
               init_file.push_back(value);
            }
         }
         break;
      }
      case string_cst_K:
      {
         auto* sc = GetPointer<string_cst>(init_node);
         std::string string_value = sc->strg;
         std::string tmp;
         for(unsigned int index = 0; index < string_value.size(); ++index)
         {
            if(string_value[index] == '\\' && string_value[index + 1] == '0')
            {
               tmp += '\0';
               ++index;
            }
            else
               tmp += string_value[index];
         }
         string_value = tmp;
         boost::replace_all(string_value, "\\a", "\a");
         boost::replace_all(string_value, "\\b", "\b");
         boost::replace_all(string_value, "\\t", "\t");
         boost::replace_all(string_value, "\\n", "\n");
         boost::replace_all(string_value, "\\v", "\v");
         boost::replace_all(string_value, "\\f", "\f");
         boost::replace_all(string_value, "\\r", "\r");
         boost::replace_all(string_value, "\\'", "'");
         boost::replace_all(string_value, "\\\"", "\"");
         boost::replace_all(string_value, "\\\\", "\\");
         unsigned int elmt_bitsize;
         std::vector<unsigned int> dims;

         tree_helper::get_array_dim_and_bitsize(TreeM, GET_INDEX_NODE(sc->type), dims, elmt_bitsize);
         if(elmt_bitsize == 32) // wide char used
         {
            THROW_ERROR("wide char conversion not supported");
         }
         else
         {
            for(char j : string_value)
            {
               auto ull_value = static_cast<unsigned long int>(j);
               trimmed_value = "";
               for(unsigned int ind = 0; ind < elmt_bitsize; ind++)
                  trimmed_value = trimmed_value + (((1LLU << (elmt_bitsize - ind - 1)) & ull_value) ? '1' : '0');
               init_file.push_back(trimmed_value);
            }
         }
         trimmed_value = "";
         for(unsigned int ind = 0; ind < elmt_bitsize; ind++)
            trimmed_value = trimmed_value + '0';
         init_file.push_back(trimmed_value);
         unsigned int type_index;
         tree_nodeRef type_n = tree_helper::get_type_node(var_node, type_index);
         THROW_ASSERT(GetPointer<array_type>(type_n), "expected an array_type");
         unsigned int size_of_data;
         dims.clear();
         tree_helper::get_array_dim_and_bitsize(TreeM, type_index, dims, size_of_data);
         THROW_ASSERT(size_of_data == elmt_bitsize, "something of wrong happen");
         unsigned int num_elements = 1;
         std::string value;
         for(std::vector<unsigned int>::const_iterator it = dims.begin(); it != dims.end(); ++it)
            num_elements *= *it;
         if(num_elements < (string_value.size() + 1))
            THROW_ERROR("C description not supported: string with undefined size or not correctly initialized " + STR(string_value.size() + 1) + "-" + STR(num_elements));
         num_elements = num_elements - static_cast<unsigned int>(string_value.size() + 1);
         for(unsigned int l = 0; l < num_elements; l++)
         {
            value = "";
            for(unsigned int ii = 0; ii < size_of_data; ii++)
               value += "0";
            init_file.push_back(value);
         }
         break;
      }
      case view_convert_expr_K:
      case nop_expr_K:
      {
         auto* ue = GetPointer<unary_expr>(init_node);
         if(GetPointer<addr_expr>(GET_NODE(ue->op)))
         {
            write_init(TreeM, GET_NODE(ue->op), GET_NODE(ue->op), init_file, mem, element_precision);
         }
         else if(GetPointer<integer_cst>(GET_NODE(ue->op)))
         {
            unsigned int type_index;
            tree_helper::get_type_node(init_node, type_index);
            unsigned int precision = std::max(std::max(8u, element_precision), tree_helper::size(TreeM, type_index));
            write_init(TreeM, GET_NODE(ue->op), GET_NODE(ue->op), init_file, mem, precision);
         }
         else
            THROW_ERROR("Something of unexpected happened: " + STR(init_node->index) + " | " + GET_NODE(ue->op)->get_kind_text());
         break;
      }
      case addr_expr_K:
      {
         auto* ae = GetPointer<addr_expr>(init_node);
         tree_nodeRef addr_expr_op = GET_NODE(ae->op);
         unsigned int addr_expr_op_idx = GET_INDEX_NODE(ae->op);
         unsigned long long int ull_value = 0;
         unsigned int type_index;
         tree_helper::get_type_node(init_node, type_index);
         unsigned int precision = tree_helper::size(TreeM, type_index);
         switch(addr_expr_op->get_kind())
         {
            case ssa_name_K:
            case var_decl_K:
            case parm_decl_K:
            case string_cst_K:
            {
               THROW_ASSERT(mem->has_base_address(addr_expr_op_idx), "missing base address for: @" + STR(addr_expr_op_idx));
               ull_value = mem->get_base_address(addr_expr_op_idx, 0);
               break;
            }
            case array_ref_K:
            {
               auto* ar = GetPointer<array_ref>(addr_expr_op);
               tree_nodeRef aridx = GET_NODE(ar->op1);
               if(aridx->get_kind() == integer_cst_K && GetPointer<integer_cst>(aridx))
               {
                  switch(GET_NODE(ar->op0)->get_kind())
                  {
                     case ssa_name_K:
                     case var_decl_K:
                     case parm_decl_K:
                     case string_cst_K:
                     {
                        unsigned int step = tree_helper::size(TreeM, tree_helper::get_type_index(TreeM, addr_expr_op_idx)) / 8;
                        ull_value = mem->get_base_address(GET_INDEX_NODE(ar->op0), 0) + step * static_cast<unsigned int>(tree_helper::get_integer_cst_value(GetPointer<integer_cst>(aridx)));
                        break;
                     }
                     case binfo_K:
                     case block_K:
                     case call_expr_K:
                     case aggr_init_expr_K:
                     case case_label_expr_K:
                     case constructor_K:
                     case identifier_node_K:
                     case statement_list_K:
                     case target_expr_K:
                     case target_mem_ref_K:
                     case target_mem_ref461_K:
                     case tree_list_K:
                     case tree_vec_K:
                     case lut_expr_K:
                     case CASE_BINARY_EXPRESSION:
                     case CASE_CPP_NODES:
                     case CASE_FAKE_NODES:
                     case CASE_GIMPLE_NODES:
                     case CASE_PRAGMA_NODES:
                     case CASE_QUATERNARY_EXPRESSION:
                     case CASE_TERNARY_EXPRESSION:
                     case CASE_TYPE_NODES:
                     case CASE_UNARY_EXPRESSION:
                     case complex_cst_K:
                     case integer_cst_K:
                     case real_cst_K:
                     case vector_cst_K:
                     case void_cst_K:
                     case const_decl_K:
                     case field_decl_K:
                     case function_decl_K:
                     case label_decl_K:
                     case namespace_decl_K:
                     case result_decl_K:
                     case translation_unit_decl_K:
                     case error_mark_K:
                     case using_decl_K:
                     case type_decl_K:
                     case template_decl_K:
                     default:
                        THROW_ERROR("addr_expr-array_ref[0] pattern not supported: " + std::string(addr_expr_op->get_kind_text()) + " @" + STR(addr_expr_op_idx));
                  }
               }
               else
                  THROW_ERROR("addr_expr-array_ref[0] pattern not supported: " + std::string(addr_expr_op->get_kind_text()) + " @" + STR(addr_expr_op_idx));
               break;
            }
            case function_decl_K:
            {
               THROW_ASSERT(mem->has_base_address(addr_expr_op_idx), "missing base address for: @" + STR(addr_expr_op_idx));
               ull_value = mem->get_base_address(addr_expr_op_idx, addr_expr_op_idx);
               break;
            }
            case CASE_BINARY_EXPRESSION:
            {
               if(addr_expr_op->get_kind() == mem_ref_K)
               {
                  auto* mr = GetPointer<mem_ref>(addr_expr_op);
                  tree_nodeRef offset = GET_NODE(mr->op1);
                  if(offset->get_kind() == integer_cst_K)
                  {
                     auto base = mr->op0;
                     auto base_index = GET_INDEX_NODE(base);
                     auto base_node = GET_NODE(base);
                     auto base_code = base_node->get_kind();
                     if(base_code == var_decl_K)
                     {
                        THROW_ASSERT(mem->has_base_address(base_index), "missing base address for: @" + STR(base_index));
                        ull_value = mem->get_base_address(base_index, 0) + static_cast<unsigned int>(tree_helper::get_integer_cst_value(GetPointer<integer_cst>(offset)));
                     }
                     else if(base_code == addr_expr_K)
                     {
                        auto base1 = GetPointer<addr_expr>(base_node)->op;
                        auto base1_index = GET_INDEX_NODE(base1);
                        auto base1_node = GET_NODE(base1);
                        auto base1_code = base1_node->get_kind();
                        if(base1_code == var_decl_K)
                        {
                           THROW_ASSERT(mem->has_base_address(base1_index), "missing base address for: @" + STR(base1_index));
                           ull_value = mem->get_base_address(base1_index, 0) + static_cast<unsigned int>(tree_helper::get_integer_cst_value(GetPointer<integer_cst>(offset)));
                        }
                        else
                           THROW_ERROR("addr_expr pattern not supported: " + std::string(addr_expr_op->get_kind_text()) + " @" + STR(addr_expr_op_idx));
                     }
                     else
                        THROW_ERROR("addr_expr pattern not supported: " + std::string(addr_expr_op->get_kind_text()) + " @" + STR(addr_expr_op_idx));
                  }
                  else
                     THROW_ERROR("addr_expr pattern not supported: " + std::string(addr_expr_op->get_kind_text()) + " @" + STR(addr_expr_op_idx));
               }
               else
                  THROW_ERROR("addr_expr pattern not supported: " + std::string(addr_expr_op->get_kind_text()) + " @" + STR(addr_expr_op_idx));
               break;
            }
            case array_range_ref_K:
            case binfo_K:
            case bit_field_ref_K:
            case block_K:
            case call_expr_K:
            case aggr_init_expr_K:
            case case_label_expr_K:
            case complex_cst_K:
            case component_ref_K:
            case cond_expr_K:
            case const_decl_K:
            case constructor_K:
            case dot_prod_expr_K:
            case ternary_plus_expr_K:
            case ternary_pm_expr_K:
            case ternary_mp_expr_K:
            case ternary_mm_expr_K:
            case bit_ior_concat_expr_K:
            case field_decl_K:
            case identifier_node_K:
            case integer_cst_K:
            case label_decl_K:
            case namespace_decl_K:
            case obj_type_ref_K:
            case real_cst_K:
            case result_decl_K:
            case save_expr_K:
            case statement_list_K:
            case target_expr_K:
            case target_mem_ref_K:
            case target_mem_ref461_K:
            case translation_unit_decl_K:
            case template_decl_K:
            case error_mark_K:
            case using_decl_K:
            case tree_list_K:
            case tree_vec_K:
            case type_decl_K:
            case vec_cond_expr_K:
            case vec_perm_expr_K:
            case vector_cst_K:
            case void_cst_K:
            case vtable_ref_K:
            case with_cleanup_expr_K:
            case lut_expr_K:
            case CASE_CPP_NODES:
            case CASE_FAKE_NODES:
            case CASE_GIMPLE_NODES:
            case CASE_PRAGMA_NODES:
            case CASE_TYPE_NODES:
            case CASE_UNARY_EXPRESSION:
            default:
               THROW_ERROR("addr_expr pattern not supported: " + std::string(addr_expr_op->get_kind_text()) + " @" + STR(addr_expr_op_idx));
         }
         for(unsigned int ind = 0; ind < precision; ind++)
            trimmed_value = trimmed_value + (((1LLU << (precision - ind - 1)) & ull_value) ? '1' : '0');
         init_file.push_back(trimmed_value);

         break;
      }
      case field_decl_K:
      {
         unsigned int type_index;
         tree_helper::get_type_node(init_node, type_index);
         unsigned int field_decl_size = tree_helper::size(TreeM, type_index);
         std::string init_string;
         for(unsigned int j = 0; j < field_decl_size; ++j)
            init_string += "0";
         if(field_decl_size)
            init_file.push_back(init_string);
         break;
      }
      case vector_cst_K:
      {
         auto* vc = GetPointer<vector_cst>(init_node);
         for(auto& i : (vc->list_of_valu)) // vector elements
         {
            write_init(TreeM, GET_NODE(i), GET_NODE(i), init_file, mem, element_precision);
         }
         break;
      }
      case void_cst_K:
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case identifier_node_K:
      case paren_expr_K:
      case ssa_name_K:
      case statement_list_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      case const_decl_K:
      case function_decl_K:
      case label_decl_K:
      case namespace_decl_K:
      case parm_decl_K:
      case result_decl_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case error_mark_K:
      case using_decl_K:
      case type_decl_K:
      case var_decl_K:
      case abs_expr_K:
      case arrow_expr_K:
      case bit_not_expr_K:
      case buffer_ref_K:
      case card_expr_K:
      case cleanup_point_expr_K:
      case conj_expr_K:
      case convert_expr_K:
      case exit_expr_K:
      case fix_ceil_expr_K:
      case fix_floor_expr_K:
      case fix_round_expr_K:
      case fix_trunc_expr_K:
      case float_expr_K:
      case imagpart_expr_K:
      case indirect_ref_K:
      case misaligned_indirect_ref_K:
      case loop_expr_K:
      case negate_expr_K:
      case non_lvalue_expr_K:
      case realpart_expr_K:
      case reference_expr_K:
      case reinterpret_cast_expr_K:
      case sizeof_expr_K:
      case static_cast_expr_K:
      case throw_expr_K:
      case target_expr_K:
      case truth_not_expr_K:
      case unsave_expr_K:
      case va_arg_expr_K:
      case reduc_max_expr_K:
      case reduc_min_expr_K:
      case reduc_plus_expr_K:
      case vec_unpack_hi_expr_K:
      case vec_unpack_lo_expr_K:
      case vec_unpack_float_hi_expr_K:
      case vec_unpack_float_lo_expr_K:
      case lut_expr_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      default:
         THROW_ERROR("elements not yet supported: " + init_node->get_kind_text() + init_node->ToString() + (var_node ? var_node->ToString() : ""));
   }
}

tree_nodeRef getFunctionType(tree_nodeRef exp)
{
   THROW_ASSERT(GetPointer<addr_expr>(exp) || GetPointer<ssa_name>(exp), "Input must be a ssa_name or an addr_expr");
   auto* sa = GetPointer<ssa_name>(exp);
   if(sa)
   {
      THROW_ASSERT(sa, "Function pointer not in SSA-form");
      pointer_type* pt;
      if(sa->var)
      {
         auto* var = GetPointer<decl_node>(GET_NODE(sa->var));
         THROW_ASSERT(var, "Call expression does not point to a declaration node");
         pt = GetPointer<pointer_type>(GET_NODE(var->type));
      }
      else
         pt = GetPointer<pointer_type>(GET_NODE(sa->type));

      THROW_ASSERT(pt, "Declaration node has not information about pointer_type");
      THROW_ASSERT(GetPointer<function_type>(GET_NODE(pt->ptd)), "Pointer type has not information about pointed function_type");

      return GET_NODE(pt->ptd);
   }

   auto* AE = GetPointer<addr_expr>(exp);
   auto* FD = GetPointer<function_decl>(GET_NODE(AE->op));
   return GET_NODE(FD->type);
}

void fu_binding::set_ports_are_swapped(vertex v, bool condition)
{
   if(condition)
      ports_are_swapped.insert(v);
   else
      ports_are_swapped.erase(v);
}

generic_objRef fu_binding::get(const vertex v) const
{
   const unsigned int statement_index = op_graph->CGetOpNodeInfo(v)->GetNodeId();
   return op_binding.find(statement_index)->second;
}
