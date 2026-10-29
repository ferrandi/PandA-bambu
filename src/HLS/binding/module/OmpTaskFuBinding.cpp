/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2025-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file OmpTaskFuBinding.cpp
 * @brief
 *
 *
 * @author Kaixi Matteo Chen  <kaiximatteo.chen@mail.polimi.it>
 * @author Giovanni Gozzi <giovanni.gozzi@polimi.it>
 *
 */

#include <boost/algorithm/string/replace.hpp>
#include <pugixml.hpp>

#include "BambuParameterRegistry.hpp"
#include "OmpTaskFuBinding.hpp"
#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "cg_graph.hpp"
#include "copyrights_strings.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"
#include "interface/module_interface.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "ir_reindex.hpp"
#include "language_writer.hpp"
#include "math_function.hpp"
#include "memory.hpp"
#include "refcount.hpp"
#include "string_manipulation.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"

#include "openmp_task_constants.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

PANDA_REGISTER_PARAMETER("enable-task", PandaParamType::Bool, "0", "Enable OpenMP task graph extraction", "openmp");

PANDA_REGISTER_PARAMETER("task-resource-duplication", PandaParamType::UInt, "1",
                         "Number of instances of each task resource", "openmp");

const OmpTaskFuBinding::task_names OmpTaskFuBinding::name_dict = {
    "Arbiter_connection_interface", "Simple_arbiter_connection_interface", "Banked_memory_connection_interface",
    "task_entry", "duplicated"};

const unsigned OmpTaskFuBinding::ptr_size = 32U;

const std::string OmpTaskFuBinding::lambda_module_param_port_prefix = "Pd";

const int OmpTaskFuBinding::not_overwrite = -1;

const std::string OmpTaskFuBinding::ptr_prefix = "PTR_";

const std::string OmpTaskFuBinding::const_int32_prefix = "COMPILATION_TIME_CONST_INT32_";

const std::vector<std::tuple<std::string, so_kind>> OmpTaskFuBinding::ports_to_internalize = {
    {"Mout_oe_ram", port_o_K},
    {"Mout_we_ram", port_o_K},
    {"Mout_addr_ram", port_o_K},
    {"Mout_Wdata_ram", port_o_K},
    {"Mout_data_ram_size", port_o_K},
    {"Mout_tag", port_o_K},
    {"M_Rdata_ram", port_o_K},
    {"M_DataRdy", port_o_K},
    {"M_tag", port_o_K},
    {"Min_oe_ram", port_o_K},
    {"Min_we_ram", port_o_K},
    {"Min_addr_ram", port_o_K},
    {"Min_Wdata_ram", port_o_K},
    {"Min_data_ram_size", port_o_K},
    {"Mout_back_pressure", port_o_K},
    {"selector_register_file", port_o_K}};

const std::vector<std::tuple<std::string, so_kind>> OmpTaskFuBinding::mem_in_ports = {
    {"Min_oe_ram", port_o_K},    {"Min_we_ram", port_o_K},        {"Min_addr_ram", port_o_K},
    {"Min_Wdata_ram", port_o_K}, {"Min_data_ram_size", port_o_K},
};

const std::vector<std::tuple<std::string, so_kind, port_o::port_direction>> OmpTaskFuBinding::m_ports = {
    {"Mout_oe_ram", port_o_K, port_o::OUT},        {"Mout_we_ram", port_o_K, port_o::OUT},
    {"Mout_addr_ram", port_o_K, port_o::OUT},      {"Mout_Wdata_ram", port_o_K, port_o::OUT},
    {"Mout_data_ram_size", port_o_K, port_o::OUT}, {"M_Rdata_ram", port_o_K, port_o::IN},
    {"M_DataRdy", port_o_K, port_o::IN},           {"Mout_back_pressure", port_o_K, port_o::IN}};

const std::vector<std::tuple<std::string, so_kind, port_o::port_direction>> OmpTaskFuBinding::start_done_ports = {
    {START_PORT_NAME, port_o_K, port_o::IN},
    {DONE_PORT_NAME, port_o_K, port_o::OUT}};

const std::vector<std::tuple<std::string, so_kind, port_o::port_direction>> OmpTaskFuBinding::em_ports = {
    {"EM_in_activation_condition_done", port_vector_o_K, port_o::IN},
    {"EM_out_request", port_o_K, port_o::OUT},
    {"EM_out_done", port_vector_o_K, port_o::OUT},
    {"EM_in_RM_done", port_o_K, port_o::IN},
    {"EM_in_base_addr", port_vector_o_K, port_o::IN},
    {"EM_out_param_addr", port_vector_o_K, port_o::OUT}};

const std::vector<std::tuple<std::string, so_kind, port_o::port_direction>> OmpTaskFuBinding::rm_ports = {
    {"RM_in_FU_done", port_vector_o_K, port_o::IN}, {"RM_out_FU_request", port_vector_o_K, port_o::OUT},
    {"RM_in_request", port_vector_o_K, port_o::IN}, {"RM_out_done", port_vector_o_K, port_o::OUT},
    {"RM_in_param", port_vector_o_K, port_o::IN},   {"RM_out_param", port_vector_o_K, port_o::OUT}};

const std::vector<OmpTaskFuBinding::task_connection> OmpTaskFuBinding::mem_arbitrer_totask_graph_ports = {
    {{"Mout_oe_ram_out", port_vector_o_K}, {"Mout_oe_ram", port_o_K}},
    {{"Mout_we_ram_out", port_vector_o_K}, {"Mout_we_ram", port_o_K}},
    {{"Mout_addr_ram_out", port_vector_o_K}, {"Mout_addr_ram", port_o_K}},
    {{"Mout_Wdata_ram_out", port_vector_o_K}, {"Mout_Wdata_ram", port_o_K}},
    {{"Mout_data_ram_size_out", port_vector_o_K}, {"Mout_data_ram_size", port_o_K}},
    {{"Mout_tag_out", port_vector_o_K}, {"Mout_tag", port_o_K}},
    {{"Mout_back_pressure_out", port_vector_o_K}, {"Mout_back_pressure", port_o_K}},
    {{"M_DataRdy", port_vector_o_K}, {"M_DataRdy", port_o_K}},
    {{"M_tag", port_vector_o_K}, {"M_tag", port_o_K}},
    {{"M_Rdata_ram", port_vector_o_K}, {"M_Rdata_ram", port_o_K}}};

const std::vector<OmpTaskFuBinding::task_connection> OmpTaskFuBinding::mem_arbitrer_to_lambda_modules = {
    {{"Mout_oe_ram", port_vector_o_K}, {"Mout_oe_ram", port_o_K}},
    {{"Mout_we_ram", port_vector_o_K}, {"Mout_we_ram", port_o_K}},
    {{"Mout_addr_ram", port_vector_o_K}, {"Mout_addr_ram", port_o_K}},
    {{"Mout_Wdata_ram", port_vector_o_K}, {"Mout_Wdata_ram", port_o_K}},
    {{"Mout_data_ram_size", port_vector_o_K}, {"Mout_data_ram_size", port_o_K}},
    {{"Mout_tag", port_vector_o_K}, {"Mout_tag", port_o_K}},
    {{"Mout_back_pressure", port_vector_o_K}, {"Mout_back_pressure", port_o_K}},
    {{"M_DataRdy_out", port_vector_o_K}, {"M_DataRdy", port_o_K}},
    {{"M_Rdata_ram_out", port_vector_o_K}, {"M_Rdata_ram", port_o_K}},
    {{"M_tag_out", port_vector_o_K}, {"M_tag", port_o_K}}};

const int OmpTaskFuBinding::UNKNOWN_NUM_SIZE = -1;

const std::vector<std::pair<int, OmpTaskFuBinding::task_connection>>
    OmpTaskFuBinding::resource_manager_to_lambda_modules = {
        {1U, {{"RM_out_FU_request", port_vector_o_K}, {"start_port", port_o_K}}},
        {UNKNOWN_NUM_SIZE, {{"RM_out_param", port_vector_o_K}, {lambda_module_param_port_prefix, port_o_K}}},
};

const std::vector<OmpTaskFuBinding::indexed_task_connection> OmpTaskFuBinding::lambda_to_resource_manager = {
    {{"done_port", port_o_K, UINT32_MAX}, {"RM_in_FU_done", port_vector_o_K, static_cast<unsigned>(RM_FU_DONE_PORT)}},
};

const std::vector<OmpTaskFuBinding::indexed_task_connection> OmpTaskFuBinding::execution_manager_to_resource_manager = {
    {{"EM_out_request", port_o_K, static_cast<unsigned>(EM_REQ_PORT)},
     {"RM_in_request", port_vector_o_K, static_cast<unsigned>(RM_REQ_PORT)}},
    {{"EM_out_param_addr", port_vector_o_K, static_cast<unsigned>(EM_PARAM_OUT_PORT)},
     {"RM_in_param", port_vector_o_K, static_cast<unsigned>(RM_PARAM_IN_PORT)}},
};

const std::vector<OmpTaskFuBinding::indexed_task_connection> OmpTaskFuBinding::resource_manager_to_execution_manager = {
    {{"RM_out_done", port_o_K, static_cast<unsigned>(RM_DONE_PORT)},
     {"EM_in_RM_done", port_o_K, static_cast<unsigned>(EM_RMDONE_PORT)}},
};

const std::vector<OmpTaskFuBinding::indexed_task_connection> OmpTaskFuBinding::execution_manager_to_execution_manager =
    {
        {{"EM_out_done", port_o_K, static_cast<unsigned>(EM_DONE_PORT)},
         {"EM_in_activation_condition_done", port_o_K, static_cast<unsigned>(EM_ACPORTS)}},
};

std::string OmpTaskFuBinding::em_name_prefix = "EM_";

std::string OmpTaskFuBinding::rm_name_prefix = "RM_";

std::string OmpTaskFuBinding::final_join_name = "final_join";

unsigned OmpTaskFuBinding::param_port_size = 32;

OmpTaskFuBinding::OmpTaskFuBinding(const HLS_managerConstRef _HLSMgr, const unsigned int _function_id,
                                   const ParameterConstRef _parameters)
    : fu_binding(_HLSMgr, _function_id, _parameters), closing_nodes(0)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

std::pair<structural_objectRef, structural_objectRef> OmpTaskFuBinding::generate_resized_connection(
    const std::string& source_port_id, const std::string& dest_port_id, structural_objectRef src,
    structural_objectRef dest, const unsigned long long size, so_kind source_port_type, so_kind dest_port_type,
    unsigned dest_port_num, bool specialize, bool resize)
{
   // Find the destination port and prepare them
   const auto dest_port_obj = collect_port(dest, dest_port_id, size, dest_port_type, dest_port_num, specialize, resize);

   const auto source_port_obj =
       collect_port(src, source_port_id, size, source_port_type, dest_port_num, specialize, resize);

   // Cache connections
   hls_params.connections[source_port_obj] = {dest_port_obj};

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "prepared resized connection between port " + source_port_id + " of " + src->get_id() +
                      " and its destination with port name " + dest_port_id);
   return {source_port_obj, dest_port_obj};
}

structural_objectRef OmpTaskFuBinding::collect_port(structural_objectRef module_obj, const std::string& p_name,
                                                    const unsigned long long size, so_kind p_type, const unsigned p_num,
                                                    bool specialize, bool resize) const
{
   structural_objectRef port_owner = nullptr;
   if(!fu_binding::try_find_port(p_name, module_obj, p_type, port_owner))
   {
      THROW_ERROR("can not find the requested port " + p_name + " inside " + module_obj->get_id());
   }
   const auto port = GetPointer<port_o>(port_owner);
   // Specialize and resize if needed
   if(specialize)
   {
      if(p_type == port_vector_o_K)
      {
         const auto count = port->get_ports_size();
         THROW_ASSERT(count == 0U || count == p_num, "Conflicting task port vector size: " + p_name);
         if(count == 0U)
         {
            port->add_n_ports(p_num, port_owner);
         }
      }
   }
   if(resize)
   {
      port_o::resize_std_port(size, 0U, 0, port_owner);
   }
   return port_owner;
}

void OmpTaskFuBinding::collect_ports(const std::vector<structural_objectRef>& modules, const std::string& p_name,
                                     const unsigned long long size, so_kind p_type, const unsigned p_num,
                                     std::vector<structural_objectRef>& collection) const
{
   for(const auto& m : modules)
   {
      collection.push_back(collect_port(m, p_name, size, p_type, p_num));
   }
}

void OmpTaskFuBinding::forward_ports(const std::vector<std::tuple<std::string, so_kind, port_o::port_direction>>& ports,
                                     const structural_objectRef& from, const structural_objectRef& to,
                                     so_kind port_type, unsigned int port_count, bool memory_port)
{
   for(const auto& entry : ports)
   {
      const auto& name = std::get<0>(entry);
      structural_objectRef source;
      if(!try_find_port(name, from, std::get<1>(entry), source))
      {
         THROW_ERROR("Missing task port " + name);
      }
      [[maybe_unused]] const auto source_port = GetPointerS<port_o>(source);
      THROW_ASSERT(!source_port->get_is_global() && !source_port->get_is_extern() && !source_port->get_is_memory(),
                   "Task port has not been internalized: " + name);
      const auto kind = memory_port ? port_type : source->get_kind();
      auto target = to->find_member(name, port_o_K, to);
      if(!target)
      {
         target = to->find_member(name, port_vector_o_K, to);
      }
      if(!target)
      {
         const auto count = kind == port_vector_o_K ? port_count : 1U;
         add_smart_port(target, source, count, name, std::get<2>(entry), to, hls_params.SM);
      }
      THROW_ASSERT(target->get_kind() == kind, "Inconsistent task port kind: " + name);
      const auto target_port = GetPointerS<port_o>(target);
      THROW_ASSERT(target_port->get_port_direction() == std::get<2>(entry),
                   "Inconsistent task port direction: " + name);
      if(kind == port_vector_o_K)
      {
         THROW_ASSERT(target_port->get_ports_size() == port_count, "Inconsistent task port count: " + name);
      }
      target_port->set_is_memory(memory_port);
   }
}

void OmpTaskFuBinding::parse_lambda_modules()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parsing lambda modules");
   CustomUnorderedSet<std::string> entry_names;
   for(auto& n : nodes)
   {
      entry_names.insert(n.second.task_name);
   }

   unsigned resource_duplication_count = 1U;
   if(parameters->IsParameter("task-resource-duplication"))
   {
      resource_duplication_count = parameters->GetParameter<unsigned>("task-resource-duplication");
   }

   THROW_ASSERT(resource_duplication_count > 0U, "Task resource duplication must be positive");
   for(const auto& m : hls_params.modules)
   {
      const auto id = m->get_typeRef()->id_type;
      if(id.find(name_dict.lambda_prefix) != std::string::npos)
      {
         hls_params.lambda_modules.push_back(m);
         if(id.find(name_dict.duplicated_fu_name_token) == std::string::npos)
         {
            hls_params.lambda_modules_map[id] = m;
         }

         // Preallocate vector in mapping
         if(id.find(name_dict.duplicated_fu_name_token) == std::string::npos)
         {
            duplicated_fus[id] = {};
         }

         // Handle resource duplication for current FU
         for(unsigned resource = 0; resource < resource_duplication_count - 1; resource++)
         {
            structural_objectRef copy(new module_o(debug_level, nullptr));
            m->copy(copy);
            copy->set_owner(m->get_owner());
            copy->set_id(m->get_id() + "_" + STR(resource));
            GetPointerS<module_o>(hls_params.circuit)->add_internal_object(copy);
            duplicated_fus[id].push_back(copy);
            hls_params.lambda_modules.push_back(copy);
         }

         // Create a name mapping
         for(auto& n : entry_names)
         {
            auto task_body_name = n;
            const auto position = id.find(task_body_name);
            const auto end = position == std::string::npos ? position : position + task_body_name.size();
            if(position != std::string::npos && (end == id.size() || id[end] == '_') &&
               (position == 0 || id[position - 1] == '_'))
            {
               THROW_ASSERT(task_entry_name_bambu_to_original_dict.find(task_body_name) ==
                                task_entry_name_bambu_to_original_dict.end(),
                            "should be empty");
               task_entry_name_bambu_to_original_dict[n] = id;
            }
         }
      }
   }
   THROW_ASSERT(hls_params.lambda_modules.size() > 0, "empty lambda modules");

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "_taskEntryNameBambuToOriginalDict:");
   for([[maybe_unused]] auto& p : task_entry_name_bambu_to_original_dict)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, p.first + " -> " + p.second);
   }
   for(const auto& [original, duplicates] : duplicated_fus)
   {
      if(duplicates.size())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "original: " + original);
         for([[maybe_unused]] auto& d : duplicates)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "duplicate: " + d->get_id());
         }
      }
   }

   for(auto& l : hls_params.lambda_modules)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "recovered lambda " + l->get_typeRef()->id_type);
      auto m = GetPointer<module_o>(l);
      auto n_ports = m->get_in_port_size();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "# in port: " + STR(n_ports));
      for(unsigned i = 0; i < n_ports; i++)
      {
         const auto& id = m->get_in_port(i)->get_id();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, id + " / " + m->get_in_port(i)->get_typeRef()->id_type);
         // Cache parameters port
         const auto port = GetPointerS<port_o>(m->get_in_port(i));
         if(id != CLOCK_PORT_NAME && id != RESET_PORT_NAME && id != START_PORT_NAME && !port->get_is_memory())
         {
            hls_params.lambda_params_ports[l->get_id()].push_back(m->get_in_port(i)->get_id());
         }
      }
      n_ports = m->get_out_port_size();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "# out port: " + STR(n_ports));
      for(unsigned i = 0; i < n_ports; i++)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, STR(m->get_out_port(i)->get_id()));
      }
   }
}

void OmpTaskFuBinding::generate_generic_module(const std::string& name, const std::vector<task_port>& ports,
                                               const std::vector<unsigned>& offsets, task_module_kind kind)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Generating generic module_o " + name);
   const auto HLS_D = hls_params.HLSMgr->get_HLS_device();
   const auto tech_man = HLS_D->get_technology_manager();
   const structural_managerRef CM(new structural_manager(parameters));
   const structural_type_descriptorRef module_type(new structural_type_descriptor(name));
   CM->set_top_info(name, module_type);
   const auto top = CM->get_circ();
   GetPointerS<module_o>(top)->set_description("Execution manager: " + name);
   GetPointerS<module_o>(top)->set_copyright(GENERATED_COPYRIGHT);
   GetPointerS<module_o>(top)->set_authors("Component automatically generated by bambu");
   GetPointerS<module_o>(top)->set_license(GENERATED_LICENSE);
   GetPointerS<module_o>(top)->set_multi_unit_multiplicity(1U);

   // Default ports
   CM->add_port("clock", port_o::IN, top, structural_type_descriptorRef(new structural_type_descriptor("bool", 0)));
   CM->add_port("reset", port_o::IN, top, structural_type_descriptorRef(new structural_type_descriptor("bool", 0)));

   for(const auto& p : ports)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "port name " + p.name);
      structural_objectRef port;
      [[maybe_unused]] port_o* port_ptr = nullptr;
      // If vector or not vector
      if(p.size == 0)
      {
         THROW_UNREACHABLE("port of size zero");
      }
      else if(p.size == 1 && !p.force_vector_port)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "adding port with type " + p.type + " and bitsize " + STR(p.type_size));
         port = CM->add_port(p.name, p.direction, top,
                             structural_type_descriptorRef(new structural_type_descriptor(p.type, p.type_size)));
         port_ptr = GetPointer<port_o>(port);
         port_o::resize_std_port(p.type_size, 0U, 0, port);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "generated port " + port_ptr->get_kind_text() + " of bitsize " +
                            STR(port_ptr->get_typeRef()->vector_size));
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "adding port vector of size " + STR(p.size) + " with type " + p.type + " and bitsize " +
                            STR(p.type_size));
         port = CM->add_port_vector(p.name, p.direction, static_cast<unsigned>(p.size), top,
                                    structural_type_descriptorRef(new structural_type_descriptor(p.type, p.type_size)));

         port_ptr = GetPointer<port_o>(port);
         port_o::resize_std_port(p.type_size, 0U, 0, port);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "generated port " + port_ptr->get_kind_text() + " of bitsize " +
                            STR(port_ptr->get_typeRef()->vector_size) + " with vecsize " +
                            STR(port_ptr->get_ports_size()));
      }
   }

   switch(kind)
   {
      case TASK_JOIN:
         write_final_join(CM, ports, offsets);
         break;
      case TASK_RESOURCE_MANAGER:
         write_resource_manager(CM, ports, offsets);
         break;
      case TASK_EXECUTION_MANAGER:
         write_execution_manager(CM, ports, offsets);
         break;
      default:
         THROW_UNREACHABLE("Unexpected task module kind");
   }

   // Save the new module
   hls_params.modules.push_back(top);

   // Add the new resource
   tech_man->add_resource(OPENMP_LIBRARY, name, CM);
}

void OmpTaskFuBinding::manage_extern_global_port(
    [[maybe_unused]] const HLS_managerRef HLSMgr, [[maybe_unused]] const hlsRef HLS,
    [[maybe_unused]] const structural_managerRef SM, [[maybe_unused]] structural_objectRef port_in,
    [[maybe_unused]] unsigned int dir, [[maybe_unused]] structural_objectRef circuit, [[maybe_unused]] unsigned int num)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "skipping manage_extern_global_port of " + port_in->get_path());
   // fu_binding::manage_extern_global_port(HLSMgr, HLS, SM, port_in, dir, circuit, num);
}

void OmpTaskFuBinding::parse_xml(const std::string& file_name)
{
   THROW_ASSERT(nodes.size() == 0 && task_bodies.size() == 0 && adjency_list.size() == 0,
                "Nodes, lambdas and adjency list must be empty");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parsing graph XML file " + file_name);

   pugi::xml_document doc;
   const pugi::xml_parse_result result = doc.load_file(file_name.c_str());
   if(!result)
   {
      THROW_ERROR("pugixml failed to open file " + file_name);
   }

   const auto read_number = [&file_name](pugi::xml_node element, const char* attribute,
                                         bool signed_value = false) -> unsigned {
      const std::string value = element.attribute(attribute).value();
      try
      {
         size_t consumed = 0;
         if(signed_value)
         {
            const auto number = std::stoi(value, &consumed);
            if(consumed == value.size())
            {
               return static_cast<unsigned>(number);
            }
         }
         else if(!value.empty() && value.front() != '-')
         {
            const auto number = std::stoul(value, &consumed);
            if(consumed == value.size() && number <= std::numeric_limits<unsigned>::max())
            {
               return static_cast<unsigned>(number);
            }
         }
      }
      catch(const std::exception&)
      {
      }
      THROW_ERROR("Invalid task graph attribute '" + std::string(attribute) + "' on <" + element.name() + "> in " +
                  file_name + ": '" + value + "'");
      return 0;
   };

   // Get the root element
   pugi::xml_node graph = doc.child("graph");
   THROW_ASSERT(graph, "No graph element found");

   // Parse nodes
   unsigned node_counter = 0;
   pugi::xml_node nodes_element = graph.child("nodes");
   if(nodes_element)
   {
      for(pugi::xml_node node_element : nodes_element.children("node"))
      {
         std::string id = node_element.attribute("id").value();
         boost::replace_all(id, "#", "_");
         boost::replace_all(id, ",", "_");
         boost::replace_all(id, "{", "$");
         boost::replace_all(id, "}", "$");
         const unsigned long in_edges = read_number(node_element, "activating_conditions_count");
         // We add at least one out edge for the final join
         const unsigned long out_edges =
             std::max(static_cast<unsigned long>(read_number(node_element, "out_activating_conditions_count")), 1UL);
         std::string task_name = node_element.attribute("omp_entry_name").value();
         boost::replace_all(task_name, "..", "_");
         boost::replace_all(task_name, ".", "_");
         std::vector<std::tuple<task_input_kind, unsigned, unsigned, unsigned>> inputs;
#ifdef REMOVE_OUTLINED_GEP
         // If we are passing unshifted addresses to the FUs we need to count each
         // address only once as bambu will do this simplyfication leading to a size mismatch between RM and Fus
         // For example:
         // %5 = call i32 @.omp_task_entry..17_transformed_2(ptr %0, ptr %0, i32 0, i32 1)
         // will be simplyfied to
         // %5 = call i32 @.omp_task_entry..17_transformed_2(ptr %0, i32 0, i32 1)

         // This does not work if the ptr addresses are not sorted in some way:
         // %5 = call i32 @.omp_task_entry..17_transformed_2(ptr %1, ptr %0, i32 0) ?
         CustomUnorderedSet<unsigned> visited_inputs;
#endif
         for(pugi::xml_node input_element : node_element.children("input"))
         {
            const auto param_type = TASK_INPUT_PTR;
            const unsigned base_ref = read_number(input_element, "base_ref");
            const unsigned offset = read_number(input_element, "offset", true);
            // TODO: Use this param
            // const int bitSize = std::stoi(inputElement.attribute("bit_size").value());
            const unsigned bit_size = OmpTaskFuBinding::param_port_size;
#ifdef REMOVE_OUTLINED_GEP
            if(visited_inputs.find(base_ref) == visited_inputs.end())
            {
               inputs.push_back({param_type, base_ref, offset, bit_size});
               visited_inputs.insert(base_ref);
            }
#else
            inputs.push_back({param_type, base_ref, offset, bit_size});
#endif
         }
         for(pugi::xml_node input_element : node_element.children("const_input"))
         {
            const auto param_type = TASK_INPUT_COMPILE_TIME_CONST_INT32;
            const unsigned base_ref = UINT32_MAX;
            const unsigned offset = read_number(input_element, "value", true);
            // TODO: Use this param
            // const int bitSize = std::stoi(inputElement.attribute("bit_size").value());
            const unsigned bit_size = OmpTaskFuBinding::param_port_size;
            inputs.push_back({param_type, base_ref, offset, bit_size});
         }
         if(id.empty() || nodes.count(id))
         {
            THROW_ERROR("Empty or duplicate task graph node in " + file_name + ": " + id);
         }
         nodes[id] = {in_edges, out_edges, task_name, inputs};
         nodes_list.push_back({id, node_counter++});
      }
   }

   pugi::xml_node task_body_element = graph.child("omp_entries");
   if(task_body_element)
   {
      for(pugi::xml_node lambda_element : task_body_element.children("entry"))
      {
         std::string id_str = std::string(lambda_element.attribute("id").value());
         boost::replace_all(id_str, "..", "_");
         boost::replace_all(id_str, ".", "_");
         task_bodies.insert(id_str);
      }
   }

   pugi::xml_node top_inputs_element = graph.child("top_inputs");
   if(top_inputs_element)
   {
      for(pugi::xml_node input_element : top_inputs_element.children("input"))
      {
         std::string id_str = std::string(input_element.attribute("id").value());
         std::string id_index_str = std::string(input_element.attribute("index").value());
         top_module_inputs_map[read_number(input_element, "id")] = read_number(input_element, "index");
      }
   }

   // Parse edges
   pugi::xml_node edges_element = graph.child("edges");
   if(edges_element)
   {
      for(pugi::xml_node edge_element : edges_element.children("edge"))
      {
         std::string src = edge_element.attribute("source").value();
         std::string dest = edge_element.attribute("target").value();
         boost::replace_all(src, "#", "_");
         boost::replace_all(src, ",", "_");
         boost::replace_all(src, "{", "$");
         boost::replace_all(src, "}", "$");
         boost::replace_all(dest, "#", "_");
         boost::replace_all(dest, ",", "_");
         boost::replace_all(dest, "{", "$");
         boost::replace_all(dest, "}", "$");
         if(!nodes.count(src) || !nodes.count(dest))
         {
            THROW_ERROR("Unknown task graph edge endpoint in " + file_name + ": " + src + " -> " + dest);
         }
         adjency_list[src].push_back(dest);
      }
   }
}

void OmpTaskFuBinding::internalize_ports(const std::vector<std::tuple<std::string, so_kind>>& ports,
                                         structural_objectRef owner)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Internalizing ports of module_o: " + owner->get_id());
   for(const auto& p : ports)
   {
      const auto found = owner->find_member(std::get<0>(p), std::get<1>(p), owner);
      if(found)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Internalizing port: " + found->get_id());
         auto port = GetPointer<port_o>(found);
         port->set_is_global(false);
         port->set_is_extern(false);
         port->set_is_memory(false);
      }
   }
}

structural_objectRef OmpTaskFuBinding::add_module(const std::string& fu_name, const std::string& module_name)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "Adding module_o from lib " + fu_name + " with name " + module_name);
   const auto technology_manager = hls_params.HLS->HLS_D->get_technology_manager();
   const auto clock_port = hls_params.circuit->find_member(CLOCK_PORT_NAME, port_o_K, hls_params.circuit);
   const auto reset_port = hls_params.circuit->find_member(RESET_PORT_NAME, port_o_K, hls_params.circuit);
   const auto module_lib = technology_manager->get_library(fu_name);
   THROW_ASSERT(module_lib != "", "module_o is null");
   const auto module_obj = hls_params.SM->add_module_from_technology_library(module_name, fu_name, module_lib,
                                                                             hls_params.circuit, technology_manager);
   THROW_ASSERT(module_obj, "module_o is null");
   const auto clock_port_module = module_obj->find_member(CLOCK_PORT_NAME, port_o_K, module_obj);
   const auto reset_port_module = module_obj->find_member(RESET_PORT_NAME, port_o_K, module_obj);
   add_smart_connection(reset_port, reset_port_module, 0, hls_params.circuit, hls_params.SM);
   add_smart_connection(clock_port, clock_port_module, 0, hls_params.circuit, hls_params.SM);
   return module_obj;
}

std::pair<structural_objectRef, std::vector<structural_objectRef>>
OmpTaskFuBinding::generate_vectorised_and_resized_connections_one_to_many(
    const std::string& source_port_id, const std::string& dest_port_id, structural_objectRef src,
    const std::vector<structural_objectRef>& dest, const unsigned long long size, so_kind source_port_type,
    so_kind dest_port_type, unsigned dest_port_num)
{
   // Find the destination port and prepare them
   std::vector<structural_objectRef> dest_ports;
   collect_ports(dest, dest_port_id, size, dest_port_type, dest_port_num, dest_ports);

   const auto source_port_obj = collect_port(src, source_port_id, size, source_port_type,
                                             std::max(static_cast<unsigned>(dest_ports.size()) * dest_port_num, 1U));

   // Cache connections
   hls_params.connections[source_port_obj] = dest_ports;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "prepared resized connection between port " + source_port_id + " of " + src->get_id() +
                      " and its destinations with port name " + dest_port_id);
   return {source_port_obj, dest_ports};
}

static void terminate_task_tag_port(const structural_objectRef& port, const structural_managerRef& SM,
                                    const structural_objectRef& circuit)
{
   const auto vector = GetPointerS<port_o>(port);
   if(vector->get_ports_size() == 0U)
   {
      vector->add_n_ports(1U, port);
   }
   port_o::resize_std_port(1U, 0U, 0, port);
   for(auto index = 0U; index < vector->get_ports_size(); ++index)
   {
      const auto element = vector->get_port(index);
      const auto name = "unused_" + port->get_id() + "_" + STR(index);
      const auto terminal = vector->get_port_direction() == port_o::IN ?
                                SM->add_constant(name, circuit, element->get_typeRef(), "0") :
                                SM->add_sign(name, circuit, element->get_typeRef());
      SM->add_connection(element, terminal);
   }
}

void OmpTaskFuBinding::generate_mem_interface()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Generating " + name_dict.mem_arbitrer_name);
   const auto addr_bus_bitsize =
       static_cast<unsigned int>(GetPointer<application_manager>(hls_params.HLSMgr)->get_address_bitsize());
   const auto data_bus_bitsize = static_cast<unsigned int>(hls_params.HLSMgr->Rmem->get_bus_data_bitsize());
   const auto size_bus_bitsize = static_cast<unsigned int>(hls_params.HLSMgr->Rmem->get_bus_size_bitsize());
   const auto Mout_tag_port =
       hls_params.lambda_modules.front()->find_member("Mout_tag", port_o_K, hls_params.lambda_modules.front());
   unsigned int inner_tag_size = 0U;
   if(Mout_tag_port)
   {
      inner_tag_size = static_cast<unsigned int>(Mout_tag_port->get_typeRef()->vector_size);
   }
   const auto tag_size =
       std::max(1U, inner_tag_size + ceil_log2(static_cast<unsigned int>(hls_params.lambda_modules.size())));

   const auto is_bus_pipelined =
       parameters->isOption(OPT_bus_pipelined) && parameters->getOption<bool>(OPT_bus_pipelined);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Bus pipelined:  " + STR(is_bus_pipelined));

   static_assert(std::is_same_v<decltype(data_bus_bitsize), decltype(size_bus_bitsize)>, "expected same type");

   CustomUnorderedMap<std::string, unsigned int> port_weights;
   for(const auto& p : mem_arbitrer_totask_graph_ports)
   {
      const auto p_name = std::get<0>(p.src);
      if(p_name == "Mout_addr_ram_out")
      {
         port_weights[p_name] = addr_bus_bitsize;
      }
      else if(p_name == "Mout_Wdata_ram_out" || p_name == "M_Rdata_ram")
      {
         port_weights[p_name] = data_bus_bitsize;
      }
      else if(p_name == "Mout_data_ram_size_out")
      {
         port_weights[p_name] = size_bus_bitsize;
      }
      else if(p_name == "Mout_tag_out" || p_name == "M_tag")
      {
         port_weights[p_name] = tag_size;
      }
      else
      {
         port_weights[p_name] = 1U;
      }
   }
   for(const auto& p : mem_arbitrer_to_lambda_modules)
   {
      const auto p_name = std::get<0>(p.src);
      if(p_name == "Mout_addr_ram")
      {
         port_weights[p_name] = addr_bus_bitsize;
      }
      else if(p_name == "Mout_Wdata_ram" || p_name == "M_Rdata_ram_out")
      {
         port_weights[p_name] = data_bus_bitsize;
      }
      else if(p_name == "Mout_data_ram_size")
      {
         port_weights[p_name] = size_bus_bitsize;
      }
      else if(p_name == "Mout_tag" || p_name == "M_tag_out")
      {
         port_weights[p_name] = std::max(inner_tag_size, 1U);
      }
      else
      {
         port_weights[p_name] = 1U;
      }
   }

   structural_objectRef arbiter_module = nullptr;
   if(hls_params.use_banked_mem)
   {
      const std::string interface_name = name_dict.banked_mem_name + "_" + hls_params.owner_module->get_id();
      arbiter_module = add_module(name_dict.banked_mem_name, interface_name);
      arbiter_module->SetParameter("SELECTOR_SIZE", STR(std::max(ceil_log2((hls_params.lambda_modules.size())), 1UL)));
      specialized_name_dict.mem_arbitrer_name = interface_name;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Generated " + interface_name);
   }
   else
   {
      std::string arbitrer_name = "";
      if(hls_params.mem_channel_num == 1)
      {
         arbitrer_name = name_dict.simple_mem_arbiter_name + "_" + hls_params.owner_module->get_id();
         arbiter_module = add_module(name_dict.simple_mem_arbiter_name, arbitrer_name);
      }
      else
      {
         arbitrer_name = name_dict.mem_arbitrer_name + "_" + hls_params.owner_module->get_id();
         arbiter_module = add_module(name_dict.mem_arbitrer_name, arbitrer_name);
      }
      arbiter_module->SetParameter("SELECTOR_SIZE", STR(std::max(ceil_log2((hls_params.lambda_modules.size())), 1UL)));
      specialized_name_dict.mem_arbitrer_name = arbitrer_name;
      const auto arbiter_type = parameters->getOption<std::string>(OPT_bus_arbiter_type);
      if(arbiter_type == "RR")
      {
         arbiter_module->SetParameter("ARBITER_TYPE", STR(1));
      }
      else if(arbiter_type == "LP")
      {
         arbiter_module->SetParameter("ARBITER_TYPE", STR(2));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Generated " + arbitrer_name);
   }

   if(is_bus_pipelined)
   {
      arbiter_module->SetParameter("IS_PIPELINED", STR(1));
   }

   if(is_bus_pipelined)
   {
      const std::vector<std::tuple<std::string, port_o::port_direction>> M_tag_ports = {{"Mout_tag", port_o::OUT},
                                                                                        {"M_tag", port_o::IN}};

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Adding missing tag ports to owner module_o");
      for(const auto& mp : M_tag_ports)
      {
         const auto pname = std::get<0>(mp);
         const auto direction = std::get<1>(mp);

         structural_objectRef port = hls_params.owner_module->find_member(
             pname, hls_params.mem_channel_num == 1U ? port_o_K : port_vector_o_K, hls_params.owner_module);
         if(!port)
         {
            if(hls_params.mem_channel_num == 1U)
            {
               port =
                   hls_params.SM->add_port(pname, direction, hls_params.owner_module,
                                           structural_type_descriptorRef(new structural_type_descriptor("bool", 1U)));
            }
            else
            {
               port = hls_params.SM->add_port_vector(
                   pname, direction, hls_params.mem_channel_num, hls_params.owner_module,
                   structural_type_descriptorRef(new structural_type_descriptor("bool", 1U)));
            }
            port_o::resize_std_port(tag_size, 0U, 0, port);
            GetPointer<port_o>(port)->set_is_memory(true);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Added tag port " + pname);
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Not adding tag port " + pname + " as already found in the owner module_o");
         }
      }
   }

   for(const auto& p : mem_arbitrer_totask_graph_ports)
   {
      const auto p_name_from = std::get<0>(p.src);
      const auto p_name_to = std::get<0>(p.dest);
      auto p_size = port_weights[p_name_from];
      const auto p_type_from = std::get<1>(p.src);
      const auto p_type_to = std::get<1>(p.dest);
      const auto dest_port_num = hls_params.mem_channel_num;

      if((p_name_from == "Mout_tag_out" || p_name_from == "M_tag") && !is_bus_pipelined)
      {
         const auto tag_port = arbiter_module->find_member(p_name_from, p_type_from, arbiter_module);
         THROW_ASSERT(tag_port, "port " + p_name_from + " not found in arbiter module_o");
         terminate_task_tag_port(tag_port, hls_params.SM, hls_params.circuit);
         continue;
      }

      const auto to_connect =
          generate_resized_connection(p_name_from, p_name_to, arbiter_module, hls_params.owner_module, p_size,
                                      p_type_from, p_type_to, dest_port_num);

      // Memory interface is already port_vector
      const auto owner_module_port_ptr = GetPointer<port_o>(to_connect.second);
      const auto iface_module_ptr = GetPointer<port_o>(to_connect.first);
      if(owner_module_port_ptr->get_kind() == port_vector_o_K)
      {
         THROW_ASSERT(owner_module_port_ptr->get_ports_size() == iface_module_ptr->get_ports_size(),
                      "different vector size. Task graph has " + STR(owner_module_port_ptr->get_ports_size()) +
                          " ports while memory interface has " + STR(iface_module_ptr->get_ports_size()) + " ports");
         THROW_ASSERT(owner_module_port_ptr->get_ports_size() > 0, "zero vector size");
         for(unsigned i = 0; i < owner_module_port_ptr->get_ports_size(); i++)
         {
            perform_connection_one_to_one({iface_module_ptr->get_port(i), owner_module_port_ptr->get_port(i)});
         }
      }
      else
      {
         auto& vector_pos = GetPointer<port_o>(to_connect.first)->get_port(0);
         perform_connection_one_to_one({vector_pos, to_connect.second});
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Generated memory interface to owner module_o ports");

   for(const auto& p : mem_arbitrer_to_lambda_modules)
   {
      const auto p_name_from = std::get<0>(p.src);
      const auto p_name_to = std::get<0>(p.dest);
      const auto p_size = port_weights[p_name_from];
      const auto p_type_from = std::get<1>(p.src);
      const auto p_type_to = std::get<1>(p.dest);
      const auto dest_port_num = 1U;

      if((p_name_from == "Mout_tag" || p_name_from == "M_tag_out") && inner_tag_size == 0)
      {
         const auto tag_port = arbiter_module->find_member(p_name_from, p_type_from, arbiter_module);
         THROW_ASSERT(tag_port, "port " + p_name_from + " not found in arbiter module_o");
         terminate_task_tag_port(tag_port, hls_params.SM, hls_params.circuit);
         continue;
      }

      const auto to_connect = generate_vectorised_and_resized_connections_one_to_many(
          p_name_from, p_name_to, arbiter_module, hls_params.lambda_modules, p_size, p_type_from, p_type_to,
          dest_port_num);
      // Src is a port vector which must be connected to multiple port_o (a single slot of the vector to multiple
      // port_o)
      const auto& src_vec_port = std::get<0>(to_connect);
      [[maybe_unused]] const auto vec_size = GetPointer<port_o>(src_vec_port)->get_ports_size();
      THROW_ASSERT(vec_size == (std::get<1>(to_connect)).size() * dest_port_num,
                   "port vector not specialized. src has " + STR(vec_size) + " ports while dest has " +
                       STR(std::get<1>(to_connect).size() * dest_port_num) + " ports");

      for(unsigned i = 0; i < std::get<1>(to_connect).size(); ++i)
      {
         for(unsigned j = 0; j < dest_port_num; j++)
         {
            const auto src_port = GetPointer<port_o>(src_vec_port)->get_port(i * dest_port_num + j);
            const auto dest_port = GetPointer<port_o>(std::get<1>(to_connect)[i]);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "connecting (one to many) " + src_port->get_id() + " of type " +
                               (src_port->get_kind() == port_o_K ? "port_o" : "port_vector_o") + " with " +
                               dest_port->get_id() + " of type " +
                               (dest_port->get_kind() == port_o_K ? "port_o" : "port_vector_o"));
            perform_connection_one_to_one(
                {src_port, dest_port_num == 1U ? std::get<1>(to_connect)[i] : dest_port->get_port(j)});
         }
      }
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Generated memory interface to FU ports");
}

void OmpTaskFuBinding::allocate_em_to_rm()
{
   for(const auto& node : nodes_list)
   {
      const auto& mapped_node = nodes[node.first];
      const auto& task_body_name = mapped_node.task_name;
      THROW_ASSERT(task_bodies.find(task_body_name) != task_bodies.end(), "unrecognized task body name");
      rm_allocation[task_body_name].push_back(node.first);
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "RM allocation: ");
   for(auto& [rm, ems] : rm_allocation)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "RM: " + rm);
      for([[maybe_unused]] auto& em : ems)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "EM: " + em);
      }
   }
}

void OmpTaskFuBinding::generate_const_rm_inputs()
{
   // We need to give to a const input a unique name but some nodes can have the same const input repeated multiple
   // times (as different input args). We generate an unique identifier.
   CustomUnorderedMap<std::string const*, CustomUnorderedMap<unsigned, unsigned>> node_const_input_value_dict;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "generating const RM inputs");
   for(const auto& node : nodes)
   {
      // This is the reference to be attached to the RM port. This last will have a vector_port for each const input
      // param from EMs
      unsigned const_input_index = 0;
      for(const auto& input : node.second.inputs)
      {
         if(std::get<0>(input) == TASK_INPUT_COMPILE_TIME_CONST_INT32)
         {
            const auto value = STR(std::get<2>(input));
            const auto unique_value_id = STR(node_const_input_value_dict[&node.first][std::get<2>(input)]++);
            structural_objectRef const_obj = hls_params.SM->add_constant(
                "const_node_" + node.first + "_" + value + "_" + unique_value_id, hls_params.circuit,
                structural_type_descriptorRef(new structural_type_descriptor("bool", std::get<3>(input))), value);
            em_const_inputs[node.first].push_back({const_input_index, const_obj});
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "generated const input module_o for " + node.first + " with value " + STR(value) + " (" +
                               const_obj->get_id() + ")");
            const_input_index++;
         }
      }
   }
}

void OmpTaskFuBinding::write_final_join(structural_managerRef component, const std::vector<task_port>& ports,
                                        const std::vector<unsigned>& /*const_offsets*/)
{
   THROW_ASSERT(ports.size() == 2, "unexpected number of ports");
   THROW_ASSERT(ports[0].direction == port_o::IN && ports[1].direction == port_o::OUT, "unexpected ports directions");
   std::string verilog_code = "";
   std::string verilog_component = "SIMPLEJOIN2_FU";

   verilog_code += "wire done;\n";

   verilog_code += "SIMPLEJOIN2_FU #(.BITSIZE_in1(" + STR(ports[0].size) + "))\n";
   verilog_code += "end_conditions (.in1(" + ports[0].name + "),\n";
   verilog_code += ".clock(clock),\n";
   verilog_code += ".reset(reset),\n";
   verilog_code += ".out1(done));\n\n";
   verilog_code += "assign " + ports[1].name + " = done;\n\n";

   component->add_NP_functionality(component->get_circ(), NP_functionality::VERILOG_PROVIDED, verilog_code);
   component->add_NP_functionality(component->get_circ(), NP_functionality::IP_COMPONENT, verilog_component);
   component->add_NP_functionality(component->get_circ(), NP_functionality::LIBRARY, ports[0].name);
}

void OmpTaskFuBinding::create_final_join()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Creating lib final join");

   const std::string port_type_name = "bool";
   unsigned last_nodes_num = 0;
   for(auto& em : hls_params.em_modules)
   {
      const auto successors = adjency_list[em.first].size();
      if(successors == 0)
      {
         last_nodes_num++;
      }
   }

   closing_nodes = last_nodes_num;

   // Allocate the ports vector
   std::vector<task_port> ports = {{"join_in", port_type_name, 1, last_nodes_num, true, port_o::IN},
                                   {"out_done", port_type_name, 1, 1U, false, port_o::OUT}

   };
   generate_generic_module(OmpTaskFuBinding::final_join_name, ports, {}, TASK_JOIN);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created lib final join");
}

void OmpTaskFuBinding::write_resource_manager(structural_managerRef component, const std::vector<task_port>& ports,
                                              const std::vector<unsigned>& /*const_offsets*/)
{
   // Share the prefix selection with technology_manager::xload. This RTL
   // is generated after library loading, so select its encoder names here rather
   // than relying on the library consumer rewrite or a Verilog parameter.
   const auto encoder_prefix =
       hls_params.HLSMgr->get_HLS_device()->get_technology_manager()->get_priority_encoder_prefix();

   // Ports sequence follow the task_resource_port up to DonePort (included). Then dynamic ports are applied. e.g:
   // FUDonePort = 0,
   // FUStartPort = 1,
   // ReqPort = 2,
   // DonePort = 3,
   // ParamIn1 (port_vector_o) = 4
   // ParamOut1 (port_o) = 5

   const unsigned resource_number =
       duplicated_fus.empty() ? 1U : static_cast<unsigned>(duplicated_fus.begin()->second.size() + 1);
   const auto resource_number_size = std::max(ceil_log2(resource_number), 1U);
   std::string verilog_code = "";
   std::string verilog_component = "left_priority_arbiter_core,MUX_N_to_1_enabled";
   THROW_ASSERT((ports.size() - 4) % 2 == 0, "unexpected number of ports");
   const unsigned param_number = static_cast<unsigned int>((ports.size() - 4) / 2);
   const auto em_bit_size_count = std::max(ceil_log2(ports[2].size), 1UL);
   const unsigned em_count = static_cast<unsigned>(ports[2].size);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "bit size EM in " + STR(em_bit_size_count));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "EM number " + STR(em_count));

   verilog_code += "localparam RESOURCE_NUMBER = " + STR(resource_number) + ";\n";
   verilog_code += "localparam REQUEST_NUMBER = " + STR(em_count) + ";\n";
   verilog_code += "localparam RESOURCE_NUMBER_SIZE = " + STR(resource_number_size) + ";\n";
   verilog_code += "localparam BITSIZE_selector = " + STR(em_bit_size_count) + ";\n";
   verilog_code += "wire [BITSIZE_selector-1:0] selector_comb_out;\n";
   if(resource_number > 1)
   {
      verilog_code += "reg [RESOURCE_NUMBER*BITSIZE_selector-1:0] selectore_storage_reg, selectore_storage_next;\n";
      verilog_code += "reg [RESOURCE_NUMBER-1:0] resource_reg;\n";
      verilog_code += "wire [RESOURCE_NUMBER-1:0] resource_next;\n";
      verilog_code += "reg [REQUEST_NUMBER-1:0] request_mask_reg;\n";
      verilog_code += "wire [REQUEST_NUMBER-1:0] request_mask_next;\n";
      verilog_code += "reg [RESOURCE_NUMBER-1:0] done_reg;\n";
      verilog_code += "wire [RESOURCE_NUMBER-1:0] done_next;\n";
      verilog_code += "wire [RESOURCE_NUMBER_SIZE-1:0] resource_selected;\n";
      verilog_code += "wire [RESOURCE_NUMBER_SIZE-1:0] done_selected;\n";
      verilog_code += "wire done_empty;\n";
   }
   else
   {
      verilog_code += "reg [BITSIZE_selector-1:0] selector_reg;\n";
      verilog_code += "wire [BITSIZE_selector-1:0] selector_next;\n";
      verilog_code += "reg ready_reg;\n";
      verilog_code += "wire ready_next;\n";
   }
   verilog_code += "wire valid_wire;\n\n";

   if(resource_number > 1)
   {
      verilog_component += "," + encoder_prefix + STR(resource_number) + "_" + STR(resource_number_size);

      verilog_code += "always @(posedge clock)\n";
      verilog_code += "begin\n";
      verilog_code += "  if(1RESET_VALUE)\n";
      verilog_code += "  begin\n";
      verilog_code += "    resource_reg <= {RESOURCE_NUMBER{1'b1}};\n";
      verilog_code += "    selectore_storage_reg <= 0;\n";
      verilog_code += "    request_mask_reg <= {REQUEST_NUMBER{1'b1}};\n";
      verilog_code += "  end\n";
      verilog_code += "  else\n";
      verilog_code += "  begin\n";
      verilog_code += "    resource_reg <= resource_next;\n";
      verilog_code += "    selectore_storage_reg <= selectore_storage_next;\n";
      verilog_code += "    request_mask_reg <= request_mask_next;\n";
      verilog_code += "  end\n";
      verilog_code += "end\n\n";

      verilog_code += " " + encoder_prefix + STR(resource_number) + "_" + STR(resource_number_size) +
                      " resource_encoder (.i(resource_reg),\n";
      verilog_code += "  .o(resource_selected));\n";
      verilog_code += "assign resource_next = (valid_wire && |resource_reg) ? (resource_reg & ~(1'b1 << "
                      "resource_selected)) | " +
                      ports[0].name + " : (resource_reg | " + ports[0].name + ");\n";
      verilog_code += "assign request_mask_next = ((valid_wire && |resource_reg) ? "
                      "(request_mask_reg & ~(1'b1 << "
                      "selector_comb_out)) : request_mask_reg) | (!done_empty << "
                      "selectore_storage_reg[done_selected*BITSIZE_selector+:BITSIZE_selector]);\n";

      verilog_code += "integer i;\n";
      verilog_code += "always @(*)\n";
      verilog_code += "begin\n";
      verilog_code += "  for(i = 0; i < RESOURCE_NUMBER; i = i + 1)\n";
      verilog_code += "  begin\n";
      verilog_code += "    if(valid_wire && |resource_reg && resource_selected == i)\n";
      verilog_code += "    begin\n";
      verilog_code += "      selectore_storage_next[i*BITSIZE_selector+:BITSIZE_selector] = selector_comb_out;\n";
      verilog_code += "    end\n";
      verilog_code += "    else\n";
      verilog_code += "    begin\n";
      verilog_code += "      selectore_storage_next[i*BITSIZE_selector+:BITSIZE_selector] = "
                      "selectore_storage_reg[i*BITSIZE_selector+:BITSIZE_selector];\n";
      verilog_code += "    end\n";
      verilog_code += "  end\n";
      verilog_code += "end\n\n";
   }
   else
   {
      verilog_code += "always @(posedge clock)\n";
      verilog_code += "begin\n";
      verilog_code += "  if(1RESET_VALUE)\n";
      verilog_code += "  begin\n";
      verilog_code += "    selector_reg <= 0;\n";
      verilog_code += "    ready_reg <= 1;\n";
      verilog_code += "  end\n";
      verilog_code += "  else\n";
      verilog_code += "  begin\n";
      verilog_code += "    selector_reg <= selector_next;\n";
      verilog_code += "    ready_reg <= ready_next;\n";
      verilog_code += "  end\n";
      verilog_code += "end\n\n";
   }

   verilog_code += "left_priority_arbiter_core #(.BITSIZE_in1(REQUEST_NUMBER),\n";
   if(resource_number > 1)
   {
      verilog_code += ".BITSIZE_out1(" + STR(em_bit_size_count) + ")) left_arbiter_p (.in1(" + ports[2].name +
                      " & request_mask_reg),\n";
   }
   else
   {
      verilog_code += ".BITSIZE_out1(" + STR(em_bit_size_count) + ")) left_arbiter_p (.in1(" + ports[2].name + "),\n";
   }
   verilog_code += ".out1(selector_comb_out),\n";
   verilog_code += ".reset(reset),\n";
   verilog_code += ".clock(clock),\n";
   verilog_code += ".valid(valid_wire));\n\n";

   std::string port_size_str = "";
   for(auto i = 0U; i < param_number; i++)
   {
      const auto port_name = ports[i + 4].name;
      const auto port_out_name = ports[i + 4 + param_number].name;

      port_size_str += port_name + " " + port_out_name + " ";

      if(resource_number > 1)
      {
         verilog_code += "wire [BITSIZE_" + port_out_name + "-1:0] " + port_out_name + "_wire;\n";
         verilog_code += "reg [RESOURCE_NUMBER * BITSIZE_" + port_out_name + "-1:0] " + port_out_name + "_reg;\n";
         verilog_code += "reg [RESOURCE_NUMBER * BITSIZE_" + port_out_name + "-1:0] " + port_out_name + "_next;\n\n";

         verilog_code += "always @(posedge clock)\n";
         verilog_code += "begin\n";
         verilog_code += "  if(1RESET_VALUE)\n";
         verilog_code += "  begin\n";
         verilog_code += "    " + port_out_name + "_reg <= 0;\n";
         verilog_code += "  end\n";
         verilog_code += "  else\n";
         verilog_code += "  begin\n";
         verilog_code += "    " + port_out_name + "_reg <= " + port_out_name + "_next;\n";
         verilog_code += "  end\n";
         verilog_code += "end\n\n";

         // We don't need the mux as all the base pointers are shared. Ports structure are left if mux will be
         // needed in the future In that case just remove the if branch
         if(port_name.find(ptr_prefix) != std::string::npos)
         {
            verilog_code +=
                "assign " + port_out_name + "_wire = " + port_name + "[BITSIZE_" + port_out_name + "-1:0];\n\n";
         }
         else
         {
            verilog_code += "MUX_N_to_1_enabled #(\n";
            verilog_code += "   .PORTSIZE_in1(PORTSIZE_" + port_name + "),\n";
            verilog_code += "   .BITSIZE_in1(BITSIZE_" + port_name + "),\n";
            verilog_code += "   .BITSIZE_selector(BITSIZE_selector),\n";
            verilog_code += "   .BITSIZE_out1(BITSIZE_" + port_out_name + ")) MUX_N_to_1_" + port_name +
                            " (.selector(selector_comb_out),\n";
            verilog_code += "   .out1(" + port_out_name + "_wire),\n";
            verilog_code += "   .in1(" + port_name + "),\n";
            verilog_code += "   .enabled(valid_wire));\n\n";
         }

         verilog_code += "integer i_" + STR(i) + ";\n";
         verilog_code += "always @(*)\n";
         verilog_code += "begin\n";
         verilog_code += "  for(i_" + STR(i) + " = 0; i_" + STR(i) + " < RESOURCE_NUMBER; i_" + STR(i) + " = i_" +
                         STR(i) + " + 1)\n";
         verilog_code += "  begin\n";
         verilog_code += "    if(valid_wire && |resource_reg && resource_selected == i_" + STR(i) + ")\n";
         verilog_code += "    begin\n";
         verilog_code += "      " + port_out_name + "_next[i_" + STR(i) + "*BITSIZE_" + port_out_name + "+:BITSIZE_" +
                         port_out_name + "] = " + port_out_name + "_wire;\n";
         verilog_code += "    end\n";
         verilog_code += "    else\n";
         verilog_code += "    begin\n";
         verilog_code += "      " + port_out_name + "_next[i_" + STR(i) + "*BITSIZE_" + port_out_name + "+:BITSIZE_" +
                         port_out_name + "] = " + port_out_name + "_reg[i_" + STR(i) + "*BITSIZE_" + port_out_name +
                         "+:BITSIZE_" + port_out_name + "];\n";
         verilog_code += "    end\n";
         verilog_code += "  end\n";
         verilog_code += "end\n\n";

         verilog_code += "assign " + port_out_name + " = " + port_out_name + "_next;\n";
      }
      else
      {
         verilog_code += "wire [BITSIZE_" + port_out_name + "-1:0] " + port_out_name + "_wire;\n";
         verilog_code += "reg [ BITSIZE_" + port_out_name + "-1:0] " + port_out_name + "_reg;\n";
         verilog_code += "reg [ BITSIZE_" + port_out_name + "-1:0] " + port_out_name + "_next;\n\n";

         verilog_code += "always @(posedge clock)\n";
         verilog_code += "begin\n";
         verilog_code += "  if(1RESET_VALUE)\n";
         verilog_code += "  begin\n";
         verilog_code += "    " + port_out_name + "_reg <= 0;\n";
         verilog_code += "  end\n";
         verilog_code += "  else\n";
         verilog_code += "  begin\n";
         verilog_code += "    " + port_out_name + "_reg <= " + port_out_name + "_next;\n";
         verilog_code += "  end\n";
         verilog_code += "end\n\n";

         // We don't need the mux as all the base pointers are shared. Ports structure are left if mux will be
         // needed in the future In that case just remove the if branch
         if(port_name.find(ptr_prefix) != std::string::npos)
         {
            verilog_code +=
                "assign " + port_out_name + "_wire = " + port_name + "[BITSIZE_" + port_out_name + "-1:0];\n\n";
         }
         else
         {
            verilog_code += "MUX_N_to_1_enabled #(\n";
            verilog_code += "   .PORTSIZE_in1(PORTSIZE_" + port_name + "),\n";
            verilog_code += "   .BITSIZE_in1(BITSIZE_" + port_name + "),\n";
            verilog_code += "   .BITSIZE_selector(BITSIZE_selector),\n";
            verilog_code += "   .BITSIZE_out1(BITSIZE_" + port_out_name + ")) MUX_N_to_1_" + port_name +
                            " (.selector(selector_comb_out),\n";
            verilog_code += "   .out1(" + port_out_name + "_wire),\n";
            verilog_code += "   .in1(" + port_name + "),\n";
            verilog_code += "   .enabled(valid_wire));\n\n";
         }

         verilog_code += "always @(*)\n";
         verilog_code += "begin\n";
         verilog_code += "  if(ready_reg)\n";
         verilog_code += "  begin\n";
         verilog_code += "      " + port_out_name + "_next = " + port_out_name + "_wire;\n";
         verilog_code += "  end\n";
         verilog_code += "  else\n";
         verilog_code += "  begin\n";
         verilog_code += "      " + port_out_name + "_next = " + port_out_name + "_reg;\n";
         verilog_code += "  end\n";
         verilog_code += "end\n\n";

         verilog_code += "assign " + port_out_name + " = " + port_out_name + "_next;\n\n";
      }
   }

   verilog_code += "\n";
   if(resource_number > 1)
   {
      verilog_code += " " + encoder_prefix + STR(resource_number) + "_" + STR(resource_number_size) +
                      " done_encoder (.i(" + ports[0].name + " | done_reg),\n";
      verilog_code += "  .o(done_selected),\n";
      verilog_code += "  .empty(done_empty));\n";
      verilog_code +=
          "assign " + ports[3].name +
          " = done_empty ? 0 : 1'b1 << selectore_storage_reg[done_selected*BITSIZE_selector+:BITSIZE_selector];\n";
      verilog_code += "assign " + ports[1].name + " = resource_reg & (valid_wire << resource_selected);\n";
      verilog_code += "assign done_next = (done_reg | " + ports[0].name + ") & ~(1'b1 << done_selected);\n";

      verilog_code += "always @(posedge clock)\n";
      verilog_code += "begin\n";
      verilog_code += "  if(1RESET_VALUE)\n";
      verilog_code += "  begin\n";
      verilog_code += "    done_reg <= 0;\n";
      verilog_code += "  end\n";
      verilog_code += "  else\n";
      verilog_code += "  begin\n";
      verilog_code += "    done_reg <= done_next;\n";
      verilog_code += "  end\n";
      verilog_code += "end\n\n";
   }
   else
   {
      verilog_code += "assign " + ports[3].name + " = " + ports[0].name + " << selector_reg;\n";
      verilog_code += "assign " + ports[1].name + " = valid_wire && ready_reg;\n";
      verilog_code += "assign selector_next = ready_reg ? selector_comb_out : selector_reg;\n";
      verilog_code += "assign ready_next =  (" + ports[0].name + " || ready_reg) && !" + ports[1].name + ";\n";
   }

   component->add_NP_functionality(component->get_circ(), NP_functionality::VERILOG_PROVIDED, verilog_code);
   component->add_NP_functionality(component->get_circ(), NP_functionality::IP_COMPONENT, verilog_component);

   component->add_NP_functionality(component->get_circ(), NP_functionality::LIBRARY,
                                   ports[2].name + " " + ports[3].name + " " + port_size_str);
}

void OmpTaskFuBinding::create_lib_resource_managers()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Creating lib RMs");

   const std::string port_type_name = "bool";
   for(const auto& [rm_name, ems] : rm_allocation)
   {
      const auto node_name = rm_name_prefix + rm_name;
      resource_manager_name_dict[rm_name].first = node_name;

      // Allocate the ports vector
      std::vector<task_port> ports;

      const auto& lambda_module = hls_params.lambda_modules_map[task_entry_name_bambu_to_original_dict[rm_name]];
      const auto id = lambda_module->get_typeRef()->id_type;
      THROW_ASSERT(duplicated_fus.count(id), "expected " + id + " in _duplicatedFUs");
      const auto duplicates_size = duplicated_fus[id].size();
      std::string name_str = std::get<0>(rm_ports[static_cast<unsigned>(RM_FU_DONE_PORT)]);
      task_port in_fu_done_port = {name_str,
                                   port_type_name,
                                   1,
                                   1 + duplicates_size,
                                   std::get<1>(rm_ports[static_cast<unsigned>(RM_FU_DONE_PORT)]) == port_vector_o_K,
                                   std::get<2>(rm_ports[static_cast<unsigned>(RM_FU_DONE_PORT)])};
      ports.push_back(in_fu_done_port);
      resource_manager_name_dict[rm_name].second[static_cast<unsigned>(RM_FU_DONE_PORT)].push_back(name_str);

      name_str = std::get<0>(rm_ports[static_cast<unsigned>(RM_FU_REQ_PORT)]);
      task_port out_fu_req_port = {name_str,
                                   port_type_name,
                                   1,
                                   1 + duplicates_size,
                                   std::get<1>(rm_ports[static_cast<unsigned>(RM_FU_REQ_PORT)]) == port_vector_o_K,
                                   std::get<2>(rm_ports[static_cast<unsigned>(RM_FU_REQ_PORT)])};
      ports.push_back(out_fu_req_port);
      resource_manager_name_dict[rm_name].second[static_cast<unsigned>(RM_FU_REQ_PORT)].push_back(name_str);

      const auto num_reqs = static_cast<unsigned>(ems.size());
      name_str = std::get<0>(rm_ports[static_cast<unsigned>(RM_REQ_PORT)]);
      task_port in_req_port = {name_str,
                               port_type_name,
                               1,
                               num_reqs,
                               std::get<1>(rm_ports[static_cast<unsigned>(RM_REQ_PORT)]) == port_vector_o_K,
                               std::get<2>(rm_ports[static_cast<unsigned>(RM_REQ_PORT)])};
      ports.push_back(in_req_port);
      resource_manager_name_dict[rm_name].second[static_cast<unsigned>(RM_REQ_PORT)].push_back(name_str);

      name_str = std::get<0>(rm_ports[static_cast<unsigned>(RM_DONE_PORT)]);
      task_port out_done_port = {name_str,
                                 port_type_name,
                                 1,
                                 num_reqs,
                                 std::get<1>(rm_ports[static_cast<unsigned>(RM_DONE_PORT)]) == port_vector_o_K,
                                 std::get<2>(rm_ports[static_cast<unsigned>(RM_DONE_PORT)])};
      ports.push_back(out_done_port);
      resource_manager_name_dict[rm_name].second[static_cast<unsigned>(RM_DONE_PORT)].push_back(name_str);

      // Retrieve for each input type its vector size (# attached EMs)
      const unsigned em_nums = static_cast<unsigned>(ems.size());
      // How many type of inputs we have
      const unsigned input_types_num = static_cast<unsigned>((nodes)[ems.front()].inputs.size());
      // The mapping is one vector port for each input type:
      // if we had 2 EM with `fn(ptr a, int32)` we will have one vector port of size 2 for ptr and one vector port of
      // size 2 for int32
      for(unsigned i = 0; i < input_types_num; i++)
      {
         // TODO: handle more port types
         // We prefix PTR or INT32 to the port name as we have to recognize it later to attach contant input ports
         const auto input = (nodes)[ems.front()].inputs[i];
         const auto bit_size = std::get<0>(input) == TASK_INPUT_PTR ? ptr_size : 32U;
         const bool is_ptr = std::get<0>(input) == TASK_INPUT_PTR;
         name_str = (is_ptr ? ptr_prefix : const_int32_prefix) +
                    std::get<0>(rm_ports[static_cast<unsigned>(RM_PARAM_IN_PORT)]) + "_paramIn_" + std::to_string(i);
         task_port in_param_port = {
             name_str,
             port_type_name,
             bit_size,
#ifdef BASE_PTR_SHARED
             is_ptr ? 1U : em_nums,
             is_ptr ? false : std::get<1>(rm_ports[static_cast<unsigned>(RM_PARAM_IN_PORT)]) == port_vector_o_K,
#else
             em_nums,
             std::get<1>(rm_ports[static_cast<unsigned>(RM_PARAM_IN_PORT)]) == port_vector_o_K,
#endif
             std::get<2>(rm_ports[static_cast<unsigned>(RM_PARAM_IN_PORT)])};
         ports.push_back(in_param_port);
         resource_manager_name_dict[rm_name].second[static_cast<unsigned>(RM_PARAM_IN_PORT)].push_back(name_str);
      }

      for(unsigned i = 0; i < input_types_num; i++)
      {
         name_str = std::get<0>(rm_ports[static_cast<unsigned>(RM_PARAM_OUT_PORT)]) + "_paramOut_" + std::to_string(i);
         const auto bit_size = std::get<0>((nodes)[ems.front()].inputs[i]) == TASK_INPUT_PTR ? ptr_size : 32U;
         task_port out_param_port = {name_str,
                                     port_type_name,
                                     bit_size,
                                     1 + duplicates_size,
                                     std::get<1>(rm_ports[static_cast<unsigned>(RM_PARAM_OUT_PORT)]) == port_vector_o_K,
                                     std::get<2>(rm_ports[static_cast<unsigned>(RM_PARAM_OUT_PORT)])};
         ports.push_back(out_param_port);
         resource_manager_name_dict[rm_name].second[static_cast<unsigned>(RM_PARAM_OUT_PORT)].push_back(name_str);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "generating " + rm_name + " with " + STR(input_types_num) + " inputs");

      generate_generic_module(node_name, ports, {}, TASK_RESOURCE_MANAGER);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created lib RMs");
}

void OmpTaskFuBinding::write_execution_manager(structural_managerRef component, const std::vector<task_port>& ports,
                                               [[maybe_unused]] const std::vector<unsigned>& const_offsets)
{
   // Ports sequence follow the task_execution_port. Ports number are fixed as we employ port_vector_O for dynamic ports
   // ACPorts = 0,
   // ReqPort = 1,
   // DonePort = 2,
   // RMDonePort = 3,
   // BaseAddrPort = 4,
   // ParamOutPort = 5

   // Correctness check
   THROW_ASSERT(ports[static_cast<unsigned>(EM_BASE_ADDR_PORT)].size == const_offsets.size(),
                "ptr ports size and offset size does not match");
   std::string verilog_code = "";
   std::string verilog_component = "SIMPLEJOIN2_FU";

   verilog_code += "reg request_reg;\n";
   verilog_code += "wire request_signal;\n";
   verilog_code += "wire request_next;\n";

   verilog_code += "SIMPLEJOIN2_FU #(.BITSIZE_in1(" + STR(ports[0].size) + "))\n";
   verilog_code += "activating_conditions (.in1(" + ports[0].name + "),\n";
   verilog_code += ".clock(clock),\n";
   verilog_code += ".reset(reset),\n";
   verilog_code += ".out1(request_signal));\n\n";

   verilog_code += "always @(posedge clock)\n";
   verilog_code += "begin\n";
   verilog_code += "  if(1RESET_VALUE)\n";
   verilog_code += "  begin\n";
   verilog_code += "    request_reg <= 0;\n";
   verilog_code += "  end\n";
   verilog_code += "  else\n";
   verilog_code += "  begin\n";
   verilog_code += "    request_reg <= request_next;\n";
   verilog_code += "  end\n";
   verilog_code += "end\n\n";

   verilog_code += "assign request_next = (request_signal || request_reg) && !" + ports[3].name + ";\n";

   verilog_code += "assign " + ports[1].name + " = request_reg;\n";
#ifdef REMOVE_OUTLINED_GEP
   verilog_code += "assign " + ports[5].name + " = " + ports[4].name + ";\n";
#else
   THROW_ERROR("OpenMP task binding requires unshifted base pointers (REMOVE_OUTLINED_GEP)");
#endif
   verilog_code += "assign " + ports[2].name + " = {" + "PORTSIZE_" + ports[2].name + "{" + ports[3].name + "}};\n";

   component->add_NP_functionality(component->get_circ(), NP_functionality::VERILOG_PROVIDED, verilog_code);
   component->add_NP_functionality(component->get_circ(), NP_functionality::IP_COMPONENT, verilog_component);
   component->add_NP_functionality(component->get_circ(), NP_functionality::LIBRARY,
                                   ports[4].name + " " + ports[5].name + " " + ports[2].name);
}

void OmpTaskFuBinding::create_lib_execution_managers()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Creating lib EMs");

   const std::string port_type_name = "bool";
   for(const auto& n : nodes)
   {
      const auto num_a_cs = std::max(static_cast<unsigned>(n.second.in_degree), 1U);
      const auto node_name = em_name_prefix + n.first;
      execution_manager_name_dict[n.first].first = node_name;

      // Split ptr inputs from int32 inputs
      std::vector<std::tuple<task_input_kind, unsigned, unsigned, unsigned> const*> ptr_inputs;
      for(const auto& param : n.second.inputs)
      {
         if(std::get<0>(param) == TASK_INPUT_PTR)
         {
            ptr_inputs.push_back(&param);
         }
      }
      const auto input_size = static_cast<unsigned>(ptr_inputs.size());

      // Allocate the ports vector
      std::vector<task_port> ports;

      std::string name_str = std::get<0>(em_ports[static_cast<unsigned>(EM_ACPORTS)]);
      task_port in_ac_port = {name_str,
                              port_type_name,
                              1U,
                              num_a_cs,
                              (std::get<1>(em_ports[static_cast<unsigned>(EM_ACPORTS)]) == port_vector_o_K),
                              std::get<2>(em_ports[static_cast<unsigned>(EM_ACPORTS)])};
      ports.push_back(in_ac_port);
      execution_manager_name_dict[n.first].second[static_cast<unsigned>(EM_ACPORTS)] = name_str;

      name_str = std::get<0>(em_ports[static_cast<unsigned>(EM_REQ_PORT)]);
      task_port out_req_port = {name_str,
                                port_type_name,
                                1,
                                1,
                                (std::get<1>(em_ports[static_cast<unsigned>(EM_REQ_PORT)]) == port_vector_o_K),
                                std::get<2>(em_ports[static_cast<unsigned>(EM_REQ_PORT)])};
      ports.push_back(out_req_port);
      execution_manager_name_dict[n.first].second[static_cast<unsigned>(EM_REQ_PORT)] = name_str;

      // This signal has to be connected to all the childs
      name_str = std::get<0>(em_ports[static_cast<unsigned>(EM_DONE_PORT)]);
      task_port out_done_port = {name_str,
                                 port_type_name,
                                 1,
                                 n.second.out_degree,
                                 (std::get<1>(em_ports[static_cast<unsigned>(EM_DONE_PORT)]) == port_vector_o_K),
                                 std::get<2>(em_ports[static_cast<unsigned>(EM_DONE_PORT)])};
      ports.push_back(out_done_port);
      execution_manager_name_dict[n.first].second[static_cast<unsigned>(EM_DONE_PORT)] = name_str;

      name_str = std::get<0>(em_ports[static_cast<unsigned>(EM_RMDONE_PORT)]);
      task_port in_rm_done_port = {name_str,
                                   port_type_name,
                                   1,
                                   1,
                                   (std::get<1>(em_ports[static_cast<unsigned>(EM_RMDONE_PORT)]) == port_vector_o_K),
                                   std::get<2>(em_ports[static_cast<unsigned>(EM_RMDONE_PORT)])};
      ports.push_back(in_rm_done_port);
      execution_manager_name_dict[n.first].second[static_cast<unsigned>(EM_RMDONE_PORT)] = name_str;

      // After the fixed ports we allocate dynamic ports
      // Input base addr
      name_str = std::get<0>(em_ports[static_cast<unsigned>(EM_BASE_ADDR_PORT)]);
      task_port in_base_addr_port = {
          name_str,
          port_type_name,
          ptr_size,
          input_size,
          (std::get<1>(em_ports[static_cast<unsigned>(EM_BASE_ADDR_PORT)]) == port_vector_o_K),
          std::get<2>(em_ports[static_cast<unsigned>(EM_BASE_ADDR_PORT)])};
      ports.push_back(in_base_addr_port);
      execution_manager_name_dict[n.first].second[static_cast<unsigned>(EM_BASE_ADDR_PORT)] = name_str;

      // Output param addr
      name_str = std::get<0>(em_ports[static_cast<unsigned>(EM_PARAM_OUT_PORT)]);
      task_port out_param_port = {name_str,
                                  port_type_name,
                                  ptr_size,
                                  input_size,
                                  (std::get<1>(em_ports[static_cast<unsigned>(EM_PARAM_OUT_PORT)]) == port_vector_o_K),
                                  std::get<2>(em_ports[static_cast<unsigned>(EM_PARAM_OUT_PORT)])};
      ports.push_back(out_param_port);
      execution_manager_name_dict[n.first].second[static_cast<unsigned>(EM_PARAM_OUT_PORT)] = name_str;

      std::vector<unsigned> offsets;
      for(unsigned i = 0; i < ptr_inputs.size(); i++)
      {
#ifdef REMOVE_OUTLINED_GEP
         offsets.push_back(0);
#else
         offsets.push_back(std::get<2>(*(ptr_inputs[i])));
#endif
      }
      generate_generic_module(node_name, ports, offsets, TASK_EXECUTION_MANAGER);
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created lib EMs");
}

void OmpTaskFuBinding::perform_connection_one_to_one(
    const std::pair<structural_objectRef, structural_objectRef>& to_connect)
{
   const auto& src_port = std::get<0>(to_connect);
   const auto& dest_port = std::get<1>(to_connect);
   THROW_ASSERT((src_port->get_kind() == port_o_K || src_port->get_kind() == constant_o_K) &&
                    (dest_port->get_kind() == port_o_K || dest_port->get_kind() == constant_o_K),
                "not impl");
   add_smart_connection(src_port, dest_port, (*hls_params.unique_id)++, hls_params.circuit, hls_params.SM);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "connected " + src_port->get_path() + " with " + dest_port->get_path());
}

void OmpTaskFuBinding::perform_connection_one_to_many(
    const std::pair<structural_objectRef, std::vector<structural_objectRef>>& to_connect)
{
   const auto& src_port = std::get<0>(to_connect);
   const auto& dest_ports = std::get<1>(to_connect);
   for(const auto& d : dest_ports)
   {
      add_smart_connection(src_port, d, (*hls_params.unique_id)++, hls_params.circuit, hls_params.SM);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "connected " + src_port->get_path() + " with " + d->get_path());
   }
}

void OmpTaskFuBinding::generate_topo_graph()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Generating topological graph");
   // Populate the EM nodes
   generate_ems();

   // Populate the RM nodes
   generate_rms();

   // Create the final join module
   create_final_join();
   generate_final_join();

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "generating FU duplications");
   // Connect clock and reset to copied modules
   for(auto& [original, duplicates] : duplicated_fus)
   {
      for(auto& d : duplicates)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "handling duplicate " + d->get_id());
         structural_objectRef clock_port;
         structural_objectRef reset_port;
         THROW_ASSERT(try_find_port("clock", hls_params.owner_module, port_o_K, clock_port),
                      "can not find clock port in owner module_o");
         THROW_ASSERT(try_find_port("reset", hls_params.owner_module, port_o_K, reset_port),
                      "can not find clock port in owner module_o");

         auto to_connect = generate_resized_connection(CLOCK_PORT_NAME, CLOCK_PORT_NAME, hls_params.owner_module, d, 1U,
                                                       port_o_K, port_o_K, 1U, false, false);
         perform_connection_one_to_one(to_connect);
         to_connect = generate_resized_connection(RESET_PORT_NAME, RESET_PORT_NAME, hls_params.owner_module, d, 1U,
                                                  port_o_K, port_o_K, 1U, false, false);
         perform_connection_one_to_one(to_connect);
      }
   }

   // Connect const zero to Min ports of FUs
   for(auto& m : hls_params.lambda_modules)
   {
      for(auto& p : mem_in_ports)
      {
         auto p_type = std::get<1>(p);
         structural_objectRef port;
         if(!try_find_port(std::get<0>(p), m, p_type, port))
         {
            THROW_ERROR("expected to find " + STR(std::get<1>(p)) + " inside " + m->get_id());
         }
         const auto type_size = GetPointer<port_o>(port)->get_typeRef()->vector_size;
         structural_objectRef const_zero_obj = hls_params.SM->add_constant(
             "const_node_zero_" + m->get_id() + "_" + std::get<0>(p), hls_params.circuit,
             structural_type_descriptorRef(new structural_type_descriptor("bool", type_size)), "0");
         perform_connection_one_to_one({const_zero_obj, port});
      }
   }

   // Connect const params to RMs
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "connecting const inputs");
   for(auto& [rm, ems] : rm_allocation)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "rm: " + rm);
      const auto& parameter_ports = resource_manager_name_dict[rm].second[static_cast<unsigned>(RM_PARAM_IN_PORT)];
      std::vector<std::string const*> rm_const_ports;
      for(auto& p : parameter_ports)
      {
         if(p.find(const_int32_prefix) != std::string::npos)
         {
            rm_const_ports.push_back(&p);
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "collected rm const input param ports, size: " + STR(rm_const_ports.size()));
      std::vector<structural_objectRef> rm_const_ports_ref;
      for(auto& p_name : rm_const_ports)
      {
         structural_objectRef found_port;
         THROW_ASSERT(hls_params.rm_modules.find(rm) != hls_params.rm_modules.end(), "can not rm module_o");
         [[maybe_unused]] const auto found =
             try_find_port(*p_name, hls_params.rm_modules[rm], port_vector_o_K, found_port);
         THROW_ASSERT(found, "can not find port");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "found const input port in RM " + found_port->get_path() + " for " + *p_name);
         rm_const_ports_ref.push_back(found_port);
      }

      unsigned em_index = 0;
      for(auto& em : ems)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "em: " + em);
         const auto& constant_inputs = em_const_inputs[em];
         THROW_ASSERT(constant_inputs.size() == rm_const_ports_ref.size(),
                      "emConstInputs.size() == rmConstPortsRef.size()");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "const input size: " + STR(constant_inputs.size()));

         unsigned i = 0;
         for(auto& em_const_input : constant_inputs)
         {
            const auto& port = em_const_input.second;
            auto& dest_port = GetPointer<port_o>(rm_const_ports_ref[i])->get_port(em_index);

            perform_connection_one_to_one({port, dest_port});
            i++;
         }
         em_index++;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "connected const inputs");

   // Generate edges between EMs
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "generating EM - EM connections");
   CustomUnorderedMap<std::string, unsigned> input_ac_index_map;
   for(const auto& [src, destinations] : adjency_list)
   {
      THROW_ASSERT(hls_params.em_modules.find(src) != hls_params.em_modules.end(),
                   "can not find " + src + " inside EM modules");
      const auto& src_module = hls_params.em_modules[src];

      // Check sizes
      const auto src_module_out_done_port =
          GetPointer<module_o>(src_module)
              ->find_member(std::get<0>(em_ports[static_cast<unsigned>(EM_DONE_PORT)]),
                            std::get<1>(em_ports[static_cast<unsigned>(EM_DONE_PORT)]), src_module);
      [[maybe_unused]] const auto src_module_out_done_port_ptr = GetPointer<port_o>(src_module_out_done_port);
      THROW_ASSERT(src_module_out_done_port_ptr->get_kind() == port_vector_o_K, "should be a port vector");
      THROW_ASSERT((src_module_out_done_port_ptr->get_ports_size() == 1UL && destinations.size() == 0UL) ||
                       (src_module_out_done_port_ptr->get_ports_size() == destinations.size()),
                   "size should be equal: " + STR(src_module_out_done_port_ptr->get_ports_size()) + " vs " +
                       STR(destinations.size()));

      unsigned src_module_out_index = 0;
      for(const auto& dst : destinations)
      {
         THROW_ASSERT(hls_params.em_modules.find(dst) != hls_params.em_modules.end(),
                      "can not find " + dst + " inside EM modules");
         const auto& dst_module = hls_params.em_modules[dst];
         for(const auto& conn : execution_manager_to_execution_manager)
         {
            auto& conn_src = conn.src;
            auto& conn_dst = conn.dest;
            const auto& source_port_id = execution_manager_name_dict[src].second[std::get<2>(conn_src)];
            const auto& dest_port_id = execution_manager_name_dict[dst].second[std::get<2>(conn_dst)];

            THROW_ASSERT(source_port_id.find(std::get<0>(conn_src)) != std::string::npos,
                         "expected " + std::get<0>(conn_src) + " inside " + source_port_id);
            THROW_ASSERT(dest_port_id.find(std::get<0>(conn_dst)) != std::string::npos,
                         "expected " + std::get<0>(conn_dst) + " inside " + source_port_id);

            const auto to_connect =
                generate_resized_connection(source_port_id, dest_port_id, src_module, dst_module, 1U,
                                            std::get<1>(conn_src), std::get<1>(conn_dst), 1U, false, false);
            const auto name = to_connect.second->get_path();
            const auto index = input_ac_index_map[name];
            input_ac_index_map[name]++;
            const auto from_port = GetPointer<port_o>(to_connect.first)->get_port(src_module_out_index++);
            const auto to_port = GetPointer<port_o>(to_connect.second)->get_port(index);
            // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
            //               "connecting " + toConnect.first->get_path() + " and " + toPort->get_path());

            perform_connection_one_to_one({from_port, to_port});
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "generated EM - EM connections");

   // Generate edges between EM and RM
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "generating EM - RM connections (both ways)");
#ifdef BASE_PTR_SHARED
   CustomUnorderedSet<std::string> connected_rm_param_in_ports;
#endif
   for(const auto& [rm_id, ems] : rm_allocation)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "RM: " + rm_id);
      unsigned em_index = 0;
      for(const auto& em_id : ems)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "EM: " + em_id);
         // EM to RM
         unsigned connection_index = 0;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "EM to RM");
         for(const auto& conn : execution_manager_to_resource_manager)
         {
            THROW_ASSERT(hls_params.em_modules.find(em_id) != hls_params.em_modules.end(), "can not find " + em_id);
            THROW_ASSERT(hls_params.rm_modules.find(rm_id) != hls_params.rm_modules.end(), "can not find " + rm_id);
            const auto& src_module = hls_params.em_modules[em_id];
            const auto& dst_module = hls_params.rm_modules[rm_id];
            auto& conn_src = conn.src;
            auto& conn_dst = conn.dest;
            const auto& source_port_id = execution_manager_name_dict[em_id].second[std::get<2>(conn_src)];

            // Some ports are direct links, others have to be connected with the vectorized logic
            if(connection_index == 0)
            {
               const auto& dest_port_id = resource_manager_name_dict[rm_id].second[std::get<2>(conn_dst)][0];
               const auto to_connect =
                   generate_resized_connection(source_port_id, dest_port_id, src_module, dst_module, 1U,
                                               std::get<1>(conn_src), std::get<1>(conn_dst), 1U, false, false);
               perform_connection_one_to_one(
                   {to_connect.first, GetPointer<port_o>(to_connect.second)->get_port(em_index)});
            }
            else
            {
               const auto source_port =
                   collect_port(src_module, source_port_id, 1U, std::get<1>(conn_src), 1U, false, false);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "collected EM source param out port " + source_port->get_path());
               std::vector<std::string const*> dest_port_ids;
               // Filter out ptr param ports
               const auto& rm_input_ports = resource_manager_name_dict[rm_id].second[std::get<2>(conn_dst)];
               for(auto& rm_input_port : rm_input_ports)
               {
                  if(rm_input_port.find(ptr_prefix) != std::string::npos)
                  {
                     dest_port_ids.push_back(&rm_input_port);
                  }
               }
               std::vector<structural_objectRef> dest_ports;
               for(const auto& dest_port_id : dest_port_ids)
               {
                  dest_ports.push_back(
                      collect_port(dst_module, *dest_port_id, 1U, std::get<1>(conn.dest), 1U, false, false));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "collected RM dest param in port " + dest_ports.back()->get_path());
               }
               // In this case we don't have to perform a naive connection as we have to link the ith param out
               // port_vector from EM to the ith port_vector of RM in the index EMth

               // Get vector size
               port_o* src_port_ptr = GetPointer<port_o>(source_port);
               unsigned vector_from_size = 1U;
               if(src_port_ptr->get_kind() == port_o_K)
               {
                  THROW_UNREACHABLE("should not be here");
               }
               else if(src_port_ptr->get_kind() == port_vector_o_K)
               {
                  vector_from_size = src_port_ptr->get_ports_size(); // = number of parameters
                  THROW_ASSERT(vector_from_size == dest_ports.size(), "expected the same size");

                  for(unsigned port_index = 0; port_index < vector_from_size; port_index++)
                  {
#ifdef BASE_PTR_SHARED
                     auto source_ith_port = src_port_ptr->get_port(port_index);
                     auto dest_port_ptr = GetPointer<port_o>(dest_ports[port_index]);
                     THROW_ASSERT(dest_port_ptr->get_kind() == port_o_K, "expected a single port");
                     if(connected_rm_param_in_ports.count(dest_port_ptr->get_path()))
                     {
                        // The base pointer is shared by these execution managers.
                        // Only the first one drives the resource manager; keep the
                        // redundant output explicitly connected to a local signal.
                        if(!GetPointerS<port_o>(source_ith_port)->find_bounded_object())
                        {
                           const auto unused =
                               hls_params.SM->add_sign("unused_param_" + STR((*hls_params.unique_id)++),
                                                       hls_params.circuit, source_ith_port->get_typeRef());
                           hls_params.SM->add_connection(source_ith_port, unused);
                        }
                        continue;
                     }
                     connected_rm_param_in_ports.insert(dest_port_ptr->get_path());
                     perform_connection_one_to_one({source_ith_port, dest_ports[port_index]});
#else
                     auto source_ith_port = src_port_ptr->get_port(port_index);
                     auto dest_port_ptr = GetPointer<port_o>(dest_ports[port_index]);
                     THROW_ASSERT(dest_port_ptr->get_kind() == port_vector_o_K, "expected a vector port");
                     auto dest_ith_port = dest_port_ptr->get_port(em_index);
                     perform_connection_one_to_one({source_ith_port, dest_ith_port});
#endif
                  }
               }
            }
            connection_index++;
         }

         // RM to EM
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "RM to EM");
         for(const auto& conn : resource_manager_to_execution_manager)
         {
            THROW_ASSERT(hls_params.em_modules.find(em_id) != hls_params.em_modules.end(), "can not find " + em_id);
            THROW_ASSERT(hls_params.rm_modules.find(rm_id) != hls_params.rm_modules.end(), "can not find " + rm_id);
            const auto& src_module = hls_params.rm_modules[rm_id];
            const auto& dst_module = hls_params.em_modules[em_id];
            auto& conn_src = conn.src;
            auto& conn_dst = conn.dest;
            const auto& source_port_id = resource_manager_name_dict[rm_id].second[std::get<2>(conn_src)][0];
            const auto& dest_port_id = execution_manager_name_dict[em_id].second[std::get<2>(conn_dst)];
            const auto to_connect =
                generate_resized_connection(source_port_id, dest_port_id, src_module, dst_module, 1U,
                                            std::get<1>(conn_src), std::get<1>(conn_dst), 1U, false, false);
            perform_connection_one_to_one(
                {GetPointer<port_o>(to_connect.first)->get_port(em_index), to_connect.second});
         }

         em_index++;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "generated EM - RM connections (both ways)");

   // Generate edges between RM and FU
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "generating RM - FU connections (both ways)");
   for(const auto& [rm_id, ems] : rm_allocation)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "RM: " + rm_id);

      THROW_ASSERT(hls_params.rm_modules.find(rm_id) != hls_params.rm_modules.end(),
                   "rm module_o not found in hlsParams.rmModules");
      const auto& rm_module = hls_params.rm_modules[rm_id];
      THROW_ASSERT(task_entry_name_bambu_to_original_dict.find(rm_id) != task_entry_name_bambu_to_original_dict.end(),
                   "can not find key");
      THROW_ASSERT(hls_params.lambda_modules_map.find(task_entry_name_bambu_to_original_dict[rm_id]) !=
                       hls_params.lambda_modules_map.end(),
                   "lambda module_o not found in hlsParams.lambdaModulesMap");
      const auto& lambda_module = hls_params.lambda_modules_map[task_entry_name_bambu_to_original_dict[rm_id]];
      const auto lambda_module_id = lambda_module->get_typeRef()->id_type;
      THROW_ASSERT(duplicated_fus.count(lambda_module_id), "expected " + lambda_module_id + " inside _duplicatedFUs");
      auto lambda_modules = duplicated_fus[lambda_module_id];
      lambda_modules.insert(lambda_modules.begin(), lambda_module);

      // RM to FU
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "RM to FU");
      for(const auto& conn : resource_manager_to_lambda_modules)
      {
         if(conn.first == 1U)
         {
            //  Fixed num of connections
            unsigned module_count = 0;
            for(auto& m : lambda_modules)
            {
               auto& conn_src = conn.second.src;
               // This is fixed
               auto& conn_dst = conn.second.dest;
               const auto& source_port_id =
                   resource_manager_name_dict[rm_id].second[static_cast<unsigned>(RM_FU_REQ_PORT)][0];
               THROW_ASSERT(source_port_id.find(conn_src.first) != std::string::npos,
                            "expected " + conn_src.first + " inside " + source_port_id);
               const auto to_connect = generate_resized_connection(source_port_id, conn_dst.first, rm_module, m, 1U,
                                                                   conn_src.second, conn_dst.second, 1U, false, false);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "Cached connection between " + conn_src.first + " and " + conn_dst.first);

               perform_connection_one_to_one(
                   {GetPointer<port_o>(to_connect.first)->get_port(module_count++), to_connect.second});
            }
         }
         else if(conn.first == UNKNOWN_NUM_SIZE)
         {
            unsigned module_count = 0;
            for(auto& m : lambda_modules)
            {
               // Dynamic num of connections
               const auto& params_vec =
                   resource_manager_name_dict[rm_id].second[static_cast<unsigned>(RM_PARAM_OUT_PORT)];
               const auto& dest_params_in = hls_params.lambda_params_ports[m->get_id()];
               THROW_ASSERT(params_vec.size() == dest_params_in.size(),
                            "port size does not match. RM has " + STR(params_vec.size()) + " output ports and FU has " +
                                STR(dest_params_in.size()) + " ports");

               // Each param has a single port_o as they may differ in bitsize
               for(unsigned i = 0; i < params_vec.size(); ++i)
               {
                  const auto& source_port_id = params_vec[i];
                  const auto& dest_port_id = dest_params_in[i];
                  const auto to_connect =
                      generate_resized_connection(source_port_id, dest_port_id, rm_module, m, 1U,
                                                  conn.second.src.second, conn.second.dest.second, 1U, false, false);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "Cached connection between " + source_port_id + " and " + dest_port_id);

                  perform_connection_one_to_one(
                      {GetPointer<port_o>(to_connect.first)->get_port(module_count), to_connect.second});
               }
               module_count++;
            }
         }
      }

      // FU to RM
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "FU to RM");
      for(const auto& conn : lambda_to_resource_manager)
      {
         unsigned module_count = 0;
         for(auto& m : lambda_modules)
         {
            auto& conn_src = conn.src;
            auto& conn_dst = conn.dest;
            // This will be start or end port
            const auto& source_port_id = std::get<0>(conn_src);
            const auto& dest_port_id = resource_manager_name_dict[rm_id].second[std::get<2>(conn_dst)][0];
            THROW_ASSERT(dest_port_id.find(std::get<0>(conn_dst)) != std::string::npos, "dest port id not found");
            const auto to_connect =
                generate_resized_connection(source_port_id, dest_port_id, m, rm_module, 1U, std::get<1>(conn_src),
                                            std::get<1>(conn_dst), 1U, false, false);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Cached connection between " + source_port_id + " and " + dest_port_id);

            perform_connection_one_to_one(
                {to_connect.first, GetPointer<port_o>(to_connect.second)->get_port(module_count++)});
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "generated RM - FU connections (both ways)");

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "connecting param in ports to EM modules");
   const auto module_ptr = GetPointer<module_o>(hls_params.owner_module);
   std::map<unsigned long, structural_objectRef> input_ports;
   const std::string input_port_name_prefix = "in_port_P";
   for(unsigned i = 0; i < module_ptr->get_in_port_size(); i++)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "datapath input port " + module_ptr->get_in_port(i)->get_path());
      if(module_ptr->get_in_port(i)->get_path().find(input_port_name_prefix) != std::string::npos)
      {
         const std::string string_id = module_ptr->get_in_port(i)->get_id().substr(input_port_name_prefix.size());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "port id" + string_id);
         unsigned long id = std::stoul(string_id);
         input_ports[id] = module_ptr->get_in_port(i);
      }
   }
   for([[maybe_unused]] auto& [id, p] : input_ports)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "collected input port from datapath " + STR(id) + " " + p->get_id());
   }

   for(auto& em : hls_params.em_modules)
   {
      auto em_module_ptr = GetPointer<module_o>(em.second);
      std::vector<structural_objectRef> em_module_input_addr_ports;
      for(unsigned i = 0; i < em_module_ptr->get_in_port_size(); i++)
      {
         const auto current_port = em_module_ptr->get_in_port(i);
         if(current_port->get_path().find(std::get<0>(em_ports[static_cast<unsigned>(EM_BASE_ADDR_PORT)])) !=
            std::string::npos)
         {
            const unsigned long size = (GetPointer<port_o>(current_port)->get_ports_size());
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "recovered base addr param in ports for " + current_port->get_path() + " with size " +
                               STR(size));

            for(unsigned port_id = 0; port_id < size; port_id++)
            {
               THROW_ASSERT(std::get<0>(nodes[em.first].inputs[port_id]) == TASK_INPUT_PTR, "expected a pointer input");
               const auto label_id = std::get<1>(nodes[em.first].inputs[port_id]);
               THROW_ASSERT(top_module_inputs_map.find(label_id) != top_module_inputs_map.end(),
                            "can not find " + STR(label_id) + " inside the mapping");
               const auto index = top_module_inputs_map[label_id];
               THROW_ASSERT(input_ports.find(index) != input_ports.end(),
                            "can not find port with index " + STR(index) +
                                " inside the top module_o input ports mapping");
               const auto ith_port = GetPointer<port_o>(current_port)->get_port(port_id);
               perform_connection_one_to_one({input_ports[index], ith_port});
            }
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "connected param in ports to EM modules");

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "connecting EM out to done port");
   auto final_join_input_port =
       GetPointer<module_o>(hls_params.final_join)
           ->find_member("join_in", closing_nodes > 1 ? port_vector_o_K : port_o_K, hls_params.final_join);
   THROW_ASSERT(final_join_input_port, "should not be null");
   unsigned join_index = 0;
   const auto final_join_input_port_ptr = GetPointer<port_o>(final_join_input_port);
   for(auto& em : hls_params.em_modules)
   {
      const auto successors = adjency_list[em.first].size();
      if(successors == 0)
      {
         auto em_done_port = GetPointer<module_o>(em.second)->find_member(
             std::get<0>(em_ports[static_cast<unsigned>(EM_DONE_PORT)]),
             std::get<1>(em_ports[static_cast<unsigned>(EM_DONE_PORT)]), em.second);
         THROW_ASSERT(em_done_port, "should not be null");
         const auto em_done_port_ptr = GetPointer<port_o>(em_done_port);
         THROW_ASSERT(em_done_port_ptr, "should not be null");
         THROW_ASSERT(final_join_input_port_ptr->get_ports_size() > join_index, "out of bounds");
         THROW_ASSERT(em_done_port_ptr->get_ports_size() == 1U, "should have only one out port");
         perform_connection_one_to_one(
             {em_done_port_ptr->get_port(0), final_join_input_port_ptr->get_port(join_index++)});
      }
   }
   const auto final_join_output_port =
       GetPointer<module_o>(hls_params.final_join)->find_member("out_done", port_o_K, hls_params.final_join);
   THROW_ASSERT(final_join_output_port, "should not be null");
   const auto owner_module_done_port =
       GetPointer<module_o>(hls_params.owner_module)->find_member(DONE_PORT_NAME, port_o_K, hls_params.owner_module);
   THROW_ASSERT(owner_module_done_port, "should not be null");
   perform_connection_one_to_one({final_join_output_port, owner_module_done_port});
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "connected EM out to done port");

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "connecting start port to initial EMs");
   const auto module_start_port =
       GetPointer<module_o>(hls_params.owner_module)->find_member(START_PORT_NAME, port_o_K, hls_params.owner_module);
   THROW_ASSERT(module_start_port, "module_o must have start_port");
   for(auto& n : nodes)
   {
      const auto in_degree = n.second.in_degree;
      if(in_degree == 0)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "starting EM " + n.first);
         THROW_ASSERT(hls_params.em_modules.find(n.first) != hls_params.em_modules.end(),
                      "cannot find " + n.first + " EM");
         const auto em = hls_params.em_modules[n.first];
         const auto em_ac_port = GetPointer<module_o>(em)->find_member(
             std::get<0>(em_ports[static_cast<unsigned>(EM_ACPORTS)]), port_o_K, em);
         THROW_ASSERT(em_ac_port, "EM module_o must have start_port");
         const auto em_ac_port_ptr = GetPointer<port_o>(em_ac_port);
         THROW_ASSERT(em_ac_port_ptr->get_ports_size() == 1U,
                      "initial EMs activation condition ports should be of size 1");
         perform_connection_one_to_one({module_start_port, em_ac_port_ptr->get_port(0)});
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "connected start port to initial EMs");
}

void OmpTaskFuBinding::generate_ems()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Generating EMs");
   for(const auto& n : nodes)
   {
      const auto& name = execution_manager_name_dict[n.first].first;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, name);
      auto module_obj = add_module(name, name);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Generated " + name);
      hls_params.em_modules[n.first] = module_obj;
   }
}

void OmpTaskFuBinding::generate_rms()
{
   for(const auto& [rm, ems] : rm_allocation)
   {
      const auto& name = resource_manager_name_dict[rm].first;
      auto module_obj = add_module(name, name);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Generated " + name);
      hls_params.rm_modules[rm] = module_obj;
   }
}

void OmpTaskFuBinding::generate_final_join()
{
   auto module_obj = add_module(OmpTaskFuBinding::final_join_name, OmpTaskFuBinding::final_join_name);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Generated " + OmpTaskFuBinding::final_join_name);
   hls_params.final_join = module_obj;
}

void OmpTaskFuBinding::manage_memory_ports_parallel_chained(const HLS_managerRef HLSMgr, const structural_managerRef SM,
                                                            const std::list<structural_objectRef>& memory_modules,
                                                            const structural_objectRef circuit, const hlsRef HLS,
                                                            unsigned int& unique_id)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "OpenMP task fu binding");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Input modules size: " + STR(memory_modules.size()));
   for([[maybe_unused]] auto& m : memory_modules)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "module_o id " + m->get_id() + " type: " + m->get_typeRef()->id_type);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Unique id " + STR(unique_id));

   // Save internal refs
   hls_params.HLSMgr = HLSMgr;
   hls_params.SM = SM;
   hls_params.circuit = circuit;
   hls_params.modules = std::vector<structural_objectRef>(memory_modules.begin(), memory_modules.end());
   hls_params.HLS = HLS;
   hls_params.unique_id = &unique_id;
   hls_params.mem_channel_num = hls_params.HLSMgr->CGetFunctionBehavior(HLS->functionId)->GetChannelsNumber() > 0 ?
                                    hls_params.HLSMgr->CGetFunctionBehavior(HLS->functionId)->GetChannelsNumber() :
                                    1U;
   hls_params.use_banked_mem = false;

   // Check if banked memory is requested
   const auto top_symbols = parameters->getOption<std::vector<std::string>>(OPT_top_functions_names);
   THROW_ASSERT(top_symbols.size() == 1, "Expected single top function name");
   const auto TM = hls_params.HLSMgr->get_ir_manager();
   const auto top_fn_node = TM->GetFunction(top_symbols.front());
   const auto top_fb = hls_params.HLSMgr->CGetFunctionBehavior(top_fn_node->index);
   const auto top_bh = top_fb->CGetBehavioralHelper();
   const auto top_fn_name = top_bh->GetFunctionName();
   const auto func_arch = hls_params.HLSMgr->module_arch->GetArchitecture(top_fn_name);
   // Use the banked memory if the top function uses the banked interface.
   if(func_arch->ifaces.find("bus") != func_arch->ifaces.end())
   {
      const auto& bus_interface = func_arch->ifaces.at("bus");
      if(bus_interface.find(FunctionArchitecture::iface_bank_number) != bus_interface.end())
      {
         hls_params.use_banked_mem =
             std::stoul((bus_interface.find(FunctionArchitecture::iface_bank_number)->second)) > 0U;
      }
   }
   if(!hls_params.use_banked_mem)
   {
      THROW_ASSERT(hls_params.mem_channel_num <= 2, "not impl");
   }

   const auto file_name =
       parameters->getOption<std::string>(OPT_output_temporary_directory) + "/" + openmp_task::GraphFilename;
   parse_xml(file_name);

   // Meory modules should contains omp lambda entry functionsUnique id
   const auto owner_module = hls_params.modules.front()->get_owner();
   hls_params.owner_module = owner_module;

   parse_lambda_modules();

   // We expect --channel-number flag to represent the total FU count if banked memory is being used
   if(hls_params.use_banked_mem)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Module configured with banked memory");
      THROW_ASSERT(hls_params.mem_channel_num == static_cast<unsigned>(hls_params.lambda_modules.size()),
                   "Invalid configuration. Channels number should be equal to the FU count");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "Module configured with " + STR(hls_params.mem_channel_num) + " channels");

   // Internalize ports of lambda fns as we have to arbiter them
   for(const auto& m : hls_params.lambda_modules)
   {
      internalize_ports(ports_to_internalize, m);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Internalized modules ports");

   // Add M ports to owner module. We take a random lambda as they share the M ports
   THROW_ASSERT(hls_params.lambda_modules.size() > 0, "no FUs");
   forward_ports(m_ports, hls_params.lambda_modules.front(), owner_module,
                 hls_params.mem_channel_num > 1U ? port_vector_o_K : port_o_K, hls_params.mem_channel_num, true);
   forward_ports(start_done_ports, hls_params.lambda_modules.front(), owner_module, port_o_K, 1U, false);

   generate_mem_interface();

   allocate_em_to_rm();

   create_lib_execution_managers();

   create_lib_resource_managers();

   generate_const_rm_inputs();

   generate_topo_graph();
}
