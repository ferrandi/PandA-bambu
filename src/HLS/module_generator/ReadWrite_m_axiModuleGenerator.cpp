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
 *              Copyright (C) 2022-2024 Politecnico di Milano
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
 * @file ReadWrite_m_axiModuleGenerator.cpp
 * @brief
 *
 *
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * @author Ankur Limaye <ankur.limaye@pnnl.gov>
 * @author Claudio Barone <claudio.barone@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "ReadWrite_m_axiModuleGenerator.hpp"

#include "BambuParameter.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "constant_strings.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "math_function.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"

#define BURST_TYPE_STR(type) std::string(type == 0 ? "FIXED" : (type == 1 ? "INCREMENTAL" : "UNKNOWN"))

enum in_port
{
   i_clock = 0,
   i_reset,
   i_start,
   i_in1,
   i_in2,
   i_in3,
   i_in4,
   i_cache_reset,
   i_awready,
   i_wready,
   i_bid,
   i_bresp,
   i_buser,
   i_bvalid,
   i_arready,
   i_rid,
   i_rdata,
   i_rresp,
   i_rlast,
   i_ruser,
   i_rvalid,
   i__n_ptr,
   i_last
};

enum out_port
{
   o_done = 0,
   o_out1,
   o_awid,
   o_awaddr,
   o_awlen,
   o_awsize,
   o_awburst,
   o_awlock,
   o_awcache,
   o_awprot,
   o_awqos,
   o_awregion,
   o_awuser,
   o_awvalid,
   o_wdata,
   o_wstrb,
   o_wlast,
   o_wuser,
   o_wvalid,
   o_bready,
   o_arid,
   o_araddr,
   o_arlen,
   o_arsize,
   o_arburst,
   o_arlock,
   o_arcache,
   o_arprot,
   o_arqos,
   o_arregion,
   o_aruser,
   o_arvalid,
   o_rready,
   o_last
};

ReadWrite_m_axiModuleGenerator::ReadWrite_m_axiModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void ReadWrite_m_axiModuleGenerator::InternalExec(std::ostream& out, structural_objectRef mod, unsigned int function_id,
                                                  vertex /* op_v */, const HDLWriter_Language language,
                                                  const std::vector<ModuleGenerator::parameter>& /* _p */,
                                                  const std::vector<ModuleGenerator::parameter>& _ports_in,
                                                  const std::vector<ModuleGenerator::parameter>& _ports_out,
                                                  const std::vector<ModuleGenerator::parameter>& /* _ports_inout */)
{
   if(language != HDLWriter_Language::VERILOG)
   {
      THROW_UNREACHABLE("Unsupported output language");
      return;
   }

   THROW_ASSERT(_ports_in.size() >= i_last, "");
   THROW_ASSERT(_ports_out.size() >= o_last, "");

   const auto bundle_name = mod->get_id().substr(0, mod->get_id().find(STR_CST_interface_parameter_keyword));
   const auto top_fid = HLSMgr->CGetCallGraphManager()->GetRootFunction(function_id);
   const auto top_fname = HLSMgr->CGetFunctionBehavior(top_fid)->CGetBehavioralHelper()->GetMangledFunctionName();
   const auto& iface_attrs = HLSMgr->module_arch->GetArchitecture(top_fname)->ifaces.at(bundle_name);

   const auto Param = HLSMgr->get_parameter();
   const auto has_requested_burst_type = Param->isOption(OPT_axi_burst_type);
   unsigned int axi_burst_type = has_requested_burst_type ? Param->getOption<unsigned int>(OPT_axi_burst_type) : 0U;

   const auto hls_d = HLSMgr->get_HLS_device();
   const auto has_device_burst_type = hls_d->has_parameter("axi_burst_type");
   const auto device_burst_type = has_device_burst_type ? hls_d->get_parameter<unsigned int>("axi_burst_type") : 0U;
   if(has_device_burst_type)
   {
      if(has_requested_burst_type && axi_burst_type != device_burst_type)
      {
         THROW_WARNING("Requested AXI burst type conflicts with selected device supported AXI burst type: " +
                       BURST_TYPE_STR(axi_burst_type) + " != " + BURST_TYPE_STR(device_burst_type));
      }
      axi_burst_type = device_burst_type;
   }

   /* Get cache info */
   unsigned long long line_count = 0;
   if(auto it = iface_attrs.find(FunctionArchitecture::iface_cache_line_count); it != iface_attrs.end())
   {
      line_count = std::stoull(it->second);
   }

   out << "localparam BITSIZE_address=BITSIZE_" << _ports_in[i_in4].name << ",\n"
       << "  BITSIZE_bus=" << _ports_out[o_wdata].type_size << ",\n"
       << "  BITSIZE_bus_size=BITSIZE_bus/8,\n"
       << "  BITSIZE_data=BITSIZE_" << _ports_in[i_in3].name << ",\n"
       << "  BITSIZE_data_size=BITSIZE_data/8,\n"
       << "  BITSIZE_awlen=" << _ports_out[o_awlen].type_size << ",\n"
       << "  BITSIZE_arlen=" << _ports_out[o_arlen].type_size << ",\n"
       << "  BITSIZE_awid=" << _ports_out[o_awid].type_size << ",\n"
       << "  BITSIZE_arid=" << _ports_out[o_arid].type_size << ",\n"
       << "  BITSIZE_bid=" << _ports_in[i_bid].type_size << ",\n"
       << "  BITSIZE_rid=" << _ports_in[i_rid].type_size << ";\n\n";

   if(line_count != 0)
   {
      out << "// BITSIZE_log_data_size = log2(BITISZE_in3 >> 3)\n"
          << "localparam BITSIZE_log_data_size = " << ceil_log2(std::max(_ports_in[i_in3].type_size, 8ull) / 8)
          << ";\n ";
   }

   /* No cache, build the AXI controller */
   std::string ip_components;
   if(line_count == 0)
   {
      ip_components = "MinimalAXI4AdapterSingleBeat";
      out << "MinimalAXI4AdapterSingleBeat #(.BURST_TYPE(" << axi_burst_type << "),\n"
          << "  .BITSIZE_Mout_addr_ram(BITSIZE_address),\n"
          << "  .BITSIZE_Mout_Wdata_ram(BITSIZE_data),\n"
          << "  .BITSIZE_Mout_data_ram_size(BITSIZE_" << _ports_in[i_in2].name << "),\n"
          << "  .BITSIZE_M_Rdata_ram(BITSIZE_data),\n"
          << "  .BITSIZE_m_axi_awid(BITSIZE_awid),\n"
          << "  .BITSIZE_m_axi_awaddr(BITSIZE_address),\n"
          << "  .BITSIZE_m_axi_awlen(BITSIZE_awlen),\n"
          << "  .BITSIZE_m_axi_wdata(BITSIZE_bus),\n"
          << "  .BITSIZE_m_axi_wstrb(BITSIZE_bus_size),\n"
          << "  .BITSIZE_m_axi_bid(BITSIZE_bid),\n"
          << "  .BITSIZE_m_axi_arid(BITSIZE_arid),\n"
          << "  .BITSIZE_m_axi_araddr(BITSIZE_address),\n"
          << "  .BITSIZE_m_axi_arlen(BITSIZE_arlen),\n"
          << "  .BITSIZE_m_axi_rid(BITSIZE_rid),\n"
          << "  .BITSIZE_m_axi_rdata(BITSIZE_bus)) adapter (.M_DataRdy(done_port),\n"
          << "  .M_Rdata_ram(" << _ports_out[o_out1].name << "),\n"
          << "  .m_axi_arid(" << _ports_out[o_arid].name << "),\n"
          << "  .m_axi_araddr(" << _ports_out[o_araddr].name << "),\n"
          << "  .m_axi_arlen(" << _ports_out[o_arlen].name << "),\n"
          << "  .m_axi_arsize(" << _ports_out[o_arsize].name << "),\n"
          << "  .m_axi_arburst(" << _ports_out[o_arburst].name << "),\n"
          << "  .m_axi_arlock(" << _ports_out[o_arlock].name << "),\n"
          << "  .m_axi_arcache(" << _ports_out[o_arcache].name << "),\n"
          << "  .m_axi_arprot(" << _ports_out[o_arprot].name << "),\n"
          << "  .m_axi_arqos(" << _ports_out[o_arqos].name << "),\n"
          << "  .m_axi_arregion(" << _ports_out[o_arregion].name << "),\n"
          << "  .m_axi_aruser(" << _ports_out[o_aruser].name << "),\n"
          << "  .m_axi_arvalid(" << _ports_out[o_arvalid].name << "),\n"
          << "  .m_axi_rready(" << _ports_out[o_rready].name << "),\n"
          << "  .m_axi_awid(" << _ports_out[o_awid].name << "),\n"
          << "  .m_axi_awaddr(" << _ports_out[o_awaddr].name << "),\n"
          << "  .m_axi_awlen(" << _ports_out[o_awlen].name << "),\n"
          << "  .m_axi_awsize(" << _ports_out[o_awsize].name << "),\n"
          << "  .m_axi_awburst(" << _ports_out[o_awburst].name << "),\n"
          << "  .m_axi_awlock(" << _ports_out[o_awlock].name << "),\n"
          << "  .m_axi_awcache(" << _ports_out[o_awcache].name << "),\n"
          << "  .m_axi_awprot(" << _ports_out[o_awprot].name << "),\n"
          << "  .m_axi_awqos(" << _ports_out[o_awqos].name << "),\n"
          << "  .m_axi_awregion(" << _ports_out[o_awregion].name << "),\n"
          << "  .m_axi_awuser(" << _ports_out[o_awuser].name << "),\n"
          << "  .m_axi_awvalid(" << _ports_out[o_awvalid].name << "),\n"
          << "  .m_axi_wdata(" << _ports_out[o_wdata].name << "),\n"
          << "  .m_axi_wstrb(" << _ports_out[o_wstrb].name << "),\n"
          << "  .m_axi_wlast(" << _ports_out[o_wlast].name << "),\n"
          << "  .m_axi_wuser(" << _ports_out[o_wuser].name << "),\n"
          << "  .m_axi_wvalid(" << _ports_out[o_wvalid].name << "),\n"
          << "  .m_axi_bready(" << _ports_out[o_bready].name << "),\n"
          << "  .clock(clock),\n"
          << "  .reset(reset),\n"
          << "  .Mout_oe_ram(" << _ports_in[i_start].name << " && !" << _ports_in[i_in1].name << "),\n"
          << "  .Mout_we_ram(" << _ports_in[i_start].name << " && " << _ports_in[i_in1].name << "),\n"
          << "  .Mout_addr_ram(" << _ports_in[i_in4].name << "),\n"
          << "  .Mout_Wdata_ram(" << _ports_in[i_in3].name << "),\n"
          << "  .Mout_data_ram_size(" << _ports_in[i_in2].name << "),\n"
          << "  .m_axi_arready(" << _ports_in[i_arready].name << "),\n"
          << "  .m_axi_rid(" << _ports_in[i_rid].name << "),\n"
          << "  .m_axi_rdata(" << _ports_in[i_rdata].name << "),\n"
          << "  .m_axi_rresp(" << _ports_in[i_rresp].name << "),\n"
          << "  .m_axi_rlast(" << _ports_in[i_rlast].name << "),\n"
          << "  .m_axi_rvalid(" << _ports_in[i_rvalid].name << "),\n"
          << "  .m_axi_awready(" << _ports_in[i_awready].name << "),\n"
          << "  .m_axi_wready(" << _ports_in[i_wready].name << "),\n"
          << "  .m_axi_bid(" << _ports_in[i_bid].name << "),\n"
          << "  .m_axi_bresp(" << _ports_in[i_bresp].name << "),\n"
          << "  .m_axi_bvalid(" << _ports_in[i_bvalid].name << "));\n";
   }
   else /* Connect to IOB cache, no need for AXI controller */
   {
      const auto line_off_w = std::to_string(ceil_log2(line_count));
      std::string word_off_w = "1";
      std::string be_data_w = std::to_string(_ports_out[o_wdata].type_size);
      std::string n_ways = "1";
      unsigned long long wtbuf_depth_w = 2ULL;
      std::string rep_policy = "0";
      std::string write_pol = "0";

      if(auto it = iface_attrs.find(FunctionArchitecture::iface_cache_line_size); it != iface_attrs.end())
      {
         word_off_w = std::to_string(ceil_log2(std::stoull(it->second)));
      }
      if(auto it = iface_attrs.find(FunctionArchitecture::iface_cache_bus_size); it != iface_attrs.end())
      {
         be_data_w = it->second;
      }
      if(auto it = iface_attrs.find(FunctionArchitecture::iface_cache_ways); it != iface_attrs.end())
      {
         n_ways = it->second;
      }
      if(auto it = iface_attrs.find(FunctionArchitecture::iface_cache_num_write_outstanding); it != iface_attrs.end())
      {
         wtbuf_depth_w = ceil_log2(std::stoull(it->second));
         if(wtbuf_depth_w < 1)
         {
            wtbuf_depth_w = 1;
         }
      }
      if(auto it = iface_attrs.find(FunctionArchitecture::iface_cache_rep_policy); it != iface_attrs.end())
      {
         const auto rp_name = boost::to_upper_copy(it->second);
         if(rp_name == "LRU")
         {
            rep_policy = "0";
         }
         else if(rp_name == "MRU")
         {
            rep_policy = "1";
         }
         else if(rp_name == "TREE")
         {
            rep_policy = "2";
         }
         else
         {
            THROW_ERROR("Unexpected cache replacement policy: " + it->second);
         }
      }
      if(auto it = iface_attrs.find(FunctionArchitecture::iface_cache_write_policy); it != iface_attrs.end())
      {
         const auto wp_name = boost::to_upper_copy(it->second);
         if(wp_name == "WT")
         {
            write_pol = "0";
         }
         else if(wp_name == "WB")
         {
            write_pol = "1";
         }
         else
         {
            THROW_ERROR("Unexpected cache write policy: " + it->second);
         }
      }

      const auto out1_port = mod->find_member("out1", port_o_K, mod);
      THROW_ASSERT(out1_port, "out1 port must be present in " + mod->get_path());
#if HAVE_ASSERTS
      const auto fe_data_w = STD_GET_SIZE(GetPointer<port_o>(out1_port)->get_typeRef());
#endif
      THROW_ASSERT((1ULL << std::stoull(word_off_w)) * fe_data_w >= std::stoull(be_data_w),
                   "ERROR: Cache line of " + STR((1ULL << std::stoull(word_off_w)) * fe_data_w) +
                       " bits is smaller than bus size (" + STR(be_data_w) + ")");

      ip_components = "IOB_cache_axi";
      out << "wire [BITSIZE_address-1:BITSIZE_log_data_size] addr;\n"
          << "wire [BITSIZE_data_size-1:0] wstrb;\n"
          << "wire [BITSIZE_data-1:0] rdata;\n"
          << "wire ready;\n"
          << "wire dirty;\n"
          << "wire reset_cache;\n"
          << "reg state, state_next;\n\n";

      const auto reset_level = Param->getOption<bool>(OPT_reset_level);
      if(reset_level)
      {
         out << "assign reset_cache = reset || cache_reset;\n\n";
      }
      else
      {
         out << "assign reset_cache = reset && !cache_reset;\n\n";
      }

      out << "localparam S_IDLE = 0, S_FLUSH = 1;\n"
          << "initial state = S_IDLE;\n\n"
          << "assign " << _ports_out[o_aruser].name << " = 0;\n"
          << "assign " << _ports_out[o_arregion].name << " = 0;\n"
          << "assign " << _ports_out[o_wuser].name << " = 0;\n"
          << "assign " << _ports_out[o_awuser].name << " = 0;\n"
          << "assign " << _ports_out[o_awregion].name << " = 0;\n\n"
          << "assign done_port = state == S_IDLE? ready : !dirty;\n"
          << "assign addr = " << _ports_in[i_in4].name << "[BITSIZE_address-1:BITSIZE_log_data_size];\n"
          << "assign wstrb = " << _ports_in[i_in1].name << " ? (1 << (" << _ports_in[i_in2].name << "/8)) - 1 : 0;\n"
          << "assign " << _ports_out[o_out1].name << " = done_port ? rdata : 0;\n\n"
          << "always @(*) begin\n"
          << "  state_next = state;\n"
          << "  if(state == S_IDLE) begin\n"
          << "    if(" << _ports_in[i_start].name << " && " << _ports_in[i_in1].name << " && " << _ports_in[i_in2].name
          << " == 0) begin\n"
          << "      state_next = S_FLUSH;\n"
          << "    end\n"
          << "  end else if(state == S_FLUSH) begin\n"
          << "    if(!dirty) begin\n"
          << "      state_next = S_IDLE;\n"
          << "    end\n"
          << "  end\n"
          << "end\n\n"
          << "always @(posedge clock 1RESET_EDGE) begin\n"
          << "  state <= state_next;\n"
          << "  if(1RESET_VALUE) begin\n"
          << "    state <= S_IDLE;\n"
          << "  end\n"
          << "end\n";

      out << R"(
`ifdef __ICARUS__
  `define _CACHE_CNT 1
`elsif VERILATOR
  `define _CACHE_CNT 1
`elsif MODEL_TECH
  `define _CACHE_CNT 1
`elsif VCS
  `define _CACHE_CNT 1
`elsif NCVERILOG
  `define _CACHE_CNT 1
`elsif XILINX_SIMULATOR
  `define _CACHE_CNT 1
`elsif XILINX_ISIM
  `define _CACHE_CNT 1
`else
  `define _CACHE_CNT 0
`endif
      )";

      out << "IOB_cache_axi #(.FE_ADDR_W(BITSIZE_address),\n"
          << "  .BE_ADDR_W(BITSIZE_address),\n"
          << "  .BE_DATA_W(" << be_data_w << "),\n"
          << "  .N_WAYS(" << n_ways << "),\n"
          << "  .LINE_OFF_W(" << line_off_w << "),\n"
          << "  .WORD_OFF_W(" << word_off_w << "),\n"
          << "  .WTBUF_DEPTH_W(" << wtbuf_depth_w << "),\n"
          << "  .REP_POLICY(" << rep_policy << "),\n"
          << "  .WRITE_POL(" << write_pol << "),\n"
          << "  .AXI_ID(0),\n"
          << "  .CTRL_CACHE(`_CACHE_CNT),\n"
          << "  .CTRL_CNT(`_CACHE_CNT),\n"
          << "  .BURST_TYPE(" << axi_burst_type << "),\n"
          << "  .BITSIZE_addr(BITSIZE_address-BITSIZE_log_data_size),\n"
          << "  .BITSIZE_wdata(BITSIZE_data),\n"
          << "  .BITSIZE_wstrb(BITSIZE_data_size),\n"
          << "  .BITSIZE_rdata(BITSIZE_data),\n"
          << "  .BITSIZE_m_axi_awid(BITSIZE_awid),\n"
          << "  .BITSIZE_m_axi_awaddr(BITSIZE_address),\n"
          << "  .BITSIZE_m_axi_awlen(BITSIZE_awlen),\n"
          << "  .BITSIZE_m_axi_wdata(" << be_data_w << "),\n"
          << "  .BITSIZE_m_axi_wstrb(" << be_data_w << " / 8),\n"
          << "  .BITSIZE_m_axi_bid(BITSIZE_bid),\n"
          << "  .BITSIZE_m_axi_arid(BITSIZE_arid),\n"
          << "  .BITSIZE_m_axi_araddr(BITSIZE_address),\n"
          << "  .BITSIZE_m_axi_arlen(BITSIZE_arlen),\n"
          << "  .BITSIZE_m_axi_rid(BITSIZE_rid),\n"
          << "  .BITSIZE_m_axi_rdata(" << be_data_w << ")) cache(.addr(addr),\n"
          << "  .wdata(" << _ports_in[i_in3].name << "),\n"
          << "  .wstrb(wstrb),\n"
          << "  .rdata(rdata),\n"
          << "  .ready(ready),\n"
          << "  .valid(" << _ports_in[i_start].name << " && !(" << _ports_in[i_in1].name << " && "
          << _ports_in[i_in2].name << " == 0)),\n"
          << "  .dirty(dirty),\n"
          << "  .flush(state == S_FLUSH),\n"
          << "  .m_axi_awready(" << _ports_in[i_awready].name << "),\n"
          << "  .m_axi_wready(" << _ports_in[i_wready].name << "),\n"
          << "  .m_axi_bid(" << _ports_in[i_bid].name << "),\n"
          << "  .m_axi_bresp(" << _ports_in[i_bresp].name << "),\n"
          << "  .m_axi_bvalid(" << _ports_in[i_bvalid].name << "),\n"
          << "  .m_axi_arready(" << _ports_in[i_arready].name << "),\n"
          << "  .m_axi_rid(" << _ports_in[i_rid].name << "),\n"
          << "  .m_axi_rdata(" << _ports_in[i_rdata].name << "),\n"
          << "  .m_axi_rresp(" << _ports_in[i_rresp].name << "),\n"
          << "  .m_axi_rlast(" << _ports_in[i_rlast].name << "),\n"
          << "  .m_axi_rvalid(" << _ports_in[i_rvalid].name << "),\n"
          << "  .m_axi_awid(" << _ports_out[o_awid].name << "),\n"
          << "  .m_axi_awaddr(" << _ports_out[o_awaddr].name << "),\n"
          << "  .m_axi_awlen(" << _ports_out[o_awlen].name << "),\n"
          << "  .m_axi_awsize(" << _ports_out[o_awsize].name << "),\n"
          << "  .m_axi_awburst(" << _ports_out[o_awburst].name << "),\n"
          << "  .m_axi_awlock(" << _ports_out[o_awlock].name << "),\n"
          << "  .m_axi_awcache(" << _ports_out[o_awcache].name << "),\n"
          << "  .m_axi_awprot(" << _ports_out[o_awprot].name << "),\n"
          << "  .m_axi_awqos(" << _ports_out[o_awqos].name << "),\n"
          << "  .m_axi_awvalid(" << _ports_out[o_awvalid].name << "),\n"
          << "  .m_axi_wdata(" << _ports_out[o_wdata].name << "),\n"
          << "  .m_axi_wstrb(" << _ports_out[o_wstrb].name << "),\n"
          << "  .m_axi_wlast(" << _ports_out[o_wlast].name << "),\n"
          << "  .m_axi_wvalid(" << _ports_out[o_wvalid].name << "),\n"
          << "  .m_axi_bready(" << _ports_out[o_bready].name << "),\n"
          << "  .m_axi_arid(" << _ports_out[o_arid].name << "),\n"
          << "  .m_axi_araddr(" << _ports_out[o_araddr].name << "),\n"
          << "  .m_axi_arlen(" << _ports_out[o_arlen].name << "),\n"
          << "  .m_axi_arsize(" << _ports_out[o_arsize].name << "),\n"
          << "  .m_axi_arburst(" << _ports_out[o_arburst].name << "),\n"
          << "  .m_axi_arlock(" << _ports_out[o_arlock].name << "),\n"
          << "  .m_axi_arcache(" << _ports_out[o_arcache].name << "),\n"
          << "  .m_axi_arprot(" << _ports_out[o_arprot].name << "),\n"
          << "  .m_axi_arqos(" << _ports_out[o_arqos].name << "),\n"
          << "  .m_axi_arvalid(" << _ports_out[o_arvalid].name << "),\n"
          << "  .m_axi_rready(" << _ports_out[o_rready].name << "),\n"
          << "  .clock(clock),\n"
          << "  .reset(reset_cache));\n\n"
          << "`undef _CACHE_CNT\n";
   }
   structural_manager::add_NP_functionality(mod, NP_functionality::IP_COMPONENT, ip_components);
}
