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
 * @file fu_binding.cpp
 * @brief Class implementation of the functional-unit binding data structure.
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#include "fu_binding.hpp"

#include "HDLGeneratorManager.hpp"
#include "Parameter.hpp"
#include "allocation_information.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "conn_binding.hpp"
#include "constant_strings.hpp"
#include "custom_set.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "function_behavior.hpp"
#include "functions.hpp"
#include "funit_obj.hpp"
#include "hls.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "language_writer.hpp"
#include "library_manager.hpp"
#include "math_function.hpp"
#include "memory.hpp"
#include "memory_allocation.hpp"
#include "memory_symbol.hpp"
#include "omp_fork_fu_binding.hpp"
#include "reg_binding.hpp"
#include "string_manipulation.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "utility.hpp"

#include <boost/algorithm/string/replace.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <regex>
#include <string>
#include <utility>
#include <vector>

const unsigned int fu_binding::UNKNOWN = std::numeric_limits<unsigned int>::max();

namespace
{
   struct dataflow_memory_port_info
   {
      unsigned int var;
      std::string kind;
      std::string index;
   };

   enum class dataflow_memory_role
   {
      none,
      reader,
      writer
   };

   bool parse_dataflow_memory_port(const structural_objectRef& port, dataflow_memory_port_info& info)
   {
      const std::regex dataflow_memory_port_re("^(?:p)?_DF_bambu_[0-9]+_([0-9]+)FO[0-9]+_(q|d|address|ce|we)([0-9]*)$");
      std::smatch match;
      const auto port_name = GetPointerS<const port_o>(port)->get_id();
      if(!std::regex_match(port_name, match, dataflow_memory_port_re))
      {
         return false;
      }
      info.var = static_cast<unsigned int>(std::stoul(match[1].str()));
      info.kind = match[2].str();
      info.index = match[3].str();
      return true;
   }

   dataflow_memory_role infer_dataflow_memory_role(const HLS_managerRef& HLSMgr, const unsigned int top_id,
                                                   const structural_objectRef& module, const unsigned int var)
   {
      if(const auto ding_dong_candidate = HLSMgr->get_ding_dong_buffer_candidate(top_id, var))
      {
         static const auto top_function_wrapper_prefix = std::string("PBI_");
         const auto mod_id = GET_TYPE_NAME(module);
         const auto msymbol = (mod_id.size() && starts_with(mod_id, top_function_wrapper_prefix)) ?
                                  mod_id.substr(top_function_wrapper_prefix.size()) :
                                  mod_id;
         if(const auto function_decl = HLSMgr->get_ir_manager()->GetFunction(msymbol))
         {
            const auto function_id = function_decl->index;
            if(function_id == ding_dong_candidate->producer_function_id)
            {
               return dataflow_memory_role::writer;
            }
            if(function_id == ding_dong_candidate->consumer_function_id)
            {
               return dataflow_memory_role::reader;
            }
         }
         return dataflow_memory_role::none;
      }
      const auto has_matching_kind = [&](const auto port_getter, const unsigned int port_count,
                                         const std::initializer_list<const char*> kinds) {
         for(unsigned int port_index = 0; port_index < port_count; ++port_index)
         {
            dataflow_memory_port_info port_info;
            if(!parse_dataflow_memory_port(port_getter(port_index), port_info) || port_info.var != var)
            {
               continue;
            }
            if(std::find_if(kinds.begin(), kinds.end(), [&](const auto* kind) { return port_info.kind == kind; }) !=
               kinds.end())
            {
               return true;
            }
         }
         return false;
      };
      const auto module_obj = GetPointerS<const module_o>(module);
      const auto has_reader_port =
          has_matching_kind([&](const auto port_index) { return module_obj->get_in_port(port_index); },
                            module_obj->get_in_port_size(), {"q"});
      const auto has_writer_port =
          has_matching_kind([&](const auto port_index) { return module_obj->get_out_port(port_index); },
                            module_obj->get_out_port_size(), {"d", "we"});
      THROW_ASSERT(!(has_reader_port && has_writer_port),
                   "Ding-dong buffers currently support only one reader or one writer per dataflow module: " +
                       module->get_path());
      if(has_reader_port)
      {
         return dataflow_memory_role::reader;
      }
      if(has_writer_port)
      {
         return dataflow_memory_role::writer;
      }
      return dataflow_memory_role::none;
   }

   structural_objectRef get_indexed_storage_port(const structural_objectRef& storage, const std::string& port_name,
                                                 so_kind requested_kind, const std::string& index)
   {
      const auto find_direct_storage_port = [&](const so_kind expected_kind) -> structural_objectRef {
         const auto storage_module = GetPointer<module_o>(storage);
         if(!storage_module)
         {
            return structural_objectRef();
         }
         const auto find_port = [&](const auto port_getter, const unsigned int port_count) -> structural_objectRef {
            for(unsigned int port_index = 0; port_index < port_count; ++port_index)
            {
               const auto port = port_getter(port_index);
               if(port->get_id() == port_name && port->get_kind() == expected_kind)
               {
                  return port;
               }
            }
            return structural_objectRef();
         };
         if(const auto port = find_port([&](const auto port_index) { return storage_module->get_in_port(port_index); },
                                        storage_module->get_in_port_size()))
         {
            return port;
         }
         if(const auto port = find_port([&](const auto port_index) { return storage_module->get_out_port(port_index); },
                                        storage_module->get_out_port_size()))
         {
            return port;
         }
         return find_port([&](const auto port_index) { return storage_module->get_in_out_port(port_index); },
                          storage_module->get_in_out_port_size());
      };

      if(!index.empty())
      {
         if(const auto vector_port = find_direct_storage_port(port_vector_o_K))
         {
            const auto port_index = static_cast<unsigned int>(std::stoul(index));
            THROW_ASSERT(port_index < GetPointerS<const port_o>(vector_port)->get_ports_size(),
                         "Port index out of range for " + vector_port->get_path());
            return GetPointerS<port_o>(vector_port)->get_port(port_index);
         }
         if(requested_kind == port_o_K && index == "0")
         {
            return find_direct_storage_port(port_o_K);
         }
         return structural_objectRef();
      }
      return find_direct_storage_port(requested_kind);
   }

   structural_objectRef get_storage_port(const structural_objectRef& storage,
                                         const std::vector<std::string>& port_names, so_kind requested_kind,
                                         const std::string& index)
   {
      for(const auto& port_name : port_names)
      {
         if(const auto port = get_indexed_storage_port(storage, port_name, requested_kind, index))
         {
            return port;
         }
      }
      return structural_objectRef();
   }

   void connect_constant_to_port(const structural_managerRef& SM, const structural_objectRef& circuit,
                                 const structural_objectRef& port, const std::string& constant_name,
                                 const std::string& constant_value)
   {
      if(!port)
      {
         return;
      }
      if(port->get_kind() == port_vector_o_K)
      {
         const auto port_size = GetPointerS<const port_o>(port)->get_ports_size();
         for(unsigned int port_index = 0; port_index < port_size; ++port_index)
         {
            const auto indexed_port = GetPointerS<port_o>(port)->get_port(port_index);
            const auto indexed_constant = SM->add_constant(constant_name + "_" + STR(port_index), circuit,
                                                           indexed_port->get_typeRef(), constant_value);
            SM->add_connection(indexed_port, indexed_constant);
         }
         return;
      }
      const auto constant = SM->add_constant(constant_name, circuit, port->get_typeRef(), constant_value);
      SM->add_connection(port, constant);
   }

   unsigned int get_dataflow_memory_channel_count(const structural_objectRef& circuit, const unsigned int top_id,
                                                  const unsigned int var)
   {
      const std::regex bundle_re("^_DF_bambu_" + STR(top_id) + "_" + STR(var) +
                                 "FO[0-9]+_(?:q|d|address|ce|we)([0-9]+)$");
      unsigned int max_channel = 0;
      const auto inspect_port = [&](const structural_objectRef& port) {
         std::smatch match;
         const auto port_name = GetPointerS<const port_o>(port)->get_id();
         if(std::regex_match(port_name, match, bundle_re))
         {
            max_channel = std::max(max_channel, static_cast<unsigned int>(std::stoul(match[1].str())));
         }
      };
      const auto circuit_obj = GetPointerS<const module_o>(circuit);
      for(unsigned int module_index = 0; module_index < circuit_obj->get_internal_objects_size(); ++module_index)
      {
         const auto module = circuit_obj->get_internal_object(module_index);
         const auto module_obj = GetPointer<module_o>(module);
         if(!module_obj)
         {
            continue;
         }
         for(unsigned int port_index = 0; port_index < module_obj->get_in_port_size(); ++port_index)
         {
            inspect_port(module_obj->get_in_port(port_index));
         }
         for(unsigned int port_index = 0; port_index < module_obj->get_out_port_size(); ++port_index)
         {
            inspect_port(module_obj->get_out_port(port_index));
         }
      }
      return max_channel + 1U;
   }

   unsigned long long get_dataflow_memory_port_bitsize(const structural_objectRef& circuit, const unsigned int var,
                                                       const std::string& kind)
   {
      unsigned long long max_bitsize = 0;
      unsigned long long max_interface_bitsize = 0;
      const auto inspect_port = [&](const structural_objectRef& port) {
         dataflow_memory_port_info port_info;
         if(parse_dataflow_memory_port(port, port_info) && port_info.var == var && port_info.kind == kind)
         {
            const auto bitsize = STD_GET_SIZE(port->get_typeRef());
            max_bitsize = std::max(max_bitsize, bitsize);
            if(starts_with(GetPointerS<const port_o>(port)->get_id(), "p_"))
            {
               max_interface_bitsize = std::max(max_interface_bitsize, bitsize);
            }
         }
      };
      const auto circuit_obj = GetPointerS<const module_o>(circuit);
      for(unsigned int module_index = 0; module_index < circuit_obj->get_internal_objects_size(); ++module_index)
      {
         const auto module = circuit_obj->get_internal_object(module_index);
         const auto module_obj = GetPointer<module_o>(module);
         if(!module_obj)
         {
            continue;
         }
         for(unsigned int port_index = 0; port_index < module_obj->get_in_port_size(); ++port_index)
         {
            inspect_port(module_obj->get_in_port(port_index));
         }
         for(unsigned int port_index = 0; port_index < module_obj->get_out_port_size(); ++port_index)
         {
            inspect_port(module_obj->get_out_port(port_index));
         }
      }
      return max_interface_bitsize != 0 ? max_interface_bitsize : max_bitsize;
   }

   structural_objectRef find_direct_module_port(const structural_objectRef& module, const std::string& port_name)
   {
      const auto module_obj = GetPointerS<const module_o>(module);
      for(unsigned int port_index = 0; port_index < module_obj->get_in_port_size(); ++port_index)
      {
         const auto port = module_obj->get_in_port(port_index);
         if(port->get_id() == port_name)
         {
            return port;
         }
      }
      for(unsigned int port_index = 0; port_index < module_obj->get_out_port_size(); ++port_index)
      {
         const auto port = module_obj->get_out_port(port_index);
         if(port->get_id() == port_name)
         {
            return port;
         }
      }
      for(unsigned int port_index = 0; port_index < module_obj->get_in_out_port_size(); ++port_index)
      {
         const auto port = module_obj->get_in_out_port(port_index);
         if(port->get_id() == port_name)
         {
            return port;
         }
      }
      return structural_objectRef();
   }

   void resize_named_port(const structural_objectRef& module, const std::string& port_name,
                          const unsigned long long bitsize, const int debug_level)
   {
      if(bitsize == 0)
      {
         return;
      }
      if(const auto port = find_direct_module_port(module, port_name))
      {
         port_o::resize_std_port(bitsize, 0U, debug_level, port);
      }
   }

   void connect_dataflow_memory_ports(const HLS_managerRef& HLSMgr, const unsigned int top_id,
                                      const structural_managerRef& SM, const structural_objectRef& circuit,
                                      const std::map<unsigned int, structural_objectRef>& storage_by_var,
                                      unsigned int& unique_id)
   {
      for(unsigned int module_index = 0; module_index < GetPointerS<module_o>(circuit)->get_internal_objects_size();
          ++module_index)
      {
         const auto module = GetPointerS<module_o>(circuit)->get_internal_object(module_index);
         if(!GetPointer<module_o>(module))
         {
            continue;
         }
         const auto module_obj = GetPointerS<module_o>(module);
         for(unsigned int port_index = 0; port_index < module_obj->get_in_port_size(); ++port_index)
         {
            const auto port = module_obj->get_in_port(port_index);
            dataflow_memory_port_info port_info;
            if(!parse_dataflow_memory_port(port, port_info) || port_info.kind != "q")
            {
               continue;
            }
            const auto storage_it = storage_by_var.find(port_info.var);
            if(storage_it == storage_by_var.end())
            {
               continue;
            }
            const auto storage_port =
                get_indexed_storage_port(storage_it->second, "out1", port->get_kind(), port_info.index);
            THROW_ASSERT(storage_port, "Missing local storage output port for " + port->get_path());
            fu_binding::add_smart_connection(storage_port, port, unique_id++, circuit, SM);
         }
         for(unsigned int port_index = 0; port_index < module_obj->get_out_port_size(); ++port_index)
         {
            const auto port = module_obj->get_out_port(port_index);
            dataflow_memory_port_info port_info;
            if(!parse_dataflow_memory_port(port, port_info))
            {
               continue;
            }
            const auto storage_it = storage_by_var.find(port_info.var);
            if(storage_it == storage_by_var.end())
            {
               continue;
            }
            const auto module_role = infer_dataflow_memory_role(HLSMgr, top_id, module, port_info.var);
            std::string selected_storage_port_name;
            const auto storage_port = [&]() -> structural_objectRef {
               if(port_info.kind == "address")
               {
                  if(module_role == dataflow_memory_role::reader)
                  {
                     selected_storage_port_name =
                         get_storage_port(storage_it->second, {"in2r"}, port->get_kind(), port_info.index) ? "in2r" :
                                                                                                             "in2";
                     return get_storage_port(storage_it->second, {selected_storage_port_name}, port->get_kind(),
                                             port_info.index);
                  }
                  if(module_role == dataflow_memory_role::writer)
                  {
                     selected_storage_port_name =
                         get_storage_port(storage_it->second, {"in2w"}, port->get_kind(), port_info.index) ? "in2w" :
                                                                                                             "in2";
                     return get_storage_port(storage_it->second, {selected_storage_port_name}, port->get_kind(),
                                             port_info.index);
                  }
                  if(get_storage_port(storage_it->second, {"in2r"}, port->get_kind(), port_info.index))
                  {
                     selected_storage_port_name = "in2r";
                  }
                  else if(get_storage_port(storage_it->second, {"in2w"}, port->get_kind(), port_info.index))
                  {
                     selected_storage_port_name = "in2w";
                  }
                  else
                  {
                     selected_storage_port_name = "in2";
                  }
                  return get_storage_port(storage_it->second, {selected_storage_port_name}, port->get_kind(),
                                          port_info.index);
               }
               if(port_info.kind == "ce")
               {
                  if(HLSMgr->get_ding_dong_buffer_candidate(top_id, port_info.var) &&
                     module_role == dataflow_memory_role::writer)
                  {
                     return structural_objectRef();
                  }
                  selected_storage_port_name = "sel_LOAD";
                  return get_storage_port(storage_it->second, {selected_storage_port_name}, port->get_kind(),
                                          port_info.index);
               }
               if(port_info.kind == "we")
               {
                  if(HLSMgr->get_ding_dong_buffer_candidate(top_id, port_info.var) &&
                     module_role != dataflow_memory_role::writer)
                  {
                     return structural_objectRef();
                  }
                  selected_storage_port_name = "sel_STORE";
                  return get_storage_port(storage_it->second, {selected_storage_port_name}, port->get_kind(),
                                          port_info.index);
               }
               if(port_info.kind == "d")
               {
                  if(HLSMgr->get_ding_dong_buffer_candidate(top_id, port_info.var) &&
                     module_role != dataflow_memory_role::writer)
                  {
                     return structural_objectRef();
                  }
                  selected_storage_port_name = "in1";
                  return get_storage_port(storage_it->second, {selected_storage_port_name}, port->get_kind(),
                                          port_info.index);
               }
               return structural_objectRef();
            }();
            if(!storage_port)
            {
               continue;
            }
            if(port->get_kind() == port_o_K && storage_port->get_kind() == port_o_K)
            {
               const auto* storage_module = GetPointerS<const module_o>(storage_it->second);
               const auto storage_bitsize_parameter =
                   starts_with(selected_storage_port_name, "in2") &&
                           storage_module->ExistsParameter("BITSIZE_proxy_" + selected_storage_port_name) ?
                       "BITSIZE_proxy_" + selected_storage_port_name :
                       "BITSIZE_" + selected_storage_port_name;
               const auto storage_bitsize = storage_module->ExistsParameter(storage_bitsize_parameter) ?
                                                std::stoull(storage_module->GetParameter(storage_bitsize_parameter)) :
                                                STD_GET_SIZE(storage_port->get_typeRef());
               if(storage_bitsize > 1U && port->get_typeRef()->type == structural_type_descriptor::BOOL)
               {
                  port->get_typeRef()->type = structural_type_descriptor::VECTOR_BOOL;
               }
               if(storage_bitsize > 1U && storage_port->get_typeRef()->type == structural_type_descriptor::BOOL)
               {
                  storage_port->get_typeRef()->type = structural_type_descriptor::VECTOR_BOOL;
               }
               port->type_resize(storage_bitsize);
               storage_port->type_resize(storage_bitsize);
            }
            fu_binding::add_smart_connection(port, storage_port, unique_id++, circuit, SM);
         }
      }
   }

} // namespace

fu_binding::fu_binding(const HLS_managerConstRef _HLSMgr, const unsigned int _function_id,
                       const ParameterConstRef _parameters)
    : allocation_information(_HLSMgr->get_HLS(_function_id)->allocation_information),
      IRM(_HLSMgr->get_ir_manager()),
      op_graph(_HLSMgr->CGetFunctionBehavior(_function_id)->GetOpGraph(FunctionBehavior::DFG)),
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
      IRM(original.IRM),
      op_graph(original.op_graph),
      parameters(original.parameters),
      debug_level(parameters->get_class_debug_level(GET_CLASS(*this))),
      has_resource_sharing_p(original.has_resource_sharing_p)
{
}

fu_bindingRef fu_binding::create_fu_binding(const HLS_managerConstRef _HLSMgr, const unsigned int _function_id,
                                            const ParameterConstRef _parameters)
{
   const auto fname = ir_helper::GetFunctionName(_HLSMgr->get_ir_manager()->GetIRNode(_function_id));
   if(boost::algorithm::starts_with(fname, "__kmp_bambu_fork_call"))
   {
      return fu_bindingRef(new omp_fork_fu_binding(_HLSMgr, _function_id, _parameters));
   }
   return fu_bindingRef(new fu_binding(_HLSMgr, _function_id, _parameters));
}

void fu_binding::bind(OpGraph::vertex_descriptor v, unsigned int unit, unsigned int index)
{
   const auto key = std::make_pair(unit, index);
   if(!unique_table.count(key))
   {
      unique_table[key] =
          generic_objRef(new funit_obj(allocation_information->get_string_name(unit) + "_i" + STR(index), unit, index));
   }
   const auto statement_index = op_graph.CGetNodeInfo(v).GetNodeId();
   op_binding[statement_index] = unique_table[key];
   if(!operations.count(key))
   {
      operations.insert(std::make_pair(key, OpVertexSet(&op_graph.GetGraphsCollection())));
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
      return OpVertexSet(&op_graph.GetGraphsCollection());
   }
   return operations.find(std::make_pair(unit, index))->second;
}

const funit_obj& fu_binding::operator[](OpGraph::vertex_descriptor v)
{
   const auto statement_index = op_graph.CGetNodeInfo(v).GetNodeId();
   THROW_ASSERT(op_binding.count(statement_index), "vertex not preset");
   return *(GetPointer<funit_obj>(op_binding.at(statement_index)));
}

bool fu_binding::is_assigned(OpGraph::vertex_descriptor v) const
{
   const auto statement_index = op_graph.CGetNodeInfo(v).GetNodeId();
   return is_assigned(statement_index);
}

bool fu_binding::is_assigned(const unsigned int statement_index) const
{
   return op_binding.count(statement_index);
}

std::list<unsigned int> fu_binding::get_allocation_list() const
{
   std::list<unsigned int> allocation_list;
   for(const auto& alloc : allocation_map)
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

void fu_binding::reset_allocation(unsigned int unit)
{
   allocation_map[unit] = 0;
}

unsigned int fu_binding::get_assign(OpGraph::vertex_descriptor v) const
{
   const auto statement_index = op_graph.CGetNodeInfo(v).GetNodeId();
   return get_assign(statement_index);
}

unsigned int fu_binding::get_assign(const unsigned int statement_index) const
{
   THROW_ASSERT(op_binding.find(statement_index) != op_binding.end(),
                "Operation " + IRM->GetIRNode(statement_index)->ToString() + " not assigned");
   THROW_ASSERT(GetPointer<funit_obj>(op_binding.find(statement_index)->second), "");
   return GetPointer<funit_obj>(op_binding.at(statement_index))->get_fu();
}

std::string fu_binding::get_fu_name(OpGraph::vertex_descriptor v) const
{
   return allocation_information->get_string_name(get_assign(v));
}

unsigned int fu_binding::get_index(OpGraph::vertex_descriptor v) const
{
   const auto statement_index = op_graph.CGetNodeInfo(v).GetNodeId();
   return GetPointer<funit_obj>(op_binding.at(statement_index))->get_index();
}

structural_objectRef fu_binding::add_gate(const HLS_managerRef HLSMgr, const hlsRef HLS, const technology_nodeRef fu,
                                          const std::string& name, const OpVertexSet& ops,
                                          structural_objectRef clock_port, structural_objectRef reset_port) const
{
   const auto FB = HLSMgr->CGetFunctionBehavior(HLS->functionId);
   const auto data = FB->GetOpGraph(FunctionBehavior::CFG);
   const auto& SM = HLS->datapath;
   const auto circuit = SM->get_circ();
   structural_objectRef curr_gate(new module_o(debug_level, circuit));
   const auto* fu_unit = GetPointer<functional_unit>(fu);
   THROW_ASSERT(fu, "Expected a technology node while instantiating a gate for function " +
                        ir_helper::GetFunctionName(IRM->GetIRNode(HLS->functionId)));
   THROW_ASSERT(fu_unit, "Expected a functional_unit while instantiating a gate for function " +
                             ir_helper::GetFunctionName(IRM->GetIRNode(HLS->functionId)) +
                             ", got technology node kind " + fu->get_kind_text());
   /// creating structural_manager starting from technology_node
   structural_objectRef curr_lib_instance;
   if(fu_unit->fu_template_name == "")
   {
      THROW_ASSERT(fu_unit->CM, "Missing structural manager for functional unit " + fu_unit->functional_unit_name +
                                    " while instantiating a gate for function " +
                                    ir_helper::GetFunctionName(IRM->GetIRNode(HLS->functionId)));
      curr_lib_instance = fu_unit->CM->get_circ();
   }
   else
   {
      const auto template_name = fu_unit->fu_template_name;
      const auto library_name = HLS->HLS_D->get_technology_manager()->get_library(template_name);
      THROW_ASSERT(!library_name.empty(), "Template " + template_name + " referenced by functional unit " +
                                              fu_unit->functional_unit_name + " is not associated with any library");
      const auto template_fu = HLS->HLS_D->get_technology_manager()->get_fu(template_name, library_name);
      THROW_ASSERT(template_fu, "Template " + template_name + " referenced by functional unit " +
                                    fu_unit->functional_unit_name + " is missing from library " + library_name);
      const auto* fu_template = GetPointer<functional_unit_template>(template_fu);
      THROW_ASSERT(fu_template, "Technology node " + template_name + " from library " + library_name +
                                    " is not a functional_unit_template");
      THROW_ASSERT(fu_template->FU, "Functional unit template " + template_name + " from library " + library_name +
                                        " does not contain a backing functional unit");
      const auto* backing_fu = GetPointer<functional_unit>(fu_template->FU);
      THROW_ASSERT(backing_fu, "Backing node for template " + template_name + " from library " + library_name +
                                   " is not a functional_unit");
      THROW_ASSERT(backing_fu->CM, "Backing functional unit for template " + template_name + " from library " +
                                       library_name + " does not provide a structural manager");
      curr_lib_instance = backing_fu->CM->get_circ();
   }
   THROW_ASSERT(curr_lib_instance, "Structural description not provided: check the library given. Component: " +
                                       fu_unit->functional_unit_name);

   curr_lib_instance->copy(curr_gate);
   if(ops.size() == 1)
   {
      curr_gate->set_id("fu_" + data.CGetNodeInfo(*(ops.begin())).vertex_name);
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
   GetPointerS<module_o>(circuit)->add_internal_object(curr_gate);

   return curr_gate;
}

void fu_binding::kill_s_memory_ports(structural_objectRef curr_gate)
{
   for(const auto p_name :
       {"S_oe_ram", "S_we_ram", "S_addr_ram", "S_Wdata_ram", "S_data_ram_size", "Sout_Rdata_ram", "Sout_DataRdy"})
   {
      const auto port = curr_gate->find_member(p_name, port_o_K, curr_gate);
      GetPointerS<port_o>(port)->set_is_memory(false);
   }
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
      auto inPortSize = static_cast<unsigned int>(GetPointer<module_o>(curr_gate)->get_in_port_size());
      for(unsigned int currentPort = 0; currentPort < inPortSize; ++currentPort)
      {
         structural_objectRef curr_port = GetPointer<module_o>(curr_gate)->get_in_port(currentPort);
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
      auto outPortSize = static_cast<unsigned int>(GetPointer<module_o>(curr_gate)->get_out_port_size());
      for(unsigned int currentPort = 0; currentPort < outPortSize; ++currentPort)
      {
         structural_objectRef curr_port = GetPointer<module_o>(curr_gate)->get_out_port(currentPort);
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

      const auto inPortSize = GetPointer<module_o>(wrapped_fu_unit)->get_in_port_size();
      for(auto currentPort = 0U; currentPort < inPortSize; ++currentPort)
      {
         const auto curr_port = GetPointerS<module_o>(wrapped_fu_unit)->get_in_port(currentPort);
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
      const auto outPortSize = GetPointer<module_o>(wrapped_fu_unit)->get_out_port_size();
      for(auto currentPort = 0U; currentPort < outPortSize; ++currentPort)
      {
         const auto wrapped_port_proxy_out_i = GetPointerS<module_o>(wrapped_fu_unit)->get_out_port(currentPort);
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
   const auto start_port = GetPointer<module_o>(circuit)->find_member(START_PORT_NAME, port_o_K, circuit);
   const auto done_port = GetPointer<module_o>(circuit)->find_member(DONE_PORT_NAME, port_o_K, circuit);
   structural_objectRef in_chain = start_port;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding parameter ports");
   for(const auto& function_parameter : function_parameters)
   {
      if(HLSMgr->Rmem->is_parm_decl_copied(function_parameter) &&
         !HLSMgr->Rmem->is_parm_decl_stored(function_parameter))
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Managing parameter copy: " + STR(function_parameter));
         const auto fu_lib_unit = TechM->get_fu(MEMCPY_STD, WORK_LIBRARY);
         THROW_ASSERT(fu_lib_unit, "functional unit not available: check the library given. Component: " MEMCPY_STD);
         const auto curr_gate = add_gate(HLSMgr, HLS, fu_lib_unit, "parameter_manager_" + STR(function_parameter),
                                         OpVertexSet(&op_graph.GetGraphsCollection()), clock_port, reset_port);
         const auto curr_gate_m = GetPointerS<module_o>(curr_gate);
         const auto port_obj = HLS->Rconn->get_port(function_parameter, conn_binding::IN);
         const auto in_par = port_obj->get_out_sign();
         const auto src = curr_gate_m->find_member("src", port_o_K, curr_gate);
         SM->add_connection(in_par, src);
         const auto dest = curr_gate_m->find_member("dest", port_o_K, curr_gate);

         auto const_obj = SM->add_constant(
             "memcpy_dest_" + HLSMgr->Rmem->get_symbol(function_parameter, HLS->functionId)->get_symbol_name(), circuit,
             dest->get_typeRef(), HLSMgr->Rmem->get_symbol(function_parameter, HLS->functionId)->get_symbol_name());
         SM->add_connection(dest, const_obj);

         const auto n = curr_gate_m->find_member("len", port_o_K, curr_gate);
         const auto n_obj = SM->add_constant("constant_len_" + STR(function_parameter), circuit, n->get_typeRef(),
                                             STR(ir_helper::SizeAlloc(IRM->GetIRNode(function_parameter)) / 8));
         SM->add_connection(n, n_obj);
         THROW_ASSERT(in_chain, "missing in chain element");
         const auto start_obj = curr_gate_m->find_member(START_PORT_NAME, port_o_K, curr_gate);

         if(HLS->registered_inputs && in_chain == start_port)
         {
            const auto delay_unit = [&]() {
               const auto reset_type = parameters->getOption<std::string>(OPT_reset_type);
               return TechM->get_fu(reset_type == "sync" ? register_SR : register_AR, LIBRARY_STD);
            }();
            THROW_ASSERT(delay_unit, "");
            const auto delay_gate = add_gate(HLSMgr, HLS, delay_unit, "start_delayed_" + STR(function_parameter),
                                             OpVertexSet(&op_graph.GetGraphsCollection()), clock_port, reset_port);
            const auto in1 = GetPointerS<module_o>(delay_gate)->get_in_port(2);
            const auto sign = SM->add_sign(START_PORT_NAME "_" + STR(sign_id), circuit, start_obj->get_typeRef());
            ++sign_id;
            SM->add_connection(sign, in_chain);
            SM->add_connection(sign, in1);
         }

         if(in_chain == start_port)
         {
            SM->add_connection(in_chain, start_obj);
         }
         else
         {
            const auto sign = SM->add_sign(START_PORT_NAME "_" + STR(sign_id), circuit, in_chain->get_typeRef());
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
         const auto is_multiport = FB->GetChannelsType() == MemoryAllocation_ChannelsType::MEM_ACC_NN;
         const auto fu_lib_unit = TechM->get_fu(is_multiport ? MEMSTORE_STDN : MEMSTORE_STD, LIBRARY_STD_FU);
         THROW_ASSERT(fu_lib_unit, "functional unit not available: check the library given. Component: " +
                                       STR(is_multiport ? MEMSTORE_STDN : MEMSTORE_STD));
         const auto max_n_ports = FB->GetChannelsNumber();
         const auto curr_gate = add_gate(HLSMgr, HLS, fu_lib_unit, "parameter_manager_" + STR(function_parameter),
                                         OpVertexSet(&op_graph.GetGraphsCollection()), clock_port, reset_port);
         const auto port_obj = HLS->Rconn->get_port(function_parameter, conn_binding::IN);
         const auto in_par = port_obj->get_out_sign();
         const auto data = GetPointer<module_o>(curr_gate)->find_member("data", port_o_K, curr_gate);
         data->type_resize(STD_GET_SIZE(in_par->get_typeRef()));
         SM->add_connection(in_par, data);

         const auto size = GetPointer<module_o>(curr_gate)->find_member("size", port_o_K, curr_gate);
         size->type_resize(STD_GET_SIZE(in_par->get_typeRef()));

         const std::string parameter_value =
             (parameters->getOption<HDLWriter_Language>(OPT_writer_language) == HDLWriter_Language::VHDL) ?
                 std::string("\"") +
                     NumberToBinaryString(STD_GET_SIZE(in_par->get_typeRef()), STD_GET_SIZE(in_par->get_typeRef())) +
                     std::string("\"") :
                 STR(STD_GET_SIZE(in_par->get_typeRef()));
         auto size_const_obj = SM->add_constant(
             "size_par_" + HLSMgr->Rmem->get_symbol(function_parameter, HLS->functionId)->get_symbol_name(), circuit,
             size->get_typeRef(), parameter_value);
         SM->add_connection(size, size_const_obj);

         const auto addr = GetPointer<module_o>(curr_gate)->find_member("addr", port_o_K, curr_gate);
         addr->type_resize(bus_addr_bitsize);

         auto addr_const_obj = SM->add_constant(
             "addr_par_" + HLSMgr->Rmem->get_symbol(function_parameter, HLS->functionId)->get_symbol_name(), circuit,
             addr->get_typeRef(), parameter_value);

         SM->add_connection(addr, addr_const_obj);

         THROW_ASSERT(in_chain, "missing in chain element");
         structural_objectRef start_obj =
             GetPointer<module_o>(curr_gate)->find_member(START_PORT_NAME, port_o_K, curr_gate);
         if(HLS->registered_inputs && in_chain == start_port)
         {
            technology_nodeRef delay_unit;
            auto reset_type = parameters->getOption<std::string>(OPT_reset_type);
            if(reset_type == "sync")
            {
               delay_unit = TechM->get_fu(register_SR, LIBRARY_STD);
            }
            else
            {
               delay_unit = TechM->get_fu(register_AR, LIBRARY_STD);
            }
            const auto delay_gate = add_gate(HLSMgr, HLS, delay_unit, "start_delayed_" + STR(function_parameter),
                                             OpVertexSet(&op_graph.GetGraphsCollection()), clock_port, reset_port);
            const auto delay_gate_m = GetPointerS<module_o>(delay_gate);
            const auto in1 = delay_gate_m->get_in_port(2);
            const auto out1 = delay_gate_m->get_out_port(0);
            const auto sign = SM->add_sign(START_PORT_NAME "_" + STR(sign_id), circuit, start_obj->get_typeRef());
            ++sign_id;
            SM->add_connection(sign, in_chain);
            SM->add_connection(sign, in1);
            in_chain = out1;
         }

         if(in_chain == start_port)
         {
            SM->add_connection(in_chain, start_obj);
         }
         else
         {
            structural_objectRef sign =
                SM->add_sign(START_PORT_NAME "_" + STR(sign_id), circuit, in_chain->get_typeRef());
            ++sign_id;
            SM->add_connection(sign, in_chain);
            SM->add_connection(sign, start_obj);
         }
         in_chain = GetPointer<module_o>(curr_gate)->find_member(DONE_PORT_NAME, port_o_K, curr_gate);
         /// component specialization
         for(unsigned int i = 0; i < GetPointer<module_o>(curr_gate)->get_in_port_size(); i++)
         {
            structural_objectRef port = GetPointer<module_o>(curr_gate)->get_in_port(i);
            if(is_multiport && port->get_kind() == port_vector_o_K && GetPointer<port_o>(port)->get_ports_size() == 0)
            {
               GetPointer<port_o>(port)->add_n_ports(static_cast<unsigned int>(max_n_ports), port);
            }
            port_o::resize_if_busport(bus_size_bitsize, bus_addr_bitsize, bus_data_bitsize, bus_tag_bitsize, port);
         }
         for(unsigned int i = 0; i < GetPointer<module_o>(curr_gate)->get_out_port_size(); i++)
         {
            structural_objectRef port = GetPointer<module_o>(curr_gate)->get_out_port(i);
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

   const auto omp_info = FB->GetOMPInfo();
   const auto is_sparse_memory =
       parameters->isOption(OPT_sparse_memory) && parameters->getOption<bool>(OPT_sparse_memory);
   const auto& ding_dong_candidates = HLSMgr->get_ding_dong_buffer_candidates(HLS->functionId);
   std::map<unsigned int, unsigned int> memory_units = allocation_information->get_memory_units();
   std::map<unsigned int, structural_objectRef> mem_obj;
   std::map<unsigned int, structural_objectRef> storage_by_var;
   for(const auto& m : memory_units)
   {
      const auto fu_type_id = m.first;
      auto var = m.second;
      if(ding_dong_candidates.count(var))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "---Skipping standard memory instantiation for ding-dong candidate " +
                            HLSMgr->CGetFunctionBehavior(HLS->functionId)->CGetBehavioralHelper()->PrintVariable(var));
         continue;
      }
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
         return memory_type == MEMORY_TYPE_SYNCHRONOUS_SDS || memory_type == MEMORY_TYPE_SYNCHRONOUS_SDS1 ||
                memory_type == MEMORY_TYPE_SYNCHRONOUS_SDS_BUS || memory_type == MEMORY_TYPE_SYNCHRONOUS_SDS_BUS1 ||
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
         OpVertexSet operations_set(&op_graph.GetGraphsCollection());
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
         structural_objectRef curr_gate =
             add_gate(HLSMgr, HLS, fu_lib_unit, name + "_" + STR(num / n_channels),
                      OpVertexSet(&op_graph.GetGraphsCollection()), clock_port, reset_port);
         specialise_fu(HLSMgr, HLS, curr_gate, fu_type_id, operations_set, var);
         specialize_memory_unit(HLSMgr, HLS, curr_gate, var, base_address, rangesize, is_splitted_memory,
                                is_sparse_memory, is_sds_memory, omp_info);
         check_parametrization(curr_gate);
         mem_obj[fu_type_id] = curr_gate;
         storage_by_var[var] = curr_gate;
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
      const auto curr_gate = add_gate(HLSMgr, HLS, fu_lib_unit, wu->second + "_instance",
                                      OpVertexSet(&op_graph.GetGraphsCollection()), clock_port, reset_port);
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
         if(allocation_information->is_memory_unit(i))
         {
            continue;
         }

         const auto module_obj = get(i, num);
         THROW_ASSERT(module_obj,
                      "module missing: " + allocation_information->get_fu_name(i).first + " instance " + STR(num));
         const auto name = module_obj->get_string();

         structural_objectRef curr_gate;
         if(wrapped_units.find(i) != wrapped_units.end())
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
                                     OpVertexSet(&op_graph.GetGraphsCollection()) :
                                     mapped_operations,
                                 clock_port, reset_port);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Added " + allocation_information->get_fu_name(i).first + " to the circuit");
            has_resource_sharing_p = has_resource_sharing_p || (mapped_operations.size() > 1);
            std::string current_op;
            if(mapped_operations.size())
            {
               current_op =
                   ir_helper::NormalizeTypename(op_graph.CGetNodeInfo(*(mapped_operations.begin())).GetOperation());
            }
            if(current_op == BUILTIN_WAIT_CALL)
            {
               has_resource_sharing_p = true;
               const auto site = *mapped_operations.begin();
               const auto vertex_node_id = op_graph.CGetNodeInfo(site).GetNodeId();

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
               auto inPortSize = static_cast<unsigned int>(GetPointer<module_o>(curr_gate)->get_in_port_size());
               for(unsigned int currentPort = 0; currentPort < inPortSize; ++currentPort)
               {
                  structural_objectRef curr_port = GetPointer<module_o>(curr_gate)->get_in_port(currentPort);
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
               auto outPortSize = static_cast<unsigned int>(GetPointer<module_o>(curr_gate)->get_out_port_size());
               for(unsigned int currentPort = 0; currentPort < outPortSize; ++currentPort)
               {
                  structural_objectRef curr_port = GetPointer<module_o>(curr_gate)->get_out_port(currentPort);
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
                         HLSMgr->CGetFunctionBehavior(HLS->functionId)->CGetBehavioralHelper()->GetFunctionName() +
                         ": function pipelining may come for free");
   }

   const auto& CGM = HLSMgr->CGetCallGraphManager();
   if(CGM.GetRootFunctions().count(HLS->functionId) &&
      (CGM.ExistsAddressedFunction() ||
       HLSMgr->unused_interfaces.find(HLS->functionId) != HLSMgr->unused_interfaces.end()))
   {
      const auto addressed_functions = CGM.GetAddressedFunctions();
      auto constBitZero_obj = SM->add_constant(
          "constBitZero", circuit, circuit->find_member(CLOCK_PORT_NAME, port_o_K, circuit)->get_typeRef(), "0");

      for(const auto& f_id : addressed_functions)
      {
         const auto FUName = functions::GetFUName(ir_helper::GetFunctionName(IRM->GetIRNode(f_id)), HLSMgr);
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
            SM->add_connection(fu_start, constBitZero_obj);
         }

         for(const auto additional_parameter :
             HLSMgr->CGetFunctionBehavior(f_id)->CGetBehavioralHelper()->get_parameters())
         {
            const auto parameterName =
                HLSMgr->CGetFunctionBehavior(f_id)->CGetBehavioralHelper()->PrintVariable(additional_parameter);

            const auto parameterPort = FU->find_member(parameterName, port_o_K, FU);
            auto constZeroParam = SM->add_constant("zeroParam_" + FUName + "_" + parameterName, circuit,
                                                   parameterPort->get_typeRef(), "0");
            SM->add_connection(parameterPort, constZeroParam);
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
            const auto has_to_be_generated = GetPointer<module_o>(structManager_obj->get_circ())->has_to_be_generated();
            if(has_to_be_generated)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Unit has to be specialized.");
               const HDLGeneratorManagerRef modGen(new HDLGeneratorManager(HLSMgr, parameters));
               {
                  fu_name = fu_name + "_modgen";
               }
               const auto& specialized_fuName = fu_name;

               const auto check_lib = TechM->get_library(specialized_fuName);
               modGen->create_generic_module(FUName, gc_null_vertex(), FB, UPlibrary, specialized_fuName);
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

   for(const auto& [var, ding_dong_info] : ding_dong_candidates)
   {
      const auto channel_count = get_dataflow_memory_channel_count(circuit, HLS->functionId, var);
      const auto ding_dong_name = "ding_dong_array_" + STR(var);
      const auto ding_dong_type = channel_count > 1U ? "ARRAY_1D_STD_BRAM_NN_SDS_BASE" : "ARRAY_1D_STD_BRAM_SDS_BASE";
      const auto ding_dong_library = TechM->get_library(ding_dong_type);
      THROW_ASSERT(!ding_dong_library.empty(), std::string("Library miss component ") + ding_dong_type);
      INDENT_DBG_MEX(
          DEBUG_LEVEL_PEDANTIC, debug_level,
          "---Adding ding-dong buffer " + ding_dong_name + " of type " + ding_dong_type + " from library " +
              ding_dong_library + " bundles=[" +
              [&]() {
                 std::string s;
                 for(const auto& b : ding_dong_info.bundle_names)
                 {
                    if(!s.empty())
                    {
                       s += ",";
                    }
                    s += b;
                 }
                 return s;
              }() +
              "]");
      const auto ding_dong_buffer =
          SM->add_module_from_technology_library(ding_dong_name, ding_dong_type, ding_dong_library, circuit, TechM);
      const auto ding_dong_module = GetPointerS<module_o>(ding_dong_buffer);
      const auto size_ding_dong_port_vectors = [&](const auto port_getter, const unsigned int port_count) {
         for(unsigned int port_index = 0; port_index < port_count; ++port_index)
         {
            const auto port = port_getter(port_index);
            if(port->get_kind() == port_vector_o_K && GetPointerS<const port_o>(port)->get_ports_size() == 0)
            {
               GetPointerS<port_o>(port)->add_n_ports(channel_count, port);
            }
         }
      };
      size_ding_dong_port_vectors([&](const auto port_index) { return ding_dong_module->get_in_port(port_index); },
                                  ding_dong_module->get_in_port_size());
      size_ding_dong_port_vectors([&](const auto port_index) { return ding_dong_module->get_out_port(port_index); },
                                  ding_dong_module->get_out_port_size());
      // Validate that the instantiated component has the expected port topology before wiring.
      // Multi-channel components must expose vector ports for every data/address/control port;
      // single-channel components use scalar ports for the same roles.
      const auto validate_ding_dong_port = [&](const std::string& port_name, so_kind expected_kind,
                                               unsigned int ASSERT_PARAMETER(expected_count)) {
         const auto port = find_direct_module_port(ding_dong_buffer, port_name);
         THROW_ASSERT(port && port->get_kind() == expected_kind,
                      "Ding-dong buffer port " + ding_dong_buffer->get_path() + "/" + port_name +
                          " has unexpected kind (expected " +
                          (expected_kind == port_vector_o_K ? "port_vector_o_K" : "port_o_K") + ")");
         if(expected_kind == port_vector_o_K)
         {
#if HAVE_ASSERTS
            const auto actual_count = GetPointerS<const port_o>(port)->get_ports_size();
#endif
            THROW_ASSERT(actual_count == expected_count, "Ding-dong buffer port " + ding_dong_buffer->get_path() + "/" +
                                                             port_name + " has " + STR(actual_count) + " entries but " +
                                                             STR(expected_count) + " (channel_count) expected");
         }
      };
      for(const auto& pname : {"in1", "in2r", "in2w", "in3r", "in3w", "in4r", "in4w", "out1", "sel_LOAD", "sel_STORE"})
      {
         validate_ding_dong_port(pname, channel_count > 1U ? port_vector_o_K : port_o_K, channel_count);
      }
      const auto base_address = HLSMgr->Rmem->get_symbol(var, HLS->functionId)->get_symbol_name();
      const auto type_node = ir_helper::CGetType(IRM->GetIRNode(var));
      const auto ding_dong_element_bitsize = std::max(1ULL, ir_helper::AccessedMaximumBitsize(type_node, 1));
      const auto ding_dong_num_elements =
          ir_helper::IsArrayType(type_node) ? std::max(1ULL, ir_helper::GetArrayTotalSize(type_node)) : 1ULL;
      const auto ding_dong_write_bitsize = std::max(1ULL, get_dataflow_memory_port_bitsize(circuit, var, "d"));
      const auto ding_dong_read_bitsize =
          std::max(ding_dong_element_bitsize, get_dataflow_memory_port_bitsize(circuit, var, "q"));
      const auto ding_dong_address_bitsize =
          std::max(std::max(1ULL, get_dataflow_memory_port_bitsize(circuit, var, "address")),
                   std::max(1ULL, ceil_log2(std::max(2ULL, ding_dong_num_elements))));
      const auto ding_dong_size_bitsize = std::max(1ULL, HLSMgr->Rmem->get_bus_size_bitsize());
      HLSMgr->Rmem->set_sds_var(var, true, ding_dong_element_bitsize);
      ding_dong_buffer->SetParameter("address_space_begin", STR(base_address));
      // Dataflow array interfaces expose element indices in the SDS case. Keep the ding-dong address
      // space in the same domain so the instantiated primitive does not widen the address ports back to the global
      // byte-address bus width.
      ding_dong_buffer->SetParameter("address_space_rangesize", STR(ding_dong_num_elements));
      ding_dong_buffer->SetParameter("USE_SPARSE_MEMORY", is_sparse_memory ? "1" : "0");
      // InterfaceInfer array ports export element indices, not byte addresses. Keep the SDS base primitive from
      // stripping byte offsets a second time by forcing byte-granular addressing here.
      ding_dong_buffer->SetParameter("ALIGNMENT", "8");
      ding_dong_buffer->SetParameter("n_elements", STR(ding_dong_num_elements));
      ding_dong_buffer->SetParameter("data_size", STR(ding_dong_element_bitsize));
      ding_dong_buffer->SetParameter("PRIVATE_MEMORY", HLSMgr->Rmem->is_private_memory(var) ? "1" : "0");
      ding_dong_buffer->SetParameter("READ_ONLY_MEMORY", HLSMgr->Rmem->is_read_only_variable(var) ? "1" : "0");
      ding_dong_buffer->SetParameter("MEMORY_INIT_file", "\"\"\"\"");
      if(omp_info)
      {
         ding_dong_buffer->SetParameter("CONTEXT_COUNT", STR(omp_info->context_count));
         ding_dong_buffer->SetParameter("CONTEXT_PAGE_BITSIZE", STR(ceil_log2(omp_info->mem_page_size)));
      }
      resize_named_port(ding_dong_buffer, "in1", ding_dong_write_bitsize, debug_level);
      resize_named_port(ding_dong_buffer, "in2r", ding_dong_address_bitsize, debug_level);
      resize_named_port(ding_dong_buffer, "in2w", ding_dong_address_bitsize, debug_level);
      resize_named_port(ding_dong_buffer, "in3r", ding_dong_size_bitsize, debug_level);
      resize_named_port(ding_dong_buffer, "in3w", ding_dong_size_bitsize, debug_level);
      resize_named_port(ding_dong_buffer, "out1", ding_dong_read_bitsize, debug_level);
      resize_named_port(ding_dong_buffer, "proxy_in1", ding_dong_write_bitsize, debug_level);
      resize_named_port(ding_dong_buffer, "proxy_in2r", ding_dong_address_bitsize, debug_level);
      resize_named_port(ding_dong_buffer, "proxy_in2w", ding_dong_address_bitsize, debug_level);
      resize_named_port(ding_dong_buffer, "proxy_in3r", ding_dong_size_bitsize, debug_level);
      resize_named_port(ding_dong_buffer, "proxy_in3w", ding_dong_size_bitsize, debug_level);
      resize_named_port(ding_dong_buffer, "proxy_out1", ding_dong_read_bitsize, debug_level);
      memory::add_memory_parameter(HLS->datapath, base_address,
                                   STR(HLSMgr->Rmem->get_base_address(var, HLS->functionId)));
      check_parametrization(ding_dong_buffer);
      memory::propagate_memory_parameters(ding_dong_buffer, HLS->datapath);
      storage_by_var[var] = ding_dong_buffer;

      if(const auto clk_port = ding_dong_buffer->find_member(CLOCK_PORT_NAME, port_o_K, ding_dong_buffer))
      {
         SM->add_connection(clk_port, clock_port);
      }
      if(const auto rst_port = ding_dong_buffer->find_member(RESET_PORT_NAME, port_o_K, ding_dong_buffer))
      {
         SM->add_connection(rst_port, reset_port);
      }

      const auto data_size = ding_dong_buffer->GetParameter("data_size");
      connect_constant_to_port(SM, circuit, find_direct_module_port(ding_dong_buffer, "in3r"),
                               "ding_dong_in3r_" + STR(var), data_size);
      connect_constant_to_port(SM, circuit, find_direct_module_port(ding_dong_buffer, "in3w"),
                               "ding_dong_in3w_" + STR(var), data_size);
      connect_constant_to_port(SM, circuit, find_direct_module_port(ding_dong_buffer, "in4r"),
                               "ding_dong_in4r_" + STR(var), "1");
      connect_constant_to_port(SM, circuit, find_direct_module_port(ding_dong_buffer, "in4w"),
                               "ding_dong_in4w_" + STR(var), "1");

      for(const auto& zero_port_name :
          {"S_oe_ram", "S_we_ram", "S_addr_ram", "S_Wdata_ram", "Sin_Rdata_ram", "S_data_ram_size", "Sin_DataRdy",
           "proxy_in1", "proxy_in2r", "proxy_in2w", "proxy_in3r", "proxy_in3w", "proxy_in4r", "proxy_in4w",
           "proxy_sel_LOAD", "proxy_sel_STORE"})
      {
         connect_constant_to_port(SM, circuit, find_direct_module_port(ding_dong_buffer, zero_port_name),
                                  "ding_dong_" + std::string(zero_port_name) + "_" + STR(var), "0");
      }
   }
   connect_dataflow_memory_ports(HLSMgr, HLS->functionId, SM, circuit, storage_by_var, unique_id);
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
      const auto inPortSize = GetPointerS<module_o>(curr_gate)->get_in_port_size();
      for(auto currentPort = 0U; currentPort < inPortSize; ++currentPort)
      {
         const auto curr_port = GetPointerS<module_o>(curr_gate)->get_in_port(currentPort);
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
      const auto outPortSize = GetPointerS<module_o>(curr_gate)->get_out_port_size();
      for(auto currentPort = 0U; currentPort < outPortSize; ++currentPort)
      {
         const auto curr_port = GetPointerS<module_o>(curr_gate)->get_out_port(currentPort);
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
   const auto selector_port = circuit->find_member(SELECTOR_REGISTER_FILE, port_o_K, circuit);
   if(selector_port)
   {
      for(auto i = 0U; i < GetPointerS<module_o>(circuit)->get_internal_objects_size(); i++)
      {
         const auto m = GetPointerS<module_o>(circuit)->get_internal_object(i);
         const auto m_selector_port = m->find_member(SELECTOR_REGISTER_FILE, port_o_K, m);
         if(m_selector_port)
         {
            SM->add_connection(selector_port, m_selector_port);
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added functional units to circuit");
}

void fu_binding::check_parametrization(structural_objectRef
#if HAVE_ASSERTS
                                           curr_gate
#endif
)
{
#if HAVE_ASSERTS
   const auto fu_module = GetPointer<module_o>(curr_gate);
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
   for(unsigned int j = 0; j < GetPointer<module_o>(curr_gate)->get_in_port_size(); j++)
   {
      const auto port_in = GetPointer<module_o>(curr_gate)->get_in_port(j);
      manage_extern_global_port(HLSMgr, HLS, SM, port_in, port_o::IN, circuit, num);
      if(GetPointer<port_o>(port_in)->get_is_memory())
      {
         added_memory_element = true;
      }
   }
   /// creating extern OUT port on datapath starting from extern ports on module
   for(unsigned int j = 0; j < GetPointer<module_o>(curr_gate)->get_out_port_size(); j++)
   {
      structural_objectRef port_out = GetPointer<module_o>(curr_gate)->get_out_port(j);
      manage_extern_global_port(HLSMgr, HLS, SM, port_out, port_o::OUT, circuit, num);
   }
   /// creating extern IO port on datapath starting from extern ports on module
   for(unsigned int j = 0; j < GetPointer<module_o>(curr_gate)->get_in_out_port_size(); j++)
   {
      structural_objectRef port_in_out = GetPointer<module_o>(curr_gate)->get_in_out_port(j);
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
      for(unsigned int j = 0; j < GetPointer<module_o>(memory_module)->get_in_port_size(); j++)
      {
         structural_objectRef port_i = GetPointer<module_o>(memory_module)->get_in_port(j);
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
      for(unsigned int j = 0; j < GetPointer<module_o>(memory_module)->get_out_port_size(); j++)
      {
         structural_objectRef port_i = GetPointer<module_o>(memory_module)->get_out_port(j);
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
         const auto bm_in_port = GetPointerS<module_o>(bus_merger_mod)->get_in_port(0U);
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
               const auto js_in_port = GetPointerS<module_o>(js_mod)->get_in_port(0U);
               GetPointerS<port_o>(js_in_port)
                   ->add_n_ports(static_cast<unsigned int>(GetPointerS<port_o>(merged_port)->get_ports_size()),
                                 js_in_port);
               port_o::resize_std_port(STD_GET_SIZE((merged_port)->get_typeRef()), 0U, DEBUG_LEVEL_NONE, js_in_port);
               SM->add_connection(sign_v_in, merged_port);
               SM->add_connection(sign_v_in, js_in_port);
               const auto js_out_port = GetPointerS<module_o>(js_mod)->get_out_port(0U);
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
         out_port = GetPointerS<module_o>(bus_merger_mod)->get_out_port(0);
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
            const auto ss_in_port = GetPointerS<module_o>(ss_mod)->get_in_port(0U);
            port_o::resize_std_port(GetPointer<port_o>(bus_port)->get_ports_size() *
                                        STD_GET_SIZE(bus_port->get_typeRef()),
                                    0U, DEBUG_LEVEL_NONE, ss_in_port);
            SM->add_connection(sign_out, out_port);
            SM->add_connection(sign_out, ss_in_port);
            out_port = GetPointerS<module_o>(ss_mod)->get_out_port(0U);
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
      for(unsigned int j = 0; j < GetPointer<module_o>(memory_module)->get_in_port_size(); ++j)
      {
         structural_objectRef port_i = GetPointer<module_o>(memory_module)->get_in_port(j);
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

      for(unsigned int j = 0; j < GetPointer<module_o>(memory_module)->get_out_port_size(); j++)
      {
         structural_objectRef port_i = GetPointer<module_o>(memory_module)->get_out_port(j);
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
      const auto fsymbol = HLSMgr->CGetFunctionBehavior(HLS->functionId)->CGetBehavioralHelper()->GetFunctionName();
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

ir_nodeRef getFunctionType(ir_nodeRef exp);
void fu_binding::specialise_fu(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef fu_obj,
                               unsigned int fu, const OpVertexSet& mapped_operations, unsigned int ar)
{
   const auto FB = HLSMgr->CGetFunctionBehavior(HLS->functionId);
   const auto memory_allocation_policy = FB->GetMemoryAllocationPolicy();
   auto bus_data_bitsize = HLSMgr->Rmem->get_bus_data_bitsize();
   auto bus_size_bitsize = HLSMgr->Rmem->get_bus_size_bitsize();
   const auto bus_addr_bitsize = HLSMgr->get_address_bitsize();
   const auto bus_tag_bitsize = 0;
   auto* fu_module = GetPointer<module_o>(fu_obj);
   const auto fu_tech_obj = allocation_information->get_fu(fu);
   const auto fun_unit = GetPointerS<functional_unit>(fu_tech_obj);
   const auto& memory_ctrl_type = fun_unit->memory_ctrl_type;
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
      bool has_unaligned_mem_access = false;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Ar is true");
      {
         const auto type_node = ir_helper::CGetType(IRM->GetIRNode(ar));
         const auto elmt_bitsize = ir_helper::AccessedMaximumBitsize(type_node, 1);

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
         const auto data = FB->GetOpGraph(FunctionBehavior::CFG);
         for(const auto& mapped_operation : mapped_operations)
         {
            const auto& op_info = data.CGetNodeInfo(mapped_operation);
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                          "  on BRAM = " + op_info.GetOperation() + " " + op_info.vertex_name);
            const auto vars = HLSMgr->get_required_values(HLS->functionId, mapped_operation);
            const auto out_var = HLSMgr->get_produced_value(HLS->functionId, mapped_operation);
            if(op_info.node_type & TYPE_STORE)
            {
               THROW_ASSERT(std::get<0>(vars[0]), "Expected an IR node in case of a value to store");
               required_variables[0] =
                   std::max(required_variables[0], ir_helper::SizeAlloc(IRM->GetIRNode(std::get<0>(vars[0]))));
               if(ir_helper::is_a_misaligned_vector(IRM, std::get<0>(vars[0])))
               {
                  has_unaligned_mem_access = true;
               }
            }
            else if(op_info.node_type & TYPE_LOAD)
            {
               THROW_ASSERT(out_var, "Expected an IR node in case of a value to load");
               produced_variables = std::max(produced_variables, ir_helper::SizeAlloc(IRM->GetIRNode(out_var)));
               if(ir_helper::is_a_misaligned_vector(IRM, out_var))
               {
                  has_unaligned_mem_access = true;
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
            bram_bitsize = has_unaligned_mem_access ? std::max(bram_bitsize, accessed_bitsize) :
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
         const auto omp_mem_arch = FB->GetOMPInfo() != nullptr;
         const auto has_extern_mem =
             ((HLSMgr->Rmem->get_memory_address() - HLSMgr->base_address) > 0 &&
              memory_allocation_policy != MemoryAllocation_Policy::EXT_PIPELINED_BRAM) ||
             (HLSMgr->Rmem->has_unknown_addresses() && memory_allocation_policy != MemoryAllocation_Policy::ALL_BRAM &&
              memory_allocation_policy != MemoryAllocation_Policy::EXT_PIPELINED_BRAM);
         fu_module->SetParameter("BUS_PIPELINED", (omp_mem_arch || !has_extern_mem) ? "1" : "0");
      }
   }
   else
   {
      const auto data = FB->GetOpGraph(FunctionBehavior::CFG);

      for(const auto& mapped_operation : mapped_operations)
      {
         const auto& op_info = data.CGetNodeInfo(mapped_operation);
         const auto vars = HLSMgr->get_required_values(HLS->functionId, mapped_operation);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "---Considering operation " +
                            HLSMgr->get_ir_manager()->GetIRNode(op_info.GetNodeId())->ToString());
         const auto out_var = HLSMgr->get_produced_value(HLS->functionId, mapped_operation);

         if(memory_ctrl_type != "")
         {
            unsigned long long mem_var_size_in = 1;
            unsigned long long mem_var_size_out = 1;

            if(op_info.node_type & TYPE_STORE)
            {
               THROW_ASSERT(std::get<0>(vars[0]), "Expected an IR node in case of a value to store");
               mem_var_size_in = std::max(mem_var_size_in, ir_helper::SizeAlloc(IRM->GetIRNode(std::get<0>(vars[0]))));
            }
            else if(op_info.node_type & TYPE_LOAD)
            {
               THROW_ASSERT(out_var, "Expected an IR node in case of a value to load");
               mem_var_size_out = std::max(mem_var_size_out, ir_helper::SizeAlloc(IRM->GetIRNode(out_var)));
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
            const auto op_name = op_info.GetOperation();
            for(auto i = 0U; i < vars.size(); ++i)
            {
               const auto& ir_var = std::get<0>(vars[i]);
               if(ir_var == 0)
               {
                  continue;
               }
               required_variables.insert(std::make_pair(i, 0));
               const auto var_node = IRM->GetIRNode(ir_var);
               if(ir_helper::IsVectorType(var_node))
               {
                  const auto type = ir_helper::CGetType(var_node);
                  const auto size = ir_helper::SizeAlloc(type);
                  const auto element_type = ir_helper::CGetElements(type);
                  const auto element_size = ir_helper::SizeAlloc(element_type);
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
                  auto bitsize = ir_helper::Size(var_node);
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
                  if(*it == "LSB_PARAMETER" && op_name == "gep_node")
                  {
                     auto op0 = IRM->GetIRNode(std::get<0>(vars[0]));
                     auto op1 = IRM->GetIRNode(std::get<0>(vars[1]));
                     unsigned int curr_LSB = 0;
                     const auto var = ir_helper::GetBaseVariable(op0);
                     if(var && FB->is_variable_mem(var->index) && HLSMgr->Rmem->is_sds_var(var->index))
                     {
                        const auto type = ir_helper::CGetType(var);
                        const auto value_bitsize = HLSMgr->Rmem->get_sds_var_size(var->index);
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
                        else if(value_bitsize == 512)
                        {
                           curr_LSB = 6;
                        }
                        else if(value_bitsize == 1024)
                        {
                           curr_LSB = 7;
                        }
                        else
                        {
                           curr_LSB = 0;
                        }
                     }
                     if(op0->get_kind() == ssa_node_K)
                     {
                        auto ssa_var0 = GetPointer<ssa_node>(op0);
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
                     if(op1->get_kind() == ssa_node_K)
                     {
                        const auto* ssa_var1 = GetPointerS<const ssa_node>(op1);
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
                     else if(op1->get_kind() == constant_int_val_node_K)
                     {
                        const auto offset_value = ir_helper::GetConstValue(op1);
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
                  if(*it == "OFFSET_PARAMETER" && op_name == "concat_bit_node")
                  {
                     auto index = op_info.GetNodeId();
                     const auto ga_node = IRM->GetIRNode(index);
                     const auto ga = GetPointer<assign_stmt>(ga_node);
                     const auto ce = GetPointer<concat_bit_node>(ga->op1);
                     const auto offset_value = ir_helper::GetConstValue(ce->op2);
                     fu_module->SetParameter("OFFSET_PARAMETER", STR(offset_value));
                  }
                  if(*it == "unlock_address" && op_name == BUILTIN_WAIT_CALL)
                  {
                     auto index = op_info.GetNodeId();
                     std::string parameterName = HLSMgr->Rmem->get_symbol(index, HLS->functionId)->get_symbol_name();
                     fu_module->SetParameter("unlock_address", parameterName);
                  }
                  if(*it == "MEMORY_INIT_file" && op_name == BUILTIN_WAIT_CALL)
                  {
                     auto index = op_info.GetNodeId();
                     const auto parameterAddressFileName = "function_addresses_" + STR(index) + ".mem";
                     const auto output_directory =
                         HLSMgr->get_parameter()->getOption<std::filesystem::path>(OPT_output_directory);
                     std::filesystem::create_directories(output_directory);
                     std::ofstream parameterAddressFile(output_directory / parameterAddressFileName);

                     const auto call = IRM->GetIRNode(index);
                     const auto& calledFunction = GetPointerS<const call_stmt>(call)->args[0];
                     const auto& hasreturn_node = GetPointerS<const call_stmt>(call)->args[1];
                     const auto hasreturn_value = ir_helper::GetConstValue(hasreturn_node);
                     const auto addrExpr = calledFunction;
                     const auto functionType = getFunctionType(addrExpr);
                     const auto alignment = HLSMgr->Rmem->get_parameter_alignment();
                     unsigned long long int address = 0;
                     address = HLSMgr->Rmem->compute_next_base_address(address, index, alignment);
                     auto paramList = GetPointerS<const function_ty_node>(functionType)->list_of_args_type;
                     for(const auto& p : paramList)
                     {
                        if(p->get_kind() != void_ty_node_K)
                        {
                           const auto str_address = convert_to_binary(address, HLSMgr->get_address_bitsize());
                           parameterAddressFile << str_address << "\n";
                           address = HLSMgr->Rmem->compute_next_base_address(address, p->index, alignment);
                        }
                     }
                     const auto return_type = GetPointerS<const function_ty_node>(functionType)->retn;
                     if(return_type && return_type->get_kind() != void_ty_node_K && hasreturn_value)
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
               const auto out_node = IRM->GetIRNode(out_var);
               if(ir_helper::IsVectorType(out_node))
               {
                  const auto type = ir_helper::CGetType(out_node);
                  const auto size = ir_helper::SizeAlloc(type);
                  const auto element_type = ir_helper::CGetElements(type);
                  const auto element_size = ir_helper::SizeAlloc(element_type);
                  n_out_elements = size / element_size;
                  produced_variables = element_size;
               }
               else
               {
                  produced_variables = std::max(produced_variables, ir_helper::Size(out_node));
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
                        auto sizetype = ir_helper::Size(ir_helper::CGetType(out_node));
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

      if(memory_ctrl_type != "" && fu_module->ExistsParameter("USE_BACK_PRESSURE"))
      {
         const bool is_openmp = parameters->isOption(OPT_openmp) && parameters->getOption<bool>(OPT_openmp);
         if(is_openmp)
         {
            fu_module->SetParameter("USE_BACK_PRESSURE", STR(1));
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "---Activated back_pressure in  " + fu_module->get_path());
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
                                        bool is_sparse_memory, bool is_sds, OMPInfoRef omp_info)
{
   const auto fu_module = GetPointer<module_o>(fu_obj);
   /// base address specialization
   fu_module->SetParameter("address_space_begin", STR(base_address));
   fu_module->SetParameter("address_space_rangesize", STR(rangesize));
   fu_module->SetParameter("USE_SPARSE_MEMORY", is_sparse_memory ? "1" : "0");
   unsigned int context_count = 1U;
   if(omp_info)
   {
      context_count = omp_info->context_count;
      fu_module->SetParameter("CONTEXT_COUNT", STR(context_count));
      fu_module->SetParameter("CONTEXT_PAGE_BITSIZE", STR(ceil_log2(omp_info->mem_page_size)));
   }
   memory::add_memory_parameter(HLS->datapath, base_address, STR(HLSMgr->Rmem->get_base_address(ar, HLS->functionId)));

   /// array ref initialization
   THROW_ASSERT(ar, "expected a real IR node index");
   const auto init_filename = "array_" + STR(ar) + ".mem";
   const auto output_directory = HLSMgr->get_parameter()->getOption<std::filesystem::path>(OPT_output_directory);
   std::filesystem::create_directories(output_directory);
   std::ofstream init_file_a(output_directory / init_filename);
   std::ofstream init_file_b;
   if(is_memory_splitted)
   {
      init_file_b.open(output_directory / ("0_" + init_filename));
   }
   unsigned long long vec_size = 0, elts_size = 0;
   const auto bitsize_align = is_sds ? 0ULL : std::stoull(fu_module->GetParameter("BRAM_BITSIZE"));
   for(unsigned int i = 0; i < context_count; i++)
   {
      fill_array_memory(init_file_a, init_file_b, ar, vec_size, elts_size, HLSMgr->Rmem, IRM, is_sds, bitsize_align);
   }
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

void fu_binding::fill_array_memory(std::ostream& init_file_a, std::ostream& init_file_b, unsigned int ar,
                                   unsigned long long& vec_size, unsigned long long& elts_size,
                                   const std::unique_ptr<memory>& mem, ir_managerConstRef TM, bool is_sds,
                                   unsigned long long bitsize_align)
{
   init_file_b.put(0);
   const auto is_memory_splitted = init_file_b.good();
   init_file_b.seekp(std::ios_base::cur - 1);

   const auto ar_node = TM->GetIRNode(ar);
   ir_nodeRef init_node;
   const auto vd = GetPointer<const variable_val_node>(ar_node);
   if(vd && vd->init)
   {
      init_node = vd->init;
   }
   const auto array_type_node = ir_helper::CGetType(ar_node);
   unsigned long long element_align = 0;
   if(is_sds)
   {
      elts_size = mem->get_sds_var_size(ar);
      auto alloc_size = ir_helper::SizeAlloc(array_type_node);
      vec_size = alloc_size / elts_size + (alloc_size % elts_size != 0 ? 1 : 0);
   }
   else if(ir_helper::IsArrayEquivType(array_type_node))
   {
      std::vector<unsigned long long> dims;
      ir_helper::get_array_dim_and_bitsize(TM, array_type_node->index, dims, elts_size);
      THROW_ASSERT(dims.size(), "something wrong happened");
      vec_size = std::accumulate(dims.begin(), dims.end(), 1ULL,
                                 [](unsigned long long a, unsigned long long b) { return a * b; });
   }
   else if(GetPointer<const integer_ty_node>(array_type_node) || ir_helper::IsRealType(array_type_node) ||
           ir_helper::IsPointerType(array_type_node) || ir_helper::IsStructType(array_type_node))
   {
      elts_size = ir_helper::SizeAlloc(array_type_node);
      vec_size = 1;
   }
   else if(ir_helper::IsVectorType(array_type_node))
   {
      elts_size = ir_helper::SizeAlloc(array_type_node);
      const auto element_type = ir_helper::CGetElements(array_type_node);
      element_align = ir_helper::SizeAlloc(element_type);
      vec_size = 1;
   }
   else
   {
      THROW_ERROR("Type not supported: " + array_type_node->get_kind_text());
   }
   THROW_ASSERT(elts_size && vec_size, "elts_size=" + STR(elts_size) + " vec_size=" + STR(vec_size));
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
      ((GetPointer<constructor_node>(init_node) && GetPointerS<constructor_node>(init_node)->list_of_idx_valu.size()) ||
       (!GetPointer<constructor_node>(init_node))))
   {
      std::vector<std::string> init_string;
      write_init(TM, ar_node, init_node, init_string, mem, element_align);
      if(is_sds && (element_align == 0 || elts_size == element_align))
      {
         THROW_ASSERT(!is_memory_splitted, "unexpected condition");
         THROW_ASSERT(elts_size, "unexpected condition");
         unsigned int row = 0;
         std::string current_string;
         for(const auto& init_value : init_string)
         {
            ++row;
            auto init_value_size = init_value.size();
            if(elts_size != init_value_size && (init_value_size % elts_size == 0))
            {
               const auto n_elmts = init_value_size / elts_size;
               for(auto index = 0u; index < n_elmts; ++index)
               {
                  init_file_a << init_value.substr(init_value_size - elts_size - index * elts_size, elts_size)
                              << std::endl;
               }
            }
            else if(elts_size > init_value_size && (elts_size % init_value_size == 0))
            {
               const auto n_elmts = elts_size / init_value_size;
               current_string = init_value + current_string;
               if(row % n_elmts == 0)
               {
                  init_file_a << current_string << std::endl;
                  current_string.clear();
               }
            }
            else
            {
               THROW_ASSERT(elts_size == init_value_size, "unexpected condition");
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
         if((ir_helper::SizeAlloc(array_type_node) / 8U) > eightbit_string.size())
         {
            size_t tail_bytes = (ir_helper::SizeAlloc(array_type_node) / 8U) - eightbit_string.size();
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

void fu_binding::write_init(const ir_managerConstRef IRM, ir_nodeRef var_node, ir_nodeRef _init_node,
                            std::vector<std::string>& init_file, const std::unique_ptr<memory>& mem,
                            unsigned long long element_align)
{
   std::string trimmed_value;
   const auto init_node = _init_node;
   switch(init_node->get_kind())
   {
      case constant_fp_val_node_K:
      {
         auto precision = ir_helper::SizeAlloc(_init_node);
         const auto rc = GetPointerS<const constant_fp_val_node>(init_node);
         std::string C_value = rc->valr;
         trimmed_value = convert_fp_to_string(C_value, precision);
         init_file.push_back(trimmed_value);
         break;
      }
      case constant_int_val_node_K:
      {
         const auto ull_value = ir_helper::GetConstValue(init_node);
         trimmed_value = "";
         auto precision = ir_helper::SizeAlloc(_init_node);
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
      case constructor_node_K:
      {
         const auto co = GetPointerS<const constructor_node>(init_node);
         bool designated_initializers_used = false;
         bool is_struct = false;
         std::vector<ir_nodeRef>* field_list = nullptr;
         /// check if designated initializers are really used
         if(co->list_of_idx_valu.size() && co->list_of_idx_valu.front().first->get_kind() == field_val_node_K)
         {
            auto iv_it = co->list_of_idx_valu.begin();
            const auto iv_end = co->list_of_idx_valu.end();
            const auto parent = GetPointerS<field_val_node>(iv_it->first)->parent;

            field_list = &GetPointerS<struct_ty_node>(parent)->list_of_flds;
            is_struct = true;
            auto fl_it = field_list->begin();
            const auto fl_end = field_list->end();
            for(; fl_it != fl_end && iv_it != iv_end; ++iv_it, ++fl_it)
            {
               if(iv_it->first && iv_it->first->index != (*fl_it)->index)
               {
                  break;
               }
            }
            if(fl_it != fl_end)
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
               if(!GetPointer<field_val_node>(*fli))
               {
                  continue;
               }
               const auto is_bitfield = GetPointer<field_val_node>(*fli)->bitfield;
               auto inext = fli;
               ++inext;
               while(inext != flend && !GetPointer<field_val_node>(*inext))
               {
                  ++inext;
               }

               if(is_bitfield)
               {
                  // fix the element precision to pass to write_init
                  element_align = ir_helper::SizeAlloc(*fli);
               }

               if(iv_it != iv_end && iv_it->first->index == (*fli)->index)
               {
                  write_init(IRM, iv_it->first, iv_it->second, init_file, mem, element_align);
                  ++iv_it;
               }
               else
               {
                  write_init(IRM, *fli, *fli, init_file, mem, element_align);
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
                     const auto idx_next_fd = GetPointerS<field_val_node>(*inext);
                     nbits = static_cast<unsigned long long int>(idx_next_fd->offset);
                  }
                  else
                  {
                     nbits = ir_helper::SizeAlloc(co->type);
                  }
                  const auto idx_curr_fd = GetPointer<field_val_node>(*fli);
                  const auto field_decl_size = ir_helper::SizeAlloc(*fli);
                  THROW_ASSERT(nbits >= (idx_curr_fd->offset + field_decl_size), "");
                  nbits -= static_cast<unsigned long long int>(idx_curr_fd->offset) + field_decl_size;
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
               THROW_ASSERT(!is_struct || GetPointer<field_val_node>(iv_it->first), "unexpected condition");
               if(is_struct && GetPointer<field_val_node>(iv_it->first)->bitsizealloc == 0)
               {
                  // skip null field decl
                  continue;
               }
               const auto is_bitfield = is_struct && GetPointer<field_val_node>(iv_it->first)->bitfield;
               auto iv_next = iv_it;
               ++iv_next;
               while(iv_next != iv_end && is_struct && !GetPointer<field_val_node>(iv_next->first))
               {
                  ++iv_next;
               }
               if(is_struct && is_bitfield)
               {
                  // fix the element precision to pass to write_init
                  element_align = ir_helper::SizeAlloc(iv_it->first);
               }
               write_init(IRM, iv_it->first, iv_it->second, init_file, mem, element_align);
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
                     const auto idx_next_fd = GetPointerS<field_val_node>(iv_next->first);
                     nbits = static_cast<unsigned long long int>(idx_next_fd->offset);
                  }
                  else
                  {
                     nbits = ir_helper::SizeAlloc(co->type);
                  }
                  const auto field_decl_size = ir_helper::SizeAlloc(iv_it->first);
                  const auto idx_curr_fd = GetPointerS<field_val_node>(iv_it->first);
                  THROW_ASSERT(nbits >= (idx_curr_fd->offset + field_decl_size), "");
                  nbits -= static_cast<unsigned long long int>(idx_curr_fd->offset) + field_decl_size;
                  if(nbits)
                  {
                     /// add padding
                     init_file.push_back(std::string(nbits, '0'));
                  }
               }
            }
         }
         const auto type_n = ir_helper::CGetType(var_node);
         if(GetPointer<const array_ty_node>(type_n))
         {
            unsigned long long size_of_data;
            std::vector<unsigned long long> dims;
            ir_helper::get_array_dim_and_bitsize(IRM, type_n->index, dims, size_of_data);
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
      case bitcast_node_K:
      case nop_node_K:
      {
         const auto ue = GetPointerS<unary_node>(init_node);
         if(GetPointer<addr_node>(ue->op))
         {
            write_init(IRM, ue->op, ue->op, init_file, mem, element_align);
         }
         else if(GetPointer<constant_int_val_node>(ue->op))
         {
            const auto precision = std::max(std::max(8ull, element_align), ir_helper::SizeAlloc(init_node));
            write_init(IRM, ue->op, ue->op, init_file, mem, precision);
         }
         else
         {
            THROW_ERROR("Something unexpected happened: " + STR(init_node->index) + " | " + ue->op->get_kind_text());
         }
         break;
      }
      case addr_node_K:
      {
         auto* ae = GetPointerS<addr_node>(init_node);
         ir_nodeRef addr_node_op = ae->op;
         auto addr_node_op_idx = ae->op->index;
         unsigned long long int ull_value = 0;
         const auto precision = ir_helper::SizeAlloc(_init_node);
         switch(addr_node_op->get_kind())
         {
            case ssa_node_K:
            case variable_val_node_K:
            case argument_val_node_K:
            {
               THROW_ASSERT(mem->has_base_address(addr_node_op_idx), "missing base address for: " + ae->ToString());
               ull_value = mem->get_base_address(addr_node_op_idx, 0);
               break;
            }
            case function_val_node_K:
            {
               THROW_ASSERT(mem->has_base_address(addr_node_op_idx), "missing base address for: " + ae->ToString());
               ull_value = mem->get_base_address(addr_node_op_idx, addr_node_op_idx);
               break;
            }
            case CASE_UNARY_NODES:
            {
               if(addr_node_op->get_kind() == mem_access_node_K)
               {
                  const auto mr = GetPointerS<mem_access_node>(addr_node_op);
                  if(mr->op->get_kind() == variable_val_node_K)
                  {
                     ull_value = mem->get_base_address(mr->op->index, 0);
                  }
                  else if(mr->op->get_kind() == addr_node_K)
                  {
                     const auto base = GetPointerS<addr_node>(mr->op)->op;
                     if(base->get_kind() == variable_val_node_K)
                     {
                        ull_value = mem->get_base_address(base->index, 0);
                     }
                     else
                     {
                        THROW_ERROR("addr_node pattern not supported: " + std::string(addr_node_op->get_kind_text()) +
                                    " @" + STR(addr_node_op_idx));
                     }
                  }
                  else
                  {
                     THROW_ERROR("addr_node pattern not supported: " + std::string(addr_node_op->get_kind_text()) +
                                 " @" + STR(addr_node_op_idx));
                  }
               }
               else
               {
                  THROW_ERROR("addr_node pattern not supported: " + std::string(addr_node_op->get_kind_text()) + " @" +
                              STR(addr_node_op_idx));
               }
               break;
            }
            case CASE_BINARY_NODES:
            case call_node_K:
            case select_node_K:
            case constructor_node_K:
            case ternary_add_node_K:
            case ternary_as_node_K:
            case ternary_sa_node_K:
            case ternary_ss_node_K:
            case fshl_node_K:
            case fshr_node_K:
            case insertvalue_node_K:
            case insertelement_node_K:
            case concat_bit_node_K:
            case field_val_node_K:
            case identifier_node_K:
            case constant_int_val_node_K:
            case constant_fp_val_node_K:
            case statement_list_node_K:
            case module_unit_node_K:
            case shufflevector_node_K:
            case constant_vector_val_node_K:
            case lut_node_K:
            case CASE_FAKE_NODES:
            case CASE_NODE_STMTS:
            case CASE_TYPE_NODES:
            default:
               THROW_ERROR("addr_node pattern not supported: " + std::string(addr_node_op->get_kind_text()) + " @" +
                           STR(addr_node_op_idx));
         }
         for(unsigned int ind = 0; ind < precision; ind++)
         {
            trimmed_value = trimmed_value + (((1LLU << (precision - ind - 1)) & ull_value) ? '1' : '0');
         }
         init_file.push_back(trimmed_value);

         break;
      }
      case field_val_node_K:
      {
         const auto field_decl_size = ir_helper::SizeAlloc(_init_node);
         if(field_decl_size)
         {
            init_file.push_back(std::string(field_decl_size, '0'));
         }
         break;
      }
      case constant_vector_val_node_K:
      {
         const auto vc = GetPointerS<constant_vector_val_node>(init_node);
         for(const auto& i : vc->list_of_valu) // vector elements
         {
            write_init(IRM, i, i, init_file, mem, element_align);
         }
         break;
      }
      case mem_access_node_K:
      case call_node_K:
      case identifier_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case function_val_node_K:
      case argument_val_node_K:
      case module_unit_node_K:
      case variable_val_node_K:
      case abs_node_K:
      case not_node_K:
      case fptoi_node_K:
      case itofp_node_K:
      case unaligned_mem_access_node_K:
      case neg_node_K:
      case lut_node_K:
      case CASE_BINARY_NODES:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TERNARY_NODES:
      case CASE_TYPE_NODES:
      default:
         THROW_ERROR("elements not yet supported: " + init_node->get_kind_text() + init_node->ToString() +
                     (var_node ? STR(var_node) : ""));
   }
}

ir_nodeRef getFunctionType(ir_nodeRef exp)
{
   THROW_ASSERT(GetPointer<addr_node>(exp) || GetPointer<ssa_node>(exp), "Input must be a ssa_node or an addr_node");
   auto* sa = GetPointer<ssa_node>(exp);
   if(sa)
   {
      THROW_ASSERT(sa, "Function pointer not in SSA-form");
      pointer_ty_node* pt;
      if(sa->var)
      {
         auto* var = GetPointer<decl_node>(sa->var);
         THROW_ASSERT(var, "Call expression does not point to a declaration node");
         pt = GetPointer<pointer_ty_node>(var->type);
      }
      else
      {
         pt = GetPointer<pointer_ty_node>(sa->type);
      }

      THROW_ASSERT(pt, "Declaration node has not information about pointer_ty_node");
      THROW_ASSERT(GetPointer<function_ty_node>(pt->ptd),
                   "Pointer type has not information about pointed function_ty_node");

      return pt->ptd;
   }

   auto* AE = GetPointer<addr_node>(exp);
   auto* FD = GetPointer<function_val_node>(AE->op);
   return FD->type;
}

void fu_binding::set_ports_are_swapped(OpGraph::vertex_descriptor v, bool condition)
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

generic_objRef fu_binding::get(OpGraph::vertex_descriptor v) const
{
   const auto statement_index = op_graph.CGetNodeInfo(v).GetNodeId();
   return op_binding.at(statement_index);
}

structural_objectRef fu_binding::add_smart_signal(const structural_objectRef src_port, unsigned int _unique_id,
                                                  const structural_objectRef& circuit, const structural_managerRef& SM)
{
   structural_objectRef sig;
   if(src_port->get_kind() == port_o_K)
   {
      sig = SM->add_sign("sig_loc_" + src_port->get_id() + "_" + STR(_unique_id), circuit, src_port->get_typeRef());
   }
   else
   {
      sig = SM->add_sign_vector("sig_loc_vector_" + src_port->get_id() + "_" + STR(_unique_id),
                                GetPointerS<port_o>(src_port)->get_ports_size(), circuit, src_port->get_typeRef());
   }
   return sig;
}

void fu_binding::add_smart_connection(const structural_objectRef src_port, const structural_objectRef dst_port,
                                      unsigned int _unique_id, const structural_objectRef& circuit,
                                      const structural_managerRef& SM)
{
   if(src_port->get_owner() != circuit && dst_port->get_owner() != circuit)
   {
      const auto src_signal = GetPointerS<port_o>(src_port)->get_connected_signal();
      const auto dst_signal = GetPointerS<port_o>(dst_port)->get_connected_signal();
      if(src_signal)
      {
         THROW_ASSERT(!dst_signal || dst_signal == src_signal,
                      "The destination port " + dst_port->get_path() + " is already connected to another signal");
         if(!dst_signal)
         {
            SM->add_connection(src_signal, dst_port);
         }
      }
      else if(dst_signal)
      {
         THROW_ASSERT(!src_signal || src_signal == dst_signal,
                      "The source port " + src_port->get_path() + " is already connected to another signal");
         SM->add_connection(dst_signal, src_port);
      }
      else
      {
         structural_objectRef sig = add_smart_signal(src_port, _unique_id, circuit, SM);
         SM->add_connection(sig, src_port);
         SM->add_connection(sig, dst_port);
      }
   }
   else
   {
      SM->add_connection(src_port, dst_port);
   }
}

void fu_binding::add_smart_port(structural_objectRef& new_port, const structural_objectRef old_port, unsigned int size,
                                const std::string pname, port_o::port_direction direction,
                                const structural_objectRef& circuit, const structural_managerRef& SM)
{
   if(size == 1U)
   {
      new_port = SM->add_port(pname, direction, circuit, old_port->get_typeRef());
   }
   else
   {
      new_port = SM->add_port_vector(pname, direction, size, circuit, old_port->get_typeRef());
   }
   port_o::fix_port_properties(old_port, new_port);
}

bool fu_binding::try_find_port(const std::string& port_name, const structural_objectRef& id_module, so_kind port_type,
                               structural_objectRef& port_obj)
{
   bool result = GetPointer<module_o>(id_module)->has_port(port_name);
   if(result)
   {
      if(port_type == port_o_K)
      {
         port_obj = GetPointer<module_o>(id_module)->find_member(port_name, port_o_K, id_module);
      }
      else
      {
         port_obj = GetPointer<module_o>(id_module)->find_member(port_name, port_vector_o_K, id_module);
      }
   }
   return result;
}
