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
 * @file InterfaceInfer_burst_exec.cpp
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#include "InterfaceInfer_internal.hpp"

#include "application_manager.hpp"
#include "area_info.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "copyrights_strings.hpp"
#include "dbgPrintHelper.hpp"
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

#include <algorithm>
#include <array>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using InterfaceInferInternal::BundleBurstDecision;
using InterfaceInferInternal::CollectOriginalMemoryAccesses;
using InterfaceInferInternal::FrozenDuplexContribution;
using InterfaceInferInternal::get_decl_name;
using InterfaceInferInternal::hasOnlyOrdinaryMemoryEffects;
using InterfaceInferInternal::RecognizeSameBaseDuplex;
using InterfaceInferInternal::SameBaseDuplexProof;

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

   void PreanalyzeBursts(const bool burst_feature_enabled, const unsigned int effective_burst_type,
                         const unsigned int root_id, const function_val_node* fd,
                         const FunctionArchitectureRef& func_arch,
                         const std::vector<OriginalMemoryAccess>& original_memory_accesses,
                         const std::vector<OriginalMemoryAccess>& burst_memory_accesses,
                         const bool function_effects_safe, const bool original_memory_effects_safe,
                         const std::map<unsigned int, std::map<std::string, FrozenDuplexContribution>>& frozen_by_root,
                         const std::map<std::string, std::string>& bundle_vetoes, const application_managerRef& AppM,
                         const ir_managerRef& TM, std::map<std::string, BundleBurstDecision>& burst_decisions,
                         int DEBUG_PARAMETER(debug_level))
   {
      if(!burst_feature_enabled)
      {
         return;
      }
      const auto frozen_proof_for_bundle = [&](const std::string& bundle_name) -> const SameBaseDuplexProof* {
         const auto root_plan = frozen_by_root.find(root_id);
         if(root_plan == frozen_by_root.end())
         {
            return nullptr;
         }
         const auto bundle_plan = root_plan->second.find(bundle_name);
         return bundle_plan == root_plan->second.end() ? nullptr : &bundle_plan->second.proof;
      };
      // Two m_axi bundles with different names and different SSA bases do not prove disjoint
      // address ranges: with non-restrict pointers two interfaces can address overlapping memory,
      // and the two burst engines order their channels independently. Coalescing a bundle therefore
      // requires the caller's restrict contract on both the owner of this bundle and every owner
      // that performs the opposite direction in another bundle; without that contract the scalar
      // interface is kept.
      const auto parm_has_restrict_contract = [&](unsigned int argument_index) {
         const auto arg = std::find_if(fd->list_of_args.begin(), fd->list_of_args.end(),
                                       [&](const auto& candidate) { return candidate->index == argument_index; });
         if(arg == fd->list_of_args.end())
         {
            return false;
         }
         const auto attrs = func_arch->parms.find(get_decl_name(*arg));
         if(attrs == func_arch->parms.end())
         {
            return false;
         }
         const auto attr = attrs->second.find(FunctionArchitecture::parm_source_restrict);
         return attr != attrs->second.end() && attr->second == "1";
      };
      const auto cross_bundle_burst_is_disjoint = [&](const std::string& bundle_name, bool coalescing_reads,
                                                      unsigned int owner_argument_index) {
         std::vector<unsigned int> opposite_owners;
         for(const auto& access : burst_memory_accesses)
         {
            if(!access.classified || !access.ordinary || !access.statement || access.bundle == bundle_name)
            {
               continue;
            }
            const bool opposite = coalescing_reads ? access.is_write : !access.is_write;
            if(opposite)
            {
               opposite_owners.push_back(access.owner_argument_index);
            }
         }
         if(opposite_owners.empty())
         {
            // No access of the opposite direction lives in another bundle, so the ordering
            // between two independent m_axi interfaces is not what has to be proven here.
            return true;
         }
         if(!parm_has_restrict_contract(owner_argument_index))
         {
            return false;
         }
         for(const auto opposite_owner : opposite_owners)
         {
            if(!parm_has_restrict_contract(opposite_owner))
            {
               return false;
            }
         }
         return true;
      };
      const auto sl = GetPointerS<const statement_list_node>(fd->body);
      for(const auto& iface : func_arch->ifaces)
      {
         const auto& bundle_name = iface.first;
         const auto& iface_attrs = iface.second;
         const auto mode = iface_attrs.find(FunctionArchitecture::iface_mode);
         if(mode == iface_attrs.end() || mode->second != "m_axi")
         {
            continue;
         }
         const auto bundle_has_bound_parameter =
             std::any_of(func_arch->parms.begin(), func_arch->parms.end(), [&](const auto& parameter) {
                const auto bundle = parameter.second.find(FunctionArchitecture::parm_bundle);
                return bundle != parameter.second.end() && bundle->second == bundle_name;
             });
         if(!bundle_has_bound_parameter)
         {
            const auto max_read = iface_attrs.find(FunctionArchitecture::iface_max_read_burst_length);
            const auto read_outstanding = iface_attrs.find(FunctionArchitecture::iface_num_read_outstanding);
            const auto read_fifo = iface_attrs.find(FunctionArchitecture::iface_read_fifo_depth);
            if(max_read != iface_attrs.end() || read_outstanding != iface_attrs.end() || read_fifo != iface_attrs.end())
            {
               // Unbound XML profiles are metadata only. Parse with the normal defaults so
               // O-only and D-only requests remain valid while malformed explicit values
               // retain the standard attribute- and bundle-specific diagnostic.
               (void)ParseAXIReadBurstProfile(iface_attrs, bundle_name);
            }
            const auto max_write = iface_attrs.find(FunctionArchitecture::iface_max_write_burst_length);
            const auto write_outstanding = iface_attrs.find(FunctionArchitecture::iface_write_burst_outstanding);
            const auto write_fifo = iface_attrs.find(FunctionArchitecture::iface_write_fifo_depth);
            if(max_write != iface_attrs.end() || write_outstanding != iface_attrs.end() ||
               write_fifo != iface_attrs.end())
            {
               (void)ParseAXIWriteBurstProfile(iface_attrs, bundle_name);
            }
            continue;
         }
         auto& decision = burst_decisions[bundle_name];
         const auto max_read_burst = iface_attrs.find(FunctionArchitecture::iface_max_read_burst_length);
         const auto max_write_burst = iface_attrs.find(FunctionArchitecture::iface_max_write_burst_length);
         const auto write_outstanding = iface_attrs.find(FunctionArchitecture::iface_write_burst_outstanding);
         const auto write_fifo_depth = iface_attrs.find(FunctionArchitecture::iface_write_fifo_depth);
         const auto read_outstanding = iface_attrs.find(FunctionArchitecture::iface_num_read_outstanding);
         const auto read_fifo_depth = iface_attrs.find(FunctionArchitecture::iface_read_fifo_depth);
         decision.read_requested = max_read_burst != iface_attrs.end();
         decision.write_requested = max_write_burst != iface_attrs.end();
         if(decision.read_requested)
         {
            decision.read_profile = ParseAXIReadBurstProfile(iface_attrs, bundle_name);
            decision.read_max_burst_length = static_cast<unsigned int>(std::stoul(max_read_burst->second));
         }
         if(max_write_burst != iface_attrs.end() || write_outstanding != iface_attrs.end() ||
            write_fifo_depth != iface_attrs.end())
         {
            decision.write_profile = ParseAXIWriteBurstProfile(iface_attrs, bundle_name);
         }
         if(max_read_burst == iface_attrs.end() &&
            (read_outstanding != iface_attrs.end() || read_fifo_depth != iface_attrs.end()))
         {
            THROW_ERROR_USAGE(
                "AXI read profile attributes num_read_outstanding/read_fifo_depth for bundle '" + bundle_name +
                "' require max_read_burst_length; the profile cannot be applied to the legacy scalar interface.");
         }
         for(const auto& access : burst_memory_accesses)
         {
            if(access.bundle == bundle_name && access.classified)
            {
               decision.bundle_has_memory_write |= access.is_write;
               decision.bundle_has_memory_read |= !access.is_write;
               auto& direction = decision.accesses_by_argument[access.owner_argument_index];
               (access.is_write ? direction.writes : direction.reads).push_back(access);
            }
         }
         const bool cache_enabled = iface_attrs.find(FunctionArchitecture::iface_cache_line_count) != iface_attrs.end();
         if(decision.read_requested && decision.bundle_has_memory_read)
         {
            if(effective_burst_type != 1U)
            {
               decision.read_fallback_reason = effective_burst_type == 0U ?
                                                   "AXI burst type is FIXED; incremental prefetch is unsupported" :
                                                   "AXI burst type is unsupported; incremental prefetch is unavailable";
            }
            else if(!function_effects_safe)
            {
               decision.read_fallback_reason = "memory writes or opaque effects; prefetch ordering is not proven";
            }
            else if(cache_enabled)
            {
               decision.read_fallback_reason = "cache-enabled bundle";
            }
            else if(decision.bundle_has_memory_write && !frozen_proof_for_bundle(bundle_name))
            {
               decision.read_fallback_reason = "bundle contains stores";
            }
            if(decision.read_fallback_reason.empty())
            {
               std::vector<ir_nodeRef> bundle_pointer_args;
               for(const auto& candidate_arg : fd->list_of_args)
               {
                  const auto candidate_name = get_decl_name(candidate_arg);
                  if(func_arch->parms.count(candidate_name) &&
                     func_arch->parms.at(candidate_name).at(FunctionArchitecture::parm_bundle) == bundle_name &&
                     ir_helper::IsPointerType(ir_helper::CGetType(candidate_arg)))
                  {
                     bundle_pointer_args.push_back(candidate_arg);
                  }
               }
               std::set<unsigned int> seen_read_statements;
               std::map<unsigned int, std::vector<ir_nodeRef>> loop_reads;
               const auto frozen_proof = frozen_proof_for_bundle(bundle_name);
               const auto read_owner = frozen_proof ?
                                           std::find_if(bundle_pointer_args.begin(), bundle_pointer_args.end(),
                                                        [&](const auto& arg) {
                                                           return arg->index == frozen_proof->read_owner_argument_index;
                                                        }) :
                                           bundle_pointer_args.begin();
               bool cross_bundle_read_disjoint = true;
               if(read_owner != bundle_pointer_args.end() &&
                  !cross_bundle_burst_is_disjoint(bundle_name, true, (*read_owner)->index))
               {
                  cross_bundle_read_disjoint = false;
                  decision.read_fallback_reason =
                      "writes through another m_axi bundle without a restrict contract on both "
                      "owners; cross-bundle prefetch ordering is not proven";
               }
               if(cross_bundle_read_disjoint && (bundle_pointer_args.size() == 1 || frozen_proof) &&
                  read_owner != bundle_pointer_args.end())
               {
                  const auto owner_arg = *read_owner;
                  const auto original_reads = decision.accesses_by_argument.find(owner_arg->index);
                  if(original_reads != decision.accesses_by_argument.end())
                  {
                     for(const auto& access : original_reads->second.reads)
                     {
                        if(access.ordinary && seen_read_statements.insert(access.statement->index).second)
                        {
                           loop_reads[GetPointerS<const node_stmt>(access.statement)->bb_index].push_back(
                               access.statement);
                        }
                     }
                  }
                  const auto expected_base_id = AppM->getSSAFromParm(root_id, owner_arg->index);
                  const auto expected_base = TM->GetIRNode(expected_base_id);
                  std::vector<ReadBurstRegion> regions;
                  bool owner_recognized = !loop_reads.empty();
                  for(const auto& read_group : loop_reads)
                  {
                     ReadBurstRegion region{};
                     if(!recognizeReadBurstRegion(sl, read_group.second, expected_base, burst_memory_accesses,
                                                  bundle_name, region,
                                                  frozen_proof ? frozen_proof->store_statement : ir_nodeRef()))
                     {
                        owner_recognized = false;
                        break;
                     }
                     regions.push_back(std::move(region));
                  }
                  if(owner_recognized && !regions.empty())
                  {
                     for(size_t first = 0; first < regions.size() && owner_recognized; ++first)
                     {
                        for(size_t second = first + 1; second < regions.size(); ++second)
                        {
                           const auto first_to_second =
                               hasBlockPath(sl, regions[first].preheader, regions[second].preheader);
                           const auto second_to_first =
                               hasBlockPath(sl, regions[second].preheader, regions[first].preheader);
                           if(first_to_second == second_to_first)
                           {
                              owner_recognized = false;
                              break;
                           }
                        }
                     }
                  }
                  if(owner_recognized && !regions.empty())
                  {
                     std::sort(regions.begin(), regions.end(), [&](const auto& lhs, const auto& rhs) {
                        return lhs.preheader != rhs.preheader && hasBlockPath(sl, lhs.preheader, rhs.preheader);
                     });
                     decision.read_regions[owner_arg->index] = std::move(regions);
                  }
               }
               decision.read_recognized = !decision.read_regions.empty();
            }
         }
         if(decision.write_requested && decision.bundle_has_memory_write)
         {
            if(effective_burst_type != 1U)
            {
               decision.write_fallback_reason = effective_burst_type == 0U ?
                                                    "AXI burst type is FIXED; write coalescing is unsupported" :
                                                    "AXI burst type is unsupported; write coalescing is unavailable";
            }
            else if(!original_memory_effects_safe)
            {
               decision.write_fallback_reason = "opaque, volatile, atomic, or unclassified memory effects";
            }
            else if(cache_enabled)
            {
               decision.write_fallback_reason = "cache-enabled bundle";
            }
            else if(decision.bundle_has_memory_read && !frozen_proof_for_bundle(bundle_name))
            {
               decision.write_fallback_reason = "bundle contains reads; same-bundle read/write requires duplex support";
            }
            if(decision.write_fallback_reason.empty())
            {
               bool any_owner_recognized = false;
               for(const auto& owner : func_arch->parms)
               {
                  if(owner.second.at(FunctionArchitecture::parm_bundle) != bundle_name)
                  {
                     continue;
                  }
                  const auto owner_arg =
                      std::find_if(fd->list_of_args.begin(), fd->list_of_args.end(),
                                   [&](const auto& candidate) { return get_decl_name(candidate) == owner.first; });
                  if(owner_arg == fd->list_of_args.end() || !ir_helper::IsPointerType(ir_helper::CGetType(*owner_arg)))
                  {
                     continue;
                  }
                  std::vector<ir_nodeRef> stores;
                  const auto original_writes = decision.accesses_by_argument.find((*owner_arg)->index);
                  if(original_writes != decision.accesses_by_argument.end())
                  {
                     for(const auto& access : original_writes->second.writes)
                     {
                        if(access.ordinary)
                        {
                           stores.push_back(access.statement);
                        }
                     }
                  }
                  std::sort(stores.begin(), stores.end(),
                            [](const auto& lhs, const auto& rhs) { return lhs->index < rhs->index; });
                  stores.erase(std::unique(stores.begin(), stores.end(),
                                           [](const auto& lhs, const auto& rhs) { return lhs->index == rhs->index; }),
                               stores.end());
                  if(stores.empty())
                  {
                     continue;
                  }
                  if(!cross_bundle_burst_is_disjoint(bundle_name, false, (*owner_arg)->index))
                  {
                     decision.write_fallback_reason =
                         "reads through another m_axi bundle without a restrict contract on both "
                         "owners; cross-bundle write ordering is not proven";
                     continue;
                  }
                  const auto expected_base_id = AppM->getSSAFromParm(root_id, (*owner_arg)->index);
                  const auto expected_base = TM->GetIRNode(expected_base_id);
                  WriteBurstRegion region{};
                  const auto frozen_proof = frozen_proof_for_bundle(bundle_name);
                  bool recognized =
                      recognizeWriteBurstRegion(sl, stores, expected_base, burst_memory_accesses, bundle_name, region,
                                                frozen_proof ? frozen_proof->load_statement : ir_nodeRef());
                  if(recognized)
                  {
                     for(const auto& access : burst_memory_accesses)
                     {
                        if(access.is_write && access.statement->index != region.store->index)
                        {
                           recognized = false;
                           break;
                        }
                        const bool paired_duplex_read =
                            frozen_proof && access.statement->index == frozen_proof->load_statement->index &&
                            access.bundle == bundle_name && access.classified && access.ordinary && access.base &&
                            expected_base && access.base->index == frozen_proof->read_base->index;
                        if(!access.is_write && !paired_duplex_read &&
                           (!access.classified || !access.ordinary || !access.base || !expected_base ||
                            access.base->index == expected_base->index || access.bundle == bundle_name))
                        {
                           recognized = false;
                           break;
                        }
                     }
                  }
                  if(recognized)
                  {
                     decision.write_regions[(*owner_arg)->index] = std::move(region);
                     any_owner_recognized = true;
                  }
               }
               decision.write_recognized = any_owner_recognized && !decision.write_regions.empty();
            }
         }
         if(decision.read_requested && decision.write_requested && decision.bundle_has_memory_read &&
            decision.bundle_has_memory_write)
         {
            decision.same_base_duplex_profile_reason =
                cache_enabled ? "cache-enabled bundle" : "read/write profile is eligible";
            decision.same_base_duplex_profile_eligible = !cache_enabled;
            if(effective_burst_type != 1U)
            {
               decision.same_base_duplex_profile_eligible = false;
               decision.same_base_duplex_profile_reason =
                   effective_burst_type == 0U ?
                       "AXI burst type is FIXED; duplex burst engines require INCREMENTAL" :
                       "AXI burst type is unsupported; duplex burst engines require INCREMENTAL";
               decision.same_base_duplex_reason = decision.same_base_duplex_profile_reason;
               continue;
            }
            decision.same_base_duplex_reason = "original effects are not fully classified ordinary accesses";
            SameBaseDuplexProof proof{};
            if(RecognizeSameBaseDuplex(sl, fd, root_id, bundle_name, original_memory_accesses, burst_memory_accesses,
                                       original_memory_effects_safe, AppM, TM, func_arch, proof,
                                       decision.same_base_duplex_reason))
            {
               decision.same_base_duplex_proof = std::make_unique<SameBaseDuplexProof>(std::move(proof));
               decision.same_base_duplex_candidate = true;
            }
         }
         else if(decision.read_requested && decision.write_requested)
         {
            decision.same_base_duplex_reason = !decision.bundle_has_memory_read ? "no read access" : "no write access";
         }
      }
      for(auto& bundle_decision : burst_decisions)
      {
         const auto frozen_root = frozen_by_root.find(root_id);
         if(frozen_root != frozen_by_root.end())
         {
            const auto frozen = frozen_root->second.find(bundle_decision.first);
            if(frozen != frozen_root->second.end())
            {
               auto& decision = bundle_decision.second;
               const auto& proof = frozen->second.proof;
               THROW_ASSERT(decision.same_base_duplex_candidate && decision.same_base_duplex_proof,
                            "Frozen duplex proof disappeared before IR transformation for bundle " +
                                bundle_decision.first);
#if HAVE_ASSERTS
               const auto& current = *decision.same_base_duplex_proof;
#endif
               THROW_ASSERT(
                   current.owner_argument_index == proof.owner_argument_index &&
                       current.read_owner_argument_index == proof.read_owner_argument_index &&
                       current.write_owner_argument_index == proof.write_owner_argument_index &&
                       current.distinct_bases == proof.distinct_bases &&
                       current.runtime_range_guard == proof.runtime_range_guard &&
                       current.base->index == proof.base->index && current.read_base->index == proof.read_base->index &&
                       current.write_base->index == proof.write_base->index &&
                       current.load_statement->index == proof.load_statement->index &&
                       current.store_statement->index == proof.store_statement->index &&
                       current.read_region.preheader == proof.read_region.preheader &&
                       current.read_region.header == proof.read_region.header &&
                       ((!current.read_region.branch && !proof.read_region.branch) ||
                        (current.read_region.branch && proof.read_region.branch &&
                         current.read_region.branch->index == proof.read_region.branch->index)) &&
                       current.read_region.guard->index == proof.read_region.guard->index &&
                       current.read_region.bound->index == proof.read_region.bound->index &&
                       current.read_region.loop_trip_bound->index == proof.read_region.loop_trip_bound->index &&
                       current.read_region.loop_trip_compare->index == proof.read_region.loop_trip_compare->index &&
                       current.read_region.effective_bound->index == proof.read_region.effective_bound->index &&
                       ((!current.read_region.effective_compare && !proof.read_region.effective_compare) ||
                        (current.read_region.effective_compare && proof.read_region.effective_compare &&
                         current.read_region.effective_compare->index == proof.read_region.effective_compare->index)) &&
                       current.read_region.access_bb == proof.read_region.access_bb &&
                       current.read_region.has_direction_guard == proof.read_region.has_direction_guard &&
                       current.read_region.induction->index == proof.read_region.induction->index &&
                       current.read_region.base->index == proof.read_region.base->index &&
                       current.write_region.preheader == proof.write_region.preheader &&
                       current.write_region.header == proof.write_region.header &&
                       current.write_region.exit == proof.write_region.exit &&
                       current.write_region.guard->index == proof.write_region.guard->index &&
                       current.write_region.bound->index == proof.write_region.bound->index &&
                       current.write_region.loop_trip_bound->index == proof.write_region.loop_trip_bound->index &&
                       current.write_region.loop_trip_compare->index == proof.write_region.loop_trip_compare->index &&
                       current.write_region.effective_bound->index == proof.write_region.effective_bound->index &&
                       ((!current.write_region.effective_compare && !proof.write_region.effective_compare) ||
                        (current.write_region.effective_compare && proof.write_region.effective_compare &&
                         current.write_region.effective_compare->index ==
                             proof.write_region.effective_compare->index)) &&
                       current.write_region.access_bb == proof.write_region.access_bb &&
                       current.write_region.has_direction_guard == proof.write_region.has_direction_guard &&
                       current.write_region.induction->index == proof.write_region.induction->index &&
                       current.write_region.base->index == proof.write_region.base->index &&
                       current.write_region.store->index == proof.write_region.store->index,
                   "Original duplex proof snapshot changed before IR transformation for bundle " +
                       bundle_decision.first);
               decision.same_base_duplex_proof = std::make_unique<SameBaseDuplexProof>(proof);
               decision.read_regions[proof.read_owner_argument_index] = {proof.read_region};
               decision.write_regions[proof.write_owner_argument_index] = proof.write_region;
               decision.read_recognized = true;
               decision.write_recognized = true;
               decision.same_base_duplex_activated = true;
               decision.same_base_duplex_activation_reason = "frozen application-wide proof and profile accepted";
            }
         }
         if(!bundle_decision.second.same_base_duplex_activated)
         {
            const auto veto = bundle_vetoes.find(bundle_decision.first);
            if(veto != bundle_vetoes.end())
            {
               bundle_decision.second.same_base_duplex_activation_reason = veto->second;
            }
            else if(bundle_decision.second.same_base_duplex_candidate)
            {
               bundle_decision.second.same_base_duplex_activation_reason =
                   bundle_decision.first.rfind("DF_bambu_", 0) == 0 ?
                       "DATAFLOW/forwarded bundle ownership is unsupported" :
                   bundle_decision.second.same_base_duplex_profile_eligible ?
                       "bundle is not a globally eligible activation candidate" :
                       bundle_decision.second.same_base_duplex_profile_reason;
            }
            else if(bundle_decision.second.read_requested && bundle_decision.second.write_requested)
            {
               bundle_decision.second.same_base_duplex_activation_reason =
                   bundle_decision.second.same_base_duplex_reason;
            }
         }
      }
#ifndef NDEBUG
      for(const auto& bundle_decision : burst_decisions)
      {
         const auto& decision = bundle_decision.second;
         for(const auto& owner_accesses : decision.accesses_by_argument)
         {
            const auto owner_argument_index = owner_accesses.first;
            const auto read_region = decision.read_regions.find(owner_argument_index);
            const auto write_region = decision.write_regions.find(owner_argument_index);
            INDENT_DBG_MEX(
                DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                "AXI_BURST_PREANALYSIS bundle=" + bundle_decision.first +
                    " owner_argument_index=" + std::to_string(owner_argument_index) +
                    " read_accesses=" + std::to_string(owner_accesses.second.reads.size()) +
                    " write_accesses=" + std::to_string(owner_accesses.second.writes.size()) + " read_requested=" +
                    std::to_string(static_cast<unsigned int>(decision.read_requested)) + " read_recognized=" +
                    std::to_string(static_cast<unsigned int>(read_region != decision.read_regions.end())) +
                    " read_fallback=\"" +
                    (owner_accesses.second.reads.empty() || decision.read_fallback_reason.empty() ?
                         "-" :
                         decision.read_fallback_reason) +
                    "\" write_requested=" + std::to_string(static_cast<unsigned int>(decision.write_requested)) +
                    " write_recognized=" +
                    std::to_string(static_cast<unsigned int>(write_region != decision.write_regions.end())) +
                    " write_fallback=\"" +
                    (owner_accesses.second.writes.empty() || decision.write_fallback_reason.empty() ?
                         "-" :
                         decision.write_fallback_reason) +
                    "\" same_base_duplex_candidate=" +
                    std::to_string(static_cast<unsigned int>(decision.same_base_duplex_candidate)) +
                    " same_base_duplex_reason=\"" + decision.same_base_duplex_reason + "\" production_activated=" +
                    std::to_string(static_cast<unsigned int>(decision.same_base_duplex_activated)) +
                    " activation_reason=\"" + decision.same_base_duplex_activation_reason + "\"");
         }
         if(decision.read_requested && decision.write_requested)
         {
            const auto json_string = [](const std::string& value) {
               std::string escaped;
               escaped.reserve(value.size() + 2U);
               for(const auto character : value)
               {
                  if(character == '\\' || character == '"')
                  {
                     escaped.push_back('\\');
                  }
                  if(character == '\n')
                  {
                     escaped += "\\n";
                  }
                  else if(character == '\r')
                  {
                     escaped += "\\r";
                  }
                  else
                  {
                     escaped.push_back(character);
                  }
               }
               return escaped;
            };
            const auto node_id = [](const ir_nodeRef& node) {
               return node ? std::to_string(node->index) : std::string("null");
            };
            const auto node_id_or_zero = [](const ir_nodeRef& node) {
               return node ? std::to_string(node->index) : std::string("0");
            };
            std::ostringstream proof_record;
            proof_record << "AXI_BURST_DUPLEX_PROOF {\"bundle\":\"" << json_string(bundle_decision.first)
                         << "\",\"semantic_proof\":" << (decision.same_base_duplex_proof ? "true" : "false")
                         << ",\"semantic_reason\":\"" << json_string(decision.same_base_duplex_reason)
                         << "\",\"runtime_range_guard\":"
                         << (decision.same_base_duplex_proof && decision.same_base_duplex_proof->runtime_range_guard ?
                                 "true" :
                                 "false")
                         << ",\"profile_eligible\":" << (decision.same_base_duplex_profile_eligible ? "true" : "false")
                         << ",\"profile_reason\":\"" << json_string(decision.same_base_duplex_profile_reason) << "\"";
            if(decision.same_base_duplex_proof)
            {
               const auto& proof = *decision.same_base_duplex_proof;
               proof_record << ",\"proof_kind\":\""
                            << (proof.runtime_range_guard ? "distinct_base_runtime_guarded_pointwise" :
                                proof.distinct_bases      ? "distinct_base_restricted_pointwise" :
                                                            "same_base_pointwise")
                            << "\",\"owner_argument_index\":" << proof.owner_argument_index
                            << ",\"read_owner_argument_index\":" << proof.read_owner_argument_index
                            << ",\"write_owner_argument_index\":" << proof.write_owner_argument_index
                            << ",\"base_ssa_index\":" << node_id(proof.base)
                            << ",\"read_base_ssa_index\":" << node_id(proof.read_base)
                            << ",\"write_base_ssa_index\":" << node_id(proof.write_base)
                            << ",\"load_statement_index\":" << node_id(proof.load_statement)
                            << ",\"store_statement_index\":" << node_id(proof.store_statement)
                            << ",\"read_preheader_bb\":" << proof.read_region.preheader
                            << ",\"read_header_bb\":" << proof.read_region.header
                            << ",\"read_branch_index\":" << node_id(proof.read_region.branch)
                            << ",\"read_guard_index\":" << node_id(proof.read_region.guard)
                            << ",\"read_bound_index\":" << node_id(proof.read_region.bound)
                            << ",\"loop_trip_bound_ssa_index\":" << node_id(proof.read_region.loop_trip_bound)
                            << ",\"loop_trip_compare_index\":" << node_id(proof.read_region.loop_trip_compare)
                            << ",\"read_effective_bound_ssa_index\":" << node_id(proof.read_region.effective_bound)
                            << ",\"read_effective_compare_index\":"
                            << node_id_or_zero(proof.read_region.effective_compare)
                            << ",\"read_access_bb\":" << proof.read_region.access_bb << ",\"read_count_recipe\":\""
                            << (proof.read_region.has_direction_guard ? "min_trip_and_effective_bound" : "trip_count")
                            << "\""
                            << ",\"read_induction_index\":" << node_id(proof.read_region.induction)
                            << ",\"write_preheader_bb\":" << proof.write_region.preheader
                            << ",\"write_header_bb\":" << proof.write_region.header
                            << ",\"write_exit_bb\":" << proof.write_region.exit
                            << ",\"write_guard_index\":" << node_id(proof.write_region.guard)
                            << ",\"write_bound_index\":" << node_id(proof.write_region.bound)
                            << ",\"write_effective_bound_ssa_index\":" << node_id(proof.write_region.effective_bound)
                            << ",\"write_effective_compare_index\":"
                            << node_id_or_zero(proof.write_region.effective_compare)
                            << ",\"write_access_bb\":" << proof.write_region.access_bb << ",\"write_count_recipe\":\""
                            << (proof.write_region.has_direction_guard ? "min_trip_and_effective_bound" : "trip_count")
                            << "\",\"pairing_domain_recipe\":\"min_read_write_counts_prefix\""
                            << ",\"write_induction_index\":" << node_id(proof.write_region.induction);
            }
            proof_record << "}";
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, proof_record.str());
         }
      }
#endif
   }

   bool IsDataflowArchitecture(const FunctionArchitectureRef& arch)
   {
      const auto it = arch->attrs.find(FunctionArchitecture::func_dataflow_module);
      return it != arch->attrs.end() && it->second == "1";
   }

   bool GetDuplexContribution(const unsigned int root_id, const FunctionArchitectureRef& arch,
                              const std::string& bundle, const bool burst_feature_enabled,
                              const unsigned int effective_burst_type, const unsigned int address_bitsize,
                              const application_managerRef& AppM, const ir_managerRef& TM, const CallGraphManager& CGM,
                              FrozenDuplexContribution& contribution, std::string& reason)
   {
      if(!burst_feature_enabled)
      {
         reason = "m-axi-burst=0 opt-out";
         return false;
      }
      if(bundle.rfind("DF_bambu_", 0) == 0 || IsDataflowArchitecture(arch))
      {
         reason = "DATAFLOW/forwarded bundle ownership is unsupported";
         return false;
      }
      const auto call_graph_vertex = CGM.GetVertex(root_id);
      if(CGM.GetCallGraph().in_degree(call_graph_vertex) != 0)
      {
         reason = "bundle contribution is not an independent entry root";
         return false;
      }
      if(effective_burst_type != 1U)
      {
         reason = "AXI burst type is not INCREMENTAL";
         return false;
      }
      const auto iface_it = arch->ifaces.find(bundle);
      if(iface_it == arch->ifaces.end())
      {
         reason = "bundle has no explicit interface architecture";
         return false;
      }
      const auto& attrs = iface_it->second;
      const auto mode = attrs.find(FunctionArchitecture::iface_mode);
      const auto read_max = attrs.find(FunctionArchitecture::iface_max_read_burst_length);
      const auto write_max = attrs.find(FunctionArchitecture::iface_max_write_burst_length);
      if(mode == attrs.end() || mode->second != "m_axi" || read_max == attrs.end() || write_max == attrs.end())
      {
         reason = "bundle does not explicitly request both read and write bursts";
         return false;
      }
      if(attrs.count(FunctionArchitecture::iface_cache_line_count))
      {
         reason = "cache-enabled bundle";
         return false;
      }
      const auto fd = GetPointer<const function_val_node>(TM->GetIRNode(root_id));
      const auto sl = GetPointerS<const statement_list_node>(fd->body);
      std::vector<ir_nodeRef> bundle_pointer_args;
      for(const auto& argument : fd->list_of_args)
      {
         if(!ir_helper::IsPointerType(ir_helper::CGetType(argument)))
         {
            continue;
         }
         const auto parm = arch->parms.find(get_decl_name(argument));
         if(parm == arch->parms.end() || parm->second.count(FunctionArchitecture::parm_bundle) == 0)
         {
            reason = "original pointer-to-bundle mapping is incomplete";
            return false;
         }
         if(parm->second.at(FunctionArchitecture::parm_bundle) == bundle)
         {
            bundle_pointer_args.push_back(argument);
         }
      }
      if(bundle_pointer_args.empty() || bundle_pointer_args.size() > 2)
      {
         reason = "bundle must have one or two pointer arguments";
         return false;
      }
      for(const auto& owner : bundle_pointer_args)
      {
         const auto owner_type = ir_helper::CGetPointedType(ir_helper::CGetType(owner));
         if(!owner_type || (owner_type->get_kind() != integer_ty_node_K && owner_type->get_kind() != real_ty_node_K) ||
            ir_helper::Size(owner_type) != 32 || ir_helper::SizeAlloc(owner_type) != 32)
         {
            reason = "bundle element must be a direct 32-bit integer or floating-point element";
            return false;
         }
      }
      const auto bitwidth = attrs.find(FunctionArchitecture::iface_bitwidth);
      const auto alignment = attrs.find(FunctionArchitecture::iface_alignment);
      if((bitwidth != attrs.end() && !bitwidth->second.empty() && std::stoul(bitwidth->second) != 32U) ||
         (alignment != attrs.end() && !alignment->second.empty() && std::stoul(alignment->second) < 4U))
      {
         reason = "unsupported interface width or alignment";
         return false;
      }
      if(address_bitsize != 32U && address_bitsize != 64U)
      {
         reason = "AXI address width is not 32 or 64 bits";
         return false;
      }
      bool classified = false;
      const auto original_accesses = CollectOriginalMemoryAccesses(fd, root_id, arch, AppM, TM, classified);
      const auto original_safe = hasOnlyOrdinaryMemoryEffects(sl) && classified;
      SameBaseDuplexProof proof{};
      if(!RecognizeSameBaseDuplex(sl, fd, root_id, bundle, original_accesses, original_accesses, original_safe, AppM,
                                  TM, arch, proof, reason))
      {
         return false;
      }
      if(proof.runtime_range_guard)
      {
         const auto pointer_bits =
             static_cast<unsigned int>(ir_helper::SizeAlloc(ir_helper::CGetType(proof.read_base)));
         if((pointer_bits != 32U && pointer_bits != 64U) || pointer_bits > address_bitsize)
         {
            reason = "runtime alias guard requires a 32/64-bit pointer no wider than the AXI address";
            return false;
         }
      }
      const auto owner_is_bundled = [&](const unsigned int owner_index) {
         return std::any_of(bundle_pointer_args.begin(), bundle_pointer_args.end(),
                            [&](const auto& owner) { return owner->index == owner_index; });
      };
      if(!owner_is_bundled(proof.read_owner_argument_index) || !owner_is_bundled(proof.write_owner_argument_index))
      {
         reason = "proof owners differ from the bundle pointer arguments";
         return false;
      }
      contribution.root_id = root_id;
      contribution.owner_argument_index = proof.read_owner_argument_index;
      contribution.bundle = bundle;
      contribution.proof = std::move(proof);
      contribution.read_max_burst_length = static_cast<unsigned int>(std::stoul(read_max->second));
      contribution.read_profile = ParseAXIReadBurstProfile(attrs, bundle);
      contribution.write_profile = ParseAXIWriteBurstProfile(attrs, bundle);
      if(contribution.read_max_burst_length < 1U || contribution.read_max_burst_length > 256U)
      {
         reason = "unsupported duplex burst profile";
         return false;
      }
      contribution.address_bitsize = address_bitsize;
      contribution.latency = ParseAXIInterfaceLatency(attrs, bundle);
      return true;
   }
} // namespace InterfaceInferInternal
