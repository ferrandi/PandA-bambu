/*
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#include "DuplexMAXIHDLGenerator.hpp"

#include "BambuParameter.hpp"
#include "HDL_manager.hpp"
#include "Parameter.hpp"
#include "constant_strings.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "library_manager.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <utility>

namespace
{
   struct DuplexProfile
   {
      std::string bundle;
      unsigned int address_bits;
      unsigned int latency;
      unsigned int read_burst;
      unsigned int read_outstanding;
      unsigned int read_fifo;
      unsigned int write_burst;
      unsigned int write_outstanding;
      unsigned int write_fifo;
   };

   std::string read_attribute(const structural_objectRef& mod, const std::string& name)
   {
      const auto& attrs = mod->get_attribute_list();
      if(std::find(attrs.begin(), attrs.end(), name) == attrs.end())
      {
         THROW_ERROR_USAGE("Duplex m_axi aggregator is missing required metadata '" + name + "'.");
      }
      const auto value = mod->get_attribute(name);
      if(!value || value->get_content_str().empty())
      {
         THROW_ERROR_USAGE("Duplex m_axi aggregator has empty metadata '" + name + "'.");
      }
      return value->get_content_str();
   }

   unsigned int parse_decimal(const structural_objectRef& mod, const std::string& name,
                              const unsigned int minimum, const unsigned int maximum)
   {
      const auto text = read_attribute(mod, name);
      unsigned int value = 0;
      for(const char ch : text)
      {
         if(ch < '0' || ch > '9')
         {
            THROW_ERROR_USAGE("Duplex m_axi aggregator metadata '" + name + "' must be an ASCII decimal integer.");
         }
         const auto digit = static_cast<unsigned int>(ch - '0');
         if(value > (maximum - digit) / 10U)
         {
            THROW_ERROR_USAGE("Duplex m_axi aggregator metadata '" + name + "' is out of range.");
         }
         value = value * 10U + digit;
      }
      if(value < minimum || value > maximum)
      {
         THROW_ERROR_USAGE("Duplex m_axi aggregator metadata '" + name + "' is out of range.");
      }
      return value;
   }

   bool is_power_of_two(const unsigned int value)
   {
      return value && (value & (value - 1U)) == 0U;
   }

   DuplexProfile load_profile(const structural_objectRef& mod)
   {
      if(read_attribute(mod, "bambu_axi_duplex_role") != "aggregator" ||
         read_attribute(mod, "bambu_axi_burst_mode") != "duplex")
      {
         THROW_ERROR_USAGE("DuplexMAXIHDLGenerator requires aggregator role and duplex burst mode metadata.");
      }
      const auto bundle = read_attribute(mod, "bambu_axi_bundle_name");
      const auto aggregator = read_attribute(mod, "bambu_axi_duplex_aggregator");
      const auto expected_aggregator = bundle + STR_CST_interface_parameter_keyword + "_duplex_aggregator";
      if(aggregator != expected_aggregator)
      {
         THROW_ERROR_USAGE("Duplex m_axi aggregator metadata is inconsistent with bundle '" + bundle + "'.");
      }
      if(parse_decimal(mod, "bambu_axi_data_bitsize", 32U, 32U) != 32U)
      {
         THROW_ERROR_USAGE("Duplex m_axi aggregator supports only 32-bit data.");
      }
      DuplexProfile profile{bundle,
                            parse_decimal(mod, "bambu_axi_address_bitsize", 32U, 64U),
                            parse_decimal(mod, "bambu_axi_latency", 0U, UINT32_MAX),
                            parse_decimal(mod, "bambu_max_read_burst_length", 1U, 256U),
                            parse_decimal(mod, "bambu_num_read_outstanding", 1U, 16U),
                            parse_decimal(mod, "bambu_read_fifo_depth", 1U, 4096U),
                            parse_decimal(mod, "bambu_max_write_burst_length", 1U, 256U),
                            parse_decimal(mod, "bambu_write_burst_outstanding", 1U, 16U),
                            parse_decimal(mod, "bambu_write_fifo_depth", 1U, 4096U)};
      if(profile.address_bits != 32U && profile.address_bits != 64U)
      {
         THROW_ERROR_USAGE("Duplex m_axi address width must be 32 or 64 bits.");
      }
      if(!is_power_of_two(profile.read_fifo) || !is_power_of_two(profile.write_fifo) ||
         profile.write_fifo < profile.write_burst)
      {
         THROW_ERROR_USAGE("Duplex m_axi FIFO depths must be powers of two; write_fifo_depth must be at least "
                           "max_write_burst_length.");
      }
      return profile;
   }

   structural_objectRef add_duplex_port(const structural_objectRef& mod, const std::string& name,
                                        const port_o::port_direction direction, const unsigned int width,
                                        const port_o::port_interface port_interface, const bool vector = false)
   {
      const structural_type_descriptorRef type(new structural_type_descriptor("bool", width));
      const auto port = vector ? structural_manager::add_port_vector(name, direction, 1U, mod, type) :
                                structural_manager::add_port(name, direction, mod, type);
      GetPointerS<port_o>(port)->set_port_interface(port_interface);
      return port;
   }

   std::string axi_name(const std::string& bundle, const std::string& signal)
   {
      return "_m_axi_" + bundle + "_" + signal;
   }
} // namespace

DuplexMAXIHDLGenerator::DuplexMAXIHDLGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void DuplexMAXIHDLGenerator::InternalExec(std::ostream& out, structural_objectRef mod, unsigned int,
                                          gc_vertex_descriptor, const HDLWriter_Language language,
                                          const std::vector<HDLGenerator::parameter>&,
                                          const std::vector<HDLGenerator::parameter>&,
                                          const std::vector<HDLGenerator::parameter>&,
                                          const std::vector<HDLGenerator::parameter>&)
{
   if(language != HDLWriter_Language::VERILOG)
   {
      THROW_ERROR_USAGE("DuplexMAXIHDLGenerator supports Verilog only.");
   }

   const auto profile = load_profile(mod);
   const auto scalar = port_o::port_interface::PI_DEFAULT;
   const auto input = port_o::IN;
   const auto output = port_o::OUT;
   const auto bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   const auto clock_port = structural_manager::add_port(CLOCK_PORT_NAME, input, mod, bool_type);
   GetPointerS<port_o>(clock_port)->set_is_clock(true);
   structural_manager::add_port(RESET_PORT_NAME, input, mod, bool_type);
   add_duplex_port(mod, CACHE_RESET_PORT_NAME, input, 0U, port_o::port_interface::CACHE_RESET);

   for(const auto& direction : {std::string("read"), std::string("write")})
   {
      add_duplex_port(mod, direction + "_start", input, 1U, scalar);
      add_duplex_port(mod, direction + "_opcode", input, 2U, scalar);
      add_duplex_port(mod, direction + "_size", input, 32U, scalar);
      add_duplex_port(mod, direction + "_data", input, 32U, scalar);
      add_duplex_port(mod, direction + "_address", input, profile.address_bits, scalar);
      add_duplex_port(mod, direction + "_count", input, 32U, scalar);
      add_duplex_port(mod, direction + "_done", output, 1U, scalar);
      add_duplex_port(mod, direction + "_result", output, 32U, scalar);
   }

   struct Pin
   {
      std::string suffix;
      port_o::port_direction direction;
      unsigned int width;
      port_o::port_interface interface;
      bool vector;
   };
   const std::array<Pin, 44> pins{{
       {"awready", input, 0, port_o::port_interface::M_AXI_AWREADY, false},
       {"wready", input, 0, port_o::port_interface::M_AXI_WREADY, false},
       {"bid", input, 6, port_o::port_interface::M_AXI_BID, true},
       {"bresp", input, 2, port_o::port_interface::M_AXI_BRESP, false},
       {"buser", input, 1, port_o::port_interface::M_AXI_BUSER, true},
       {"bvalid", input, 0, port_o::port_interface::M_AXI_BVALID, false},
       {"arready", input, 0, port_o::port_interface::M_AXI_ARREADY, false},
       {"rid", input, 6, port_o::port_interface::M_AXI_RID, true},
       {"rdata", input, 32, port_o::port_interface::M_AXI_RDATA, false},
       {"rresp", input, 2, port_o::port_interface::M_AXI_RRESP, false},
       {"rlast", input, 0, port_o::port_interface::M_AXI_RLAST, false},
       {"ruser", input, 1, port_o::port_interface::M_AXI_RUSER, true},
       {"rvalid", input, 0, port_o::port_interface::M_AXI_RVALID, false},
       {"awid", output, 6, port_o::port_interface::M_AXI_AWID, true},
       {"awaddr", output, profile.address_bits, port_o::port_interface::M_AXI_AWADDR, false},
       {"awlen", output, 8, port_o::port_interface::M_AXI_AWLEN, false},
       {"awsize", output, 3, port_o::port_interface::M_AXI_AWSIZE, false},
       {"awburst", output, 2, port_o::port_interface::M_AXI_AWBURST, false},
       {"awlock", output, 1, port_o::port_interface::M_AXI_AWLOCK, true},
       {"awcache", output, 4, port_o::port_interface::M_AXI_AWCACHE, false},
       {"awprot", output, 3, port_o::port_interface::M_AXI_AWPROT, false},
       {"awqos", output, 4, port_o::port_interface::M_AXI_AWQOS, false},
       {"awregion", output, 4, port_o::port_interface::M_AXI_AWREGION, false},
       {"awuser", output, 1, port_o::port_interface::M_AXI_AWUSER, true},
       {"awvalid", output, 0, port_o::port_interface::M_AXI_AWVALID, false},
       {"wdata", output, 32, port_o::port_interface::M_AXI_WDATA, false},
       {"wstrb", output, 4, port_o::port_interface::M_AXI_WSTRB, true},
       {"wlast", output, 0, port_o::port_interface::M_AXI_WLAST, false},
       {"wuser", output, 1, port_o::port_interface::M_AXI_WUSER, true},
       {"wvalid", output, 0, port_o::port_interface::M_AXI_WVALID, false},
       {"bready", output, 0, port_o::port_interface::M_AXI_BREADY, false},
       {"arid", output, 6, port_o::port_interface::M_AXI_ARID, true},
       {"araddr", output, profile.address_bits, port_o::port_interface::M_AXI_ARADDR, false},
       {"arlen", output, 8, port_o::port_interface::M_AXI_ARLEN, false},
       {"arsize", output, 3, port_o::port_interface::M_AXI_ARSIZE, false},
       {"arburst", output, 2, port_o::port_interface::M_AXI_ARBURST, false},
       {"arlock", output, 1, port_o::port_interface::M_AXI_ARLOCK, true},
       {"arcache", output, 4, port_o::port_interface::M_AXI_ARCACHE, false},
       {"arprot", output, 3, port_o::port_interface::M_AXI_ARPROT, false},
       {"arqos", output, 4, port_o::port_interface::M_AXI_ARQOS, false},
       {"arregion", output, 4, port_o::port_interface::M_AXI_ARREGION, false},
       {"aruser", output, 1, port_o::port_interface::M_AXI_ARUSER, true},
       {"arvalid", output, 0, port_o::port_interface::M_AXI_ARVALID, false},
       {"rready", output, 0, port_o::port_interface::M_AXI_RREADY, false},
   }};
   for(const auto& pin : pins)
   {
      const auto port_name = axi_name(profile.bundle, pin.suffix);
      const auto port = add_duplex_port(mod, port_name, pin.direction, pin.width, pin.interface, pin.vector);
      if(pin.suffix == "awaddr" || pin.suffix == "araddr")
      {
         GetPointerS<port_o>(port)->set_is_addr_bus(true);
      }
   }

   const auto p = [&](const std::string& signal) {
      return HDL_manager::convert_to_identifier(axi_name(profile.bundle, signal));
   };
   const auto reset_level = HLSMgr->get_parameter()->getOption<bool>(OPT_reset_level);
   out << "localparam integer BUNDLE_AXI_LATENCY = " << profile.latency << ";\n"
       << "wire reset_cache = " << (reset_level ? "(reset || cache_reset)" : "(reset && !cache_reset)") << ";\n"
       << "wire read_core_done, read_core_fault;\n"
       << "wire [31:0] read_core_result;\n"
       << "wire [" << profile.address_bits - 1 << ":0] read_core_araddr;\n"
       << "wire [7:0] read_core_arlen;\n"
       << "wire [2:0] read_core_arsize;\n"
       << "wire [1:0] read_core_arburst;\n"
       << "wire read_core_arid, read_core_arvalid, read_core_rready;\n"
       << "wire write_core_done, write_core_fault;\n"
       << "wire [31:0] write_core_result, write_core_wdata;\n"
       << "wire [" << profile.address_bits - 1 << ":0] write_core_awaddr;\n"
       << "wire [7:0] write_core_awlen;\n"
       << "wire [2:0] write_core_awsize;\n"
       << "wire [1:0] write_core_awburst;\n"
       << "wire write_core_awid, write_core_awvalid, write_core_wlast, write_core_wvalid, write_core_bready;\n"
       << "wire [3:0] write_core_wstrb;\n"
       << "assign read_done = read_core_done;\n"
       << "assign read_result = read_core_result;\n"
       << "assign write_done = write_core_done;\n"
       << "assign write_result = write_core_result;\n"
       << "assign " << p("araddr") << " = read_core_araddr;\n"
       << "assign " << p("arlen") << " = read_core_arlen;\n"
       << "assign " << p("arsize") << " = read_core_arsize;\n"
       << "assign " << p("arburst") << " = read_core_arburst;\n"
       << "assign " << p("arid") << " = {5'b0, read_core_arid};\n"
       << "assign " << p("arvalid") << " = read_core_arvalid;\n"
       << "assign " << p("rready") << " = read_core_rready;\n"
       << "assign " << p("arcache") << " = 4'b0010;\n"
       << "assign " << p("arlock") << " = 1'b0;\n"
       << "assign " << p("arprot") << " = 3'b0;\n"
       << "assign " << p("arqos") << " = 4'b0;\n"
       << "assign " << p("arregion") << " = 4'b0;\n"
       << "assign " << p("aruser") << " = 1'b0;\n"
       << "assign " << p("awaddr") << " = write_core_awaddr;\n"
       << "assign " << p("awlen") << " = write_core_awlen;\n"
       << "assign " << p("awsize") << " = write_core_awsize;\n"
       << "assign " << p("awburst") << " = write_core_awburst;\n"
       << "assign " << p("awid") << " = {5'b0, write_core_awid};\n"
       << "assign " << p("awvalid") << " = write_core_awvalid;\n"
       << "assign " << p("awcache") << " = 4'b0010;\n"
       << "assign " << p("awlock") << " = 1'b0;\n"
       << "assign " << p("awprot") << " = 3'b0;\n"
       << "assign " << p("awqos") << " = 4'b0;\n"
       << "assign " << p("awregion") << " = 4'b0;\n"
       << "assign " << p("awuser") << " = 1'b0;\n"
       << "assign " << p("wdata") << " = write_core_wdata;\n"
       << "assign " << p("wstrb") << " = write_core_wstrb;\n"
       << "assign " << p("wlast") << " = write_core_wlast;\n"
       << "assign " << p("wvalid") << " = write_core_wvalid;\n"
       << "assign " << p("wuser") << " = 1'b0;\n"
       << "assign " << p("bready") << " = write_core_bready;\n"
       << "wire read_peer_fault = write_core_fault;\n"
       << "wire write_peer_fault = read_core_fault;\n"
       << "MinimalAXI4MasterPipelined #(.B_MAX(" << profile.read_burst << "), .MAX_OUTSTANDING("
       << profile.read_outstanding << "), .FIFO_DEPTH(" << profile.read_fifo << "), .PEER_FAULT_ENABLE(1),\n"
       << "  .BITSIZE_in1(2), .BITSIZE_in2(32), .BITSIZE_in3(32), .BITSIZE_in4(" << profile.address_bits
       << "), .BITSIZE_in5(32), .BITSIZE_out1(32), .BITSIZE_m_axi_araddr(" << profile.address_bits
       << "), .BITSIZE_m_axi_arlen(8), .BITSIZE_m_axi_arsize(3), .BITSIZE_m_axi_arburst(2),\n"
       << "  .BITSIZE_m_axi_arid(1), .BITSIZE_m_axi_rdata(32), .BITSIZE_m_axi_rresp(2), .BITSIZE_m_axi_rid(1)) read_engine (\n"
       << "  .clock(clock), .reset(reset_cache), .start(read_start), .peer_fault(read_peer_fault),\n"
       << "  .in1(read_opcode), .in2(read_size), .in3(read_data), .in4(read_address), .in5(read_count),\n"
       << "  .done(read_core_done), .out1(read_core_result), .fault(read_core_fault),\n"
       << "  .m_axi_araddr(read_core_araddr), .m_axi_arlen(read_core_arlen), .m_axi_arsize(read_core_arsize),\n"
       << "  .m_axi_arburst(read_core_arburst), .m_axi_arid(read_core_arid), .m_axi_arvalid(read_core_arvalid),\n"
       << "  .m_axi_arready(" << p("arready") << "), .m_axi_rdata(" << p("rdata") << "),\n"
       << "  .m_axi_rresp(" << p("rresp") << "), .m_axi_rid((|" << p("rid") << ")), .m_axi_rlast("
       << p("rlast") << "), .m_axi_rvalid(" << p("rvalid") << "), .m_axi_rready(read_core_rready));\n"
       << "MinimalAXI4MasterPipelinedWrite #(.B_MAX(" << profile.write_burst << "), .MAX_OUTSTANDING("
       << profile.write_outstanding << "), .FIFO_DEPTH(" << profile.write_fifo << "), .PEER_FAULT_ENABLE(1),\n"
       << "  .BITSIZE_in1(2), .BITSIZE_in2(32), .BITSIZE_in3(32), .BITSIZE_in4(" << profile.address_bits
       << "), .BITSIZE_in5(32), .BITSIZE_out1(32), .BITSIZE_m_axi_awaddr(" << profile.address_bits
       << "), .BITSIZE_m_axi_awlen(8), .BITSIZE_m_axi_awsize(3), .BITSIZE_m_axi_awburst(2), .BITSIZE_m_axi_awid(1),\n"
       << "  .BITSIZE_m_axi_wdata(32), .BITSIZE_m_axi_wstrb(4), .BITSIZE_m_axi_bresp(2), .BITSIZE_m_axi_bid(1)) write_engine (\n"
       << "  .clock(clock), .reset(reset_cache), .start(write_start), .peer_fault(write_peer_fault),\n"
       << "  .in1(write_opcode), .in2(write_size), .in3(write_data), .in4(write_address), .in5(write_count),\n"
       << "  .done(write_core_done), .out1(write_core_result), .fault(write_core_fault),\n"
       << "  .m_axi_awaddr(write_core_awaddr), .m_axi_awlen(write_core_awlen), .m_axi_awsize(write_core_awsize),\n"
       << "  .m_axi_awburst(write_core_awburst), .m_axi_awid(write_core_awid), .m_axi_awvalid(write_core_awvalid),\n"
       << "  .m_axi_awready(" << p("awready") << "), .m_axi_wdata(write_core_wdata), .m_axi_wstrb(write_core_wstrb),\n"
       << "  .m_axi_wlast(write_core_wlast), .m_axi_wvalid(write_core_wvalid), .m_axi_wready(" << p("wready")
       << "),\n"
       << "  .m_axi_bresp(" << p("bresp") << "), .m_axi_bid((|" << p("bid") << ")), .m_axi_bvalid("
       << p("bvalid") << "), .m_axi_bready(write_core_bready));\n";

   structural_manager::add_NP_functionality(mod, NP_functionality::IP_COMPONENT,
                                            "MinimalAXI4MasterPipelined,MinimalAXI4MasterPipelinedWrite");
}
