/*
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
/**
 * @file AXIDuplexBinding.cpp
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#include "AXIDuplexBinding.hpp"

#include "HDLGeneratorManager.hpp"
#include "HLS/axi_interface_profile.hpp"
#include "Parameter.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "library_manager.hpp"
#include "structural_manager.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "utility.hpp"

#include <algorithm>
#include <cstdint>
#include <set>
#include <tuple>

namespace AXIDuplexBinding
{
   namespace
   {
      void require(const bool condition, const std::string& message)
      {
         if(!condition)
         {
            THROW_ERROR(message);
         }
      }

      const std::map<std::string, std::string> collect_metadata(const functional_unit* unit)
      {
         const auto role = unit->attributes.find("bambu_axi_duplex_role");
         if(role == unit->attributes.end())
         {
            if(unit->attributes.count("bambu_axi_duplex_aggregator") || unit->attributes.count("bambu_axi_bundle_name"))
            {
               THROW_ERROR("Duplex endpoint metadata has no bambu_axi_duplex_role");
            }
            return {};
         }
         if(!role->second)
         {
            THROW_ERROR("Duplex endpoint has null role metadata");
         }
         const std::vector<std::string> keys{
             "bambu_axi_bundle_name",        "bambu_axi_burst_mode",          "bambu_axi_duplex_aggregator",
             "bambu_axi_address_bitsize",    "bambu_axi_data_bitsize",        "bambu_axi_latency",
             "bambu_max_read_burst_length",  "bambu_num_read_outstanding",    "bambu_read_fifo_depth",
             "bambu_max_write_burst_length", "bambu_write_burst_outstanding", "bambu_write_fifo_depth"};
         std::map<std::string, std::string> result;
         result.emplace("bambu_axi_duplex_role", role->second->get_content_str());
         for(const auto& key : keys)
         {
            const auto it = unit->attributes.find(key);
            if(it == unit->attributes.end() || !it->second)
            {
               THROW_ERROR("Missing duplex endpoint metadata " + key);
            }
            result.emplace(key, it->second->get_content_str());
         }
         return result;
      }

      std::uint32_t parse_decimal(const std::string& value, const std::uint32_t minimum, const std::uint32_t maximum,
                                  const std::string& field)
      {
         std::uint32_t parsed = 0U;
         if(panda::utility::parse_axi_profile_decimal(value, minimum, maximum, parsed) !=
            panda::utility::bounded_decimal_result::valid)
         {
            THROW_ERROR("Invalid duplex metadata '" + field + "': expected a decimal integer in [" + STR(minimum) +
                        "," + STR(maximum) + "]");
         }
         return parsed;
      }
   } // namespace

   bool metadata::operator==(const metadata& other) const
   {
      return bundle == other.bundle && aggregator == other.aggregator && address_bitsize == other.address_bitsize &&
             data_bitsize == other.data_bitsize && latency == other.latency &&
             read_burst_length == other.read_burst_length && read_outstanding == other.read_outstanding &&
             read_fifo_depth == other.read_fifo_depth && write_burst_length == other.write_burst_length &&
             write_outstanding == other.write_outstanding && write_fifo_depth == other.write_fifo_depth;
   }

   std::map<std::string, std::string> metadata::technology_attributes() const
   {
      return {{"bambu_axi_bundle_name", bundle},
              {"bambu_axi_duplex_aggregator", aggregator},
              {"bambu_axi_address_bitsize", STR(address_bitsize)},
              {"bambu_axi_data_bitsize", STR(data_bitsize)},
              {"bambu_axi_latency", STR(latency)},
              {"bambu_max_read_burst_length", STR(read_burst_length)},
              {"bambu_num_read_outstanding", STR(read_outstanding)},
              {"bambu_read_fifo_depth", STR(read_fifo_depth)},
              {"bambu_max_write_burst_length", STR(write_burst_length)},
              {"bambu_write_burst_outstanding", STR(write_outstanding)},
              {"bambu_write_fifo_depth", STR(write_fifo_depth)}};
   }

   metadata parse_metadata(const std::map<std::string, std::string>& values)
   {
      const auto get = [&](const std::string& key) -> const std::string& {
         const auto it = values.find(key);
         if(it == values.end())
         {
            THROW_ERROR("Missing duplex endpoint metadata " + key);
         }
         return it->second;
      };
      const auto role_text = get("bambu_axi_duplex_role");
      const auto burst_mode = get("bambu_axi_burst_mode");
      metadata result{};
      if(role_text == "read_endpoint" && burst_mode == "read")
      {
         result.role = endpoint_role::read;
      }
      else if(role_text == "write_endpoint" && burst_mode == "write")
      {
         result.role = endpoint_role::write;
      }
      else
      {
         THROW_ERROR("Invalid duplex endpoint role or burst mode");
      }
      result.bundle = get("bambu_axi_bundle_name");
      result.aggregator = get("bambu_axi_duplex_aggregator");
      if(result.bundle.empty() || result.aggregator.empty())
      {
         THROW_ERROR("Duplex endpoint bundle and aggregator names must not be empty");
      }
      result.address_bitsize = parse_decimal(get("bambu_axi_address_bitsize"), 32U, 64U, "bambu_axi_address_bitsize");
      if(result.address_bitsize != 32U && result.address_bitsize != 64U)
      {
         THROW_ERROR("Duplex AXI address width must be 32 or 64 bits");
      }
      result.data_bitsize = parse_decimal(get("bambu_axi_data_bitsize"), 32U, 32U, "bambu_axi_data_bitsize");
      FunctionArchitecture::iface_attrs latency_attrs;
      latency_attrs[FunctionArchitecture::iface_latency] = get("bambu_axi_latency");
      result.latency = ParseAXIInterfaceLatency(latency_attrs, result.bundle);

      FunctionArchitecture::iface_attrs attrs;
      attrs[FunctionArchitecture::iface_max_read_burst_length] = get("bambu_max_read_burst_length");
      attrs[FunctionArchitecture::iface_num_read_outstanding] = get("bambu_num_read_outstanding");
      attrs[FunctionArchitecture::iface_read_fifo_depth] = get("bambu_read_fifo_depth");
      attrs[FunctionArchitecture::iface_max_write_burst_length] = get("bambu_max_write_burst_length");
      attrs[FunctionArchitecture::iface_write_burst_outstanding] = get("bambu_write_burst_outstanding");
      attrs[FunctionArchitecture::iface_write_fifo_depth] = get("bambu_write_fifo_depth");
      const auto read = ParseAXIReadBurstProfile(attrs, result.bundle);
      const auto write = ParseAXIWriteBurstProfile(attrs, result.bundle);
      result.read_burst_length = read.max_burst_length;
      result.read_outstanding = read.max_outstanding;
      result.read_fifo_depth = read.fifo_depth;
      result.write_burst_length = write.max_burst_length;
      result.write_outstanding = write.max_outstanding;
      result.write_fifo_depth = write.fifo_depth;
      return result;
   }

   bool collect_endpoint(const functional_unit* unit, const structural_objectRef& module, endpoint& result)
   {
      if(!unit || !module)
      {
         THROW_ERROR("Cannot collect a duplex endpoint without its technology and structural module");
      }
      const auto values = collect_metadata(unit);
      if(values.empty())
      {
         return false;
      }
      result.module = module;
      result.profile = parse_metadata(values);
      return true;
   }

   void add_endpoint(bundles& collected, endpoint value)
   {
      auto& group = collected[value.profile.bundle];
      auto& direction = value.profile.role == endpoint_role::read ? group.readers : group.writers;
      if(!direction.empty())
      {
         THROW_ERROR("Duplicate duplex endpoint for bundle " + value.profile.bundle);
      }
      direction.push_back(std::move(value));
   }

   std::vector<structural_objectRef> connect(const HLS_managerRef HLSMgr, const hlsRef HLS,
                                             const structural_objectRef& clock_port,
                                             const structural_objectRef& reset_port, const bundles& duplex_bundles,
                                             const ParameterConstRef parameters, unsigned int& unique_id)
   {
      std::vector<structural_objectRef> aggregators;
      const auto SM = HLS->datapath;
      const auto TechM = HLS->HLS_D->get_technology_manager();
      const auto circuit = SM->get_circ();
      const auto FB = HLSMgr->CGetFunctionBehavior(HLS->functionId);

      for(const auto& [bundle, endpoints] : duplex_bundles)
      {
         if(endpoints.readers.size() != 1U || endpoints.writers.size() != 1U)
         {
            THROW_ERROR("Duplex bundle " + bundle + " requires exactly one read and one write endpoint");
         }
         const auto& reader = endpoints.readers.front();
         const auto& writer = endpoints.writers.front();
         if(!(reader.profile == writer.profile) || reader.profile.bundle != bundle ||
            reader.profile.role != endpoint_role::read || writer.profile.role != endpoint_role::write)
         {
            THROW_ERROR("Duplex endpoint profiles differ for bundle " + bundle);
         }
         const auto& profile = reader.profile;
         const auto metadata = profile.technology_attributes();
         const auto aggregator_name = profile.aggregator;
         const auto aggregator_template = TechM->get_fu(aggregator_name, INTERFACE_LIBRARY);
         if(!aggregator_template)
         {
            THROW_ERROR("Missing duplex aggregator template " + aggregator_name);
         }
         const auto aggregator_technology = GetPointerS<const functional_unit>(aggregator_template);
         if(!aggregator_technology->CM)
         {
            THROW_ERROR("Missing duplex aggregator structural template " + aggregator_name);
         }
         const auto aggregator_np =
             GetPointerS<const module_o>(aggregator_technology->CM->get_circ())->get_NP_functionality();
         if(!aggregator_np ||
            aggregator_np->get_NP_functionality(NP_functionality::VERILOG_GENERATOR) != "DuplexMAXIHDLGenerator")
         {
            THROW_ERROR("Missing duplex HDL generator on template " + aggregator_name);
         }
         for(const auto& item : metadata)
         {
            const auto found = aggregator_technology->attributes.find(item.first);
            if(found == aggregator_technology->attributes.end() || !found->second ||
               found->second->get_content_str() != item.second)
            {
               THROW_ERROR("Duplex aggregator metadata mismatch for bundle " + bundle + " at " + item.first);
            }
         }
         const auto aggregator_role = aggregator_technology->attributes.find("bambu_axi_duplex_role");
         const auto aggregator_mode = aggregator_technology->attributes.find("bambu_axi_burst_mode");
         if(aggregator_role == aggregator_technology->attributes.end() || !aggregator_role->second ||
            aggregator_role->second->get_content_str() != "aggregator" ||
            aggregator_mode == aggregator_technology->attributes.end() || !aggregator_mode->second ||
            aggregator_mode->second->get_content_str() != "duplex")
         {
            THROW_ERROR("Invalid duplex aggregator role or mode for bundle " + bundle);
         }
         const auto address_bits = profile.address_bitsize;
         const auto data_bits = profile.data_bitsize;

         const auto validate_port = [&](const structural_objectRef& module, const std::string& name,
                                        const port_o::port_direction direction, const unsigned int width,
                                        const bool is_extern, const port_o::port_interface interface_tag) {
            const auto port = module->find_member(name, port_o_K, module);
            if(!port || GetPointerS<const port_o>(port)->get_port_direction() != direction ||
               GET_TYPE_SIZE(GetPointerS<const port_o>(port)) != width ||
               GetPointerS<const port_o>(port)->get_is_extern() != is_extern ||
               GetPointerS<const port_o>(port)->get_port_interface() != interface_tag)
            {
               THROW_ERROR("Invalid duplex port " + name + " on " + module->get_id() + " for bundle " + bundle);
            }
            return port;
         };
         const auto connect_command = [&](const structural_objectRef& endpoint, const std::string& endpoint_name,
                                          const structural_objectRef& aggregator, const std::string& aggregator_port,
                                          const port_o::port_direction endpoint_direction,
                                          const port_o::port_direction aggregator_direction, const unsigned int width) {
            const auto source = validate_port(endpoint, endpoint_name, endpoint_direction, width, true,
                                              port_o::port_interface::PI_DEFAULT);
            const auto destination = validate_port(aggregator, aggregator_port, aggregator_direction, width, false,
                                                   port_o::port_interface::PI_DEFAULT);
            if(endpoint == aggregator || endpoint->get_owner() != circuit || aggregator->get_owner() != circuit ||
               source->get_owner() != endpoint || destination->get_owner() != aggregator)
            {
               THROW_ERROR("Duplex ports must belong to sibling modules for bundle " + bundle);
            }
            if(GetPointerS<const port_o>(source)->get_connected_signal() ||
               GetPointerS<const port_o>(destination)->get_connected_signal())
            {
               THROW_ERROR("Duplex ports must be initially unconnected for bundle " + bundle);
            }

            structural_objectRef producer;
            structural_objectRef consumer;
            if(endpoint_direction == port_o::OUT && aggregator_direction == port_o::IN)
            {
               producer = source;
               consumer = destination;
            }
            else
            {
               if(endpoint_direction != port_o::IN || aggregator_direction != port_o::OUT)
               {
                  THROW_ERROR("Invalid duplex producer/consumer directions for bundle " + bundle);
               }
               producer = destination;
               consumer = source;
            }

            const auto signal = SM->add_sign("duplex_" + STR(unique_id++), circuit, producer->get_typeRef());
            SM->add_connection(signal, producer);
            SM->add_connection(signal, consumer);
            if(GetPointerS<const port_o>(producer)->get_connected_signal() != signal ||
               GetPointerS<const port_o>(consumer)->get_connected_signal() != signal ||
               GetPointerS<const port_o>(producer)->find_bounded_object(circuit) != signal ||
               GetPointerS<const port_o>(consumer)->find_bounded_object(circuit) != signal)
            {
               THROW_ERROR("Duplex ports do not resolve to the same circuit signal for bundle " + bundle);
            }
         };
         const HDLGeneratorManagerRef HDLGen(new HDLGeneratorManager(HLSMgr, parameters));
         const auto specialized_name = aggregator_name + "_modgen";
         if(GetPointerS<const module_o>(aggregator_technology->CM->get_circ())->has_to_be_generated())
         {
            HDLGen->create_generic_module(aggregator_name, gc_null_vertex(), FB, INTERFACE_LIBRARY, specialized_name);
         }
         const auto specialized_technology = TechM->get_fu(specialized_name, INTERFACE_LIBRARY);
         if(!specialized_technology)
         {
            THROW_ERROR("Failed to specialize duplex aggregator " + aggregator_name);
         }
         if(!GetPointerS<const functional_unit>(specialized_technology)->CM)
         {
            THROW_ERROR("Missing specialized duplex aggregator structure " + specialized_name);
         }
         const auto specialized_module =
             GetPointerS<const module_o>(GetPointerS<const functional_unit>(specialized_technology)->CM->get_circ());
         for(const auto& item : metadata)
         {
            const auto& attributes = specialized_module->get_attribute_list();
            const auto found = std::find(attributes.begin(), attributes.end(), item.first);
            const auto attribute =
                found == attributes.end() ? attributeRef() : specialized_module->get_attribute(item.first);
            if(!attribute || attribute->get_content_str() != item.second)
            {
               THROW_ERROR("Specialized duplex metadata mismatch for bundle " + bundle + " at " + item.first);
            }
         }
         const auto aggregator = SM->add_module_from_technology_library(
             "duplex_aggregator_" + STR(unique_id++), specialized_name, INTERFACE_LIBRARY, circuit, TechM);
         const auto clock =
             validate_port(aggregator, CLOCK_PORT_NAME, port_o::IN, 1, false, port_o::port_interface::PI_DEFAULT);
         const auto reset =
             validate_port(aggregator, RESET_PORT_NAME, port_o::IN, 1, false, port_o::port_interface::PI_DEFAULT);
         if(!clock_port || !reset_port || GetPointerS<const port_o>(clock_port)->get_port_direction() != port_o::IN ||
            GetPointerS<const port_o>(reset_port)->get_port_direction() != port_o::IN ||
            GET_TYPE_SIZE(GetPointerS<const port_o>(clock_port)) != 1U ||
            GET_TYPE_SIZE(GetPointerS<const port_o>(reset_port)) != 1U)
         {
            THROW_ERROR("Invalid duplex clock/reset connection for bundle " + bundle);
         }
         SM->add_connection(clock, clock_port);
         SM->add_connection(reset, reset_port);

         const std::vector<std::tuple<std::string, unsigned int>> command_fields{
             {"start", 1U}, {"opcode", 2U}, {"size", 32U}, {"data", data_bits}, {"address", address_bits},
             {"count", 32U}};
         for(const auto& [field, width] : command_fields)
         {
            connect_command(reader.module, "duplex_" + field, aggregator, "read_" + field, port_o::OUT, port_o::IN,
                            width);
            connect_command(writer.module, "duplex_" + field, aggregator, "write_" + field, port_o::OUT, port_o::IN,
                            width);
         }
         connect_command(reader.module, "duplex_done", aggregator, "read_done", port_o::IN, port_o::OUT, 1U);
         connect_command(reader.module, "duplex_result", aggregator, "read_result", port_o::IN, port_o::OUT, data_bits);
         connect_command(writer.module, "duplex_done", aggregator, "write_done", port_o::IN, port_o::OUT, 1U);
         connect_command(writer.module, "duplex_result", aggregator, "write_result", port_o::IN, port_o::OUT,
                         data_bits);

         unsigned int cache_reset_ports = 0U;
         for(unsigned int port_index = 0; port_index < GetPointerS<const module_o>(aggregator)->get_in_port_size();
             ++port_index)
         {
            const auto port = GetPointerS<const module_o>(aggregator)->get_in_port(port_index);
            const auto port_info = GetPointerS<const port_o>(port);
            if(port_info->get_port_interface() == port_o::port_interface::CACHE_RESET)
            {
               ++cache_reset_ports;
               require(port->get_id() == CACHE_RESET_PORT_NAME && port_info->get_port_direction() == port_o::IN &&
                           GET_TYPE_SIZE(port_info) == 1U && !port_info->get_is_extern(),
                       "Invalid duplex cache_reset port for bundle " + bundle);
            }
         }
         require(cache_reset_ports == 1U,
                 "Duplex aggregator must expose exactly one cache_reset input for bundle " + bundle);

         std::set<int> physical_interface_tags;
         for(unsigned int port_index = 0; port_index < GetPointerS<const module_o>(aggregator)->get_in_port_size();
             ++port_index)
         {
            const auto port = GetPointerS<const module_o>(aggregator)->get_in_port(port_index);
            const auto tag = GetPointerS<const port_o>(port)->get_port_interface();
            if(tag >= port_o::port_interface::M_AXI_AWVALID && tag <= port_o::port_interface::M_AXI_BUSER)
            {
               require(physical_interface_tags.insert(static_cast<int>(tag)).second,
                       "Duplicate physical M_AXI interface tag in duplex aggregator for bundle " + bundle);
            }
         }
         for(unsigned int port_index = 0; port_index < GetPointerS<const module_o>(aggregator)->get_out_port_size();
             ++port_index)
         {
            const auto port = GetPointerS<const module_o>(aggregator)->get_out_port(port_index);
            const auto tag = GetPointerS<const port_o>(port)->get_port_interface();
            if(tag >= port_o::port_interface::M_AXI_AWVALID && tag <= port_o::port_interface::M_AXI_BUSER)
            {
               require(physical_interface_tags.insert(static_cast<int>(tag)).second,
                       "Duplicate physical M_AXI interface tag in duplex aggregator for bundle " + bundle);
            }
         }
         require(!physical_interface_tags.empty(),
                 "Duplex aggregator has no physical M_AXI ports for bundle " + bundle);
         const std::set<port_o::port_interface> required_physical_tags{
             port_o::port_interface::M_AXI_ARADDR,   port_o::port_interface::M_AXI_ARBURST,
             port_o::port_interface::M_AXI_ARCACHE,  port_o::port_interface::M_AXI_ARID,
             port_o::port_interface::M_AXI_ARLEN,    port_o::port_interface::M_AXI_ARLOCK,
             port_o::port_interface::M_AXI_ARPROT,   port_o::port_interface::M_AXI_ARQOS,
             port_o::port_interface::M_AXI_ARREADY,  port_o::port_interface::M_AXI_ARREGION,
             port_o::port_interface::M_AXI_ARSIZE,   port_o::port_interface::M_AXI_ARUSER,
             port_o::port_interface::M_AXI_ARVALID,  port_o::port_interface::M_AXI_AWADDR,
             port_o::port_interface::M_AXI_AWBURST,  port_o::port_interface::M_AXI_AWCACHE,
             port_o::port_interface::M_AXI_AWID,     port_o::port_interface::M_AXI_AWLEN,
             port_o::port_interface::M_AXI_AWLOCK,   port_o::port_interface::M_AXI_AWPROT,
             port_o::port_interface::M_AXI_AWQOS,    port_o::port_interface::M_AXI_AWREADY,
             port_o::port_interface::M_AXI_AWREGION, port_o::port_interface::M_AXI_AWSIZE,
             port_o::port_interface::M_AXI_AWUSER,   port_o::port_interface::M_AXI_AWVALID,
             port_o::port_interface::M_AXI_BID,      port_o::port_interface::M_AXI_BREADY,
             port_o::port_interface::M_AXI_BRESP,    port_o::port_interface::M_AXI_BUSER,
             port_o::port_interface::M_AXI_BVALID,   port_o::port_interface::M_AXI_RDATA,
             port_o::port_interface::M_AXI_RID,      port_o::port_interface::M_AXI_RLAST,
             port_o::port_interface::M_AXI_RREADY,   port_o::port_interface::M_AXI_RRESP,
             port_o::port_interface::M_AXI_RUSER,    port_o::port_interface::M_AXI_RVALID,
             port_o::port_interface::M_AXI_WDATA,    port_o::port_interface::M_AXI_WLAST,
             port_o::port_interface::M_AXI_WREADY,   port_o::port_interface::M_AXI_WSTRB,
             port_o::port_interface::M_AXI_WUSER,    port_o::port_interface::M_AXI_WVALID};
         const std::set<port_o::port_interface> physical_input_tags{
             port_o::port_interface::M_AXI_AWREADY, port_o::port_interface::M_AXI_WREADY,
             port_o::port_interface::M_AXI_BVALID,  port_o::port_interface::M_AXI_BRESP,
             port_o::port_interface::M_AXI_BID,     port_o::port_interface::M_AXI_BUSER,
             port_o::port_interface::M_AXI_ARREADY, port_o::port_interface::M_AXI_RVALID,
             port_o::port_interface::M_AXI_RDATA,   port_o::port_interface::M_AXI_RLAST,
             port_o::port_interface::M_AXI_RID,     port_o::port_interface::M_AXI_RUSER,
             port_o::port_interface::M_AXI_RRESP};
         const auto expected_physical_width = [&](const port_o::port_interface tag) {
            using port_interface = port_o::port_interface;
            if(tag == port_interface::M_AXI_AWADDR || tag == port_interface::M_AXI_ARADDR)
               return address_bits;
            if(tag == port_interface::M_AXI_WDATA || tag == port_interface::M_AXI_RDATA)
               return data_bits;
            if(tag == port_interface::M_AXI_WSTRB)
               return data_bits / 8U;
            if(tag == port_interface::M_AXI_AWLEN || tag == port_interface::M_AXI_ARLEN)
               return 8U;
            if(tag == port_interface::M_AXI_AWSIZE || tag == port_interface::M_AXI_ARSIZE ||
               tag == port_interface::M_AXI_AWPROT || tag == port_interface::M_AXI_ARPROT)
               return 3U;
            if(tag == port_interface::M_AXI_AWBURST || tag == port_interface::M_AXI_ARBURST ||
               tag == port_interface::M_AXI_BRESP || tag == port_interface::M_AXI_RRESP)
               return 2U;
            if(tag == port_interface::M_AXI_AWCACHE || tag == port_interface::M_AXI_ARCACHE ||
               tag == port_interface::M_AXI_AWQOS || tag == port_interface::M_AXI_ARQOS ||
               tag == port_interface::M_AXI_AWREGION || tag == port_interface::M_AXI_ARREGION)
               return 4U;
            if(tag == port_interface::M_AXI_AWVALID || tag == port_interface::M_AXI_AWREADY ||
               tag == port_interface::M_AXI_WVALID || tag == port_interface::M_AXI_WREADY ||
               tag == port_interface::M_AXI_WLAST || tag == port_interface::M_AXI_BREADY ||
               tag == port_interface::M_AXI_BVALID || tag == port_interface::M_AXI_ARVALID ||
               tag == port_interface::M_AXI_ARREADY || tag == port_interface::M_AXI_RVALID ||
               tag == port_interface::M_AXI_RREADY || tag == port_interface::M_AXI_RLAST)
               return 1U;
            if(tag == port_interface::M_AXI_AWID || tag == port_interface::M_AXI_ARID ||
               tag == port_interface::M_AXI_BID || tag == port_interface::M_AXI_RID || tag == port_interface::M_AXI_WID)
               return 6U;
            return 1U;
         };
         const auto validate_physical_port = [&](const structural_objectRef& port) {
            const auto port_info = GetPointerS<const port_o>(port);
            const auto tag = port_info->get_port_interface();
            const std::set<port_o::port_interface> vector_tags{
                port_o::port_interface::M_AXI_AWID,   port_o::port_interface::M_AXI_AWLOCK,
                port_o::port_interface::M_AXI_AWUSER, port_o::port_interface::M_AXI_WSTRB,
                port_o::port_interface::M_AXI_WUSER,  port_o::port_interface::M_AXI_BID,
                port_o::port_interface::M_AXI_BUSER,  port_o::port_interface::M_AXI_ARID,
                port_o::port_interface::M_AXI_ARLOCK, port_o::port_interface::M_AXI_ARUSER,
                port_o::port_interface::M_AXI_RID,    port_o::port_interface::M_AXI_RUSER,
                port_o::port_interface::M_AXI_WID};
            const auto expected_kind = vector_tags.count(tag) ? port_vector_o_K : port_o_K;
            require(tag >= port_o::port_interface::M_AXI_AWVALID && tag <= port_o::port_interface::M_AXI_BUSER &&
                        port_info->get_port_direction() ==
                            (physical_input_tags.count(tag) ? port_o::IN : port_o::OUT) &&
                        port->get_kind() == expected_kind && GET_TYPE_SIZE(port_info) == expected_physical_width(tag) &&
                        (expected_kind == port_o_K || port_info->get_ports_size() == 1U),
                    "Invalid physical M_AXI port shape: " + port->get_id() + " for bundle " + bundle);
         };
         for(const auto expected_tag : required_physical_tags)
         {
            require(physical_interface_tags.count(static_cast<int>(expected_tag)) == 1U,
                    "Missing physical M_AXI interface tag in duplex aggregator for bundle " + bundle);
         }
         for(unsigned int port_index = 0; port_index < GetPointerS<const module_o>(aggregator)->get_in_port_size();
             ++port_index)
         {
            const auto port = GetPointerS<const module_o>(aggregator)->get_in_port(port_index);
            const auto tag = GetPointerS<const port_o>(port)->get_port_interface();
            if(tag >= port_o::port_interface::M_AXI_AWVALID && tag <= port_o::port_interface::M_AXI_BUSER)
            {
               validate_physical_port(port);
               require(!circuit->find_member(port->get_id(), port_o_K, circuit) &&
                           !circuit->find_member(port->get_id(), port_vector_o_K, circuit),
                       "Physical M_AXI pin already mapped for duplex bundle " + bundle + ": " + port->get_id());
            }
         }
         for(unsigned int port_index = 0; port_index < GetPointerS<const module_o>(aggregator)->get_out_port_size();
             ++port_index)
         {
            const auto port = GetPointerS<const module_o>(aggregator)->get_out_port(port_index);
            const auto tag = GetPointerS<const port_o>(port)->get_port_interface();
            if(tag >= port_o::port_interface::M_AXI_AWVALID && tag <= port_o::port_interface::M_AXI_BUSER)
            {
               validate_physical_port(port);
               require(!circuit->find_member(port->get_id(), port_o_K, circuit) &&
                           !circuit->find_member(port->get_id(), port_vector_o_K, circuit),
                       "Physical M_AXI pin already mapped for duplex bundle " + bundle + ": " + port->get_id());
            }
         }
         aggregators.push_back(aggregator);
      }
      return aggregators;
   }
} // namespace AXIDuplexBinding
