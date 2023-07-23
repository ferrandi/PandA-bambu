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

#include "language_writer.hpp"

/* To access module parameters */
#include "structural_objects.hpp"

/* For integer logarithms */
#include "math_function.hpp"

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
   // o_wid, only AXI3 has wid
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
   THROW_ASSERT(_ports_in.size() >= i_last, "");
   THROW_ASSERT(_ports_out.size() >= o_last, "");
   const auto addr_bitsize = STR(_ports_out[o_awaddr].type_size);
   const auto data_bitsize = STR(_ports_out[o_wdata].type_size);

   unsigned way_size = 0;

   /* Get cache info */
   const std::string c_size_str = "WAY_LINES";
   const auto params = mod->GetParameters();
   if(params.find(c_size_str) != params.end())
   {
      way_size = boost::lexical_cast<unsigned>(params.at(c_size_str));
   }

   /* No cache, build the AXI controller */
   if(way_size == 0)
   {
      out << R"(
`ifndef _SIM_HAVE_CLOG2
  `define CLOG2(x) \
    (x <= 2) ? 1 : \
    (x <= 4) ? 2 : \
    (x <= 8) ? 3 : \
    (x <= 16) ? 4 : \
    (x <= 32) ? 5 : \
    (x <= 64) ? 6 : \
    (x <= 128) ? 7 : \
    -1
`endif

)";

      out << "assign " << _ports_out[o_awid].name << " = 0;\n";
      out << "assign " << _ports_out[o_awlock].name << " = 0;\n";
      out << "assign " << _ports_out[o_awcache].name << " = 0;\n";
      out << "assign " << _ports_out[o_awprot].name << " = 0;\n";
      out << "assign " << _ports_out[o_awqos].name << " = 0;\n";
      out << "assign " << _ports_out[o_awregion].name << " = 0;\n";
      out << "assign " << _ports_out[o_awuser].name << " = 0;\n";
      // out << "assign " << _ports_out[o_wid].name << " = 0;\n";
      out << "assign " << _ports_out[o_wuser].name << " = 0;\n";
      out << "assign " << _ports_out[o_arid].name << " = 0;\n";
      out << "assign " << _ports_out[o_arlock].name << " = 0;\n";
      out << "assign " << _ports_out[o_arcache].name << " = 0;\n";
      out << "assign " << _ports_out[o_arprot].name << " = 0;\n";
      out << "assign " << _ports_out[o_arqos].name << " = 0;\n";
      out << "assign " << _ports_out[o_arregion].name << " = 0;\n";
      out << "assign " << _ports_out[o_aruser].name << " = 0;\n";

      out << R"(
localparam [2:0] S_IDLE = 3'b000,
  S_RD_BURST = 3'b001,
  S_WR_BURST = 3'b101;
)";

      out << R"(
reg [2:0] _present_state, _next_state;
)";
      out << "reg [" + addr_bitsize + "-1:0] axi_awaddr, next_axi_awaddr;\n";
      out << "reg [" + addr_bitsize + "-1:0] axi_araddr, next_axi_araddr;\n";
      out << "reg [" + data_bitsize + "-1:0] axi_wdata, next_axi_wdata;\n";
      out << "reg [2:0] awsize, next_awsize;\n";
      out << "reg [(" + data_bitsize + "/8)-1:0] wstrb, next_wstrb;\n";
      out << "reg [1:0] awburst, next_awburst;\n";
      out << "reg [7:0] awlen, next_awlen;\n";
      out << "reg awready, next_awready;\n";
      out << "reg [2:0] arsize, next_arsize;\n";
      out << "reg [1:0] arburst, next_arburst;\n";
      out << "reg [7:0] arlen, next_arlen;\n";
      unsigned log_data_size = (boost::lexical_cast<unsigned>(data_bitsize) / 8);
      out << "reg [(" + STR(log_data_size) + ")-1:0] misalignment;\n";
      out << "reg [(" + data_bitsize + ")-1:0] read_mask, next_read_mask;\n";

      out << R"(
reg axi_awvalid, next_axi_awvalid;
reg axi_wlast, next_axi_wlast;
reg axi_wvalid, next_axi_wvalid;
reg axi_bready, next_axi_bready;
reg axi_arvalid, next_axi_arvalid;
reg axi_rready, next_axi_rready;

reg first_read, next_first_read;

reg acc_done, next_acc_done;
)";
      out << "reg [" + data_bitsize + "-1:0] acc_rdata, next_acc_rdata;\n"
          << "assign " << _ports_out[o_awlen].name << " = awlen;\n"
          << "assign " << _ports_out[o_awsize].name << " = awsize;\n"
          << "assign " << _ports_out[o_awburst].name << " = awburst;\n"
          << "assign " << _ports_out[o_wstrb].name << " = wstrb;\n"
          << "assign " << _ports_out[o_arlen].name << " = arlen;\n"
          << "assign " << _ports_out[o_arsize].name << " = arsize;\n"
          << "assign " << _ports_out[o_arburst].name << " = arburst;\n";

      // Assign reg values
      out << "assign " << _ports_out[o_awaddr].name << " = axi_awaddr;\n"
          << "assign " << _ports_out[o_awvalid].name << " = axi_awvalid;\n"
          << "assign " << _ports_out[o_wdata].name << " = axi_wdata;\n"
          << "assign " << _ports_out[o_wlast].name << " = axi_wlast;\n"
          << "assign " << _ports_out[o_wvalid].name << " = axi_wvalid;\n"
          << "assign " << _ports_out[o_bready].name << " = axi_bready;\n"
          << "assign " << _ports_out[o_araddr].name << " = axi_araddr;\n"
          << "assign " << _ports_out[o_arvalid].name << " = axi_arvalid;\n"
          << "assign " << _ports_out[o_rready].name << " = axi_rready;\n"
          << R"(
assign done_port = acc_done;
assign out1 = acc_done ? acc_rdata : 0;

initial begin
  _present_state = S_IDLE;
  axi_awaddr = 0;
  axi_awvalid = 0;
  axi_wdata = 0;
  axi_wlast = 0;
  axi_wvalid = 0;
  axi_bready = 0;
  axi_araddr = 0;
  axi_arvalid = 0;
  axi_rready = 0;
  acc_done = 0;
  acc_rdata = 0;
  awlen = 0;
  awburst = 0;
  arlen = 0;
  arburst = 0;
end
  
always @(*) begin
  _next_state = S_IDLE;
  next_axi_awaddr = axi_awaddr;
  next_axi_awvalid = axi_awvalid;
  next_axi_wdata = 0;
  next_axi_wlast = 0;
  next_axi_wvalid = 0;
  next_axi_bready = 0;
  next_axi_araddr = axi_araddr;
  next_axi_arvalid = 0;
  next_axi_rready = 0;
  next_acc_done = acc_done;
  next_acc_rdata = acc_rdata;
  next_awsize = awsize;
  next_awburst = awburst;
  next_awlen = awlen;
  next_arsize = arsize;
  next_arburst = arburst;
  next_arlen = arlen;
  next_first_read = first_read;
  next_read_mask = read_mask;
  next_awready = awready;
  next_wstrb = wstrb;
)";
      out << R"(
   case (_present_state)
    S_IDLE: begin 
      misalignment = 0;
      next_axi_awaddr = 0;
      next_axi_awvalid = 0;
      next_axi_wdata = 0;
      next_axi_wvalid = 0;
      next_axi_bready = 0;
      next_axi_araddr = 0;
      next_axi_arvalid = 0;
      next_axi_rready = 1;
      next_acc_done = 0;
      next_acc_rdata = 0;
      next_awsize = 0;
      next_awburst = 0;
      next_awlen = 0;
      next_wstrb = 0;
      next_arsize = 0;
      next_arburst = 0;
      next_arlen = 0;
      next_first_read = 0;
      next_read_mask = 0;
      next_acc_rdata = 0;
      next_awready = 0;
)";
      out << "      if (" << _ports_in[i_start].name << " && !" << _ports_in[i_in1].name << ") begin\n";

      out << R"(
          `ifdef _SIM_HAVE_CLOG2
            next_arsize = $clog2(in2 >> 3);
          `else
            next_arsize = `CLOG2(in2 >> 3);
          `endif
          next_axi_bready = 0;
          next_axi_rready = 1;
)";

      out << "          next_first_read = 1;\n";
      out << "          next_axi_araddr = " << _ports_in[i_in4].name << ";\n";
      out << "          misalignment = " << _ports_in[i_in4].name << " & ((1 << next_arsize) - 1);\n";
      out << "          if(misalignment > 0) begin\n";
      out << "            next_arlen = 'b1;\n";
      out << "            next_arburst = 'b1;\n";
      out << "            next_read_mask = -(1 << (misalignment << 3));\n";
      out << "          end else begin\n";
      out << "            next_arlen = 0;\n";
      out << "            next_arburst = 0;\n";
      out << "            next_read_mask = -1;\n";
      out << "          end\n";
      out << "          next_read_mask = next_read_mask & ((1 << in2) - 1);\n";
      out << "          next_axi_arvalid = 1;\n";
      out << "          _next_state = S_RD_BURST;\n";
      out << "      end else if (" << _ports_in[i_start].name << " && " << _ports_in[i_in1].name << ") begin\n";
      out << "        if(" << _ports_in[i_in2].name << " == 0) begin\n";
      out << "          next_acc_done = 1;\n";
      out << "        end else begin\n";
      out << "          next_axi_awaddr = " << _ports_in[i_in4].name << ";\n";
      out << "          next_axi_awvalid = 1;\n";
      out << "          `ifdef _SIM_HAVE_CLOG2\n";
      out << "            next_awsize = $clog2(in2 >> 3);\n";
      out << "          `else\n";
      out << "            next_awsize = `CLOG2(in2 >> 3);\n";
      out << "          `endif\n";
      /* Compute the misalignment, assert all the bits to the left of the misaligned one */
      out << "          misalignment = " << _ports_in[i_in4].name << " & ((1 << next_awsize) - 1);\n";
      for(unsigned i = 0; i < _ports_out[o_wstrb].type_size; i++)
      {
         out << "          next_wstrb[" << STR(i) << "] = " << STR(i) << " >= misalignment && (in2 >> 3 > " << STR(i)
             << ");\n";
      }
      out << "          next_axi_wdata = " << _ports_in[i_in3].name << " << (misalignment << 3);\n";
      out << R"(          next_axi_wvalid = 1;
          next_axi_wlast = !(misalignment > 0);
          if(next_axi_wlast) begin
            next_awburst = 2'b00;
            next_awlen = 8'b00000000;
          end else begin
            next_awburst = 2'b01;
            next_awlen = 8'b00000001;
          end
          next_axi_rready = 0;
)";
      out << R"(               _next_state = S_WR_BURST;
        end
      end else begin
        _next_state = S_IDLE;
      end
    end
)";
      out << " S_RD_BURST: begin\n";
      out << "      if(" << _ports_in[i_arready].name << ") begin\n";
      out << R"(        next_axi_arvalid = 0;
        next_arsize = 0;
        next_arburst = 0;
        next_arlen = 0;
        next_axi_araddr = 0;
        next_axi_arvalid = 0;
      end
      else begin
        next_axi_arvalid = axi_arvalid;
        next_axi_araddr = axi_araddr;
      end
      _next_state = S_RD_BURST;
      next_axi_rready = 1;
      
)";
      out << "      if(" << _ports_in[i_rvalid].name << " && axi_rready) begin\n";
      out << R"(          if(!first_read) begin
            if(~read_mask != 0)
              next_acc_rdata = acc_rdata >> (misalignment << 3) | (()" +
                 _ports_in[i_rdata].name + R"( & (~read_mask & ((1 << in2) - 1))) << (misalignment << 3));
            next_axi_rready = 0;
)";
      out << "             if(!" << _ports_in[i_rlast].name << ") begin\n";

      out << "                 _next_state = S_RD_BURST;\n";

      out << "             end else begin\n";
      out << "               _next_state = S_IDLE;\n";
      out << "               next_acc_done = 1;\n";
      out << "             end\n";
      out << R"(           end else if()" + _ports_in[i_rlast].name +
                 R"() begin
            next_acc_rdata = )" +
                 _ports_in[i_rdata].name + R"( & read_mask;
)";
      out << "            next_axi_rready = 0;\n";
      out << "            next_acc_done = 1;\n";
      out << "            _next_state = S_IDLE;\n";
      out << "          end else if (first_read) begin\n";
      out << "            next_acc_rdata = " << _ports_in[i_rdata].name << " & read_mask;\n";
      out << "               _next_state = S_RD_BURST;\n";
      out << R"(
            next_acc_done = 0;
            next_first_read = 0;
          end
      end else begin
        _next_state = S_RD_BURST;
      end
    end

    S_WR_BURST : begin 
    _next_state = S_WR_BURST;
    next_axi_bready = 1;
)";
      out << "      if(!" << _ports_in[i_wready].name << ") begin\n";
      out << "        next_axi_wvalid = axi_wvalid;\n";
      out << "        next_axi_wlast = axi_wlast;\n";
      out << "        next_axi_wdata = axi_wdata;\n";
      out << "      end\n";
      out << "      if(" << _ports_in[i_awready].name << ") begin";
      out << R"(
        next_awsize = 0;
        next_awburst = 0;
        next_awlen = 0;
        next_axi_awvalid = 0;
        next_axi_awaddr = 0;
        next_awready = 1;
      end
)";

      out << "      if (next_awready &&" << _ports_in[i_wready].name << " && !wstrb[0]) begin";
      out << R"(
        /* If the last transfer was not aligned and the slave is ready, transfer the rest */
        next_wstrb = ~wstrb;
        next_axi_wdata = )"
          << _ports_in[i_in3].name << R"( >> (misalignment << 3);
          next_axi_wvalid = 1;
      next_axi_wlast = 1;
)";
      out << R"(      end
      else if (next_awready && !wstrb[0]) begin
        /* If it's an aligned transfer but the slave is not ready, just keep the signals */
        next_axi_wdata = axi_wdata;
        next_axi_wvalid = axi_wvalid;
        next_wstrb = wstrb;
        next_axi_wlast = axi_wlast;
      end
      if(!next_awready) begin
        next_axi_awvalid = axi_awvalid;
        next_axi_awaddr = axi_awaddr;
        next_axi_wvalid = axi_wvalid;
        next_axi_wdata = axi_wdata;
      end 
      /* If the last transfer was complete, deassert the validity bits and check if you can go back to
      IDLE */
)";
      out << "      if (" << _ports_in[i_bvalid].name << ") begin\n";
      out << R"(        next_acc_done = 1;
        next_axi_wvalid = 0;
        next_axi_wdata = 0;
        next_wstrb = 0;
        next_axi_wlast = 0;
)";
      out << R"(        _next_state = S_IDLE;
      end
    end
  endcase
end

always @(posedge clock) begin
  _present_state <= _next_state;

  axi_awaddr <= next_axi_awaddr;
  axi_awvalid <= next_axi_awvalid;
  axi_wdata <= next_axi_wdata;
  axi_wlast <= next_axi_wlast;
  axi_wvalid <= next_axi_wvalid;
  axi_bready <= next_axi_bready;
  axi_araddr <= next_axi_araddr;
  axi_arvalid <= next_axi_arvalid;
  axi_rready <= next_axi_rready;
  acc_done <= next_acc_done;
  acc_rdata <= next_acc_rdata;
  awsize <= next_awsize;
  awburst <= next_awburst;
  awlen <= next_awlen;
  wstrb <= next_wstrb;
  arsize <= next_arsize;
  arburst <= next_arburst;
  arlen <= next_arlen;
  first_read <= next_first_read;
  read_mask <= next_read_mask;
  awready <= next_awready;
)";
      out << R"(
  if(1RESET_VALUE) begin 
    _present_state <= S_IDLE;
  end
end)";
   }
   else /* Connect to IOB cache, no need for AXI controller */
   {
      unsigned line_off_w = ceil_log2(way_size);
      unsigned word_off_w = 1;
      std::string be_data_w = data_bitsize;
      std::string n_ways = "1";
      unsigned wtbuf_depth_w = 2;
      std::string rep_policy = "0";
      std::string write_pol = "0";

      if(params.find("LINE_SIZE") != params.end())
      {
         word_off_w = ceil_log2(boost::lexical_cast<unsigned>(params.at("LINE_SIZE")));
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
         wtbuf_depth_w = ceil_log2(boost::lexical_cast<unsigned>(params.at("BUF_SIZE")));
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
  wire   [BITSIZE_in4 - 1: $clog2(BITSIZE_in3 / 8)] addr;
  wire   [(BITSIZE_in3 / 8) - 1: 0]                 wstrb;
  wire   [BITSIZE_in3 - 1: 0]                       rdata;
  wire                                              ready;
  wire                                              dirty;
  reg                                               state, state_next;
)";
      out << "localparam S_IDLE = 0, S_FLUSH = 1;\n";
      out << "initial begin\n";
      out << "  state = S_IDLE;\n";
      out << "end\n";
      out << "assign " << _ports_out[o_aruser].name << " = 0;\n";
      out << "assign " << _ports_out[o_arregion].name << " = 0;\n";
      out << "assign " << _ports_out[o_wuser].name << " = 0;\n";
      // out << "assign " << _ports_out[o_wid].name << " = 0;\n";
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
      out << "          .clk(clock),\n";
      out << "          .reset(!reset) /* IOB reset is active high */\n";
      out << "       );\n";
      out << "`undef _CACHE_CNT\n";
   }
}