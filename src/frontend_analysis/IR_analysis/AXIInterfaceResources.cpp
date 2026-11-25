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
 *   Copyright (C) 2022-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file AXIInterfaceResources.cpp
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#include "AXIInterfaceResources.hpp"

#include "application_manager.hpp"
#include "area_info.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "copyrights_strings.hpp"
#include "dbgPrintHelper.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "library_manager.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "time_info.hpp"
#include "utility/string_manipulation.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace InterfaceInferInternal
{
   constexpr double EPSILON = 0.000000001;

   namespace
   {
      [[maybe_unused]] void register_duplex_m_axi_resources(
          const technology_managerRef& TechMan, const ParameterConstRef& parameters, const std::string& bundle_name,
          const unsigned int address_bitsize, const unsigned int data_bitsize, const unsigned int backend_data_bitsize,
          const std::uint32_t axi_latency, const unsigned int read_max_burst_length,
          const AXIReadBurstProfile& read_profile, const AXIWriteBurstProfile& write_profile)
      {
         if((address_bitsize != 32U && address_bitsize != 64U) || data_bitsize != 32U || backend_data_bitsize != 32U)
         {
            THROW_ERROR_USAGE("Duplex m_axi bundle '" + bundle_name +
                              "' requires 32 or 64 address bits and 32-bit interface/backend data widths.");
         }
         const auto read_name = ENCODE_FDNAME(bundle_name, "_duplex_read_endpoint", "");
         const auto write_name = ENCODE_FDNAME(bundle_name, "_duplex_write_endpoint", "");
         const auto aggregator_name = ENCODE_FDNAME(bundle_name, "_duplex_aggregator", "");
         const std::map<std::string, std::string> metadata{
             {"bambu_axi_bundle_name", bundle_name},
             {"bambu_axi_duplex_aggregator", aggregator_name},
             {"bambu_axi_address_bitsize", std::to_string(address_bitsize)},
             {"bambu_axi_data_bitsize", std::to_string(data_bitsize)},
             {"bambu_axi_latency", std::to_string(axi_latency)},
             {"bambu_max_read_burst_length", std::to_string(read_max_burst_length)},
             {"bambu_num_read_outstanding", std::to_string(read_profile.max_outstanding)},
             {"bambu_read_fifo_depth", std::to_string(read_profile.fifo_depth)},
             {"bambu_max_write_burst_length", std::to_string(write_profile.max_burst_length)},
             {"bambu_write_burst_outstanding", std::to_string(write_profile.max_outstanding)},
             {"bambu_write_fifo_depth", std::to_string(write_profile.fifo_depth)}};
         const auto read_mode = std::string("read");
         const auto write_mode = std::string("write");
         const auto duplex_mode = std::string("duplex");
         const auto register_resource = [&](const std::string& resource_name, const std::string& role,
                                            const std::string& mode, const bool endpoint) {
            const auto library_exists = TechMan->is_library_manager(INTERFACE_LIBRARY);
            const auto resource_exists =
                library_exists && TechMan->get_library_manager(INTERFACE_LIBRARY)->is_fu(resource_name);
            const auto expected_attributes = [&]() {
               auto attributes = metadata;
               attributes["bambu_axi_duplex_role"] = role;
               attributes["bambu_axi_burst_mode"] = mode;
               return attributes;
            }();
            if(resource_exists)
            {
               const auto fu = GetPointerS<functional_unit>(TechMan->get_fu(resource_name, INTERFACE_LIBRARY));
               const auto module = fu->CM ? GetPointer<module_o>(fu->CM->get_circ()) : nullptr;
               for(const auto& metadata_entry : expected_attributes)
               {
                  const auto found = fu->attributes.find(metadata_entry.first);
                  const auto module_has_attribute =
                      module && std::find(module->get_attribute_list().begin(), module->get_attribute_list().end(),
                                          metadata_entry.first) != module->get_attribute_list().end();
                  const auto module_attribute =
                      module_has_attribute ? module->get_attribute(metadata_entry.first) : attributeRef();
                  if(found == fu->attributes.end() || !found->second ||
                     found->second->get_content_str() != metadata_entry.second || !module_attribute ||
                     module_attribute->get_content_str() != metadata_entry.second)
                  {
                     THROW_ERROR_USAGE("Incompatible duplex m_axi resource metadata for bundle '" + bundle_name +
                                       "' at '" + resource_name + "'.");
                  }
               }
               const auto np = module ? module->get_NP_functionality() : NP_functionalityRef();
               if(!np ||
                  (endpoint && (!np->exist_NP_functionality(NP_functionality::VERILOG_PROVIDED) ||
                                np->exist_NP_functionality(NP_functionality::VERILOG_GENERATOR) ||
                                np->exist_NP_functionality(NP_functionality::VHDL_GENERATOR))) ||
                  (!endpoint &&
                   (!np->exist_NP_functionality(NP_functionality::VERILOG_GENERATOR) ||
                    np->get_NP_functionality(NP_functionality::VERILOG_GENERATOR) != "DuplexMAXIHDLGenerator")))
               {
                  THROW_ERROR_USAGE("Incompatible duplex m_axi resource HDL functionality for bundle '" + bundle_name +
                                    "' at '" + resource_name + "'.");
               }
               if(endpoint)
               {
                  const auto top = fu->CM->get_circ();
                  const auto check_port = [&](const std::string& name, const port_o::port_direction direction,
                                              const unsigned int width, const bool is_extern) {
                     const auto port = top->find_member(name, port_o_K, top);
                     if(!port || GetPointerS<port_o>(port)->get_port_direction() != direction ||
                        GET_TYPE_SIZE(GetPointerS<port_o>(port)) != width ||
                        GetPointerS<port_o>(port)->get_is_extern() != is_extern)
                     {
                        THROW_ERROR_USAGE("Incompatible duplex m_axi endpoint port '" + name + "' for bundle '" +
                                          bundle_name + "'.");
                     }
                  };
                  check_port("in1", port_o::IN, 2, false);
                  check_port("in2", port_o::IN, 32, false);
                  check_port("in3", port_o::IN, data_bitsize, false);
                  check_port("in4", port_o::IN, address_bitsize, false);
                  check_port("in5", port_o::IN, 32, false);
                  check_port(CLOCK_PORT_NAME, port_o::IN, 1, false);
                  check_port(RESET_PORT_NAME, port_o::IN, 1, false);
                  check_port(DONE_PORT_NAME, port_o::OUT, 1, false);
                  check_port("out1", port_o::OUT, data_bitsize, false);
                  check_port("duplex_start", port_o::OUT, 1, true);
                  check_port("duplex_opcode", port_o::OUT, 2, true);
                  check_port("duplex_size", port_o::OUT, 32, true);
                  check_port("duplex_data", port_o::OUT, data_bitsize, true);
                  check_port("duplex_address", port_o::OUT, address_bitsize, true);
                  check_port("duplex_count", port_o::OUT, 32, true);
                  check_port("duplex_done", port_o::IN, 1, true);
                  check_port("duplex_result", port_o::IN, data_bitsize, true);
                  const auto start_port = top->find_member(START_PORT_NAME, port_o_K, top);
                  if(!start_port || GetPointerS<port_o>(start_port)->get_port_direction() != port_o::IN ||
                     GetPointerS<port_o>(start_port)->get_ports_size() != 1 ||
                     GetPointerS<port_o>(start_port)->get_is_extern())
                  {
                     THROW_ERROR_USAGE("Incompatible duplex m_axi start port for bundle '" + bundle_name + "'.");
                  }
               }
               return;
            }

            const structural_managerRef CM(new structural_manager(parameters));
            const structural_type_descriptorRef module_type(new structural_type_descriptor(resource_name));
            CM->set_top_info(resource_name, module_type);
            const auto top = CM->get_circ();
            const auto module = GetPointerS<module_o>(top);
            module->set_description("Duplex m_axi resource for bundle: " + bundle_name);
            module->set_copyright(GENERATED_COPYRIGHT);
            module->set_authors("Component automatically generated by bambu");
            module->set_license(GENERATED_LICENSE);
            module->set_multi_unit_multiplicity(1U);

            const auto add_attribute = [&](const std::string& name, const std::string& value) {
               const auto numeric = name == "bambu_axi_address_bitsize" || name == "bambu_axi_data_bitsize" ||
                                    name == "bambu_max_read_burst_length" || name == "bambu_num_read_outstanding" ||
                                    name == "bambu_read_fifo_depth" || name == "bambu_max_write_burst_length" ||
                                    name == "bambu_write_burst_outstanding" || name == "bambu_write_fifo_depth";
               const attributeRef attr(new attribute(numeric ? attribute::INT32 : attribute::STRING, value));
               module->add_attribute(name, attr);
            };
            for(const auto& metadata_entry : expected_attributes)
            {
               add_attribute(metadata_entry.first, metadata_entry.second);
            }

            const structural_type_descriptorRef bool_type(new structural_type_descriptor("bool", 0));
            if(endpoint)
            {
               const structural_type_descriptorRef opcode_type(new structural_type_descriptor("bool", 2));
               const structural_type_descriptorRef size_type(new structural_type_descriptor("bool", 32));
               const structural_type_descriptorRef data_type(new structural_type_descriptor("bool", data_bitsize));
               const structural_type_descriptorRef address_type(
                   new structural_type_descriptor("bool", address_bitsize));
               CM->add_port(CLOCK_PORT_NAME, port_o::IN, top, bool_type);
               CM->add_port(RESET_PORT_NAME, port_o::IN, top, bool_type);
               CM->add_port_vector(START_PORT_NAME, port_o::IN, 1U, top, bool_type);
               CM->add_port("in1", port_o::IN, top, opcode_type);
               CM->add_port("in2", port_o::IN, top, size_type);
               CM->add_port("in3", port_o::IN, top, data_type);
               CM->add_port("in4", port_o::IN, top, address_type);
               CM->add_port("in5", port_o::IN, top, size_type);
               CM->add_port(DONE_PORT_NAME, port_o::OUT, top, bool_type);
               CM->add_port("out1", port_o::OUT, top, data_type);
               const auto private_port = [&](const std::string& name, const port_o::port_direction direction,
                                             const unsigned int width) {
                  const auto port =
                      CM->add_port(name, direction, top,
                                   structural_type_descriptorRef(new structural_type_descriptor("bool", width)));
                  GetPointerS<port_o>(port)->set_is_extern(true);
               };
               private_port("duplex_start", port_o::OUT, 1);
               private_port("duplex_opcode", port_o::OUT, 2);
               private_port("duplex_size", port_o::OUT, 32);
               private_port("duplex_data", port_o::OUT, data_bitsize);
               private_port("duplex_address", port_o::OUT, address_bitsize);
               private_port("duplex_count", port_o::OUT, 32);
               private_port("duplex_done", port_o::IN, 1);
               private_port("duplex_result", port_o::IN, data_bitsize);
               const auto assert_port_width = [&](const std::string& name, const unsigned int ASSERT_PARAMETER(width)) {
                  const auto port = top->find_member(name, port_o_K, top);
                  THROW_ASSERT(port && GET_TYPE_SIZE(GetPointerS<port_o>(port)) == width,
                               "Unexpected width for duplex m_axi port " + name);
               };
               assert_port_width("in1", 2);
               assert_port_width("in2", 32);
               assert_port_width("in3", data_bitsize);
               assert_port_width("in4", address_bitsize);
               assert_port_width("in5", 32);
               assert_port_width("out1", data_bitsize);
               assert_port_width(CLOCK_PORT_NAME, 1);
               assert_port_width(RESET_PORT_NAME, 1);
               assert_port_width(DONE_PORT_NAME, 1);
               assert_port_width("duplex_start", 1);
               assert_port_width("duplex_opcode", 2);
               assert_port_width("duplex_size", 32);
               assert_port_width("duplex_data", data_bitsize);
               assert_port_width("duplex_address", address_bitsize);
               assert_port_width("duplex_count", 32);
               assert_port_width("duplex_done", 1);
               assert_port_width("duplex_result", data_bitsize);
               CM->add_NP_functionality(top, NP_functionality::LIBRARY, "in1 in2 in3 in4 in5 out1");
               CM->add_NP_functionality(top, NP_functionality::VERILOG_PROVIDED,
                                        "assign duplex_start = " START_PORT_NAME "[0];\n"
                                        "assign duplex_opcode = in1;\n"
                                        "assign duplex_size = in2;\n"
                                        "assign duplex_data = in3;\n"
                                        "assign duplex_address = in4;\n"
                                        "assign duplex_count = in5;\n"
                                        "assign " DONE_PORT_NAME " = duplex_done;\n"
                                        "assign out1 = duplex_result;");
            }
            else
            {
               CM->add_NP_functionality(top, NP_functionality::VERILOG_GENERATOR, "DuplexMAXIHDLGenerator");
            }

            TechMan->add_resource(INTERFACE_LIBRARY, resource_name, CM);
            const auto fu = GetPointerS<functional_unit>(TechMan->get_fu(resource_name, INTERFACE_LIBRARY));
            for(const auto& metadata_entry : expected_attributes)
            {
               fu->ordered_attributes.push_back(metadata_entry.first);
               const auto numeric = metadata_entry.first == "bambu_max_read_burst_length" ||
                                    metadata_entry.first == "bambu_axi_address_bitsize" ||
                                    metadata_entry.first == "bambu_axi_data_bitsize" ||
                                    metadata_entry.first == "bambu_num_read_outstanding" ||
                                    metadata_entry.first == "bambu_read_fifo_depth" ||
                                    metadata_entry.first == "bambu_max_write_burst_length" ||
                                    metadata_entry.first == "bambu_write_burst_outstanding" ||
                                    metadata_entry.first == "bambu_write_fifo_depth";
               fu->attributes[metadata_entry.first] =
                   attributeRef(new attribute(numeric ? attribute::INT32 : attribute::STRING, metadata_entry.second));
            }
            fu->area_m = std::make_shared<area_info>();
            fu->area_m->resources[area_info::AREA] = 0;
         };

         register_resource(read_name, "read_endpoint", read_mode, true);
         register_resource(write_name, "write_endpoint", write_mode, true);
         register_resource(aggregator_name, "aggregator", duplex_mode, false);
      }
   } // namespace

   void RegisterFrozenDuplexResources(
       const technology_managerRef& TechMan, const ParameterConstRef& parameters,
       const std::map<std::string, FrozenDuplexContribution>& frozen_by_bundle,
       std::map<std::pair<std::string, std::string>, std::pair<unsigned int, unsigned int>>& global_constraints)
   {
      const auto operation_names = [](const std::string& bundle) {
         return std::array<std::string, 5>{
             ENCODE_FDNAME(bundle, "_configure_read", ""), ENCODE_FDNAME(bundle, "_burst_read", ""),
             ENCODE_FDNAME(bundle, "_configure_write", ""), ENCODE_FDNAME(bundle, "_store_write", ""),
             ENCODE_FDNAME(bundle, "_drain_write", "")};
      };
      // Validate every bundle before creating any resource or changing any operation mapping.
      for(const auto& bundle_plan : frozen_by_bundle)
      {
         const auto bundle = bundle_plan.first;
         const auto names = operation_names(bundle);
         const auto read_resource = ENCODE_FDNAME(bundle, "_duplex_read_endpoint", "");
         const auto write_resource = ENCODE_FDNAME(bundle, "_duplex_write_endpoint", "");
         for(const auto& item : {std::make_pair(names[0], read_resource), std::make_pair(names[1], read_resource),
                                 std::make_pair(names[2], write_resource), std::make_pair(names[3], write_resource),
                                 std::make_pair(names[4], write_resource)})
         {
            const auto mapped = TechMan->GetFunctionFU(item.first);
            if(mapped && mapped != TechMan->get_fu(item.second, INTERFACE_LIBRARY))
            {
               THROW_ERROR_USAGE("Duplex m_axi operation '" + item.first +
                                 "' is already owned by an incompatible resource.");
            }
         }
         const auto plain_resource = ENCODE_FDNAME(bundle, "", "");
         if(TechMan->is_library_manager(INTERFACE_LIBRARY) &&
            TechMan->get_library_manager(INTERFACE_LIBRARY)->is_fu(plain_resource))
         {
            THROW_ERROR_USAGE("Duplex m_axi bundle '" + bundle +
                              "' conflicts with an existing single-mode m_axi resource.");
         }
         for(const auto& resource : {read_resource, write_resource})
         {
            const auto constraint = global_constraints.find({resource, INTERFACE_LIBRARY});
            if(constraint != global_constraints.end() && constraint->second != std::make_pair(1U, 1U))
            {
               THROW_ERROR_USAGE("Incompatible duplex resource constraint for bundle '" + bundle + "'.");
            }
         }
      }
      // Registration starts only after the complete preflight above succeeds.
      for(const auto& bundle_plan : frozen_by_bundle)
      {
         const auto& contribution = bundle_plan.second;
         register_duplex_m_axi_resources(TechMan, parameters, bundle_plan.first, contribution.address_bitsize,
                                         contribution.data_bitsize, contribution.data_bitsize, contribution.latency,
                                         contribution.read_max_burst_length, contribution.read_profile,
                                         contribution.write_profile);
         const auto names = operation_names(bundle_plan.first);
         const auto add_if_unmapped = [&](const std::string& op, const std::string& resource) {
            if(!TechMan->GetFunctionFU(op))
            {
               TechMan->add_operation(INTERFACE_LIBRARY, resource, op);
            }
            const auto mapped_fu = GetPointerS<functional_unit>(TechMan->GetFunctionFU(op));
            const auto operation_node = GetPointerS<operation>(mapped_fu->get_operation(op));
            operation_node->time_m = std::make_shared<time_info>();
            operation_node->bounded = false;
            operation_node->time_m->set_execution_time(TechMan->CGetSetupHoldTime() + EPSILON, 0);
            operation_node->time_m->set_synthesis_dependent(true);
         };
         const auto read_resource = ENCODE_FDNAME(bundle_plan.first, "_duplex_read_endpoint", "");
         const auto write_resource = ENCODE_FDNAME(bundle_plan.first, "_duplex_write_endpoint", "");
         add_if_unmapped(names[0], read_resource);
         add_if_unmapped(names[1], read_resource);
         add_if_unmapped(names[2], write_resource);
         add_if_unmapped(names[3], write_resource);
         add_if_unmapped(names[4], write_resource);
         for(const auto& resource : {read_resource, write_resource})
         {
            global_constraints[{resource, INTERFACE_LIBRARY}] = std::make_pair(1U, 1U);
         }
      }
   }

   void CreateMAXIResource(const std::set<std::string>& operationsR, const std::set<std::string>& operationsW,
                           const AXIResourceRequest& request, FunctionArchitectureRef func_arch, bool unused_port,
                           unsigned int root_id, const application_managerRef& AppM,
                           const ParameterConstRef& parameters, [[maybe_unused]] int debug_level)
   {
      THROW_ASSERT(GetPointer<HLS_manager>(AppM), "");
      const auto HLSMgr = GetPointerS<HLS_manager>(AppM);
      THROW_ASSERT(func_arch->parms.find(request.argument_id) != func_arch->parms.end(),
                   "Missing parameter '" + request.argument_id + "' from function architecute.");
      const auto& parm_attrs = func_arch->parms.at(request.argument_id);
      const auto& bundle_name = parm_attrs.at(FunctionArchitecture::parm_bundle);
      const auto ResourceName = ENCODE_FDNAME(bundle_name, "", "");
      const auto configure_read_name = ENCODE_FDNAME(bundle_name, "_configure_read", "");
      const auto configure_write_name = ENCODE_FDNAME(bundle_name, "_configure_write", "");
      const bool configured_read_resource = operationsR.count(configure_read_name) != 0;
      const bool configured_write_resource = operationsW.count(configure_write_name) != 0;
      if(configured_read_resource && configured_write_resource)
      {
         THROW_ERROR_USAGE("A single m_axi resource cannot be configured for both read and write burst modes.");
      }
      const bool configured_burst_resource = configured_read_resource || configured_write_resource;
      const std::string requested_burst_mode = configured_read_resource ? "read" : "write";
      const auto HLS_D = HLSMgr->get_HLS_device();
      const auto TechMan = HLS_D->get_technology_manager();
      if(TechMan->is_library_manager(INTERFACE_LIBRARY))
      {
         const auto library_manager = TechMan->get_library_manager(INTERFACE_LIBRARY);
         for(const auto& suffix : {"_duplex_read_endpoint", "_duplex_write_endpoint", "_duplex_aggregator"})
         {
            if(library_manager->is_fu(ENCODE_FDNAME(bundle_name, suffix, "")))
            {
               THROW_ERROR_USAGE("Legacy m_axi resource for bundle '" + bundle_name +
                                 "' conflicts with a registered duplex resource.");
            }
         }
      }
      const bool resource_already_exists = TechMan->is_library_manager(INTERFACE_LIBRARY) &&
                                           TechMan->get_library_manager(INTERFACE_LIBRARY)->is_fu(ResourceName);
      const auto& iface_attrs = func_arch->ifaces.at(bundle_name);
      const auto burst_profile_attr = std::string("bambu_max_read_burst_length");
      const auto outstanding_profile_attr = std::string("bambu_num_read_outstanding");
      const auto fifo_depth_profile_attr = std::string("bambu_read_fifo_depth");
      const auto write_burst_profile_attr = std::string("bambu_max_write_burst_length");
      const auto write_outstanding_profile_attr = std::string("bambu_write_burst_outstanding");
      const auto write_fifo_depth_profile_attr = std::string("bambu_write_fifo_depth");
      const auto burst_mode_attr = std::string("bambu_axi_burst_mode");
      const auto axi_latency_attr = std::string("bambu_axi_latency");
      unsigned int requested_burst_max = 0;
      auto requested_read_profile = AXIReadBurstProfile{};
      auto requested_write_profile = AXIWriteBurstProfile{};
      const auto requested_axi_latency = ParseAXIInterfaceLatency(iface_attrs, bundle_name);
      if(configured_read_resource)
      {
         const auto max_read_burst = iface_attrs.find(FunctionArchitecture::iface_max_read_burst_length);
         if(max_read_burst == iface_attrs.end())
         {
            THROW_ERROR_USAGE("Configured burst-read operation has no max_read_burst_length attribute for bundle '" +
                              bundle_name + "'.");
         }
         requested_read_profile = ParseAXIReadBurstProfile(iface_attrs, bundle_name);
         requested_burst_max = requested_read_profile.max_burst_length;
      }
      if(configured_write_resource)
      {
         requested_write_profile = ParseAXIWriteBurstProfile(iface_attrs, bundle_name);
      }

      if(!resource_already_exists)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "-->Creating interface resource: " + INTERFACE_LIBRARY + ":" + ResourceName);
         const structural_managerRef CM(new structural_manager(parameters));
         const structural_type_descriptorRef module_type(new structural_type_descriptor(ResourceName));
         CM->set_top_info(ResourceName, module_type);
         const auto interface_top = CM->get_circ();
         /// add description and license
         GetPointerS<module_o>(interface_top)->set_description("Interface module for function: " + ResourceName);
         GetPointerS<module_o>(interface_top)->set_copyright(GENERATED_COPYRIGHT);
         GetPointerS<module_o>(interface_top)->set_authors("Component automatically generated by bambu");
         GetPointerS<module_o>(interface_top)->set_license(GENERATED_LICENSE);
         GetPointerS<module_o>(interface_top)->set_multi_unit_multiplicity(1U);
         if(configured_burst_resource)
         {
            GetPointerS<module_o>(interface_top)
                ->add_attribute(burst_mode_attr, attributeRef(new attribute(attribute::STRING, requested_burst_mode)));
         }

         const auto address_bitsize = HLSMgr->get_address_bitsize();
         const auto interface_bitwidth =
             iface_attrs.find(FunctionArchitecture::iface_bitwidth) != iface_attrs.end() ?
                 std::stoull(iface_attrs.find(FunctionArchitecture::iface_bitwidth)->second) :
                 request.bitwidth;
         const auto backEndBitsize =
             iface_attrs.find(FunctionArchitecture::iface_cache_bus_size) != iface_attrs.end() ?
                 std::stoull(iface_attrs.find(FunctionArchitecture::iface_cache_bus_size)->second) :
                 interface_bitwidth;
         if(configured_burst_resource && (request.bitwidth != 32 || interface_bitwidth != 32 || backEndBitsize != 32 ||
                                          (address_bitsize != 32 && address_bitsize != 64)))
         {
            THROW_ERROR_USAGE("Configured AXI burst resource requires 32-bit data/backend widths and a 32 or 64 bit "
                              "address for bundle '" +
                              bundle_name + "'.");
         }
         const auto nbitDataSize = 64u - static_cast<unsigned>(__builtin_clzll(interface_bitwidth));

         const structural_type_descriptorRef address_interface_datatype(
             new structural_type_descriptor("bool", address_bitsize));
         const structural_type_descriptorRef size1(
             new structural_type_descriptor("bool", configured_burst_resource ? 2 : 1));
         const structural_type_descriptorRef rwsize(
             new structural_type_descriptor("bool", configured_burst_resource ? 32 : nbitDataSize));
         const structural_type_descriptorRef rwtypeIn(new structural_type_descriptor("bool", interface_bitwidth));
         const structural_type_descriptorRef rwtypeOut(new structural_type_descriptor("bool", backEndBitsize));
         const structural_type_descriptorRef idType(new structural_type_descriptor("bool", 6));
         const structural_type_descriptorRef lenType(new structural_type_descriptor("bool", 8));
         const structural_type_descriptorRef sizeType(new structural_type_descriptor("bool", 3));
         const structural_type_descriptorRef burstType(new structural_type_descriptor("bool", 2));
         const structural_type_descriptorRef lockType(new structural_type_descriptor("bool", 1));
         const structural_type_descriptorRef cacheType(new structural_type_descriptor("bool", 4));
         const structural_type_descriptorRef protType(new structural_type_descriptor("bool", 3));
         const structural_type_descriptorRef qosType(new structural_type_descriptor("bool", 4));
         const structural_type_descriptorRef regionType(new structural_type_descriptor("bool", 4));
         const structural_type_descriptorRef userType(new structural_type_descriptor("bool", 1));
         const structural_type_descriptorRef strbType(new structural_type_descriptor("bool", backEndBitsize / 8ULL));
         const structural_type_descriptorRef respType(new structural_type_descriptor("bool", 2));
         const structural_type_descriptorRef bool_type(new structural_type_descriptor("bool", 0));

         CM->add_port(CLOCK_PORT_NAME, port_o::IN, interface_top, bool_type);
         CM->add_port(RESET_PORT_NAME, port_o::IN, interface_top, bool_type);
         CM->add_port_vector(START_PORT_NAME, port_o::IN, 1U, interface_top, bool_type);

         // when 0 is a read otherwise is a write
         CM->add_port("in1", port_o::IN, interface_top, size1);
         // bit-width size of the written or read data
         CM->add_port("in2", port_o::IN, interface_top, rwsize);
         // value written when the first operand is 1, 0 otherwise
         CM->add_port("in3", port_o::IN, interface_top, rwtypeIn);

         const auto addrPort = CM->add_port("in4", port_o::IN, interface_top, address_interface_datatype);
         GetPointerS<port_o>(addrPort)->set_is_addr_bus(true);

         if(configured_burst_resource)
         {
            const structural_type_descriptorRef burst_count_type(new structural_type_descriptor("bool", 32));
            CM->add_port("in5", port_o::IN, interface_top, burst_count_type);
         }

         const auto cache_reset_port = CM->add_port(CACHE_RESET_PORT_NAME, port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(cache_reset_port)->set_port_interface(port_o::port_interface::CACHE_RESET);

         const auto awready = CM->add_port("_m_axi_" + bundle_name + "_awready", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(awready)->set_port_interface(port_o::port_interface::M_AXI_AWREADY);

         const auto wready = CM->add_port("_m_axi_" + bundle_name + "_wready", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(wready)->set_port_interface(port_o::port_interface::M_AXI_WREADY);

         const auto bid = CM->add_port_vector("_m_axi_" + bundle_name + "_bid", port_o::IN, 1, interface_top, idType);
         GetPointerS<port_o>(bid)->set_port_interface(port_o::port_interface::M_AXI_BID);

         const auto bresp = CM->add_port("_m_axi_" + bundle_name + "_bresp", port_o::IN, interface_top, respType);
         GetPointerS<port_o>(bresp)->set_port_interface(port_o::port_interface::M_AXI_BRESP);

         const auto buser =
             CM->add_port_vector("_m_axi_" + bundle_name + "_buser", port_o::IN, 1, interface_top, userType);
         GetPointerS<port_o>(buser)->set_port_interface(port_o::port_interface::M_AXI_BUSER);

         const auto bvalid = CM->add_port("_m_axi_" + bundle_name + "_bvalid", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(bvalid)->set_port_interface(port_o::port_interface::M_AXI_BVALID);

         const auto arready = CM->add_port("_m_axi_" + bundle_name + "_arready", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(arready)->set_port_interface(port_o::port_interface::M_AXI_ARREADY);

         const auto rid = CM->add_port_vector("_m_axi_" + bundle_name + "_rid", port_o::IN, 1, interface_top, idType);
         GetPointerS<port_o>(rid)->set_port_interface(port_o::port_interface::M_AXI_RID);

         const auto rdata = CM->add_port("_m_axi_" + bundle_name + "_rdata", port_o::IN, interface_top, rwtypeOut);
         GetPointerS<port_o>(rdata)->set_port_interface(port_o::port_interface::M_AXI_RDATA);

         const auto rresp = CM->add_port("_m_axi_" + bundle_name + "_rresp", port_o::IN, interface_top, respType);
         GetPointerS<port_o>(rresp)->set_port_interface(port_o::port_interface::M_AXI_RRESP);

         const auto rlast = CM->add_port("_m_axi_" + bundle_name + "_rlast", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(rlast)->set_port_interface(port_o::port_interface::M_AXI_RLAST);

         const auto ruser =
             CM->add_port_vector("_m_axi_" + bundle_name + "_ruser", port_o::IN, 1, interface_top, userType);
         GetPointerS<port_o>(ruser)->set_port_interface(port_o::port_interface::M_AXI_RUSER);

         const auto rvalid = CM->add_port("_m_axi_" + bundle_name + "_rvalid", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(rvalid)->set_port_interface(port_o::port_interface::M_AXI_RVALID);

         CM->add_port(DONE_PORT_NAME, port_o::OUT, interface_top, bool_type);
         CM->add_port("out1", port_o::OUT, interface_top, rwtypeIn);

         const auto awid =
             CM->add_port_vector("_m_axi_" + bundle_name + "_awid", port_o::OUT, 1, interface_top, idType);
         GetPointerS<port_o>(awid)->set_port_interface(port_o::port_interface::M_AXI_AWID);

         const auto awaddr =
             CM->add_port("_m_axi_" + bundle_name + "_awaddr", port_o::OUT, interface_top, address_interface_datatype);
         GetPointerS<port_o>(awaddr)->set_port_interface(port_o::port_interface::M_AXI_AWADDR);

         const auto awlen = CM->add_port("_m_axi_" + bundle_name + "_awlen", port_o::OUT, interface_top, lenType);
         GetPointerS<port_o>(awlen)->set_port_interface(port_o::port_interface::M_AXI_AWLEN);

         const auto awsize = CM->add_port("_m_axi_" + bundle_name + "_awsize", port_o::OUT, interface_top, sizeType);
         GetPointerS<port_o>(awsize)->set_port_interface(port_o::port_interface::M_AXI_AWSIZE);

         const auto awburst = CM->add_port("_m_axi_" + bundle_name + "_awburst", port_o::OUT, interface_top, burstType);
         GetPointerS<port_o>(awburst)->set_port_interface(port_o::port_interface::M_AXI_AWBURST);

         const auto awlock =
             CM->add_port_vector("_m_axi_" + bundle_name + "_awlock", port_o::OUT, 1, interface_top, lockType);
         GetPointerS<port_o>(awlock)->set_port_interface(port_o::port_interface::M_AXI_AWLOCK);

         const auto awcache = CM->add_port("_m_axi_" + bundle_name + "_awcache", port_o::OUT, interface_top, cacheType);
         GetPointerS<port_o>(awcache)->set_port_interface(port_o::port_interface::M_AXI_AWCACHE);

         const auto awprot = CM->add_port("_m_axi_" + bundle_name + "_awprot", port_o::OUT, interface_top, protType);
         GetPointerS<port_o>(awprot)->set_port_interface(port_o::port_interface::M_AXI_AWPROT);

         const auto awqos = CM->add_port("_m_axi_" + bundle_name + "_awqos", port_o::OUT, interface_top, qosType);
         GetPointerS<port_o>(awqos)->set_port_interface(port_o::port_interface::M_AXI_AWQOS);

         const auto awregion =
             CM->add_port("_m_axi_" + bundle_name + "_awregion", port_o::OUT, interface_top, regionType);
         GetPointerS<port_o>(awregion)->set_port_interface(port_o::port_interface::M_AXI_AWREGION);

         const auto awuser =
             CM->add_port_vector("_m_axi_" + bundle_name + "_awuser", port_o::OUT, 1, interface_top, userType);
         GetPointerS<port_o>(awuser)->set_port_interface(port_o::port_interface::M_AXI_AWUSER);

         const auto awvalid = CM->add_port("_m_axi_" + bundle_name + "_awvalid", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(awvalid)->set_port_interface(port_o::port_interface::M_AXI_AWVALID);

         const auto wdata = CM->add_port("_m_axi_" + bundle_name + "_wdata", port_o::OUT, interface_top, rwtypeOut);
         GetPointerS<port_o>(wdata)->set_port_interface(port_o::port_interface::M_AXI_WDATA);

         const auto wstrb =
             CM->add_port_vector("_m_axi_" + bundle_name + "_wstrb", port_o::OUT, 1, interface_top, strbType);
         GetPointerS<port_o>(wstrb)->set_port_interface(port_o::port_interface::M_AXI_WSTRB);

         const auto wlast = CM->add_port("_m_axi_" + bundle_name + "_wlast", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(wlast)->set_port_interface(port_o::port_interface::M_AXI_WLAST);

         const auto wuser =
             CM->add_port_vector("_m_axi_" + bundle_name + "_wuser", port_o::OUT, 1, interface_top, userType);
         GetPointerS<port_o>(wuser)->set_port_interface(port_o::port_interface::M_AXI_WUSER);

         const auto wvalid = CM->add_port("_m_axi_" + bundle_name + "_wvalid", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(wvalid)->set_port_interface(port_o::port_interface::M_AXI_WVALID);

         const auto bready = CM->add_port("_m_axi_" + bundle_name + "_bready", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(bready)->set_port_interface(port_o::port_interface::M_AXI_BREADY);

         const auto arid =
             CM->add_port_vector("_m_axi_" + bundle_name + "_arid", port_o::OUT, 1, interface_top, idType);
         GetPointerS<port_o>(arid)->set_port_interface(port_o::port_interface::M_AXI_ARID);

         const auto araddr =
             CM->add_port("_m_axi_" + bundle_name + "_araddr", port_o::OUT, interface_top, address_interface_datatype);
         GetPointerS<port_o>(araddr)->set_port_interface(port_o::port_interface::M_AXI_ARADDR);

         const auto arlen = CM->add_port("_m_axi_" + bundle_name + "_arlen", port_o::OUT, interface_top, lenType);
         GetPointerS<port_o>(arlen)->set_port_interface(port_o::port_interface::M_AXI_ARLEN);

         const auto arsize = CM->add_port("_m_axi_" + bundle_name + "_arsize", port_o::OUT, interface_top, sizeType);
         GetPointerS<port_o>(arsize)->set_port_interface(port_o::port_interface::M_AXI_ARSIZE);

         const auto arburst = CM->add_port("_m_axi_" + bundle_name + "_arburst", port_o::OUT, interface_top, burstType);
         GetPointerS<port_o>(arburst)->set_port_interface(port_o::port_interface::M_AXI_ARBURST);

         const auto arlock =
             CM->add_port_vector("_m_axi_" + bundle_name + "_arlock", port_o::OUT, 1, interface_top, lockType);
         GetPointerS<port_o>(arlock)->set_port_interface(port_o::port_interface::M_AXI_ARLOCK);

         const auto arcache = CM->add_port("_m_axi_" + bundle_name + "_arcache", port_o::OUT, interface_top, cacheType);
         GetPointerS<port_o>(arcache)->set_port_interface(port_o::port_interface::M_AXI_ARCACHE);

         const auto arprot = CM->add_port("_m_axi_" + bundle_name + "_arprot", port_o::OUT, interface_top, protType);
         GetPointerS<port_o>(arprot)->set_port_interface(port_o::port_interface::M_AXI_ARPROT);

         const auto arqos = CM->add_port("_m_axi_" + bundle_name + "_arqos", port_o::OUT, interface_top, qosType);
         GetPointerS<port_o>(arqos)->set_port_interface(port_o::port_interface::M_AXI_ARQOS);

         const auto arregion =
             CM->add_port("_m_axi_" + bundle_name + "_arregion", port_o::OUT, interface_top, regionType);
         GetPointerS<port_o>(arregion)->set_port_interface(port_o::port_interface::M_AXI_ARREGION);

         const auto aruser =
             CM->add_port_vector("_m_axi_" + bundle_name + "_aruser", port_o::OUT, 1, interface_top, userType);
         GetPointerS<port_o>(aruser)->set_port_interface(port_o::port_interface::M_AXI_ARUSER);

         const auto arvalid = CM->add_port("_m_axi_" + bundle_name + "_arvalid", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(arvalid)->set_port_interface(port_o::port_interface::M_AXI_ARVALID);

         const auto rready = CM->add_port("_m_axi_" + bundle_name + "_rready", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(rready)->set_port_interface(port_o::port_interface::M_AXI_RREADY);

         bool has_slave = false;
         bool has_direct = false;
         for(auto& p : func_arch->parms)
         {
            if(p.second.at(FunctionArchitecture::parm_bundle) == bundle_name)
            {
               const auto& parm_offset = p.second.at(FunctionArchitecture::parm_offset);
               if(parm_offset == "slave")
               {
                  has_slave = true;
               }
               else if(parm_offset == "direct")
               {
                  has_direct = true;
                  const auto offset_port =
                      CM->add_port("_" + p.first, port_o::IN, interface_top, address_interface_datatype);
                  GetPointerS<port_o>(offset_port)->set_port_interface(port_o::port_interface::PI_M_AXI_DIRECT);
               }
            }
         }
         if(!has_direct)
         {
            THROW_ERROR_USAGE("Only 'direct' AXI interfaces are supported for this configuration.");
         }
         if(has_slave)
         {
            const auto s_awvalid = CM->add_port("_s_axi_AXILiteS_AWVALID", port_o::IN, interface_top, bool_type);
            GetPointerS<port_o>(s_awvalid)->set_port_interface(port_o::port_interface::S_AXIL_AWVALID);
            const auto s_awaddr =
                CM->add_port("_s_axi_AXILiteS_AWADDR", port_o::IN, interface_top, address_interface_datatype);
            GetPointerS<port_o>(s_awaddr)->set_port_interface(port_o::port_interface::S_AXIL_AWADDR);
            const auto s_wvalid = CM->add_port("_s_axi_AXILiteS_WVALID", port_o::IN, interface_top, bool_type);
            GetPointerS<port_o>(s_wvalid)->set_port_interface(port_o::port_interface::S_AXIL_WVALID);
            const auto s_wdata =
                CM->add_port("_s_axi_AXILiteS_WDATA", port_o::IN, interface_top, address_interface_datatype);
            GetPointerS<port_o>(s_wdata)->set_port_interface(port_o::port_interface::S_AXIL_WDATA);
            const auto s_wstrb = CM->add_port("_s_axi_AXILiteS_WSTRB", port_o::IN, interface_top, strbType);
            GetPointerS<port_o>(s_wstrb)->set_port_interface(port_o::port_interface::S_AXIL_WSTRB);
            const auto s_arvalid = CM->add_port("_s_axi_AXILiteS_ARVALID", port_o::IN, interface_top, bool_type);
            GetPointerS<port_o>(s_arvalid)->set_port_interface(port_o::port_interface::S_AXIL_ARVALID);
            const auto s_araddr =
                CM->add_port("_s_axi_AXILiteS_ARADDR", port_o::IN, interface_top, address_interface_datatype);
            GetPointerS<port_o>(s_araddr)->set_port_interface(port_o::port_interface::S_AXIL_ARADDR);
            const auto s_rready = CM->add_port("_s_axi_AXILiteS_RREADY", port_o::IN, interface_top, bool_type);
            GetPointerS<port_o>(s_rready)->set_port_interface(port_o::port_interface::S_AXIL_RREADY);
            const auto s_bready = CM->add_port("_s_axi_AXILiteS_BREADY", port_o::IN, interface_top, bool_type);
            GetPointerS<port_o>(s_bready)->set_port_interface(port_o::port_interface::S_AXIL_BREADY);

            const auto s_awready = CM->add_port("_s_axi_AXILiteS_AWREADY", port_o::OUT, interface_top, bool_type);
            GetPointerS<port_o>(s_awready)->set_port_interface(port_o::port_interface::S_AXIL_AWREADY);
            const auto s_wready = CM->add_port("_s_axi_AXILiteS_WREADY", port_o::OUT, interface_top, bool_type);
            GetPointerS<port_o>(s_wready)->set_port_interface(port_o::port_interface::S_AXIL_WREADY);
            const auto s_arready = CM->add_port("_s_axi_AXILiteS_ARREADY", port_o::OUT, interface_top, bool_type);
            GetPointerS<port_o>(s_arready)->set_port_interface(port_o::port_interface::S_AXIL_ARREADY);
            const auto s_rvalid = CM->add_port("_s_axi_AXILiteS_RVALID", port_o::OUT, interface_top, bool_type);
            GetPointerS<port_o>(s_rvalid)->set_port_interface(port_o::port_interface::S_AXIL_RVALID);
            const auto s_rdata =
                CM->add_port("_s_axi_AXILiteS_RDATA", port_o::OUT, interface_top, address_interface_datatype);
            GetPointerS<port_o>(s_rdata)->set_port_interface(port_o::port_interface::S_AXIL_RDATA);
            const auto s_rresp = CM->add_port("_s_axi_AXILiteS_RRESP", port_o::OUT, interface_top, respType);
            GetPointerS<port_o>(s_rresp)->set_port_interface(port_o::port_interface::S_AXIL_RRESP);
            const auto s_bvalid = CM->add_port("_s_axi_AXILiteS_BVALID", port_o::OUT, interface_top, bool_type);
            GetPointerS<port_o>(s_bvalid)->set_port_interface(port_o::port_interface::S_AXIL_BVALID);
            const auto s_bresp = CM->add_port("_s_axi_AXILiteS_BRESP", port_o::OUT, interface_top, respType);
            GetPointerS<port_o>(s_bresp)->set_port_interface(port_o::port_interface::S_AXIL_BRESP);
         }

         const auto library_args = configured_burst_resource ? "in1 in2 in3 in4 in5 out1" : "in1 in2 in3 in4 out1";
         CM->add_NP_functionality(interface_top, NP_functionality::LIBRARY, library_args);
         CM->add_NP_functionality(interface_top, NP_functionality::VERILOG_GENERATOR,
                                  "ReadWrite" + getHDLGeneratorNameToken(request.interface_name) + "HDLGenerator");

         TechMan->add_resource(INTERFACE_LIBRARY, ResourceName, CM);
         if(unused_port)
         {
            HLSMgr->unused_interfaces[root_id].insert(std::make_pair(INTERFACE_LIBRARY, ResourceName));
         }
         const auto fu = GetPointerS<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
         if(configured_burst_resource)
         {
            fu->ordered_attributes.push_back(burst_mode_attr);
            fu->attributes[burst_mode_attr] = attributeRef(new attribute(attribute::STRING, requested_burst_mode));
         }
         if(configured_read_resource)
         {
            fu->ordered_attributes.push_back(burst_profile_attr);
            fu->attributes[burst_profile_attr] =
                attributeRef(new attribute(attribute::INT32, STR(requested_burst_max)));
            fu->ordered_attributes.push_back(outstanding_profile_attr);
            fu->attributes[outstanding_profile_attr] =
                attributeRef(new attribute(attribute::INT32, STR(requested_read_profile.max_outstanding)));
            fu->ordered_attributes.push_back(fifo_depth_profile_attr);
            fu->attributes[fifo_depth_profile_attr] =
                attributeRef(new attribute(attribute::INT32, STR(requested_read_profile.fifo_depth)));
         }
         if(configured_write_resource)
         {
            fu->ordered_attributes.push_back(write_burst_profile_attr);
            fu->attributes[write_burst_profile_attr] =
                attributeRef(new attribute(attribute::INT32, STR(requested_write_profile.max_burst_length)));
            fu->ordered_attributes.push_back(write_outstanding_profile_attr);
            fu->attributes[write_outstanding_profile_attr] =
                attributeRef(new attribute(attribute::INT32, STR(requested_write_profile.max_outstanding)));
            fu->ordered_attributes.push_back(write_fifo_depth_profile_attr);
            fu->attributes[write_fifo_depth_profile_attr] =
                attributeRef(new attribute(attribute::INT32, STR(requested_write_profile.fifo_depth)));
         }
         fu->ordered_attributes.push_back(axi_latency_attr);
         fu->attributes[axi_latency_attr] =
             attributeRef(new attribute(attribute::STRING, std::to_string(requested_axi_latency)));
         fu->area_m = std::make_shared<area_info>();
         fu->area_m->resources[area_info::AREA] = 0;
         if(iface_attrs.find(FunctionArchitecture::iface_cache_line_count) != iface_attrs.end())
         {
            const auto flushName = ENCODE_FDNAME(bundle_name, "_Flush_", "m_axi");
            TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, flushName);
            const auto op = GetPointer<operation>(fu->get_operation(flushName));
            op->time_m = std::make_shared<time_info>();
            op->bounded = false;
            op->time_m->set_execution_time(HLS_D->get_technology_manager()->CGetSetupHoldTime() + EPSILON, 0);
            op->time_m->set_synthesis_dependent(true);
         }
         HLSMgr->global_resource_constraints[std::make_pair(ResourceName, INTERFACE_LIBRARY)] = std::make_pair(1U, 1U);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Interface resource created");
      }

      // A bundle is shared globally across roots. Its functional-unit ABI is fixed
      // by the first root that creates it, so reject an incompatible later root
      // before adding operations that would otherwise silently bind to that ABI.
      if(resource_already_exists)
      {
         const auto existing_fu = GetPointerS<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
         const auto existing_latency = existing_fu->attributes.find(axi_latency_attr);
         const auto existing_latency_value =
             existing_latency == existing_fu->attributes.end() || !existing_latency->second ?
                 std::string("0") :
                 existing_latency->second->get_content_str();
         const auto requested_latency_value = std::to_string(requested_axi_latency);
         if(existing_latency_value != requested_latency_value)
         {
            THROW_ERROR_USAGE("Incompatible AXI latency for bundle '" + bundle_name + "': existing roots use latency=" +
                              existing_latency_value + " while this root requires latency=" + requested_latency_value +
                              ". Use distinct bundle names for roots with different AXI latencies.");
         }
         const auto existing_mode = existing_fu->attributes.find(burst_mode_attr);
         const auto existing_mode_value = existing_mode == existing_fu->attributes.end() || !existing_mode->second ?
                                              std::string("legacy") :
                                              existing_mode->second->get_content_str();
         const auto requested_mode_value = configured_burst_resource ? requested_burst_mode : "legacy";
         if(existing_mode_value != requested_mode_value)
         {
            THROW_ERROR_USAGE("Incompatible m_axi resource ABI for bundle '" + bundle_name +
                              "': existing roots use burst mode '" + existing_mode_value +
                              "' while this root requires '" + requested_mode_value +
                              "'. Use distinct bundle names for incompatible roots.");
         }
         if(configured_read_resource)
         {
            const auto profile = existing_fu->attributes.find(burst_profile_attr);
            if(profile == existing_fu->attributes.end() || !profile->second ||
               boost::lexical_cast<unsigned int>(profile->second->get_content_str()) != requested_burst_max)
            {
               const auto existing_max = profile == existing_fu->attributes.end() || !profile->second ?
                                             std::string("unknown") :
                                             profile->second->get_content_str();
               THROW_ERROR_USAGE("Incompatible m_axi burst profile for bundle '" + bundle_name +
                                 "': existing roots use max_read_burst_length=" + existing_max +
                                 " while this root requires max_read_burst_length=" + STR(requested_burst_max) +
                                 ". Use distinct bundle names for roots with different burst maxima.");
            }
            const auto existing_outstanding = existing_fu->attributes.find(outstanding_profile_attr);
            const auto existing_fifo_depth = existing_fu->attributes.find(fifo_depth_profile_attr);
            const unsigned int existing_o =
                existing_outstanding == existing_fu->attributes.end() || !existing_outstanding->second ?
                    1U :
                    boost::lexical_cast<unsigned int>(existing_outstanding->second->get_content_str());
            // A root with no explicit depth uses the derived default, which is
            // exactly the value requested_read_profile already carries.
            const unsigned int existing_d =
                existing_fifo_depth == existing_fu->attributes.end() || !existing_fifo_depth->second ?
                    requested_read_profile.fifo_depth :
                    boost::lexical_cast<unsigned int>(existing_fifo_depth->second->get_content_str());
            if(existing_o != requested_read_profile.max_outstanding || existing_d != requested_read_profile.fifo_depth)
            {
               THROW_ERROR_USAGE("Incompatible m_axi burst profile for bundle '" + bundle_name +
                                 "': existing roots use num_read_outstanding=" + STR(existing_o) +
                                 ", read_fifo_depth=" + STR(existing_d) +
                                 " while this root requires "
                                 "num_read_outstanding=" +
                                 STR(requested_read_profile.max_outstanding) +
                                 ", read_fifo_depth=" + STR(requested_read_profile.fifo_depth) +
                                 ". Use distinct bundle names for roots with different burst profiles.");
            }
         }
         if(configured_write_resource)
         {
            const auto check_write_attribute = [&](const std::string& name, const unsigned int requested) {
               const auto profile = existing_fu->attributes.find(name);
               if(profile == existing_fu->attributes.end() || !profile->second ||
                  boost::lexical_cast<unsigned int>(profile->second->get_content_str()) != requested)
               {
                  THROW_ERROR_USAGE("Incompatible m_axi write-burst profile for bundle '" + bundle_name +
                                    "': attribute '" + name + "' differs across roots. Use distinct bundle names.");
               }
            };
            check_write_attribute(write_burst_profile_attr, requested_write_profile.max_burst_length);
            check_write_attribute(write_outstanding_profile_attr, requested_write_profile.max_outstanding);
            check_write_attribute(write_fifo_depth_profile_attr, requested_write_profile.fifo_depth);
         }
      }

      for(const auto& fdName : operationsR)
      {
         TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, fdName);
      }
      for(const auto& fdName : operationsW)
      {
         TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, fdName);
      }

      /* Flush Op */
      const auto fu = GetPointerS<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
      if(configured_burst_resource)
      {
         if(!fu->CM || !fu->CM->get_circ() ||
            fu->CM->get_circ()->find_member("in5", port_o_K, fu->CM->get_circ()) == nullptr)
         {
            THROW_ERROR("Configured m_axi resource for bundle '" + bundle_name +
                        "' is missing its in5 burst-count port.");
         }
         const auto mode = fu->attributes.find(burst_mode_attr);
         if(mode == fu->attributes.end() || !mode->second || mode->second->get_content_str() != requested_burst_mode)
         {
            THROW_ERROR("Configured m_axi resource for bundle '" + bundle_name +
                        "' is missing its explicit read/write burst mode.");
         }
      }

      for(const auto& fdName : operationsR)
      {
         const auto op = GetPointerS<operation>(fu->get_operation(fdName));
         op->time_m = std::make_shared<time_info>();
         op->bounded = false;
         op->time_m->set_execution_time(HLS_D->get_technology_manager()->CGetSetupHoldTime() + EPSILON, 0);
         op->time_m->set_synthesis_dependent(true);
      }
      for(const auto& fdName : operationsW)
      {
         const auto op = GetPointer<operation>(fu->get_operation(fdName));
         op->time_m = std::make_shared<time_info>();
         op->bounded = false;
         op->time_m->set_execution_time(HLS_D->get_technology_manager()->CGetSetupHoldTime() + EPSILON, 0);
         op->time_m->set_synthesis_dependent(true);
      }
   }

   std::string getHDLGeneratorNameToken(const std::string& interface_name)
   {
      if(interface_name == "DP_array")
      {
         return "DPArray";
      }
      if(interface_name == "m_axi")
      {
         return "MAXI";
      }

      std::string token;
      std::stringstream ss(interface_name);
      std::string chunk;
      while(std::getline(ss, chunk, '_'))
      {
         token += capitalize(chunk);
      }
      return token;
   }

} // namespace InterfaceInferInternal
