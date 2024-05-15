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

#include "ModuleGeneratorManager.hpp"
#include "exceptions.hpp"
#include "fu_binding_cs.hpp"
#include "funit_obj.hpp"

#include "call_graph_manager.hpp"
#include "hls.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
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

#include "conn_binding.hpp"

#include "library_manager.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"

#include <boost/algorithm/string/replace.hpp>

#include "Parameter.hpp"

/// design_flows/backend/ToHDL include
#include "language_writer.hpp"

/// HLS/module_allocation
#include "allocation_information.hpp"

/// STD include
#include "custom_set.hpp"
#include <algorithm>
#include <fstream>
#include <limits>
#include <regex>
#include <string>
#include <utility>
#include <vector>

/// utility include
#include "fileIO.hpp"
#include "math_function.hpp"
#include "string_manipulation.hpp"

const unsigned int fu_binding::UNKNOWN = std::numeric_limits<unsigned int>::max();

fu_binding::fu_binding(const HLS_managerConstRef _HLSMgr, const unsigned int _function_id,
                       const ParameterConstRef _parameters)
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

fu_bindingRef fu_binding::create_fu_binding(const HLS_managerConstRef _HLSMgr, const unsigned int _function_id,
                                            const ParameterConstRef _parameters)
{
   if(_parameters->isOption(OPT_context_switch))
   {
      auto omp_functions = GetPointer<OmpFunctions>(_HLSMgr->Rfuns);
      bool found = false;
      if(omp_functions->kernel_functions.find(_function_id) != omp_functions->kernel_functions.end())
      {
         found = true;
      }
      if(omp_functions->hierarchical_functions.find(_function_id) != omp_functions->hierarchical_functions.end())
      {
         found = true;
      }
      if(omp_functions->parallelized_functions.find(_function_id) != omp_functions->parallelized_functions.end())
      {
         found = true;
      }
      if(omp_functions->atomic_functions.find(_function_id) != omp_functions->atomic_functions.end())
      {
         found = true;
      }
      if(found)
      {
         return fu_bindingRef(new fu_binding_cs(_HLSMgr, _function_id, _parameters));
      }
      else
      {
         return fu_bindingRef(new fu_binding(_HLSMgr, _function_id, _parameters));
      }
   }
   else
   {
      return fu_bindingRef(new fu_binding(_HLSMgr, _function_id, _parameters));
   }
}

void fu_binding::bind(const vertex& v, unsigned int unit, unsigned int index)
{
   const auto key = std::make_pair(unit, index);
   if(!unique_table.count(key))
   {
      unique_table[key] =
          generic_objRef(new funit_obj(allocation_information->get_string_name(unit) + "_i" + STR(index), unit, index));
   }
   const auto statement_index = op_graph->CGetOpNodeInfo(v)->GetNodeId();
   op_binding[statement_index] = unique_table[key];
   if(!operations.count(key))
   {
      operations.insert(std::make_pair(key, OpVertexSet(op_graph)));
   }
   operations.at(key).insert(v);
   if(index != INFINITE_UINT)
   {
      update_allocation(unit, index + 1);
   }
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
   THROW_ASSERT(op_binding.count(statement_index), "vertex not preset");
   return *(GetPointer<funit_obj>(op_binding.at(statement_index)));
}

bool fu_binding::is_assigned(const vertex& v) const
{
   const auto statement_index = op_graph->CGetOpNodeInfo(v)->GetNodeId();
   return is_assigned(statement_index);
}

bool fu_binding::is_assigned(const unsigned int statement_index) const
{
   return op_binding.count(statement_index);
}

std::list<unsigned int> fu_binding::get_allocation_list() const
{
   std::list<unsigned int> allocation_list;
   for(const auto alloc : allocation_map)
   {
      if(alloc.second > 0)
      {
         allocation_list.push_back(alloc.first);
      }
   }
   return allocation_list;
}

void fu_binding::update_allocation(unsigned int unit, unsigned int number)
{
   if(number > allocation_map[unit])
   {
      allocation_map[unit] = number;
   }
}

unsigned int fu_binding::get_assign(vertex const& v) const
{
   const auto statement_index = op_graph->CGetOpNodeInfo(v)->GetNodeId();
   return get_assign(statement_index);
}

unsigned int fu_binding::get_assign(const unsigned int statement_index) const
{
   THROW_ASSERT(op_binding.find(statement_index) != op_binding.end(),
                "Operation " + TreeM->GetTreeNode(statement_index)->ToString() + " not assigned");
   THROW_ASSERT(GetPointer<funit_obj>(op_binding.find(statement_index)->second), "");
   return GetPointer<funit_obj>(op_binding.at(statement_index))->get_fu();
}

std::string fu_binding::get_fu_name(vertex const& v) const
{
   return allocation_information->get_string_name(get_assign(v));
}

unsigned int fu_binding::get_index(vertex const& v) const
{
   const auto statement_index = op_graph->CGetOpNodeInfo(v)->GetNodeId();
   return GetPointer<funit_obj>(op_binding.at(statement_index))->get_index();
}

structural_objectRef fu_binding::add_gate(const HLS_managerRef HLSMgr, const hlsRef HLS, const technology_nodeRef fu,
                                          const std::string& name, const OpVertexSet& ops,
                                          structural_objectRef clock_port, structural_objectRef reset_port) const
{
   const auto FB = HLSMgr->CGetFunctionBehavior(HLS->functionId);
   const auto data = FB->CGetOpGraph(FunctionBehavior::CFG);
   const auto& SM = HLS->datapath;
   const auto circuit = SM->get_circ();
   structural_objectRef curr_gate(new component_o(debug_level, circuit));
   /// creating structural_manager starting from technology_node
   structural_objectRef curr_lib_instance;
   if(GetPointerS<functional_unit>(fu)->fu_template_name == "")
   {
      curr_lib_instance = GetPointer<functional_unit>(fu)->CM->get_circ();
   }
   else
   {
      const auto template_name = GetPointer<functional_unit>(fu)->fu_template_name;
      const auto library_name = HLS->HLS_D->get_technology_manager()->get_library(template_name);
      curr_lib_instance =
          GetPointerS<functional_unit>(GetPointerS<functional_unit_template>(
                                           HLS->HLS_D->get_technology_manager()->get_fu(template_name, library_name))
                                           ->FU)
              ->CM->get_circ();
   }
   THROW_ASSERT(curr_lib_instance, "structural description not provided: check the library given. Component: " +
                                       GetPointerS<functional_unit>(fu)->functional_unit_name);

   curr_lib_instance->copy(curr_gate);
   if(ops.size() == 1)
   {
      curr_gate->set_id("fu_" + GET_NAME(data, *(ops.begin())));
   }
   else
   {
      THROW_ASSERT(!name.empty(), "cannot name the added gate if the name is empty");
      curr_gate->set_id(name);
   }

   /// connecting clock and reset ports, if any
   const auto port_ck = curr_gate->find_member(CLOCK_PORT_NAME, port_o_K, curr_gate);
   if(port_ck)
   {
      SM->add_connection(clock_port, port_ck);
   }
   const auto port_rst = curr_gate->find_member(RESET_PORT_NAME, port_o_K, curr_gate);
   if(port_rst)
   {
      SM->add_connection(reset_port, port_rst);
   }
   GetPointerS<module>(circuit)->add_internal_object(curr_gate);

   return curr_gate;
}

void fu_binding::kill_proxy_memory_units(std::map<unsigned int, unsigned int>& memory_units,
                                         structural_objectRef curr_gate,
                                         std::map<unsigned int, std::list<structural_objectRef>>& var_call_sites_rel,
                                         std::map<unsigned int, unsigned int>& reverse_memory_units)
{
   /// compute the set of killing vars
   OrderedSetStd<unsigned int> killing_vars;
   const auto it_mu_end = memory_units.end();
   for(auto it_mu = memory_units.begin(); it_mu != it_mu_end; ++it_mu)
   {
      killing_vars.insert(it_mu->second);
      reverse_memory_units[it_mu->second] = it_mu->first;
   }
   for(const auto kv : killing_vars)
   {
      structural_objectRef port_proxy_in1 = curr_gate->find_member("proxy_in1_" + STR(kv), port_o_K, curr_gate);
      if(port_proxy_in1)
      {
         var_call_sites_rel[kv].push_back(curr_gate);
         GetPointer<port_o>(port_proxy_in1)->set_is_memory(false);
         for(const auto p_name :
             {"proxy_in2_", "proxy_in2r_", "proxy_in2w_", "proxy_in3_", "proxy_in3r_", "proxy_in3w_", "proxy_out1_",
              "proxy_sel_LOAD_", "proxy_sel_STORE_", "proxy_in4r_", "proxy_in4w_"})
         {
            const auto port = curr_gate->find_member(p_name + STR(kv), port_o_K, curr_gate);
            if(port)
            {
               GetPointerS<port_o>(port)->set_is_memory(false);
            }
         }
      }
   }
}

void fu_binding::kill_proxy_function_units(std::map<unsigned int, std::string>& wrapped_units,
                                           structural_objectRef curr_gate,
                                           std::map<std::string, std::list<structural_objectRef>>& fun_call_sites_rel,
                                           std::map<std::string, unsigned int>& reverse_wrapped_units)
{
   /// compute the set of killing functions
   OrderedSetStd<std::string> killing_funs;
   const auto it_mu_end = wrapped_units.end();
   for(auto it_mu = wrapped_units.begin(); it_mu != it_mu_end; ++it_mu)
   {
      killing_funs.insert(it_mu->second);
      reverse_wrapped_units[it_mu->second] = it_mu->first;
   }
   for(const auto& fun_name : killing_funs)
   {
      auto inPortSize = static_cast<unsigned int>(GetPointer<module>(curr_gate)->get_in_port_size());
      for(unsigned int currentPort = 0; currentPort < inPortSize; ++currentPort)
      {
         structural_objectRef curr_port = GetPointer<module>(curr_gate)->get_in_port(currentPort);
         if(!GetPointer<port_o>(curr_port)->get_is_memory())
         {
            continue;
         }
         std::string port_name = curr_port->get_id();
         if(starts_with(port_name, PROXY_PREFIX))
         {
            size_t found = port_name.rfind(fun_name);
            if(found != std::string::npos && found + fun_name.size() == port_name.size())
            {
               GetPointer<port_o>(curr_port)->set_is_memory(false);
               if(std::find(fun_call_sites_rel[fun_name].begin(), fun_call_sites_rel[fun_name].end(), curr_gate) ==
                  fun_call_sites_rel[fun_name].end())
               {
                  fun_call_sites_rel[fun_name].push_back(curr_gate);
               }
            }
         }
      }
      auto outPortSize = static_cast<unsigned int>(GetPointer<module>(curr_gate)->get_out_port_size());
      for(unsigned int currentPort = 0; currentPort < outPortSize; ++currentPort)
      {
         structural_objectRef curr_port = GetPointer<module>(curr_gate)->get_out_port(currentPort);
         if(!GetPointer<port_o>(curr_port)->get_is_memory())
         {
            continue;
         }
         std::string port_name = curr_port->get_id();
         size_t found = port_name.rfind(fun_name);
         if(found != std::string::npos && found + fun_name.size() == port_name.size())
         {
            GetPointer<port_o>(curr_port)->set_is_memory(false);
            if(std::find(fun_call_sites_rel[fun_name].begin(), fun_call_sites_rel[fun_name].end(), curr_gate) ==
               fun_call_sites_rel[fun_name].end())
            {
               fun_call_sites_rel[fun_name].push_back(curr_gate);
            }
         }
      }
   }
}

void fu_binding::manage_killing_memory_proxies(
    std::map<unsigned int, structural_objectRef>& mem_obj, std::map<unsigned int, unsigned int>& reverse_memory_units,
    std::map<unsigned int, std::list<structural_objectRef>>& var_call_sites_rel, const structural_managerRef SM,
    const hlsRef HLS, unsigned int& _unique_id)
{
   const auto circuit = SM->get_circ();
   for(const auto& vcsr : var_call_sites_rel)
   {
      const auto& var = vcsr.first;
      const auto& proxies = vcsr.second;

      THROW_ASSERT(reverse_memory_units.count(var), "var not found");
      const auto storage_fu_id = reverse_memory_units.at(var);
      THROW_ASSERT(mem_obj.count(storage_fu_id), "storage_fu not found: " + STR(storage_fu_id));
      const auto storage_fu = mem_obj.at(storage_fu_id);
      const auto storage_port_out = storage_fu->find_member("proxy_out1", port_o_K, storage_fu);
      THROW_ASSERT(storage_port_out, "missing proxy_out1 port");
      const auto storage_port_out_sign = [&]() {
         if(storage_port_out->get_kind() == port_vector_o_K)
         {
            return SM->add_sign_vector("S" + storage_port_out->get_id() + "_" + STR(var),
                                       GetPointerS<port_o>(storage_port_out)->get_ports_size(), circuit,
                                       storage_port_out->get_typeRef());
         }
         return SM->add_sign("S" + storage_port_out->get_id() + "_" + STR(var), circuit,
                             storage_port_out->get_typeRef());
      }();
      SM->add_connection(storage_port_out_sign, storage_port_out);

      const auto proxy_ports = [&]() {
         std::vector<structural_objectRef> ports;
         for(const auto& pname : {"proxy_in1", "proxy_in2", "proxy_in2r", "proxy_in2w", "proxy_in3", "proxy_in3r",
                                  "proxy_in3w", "proxy_sel_LOAD", "proxy_sel_STORE", "proxy_in4r", "proxy_in4w"})
         {
            const auto port = storage_fu->find_member(pname, port_o_K, storage_fu);
            if(port)
            {
               ports.push_back(port);
            }
         }
         return ports;
      }();
      std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter> to_be_merged;
      const auto var_suffix = "_" + STR(var);
      for(const auto& proxied_unit : proxies)
      {
         const auto port_out = proxied_unit->find_member("proxy_out1" + var_suffix, port_o_K, proxied_unit);
         THROW_ASSERT(port_out, "missing proxied proxy_out1 port");
         SM->add_connection(storage_port_out_sign, port_out);

         for(const auto& pport : proxy_ports)
         {
            const auto port = proxied_unit->find_member(pport->get_id() + var_suffix, port_o_K, proxied_unit);
            if(port)
            {
               to_be_merged[pport].push_back(port);
            }
         }
      }
      join_merge_split(SM, HLS, to_be_merged, circuit, _unique_id);
   }
}

void fu_binding::manage_killing_function_proxies(
    std::map<unsigned int, structural_objectRef>& fun_obj, std::map<std::string, unsigned int>& reverse_function_units,
    std::map<std::string, std::list<structural_objectRef>>& fun_call_sites_rel, const structural_managerRef SM,
    const hlsRef HLS, unsigned int& _unique_id)
{
   const auto circuit = SM->get_circ();
   for(const auto& fcsr : fun_call_sites_rel)
   {
      const auto& fun = fcsr.first;
      const auto& proxies = fcsr.second;
      THROW_ASSERT(reverse_function_units.count(fun), "fun not found");
      const auto wrapped_fu_unit_id = reverse_function_units.at(fun);
      THROW_ASSERT(fun_obj.count(wrapped_fu_unit_id), "wrapped_fu_unit not found");
      const auto wrapped_fu_unit = fun_obj.at(wrapped_fu_unit_id);
      std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter> to_be_merged;

      const auto inPortSize = GetPointer<module>(wrapped_fu_unit)->get_in_port_size();
      for(auto currentPort = 0U; currentPort < inPortSize; ++currentPort)
      {
         const auto curr_port = GetPointerS<module>(wrapped_fu_unit)->get_in_port(currentPort);
         const auto port_name = curr_port->get_id();
         if(starts_with(port_name, PROXY_PREFIX))
         {
            for(const auto& proxied_unit : proxies)
            {
               const auto port_proxy_in_i = proxied_unit->find_member(port_name + "_" + fun, port_o_K, proxied_unit);
               if(port_proxy_in_i)
               {
                  to_be_merged[curr_port].push_back(port_proxy_in_i);
               }
            }
         }
      }
      const auto outPortSize = GetPointer<module>(wrapped_fu_unit)->get_out_port_size();
      for(auto currentPort = 0U; currentPort < outPortSize; ++currentPort)
      {
         const auto wrapped_port_proxy_out_i = GetPointerS<module>(wrapped_fu_unit)->get_out_port(currentPort);
         const auto port_name = wrapped_port_proxy_out_i->get_id();
         if(starts_with(port_name, PROXY_PREFIX))
         {
            structural_objectRef wrapped_port_proxy_out_i_sign;
            for(const auto& proxied_unit : proxies)
            {
               const auto port_proxy_out_i = proxied_unit->find_member(port_name + "_" + fun, port_o_K, proxied_unit);
               if(port_proxy_out_i)
               {
                  if(!wrapped_port_proxy_out_i_sign)
                  {
                     if(wrapped_port_proxy_out_i->get_kind() == port_vector_o_K)
                     {
                        wrapped_port_proxy_out_i_sign =
                            SM->add_sign_vector(wrapped_port_proxy_out_i->get_id() + "_" + fun,
                                                GetPointer<port_o>(wrapped_port_proxy_out_i)->get_ports_size(), circuit,
                                                wrapped_port_proxy_out_i->get_typeRef());
                     }
                     else
                     {
                        wrapped_port_proxy_out_i_sign = SM->add_sign(wrapped_port_proxy_out_i->get_id() + "_" + fun,
                                                                     circuit, wrapped_port_proxy_out_i->get_typeRef());
                     }
                     SM->add_connection(wrapped_port_proxy_out_i_sign, wrapped_port_proxy_out_i);
                  }
                  SM->add_connection(wrapped_port_proxy_out_i_sign, port_proxy_out_i);
               }
            }
         }
      }
      join_merge_split(SM, HLS, to_be_merged, circuit, _unique_id);
   }
}

void fu_binding::add_to_SM(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef clock_port,
                           structural_objectRef reset_port)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding functional units to circuit");
   const auto SM = HLS->datapath;
   const auto TechM = HLS->HLS_D->get_technology_manager();

   /// unique id identifier
   unsigned int unique_id = 0U;

   /// initialize resource sharing to false
   has_resource_sharing_p = !HLS->Rreg->is_all_regs_without_enable(); // it assumes that HLS->Rreg->add_to_SM is called
                                                                      // first and then HLS->Rfu->add_to_SM

   const auto circuit = SM->get_circ();

   std::list<structural_objectRef> memory_modules;

   /// add the MEMCPY_STD component when parameters has to be copied into the local store
   const auto FB = HLSMgr->CGetFunctionBehavior(HLS->functionId);
   const auto function_parameters = FB->CGetBehavioralHelper()->get_parameters();
   unsigned int sign_id = 0;
   const auto start_port = GetPointer<module>(circuit)->find_member(START_PORT_NAME, port_o_K, circuit);
   const auto done_port = GetPointer<module>(circuit)->find_member(DONE_PORT_NAME, port_o_K, circuit);
   structural_objectRef in_chain = start_port;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding parameter ports");
   for(const auto& function_parameter : function_parameters)
   {
      if(HLSMgr->Rmem->is_parm_decl_copied(function_parameter) &&
         !HLSMgr->Rmem->is_parm_decl_stored(function_parameter))
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Managing parameter copy: " + STR(function_parameter));
         const auto fu_lib_unit = TechM->get_fu(MEMCPY_STD, WORK_LIBRARY);
         THROW_ASSERT(fu_lib_unit,
                      "functional unit not available: check the library given. Component: " + std::string(MEMCPY_STD));
         const auto curr_gate = add_gate(HLSMgr, HLS, fu_lib_unit, "parameter_manager_" + STR(function_parameter),
                                         OpVertexSet(op_graph), clock_port, reset_port);
         const auto curr_gate_m = GetPointerS<module>(curr_gate);
         const auto port_obj = HLS->Rconn->get_port(function_parameter, conn_binding::IN);
         const auto in_par = port_obj->get_out_sign();
         const auto src = curr_gate_m->find_member("src", port_o_K, curr_gate);
         SM->add_connection(in_par, src);
         const auto dest = curr_gate_m->find_member("dest", port_o_K, curr_gate);

         const auto const_obj = SM->add_module_from_technology_library(
             "memcpy_dest_" + HLSMgr->Rmem->get_symbol(function_parameter, HLS->functionId)->get_symbol_name(),
             CONSTANT_STD, LIBRARY_STD, circuit, TechM);
         const_obj->SetParameter("value",
                                 HLSMgr->Rmem->get_symbol(function_parameter, HLS->functionId)->get_symbol_name());
         const auto name = "out_const_memcpy_dest_" +
                           HLSMgr->Rmem->get_symbol(function_parameter, HLS->functionId)->get_symbol_name();
         const auto dest_sign = SM->add_sign(name, circuit, dest->get_typeRef());
         const auto out_port = const_obj->find_member("out1", port_o_K, const_obj);
         // customize output port size
         out_port->type_resize(STD_GET_SIZE(dest->get_typeRef()));
         SM->add_connection(dest_sign, out_port);
         SM->add_connection(dest, dest_sign);
         const auto n = curr_gate_m->find_member("len", port_o_K, curr_gate);
         const auto n_obj = SM->add_constant(
             "constant_len_" + STR(function_parameter), circuit, n->get_typeRef(),
             STR(tree_helper::SizeAlloc(tree_helper::CGetType(TreeM->GetTreeNode(function_parameter))) / 8));
         SM->add_connection(n, n_obj);
         THROW_ASSERT(in_chain, "missing in chain element");
         const auto start_obj = curr_gate_m->find_member(START_PORT_NAME, port_o_K, curr_gate);

         if(HLS->registered_inputs && in_chain == start_port)
         {
            const auto delay_unit = [&]() {
               const auto reset_type = parameters->getOption<std::string>(OPT_reset_type);
               return TechM->get_fu(reset_type == "sync" ? flipflop_SR : flipflop_AR, LIBRARY_STD);
            }();
            THROW_ASSERT(delay_unit, "");
            const auto delay_gate = add_gate(HLSMgr, HLS, delay_unit, "start_delayed_" + STR(function_parameter),
                                             OpVertexSet(op_graph), clock_port, reset_port);
            const auto sign =
                SM->add_sign(START_PORT_NAME + STR("_") + STR(sign_id), circuit, start_obj->get_typeRef());
            ++sign_id;
            SM->add_connection(sign, in_chain);
            SM->add_connection(sign, GetPointer<module>(delay_gate)->get_in_port(2));
         }

         if(in_chain == start_port)
         {
            SM->add_connection(in_chain, start_obj);
         }
         else
         {
            const auto sign = SM->add_sign(START_PORT_NAME + STR("_") + STR(sign_id), circuit, in_chain->get_typeRef());
            ++sign_id;
            SM->add_connection(sign, in_chain);
            SM->add_connection(sign, start_obj);
         }
         in_chain = curr_gate_m->find_member(DONE_PORT_NAME, port_o_K, curr_gate);
         manage_module_ports(HLSMgr, HLS, SM, curr_gate, 0);
         memory_modules.push_back(curr_gate);
      }
      else if(HLSMgr->Rmem->is_parm_decl_stored(function_parameter))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "---Managing parameter initialization: " + STR(function_parameter));
         auto bus_data_bitsize = HLSMgr->Rmem->get_bus_data_bitsize();
         auto bus_addr_bitsize = HLSMgr->get_address_bitsize();
         auto bus_size_bitsize = HLSMgr->Rmem->get_bus_size_bitsize();
         unsigned long long bus_tag_bitsize = 0;
         if(HLS->Param->getOption<bool>(OPT_parse_pragma) && HLS->Param->isOption(OPT_context_switch))
         {
            bus_tag_bitsize = GetPointer<memory_cs>(HLSMgr->Rmem)->get_bus_tag_bitsize();
         }
         const auto is_multiport = FB->GetChannelsType() == MemoryAllocation_ChannelsType::MEM_ACC_NN;
         const auto fu_lib_unit = TechM->get_fu(is_multiport ? MEMSTORE_STDN : MEMSTORE_STD, LIBRARY_STD_FU);
         THROW_ASSERT(fu_lib_unit, "functional unit not available: check the library given. Component: " +
                                       STR(is_multiport ? MEMSTORE_STDN : MEMSTORE_STD));
         const auto max_n_ports = FB->GetChannelsNumber();
         const auto curr_gate = add_gate(HLSMgr, HLS, fu_lib_unit, "parameter_manager_" + STR(function_parameter),
                                         OpVertexSet(op_graph), clock_port, reset_port);
         const auto port_obj = HLS->Rconn->get_port(function_parameter, conn_binding::IN);
         const auto in_par = port_obj->get_out_sign();
         const auto data = GetPointer<module>(curr_gate)->find_member("data", port_o_K, curr_gate);
         data->type_resize(STD_GET_SIZE(in_par->get_typeRef()));
         SM->add_connection(in_par, data);

         const auto size = GetPointer<module>(curr_gate)->find_member("size", port_o_K, curr_gate);
         size->type_resize(STD_GET_SIZE(in_par->get_typeRef()));
         const auto size_const_obj = SM->add_module_from_technology_library(
             "size_par_" + HLSMgr->Rmem->get_symbol(function_parameter, HLS->functionId)->get_symbol_name(),
             CONSTANT_STD, LIBRARY_STD, circuit, TechM);
         const std::string parameter_value =
             (parameters->getOption<HDLWriter_Language>(OPT_writer_language) == HDLWriter_Language::VHDL) ?
                 std::string("\"") +
                     NumberToBinaryString(STD_GET_SIZE(in_par->get_typeRef()), STD_GET_SIZE(in_par->get_typeRef())) +
                     std::string("\"") :
                 STR(STD_GET_SIZE(in_par->get_typeRef()));
         size_const_obj->SetParameter("value", parameter_value);
         const auto size_name =
             "out_const_size_par_" + HLSMgr->Rmem->get_symbol(function_parameter, HLS->functionId)->get_symbol_name();
         const auto size_sign = SM->add_sign(size_name, circuit, size->get_typeRef());
         const auto size_out_port = size_const_obj->find_member("out1", port_o_K, size_const_obj);
         // customize output port size
         size_out_port->type_resize(STD_GET_SIZE(in_par->get_typeRef()));
         SM->add_connection(size_sign, size_out_port);
         SM->add_connection(size, size_sign);

         const auto addr = GetPointer<module>(curr_gate)->find_member("addr", port_o_K, curr_gate);
         addr->type_resize(bus_addr_bitsize);
         const auto const_obj = SM->add_module_from_technology_library(
             "addr_par_" + HLSMgr->Rmem->get_symbol(function_parameter, HLS->functionId)->get_symbol_name(),
             CONSTANT_STD, LIBRARY_STD, circuit, TechM);
         const_obj->SetParameter("value",
                                 HLSMgr->Rmem->get_symbol(function_parameter, HLS->functionId)->get_symbol_name());
         const auto name =
             "out_const_addr_par_" + HLSMgr->Rmem->get_symbol(function_parameter, HLS->functionId)->get_symbol_name();
         const auto addr_sign = SM->add_sign(name, circuit, addr->get_typeRef());
         const auto out_port = const_obj->find_member("out1", port_o_K, const_obj);
         // customize output port size
         out_port->type_resize(bus_addr_bitsize);
         SM->add_connection(addr_sign, out_port);
         SM->add_connection(addr, addr_sign);

         THROW_ASSERT(in_chain, "missing in chain element");
         structural_objectRef start_obj =
             GetPointer<module>(curr_gate)->find_member(START_PORT_NAME, port_o_K, curr_gate);
         if(HLS->registered_inputs && in_chain == start_port)
         {
            technology_nodeRef delay_unit;
            auto reset_type = parameters->getOption<std::string>(OPT_reset_type);
            if(reset_type == "sync")
            {
               delay_unit = TechM->get_fu(flipflop_SR, LIBRARY_STD);
            }
            else
            {
               delay_unit = TechM->get_fu(flipflop_AR, LIBRARY_STD);
            }
            structural_objectRef delay_gate =
                add_gate(HLSMgr, HLS, delay_unit, "start_delayed_" + STR(function_parameter), OpVertexSet(op_graph),
                         clock_port, reset_port);
            structural_objectRef sign =
                SM->add_sign(START_PORT_NAME + STR("_") + STR(sign_id), circuit, start_obj->get_typeRef());
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
            structural_objectRef sign =
                SM->add_sign(START_PORT_NAME + STR("_") + STR(sign_id), circuit, in_chain->get_typeRef());
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
            {
               GetPointer<port_o>(port)->add_n_ports(static_cast<unsigned int>(max_n_ports), port);
            }
            port_o::resize_if_busport(bus_size_bitsize, bus_addr_bitsize, bus_data_bitsize, bus_tag_bitsize, port);
         }
         for(unsigned int i = 0; i < GetPointer<module>(curr_gate)->get_out_port_size(); i++)
         {
            structural_objectRef port = GetPointer<module>(curr_gate)->get_out_port(i);
            if(is_multiport && port->get_kind() == port_vector_o_K && GetPointer<port_o>(port)->get_ports_size() == 0)
            {
               GetPointer<port_o>(port)->add_n_ports(static_cast<unsigned int>(max_n_ports), port);
            }
            port_o::resize_if_busport(bus_size_bitsize, bus_addr_bitsize, bus_data_bitsize, bus_tag_bitsize, port);
         }
         manage_module_ports(HLSMgr, HLS, SM, curr_gate, 0);
         memory_modules.push_back(curr_gate);
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added parameter ports");
   if(in_chain)
   {
      SM->add_connection(in_chain, done_port);
   }
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
      structural_objectRef start_CFC =
          SM->add_port(START_PORT_NAME_CFC, port_o::IN, circuit,
                       structural_type_descriptorRef(new structural_type_descriptor("bool", 0)));
      SM->add_connection(start_CFC, controller_flow_start);
      structural_objectRef controller_flow_done = curr_gate->find_member(DONE_PORT_NAME, port_o_K, curr_gate);
      structural_objectRef done_CFC =
          SM->add_port(DONE_PORT_NAME_CFC, port_o::IN, circuit,
                       structural_type_descriptorRef(new structural_type_descriptor("bool", 0)));
      SM->add_connection(done_CFC, controller_flow_done);
      structural_objectRef controller_flow_present_state =
          curr_gate->find_member(PRESENT_STATE_PORT_NAME, port_o_K, curr_gate);
      structural_objectRef controller_present_state =
          SM->add_port(PRESENT_STATE_PORT_NAME, port_o::IN, circuit, controller_flow_present_state->get_typeRef());
      SM->add_connection(controller_present_state, controller_flow_present_state);
      structural_objectRef controller_flow_next_state =
          curr_gate->find_member(NEXT_STATE_PORT_NAME, port_o_K, curr_gate);
      structural_objectRef controller_next_state =
          SM->add_port(NEXT_STATE_PORT_NAME, port_o::IN, circuit, controller_flow_next_state->get_typeRef());
      SM->add_connection(controller_next_state, controller_flow_next_state);
      memory_modules.push_back(curr_gate);
   }

   const auto is_sparse_memory =
       parameters->isOption(OPT_sparse_memory) && parameters->getOption<bool>(OPT_sparse_memory);
   std::map<unsigned int, unsigned int> memory_units = allocation_information->get_memory_units();
   std::map<unsigned int, structural_objectRef> mem_obj;
   for(const auto& m : memory_units)
   {
      const auto fu_type_id = m.first;
      auto var = m.second;
      std::string name;
      const auto fun_unit_name = allocation_information->get_fu_name(fu_type_id).first;
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
      const auto is_splitted_memory = [&]() {
         const auto& memory_type = GetPointerS<const functional_unit>(fu_lib_unit)->memory_type;
         const auto& channels_type = GetPointerS<const functional_unit>(fu_lib_unit)->channels_type;
         return memory_type == MEMORY_TYPE_SYNCHRONOUS_UNALIGNED &&
                (channels_type == CHANNELS_TYPE_MEM_ACC_N1 || channels_type == CHANNELS_TYPE_MEM_ACC_NN);
      }();
      const auto is_sds_memory = [&]() {
         const auto& memory_type = GetPointerS<const functional_unit>(fu_lib_unit)->memory_type;
         return memory_type == MEMORY_TYPE_SYNCHRONOUS_SDS || memory_type == MEMORY_TYPE_SYNCHRONOUS_SDS_BUS ||
                memory_type == MEMORY_TYPE_ASYNCHRONOUS;
      }();

      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                    "Memory Unit: " + allocation_information->get_string_name(fu_type_id) + " for variable: " +
                        HLSMgr->CGetFunctionBehavior(HLS->functionId)->CGetBehavioralHelper()->PrintVariable(var));
      const auto base_address = HLSMgr->Rmem->get_symbol(var, HLS->functionId)->get_symbol_name();
      const auto rangesize = HLSMgr->Rmem->get_rangesize(var);
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  - base address: " + STR(base_address));
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  - range size: " + STR(rangesize));
      const auto n_channels = allocation_information->get_number_channels(fu_type_id);
      const auto total_allocated = get_number(fu_type_id);
      const auto n_iterations = std::max(1u, total_allocated);
      for(unsigned int num = 0; num < n_iterations; num += n_channels)
      {
         OpVertexSet operations_set(op_graph);
         for(unsigned int channel_index = 0; channel_index < n_channels && (num + channel_index < total_allocated);
             ++channel_index)
         {
            auto key = std::make_pair(fu_type_id, num + channel_index);
            if(operations.find(key) != operations.end())
            {
               auto opset = operations.at(key);
               operations_set.insert(opset.begin(), opset.end());
            }
         }
         has_resource_sharing_p = has_resource_sharing_p || (operations_set.size() > 1);
         structural_objectRef curr_gate = add_gate(HLSMgr, HLS, fu_lib_unit, name + "_" + STR(num / n_channels),
                                                   OpVertexSet(op_graph), clock_port, reset_port);
         specialise_fu(HLSMgr, HLS, curr_gate, fu_type_id, operations_set, var);
         specialize_memory_unit(HLSMgr, HLS, curr_gate, var, base_address, rangesize, is_splitted_memory,
                                is_sparse_memory, is_sds_memory);
         check_parametrization(curr_gate);
         mem_obj[fu_type_id] = curr_gate;
         for(unsigned int channel_index = 0; (channel_index < n_channels) && ((num + channel_index) < total_allocated);
             ++channel_index)
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
      const auto fu_lib_unit = allocation_information->get_fu(wu->first);
      THROW_ASSERT(fu_lib_unit, "functional unit not available: check the library given. Component: " +
                                    allocation_information->get_fu_name(wu->first).first);
      const auto curr_gate =
          add_gate(HLSMgr, HLS, fu_lib_unit, wu->second + "_instance", OpVertexSet(op_graph), clock_port, reset_port);
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                    "Wrapped Unit: " + allocation_information->get_string_name(wu->first));
      const auto mapped_operations = get_operations(wu->first, 0);
      specialise_fu(HLSMgr, HLS, curr_gate, wu->first, mapped_operations, 0);
      check_parametrization(curr_gate);
      fun_obj[wu->first] = curr_gate;
      kill_proxy_memory_units(memory_units, curr_gate, var_call_sites_rel, reverse_memory_units);
      kill_proxy_function_units(wrapped_units, curr_gate, fun_call_sites_rel, reverse_function_units);
      const auto added_memory_element = manage_module_ports(HLSMgr, HLS, SM, curr_gate, 0);
      if(added_memory_element)
      {
         memory_modules.push_back(curr_gate);
      }
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
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "-->Functional Unit: " + allocation_information->get_string_name(i));
      if(allocation_information->is_return(i))
      {
         const auto obj = circuit->find_member(RETURN_PORT_NAME, port_o_K, circuit);
         get(i, 0)->set_structural_obj(obj);
      }

      if(!allocation_information->has_to_be_synthetized(i))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--It has not to be synthesized");
         continue;
      }
      const auto is_multichannel = allocation_information->get_number_channels(i) > 1;

      for(unsigned int num = 0; num < get_number(i); num++)
      {
         const auto module_obj = get(i, num);
         THROW_ASSERT(module_obj,
                      "module missing: " + allocation_information->get_fu_name(i).first + " instance " + STR(num));
         const auto name = module_obj->get_string();

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
            const auto n_channels = allocation_information->get_number_channels(i);
            const auto true_module_obj = get(i, (num / n_channels) * n_channels);
            curr_gate = true_module_obj->get_structural_obj();
            const auto mapped_operations = get_operations(i, num);
            has_resource_sharing_p = has_resource_sharing_p || (mapped_operations.size() > 1);
            const auto ar_var =
                allocation_information->is_proxy_memory_unit(i) ? allocation_information->get_proxy_memory_var(i) : 0;
            specialise_fu(HLSMgr, HLS, curr_gate, i, mapped_operations, ar_var);
            module_obj->set_structural_obj(curr_gate);
         }
         else
         {
            const auto fu_lib_unit = allocation_information->get_fu(i);
            const auto mapped_operations = get_operations(i, num);
            THROW_ASSERT(fu_lib_unit, "functional unit not available: check the library given. Component: " +
                                          allocation_information->get_fu_name(i).first);
            curr_gate = add_gate(HLSMgr, HLS, fu_lib_unit, name,
                                 allocation_information->is_direct_proxy_memory_unit(i) ||
                                         allocation_information->is_indirect_access_memory_unit(i) ?
                                     OpVertexSet(op_graph) :
                                     mapped_operations,
                                 clock_port, reset_port);
            has_resource_sharing_p = has_resource_sharing_p || (mapped_operations.size() > 1);
            std::string current_op;
            if(mapped_operations.size())
            {
               current_op = tree_helper::NormalizeTypename(
                   op_graph->CGetOpNodeInfo(*(mapped_operations.begin()))->GetOperation());
            }
            if(current_op == BUILTIN_WAIT_CALL)
            {
               has_resource_sharing_p = true;
               const auto site = *mapped_operations.begin();
               const auto vertex_node_id = op_graph->CGetOpNodeInfo(site)->GetNodeId();

               const auto callSiteMemorySym = HLSMgr->Rmem->get_symbol(vertex_node_id, HLS->functionId);
               memory::add_memory_parameter(HLS->datapath, callSiteMemorySym->get_symbol_name(),
                                            STR(callSiteMemorySym->get_address()));
            }
            const auto ar_var =
                allocation_information->is_proxy_memory_unit(i) ? allocation_information->get_proxy_memory_var(i) : 0;
            specialise_fu(HLSMgr, HLS, curr_gate, i, mapped_operations, ar_var);
            check_parametrization(curr_gate);
            if(proxy_memory_units.find(i) != proxy_memory_units.end())
            {
               proxy_memory_units_to_be_renamed_back.push_back(
                   std::make_pair(curr_gate, proxy_memory_units.find(i)->second));
               std::string var_name = "_" + STR(proxy_memory_units.find(i)->second);

               /// rename proxy ports
               const auto port_proxy_in1 = curr_gate->find_member("proxy_in1", port_o_K, curr_gate);
               port_proxy_in1->set_id(port_proxy_in1->get_id() + var_name);
               const auto is_dual = allocation_information->is_dual_port_memory(i);
               if(is_dual)
               {
                  const auto port_proxy_in2r = curr_gate->find_member("proxy_in2r", port_o_K, curr_gate);
                  const auto port_proxy_in2w = curr_gate->find_member("proxy_in2w", port_o_K, curr_gate);
                  const auto port_proxy_in3r = curr_gate->find_member("proxy_in3r", port_o_K, curr_gate);
                  const auto port_proxy_in3w = curr_gate->find_member("proxy_in3w", port_o_K, curr_gate);
                  const auto port_proxy_in4r = curr_gate->find_member("proxy_in4r", port_o_K, curr_gate);
                  const auto port_proxy_in4w = curr_gate->find_member("proxy_in4w", port_o_K, curr_gate);
                  port_proxy_in2r->set_id(port_proxy_in2r->get_id() + var_name);
                  port_proxy_in2w->set_id(port_proxy_in2w->get_id() + var_name);
                  port_proxy_in3r->set_id(port_proxy_in3r->get_id() + var_name);
                  port_proxy_in3w->set_id(port_proxy_in3w->get_id() + var_name);
                  port_proxy_in4r->set_id(port_proxy_in4r->get_id() + var_name);
                  port_proxy_in4w->set_id(port_proxy_in4w->get_id() + var_name);
               }
               else
               {
                  const auto port_proxy_in2 = curr_gate->find_member("proxy_in2", port_o_K, curr_gate);
                  const auto port_proxy_in3 = curr_gate->find_member("proxy_in3", port_o_K, curr_gate);
                  port_proxy_in2->set_id(port_proxy_in2->get_id() + var_name);
                  port_proxy_in3->set_id(port_proxy_in3->get_id() + var_name);
               }
               const auto port_sel_LOAD = curr_gate->find_member("proxy_sel_LOAD", port_o_K, curr_gate);
               const auto port_sel_STORE = curr_gate->find_member("proxy_sel_STORE", port_o_K, curr_gate);
               const auto port_proxy_out1 = curr_gate->find_member("proxy_out1", port_o_K, curr_gate);
               port_sel_LOAD->set_id(port_sel_LOAD->get_id() + var_name);
               port_sel_STORE->set_id(port_sel_STORE->get_id() + var_name);
               port_proxy_out1->set_id(port_proxy_out1->get_id() + var_name);
            }
            kill_proxy_memory_units(memory_units, curr_gate, var_call_sites_rel, reverse_memory_units);

            if(proxy_function_units.find(i) != proxy_function_units.end())
            {
               std::string fun_name = "_" + STR(proxy_function_units.find(i)->second);
               proxy_function_units_to_be_renamed_back.push_back(
                   std::make_pair(curr_gate, proxy_function_units.find(i)->second));
               auto inPortSize = static_cast<unsigned int>(GetPointer<module>(curr_gate)->get_in_port_size());
               for(unsigned int currentPort = 0; currentPort < inPortSize; ++currentPort)
               {
                  structural_objectRef curr_port = GetPointer<module>(curr_gate)->get_in_port(currentPort);
                  if(!GetPointer<port_o>(curr_port)->get_is_memory())
                  {
                     continue;
                  }
                  std::string port_name = curr_port->get_id();
                  if(starts_with(port_name, PROXY_PREFIX))
                  {
                     GetPointer<port_o>(curr_port)->set_id(port_name + fun_name);
                  }
               }
               auto outPortSize = static_cast<unsigned int>(GetPointer<module>(curr_gate)->get_out_port_size());
               for(unsigned int currentPort = 0; currentPort < outPortSize; ++currentPort)
               {
                  structural_objectRef curr_port = GetPointer<module>(curr_gate)->get_out_port(currentPort);
                  if(!GetPointer<port_o>(curr_port)->get_is_memory())
                  {
                     continue;
                  }
                  std::string port_name = curr_port->get_id();
                  if(starts_with(port_name, PROXY_PREFIX))
                  {
                     GetPointer<port_o>(curr_port)->set_id(port_name + fun_name);
                  }
               }
            }
            kill_proxy_function_units(wrapped_units, curr_gate, fun_call_sites_rel, reverse_function_units);

            bool added_memory_element = manage_module_ports(HLSMgr, HLS, SM, curr_gate, num);
            if(added_memory_element &&
               std::find(memory_modules.begin(), memory_modules.end(), curr_gate) == memory_modules.end())
            {
               memory_modules.push_back(curr_gate);
            }

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
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, HLS->output_level,
                     "---Resources are not shared in function " +
                         HLSMgr->CGetFunctionBehavior(HLS->functionId)->CGetBehavioralHelper()->get_function_name() +
                         ": function pipelining may come for free");
   }

   const auto cg_man = HLSMgr->CGetCallGraphManager();
   if(cg_man->GetRootFunctions().count(HLS->functionId) &&
      (cg_man->ExistsAddressedFunction() ||
       HLSMgr->unused_interfaces.find(HLS->functionId) != HLSMgr->unused_interfaces.end()))
   {
      const auto addressed_functions = cg_man->GetAddressedFunctions();
      const auto constBitZero =
          SM->add_module_from_technology_library("constBitZero", CONSTANT_STD, LIBRARY_STD, circuit, TechM);
      const auto signBitZero =
          SM->add_sign("bitZero", circuit, circuit->find_member(CLOCK_PORT_NAME, port_o_K, circuit)->get_typeRef());
      SM->add_connection(signBitZero, constBitZero->find_member("out1", port_o_K, constBitZero));

      for(const auto& f_id : addressed_functions)
      {
         const auto FUName = functions::GetFUName(tree_helper::name_function(TreeM, f_id), HLSMgr);
         if(HLSMgr->Rfuns->is_a_proxied_function(FUName))
         {
            continue;
         }

         const auto FU = SM->add_module_from_technology_library(FUName + "_i0", FUName, WORK_LIBRARY, circuit, TechM);
         if(std::find(memory_modules.begin(), memory_modules.end(), FU) == memory_modules.end())
         {
            memory_modules.push_back(FU);
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Considering additional top: " + FUName + "@" + STR(f_id));
         if(HLSMgr->Rfuns->has_proxied_shared_functions(f_id))
         {
            auto proxied_shared_functions = HLSMgr->Rfuns->get_proxied_shared_functions(f_id);
#ifndef NDEBUG
            for(const auto& name : proxied_shared_functions)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---  proxy shared function: " + name);
            }
#endif
            kill_proxy_function_units(wrapped_units, FU, fun_call_sites_rel, reverse_function_units);
         }

         SM->add_connection(FU->find_member(CLOCK_PORT_NAME, port_o_K, FU),
                            circuit->find_member(CLOCK_PORT_NAME, port_o_K, circuit));
         SM->add_connection(FU->find_member(RESET_PORT_NAME, port_o_K, FU),
                            circuit->find_member(RESET_PORT_NAME, port_o_K, circuit));

         const auto fu_start = FU->find_member(START_PORT_NAME, port_o_K, FU);
         if(fu_start)
         {
            SM->add_connection(fu_start, signBitZero);
         }

         for(const auto additional_parameter :
             HLSMgr->CGetFunctionBehavior(f_id)->CGetBehavioralHelper()->get_parameters())
         {
            const auto parameterName =
                HLSMgr->CGetFunctionBehavior(f_id)->CGetBehavioralHelper()->PrintVariable(additional_parameter);

            const auto parameterPort = FU->find_member(parameterName, port_o_K, FU);
            const auto constZeroParam = SM->add_module_from_technology_library(
                "zeroParam_" + FUName + "_" + parameterName, CONSTANT_STD, LIBRARY_STD, circuit, TechM);
            const auto constZeroOutPort = constZeroParam->find_member("out1", port_o_K, constZeroParam);
            const auto parameter_value =
                (parameters->getOption<HDLWriter_Language>(OPT_writer_language) == HDLWriter_Language::VHDL) ?
                    std::string("\"") + NumberToBinaryString(0, STD_GET_SIZE(parameterPort->get_typeRef())) +
                        std::string("\"") :
                    "0";
            constZeroParam->SetParameter("value", parameter_value);

            constZeroOutPort->type_resize(STD_GET_SIZE(parameterPort->get_typeRef()));

            const auto signBitZeroParam = SM->add_sign("signZeroParam_" + FUName + "_" + parameterName, SM->get_circ(),
                                                       constZeroOutPort->get_typeRef());

            SM->add_connection(parameterPort, signBitZeroParam);
            SM->add_connection(constZeroOutPort, signBitZeroParam);
         }
         manage_module_ports(HLSMgr, HLS, SM, FU, 0);
         memory::propagate_memory_parameters(FU, HLS->datapath);
      }
      if(HLSMgr->unused_interfaces.find(HLS->functionId) != HLSMgr->unused_interfaces.end())
      {
         const auto HLS_D = HLSMgr->get_HLS_device();
         const auto TechMan = HLS_D->get_technology_manager();
         const auto& res_pair_set = HLSMgr->unused_interfaces.at(HLS->functionId);
         for(const auto& [UPlibrary, FUName] : res_pair_set)
         {
            auto fu_name = FUName;
            technology_nodeRef fuObj = TechMan->get_fu(fu_name, UPlibrary);
            const auto structManager_obj = GetPointer<functional_unit>(fuObj)->CM;
            THROW_ASSERT(structManager_obj, "unexpected condition");
            const auto has_to_be_generated = GetPointer<module>(structManager_obj->get_circ())->has_to_be_generated();
            if(has_to_be_generated)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Unit has to be specialized.");
               const ModuleGeneratorManagerRef modGen(new ModuleGeneratorManager(HLSMgr, parameters));
               {
                  fu_name = fu_name + "_modgen";
               }
               const auto& specialized_fuName = fu_name;

               const auto check_lib = TechM->get_library(specialized_fuName);
               modGen->create_generic_module(FUName, NULL_VERTEX, FB, UPlibrary, specialized_fuName);
            }
            const auto FU = SM->add_module_from_technology_library(fu_name + "_i0", fu_name, UPlibrary, circuit, TechM);
            if(std::find(memory_modules.begin(), memory_modules.end(), FU) == memory_modules.end())
            {
               memory_modules.push_back(FU);
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding unused interface component: " + FUName);
            if(const auto clk_prt = FU->find_member(CLOCK_PORT_NAME, port_o_K, FU))
            {
               SM->add_connection(clk_prt, circuit->find_member(CLOCK_PORT_NAME, port_o_K, circuit));
            }
            if(const auto reset_prt = FU->find_member(RESET_PORT_NAME, port_o_K, FU))
            {
               SM->add_connection(reset_prt, circuit->find_member(RESET_PORT_NAME, port_o_K, circuit));
            }

            manage_module_ports(HLSMgr, HLS, SM, FU, 0);
            memory::propagate_memory_parameters(FU, HLS->datapath);
         }
      }
   }

   if(parameters->IsParameter("chained-memory-modules") && parameters->GetParameter<int>("chained-memory-modules") == 1)
   {
      manage_memory_ports_chained(SM, memory_modules, circuit);
   }
   else
   {
      manage_memory_ports_parallel_chained(HLSMgr, SM, memory_modules, circuit, HLS, unique_id);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Managed memory ports");

   /// rename back all the memory proxies ports
   for(const auto& pmutbrb : proxy_memory_units_to_be_renamed_back)
   {
      const auto curr_gate = pmutbrb.first;
      THROW_ASSERT(curr_gate, "missing structural object");
      const auto var_name = "_" + STR(pmutbrb.second);
      for(const auto& pname :
          {"proxy_in1", "proxy_in2", "proxy_in2r", "proxy_in2w", "proxy_in3", "proxy_in3r", "proxy_in3w", "proxy_in4r",
           "proxy_in4w", "proxy_sel_LOAD", "proxy_sel_STORE", "proxy_out1"})
      {
         const auto port = curr_gate->find_member(pname + var_name, port_o_K, curr_gate);
         if(port)
         {
            port->set_id(pname);
         }
      }
   }
   HLS->Rfu->manage_killing_memory_proxies(mem_obj, reverse_memory_units, var_call_sites_rel, SM, HLS, unique_id);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Managed memory proxies");

   /// rename back all the function proxies ports
   for(const auto& pfutbrb : proxy_function_units_to_be_renamed_back)
   {
      const auto& curr_gate = pfutbrb.first;
      THROW_ASSERT(curr_gate, "missing structural object");
      const auto fun_name = "_" + STR(pfutbrb.second);
      const auto inPortSize = GetPointerS<module>(curr_gate)->get_in_port_size();
      for(auto currentPort = 0U; currentPort < inPortSize; ++currentPort)
      {
         const auto curr_port = GetPointerS<module>(curr_gate)->get_in_port(currentPort);
         const auto port_name = curr_port->get_id();
         if(starts_with(port_name, PROXY_PREFIX))
         {
            const auto found = port_name.rfind(fun_name);
            if(found != std::string::npos)
            {
               const auto orig_port_name = port_name.substr(0, found);
               GetPointerS<port_o>(curr_port)->set_id(orig_port_name);
            }
         }
      }
      const auto outPortSize = GetPointerS<module>(curr_gate)->get_out_port_size();
      for(auto currentPort = 0U; currentPort < outPortSize; ++currentPort)
      {
         const auto curr_port = GetPointerS<module>(curr_gate)->get_out_port(currentPort);
         const auto port_name = curr_port->get_id();
         if(starts_with(port_name, PROXY_PREFIX))
         {
            const auto found = port_name.rfind(fun_name);
            if(found != std::string::npos)
            {
               const auto orig_port_name = port_name.substr(0, found);
               GetPointerS<port_o>(curr_port)->set_id(orig_port_name);
            }
         }
      }
   }
   HLS->Rfu->manage_killing_function_proxies(fun_obj, reverse_function_units, fun_call_sites_rel, SM, HLS, unique_id);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added functional units to circuit");
}

void fu_binding::check_parametrization(structural_objectRef
#if HAVE_ASSERTS
                                           curr_gate
#endif
)
{
#if HAVE_ASSERTS
   const auto fu_module = GetPointer<module>(curr_gate);
   const auto np = fu_module->get_NP_functionality();
   if(np)
   {
      std::vector<std::string> param;
      np->get_library_parameters(param);
      for(const auto& p : param)
      {
         THROW_ASSERT(curr_gate->find_member(p, port_o_K, curr_gate) ||
                          curr_gate->find_member(p, port_vector_o_K, curr_gate) || curr_gate->ExistsParameter(p),
                      "parameter not yet specialized: " + p + " for module " + GET_TYPE_NAME(curr_gate));
      }
   }
#endif
}

bool fu_binding::manage_module_ports(const HLS_managerRef HLSMgr, const hlsRef HLS, const structural_managerRef SM,
                                     const structural_objectRef curr_gate, unsigned int num)
{
   const auto circuit = SM->get_circ();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Routing module ports - " + circuit->get_path());
   /// creating extern IN port on datapath starting from extern ports on module
   bool added_memory_element = false;
   for(unsigned int j = 0; j < GetPointer<module>(curr_gate)->get_in_port_size(); j++)
   {
      const auto port_in = GetPointer<module>(curr_gate)->get_in_port(j);
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
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   return added_memory_element;
}

void fu_binding::manage_memory_ports_chained(const structural_managerRef SM,
                                             const std::list<structural_objectRef>& memory_modules,
                                             const structural_objectRef circuit)
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
         if(GetPointer<port_o>(port_i)->get_is_memory() && (!GetPointer<port_o>(port_i)->get_is_global()) &&
            (!GetPointer<port_o>(port_i)->get_is_extern()))
         {
            std::string port_name = GetPointer<port_o>(port_i)->get_id();
            THROW_ASSERT(port_name.substr(1, 3) == "in_", "Expected the \"?in_\" prefix:" + port_name);
            std::string key = port_name.substr(0, 1) + port_name.substr(4); /// trimmed away the "?in_"prefix
            cir_port = circuit->find_member(port_name, port_i->get_kind(), circuit);
            THROW_ASSERT(!cir_port || GetPointer<port_o>(cir_port), "should be a port or null");
            if(!cir_port)
            {
               if(port_i->get_kind() == port_vector_o_K)
               {
                  cir_port = SM->add_port_vector(port_name, port_o::IN, GetPointer<port_o>(port_i)->get_ports_size(),
                                                 circuit, port_i->get_typeRef());
               }
               else
               {
                  cir_port = SM->add_port(port_name, port_o::IN, circuit, port_i->get_typeRef());
               }
               port_o::fix_port_properties(port_i, cir_port);
               SM->add_connection(cir_port, port_i);
            }
            else
            {
               THROW_ASSERT(from_ports.find(key) != from_ports.end(),
                            "somewhere the signal " + key + " should be produced");
               structural_objectRef sign;
               if(port_i->get_kind() == port_vector_o_K)
               {
                  sign =
                      SM->add_sign_vector(key + "_" + std::to_string(sign_id),
                                          GetPointer<port_o>(port_i)->get_ports_size(), circuit, port_i->get_typeRef());
               }
               else
               {
                  sign = SM->add_sign(key + "_" + std::to_string(sign_id), circuit, port_i->get_typeRef());
               }
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
         if(GetPointer<port_o>(port_i)->get_is_memory() && (!GetPointer<port_o>(port_i)->get_is_global()) &&
            (!GetPointer<port_o>(port_i)->get_is_extern()))
         {
            std::string port_name = GetPointer<port_o>(port_i)->get_id();
            THROW_ASSERT(port_name.substr(1, 4) == "out_", "Expected the \"?out_\" prefix: " + port_name);
            std::string key = port_name.substr(0, 1) + port_name.substr(5); /// trimmed away the "?out_"prefix
            cir_port = circuit->find_member(port_name, port_i->get_kind(), circuit);
            THROW_ASSERT(!cir_port || GetPointer<port_o>(cir_port), "should be a port or null");
            if(!cir_port)
            {
               if(port_i->get_kind() == port_vector_o_K)
               {
                  cir_port = SM->add_port_vector(port_name, port_o::OUT, GetPointer<port_o>(port_i)->get_ports_size(),
                                                 circuit, port_i->get_typeRef());
               }
               else
               {
                  cir_port = SM->add_port(port_name, port_o::OUT, circuit, port_i->get_typeRef());
               }
               port_o::fix_port_properties(port_i, cir_port);
               primary_outs[port_name] = cir_port;
            }
            from_ports[key] = port_i;
         }
      }
   }
   auto po_end = primary_outs.end();
   for(auto po = primary_outs.begin(); po != po_end; ++po)
   {
      THROW_ASSERT(from_ports.find(po->first.substr(0, 1) + po->first.substr(5)) != from_ports.end(),
                   "Port source not present");
      SM->add_connection(po->second, from_ports.find(po->first.substr(0, 1) + po->first.substr(5))->second);
   }
}

void fu_binding::join_merge_split(
    const structural_managerRef SM, const hlsRef HLS,
    std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter>& primary_outs,
    const structural_objectRef circuit, unsigned int& _unique_id)
{
   static const auto js_name = "join_signal";
   static const auto ss_name = "split_signal";
   static const auto bus_merger_res_name = "bus_merger";
   const auto js_library = HLS->HLS_D->get_technology_manager()->get_library(js_name);
   const auto ss_library = HLS->HLS_D->get_technology_manager()->get_library(ss_name);
   const auto bm_library = HLS->HLS_D->get_technology_manager()->get_library(bus_merger_res_name);

   for(const auto& po : primary_outs)
   {
      const auto& bus_port = po.first;
      const auto is_vector_bus = bus_port->get_kind() == port_vector_o_K;
      const auto& merged_ports = po.second;
      const auto bus_merger_inst_name = bus_merger_res_name + bus_port->get_id() + STR(_unique_id++) + "_";
      structural_objectRef out_port;

      if(merged_ports.size() == 1U)
      {
         out_port = merged_ports.front();
         THROW_ASSERT(out_port->get_kind() == bus_port->get_kind(), "Out port has type " + bus_port->get_kind_text() +
                                                                        " while internal port has type " +
                                                                        out_port->get_kind_text());
      }
      else
      {
         const auto bus_merger_mod = SM->add_module_from_technology_library(
             bus_merger_inst_name, bus_merger_res_name, bm_library, circuit, HLS->HLS_D->get_technology_manager());
         const auto bm_in_port = GetPointerS<module>(bus_merger_mod)->get_in_port(0U);
         GetPointerS<port_o>(bm_in_port)->add_n_ports(static_cast<unsigned int>(merged_ports.size()), bm_in_port);
         if(is_vector_bus)
         {
            port_o::resize_std_port(GetPointerS<port_o>(bus_port)->get_ports_size() *
                                        STD_GET_SIZE(bus_port->get_typeRef()),
                                    0U, DEBUG_LEVEL_NONE, bm_in_port);
         }
         else
         {
            port_o::resize_std_port(STD_GET_SIZE(bus_port->get_typeRef()), 0U, DEBUG_LEVEL_NONE, bm_in_port);
         }
         auto in_id = 0U;
         for(const auto& merged_port : merged_ports)
         {
            if(is_vector_bus)
            {
               const auto sign_v_in = SM->add_sign_vector("sig_in_vector_" + bus_merger_inst_name + STR(in_id),
                                                          GetPointerS<port_o>(merged_port)->get_ports_size(), circuit,
                                                          merged_port->get_typeRef());
               const auto js_mod =
                   SM->add_module_from_technology_library(js_name + bus_merger_inst_name + STR(in_id), js_name,
                                                          js_library, circuit, HLS->HLS_D->get_technology_manager());
               const auto js_in_port = GetPointerS<module>(js_mod)->get_in_port(0U);
               GetPointerS<port_o>(js_in_port)
                   ->add_n_ports(static_cast<unsigned int>(GetPointerS<port_o>(merged_port)->get_ports_size()),
                                 js_in_port);
               port_o::resize_std_port(STD_GET_SIZE((merged_port)->get_typeRef()), 0U, DEBUG_LEVEL_NONE, js_in_port);
               SM->add_connection(sign_v_in, merged_port);
               SM->add_connection(sign_v_in, js_in_port);
               const auto js_out_port = GetPointerS<module>(js_mod)->get_out_port(0U);
               port_o::resize_std_port(GetPointerS<port_o>(bus_port)->get_ports_size() *
                                           STD_GET_SIZE(bus_port->get_typeRef()),
                                       0U, DEBUG_LEVEL_NONE, js_out_port);
               structural_type_descriptorRef sig_type(new structural_type_descriptor);
               bus_port->get_typeRef()->copy(sig_type);
               const auto sign_in = SM->add_sign("sig_in_" + bus_merger_inst_name + STR(in_id), circuit, sig_type);
               if(sig_type->type == structural_type_descriptor::BOOL)
               {
                  sig_type->type = structural_type_descriptor::VECTOR_BOOL;
                  sign_in->type_resize(1U, GetPointerS<port_o>(bus_port)->get_ports_size() *
                                               STD_GET_SIZE(bus_port->get_typeRef()));
               }
               else
               {
                  sign_in->type_resize(GetPointer<port_o>(bus_port)->get_ports_size() *
                                       STD_GET_SIZE(bus_port->get_typeRef()));
               }
               SM->add_connection(sign_in, js_out_port);
               SM->add_connection(sign_in, GetPointer<port_o>(bm_in_port)->get_port(in_id));
            }
            else
            {
               const auto sign_in =
                   SM->add_sign("sig_in_" + bus_merger_inst_name + STR(in_id), circuit, bus_port->get_typeRef());
               SM->add_connection(sign_in, merged_port);
               SM->add_connection(sign_in, GetPointerS<port_o>(bm_in_port)->get_port(in_id));
            }
            in_id += 1U;
         }
         out_port = GetPointerS<module>(bus_merger_mod)->get_out_port(0);
         if(is_vector_bus)
         {
            port_o::resize_std_port(GetPointerS<port_o>(bus_port)->get_ports_size() *
                                        STD_GET_SIZE(bus_port->get_typeRef()),
                                    0U, DEBUG_LEVEL_NONE, out_port);
         }
         else
         {
            port_o::resize_std_port(STD_GET_SIZE(bus_port->get_typeRef()), 0U, DEBUG_LEVEL_NONE, out_port);
         }
         if(is_vector_bus)
         {
            const auto ss_mod = SM->add_module_from_technology_library(
                ss_name + bus_merger_inst_name, ss_name, ss_library, circuit, HLS->HLS_D->get_technology_manager());
            structural_type_descriptorRef sig_type(new structural_type_descriptor);
            bus_port->get_typeRef()->copy(sig_type);
            const auto sign_out = SM->add_sign("sig_out_" + bus_merger_inst_name, circuit, sig_type);
            if(sig_type->type == structural_type_descriptor::BOOL)
            {
               sig_type->type = structural_type_descriptor::VECTOR_BOOL;
               sign_out->type_resize(1, GetPointer<port_o>(bus_port)->get_ports_size() *
                                            STD_GET_SIZE(bus_port->get_typeRef()));
            }
            else
            {
               sign_out->type_resize(GetPointer<port_o>(bus_port)->get_ports_size() *
                                     STD_GET_SIZE(bus_port->get_typeRef()));
            }
            const auto ss_in_port = GetPointerS<module>(ss_mod)->get_in_port(0U);
            port_o::resize_std_port(GetPointer<port_o>(bus_port)->get_ports_size() *
                                        STD_GET_SIZE(bus_port->get_typeRef()),
                                    0U, DEBUG_LEVEL_NONE, ss_in_port);
            SM->add_connection(sign_out, out_port);
            SM->add_connection(sign_out, ss_in_port);
            out_port = GetPointerS<module>(ss_mod)->get_out_port(0U);
            GetPointerS<port_o>(out_port)->add_n_ports(
                static_cast<unsigned int>(GetPointerS<port_o>(bus_port)->get_ports_size()), out_port);
            port_o::resize_std_port(STD_GET_SIZE(bus_port->get_typeRef()), 0U, DEBUG_LEVEL_NONE, out_port);
         }
      }
      THROW_ASSERT(out_port, "");
      if(is_vector_bus)
      {
         if(bus_port->get_owner() != circuit)
         {
            structural_objectRef sig;
            if(is_vector_bus)
            {
               sig = SM->add_sign_vector("sig_out_vector_" + bus_merger_inst_name,
                                         GetPointerS<port_o>(bus_port)->get_ports_size(), circuit,
                                         bus_port->get_typeRef());
            }
            else
            {
               sig = SM->add_sign("sig_out_" + bus_merger_inst_name, circuit, out_port->get_typeRef());
            }
            SM->add_connection(sig, out_port);
            SM->add_connection(sig, bus_port);
         }
         else
         {
            SM->add_connection(out_port, bus_port);
         }
      }
      else
      {
         structural_type_descriptorRef sig_type(new structural_type_descriptor);
         out_port->get_typeRef()->copy(sig_type);
         const auto sign_out = SM->add_sign("sig_out_" + bus_merger_inst_name, circuit, sig_type);
         sign_out->type_resize(STD_GET_SIZE(bus_port->get_typeRef()));
         SM->add_connection(sign_out, out_port);
         SM->add_connection(sign_out, bus_port);
      }
   }
}

bool jms_sorter::operator()(const structural_objectRef& a, const structural_objectRef& b) const
{
   return a->get_path() < b->get_path();
}

void fu_binding::manage_memory_ports_parallel_chained(const HLS_managerRef, const structural_managerRef SM,
                                                      const std::list<structural_objectRef>& memory_modules,
                                                      const structural_objectRef circuit, const hlsRef HLS,
                                                      unsigned int& _unique_id)
{
   std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter> primary_outs;
   structural_objectRef cir_port;
   const auto vector_to_port = [](structural_objectRef& a, structural_objectRef& b) {
      if(a->get_kind() != b->get_kind())
      {
         if(a->get_kind() == port_vector_o_K)
         {
            THROW_ASSERT(b->get_kind() == port_o_K, "");
            const auto port_v = GetPointerS<const port_o>(a);
            THROW_ASSERT(port_v->get_ports_size() == 1U,
                         "Multiple ports: " + STR(port_v->get_ports_size()) + " in " + port_v->get_path());
            a = port_v->get_port(0U);
         }
         else if(b->get_kind() == port_vector_o_K)
         {
            THROW_ASSERT(a->get_kind() == port_o_K, "");
            const auto port_v = GetPointerS<const port_o>(b);
            THROW_ASSERT(port_v->get_ports_size() == 1U,
                         "Multiple ports: " + STR(port_v->get_ports_size()) + " in " + port_v->get_path());
            b = port_v->get_port(0U);
         }
      }
   };
   for(const auto& memory_module : memory_modules)
   {
      for(unsigned int j = 0; j < GetPointer<module>(memory_module)->get_in_port_size(); ++j)
      {
         structural_objectRef port_i = GetPointer<module>(memory_module)->get_in_port(j);
         if(GetPointer<port_o>(port_i)->get_is_memory() && (!GetPointer<port_o>(port_i)->get_is_global()) &&
            (!GetPointer<port_o>(port_i)->get_is_extern()))
         {
            std::string port_name = GetPointer<port_o>(port_i)->get_id();
            cir_port = circuit->find_member(port_name, port_i->get_kind(), circuit);
            THROW_ASSERT(!cir_port || GetPointer<port_o>(cir_port), "should be a port or null");
            if(!cir_port)
            {
               if(port_i->get_kind() == port_vector_o_K)
               {
                  cir_port = SM->add_port_vector(port_name, port_o::IN, GetPointer<port_o>(port_i)->get_ports_size(),
                                                 circuit, port_i->get_typeRef());
               }
               else
               {
                  cir_port = SM->add_port(port_name, port_o::IN, circuit, port_i->get_typeRef());
               }
               port_o::fix_port_properties(port_i, cir_port);
               SM->add_connection(cir_port, port_i);
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              port_i->get_path() + " <-> " + cir_port->get_path());
               vector_to_port(port_i, cir_port);
               THROW_ASSERT(port_i->get_kind() == cir_port->get_kind(),
                            "unexpected condition: " + port_i->get_path() + "(" + port_i->get_kind_text() +
                                ") != " + cir_port->get_path() + "(" + cir_port->get_kind_text() + ")");
               if(port_i->get_kind() == port_vector_o_K &&
                  GetPointerS<port_o>(port_i)->get_ports_size() > GetPointerS<port_o>(cir_port)->get_ports_size())
               {
                  const auto n_ports =
                      GetPointerS<port_o>(port_i)->get_ports_size() - GetPointerS<port_o>(cir_port)->get_ports_size();
                  GetPointerS<port_o>(cir_port)->add_n_ports(n_ports, cir_port);
               }
               SM->add_connection(cir_port, port_i);
            }
         }
      }

      for(unsigned int j = 0; j < GetPointer<module>(memory_module)->get_out_port_size(); j++)
      {
         structural_objectRef port_i = GetPointer<module>(memory_module)->get_out_port(j);
         if(GetPointer<port_o>(port_i)->get_is_memory() && (!GetPointer<port_o>(port_i)->get_is_global()) &&
            (!GetPointer<port_o>(port_i)->get_is_extern()))
         {
            const auto port_name = GetPointerS<port_o>(port_i)->get_id();
            cir_port = circuit->find_member(port_name, port_i->get_kind(), circuit);
            THROW_ASSERT(!cir_port || GetPointer<port_o>(cir_port), "should be a port or null");
            if(!cir_port)
            {
               if(port_i->get_kind() == port_vector_o_K)
               {
                  cir_port = SM->add_port_vector(port_name, port_o::OUT, GetPointerS<port_o>(port_i)->get_ports_size(),
                                                 circuit, port_i->get_typeRef());
               }
               else
               {
                  cir_port = SM->add_port(port_name, port_o::OUT, circuit, port_i->get_typeRef());
               }
               port_o::fix_port_properties(port_i, cir_port);
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              port_i->get_path() + " <-> " + cir_port->get_path());
               vector_to_port(port_i, cir_port);
               THROW_ASSERT(port_i->get_kind() == cir_port->get_kind(),
                            "unexpected condition: " + port_i->get_path() + "(" + port_i->get_kind_text() +
                                ") != " + cir_port->get_path() + "(" + cir_port->get_kind_text() + ")");
               if(port_i->get_kind() == port_vector_o_K &&
                  GetPointerS<port_o>(port_i)->get_ports_size() > GetPointerS<port_o>(cir_port)->get_ports_size())
               {
                  auto n_ports =
                      GetPointerS<port_o>(port_i)->get_ports_size() - GetPointerS<port_o>(cir_port)->get_ports_size();
                  GetPointerS<port_o>(cir_port)->add_n_ports(n_ports, cir_port);
               }
            }
            if(std::find(primary_outs[cir_port].begin(), primary_outs[cir_port].end(), port_i) ==
               primary_outs[cir_port].end())
            {
               primary_outs[cir_port].push_back(port_i);
            }
         }
      }
   }
   join_merge_split(SM, HLS, primary_outs, circuit, _unique_id);
}

void fu_binding::manage_extern_global_port(const HLS_managerRef HLSMgr, const hlsRef HLS,
                                           const structural_managerRef SM, structural_objectRef port_in,
                                           unsigned int _dir, structural_objectRef circuit, unsigned int num)
{
   auto dir = static_cast<port_o::port_direction>(_dir);
   const auto inPort = GetPointer<port_o>(port_in);
   const auto port_name = inPort->get_id();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Port: " + inPort->get_path());
   structural_objectRef ext_port = circuit->find_member(port_name, port_in->get_kind(), circuit);
   if(inPort->get_is_extern())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Manage extern port");
      if(inPort->get_is_global())
      {
         THROW_ASSERT(!ext_port || GetPointer<port_o>(ext_port), "should be a port or null");
         if(ext_port && GetPointerS<port_o>(ext_port)->get_port_direction() != dir)
         {
            THROW_ASSERT(port_in->get_kind() == ext_port->get_kind(), "unexpected condition");
            if(port_in->get_kind() == port_vector_o_K &&
               inPort->get_ports_size() > GetPointerS<port_o>(ext_port)->get_ports_size())
            {
               const auto n_ports = inPort->get_ports_size() - GetPointerS<port_o>(ext_port)->get_ports_size();
               GetPointerS<port_o>(ext_port)->add_n_ports(n_ports, ext_port);
            }
            SM->change_port_direction(ext_port, dir, circuit);
            if(STD_GET_SIZE(ext_port->get_typeRef()) < STD_GET_SIZE(port_in->get_typeRef()))
            {
               port_o::resize_std_port(STD_GET_SIZE(port_in->get_typeRef()), 0, 0, ext_port);
            }
         }
         else if(!ext_port)
         {
            if(port_in->get_kind() == port_vector_o_K)
            {
               ext_port =
                   SM->add_port_vector(port_name, dir, inPort->get_ports_size(), circuit, port_in->get_typeRef());
            }
            else
            {
               ext_port = SM->add_port(port_name, dir, circuit, port_in->get_typeRef());
            }
         }
         else
         {
            THROW_ASSERT(port_in->get_kind() == ext_port->get_kind(), "unexpected condition");
            if(port_in->get_kind() == port_vector_o_K &&
               inPort->get_ports_size() > GetPointerS<port_o>(ext_port)->get_ports_size())
            {
               const auto n_ports = inPort->get_ports_size() - GetPointerS<port_o>(ext_port)->get_ports_size();
               GetPointer<port_o>(ext_port)->add_n_ports(n_ports, ext_port);
            }
         }
      }
      else
      {
         if(port_in->get_kind() == port_vector_o_K)
         {
            ext_port = SM->add_port_vector("ext_" + inPort->get_id() + "_" + std::to_string(num), dir,
                                           inPort->get_ports_size(), circuit, port_in->get_typeRef());
         }
         else
         {
            ext_port = SM->add_port("ext_" + inPort->get_id() + "_" + std::to_string(num), dir, circuit,
                                    port_in->get_typeRef());
         }
      }
   }
   else if(inPort->get_is_global())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Manage global port");
      THROW_ASSERT(!ext_port || GetPointer<port_o>(ext_port), "should be a port or null");
      if(!ext_port)
      {
         if(port_in->get_kind() == port_vector_o_K)
         {
            ext_port = SM->add_port_vector(port_name, dir, inPort->get_ports_size(), circuit, port_in->get_typeRef());
         }
         else
         {
            ext_port = SM->add_port(port_name, dir, circuit, port_in->get_typeRef());
         }
      }
   }
   else if(inPort->get_port_interface() != port_o::port_interface::PI_DEFAULT)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Manage interface port");
      const auto fsymbol =
          HLSMgr->CGetFunctionBehavior(HLS->functionId)->CGetBehavioralHelper()->GetMangledFunctionName();
      const auto func_arch = HLSMgr->module_arch->GetArchitecture(fsymbol);
      const auto is_dataflow_top =
          func_arch && func_arch->attrs.find(FunctionArchitecture::func_dataflow_top) != func_arch->attrs.end() &&
          func_arch->attrs.find(FunctionArchitecture::func_dataflow_top)->second == "1";
      if(is_dataflow_top)
      {
         const std::regex regx(R"(_DF_bambu_(\d+)_\d+FO\d+_.*)");
         std::cmatch what;
         if(std::regex_search(port_name.data(), what, regx))
         {
            const auto owner_id = what[1].str();
            if(std::to_string(HLS->functionId) == owner_id)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Stop interface propagation at dataflow top module");
               return;
            }
         }
      }
      THROW_ASSERT(!ext_port || GetPointer<port_o>(ext_port), "should be a port or null");
      if(ext_port && GetPointer<port_o>(ext_port)->get_port_direction() != dir)
      {
         SM->change_port_direction(ext_port, dir, circuit);
         if(STD_GET_SIZE(ext_port->get_typeRef()) < STD_GET_SIZE(port_in->get_typeRef()))
         {
            port_o::resize_std_port(STD_GET_SIZE(port_in->get_typeRef()), 0, 0, ext_port);
         }
      }
      else if(!ext_port)
      {
         if(port_in->get_kind() == port_vector_o_K)
         {
            ext_port = SM->add_port_vector(port_name, dir, inPort->get_ports_size(), circuit, port_in->get_typeRef());
         }
         else
         {
            ext_port = SM->add_port(port_name, dir, circuit, port_in->get_typeRef());
         }
      }
   }
   else
   {
      ext_port = nullptr;
   }
   if(ext_port)
   {
      port_o::fix_port_properties(port_in, ext_port);
      SM->add_connection(port_in, ext_port);
   }
}

tree_nodeRef getFunctionType(tree_nodeRef exp);
void fu_binding::specialise_fu(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef fu_obj,
                               unsigned int fu, const OpVertexSet& mapped_operations, unsigned int ar)
{
   const auto FB = HLSMgr->CGetFunctionBehavior(HLS->functionId);
   const auto memory_allocation_policy = FB->GetMemoryAllocationPolicy();
   auto bus_data_bitsize = HLSMgr->Rmem->get_bus_data_bitsize();
   auto bus_size_bitsize = HLSMgr->Rmem->get_bus_size_bitsize();
   const auto bus_addr_bitsize = HLSMgr->get_address_bitsize();
   const auto bus_tag_bitsize =
       HLS->Param->isOption(OPT_context_switch) ? GetPointer<memory_cs>(HLSMgr->Rmem)->get_bus_tag_bitsize() : 0;
   auto* fu_module = GetPointer<module>(fu_obj);
   const auto fu_tech_obj = allocation_information->get_fu(fu);
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                  "-->Specializing " + fu_obj->get_path() + " of type " + GET_TYPE_NAME(fu_obj));
   std::map<unsigned int, unsigned long long> required_variables;
   std::map<unsigned int, unsigned long long> num_elements;
   unsigned long long n_out_elements = 0;
   unsigned long long produced_variables = 1;
   const auto is_multiport = allocation_information->get_number_channels(fu) > 1U;
   const auto max_n_ports = is_multiport ? allocation_information->get_number_channels(fu) : 0U;

   if(ar)
   {
      bool has_misaligned_indirect_ref = false;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Ar is true");
      {
         const auto type_node = tree_helper::CGetType(TreeM->GetTreeNode(ar));
         const auto elmt_bitsize = tree_helper::AccessedMaximumBitsize(type_node, 1);

         if(allocation_information->is_direct_access_memory_unit(fu))
         {
            required_variables[0] = elmt_bitsize;
            if(HLSMgr->Rmem->is_private_memory(ar))
            {
               bus_data_bitsize = std::max(bus_data_bitsize, elmt_bitsize);
            }
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
         {
            THROW_ERROR("Unit currently not supported: " + allocation_information->get_fu_name(fu).first);
         }
         const OpGraphConstRef data = FB->CGetOpGraph(FunctionBehavior::CFG);
         for(const auto& mapped_operation : mapped_operations)
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                          "  on BRAM = " + data->CGetOpNodeInfo(mapped_operation)->GetOperation() + " " +
                              GET_NAME(data, mapped_operation));
            const auto vars = HLSMgr->get_required_values(HLS->functionId, mapped_operation);
            const auto out_var = HLSMgr->get_produced_value(HLS->functionId, mapped_operation);
            if(GET_TYPE(data, mapped_operation) & TYPE_STORE)
            {
               THROW_ASSERT(std::get<0>(vars[0]), "Expected a tree node in case of a value to store");
               required_variables[0] =
                   std::max(required_variables[0],
                            tree_helper::SizeAlloc(tree_helper::CGetType(TreeM->GetTreeNode(std::get<0>(vars[0])))));
               if(tree_helper::is_a_misaligned_vector(TreeM, std::get<0>(vars[0])))
               {
                  has_misaligned_indirect_ref = true;
               }
            }
            else if(GET_TYPE(data, mapped_operation) & TYPE_LOAD)
            {
               THROW_ASSERT(out_var, "Expected a tree node in case of a value to load");
               produced_variables = std::max(
                   produced_variables, tree_helper::SizeAlloc(tree_helper::CGetType(TreeM->GetTreeNode(out_var))));
               if(tree_helper::is_a_misaligned_vector(TreeM, out_var))
               {
                  has_misaligned_indirect_ref = true;
               }
            }
         }
      }
      if(fu_module->ExistsParameter("BRAM_BITSIZE"))
      {
         auto bram_bitsize = HLSMgr->Rmem->get_bram_bitsize();
         if(HLSMgr->Rmem->is_private_memory(ar))
         {
            auto accessed_bitsize = std::max(required_variables[0], produced_variables);
            accessed_bitsize = ceil_pow2(accessed_bitsize);
            bram_bitsize = has_misaligned_indirect_ref ? std::max(bram_bitsize, accessed_bitsize) :
                                                         std::max(bram_bitsize, accessed_bitsize / 2);
            if(bram_bitsize > HLSMgr->Rmem->get_maxbram_bitsize())
            {
               THROW_ERROR("incorrect operation mapping on memory module");
            }
         }
         fu_module->SetParameter("BRAM_BITSIZE", STR(bram_bitsize));
      }
      if(fu_module->ExistsParameter("BUS_PIPELINED"))
      {
         const auto has_extern_mem =
             ((HLSMgr->Rmem->get_memory_address() - HLSMgr->base_address) > 0 &&
              memory_allocation_policy != MemoryAllocation_Policy::EXT_PIPELINED_BRAM) ||
             (HLSMgr->Rmem->has_unknown_addresses() && memory_allocation_policy != MemoryAllocation_Policy::ALL_BRAM &&
              memory_allocation_policy != MemoryAllocation_Policy::EXT_PIPELINED_BRAM);
         fu_module->SetParameter("BUS_PIPELINED", has_extern_mem ? "0" : "1");
      }
   }
   else
   {
      const auto data = FB->CGetOpGraph(FunctionBehavior::CFG);

      for(const auto& mapped_operation : mapped_operations)
      {
         const auto vars = HLSMgr->get_required_values(HLS->functionId, mapped_operation);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "---Considering operation " +
                            HLSMgr->get_tree_manager()
                                ->GetTreeNode(data->CGetOpNodeInfo(mapped_operation)->GetNodeId())
                                ->ToString());
         const auto out_var = HLSMgr->get_produced_value(HLS->functionId, mapped_operation);
         const auto fun_unit = GetPointerS<functional_unit>(fu_tech_obj);
         const auto& memory_ctrl_type = fun_unit->memory_ctrl_type;

         if(memory_ctrl_type != "")
         {
            unsigned long long mem_var_size_in = 1;
            unsigned long long mem_var_size_out = 1;

            if(GET_TYPE(data, mapped_operation) & TYPE_STORE)
            {
               THROW_ASSERT(std::get<0>(vars[0]), "Expected a tree node in case of a value to store");
               mem_var_size_in =
                   std::max(mem_var_size_in,
                            tree_helper::SizeAlloc(tree_helper::CGetType(TreeM->GetTreeNode(std::get<0>(vars[0])))));
            }
            else if(GET_TYPE(data, mapped_operation) & TYPE_LOAD)
            {
               THROW_ASSERT(out_var, "Expected a tree node in case of a value to load");
               mem_var_size_out = std::max(mem_var_size_out,
                                           tree_helper::SizeAlloc(tree_helper::CGetType(TreeM->GetTreeNode(out_var))));
            }
            /// specializing MEMORY_STD ports
            required_variables.insert(std::make_pair(0, 0));
            required_variables[0] = std::max(required_variables[0], mem_var_size_in);
            required_variables[1] = bus_addr_bitsize;
            if(allocation_information->is_direct_access_memory_unit(fu))
            {
               bus_data_bitsize = std::max(bus_data_bitsize, std::max(mem_var_size_in, mem_var_size_out));
               for(; bus_data_bitsize >= (1u << bus_size_bitsize); ++bus_size_bitsize)
               {
                  ;
               }
            }
            required_variables[2] = bus_size_bitsize;
            produced_variables = std::max(produced_variables, mem_var_size_out);
         }
         else if(HLS->HLS_D->get_technology_manager()->get_library(allocation_information->get_fu_name(fu).first) !=
                     WORK_LIBRARY &&
                 HLS->HLS_D->get_technology_manager()->get_library(allocation_information->get_fu_name(fu).first) !=
                     PROXY_LIBRARY) // functions just synthesized shouldn't be customized
         {
            const auto np = fu_module->get_NP_functionality();
            const auto is_flopoco = np && np->exist_NP_functionality(NP_functionality::FLOPOCO_PROVIDED);
            const auto op_name = data->CGetOpNodeInfo(mapped_operation)->GetOperation();
            const auto is_float_expr = op_name.find(FLOAT_EXPR) != std::string::npos;
            for(auto i = 0U; i < vars.size(); ++i)
            {
               const auto& tree_var = std::get<0>(vars[i]);
               if(tree_var == 0)
               {
                  continue;
               }
               required_variables.insert(std::make_pair(i, 0));
               const auto var_node = TreeM->GetTreeNode(tree_var);
               if(tree_helper::IsVectorType(var_node))
               {
                  const auto type = tree_helper::CGetType(var_node);
                  const auto size = tree_helper::SizeAlloc(type);
                  const auto element_type = tree_helper::CGetElements(type);
                  const auto element_size = tree_helper::SizeAlloc(element_type);
                  required_variables[i] = std::max(required_variables[i], element_size);

                  if(num_elements.find(i) == num_elements.end())
                  {
                     num_elements[i] = size / element_size;
                  }
                  else
                  {
                     THROW_ASSERT(num_elements.find(i)->second == size / element_size,
                                  "Performed a wrong module allocation");
                  }
               }
               else
               {
                  auto bitsize = tree_helper::Size(var_node);
                  if(is_float_expr && is_flopoco)
                  {
                     if(bitsize < 32)
                     {
                        bitsize = 32;
                     }
                     else if(bitsize > 32 && bitsize < 64)
                     {
                        bitsize = 64;
                     }
                  }
                  required_variables[i] = std::max(required_variables[i], bitsize);
               }
            }
            if(np)
            {
               std::vector<std::string> param;
               np->get_library_parameters(param);
               auto it_end = param.end();
               for(auto it = param.begin(); it != it_end; ++it)
               {
                  if(*it == "LSB_PARAMETER" && op_name == "pointer_plus_expr")
                  {
                     unsigned int curr_LSB = 0;
                     auto op0_tree_var = std::get<0>(vars[0]);
                     if(op0_tree_var)
                     {
                        const auto var = tree_helper::GetBaseVariable(TreeM->GetTreeNode(op0_tree_var));
                        if(var && FB->is_variable_mem(var->index) && HLSMgr->Rmem->is_sds_var(var->index))
                        {
                           const auto type = tree_helper::CGetType(var);
                           const auto value_bitsize = tree_helper::AccessedMaximumBitsize(type, 1);
                           if(value_bitsize <= 8)
                           {
                              curr_LSB = 0;
                           }
                           else if(value_bitsize == 16)
                           {
                              curr_LSB = 1;
                           }
                           else if(value_bitsize == 32)
                           {
                              curr_LSB = 2;
                           }
                           else if(value_bitsize == 64)
                           {
                              curr_LSB = 3;
                           }
                           else if(value_bitsize == 128)
                           {
                              curr_LSB = 4;
                           }
                           else if(value_bitsize == 256)
                           {
                              curr_LSB = 5;
                           }
                           else
                           {
                              curr_LSB = 0;
                           }
                        }
                     }
                     auto op0 = TreeM->GetTreeNode(op0_tree_var);
                     auto op1 = TreeM->GetTreeNode(std::get<0>(vars[1]));
                     if(op0->get_kind() == ssa_name_K)
                     {
                        auto ssa_var0 = GetPointer<ssa_name>(op0);
                        if(!ssa_var0->bit_values.empty())
                        {
                           auto tailZeros = 0u;
                           const auto lengthBV = ssa_var0->bit_values.size();
                           const auto& currBit = ssa_var0->bit_values.at(lengthBV - 1 - tailZeros);
                           while(lengthBV > tailZeros && (currBit == '0' || currBit == 'X'))
                           {
                              ++tailZeros;
                           }
                           if(tailZeros < curr_LSB)
                           {
                              curr_LSB = tailZeros;
                           }
                        }
                        else
                        {
                           curr_LSB = 0;
                        }
                     }
                     else
                     {
                        curr_LSB = 0;
                     }
                     if(op1->get_kind() == ssa_name_K)
                     {
                        auto ssa_var1 = GetPointer<ssa_name>(op1);
                        if(!ssa_var1->bit_values.empty())
                        {
                           auto tailZeros = 0u;
                           const auto lengthBV = ssa_var1->bit_values.size();
                           const auto& currBit = ssa_var1->bit_values.at(lengthBV - 1 - tailZeros);
                           while(lengthBV > tailZeros && (currBit == '0' || currBit == 'X'))
                           {
                              ++tailZeros;
                           }
                           if(tailZeros < curr_LSB)
                           {
                              curr_LSB = tailZeros;
                           }
                        }
                        else
                        {
                           curr_LSB = 0;
                        }
                     }
                     else if(op1->get_kind() == integer_cst_K)
                     {
                        const auto offset_value = tree_helper::GetConstValue(op1);
                        if(offset_value)
                        {
                           auto tailZeros = 0u;
                           while((offset_value & (integer_cst_t(1) << tailZeros)) == 0)
                           {
                              ++tailZeros;
                           }
                           if(tailZeros < curr_LSB)
                           {
                              curr_LSB = tailZeros;
                           }
                        }
                     }
                     else
                     {
                        curr_LSB = 0;
                     }
                     if(fu_module->ExistsParameter("LSB_PARAMETER"))
                     {
                        int lsb_parameter = std::stoi(fu_module->GetParameter("LSB_PARAMETER"));
                        if(lsb_parameter < 0)
                        {
                           lsb_parameter = static_cast<int>(curr_LSB);
                        }
                        else
                        {
                           lsb_parameter = std::min(lsb_parameter, static_cast<int>(curr_LSB));
                        }
                        fu_module->SetParameter("LSB_PARAMETER", STR(lsb_parameter));
                     }
                     else
                     {
                        fu_module->SetParameter("LSB_PARAMETER", STR(curr_LSB));
                     }
                  }
                  if(*it == "OFFSET_PARAMETER" && op_name == "bit_ior_concat_expr")
                  {
                     auto index = data->CGetOpNodeInfo(mapped_operation)->GetNodeId();
                     const auto ga_node = TreeM->GetTreeNode(index);
                     const auto ga = GetPointer<gimple_assign>(ga_node);
                     const auto ce = GetPointer<bit_ior_concat_expr>(ga->op1);
                     const auto offset_value = tree_helper::GetConstValue(ce->op2);
                     fu_module->SetParameter("OFFSET_PARAMETER", STR(offset_value));
                  }
                  if(*it == "unlock_address" && op_name == BUILTIN_WAIT_CALL)
                  {
                     auto index = data->CGetOpNodeInfo(mapped_operation)->GetNodeId();
                     std::string parameterName = HLSMgr->Rmem->get_symbol(index, HLS->functionId)->get_symbol_name();
                     fu_module->SetParameter("unlock_address", parameterName);
                  }
                  if(*it == "MEMORY_INIT_file" && op_name == BUILTIN_WAIT_CALL)
                  {
                     auto index = data->CGetOpNodeInfo(mapped_operation)->GetNodeId();
                     const auto parameterAddressFileName = "function_addresses_" + STR(index) + ".mem";
                     std::ofstream parameterAddressFile(parameterAddressFileName);

                     const auto call = TreeM->GetTreeNode(index);
                     const auto& calledFunction = GetPointerS<const gimple_call>(call)->args[0];
                     const auto& hasreturn_node = GetPointerS<const gimple_call>(call)->args[1];
                     const auto hasreturn_value =
                         tree_helper::get_integer_cst_value(GetPointerS<const integer_cst>(hasreturn_node));
                     const auto addrExpr = calledFunction;
                     const auto functionType = getFunctionType(addrExpr);
                     const auto alignment = HLSMgr->Rmem->get_parameter_alignment();
                     unsigned long long int address = 0;
                     address = HLSMgr->Rmem->compute_next_base_address(address, index, alignment);
                     auto paramList = GetPointerS<const function_type>(functionType)->prms;
                     while(paramList)
                     {
                        const auto node = GetPointerS<const tree_list>(paramList);
                        if(node->valu->get_kind() != void_type_K)
                        {
                           const auto str_address = convert_to_binary(address, HLSMgr->get_address_bitsize());
                           parameterAddressFile << str_address << "\n";
                           address = HLSMgr->Rmem->compute_next_base_address(address, node->valu->index, alignment);
                        }
                        paramList = node->chan;
                     }
                     const auto return_type = GetPointerS<const function_type>(functionType)->retn;
                     if(return_type && return_type->get_kind() != void_type_K && hasreturn_value)
                     {
                        const auto str_address = convert_to_binary(address, HLSMgr->get_address_bitsize());
                        parameterAddressFile << str_address << "\n";
                     }
                     parameterAddressFile.close();
                     fu_module->SetParameter("MEMORY_INIT_file", "\"\"" + parameterAddressFileName + "\"\"");
                  }
               }
            }
            if(out_var)
            {
               const auto out_node = TreeM->GetTreeNode(out_var);
               if(tree_helper::IsVectorType(out_node))
               {
                  const auto type = tree_helper::CGetType(out_node);
                  const auto size = tree_helper::SizeAlloc(type);
                  const auto element_type = tree_helper::CGetElements(type);
                  const auto element_size = tree_helper::SizeAlloc(element_type);
                  n_out_elements = size / element_size;
                  produced_variables = element_size;
               }
               else
               {
                  produced_variables = std::max(produced_variables, tree_helper::Size(out_node));
               }
               /// check for precision parameter
               if(np)
               {
                  std::vector<std::string> param;
                  np->get_library_parameters(param);
                  auto it_end = param.end();
                  for(auto it = param.begin(); it != it_end; ++it)
                  {
                     if(*it == "PRECISION")
                     {
                        auto sizetype = tree_helper::Size(tree_helper::CGetType(out_node));
                        if(sizetype == 1)
                        {
                           sizetype = 8;
                        }
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
      {
         ++offset;
      }
      if(is_multiport && port->get_kind() == port_vector_o_K && GetPointer<port_o>(port)->get_ports_size() == 0)
      {
         GetPointer<port_o>(port)->add_n_ports(static_cast<unsigned int>(max_n_ports), port);
      }
      else if(is_multi_read_cond && port->get_kind() == port_vector_o_K &&
              GetPointer<port_o>(port)->get_ports_size() == 0)
      {
         GetPointer<port_o>(port)->add_n_ports(static_cast<unsigned int>(required_variables.size()), port);
      }
      port_o::resize_if_busport(bus_size_bitsize, bus_addr_bitsize, bus_data_bitsize, bus_tag_bitsize, port);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Resized input ports");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Resizing variables");
   auto is_dual = allocation_information->is_dual_port_memory(fu);

   for(auto l = required_variables.begin(); l != required_variables.end() && !is_multi_read_cond; ++l)
   {
      unsigned long long n_elmts = 0;
      if(num_elements.find(l->first) != num_elements.end())
      {
         n_elmts = num_elements.find(l->first)->second;
      }
      auto bitsize_variable = l->second;
      auto piIndexLimit = 1U;
      if(is_dual && l->first)
      {
         piIndexLimit = 2U;
      }
      for(auto piOffset = 0U; piOffset < piIndexLimit; ++piOffset)
      {
         structural_objectRef port = fu_module->get_in_port(l->first + offset + piOffset);
         port_o::resize_std_port(bitsize_variable, n_elmts, debug_level, port);
      }
      if(is_dual && l->first)
      {
         ++offset;
      }
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Resized variables");
   offset = 0;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Resizing output ports");
   for(unsigned int i = 0; i < fu_module->get_out_port_size(); i++)
   {
      structural_objectRef port = fu_module->get_out_port(i);
      if(port->get_id() == DONE_PORT_NAME)
      {
         offset++;
      }
      if(is_multiport && port->get_kind() == port_vector_o_K && GetPointer<port_o>(port)->get_ports_size() == 0)
      {
         GetPointer<port_o>(port)->add_n_ports(static_cast<unsigned int>(max_n_ports), port);
      }
      port_o::resize_if_busport(bus_size_bitsize, bus_addr_bitsize, bus_data_bitsize, bus_tag_bitsize, port);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Resized output ports");
   if(offset < fu_module->get_out_port_size())
   {
      structural_objectRef port = fu_module->get_out_port(offset);
      if(port->get_typeRef()->type != structural_type_descriptor::BOOL)
      {
         if(is_multi_read_cond)
         {
            port_o::resize_std_port(static_cast<unsigned int>(required_variables.size()), 0, debug_level, port);
         }
         else
         {
            port_o::resize_std_port(produced_variables, n_out_elements, debug_level, port);
         }
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

void fu_binding::specialize_memory_unit(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef fu_obj,
                                        unsigned int ar, const std::string& base_address,
                                        unsigned long long int rangesize, bool is_memory_splitted,
                                        bool is_sparse_memory, bool is_sds)
{
   const auto fu_module = GetPointer<module>(fu_obj);
   /// base address specialization
   fu_module->SetParameter("address_space_begin", STR(base_address));
   fu_module->SetParameter("address_space_rangesize", STR(rangesize));
   fu_module->SetParameter("USE_SPARSE_MEMORY", is_sparse_memory ? "1" : "0");
   memory::add_memory_parameter(HLS->datapath, base_address, STR(HLSMgr->Rmem->get_base_address(ar, HLS->functionId)));

   /// array ref initialization
   THROW_ASSERT(ar, "expected a real tree node index");
   const auto init_filename = "array_ref_" + STR(ar) + ".mem";
   std::ofstream init_file_a(init_filename);
   std::ofstream init_file_b;
   if(is_memory_splitted)
   {
      init_file_b.open("0_" + init_filename);
   }
   unsigned long long vec_size = 0, elts_size = 0;
   const auto bitsize_align = is_sds ? 0ULL : std::stoull(fu_module->GetParameter("BRAM_BITSIZE"));
   fill_array_ref_memory(init_file_a, init_file_b, ar, vec_size, elts_size, HLSMgr->Rmem, TreeM, is_sds, bitsize_align);
   THROW_ASSERT(vec_size, "at least one element is expected");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---elts_size " + STR(elts_size));
   if(is_sds)
   {
      fu_module->SetParameter("ALIGNMENT", STR(elts_size));
   }
   if(is_memory_splitted)
   {
      fu_module->SetParameter("MEMORY_INIT_file_a", "\"\"" + init_filename + "\"\"");
      fu_module->SetParameter("MEMORY_INIT_file_b", "\"\"0_" + init_filename + "\"\"");
   }
   else
   {
      fu_module->SetParameter("MEMORY_INIT_file", "\"\"" + init_filename + "\"\"");
   }

   /// specialize the number of elements in the array
   fu_module->SetParameter("n_elements", STR(vec_size));
   fu_module->SetParameter("data_size", STR(elts_size));
   fu_module->SetParameter("PRIVATE_MEMORY", HLSMgr->Rmem->is_private_memory(ar) ? "1" : "0");
   fu_module->SetParameter("READ_ONLY_MEMORY", HLSMgr->Rmem->is_read_only_variable(ar) ? "1" : "0");
}

void fu_binding::fill_array_ref_memory(std::ostream& init_file_a, std::ostream& init_file_b, unsigned int ar,
                                       unsigned long long& vec_size, unsigned long long& elts_size, const memoryRef mem,
                                       tree_managerConstRef TM, bool is_sds, unsigned long long bitsize_align)
{
   init_file_b.put(0);
   const auto is_memory_splitted = init_file_b.good();
   init_file_b.seekp(std::ios_base::beg);

   const auto ar_node = TM->GetTreeNode(ar);
   tree_nodeRef init_node;
   const auto vd = GetPointer<const var_decl>(ar_node);
   if(vd && vd->init)
   {
      init_node = vd->init;
   }
   else if(GetPointer<const string_cst>(ar_node))
   {
      init_node = ar_node;
   }
   const auto array_type_node = tree_helper::CGetType(ar_node);
   unsigned long long element_align = 0;
   if(tree_helper::IsArrayEquivType(array_type_node))
   {
      std::vector<unsigned long long> dims;
      tree_helper::get_array_dim_and_bitsize(TM, array_type_node->index, dims, elts_size);
      THROW_ASSERT(dims.size(), "something wrong happened");
      vec_size = std::accumulate(dims.begin(), dims.end(), 1ULL,
                                 [](unsigned long long a, unsigned long long b) { return a * b; });
   }
   else if(GetPointer<const integer_type>(array_type_node) || tree_helper::IsRealType(array_type_node) ||
           tree_helper::IsEnumType(array_type_node) || tree_helper::IsPointerType(array_type_node) ||
           tree_helper::IsStructType(array_type_node) || tree_helper::IsUnionType(array_type_node) ||
           tree_helper::IsComplexType(array_type_node))
   {
      elts_size = tree_helper::SizeAlloc(array_type_node);
      vec_size = 1;
   }
   else if(tree_helper::IsBooleanType(array_type_node))
   {
      elts_size = 8;
      vec_size = 1;
   }
   else if(tree_helper::IsVectorType(array_type_node))
   {
      elts_size = tree_helper::SizeAlloc(array_type_node);
      const auto element_type = tree_helper::CGetElements(array_type_node);
      element_align = tree_helper::SizeAlloc(element_type);
      vec_size = 1;
   }
   else
   {
      THROW_ERROR("Type not supported: " + array_type_node->get_kind_text());
   }
   THROW_ASSERT(elts_size && vec_size, "");
   if(is_sds)
   {
      bitsize_align = elts_size;
   }
   else
   {
      elts_size = get_aligned_bitsize(elts_size, 8ULL);
   }

   const auto nbyte_on_memory = bitsize_align / 8;

   if(init_node &&
      ((GetPointer<constructor>(init_node) && GetPointerS<constructor>(init_node)->list_of_idx_valu.size()) ||
       (GetPointer<string_cst>(init_node) && GetPointerS<string_cst>(init_node)->strg.size()) ||
       (!GetPointer<constructor>(init_node) && !GetPointer<string_cst>(init_node))))
   {
      std::vector<std::string> init_string;
      write_init(TM, ar_node, init_node, init_string, mem, element_align);
      if(is_sds && (element_align == 0 || elts_size == element_align))
      {
         THROW_ASSERT(!is_memory_splitted, "unexpected condition");
         for(const auto& init_value : init_string)
         {
            THROW_ASSERT(elts_size, "unexpected condition");
            if(elts_size != init_value.size() && (init_value.size() % elts_size == 0))
            {
               const auto n_elmts = init_value.size() / elts_size;
               for(auto index = 0u; index < n_elmts; ++index)
               {
                  init_file_a << init_value.substr(init_value.size() - elts_size - index * elts_size, elts_size)
                              << std::endl;
               }
            }
            else
            {
               init_file_a << init_value << std::endl;
            }
         }
      }
      else
      {
         std::vector<std::string> eightbit_string;
         std::string bits_offset = "";
         for(unsigned int l = 0; l < init_string.size(); ++l)
         {
            if(init_string[l].size() < 8 && init_string.size() == 1)
            {
               std::string res = init_string[l];
               while(res.size() < 8)
               {
                  res = "0" + res;
               }
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
                     eightbit_string.push_back(
                         init_string[l].substr(data_bitsize - (8 - bits_offset.size()), 8 - bits_offset.size()) +
                         bits_offset);
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
                  {
                     eightbit_string.push_back(local_binary_string.substr(local_data_bitsize - 8 - base_index, 8));
                  }
                  else
                  {
                     bits_offset = local_binary_string.substr(0, local_data_bitsize - base_index);
                  }
               }
            }
         }
         if(bits_offset.size())
         {
            std::string tail_padding;
            for(auto tail_padding_ind = bits_offset.size(); tail_padding_ind < 8; ++tail_padding_ind)
            {
               tail_padding += "0";
            }
            tail_padding = tail_padding + bits_offset;
            eightbit_string.push_back(tail_padding);
         }
         if(eightbit_string.size() % nbyte_on_memory != 0)
         {
            for(size_t l = eightbit_string.size() % nbyte_on_memory; l < nbyte_on_memory; ++l)
            {
               eightbit_string.push_back("00000000");
            }
         }
         if((tree_helper::SizeAlloc(array_type_node) / 8U) > eightbit_string.size())
         {
            size_t tail_bytes = (tree_helper::SizeAlloc(array_type_node) / 8U) - eightbit_string.size();
            for(size_t l = 0; l < tail_bytes; ++l)
            {
               eightbit_string.push_back("00000000");
            }
         }

         std::string str_bit;
         bool is_even = true;
         unsigned int counter;
         for(unsigned int l = 0; l < eightbit_string.size();)
         {
            str_bit = "";
            for(counter = 0; counter < nbyte_on_memory && l < eightbit_string.size(); counter++, l++)
            {
               str_bit = eightbit_string[l] + str_bit;
            }
            if(is_even || !is_memory_splitted)
            {
               init_file_a << str_bit << std::endl;
            }
            else
            {
               init_file_b << str_bit << std::endl;
            }
            is_even = !is_even;
         }
         if(!is_even && is_memory_splitted)
         {
            bool need_newline_b = false;
            for(unsigned int l = 0; l < (nbyte_on_memory * 8); ++l)
            {
               init_file_b << "0";
               need_newline_b = true;
            }
            if(need_newline_b)
            {
               init_file_b << std::endl;
            }
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
            {
               init_file_a << "0";
            }
            init_file_a << std::endl;
         }
      }
      else
      {
         unsigned int counter = 0;
         bool is_even = true;
         bool need_newline_a = false;
         bool need_newline_b = false;
         for(unsigned int i = 0; i < vec_size; ++i)
         {
            for(unsigned int j = 0; j < elts_size; ++j)
            {
               if(is_even || !is_memory_splitted)
               {
                  init_file_a << "0";
                  need_newline_a = true;
               }
               else
               {
                  init_file_b << "0";
                  need_newline_b = true;
               }
               counter++;
               if(counter % (nbyte_on_memory * 8) == 0)
               {
                  if(is_even || !is_memory_splitted)
                  {
                     init_file_a << std::endl;
                     need_newline_a = false;
                  }
                  else
                  {
                     init_file_b << std::endl;
                     need_newline_b = false;
                  }
                  is_even = !is_even;
               }
            }
         }
         if(counter % (nbyte_on_memory * 8) != 0)
         {
            for(auto l = counter % (nbyte_on_memory * 8); l < (nbyte_on_memory * 8); ++l)
            {
               if(is_even || !is_memory_splitted)
               {
                  init_file_a << "0";
                  need_newline_a = true;
               }
               else
               {
                  init_file_b << "0";
                  need_newline_b = true;
               }
            }
            is_even = !is_even;
         }
         if(!is_even && is_memory_splitted)
         {
            for(unsigned int l = 0; l < (nbyte_on_memory * 8); ++l)
            {
               init_file_b << "0";
               need_newline_b = true;
            }
         }
         if(need_newline_a)
         {
            init_file_a << std::endl;
         }
         if(need_newline_b)
         {
            init_file_b << std::endl;
         }
      }
   }
}

void fu_binding::write_init(const tree_managerConstRef TreeM, tree_nodeRef var_node, tree_nodeRef _init_node,
                            std::vector<std::string>& init_file, const memoryRef mem, unsigned long long element_align)
{
   std::string trimmed_value;
   const auto init_node = _init_node;
   switch(init_node->get_kind())
   {
      case real_cst_K:
      {
         auto precision = tree_helper::SizeAlloc(_init_node);
         const auto rc = GetPointerS<const real_cst>(init_node);
         std::string C_value = rc->valr;
         trimmed_value = convert_fp_to_string(C_value, precision);
         init_file.push_back(trimmed_value);
         break;
      }
      case integer_cst_K:
      {
         const auto ull_value = tree_helper::GetConstValue(init_node);
         trimmed_value = "";
         auto precision = tree_helper::SizeAlloc(_init_node);
         if(element_align)
         {
            precision = std::min(precision, element_align);
         }
         for(auto ind = 1U; ind <= precision; ind++)
         {
            trimmed_value.push_back(((integer_cst_t(1) << (precision - ind)) & ull_value) ? '1' : '0');
         }
         init_file.push_back(trimmed_value);
         break;
      }
      case complex_cst_K:
      {
         const auto precision = tree_helper::SizeAlloc(_init_node);
         const auto cc = GetPointerS<const complex_cst>(init_node);
         write_init(TreeM, var_node, cc->real, init_file, mem, precision / 2);
         write_init(TreeM, var_node, cc->imag, init_file, mem, precision / 2);
         break;
      }
      case constructor_K:
      {
         const auto co = GetPointerS<const constructor>(init_node);
         bool designated_initializers_used = false;
         bool is_struct = false;
         bool is_union = false;
         unsigned long long union_size = 0;
         std::vector<tree_nodeRef>* field_list = nullptr;
         /// check if designated initializers are really used
         if(co->list_of_idx_valu.size() && co->list_of_idx_valu.front().first->get_kind() == field_decl_K)
         {
            auto iv_it = co->list_of_idx_valu.begin();
            const auto iv_end = co->list_of_idx_valu.end();
            const auto scpe = GetPointerS<field_decl>(iv_it->first)->scpe;

            if(scpe->get_kind() == record_type_K)
            {
               field_list = &GetPointerS<record_type>(scpe)->list_of_flds;
               // struct_or_union_align = GetPointer<record_type>(scpe)->algn;
               is_struct = true;
            }
            else if(scpe->get_kind() == union_type_K)
            {
               field_list = &GetPointerS<union_type>(scpe)->list_of_flds;
               is_union = true;
               union_size = tree_helper::SizeAlloc(scpe);
            }
            else
            {
               THROW_ERROR("expected a record_type or a union_type");
            }
            auto fl_it = field_list->begin();
            const auto fl_end = field_list->end();
            for(; fl_it != fl_end && iv_it != iv_end; ++iv_it, ++fl_it)
            {
               if(iv_it->first && iv_it->first->index != (*fl_it)->index)
               {
                  break;
               }
            }
            if(fl_it != fl_end && is_struct)
            {
               designated_initializers_used = true;
            }
         }

         const auto main_element_align = element_align;
         if(designated_initializers_used)
         {
            THROW_ASSERT(field_list, "something wrong happened");
            auto fli = field_list->begin();
            const auto flend = field_list->end();
            auto iv_it = co->list_of_idx_valu.begin();
            const auto iv_end = co->list_of_idx_valu.end();
            for(; fli != flend; ++fli)
            {
               if(!GetPointer<field_decl>(*fli))
               {
                  continue;
               }
               const auto is_bitfield = GetPointer<field_decl>(*fli)->is_bitfield();
               auto inext = fli;
               ++inext;
               while(inext != flend && !GetPointer<field_decl>(*inext))
               {
                  ++inext;
               }

               if(is_bitfield)
               {
                  // fix the element precision to pass to write_init
                  element_align = tree_helper::SizeAlloc(*fli);
               }

               if(iv_it != iv_end && iv_it->first->index == (*fli)->index)
               {
                  write_init(TreeM, iv_it->first, iv_it->second, init_file, mem, element_align);
                  ++iv_it;
               }
               else
               {
                  write_init(TreeM, *fli, *fli, init_file, mem, element_align);
               }

               if(is_bitfield)
               {
                  // reset the element_align to the main value
                  element_align = main_element_align;
               }

               if(!is_bitfield)
               {
                  /// check if padding is needed
                  unsigned long long int nbits;
                  if(inext != flend)
                  {
                     const auto idx_next_fd = GetPointerS<field_decl>(*inext);
                     THROW_ASSERT(tree_helper::GetConstValue(idx_next_fd->bpos) >= 0, "");
                     nbits = static_cast<unsigned long long int>(tree_helper::GetConstValue(idx_next_fd->bpos));
                  }
                  else
                  {
                     nbits = tree_helper::SizeAlloc(co->type);
                  }
                  const auto idx_curr_fd = GetPointer<field_decl>(*fli);
                  const auto field_decl_size = tree_helper::SizeAlloc(*fli);
                  THROW_ASSERT(nbits >= (tree_helper::GetConstValue(idx_curr_fd->bpos) + field_decl_size), "");
                  nbits -= static_cast<unsigned long long int>(tree_helper::GetConstValue(idx_curr_fd->bpos)) +
                           field_decl_size;
                  if(nbits)
                  {
                     /// add padding
                     init_file.push_back(std::string(nbits, '0'));
                  }
               }
            }
         }
         else
         {
            auto iv_it = co->list_of_idx_valu.begin();
            const auto iv_end = co->list_of_idx_valu.end();
            for(; iv_it != iv_end; ++iv_it)
            {
               if(is_struct && !GetPointer<field_decl>(iv_it->first))
               {
                  continue;
               }
               const auto is_bitfield = is_struct && GetPointer<field_decl>(iv_it->first)->is_bitfield();
               auto iv_next = iv_it;
               ++iv_next;
               while(iv_next != iv_end && is_struct && !GetPointer<field_decl>(iv_next->first))
               {
                  ++iv_next;
               }
               if(is_struct && is_bitfield)
               {
                  // fix the element precision to pass to write_init
                  element_align = tree_helper::SizeAlloc(iv_it->first);
               }
               write_init(TreeM, iv_it->first, iv_it->second, init_file, mem, element_align);
               if(is_struct && is_bitfield)
               {
                  // reset the element_align to the main value
                  element_align = main_element_align;
               }

               if(is_struct && !is_bitfield)
               {
                  /// check if padding is needed
                  unsigned long long int nbits;
                  if(iv_next != iv_end)
                  {
                     const auto idx_next_fd = GetPointerS<field_decl>(iv_next->first);
                     THROW_ASSERT(tree_helper::GetConstValue(idx_next_fd->bpos) >= 0, "");
                     nbits = static_cast<unsigned long long int>(tree_helper::GetConstValue(idx_next_fd->bpos));
                  }
                  else
                  {
                     nbits = tree_helper::SizeAlloc(co->type);
                  }
                  const auto field_decl_size = tree_helper::SizeAlloc(iv_it->first);
                  const auto idx_curr_fd = GetPointerS<field_decl>(iv_it->first);
                  THROW_ASSERT(nbits >= (tree_helper::GetConstValue(idx_curr_fd->bpos) + field_decl_size), "");
                  nbits -= static_cast<unsigned long long int>(tree_helper::GetConstValue(idx_curr_fd->bpos)) +
                           field_decl_size;
                  if(nbits)
                  {
                     /// add padding
                     init_file.push_back(std::string(nbits, '0'));
                  }
               }
               else if(is_union)
               {
                  /// check if padding is needed
                  THROW_ASSERT(co->list_of_idx_valu.size() == 1, "just one initializer is possible");
                  const auto field_decl_size = tree_helper::SizeAlloc(iv_it->first);
                  THROW_ASSERT(union_size >= field_decl_size, "");
                  const auto nbits = union_size - field_decl_size;
                  if(nbits)
                  {
                     /// add padding
                     init_file.push_back(std::string(nbits, '0'));
                  }
               }
            }
         }
         const auto type_n = tree_helper::CGetType(var_node);
         if(GetPointer<const array_type>(type_n))
         {
            unsigned long long size_of_data;
            std::vector<unsigned long long> dims;
            tree_helper::get_array_dim_and_bitsize(TreeM, type_n->index, dims, size_of_data);
            if(element_align)
            {
               size_of_data = std::min(size_of_data, element_align);
            }
            auto num_elements = dims[0];
            if(num_elements < co->list_of_idx_valu.size())
            {
               THROW_ERROR("C description not supported: Array with undefined size or not correctly initialized " +
                           STR(co->list_of_idx_valu.size()) + "-" + STR(num_elements));
            }
            THROW_ASSERT(num_elements >= static_cast<unsigned long long>(co->list_of_idx_valu.size()), "");
            num_elements -= static_cast<unsigned long long>(co->list_of_idx_valu.size());
            init_file.insert(init_file.end(), num_elements, std::string(size_of_data, '0'));
         }
         break;
      }
      case string_cst_K:
      {
         const auto sc = GetPointerS<const string_cst>(init_node);
         const auto string_value = [&]() {
            std::string tmp;
            const char* c_str = sc->strg.c_str();
            for(size_t index = 0; index < sc->strg.size(); ++index)
            {
               if(c_str[index] == '\\' && c_str[index + 1] == '0')
               {
                  tmp += '\0';
                  ++index;
               }
               else
               {
                  tmp += c_str[index];
               }
            }
            boost::replace_all(tmp, "\\a", "\a");
            boost::replace_all(tmp, "\\b", "\b");
            boost::replace_all(tmp, "\\t", "\t");
            boost::replace_all(tmp, "\\n", "\n");
            boost::replace_all(tmp, "\\v", "\v");
            boost::replace_all(tmp, "\\f", "\f");
            boost::replace_all(tmp, "\\r", "\r");
            boost::replace_all(tmp, "\\'", "'");
            boost::replace_all(tmp, "\\\"", "\"");
            boost::replace_all(tmp, "\\\\", "\\");
            return tmp;
         }();
         unsigned long long elmt_bitsize;
         std::vector<unsigned long long> dims;

         tree_helper::get_array_dim_and_bitsize(TreeM, sc->type->index, dims, elmt_bitsize);
         if(elmt_bitsize != 8)
         {
            THROW_ERROR("non-standard 8-bit char conversion not supported");
         }
         for(const auto j : string_value)
         {
            auto ull_value = static_cast<unsigned long int>(j);
            trimmed_value = "";
            for(auto ind = 0U; ind < elmt_bitsize; ++ind)
            {
               trimmed_value = trimmed_value + (((1LLU << (elmt_bitsize - ind - 1)) & ull_value) ? '1' : '0');
            }
            init_file.push_back(trimmed_value);
         }
         // String terminator
         init_file.push_back(std::string(elmt_bitsize, '0'));

         const auto type_n = tree_helper::CGetType(var_node);
         THROW_ASSERT(GetPointer<const array_type>(type_n), "expected an array_type");
         dims.clear();
         unsigned long long size_of_data;
         tree_helper::get_array_dim_and_bitsize(TreeM, type_n->index, dims, size_of_data);
         THROW_ASSERT(size_of_data == elmt_bitsize, "something wrong happened");
         auto num_elements = std::accumulate(dims.begin(), dims.end(), 1ULL,
                                             [](unsigned long long a, unsigned long long b) { return a * b; });
         std::string value;
         if(num_elements < (string_value.size() + 1))
         {
            THROW_ERROR("C description not supported: string with undefined size or not correctly initialized " +
                        STR(string_value.size() + 1) + "-" + STR(num_elements));
         }
         num_elements -= string_value.size() + 1;
         init_file.insert(init_file.end(), num_elements, std::string(size_of_data, '0'));
         break;
      }
      case view_convert_expr_K:
      case nop_expr_K:
      {
         const auto ue = GetPointerS<unary_expr>(init_node);
         if(GetPointer<addr_expr>(ue->op))
         {
            write_init(TreeM, ue->op, ue->op, init_file, mem, element_align);
         }
         else if(GetPointer<integer_cst>(ue->op))
         {
            const auto precision = std::max(std::max(8ull, element_align), tree_helper::SizeAlloc(init_node));
            write_init(TreeM, ue->op, ue->op, init_file, mem, precision);
         }
         else
         {
            THROW_ERROR("Something unexpected happened: " + STR(init_node->index) + " | " + ue->op->get_kind_text());
         }
         break;
      }
      case addr_expr_K:
      {
         auto* ae = GetPointerS<addr_expr>(init_node);
         tree_nodeRef addr_expr_op = ae->op;
         auto addr_expr_op_idx = ae->op->index;
         unsigned long long int ull_value = 0;
         const auto precision = tree_helper::SizeAlloc(_init_node);
         switch(addr_expr_op->get_kind())
         {
            case ssa_name_K:
            case var_decl_K:
            case parm_decl_K:
            case string_cst_K:
            {
               THROW_ASSERT(mem->has_base_address(addr_expr_op_idx), "missing base address for: " + ae->ToString());
               ull_value = mem->get_base_address(addr_expr_op_idx, 0);
               break;
            }
            case array_ref_K:
            {
               const auto ar = GetPointerS<array_ref>(addr_expr_op);
               if(GetPointer<integer_cst>(ar->op1))
               {
                  switch(ar->op0->get_kind())
                  {
                     case ssa_name_K:
                     case var_decl_K:
                     case parm_decl_K:
                     case string_cst_K:
                     {
                        const auto step = tree_helper::SizeAlloc(ae->op) / 8;
                        THROW_ASSERT(tree_helper::GetConstValue(ar->op1) >= 0, "");
                        ull_value = mem->get_base_address(ar->op0->index, 0) +
                                    step * static_cast<unsigned long long>(tree_helper::GetConstValue(ar->op1));
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
                        THROW_ERROR("addr_expr-array_ref[0] pattern not supported: " +
                                    std::string(addr_expr_op->get_kind_text()) + " @" + STR(addr_expr_op_idx));
                  }
               }
               else
               {
                  THROW_ERROR("addr_expr-array_ref[0] pattern not supported: " +
                              std::string(addr_expr_op->get_kind_text()) + " @" + STR(addr_expr_op_idx));
               }
               break;
            }
            case function_decl_K:
            {
               THROW_ASSERT(mem->has_base_address(addr_expr_op_idx), "missing base address for: " + ae->ToString());
               ull_value = mem->get_base_address(addr_expr_op_idx, addr_expr_op_idx);
               break;
            }
            case CASE_BINARY_EXPRESSION:
            {
               if(addr_expr_op->get_kind() == mem_ref_K)
               {
                  const auto mr = GetPointerS<mem_ref>(addr_expr_op);
                  const auto op1 = mr->op1;
                  if(op1->get_kind() == integer_cst_K)
                  {
                     THROW_ASSERT(tree_helper::GetConstValue(mr->op1) >= 0, "");
                     const auto offset = static_cast<unsigned long long>(tree_helper::GetConstValue(mr->op1));
                     if(mr->op0->get_kind() == var_decl_K)
                     {
                        ull_value = mem->get_base_address(mr->op0->index, 0) + offset;
                     }
                     else if(mr->op0->get_kind() == addr_expr_K)
                     {
                        const auto base = GetPointerS<addr_expr>(mr->op0)->op;
                        if(base->get_kind() == var_decl_K)
                        {
                           ull_value = mem->get_base_address(base->index, 0) + offset;
                        }
                        else
                        {
                           THROW_ERROR("addr_expr pattern not supported: " +
                                       std::string(addr_expr_op->get_kind_text()) + " @" + STR(addr_expr_op_idx));
                        }
                     }
                     else
                     {
                        THROW_ERROR("addr_expr pattern not supported: " + std::string(addr_expr_op->get_kind_text()) +
                                    " @" + STR(addr_expr_op_idx));
                     }
                  }
                  else
                  {
                     THROW_ERROR("addr_expr pattern not supported: " + std::string(addr_expr_op->get_kind_text()) +
                                 " @" + STR(addr_expr_op_idx));
                  }
               }
               else
               {
                  THROW_ERROR("addr_expr pattern not supported: " + std::string(addr_expr_op->get_kind_text()) + " @" +
                              STR(addr_expr_op_idx));
               }
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
            case fshl_expr_K:
            case fshr_expr_K:
            case insertvalue_expr_K:
            case insertelement_expr_K:
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
               THROW_ERROR("addr_expr pattern not supported: " + std::string(addr_expr_op->get_kind_text()) + " @" +
                           STR(addr_expr_op_idx));
         }
         for(unsigned int ind = 0; ind < precision; ind++)
         {
            trimmed_value = trimmed_value + (((1LLU << (precision - ind - 1)) & ull_value) ? '1' : '0');
         }
         init_file.push_back(trimmed_value);

         break;
      }
      case field_decl_K:
      {
         const auto field_decl_size = tree_helper::SizeAlloc(_init_node);
         if(field_decl_size)
         {
            init_file.push_back(std::string(field_decl_size, '0'));
         }
         break;
      }
      case vector_cst_K:
      {
         const auto vc = GetPointerS<vector_cst>(init_node);
         for(const auto& i : vc->list_of_valu) // vector elements
         {
            write_init(TreeM, i, i, init_file, mem, element_align);
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
      case alignof_expr_K:
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
         THROW_ERROR("elements not yet supported: " + init_node->get_kind_text() + init_node->ToString() +
                     (var_node ? STR(var_node) : ""));
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
         auto* var = GetPointer<decl_node>(sa->var);
         THROW_ASSERT(var, "Call expression does not point to a declaration node");
         pt = GetPointer<pointer_type>(var->type);
      }
      else
      {
         pt = GetPointer<pointer_type>(sa->type);
      }

      THROW_ASSERT(pt, "Declaration node has not information about pointer_type");
      THROW_ASSERT(GetPointer<function_type>(pt->ptd), "Pointer type has not information about pointed function_type");

      return pt->ptd;
   }

   auto* AE = GetPointer<addr_expr>(exp);
   auto* FD = GetPointer<function_decl>(AE->op);
   return FD->type;
}

void fu_binding::set_ports_are_swapped(vertex v, bool condition)
{
   if(condition)
   {
      ports_are_swapped.insert(v);
   }
   else
   {
      ports_are_swapped.erase(v);
   }
}

generic_objRef fu_binding::get(const vertex v) const
{
   const auto statement_index = op_graph->CGetOpNodeInfo(v)->GetNodeId();
   return op_binding.at(statement_index);
}
