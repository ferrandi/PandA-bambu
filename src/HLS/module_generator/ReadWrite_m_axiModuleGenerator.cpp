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
   i_AWREADY,
   i_WREADY,
   i_BID,
   i_BRESP,
   i_BUSER,
   i_BVALID,
   i_ARREADY,
   i_RID,
   i_RDATA,
   i_RRESP,
   i_RLAST,
   i_RUSER,
   i_RVALID,
   i__n_ptr,
   i_last
};

enum out_port
{
   o_done = 0,
   o_out1,
   o_AWID,
   o_AWADDR,
   o_AWLEN,
   o_AWSIZE,
   o_AWBURST,
   o_AWLOCK,
   o_AWCACHE,
   o_AWPROT,
   o_AWQOS,
   o_AWREGION,
   o_AWUSER,
   o_AWVALID,
   o_WID,
   o_WDATA,
   o_WSTRB,
   o_WLAST,
   o_WUSER,
   o_WVALID,
   o_BREADY,
   o_ARID,
   o_ARADDR,
   o_ARLEN,
   o_ARSIZE,
   o_ARBURST,
   o_ARLOCK,
   o_ARCACHE,
   o_ARPROT,
   o_ARQOS,
   o_ARREGION,
   o_ARUSER,
   o_ARVALID,
   o_RREADY,
   o_last
};

ReadWrite_m_axiModuleGenerator::ReadWrite_m_axiModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void ReadWrite_m_axiModuleGenerator::InternalExec(std::ostream& out, const module* mod, unsigned int /* function_id */,
                                                  vertex /* op_v */, const HDLWriter_Language /* language */,
                                                  const std::vector<ModuleGenerator::parameter>& /* _p */,
                                                  const std::vector<ModuleGenerator::parameter>& _ports_in,
                                                  const std::vector<ModuleGenerator::parameter>& _ports_out,
                                                  const std::vector<ModuleGenerator::parameter>& /* _ports_inout */)
{
   THROW_ASSERT(_ports_in.size() >= i_last, "");
   THROW_ASSERT(_ports_out.size() >= o_last, "");
   const auto addr_bitsize = STR(_ports_out[o_AWADDR].type_size);
   const auto data_bitsize = STR(_ports_out[o_WDATA].type_size);

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

      out << "assign " << _ports_out[o_AWID].name << " = 'b0;\n";
      out << "assign " << _ports_out[o_AWLOCK].name << " = 'b0;\n";
      out << "assign " << _ports_out[o_AWCACHE].name << " = 'b0;\n";
      out << "assign " << _ports_out[o_AWPROT].name << " = 'b0;\n";
      out << "assign " << _ports_out[o_AWQOS].name << " = 'b0;\n";
      out << "assign " << _ports_out[o_AWREGION].name << " = 'b0;\n";
      out << "assign " << _ports_out[o_AWUSER].name << " = 'b0;\n";
      out << "assign " << _ports_out[o_WID].name << " = 'b0;\n";
      out << "assign " << _ports_out[o_WUSER].name << " = 'b0;\n";
      out << "assign " << _ports_out[o_ARID].name << " = 'b0;\n";
      out << "assign " << _ports_out[o_ARLOCK].name << " = 'b0;\n";
      out << "assign " << _ports_out[o_ARCACHE].name << " = 'b0;\n";
      out << "assign " << _ports_out[o_ARPROT].name << " = 'b0;\n";
      out << "assign " << _ports_out[o_ARQOS].name << " = 'b0;\n";
      out << "assign " << _ports_out[o_ARREGION].name << " = 'b0;\n";
      out << "assign " << _ports_out[o_ARUSER].name << " = 'b0;\n";

      out << R"(
localparam [2:0] S_IDLE = 3'b000,
  S_RD_BURST = 3'b001,
  S_WR_BURST = 3'b101;\n)";

      out << R"(
reg [2:0] _present_state, _next_state;
)";
      out << "reg [" + addr_bitsize + "+(-1):0] axi_awaddr, next_axi_awaddr;\n";
      out << "reg [" + addr_bitsize + "+(-1):0] axi_araddr, next_axi_araddr;\n";
      out << "reg [" + data_bitsize + "+(-1):0] axi_wdata, next_axi_wdata;\n";
      out << "reg [2:0] AWSIZE, next_AWSIZE;\n";
      out << "reg [(" + data_bitsize + "/8)+(-1):0] WSTRB, next_WSTRB;\n";
      out << "reg [1:0] AWBURST, next_AWBURST;\n";
      out << "reg [7:0] AWLEN, next_AWLEN;\n";
      out << "reg AWREADY, next_AWREADY;\n";
      out << "reg [2:0] ARSIZE, next_ARSIZE;\n";
      out << "reg [1:0] ARBURST, next_ARBURST;\n";
      out << "reg [7:0] ARLEN, next_ARLEN;\n";
      unsigned log_data_size = (boost::lexical_cast<unsigned>(data_bitsize) / 8);
      out << "reg [(" + STR(log_data_size) + ")+(-1):0] misalignment;\n";
      out << "reg [(" + data_bitsize + ")+(-1):0] read_mask, next_read_mask;\n";

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
      out << "reg [" + data_bitsize + "+(-1):0] acc_rdata, next_acc_rdata;\n";
      out << "generate\n";
      out << "  assign " << _ports_out[o_AWLEN].name << " = AWLEN;\n";
      out << "  assign " << _ports_out[o_AWSIZE].name << " = AWSIZE;\n";
      out << "  assign " << _ports_out[o_AWBURST].name << " = AWBURST;\n";
      out << "  assign " << _ports_out[o_WSTRB].name << " = WSTRB;\n";
      out << "  assign " << _ports_out[o_ARLEN].name << " = ARLEN;\n";
      out << "  assign " << _ports_out[o_ARSIZE].name << " = ARSIZE;\n";
      out << "  assign " << _ports_out[o_ARBURST].name << " = ARBURST;\n";
      out << "endgenerate\n";

      // Assign reg values
      out << "assign " << _ports_out[o_AWADDR].name << " = axi_awaddr;\n";
      out << "assign " << _ports_out[o_AWVALID].name << " = axi_awvalid;\n";
      out << "assign " << _ports_out[o_WDATA].name << " = axi_wdata;\n";
      out << "assign " << _ports_out[o_WLAST].name << " = axi_wlast;\n";
      out << "assign " << _ports_out[o_WVALID].name << " = axi_wvalid;\n";
      out << "assign " << _ports_out[o_BREADY].name << " = axi_bready;\n";
      out << "assign " << _ports_out[o_ARADDR].name << " = axi_araddr;\n";
      out << "assign " << _ports_out[o_ARVALID].name << " = axi_arvalid;\n";
      out << "assign " << _ports_out[o_RREADY].name << " = axi_rready;\n";
      out << R"(
assign done_port = acc_done;
assign out1 = acc_done ? acc_rdata : 'b0;

initial begin
  _present_state = S_IDLE;
  axi_awaddr = 'b0;
  axi_awvalid = 1'b0;
  axi_wdata = 'b0;
  axi_wlast = 1'b0;
  axi_wvalid = 1'b0;
  axi_bready = 1'b0;
  axi_araddr = 'b0;
  axi_arvalid = 1'b0;
  axi_rready = 1'b0;
  acc_done = 1'b0;
  acc_rdata = 'b0;
  AWLEN = 'b0;
  AWBURST = 'b0;
  ARLEN = 'b0;
  ARBURST = 'b0;
end
  
always @(*) begin
  _next_state = S_IDLE;
  next_axi_awaddr = axi_awaddr;
  next_axi_awvalid = axi_awvalid;
  next_axi_wdata = 'b0;
  next_axi_wlast = 1'b0;
  next_axi_wvalid = 1'b0;
  next_axi_bready = 1'b0;
  next_axi_araddr = axi_araddr;
  next_axi_arvalid = 1'b0;
  next_axi_rready = 1'b0;
  next_acc_done = acc_done;
  next_acc_rdata = acc_rdata;
  next_AWSIZE = AWSIZE;
  next_AWBURST = AWBURST;
  next_AWLEN = AWLEN;
  next_ARSIZE = ARSIZE;
  next_ARBURST = ARBURST;
  next_ARLEN = ARLEN;
  misalignment = 0;
  next_first_read = first_read;
  next_read_mask = read_mask;
  next_AWREADY = AWREADY;
  next_WSTRB = WSTRB;
)";
      out << R"(
   case (_present_state)
    S_IDLE: begin 
      next_axi_awaddr = 'b0;
      next_axi_awvalid = 1'b0;
      next_axi_wdata = 'b0;
      next_axi_wvalid = 1'b0;
      next_axi_bready = 1'b0;
      next_axi_araddr = 'b0;
      next_axi_arvalid = 1'd0;
      next_axi_rready = 1'b1;
      next_acc_done = 1'b0;
      next_acc_rdata = 'b0;
      next_AWSIZE = 'b0;
      next_AWBURST = 'b0;
      next_AWLEN = 'b0;
      next_WSTRB = 'b0;
      next_ARSIZE = 'b0;
      next_ARBURST = 'b0;
      next_ARLEN = 'b0;
      next_first_read = 'b0;
      next_read_mask = 'b0;
      next_acc_rdata = 'b0;
      next_AWREADY = 'b0;
)";
      out << "      if (" << _ports_in[i_start].name << " && !" << _ports_in[i_in1].name << ") begin\n";

      out << R"(
          `ifdef _SIM_HAVE_CLOG2
            next_ARSIZE = $clog2(in2 / 8);
          `else
            next_ARSIZE = `CLOG2(in2 / 8);
          `endif
          next_axi_bready = 1'b0;
          next_axi_rready = 1'b1;
)";

      out << "          next_first_read = 1'b1;\n";
      out << "          next_axi_araddr = " << _ports_in[i_in4].name << ";\n";
      out << "          misalignment = " << _ports_in[i_in4].name << " % (1 << next_ARSIZE);\n";
      out << "          if(misalignment > 'b0) begin\n";
      out << "            next_ARLEN = 'b1;\n";
      out << "            next_ARBURST = 'b1;\n";
      out << "            next_read_mask = -(1 << (misalignment * 8));\n";
      out << "          end else begin\n";
      out << "            next_ARLEN = 'b0;\n";
      out << "            next_ARBURST = 'b0;\n";
      out << "            next_read_mask = -1;\n";
      out << "          end\n";
      out << "          next_axi_arvalid = 1'b1;\n";
      out << "          _next_state = S_RD_BURST;\n";
      out << "      end else if (" << _ports_in[i_start].name << " && " << _ports_in[i_in1].name << ") begin\n";
      out << "        if(" << _ports_in[i_in2].name << " == 'b0) begin\n";
      out << "          next_acc_done = 1'b1;\n";
      out << "        end else begin\n";
      out << "          next_axi_awaddr = " << _ports_in[i_in4].name << ";\n";
      out << "          next_axi_awvalid = 1'b1;\n";
      out << "          `ifdef _SIM_HAVE_CLOG2\n";
      out << "            next_AWSIZE = $clog2(in2 / 8);\n";
      out << "          `else\n";
      out << "            next_AWSIZE = `CLOG2(in2 / 8);\n";
      out << "          `endif\n";
      /* Compute the misalignment, assert all the bits to the left of the misaligned one */
      out << "          misalignment = " << _ports_in[i_in4].name << " % (1 << next_AWSIZE);\n";
      out << "          next_WSTRB = -(1 << misalignment);\n";
      out << "          next_axi_wdata = " << _ports_in[i_in3].name << ";\n";
      out << R"(          next_axi_wvalid = 1'b1;
          next_axi_wlast = !(misalignment > 'b0);
          if(next_axi_wlast) begin
            next_AWBURST = 2'b00;
            next_AWLEN = 8'b00000000;
          end else begin
            next_AWBURST = 2'b01;
            next_AWLEN = 8'b00000001;
          end
          next_axi_rready = 1'b0;
)";
      out << R"(               _next_state = S_WR_BURST;
        end\n
      end else begin
        _next_state = S_IDLE;
      end
    end
)";
      out << " S_RD_BURST: begin\n";
      out << "      if(" << _ports_in[i_ARREADY].name << ") begin\n";
      out << R"(        next_axi_arvalid = 1'b0;
        next_ARSIZE = 'b0;
        next_ARBURST = 'b0;
        next_ARLEN = 'b0;
        next_axi_araddr = 'b0;
        next_axi_arvalid = 1'b0;
      end
      else begin
        next_axi_arvalid = axi_arvalid;
        next_axi_araddr = axi_araddr;
      end
      _next_state = S_RD_BURST;
      next_axi_rready = 1'b1;
      
)";
      out << "      if(" << _ports_in[i_RVALID].name << " && axi_rready) begin\n";
      out << R"(          if(!first_read) begin
            if(~read_mask != 'b0)
              next_acc_rdata = acc_rdata | ()" +
                 _ports_in[i_RDATA].name + R"( & (~read_mask));
            next_axi_rready = 1'b0;
)";
      out << "             if(!" << _ports_in[i_RLAST].name << ") begin\n";

      out << "                 _next_state = S_RD_BURST;\n";

      out << "             end else begin\n";
      out << "               _next_state = S_IDLE;\n";
      out << "               next_acc_done = 1'b1;\n";
      out << "             end\n";
      out << R"(           end else if()" + _ports_in[i_RLAST].name +
                 R"() begin
            next_acc_rdata = )" +
                 _ports_in[i_RDATA].name + R"( & read_mask;
)";
      out << "            next_axi_rready = 1'b0;\n";
      out << "            next_acc_done = 1'b1;\n";
      out << "            _next_state = S_IDLE;\n";
      out << "          end else if (first_read) begin\n";
      out << "            next_acc_rdata = " << _ports_in[i_RDATA].name << " & read_mask;\n";
      out << "               _next_state = S_RD_BURST;\n";
      out << R"(
            next_acc_done = 1'b0;
            next_first_read = 1'b0;
          end
      end else begin
        _next_state = S_RD_BURST;
      end
    end

    S_WR_BURST : begin 
    _next_state = S_WR_BURST;
    next_axi_bready = 1'b1;
)";
      out << "      if(!" << _ports_in[i_WREADY].name << ") begin\n";
      out << "        next_axi_wvalid = axi_wvalid;\n";
      out << "        next_axi_wlast = axi_wlast;\n";
      out << "        next_axi_wdata = axi_wdata;\n";
      out << "      end\n";
      out << "      if(" << _ports_in[i_AWREADY].name << ") begin";
      out << R"(
        next_AWSIZE = 'b0;
        next_AWBURST = 'b0;
        next_AWLEN = 'b0;
        next_axi_awvalid = 'b0;
        next_axi_awaddr = 'b0;
        next_AWREADY = 1'b1;
      end
)";

      out << "      if (next_AWREADY &&" << _ports_in[i_WREADY].name << " && !WSTRB[0]) begin";
      out << R"(
        /* If the last transfer was not aligned and the slave is ready, transfer the rest */
        next_WSTRB = ~WSTRB;
        next_axi_wdata = axi_wdata;
        next_axi_wvalid = 1'b1;
        next_axi_wlast = 1'b1;
)";
      out << R"(      end
      else if (next_AWREADY && !WSTRB[0]) begin
        /* If it's an aligned transfer but the slave is not ready, just keep the signals */
        next_axi_wdata = axi_wdata;
        next_axi_wvalid = axi_wvalid;
        next_WSTRB = WSTRB;
        next_axi_wlast = axi_wlast;
      end
      if(!next_AWREADY) begin
        next_axi_awvalid = axi_awvalid;
        next_axi_awaddr = axi_awaddr;
        next_axi_wvalid = axi_wvalid;
        next_axi_wdata = axi_wdata;
      end 
      /* If the last transfer was complete, deassert the validity bits and check if you can go back to
      IDLE */
)";
      out << "      if (" << _ports_in[i_BVALID].name << ") begin\n";
      out << R"(        next_acc_done = 1'b1;
        next_axi_wvalid = 1'b0;
        next_axi_wdata = 'b0;
        next_WSTRB = 'b0;
        next_axi_wlast = 1'b0;
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
  AWSIZE <= next_AWSIZE;
  AWBURST <= next_AWBURST;
  AWLEN <= next_AWLEN;
  WSTRB <= next_WSTRB;
  ARSIZE <= next_ARSIZE;
  ARBURST <= next_ARBURST;
  ARLEN <= next_ARLEN;
  first_read <= next_first_read;
  read_mask <= next_read_mask;
  AWREADY <= next_AWREADY;
)";
      out << R"(
  if(!reset) begin 
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
      out << "localparam S_IDLE = 1'b0, S_FLUSH = 1'b1;\n";
      out << "assign " << _ports_out[o_ARUSER].name << " = 0;\n";
      out << "assign " << _ports_out[o_ARREGION].name << " = 0;\n";
      out << "assign " << _ports_out[o_WUSER].name << " = 0;\n";
      out << "assign " << _ports_out[o_WID].name << " = 0;\n";
      out << "assign " << _ports_out[o_AWUSER].name << " = 0;\n";
      out << "assign " << _ports_out[o_AWREGION].name << " = 0;\n";

      out << R"(
      assign done_port = state == S_IDLE? ready : !dirty;
      assign addr = in4[BITSIZE_in4 - 1:$clog2(BITSIZE_in3 / 8)];
      assign wstrb = in1 ? (1 << (in2 / 8)) - 1 : 0;
      assign out1 = done_port ? rdata : 'b0;

      always @(*) begin
        state_next = state;
        if(state == S_IDLE) begin
)";
      out << "          if(" << _ports_in[i_start].name << " && " << _ports_in[i_in1].name << " && "
          << _ports_in[i_in2].name << " == 'b0) begin\n";
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
      out << "        if(!reset) begin\n";
      out << "          state <= S_IDLE;\n";
      out << "        end\n";
      out << "      end\n";

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
          .CTRL_CACHE(0),
          .CTRL_CNT(0),
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
          << _ports_in[i_in2].name << R"( == 'b0)),
          .dirty(dirty),
          .flush(state == S_FLUSH),
)";
      out << "          .m_axi_awready(" << _ports_in[i_AWREADY].name << "),\n";
      out << "          .m_axi_wready(" << _ports_in[i_WREADY].name << "),\n";
      out << "          .m_axi_bid(" << _ports_in[i_BID].name << "),\n";
      out << "          .m_axi_bresp(" << _ports_in[i_BRESP].name << "),\n";
      out << "          .m_axi_bvalid(" << _ports_in[i_BVALID].name << "),\n";
      out << "          .m_axi_arready(" << _ports_in[i_ARREADY].name << "),\n";
      out << "          .m_axi_rid(" << _ports_in[i_RID].name << "),\n";
      out << "          .m_axi_rdata(" << _ports_in[i_RDATA].name << "),\n";
      out << "          .m_axi_rresp(" << _ports_in[i_RRESP].name << "),\n";
      out << "          .m_axi_rlast(" << _ports_in[i_RLAST].name << "),\n";
      out << "          .m_axi_rvalid(" << _ports_in[i_RVALID].name << "),\n";
      out << "          .m_axi_awid(" << _ports_out[o_AWID].name << "),\n";
      out << "          .m_axi_awaddr(" << _ports_out[o_AWADDR].name << "),\n";
      out << "          .m_axi_awlen(" << _ports_out[o_AWLEN].name << "),\n";
      out << "          .m_axi_awsize(" << _ports_out[o_AWSIZE].name << "),\n";
      out << "          .m_axi_awburst(" << _ports_out[o_AWBURST].name << "),\n";
      out << "          .m_axi_awlock(" << _ports_out[o_AWLOCK].name << "),\n";
      out << "          .m_axi_awcache(" << _ports_out[o_AWCACHE].name << "),\n";
      out << "          .m_axi_awprot(" << _ports_out[o_AWPROT].name << "),\n";
      out << "          .m_axi_awqos(" << _ports_out[o_AWQOS].name << "),\n";
      out << "          .m_axi_awvalid(" << _ports_out[o_AWVALID].name << "),\n";
      out << "          .m_axi_wdata(" << _ports_out[o_WDATA].name << "),\n";
      out << "          .m_axi_wstrb(" << _ports_out[o_WSTRB].name << "),\n";
      out << "          .m_axi_wlast(" << _ports_out[o_WLAST].name << "),\n";
      out << "          .m_axi_wvalid(" << _ports_out[o_WVALID].name << "),\n";
      out << "          .m_axi_bready(" << _ports_out[o_BREADY].name << "),\n";
      out << "          .m_axi_arid(" << _ports_out[o_ARID].name << "),\n";
      out << "          .m_axi_araddr(" << _ports_out[o_ARADDR].name << "),\n";
      out << "          .m_axi_arlen(" << _ports_out[o_ARLEN].name << "),\n";
      out << "          .m_axi_arsize(" << _ports_out[o_ARSIZE].name << "),\n";
      out << "          .m_axi_arburst(" << _ports_out[o_ARBURST].name << "),\n";
      out << "          .m_axi_arlock(" << _ports_out[o_ARLOCK].name << "),\n";
      out << "          .m_axi_arcache(" << _ports_out[o_ARCACHE].name << "),\n";
      out << "          .m_axi_arprot(" << _ports_out[o_ARPROT].name << "),\n";
      out << "          .m_axi_arqos(" << _ports_out[o_ARQOS].name << "),\n";
      out << "          .m_axi_arvalid(" << _ports_out[o_ARVALID].name << "),\n";
      out << "          .m_axi_rready(" << _ports_out[o_RREADY].name << "),\n";
      out << "          .clk(clock),\n";
      out << "          .reset(!reset) /* IOB reset is active high */\n";
      out << "       );\n";
   }
}