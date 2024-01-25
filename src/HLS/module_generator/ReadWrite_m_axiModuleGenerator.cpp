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
 *              Copyright (C) 2022-2023 Politecnico di Milano
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
#include "call_graph_manager.hpp"
#include "hls.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "math_function.hpp"
#include "structural_objects.hpp"

enum in_port
{
   i_clock = 0,
   i_reset,
   i_start,
   i_in1,
   i_in2,
   i_in3,
   i_in4,
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

void ReadWrite_m_axiModuleGenerator::InternalExec(std::ostream& out, structural_objectRef mod,
                                                  unsigned int /* function_id */, vertex /* op_v */,
                                                  const HDLWriter_Language /* language */,
                                                  const std::vector<ModuleGenerator::parameter>& /* _p */,
                                                  const std::vector<ModuleGenerator::parameter>& _ports_in,
                                                  const std::vector<ModuleGenerator::parameter>& _ports_out,
                                                  const std::vector<ModuleGenerator::parameter>& /* _ports_inout */)
{
   THROW_ASSERT(_ports_in.size() >= i_last, "(_ports_in.size() = " + STR(_ports_in.size()) + ")");
   THROW_ASSERT(_ports_out.size() >= o_last, "");
   const auto addr_bitsize = STR(_ports_out[o_awaddr].type_size);
   const auto data_bitsize = STR(_ports_out[o_wdata].type_size);
   const auto data_size = STR(_ports_out[o_wdata].type_size / 8);

   auto AXI_conversion = [&](unsigned int type) -> std::string {
      if(type == 0)
      {
         return "FIXED";
      }
      else if(type == 1)
      {
         return "INCREMENTAL";
      }
      else
      {
         return "unsupported AXI burst type";
      }
   };

   unsigned int axi_burst_type = 0U;

   THROW_ASSERT(HLSMgr->CGetCallGraphManager()->GetRootFunctions().size() == 1, "Multiple top functions not supported");
   const auto top_id = *HLSMgr->CGetCallGraphManager()->GetRootFunctions().begin();
   const auto HLS = HLSMgr->get_HLS(top_id);
   const auto Param = HLS->Param;
   const auto use_specific_axi_burst_type = Param->isOption(OPT_axi_burst_type);
   const unsigned int requested_axi_burst_type =
       use_specific_axi_burst_type ? Param->getOption<unsigned int>(OPT_axi_burst_type) : 0U;
   if(use_specific_axi_burst_type)
   {
      axi_burst_type = requested_axi_burst_type;
   }

   const auto hls_d = HLSMgr->get_HLS_device();
   const auto use_device_axi_burst_type = hls_d->has_parameter("axi_burst_type");
   const unsigned int device_axi_burst_type =
       use_device_axi_burst_type ? hls_d->get_parameter<unsigned int>("axi_burst_type") : 0U;
   if(use_device_axi_burst_type)
   {
      axi_burst_type = device_axi_burst_type;
   }

   if(use_device_axi_burst_type && use_specific_axi_burst_type && device_axi_burst_type != requested_axi_burst_type)
   {
      THROW_WARNING("User required " + AXI_conversion(requested_axi_burst_type) +
                    " axi burst but the requested board needs " + AXI_conversion(device_axi_burst_type));
   }
   else if(use_specific_axi_burst_type)
   {
      THROW_WARNING("The requested board needs " + AXI_conversion(device_axi_burst_type) + " AXI burst type");
   }

   unsigned long long line_count = 0;

   /* Get cache info */
   const std::string c_size_str = "WAY_LINES";
   const auto params = mod->GetParameters();
   if(params.find(c_size_str) != params.end())
   {
      line_count = std::stoull(params.at(c_size_str));
   }

   /* No cache, build the AXI controller */
   if(line_count == 0)
   {
      out << "MinimalAXI4AdapterSingleBeat #(.BURST_TYPE(" + STR(axi_burst_type) + "),\n"
          << ".BITSIZE_Mout_addr_ram(" + addr_bitsize + "),\n"
          << ".BITSIZE_Mout_Wdata_ram(" + data_bitsize + "),\n"
          << ".BITSIZE_Mout_data_ram_size(BITSIZE_in2),\n"
          << ".BITSIZE_M_Rdata_ram(" + data_bitsize + "),\n"
          << ".BITSIZE_m_axi_awid(6),\n"
          << ".BITSIZE_m_axi_awaddr(" + addr_bitsize + "),\n"
          << ".BITSIZE_m_axi_awlen(8),\n"
          << ".BITSIZE_m_axi_wdata(" + data_bitsize + "),\n"
          << ".BITSIZE_m_axi_wstrb(" + data_size + "),\n"
          << ".BITSIZE_m_axi_bid(6),\n"
          << ".BITSIZE_m_axi_arid(6),\n"
          << ".BITSIZE_m_axi_araddr(" + addr_bitsize + "),\n"
          << ".BITSIZE_m_axi_arlen(8),\n"
          << ".BITSIZE_m_axi_rid(6),\n"
          << ".BITSIZE_m_axi_rdata(" + data_bitsize + ")) adapter (.M_DataRdy(done_port),\n"
          << ".M_Rdata_ram(out1),\n"
          << ".m_axi_arid(" << _ports_out[o_arid].name << "),\n"
          << ".m_axi_araddr(" << _ports_out[o_araddr].name << "),\n"
          << ".m_axi_arlen(" << _ports_out[o_arlen].name << "),\n"
          << ".m_axi_arsize(" << _ports_out[o_arsize].name << "),\n"
          << ".m_axi_arburst(" << _ports_out[o_arburst].name << "),\n"
          << ".m_axi_arlock(" << _ports_out[o_arlock].name << "),\n"
          << ".m_axi_arcache(" << _ports_out[o_arcache].name << "),\n"
          << ".m_axi_arprot(" << _ports_out[o_arprot].name << "),\n"
          << ".m_axi_arqos(" << _ports_out[o_arqos].name << "),\n"
          << ".m_axi_arregion(" << _ports_out[o_arregion].name << "),\n"
          << ".m_axi_aruser(" << _ports_out[o_aruser].name << "),\n"
          << ".m_axi_arvalid(" << _ports_out[o_arvalid].name << "),\n"
          << ".m_axi_rready(" << _ports_out[o_rready].name << "),\n"
          << ".m_axi_awid(" << _ports_out[o_awid].name << "),\n"
          << ".m_axi_awaddr(" << _ports_out[o_awaddr].name << "),\n"
          << ".m_axi_awlen(" << _ports_out[o_awlen].name << "),\n"
          << ".m_axi_awsize(" << _ports_out[o_awsize].name << "),\n"
          << ".m_axi_awburst(" << _ports_out[o_awburst].name << "),\n"
          << ".m_axi_awlock(" << _ports_out[o_awlock].name << "),\n"
          << ".m_axi_awcache(" << _ports_out[o_awcache].name << "),\n"
          << ".m_axi_awprot(" << _ports_out[o_awprot].name << "),\n"
          << ".m_axi_awqos(" << _ports_out[o_awqos].name << "),\n"
          << ".m_axi_awregion(" << _ports_out[o_awregion].name << "),\n"
          << ".m_axi_awuser(" << _ports_out[o_awuser].name << "),\n"
          << ".m_axi_awvalid(" << _ports_out[o_awvalid].name << "),\n"
          << ".m_axi_wdata(" << _ports_out[o_wdata].name << "),\n"
          << ".m_axi_wstrb(" << _ports_out[o_wstrb].name << "),\n"
          << ".m_axi_wlast(" << _ports_out[o_wlast].name << "),\n"
          << ".m_axi_wuser(" << _ports_out[o_wuser].name << "),\n"
          << ".m_axi_wvalid(" << _ports_out[o_wvalid].name << "),\n"
          << ".m_axi_bready(" << _ports_out[o_bready].name << "),\n"
          << ".clock(clock),\n"
          << ".reset(reset),\n"
          << ".Mout_oe_ram(" << _ports_in[i_start].name << " && !" << _ports_in[i_in1].name << "),\n"
          << ".Mout_we_ram(" << _ports_in[i_start].name << " && " << _ports_in[i_in1].name << "),\n"
          << ".Mout_addr_ram(in4),\n"
          << ".Mout_Wdata_ram(in3),\n"
          << ".Mout_data_ram_size(in2),\n"
          << ".m_axi_arready(" << _ports_in[i_arready].name << "),\n"
          << ".m_axi_rid(" << _ports_in[i_rid].name << "),\n"
          << ".m_axi_rdata(" << _ports_in[i_rdata].name << "),\n"
          << ".m_axi_rresp(" << _ports_in[i_rresp].name << "),\n"
          << ".m_axi_rlast(" << _ports_in[i_rlast].name << "),\n"
          << ".m_axi_rvalid(" << _ports_in[i_rvalid].name << "),\n"
          << ".m_axi_awready(" << _ports_in[i_awready].name << "),\n"
          << ".m_axi_wready(" << _ports_in[i_wready].name << "),\n"
          << ".m_axi_bid(" << _ports_in[i_bid].name << "),\n"
          << ".m_axi_bresp(" << _ports_in[i_bresp].name << "),\n"
          << ".m_axi_bvalid(" << _ports_in[i_bvalid].name << "));\n";
   }
   else /* Connect to IOB cache, no need for AXI controller */
   {
      auto line_off_w = ceil_log2(line_count);
      unsigned long long word_off_w = 1;
      std::string be_data_w = data_bitsize;
      std::string n_ways = "1";
      unsigned long long wtbuf_depth_w = 2;
      std::string rep_policy = "0";
      std::string write_pol = "0";

      if(params.find("LINE_SIZE") != params.end())
      {
         word_off_w = ceil_log2(std::stoull(params.at("LINE_SIZE")));
      }
      if(params.find("BUS_SIZE") != params.end())
      {
         be_data_w = params.at("BUS_SIZE");
      }
      if(params.find("N_WAYS") != params.end())
      {
         n_ways = params.at("N_WAYS");
      }
      if(params.find("BUF_SIZE") != params.end())
      {
         wtbuf_depth_w = ceil_log2(std::stoull(params.at("BUF_SIZE")));
         if(wtbuf_depth_w < 1)
         {
            wtbuf_depth_w = 1;
         }
      }
      if(params.find("REP_POL") != params.end())
      {
         rep_policy = params.at("REP_POL");
      }
      if(params.find("WR_POL") != params.end())
      {
         write_pol = params.at("WR_POL");
      }

      out << R"(
  wire [BITSIZE_in4 - 1: $clog2(BITSIZE_in3 / 8)] addr;
  wire [(BITSIZE_in3 / 8) - 1: 0] wstrb;
  wire [BITSIZE_in3 - 1: 0] rdata;
  wire ready;
  wire dirty;
  reg state, state_next;
)";
      out << "localparam S_IDLE = 0, S_FLUSH = 1;\n";
      out << "initial begin\n";
      out << "  state = S_IDLE;\n";
      out << "end\n";
      out << "assign " << _ports_out[o_aruser].name << " = 0;\n";
      out << "assign " << _ports_out[o_arregion].name << " = 0;\n";
      out << "assign " << _ports_out[o_wuser].name << " = 0;\n";
      out << "assign " << _ports_out[o_awuser].name << " = 0;\n";
      out << "assign " << _ports_out[o_awregion].name << " = 0;\n";

      out << R"(
      assign done_port = state == S_IDLE? ready : !dirty;
      assign addr = in4[BITSIZE_in4 - 1:$clog2(BITSIZE_in3 / 8)];
      assign wstrb = in1 ? (1 << (in2 / 8)) - 1 : 0;
      assign out1 = done_port ? rdata : 0;

      always @(*) begin
        state_next = state;
        if(state == S_IDLE) begin
)";
      out << "          if(" << _ports_in[i_start].name << " && " << _ports_in[i_in1].name << " && "
          << _ports_in[i_in2].name << " == 0) begin\n";
      out << "            state_next = S_FLUSH;\n";
      out << "          end\n";
      out << "        end else if(state == S_FLUSH) begin\n";
      out << "          if(!dirty) begin\n";
      out << "            state_next = S_IDLE;\n";
      out << "          end\n";
      out << "        end\n";
      out << "      end\n";
      out << "      always @(posedge " << CLOCK_PORT_NAME << ") begin\n";
      out << "        state <= state_next;\n";
      out << "        if(1RESET_VALUE) begin\n";
      out << "          state <= S_IDLE;\n";
      out << "        end\n";
      out << "      end\n";

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

      out << R"(
      IOB_cache_axi #(
          .FE_ADDR_W(BITSIZE_in4),
          .BE_ADDR_W(BITSIZE_in4),
          .BE_DATA_W()"
          << be_data_w << R"(),
          .N_WAYS()"
          << n_ways << R"(),
          .LINE_OFF_W()"
          << STR(line_off_w) << R"(),
          .WORD_OFF_W()"
          << STR(word_off_w) << R"(),
          .WTBUF_DEPTH_W()"
          << STR(wtbuf_depth_w) << R"(),
          .REP_POLICY()"
          << rep_policy << R"(),
          .WRITE_POL()"
          << write_pol << R"(),
          .AXI_ID(0),
          .CTRL_CACHE(`_CACHE_CNT),
          .CTRL_CNT(`_CACHE_CNT),
          .BURST_TYPE()"
          << STR(axi_burst_type) << R"(),
          .BITSIZE_addr(0 + BITSIZE_in4 - $clog2(BITSIZE_in3 / 8)),
          .BITSIZE_wdata(BITSIZE_in3),
          .BITSIZE_wstrb(BITSIZE_in3 / 8),
          .BITSIZE_rdata(BITSIZE_in3),
          .BITSIZE_m_axi_awid(1),
          .BITSIZE_m_axi_awaddr(BITSIZE_in4),
          .BITSIZE_m_axi_awlen(8),
          .BITSIZE_m_axi_wdata()"
          << be_data_w << R"(),
          .BITSIZE_m_axi_wstrb()"
          << be_data_w << R"( / 8),
          .BITSIZE_m_axi_bid(1),
          .BITSIZE_m_axi_arid(1),
          .BITSIZE_m_axi_araddr(BITSIZE_in4),
          .BITSIZE_m_axi_arlen(8),
          .BITSIZE_m_axi_rid(1),
          .BITSIZE_m_axi_rdata()"
          << be_data_w << R"()
        )
        cache(
          .addr(addr),
          .wdata(in3),
          .wstrb(wstrb),
          .rdata(rdata),
          .ready(ready),
          )";
      out << ".valid(" << _ports_in[i_start].name << " && !(" << _ports_in[i_in1].name << " && "
          << _ports_in[i_in2].name << R"( == 0)),
          .dirty(dirty),
          .flush(state == S_FLUSH),
)";
      out << "          .m_axi_awready(" << _ports_in[i_awready].name << "),\n";
      out << "          .m_axi_wready(" << _ports_in[i_wready].name << "),\n";
      out << "          .m_axi_bid(" << _ports_in[i_bid].name << "),\n";
      out << "          .m_axi_bresp(" << _ports_in[i_bresp].name << "),\n";
      out << "          .m_axi_bvalid(" << _ports_in[i_bvalid].name << "),\n";
      out << "          .m_axi_arready(" << _ports_in[i_arready].name << "),\n";
      out << "          .m_axi_rid(" << _ports_in[i_rid].name << "),\n";
      out << "          .m_axi_rdata(" << _ports_in[i_rdata].name << "),\n";
      out << "          .m_axi_rresp(" << _ports_in[i_rresp].name << "),\n";
      out << "          .m_axi_rlast(" << _ports_in[i_rlast].name << "),\n";
      out << "          .m_axi_rvalid(" << _ports_in[i_rvalid].name << "),\n";
      out << "          .m_axi_awid(" << _ports_out[o_awid].name << "),\n";
      out << "          .m_axi_awaddr(" << _ports_out[o_awaddr].name << "),\n";
      out << "          .m_axi_awlen(" << _ports_out[o_awlen].name << "),\n";
      out << "          .m_axi_awsize(" << _ports_out[o_awsize].name << "),\n";
      out << "          .m_axi_awburst(" << _ports_out[o_awburst].name << "),\n";
      out << "          .m_axi_awlock(" << _ports_out[o_awlock].name << "),\n";
      out << "          .m_axi_awcache(" << _ports_out[o_awcache].name << "),\n";
      out << "          .m_axi_awprot(" << _ports_out[o_awprot].name << "),\n";
      out << "          .m_axi_awqos(" << _ports_out[o_awqos].name << "),\n";
      out << "          .m_axi_awvalid(" << _ports_out[o_awvalid].name << "),\n";
      out << "          .m_axi_wdata(" << _ports_out[o_wdata].name << "),\n";
      out << "          .m_axi_wstrb(" << _ports_out[o_wstrb].name << "),\n";
      out << "          .m_axi_wlast(" << _ports_out[o_wlast].name << "),\n";
      out << "          .m_axi_wvalid(" << _ports_out[o_wvalid].name << "),\n";
      out << "          .m_axi_bready(" << _ports_out[o_bready].name << "),\n";
      out << "          .m_axi_arid(" << _ports_out[o_arid].name << "),\n";
      out << "          .m_axi_araddr(" << _ports_out[o_araddr].name << "),\n";
      out << "          .m_axi_arlen(" << _ports_out[o_arlen].name << "),\n";
      out << "          .m_axi_arsize(" << _ports_out[o_arsize].name << "),\n";
      out << "          .m_axi_arburst(" << _ports_out[o_arburst].name << "),\n";
      out << "          .m_axi_arlock(" << _ports_out[o_arlock].name << "),\n";
      out << "          .m_axi_arcache(" << _ports_out[o_arcache].name << "),\n";
      out << "          .m_axi_arprot(" << _ports_out[o_arprot].name << "),\n";
      out << "          .m_axi_arqos(" << _ports_out[o_arqos].name << "),\n";
      out << "          .m_axi_arvalid(" << _ports_out[o_arvalid].name << "),\n";
      out << "          .m_axi_rready(" << _ports_out[o_rready].name << "),\n";
      out << "          .clock(clock),\n";
      out << "          .reset(reset)\n";
      out << "       );\n";
      out << "`undef _CACHE_CNT\n";
   }
}