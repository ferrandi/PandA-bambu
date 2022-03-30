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
 *                 URL: http://trac.ws.dei.polimi.it/panda
 *                      Microarchitectures Laboratory
 *                       Politecnico di Milano - DEIB
 *             ***********************************************
 *              Copyright (c) 2018-2022 Politecnico di Milano
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA.
 *
 */
/**
 * @file ReadWrite_m_axi.cpp
 * @brief Snippet for the ReadWrite_m_axi dynamic generator.
 *
 * @author Ankur Limaye <ankur.limaye@pnnl.gov>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */

enum in_port
{
   clock = 0,
   reset,
   start_port,
   in1,
   in2,
   in3,
   in4,
   AWREADY,
   WREADY,
   BID,
   BRESP,
   BUSER,
   BVALID,
   ARREADY,
   RID,
   RDATA,
   RRESP,
   RLAST,
   RUSER,
   RVALID,
   _n_ptr,
};

enum out_port
{
   done_port = 0,
   out1,
   AWID,
   AWADDR,
   AWLEN,
   AWSIZE,
   AWBURST,
   AWLOCK,
   AWCACHE,
   AWPROT,
   AWQOS,
   AWREGION,
   AWUSER,
   AWVALID,
   WID,
   WDATA,
   WSTRB,
   WLAST,
   WUSER,
   WVALID,
   BREADY,
   ARID,
   ARADDR,
   ARLEN,
   ARSIZE,
   ARBURST,
   ARLOCK,
   ARCACHE,
   ARPROT,
   ARQOS,
   ARREGION,
   ARUSER,
   ARVALID,
   RREADY
};

std::cout << R"(
`ifndef _SIM_HAVE_CLOG2
  function integer log2;
    input integer value;
    integer temp_value;
    begin
      temp_value = value - 1;
      for(log2 = 0; temp_value > 0; log2 = log2 + 1)
        temp_value = temp_value >> 1;
    end
  endfunction
`endif

)";

std::cout << "assign " << _ports_out[AWID].name << " = 'b0;\n";
std::cout << "assign " << _ports_out[AWLOCK].name << " = 'b0;\n";
std::cout << "assign " << _ports_out[AWCACHE].name << " = 'b0;\n";
std::cout << "assign " << _ports_out[AWPROT].name << " = 'b0;\n";
std::cout << "assign " << _ports_out[AWQOS].name << " = 'b0;\n";
std::cout << "assign " << _ports_out[AWREGION].name << " = 'b0;\n";
std::cout << "assign " << _ports_out[AWUSER].name << " = 'b0;\n";
std::cout << "assign " << _ports_out[WID].name << " = 'b0;\n";
std::cout << "assign " << _ports_out[WUSER].name << " = 'b0;\n";
std::cout << "assign " << _ports_out[ARID].name << " = 'b0;\n";
std::cout << "assign " << _ports_out[ARLOCK].name << " = 'b0;\n";
std::cout << "assign " << _ports_out[ARCACHE].name << " = 'b0;\n";
std::cout << "assign " << _ports_out[ARPROT].name << " = 'b0;\n";
std::cout << "assign " << _ports_out[ARQOS].name << " = 'b0;\n";
std::cout << "assign " << _ports_out[ARREGION].name << " = 'b0;\n";
std::cout << "assign " << _ports_out[ARUSER].name << " = 'b0;\n";

std::cout << R"(
localparam [2:0] S_IDLE = 3'b000,
  S_RD_BURST = 3'b001,
  S_WR_BURST = 3'b101;
)";

std::cout << "generate\n";
std::cout << "  assign " << _ports_out[AWLEN].name << " = AWLEN;\n";
std::cout << "  assign " << _ports_out[AWSIZE].name << " = AWSIZE;\n";
std::cout << "  assign " << _ports_out[AWBURST].name << " = AWBURST;\n";
std::cout << "  assign " << _ports_out[WSTRB].name << " = WSTRB;\n";
std::cout << "  assign " << _ports_out[ARLEN].name << " = ARLEN;\n";
std::cout << "  assign " << _ports_out[ARSIZE].name << " = ARSIZE;\n";
std::cout << "  assign " << _ports_out[ARBURST].name << " = ARBURST;\n";
std::cout << "endgenerate\n";

std::cout << R"(
reg [2:0] _present_state, _next_state;
)";
std::cout << "reg [BITSIZE_" + _ports_out[AWADDR].name + "+(-1):0] axi_awaddr, next_axi_awaddr;\n";
std::cout << "reg [BITSIZE_" + _ports_out[ARADDR].name + "+(-1):0] axi_araddr, next_axi_araddr;\n";
std::cout << "reg [BITSIZE_" + _ports_out[WDATA].name + "+(-1):0] axi_wdata, next_axi_wdata;\n";
std::cout << "reg [2:0] AWSIZE, next_AWSIZE;\n";
std::cout << "reg [(BITSIZE_" + _ports_out[WDATA].name + "/8)+(-1):0] WSTRB, next_WSTRB;\n";
std::cout << "reg [1:0] AWBURST, next_AWBURST;\n";
std::cout << "reg [7:0] AWLEN, next_AWLEN;\n";
std::cout << "reg AWREADY;\n";
std::cout << "reg [2:0] ARSIZE, next_ARSIZE;\n";
std::cout << "reg [1:0] ARBURST, next_ARBURST;\n";
std::cout << "reg [7:0] ARLEN, next_ARLEN;\n";
/* This register will most likely be oversized. The actual size would be the logarithm of the size of WDATA in bytes,
 * but I don't think that it's possible to use the log when declaring a reg */
std::cout << "reg [(BITSIZE_" + _ports_out[WDATA].name + "/8)+(-1):0] misalignment, next_misalignment;\n";
std::cout << "reg [(BITSIZE_" + _ports_out[WDATA].name + ")+(-1):0] read_mask;\n";

std::cout << R"(
reg axi_awvalid, next_axi_awvalid;
reg axi_wlast, next_axi_wlast;
reg axi_wvalid, next_axi_wvalid;
reg axi_bready, next_axi_bready;
reg axi_arvalid, next_axi_arvalid;
reg axi_rready, next_axi_rready;

reg first_read, next_first_read;

reg acc_done, next_acc_done;
)";
std::cout << "reg [BITSIZE_" + _ports_in[RDATA].name + "+(-1):0] acc_rdata, next_acc_rdata;\n";

// Assign reg values
std::cout << "assign " << _ports_out[AWADDR].name << " = axi_awaddr;\n";
std::cout << "assign " << _ports_out[AWVALID].name << " = axi_awvalid;\n";
std::cout << "assign " << _ports_out[WDATA].name << " = axi_wdata;\n";
std::cout << "assign " << _ports_out[WLAST].name << " = axi_wlast;\n";
std::cout << "assign " << _ports_out[WVALID].name << " = axi_wvalid;\n";
std::cout << "assign " << _ports_out[BREADY].name << " = axi_bready;\n";
std::cout << "assign " << _ports_out[ARADDR].name << " = axi_araddr;\n";
std::cout << "assign " << _ports_out[ARVALID].name << " = axi_arvalid;\n";
std::cout << "assign " << _ports_out[RREADY].name << " = axi_rready;\n";
std::cout << R"(
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
  next_misalignment = 0;

  case (_present_state)
    S_IDLE: begin
      next_axi_awaddr = 'b0;
      next_axi_awvalid = 1'b0;
      next_axi_wdata = 'b0;
      next_axi_wvalid = 1'b0;
      next_axi_bready = 1'b1;
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
      read_mask = 'b0;
      next_acc_rdata = 'b0;
      AWREADY = 'b0;
)";
std::cout << "      if (" << _ports_in[start_port].name << " && !" << _ports_in[in1].name << ") begin\n";
std::cout << R"(        `ifdef _SIM_HAVE_CLOG2
          next_ARSIZE = $clog2(in2 / 8);
        `else
          next_ARSIZE = log2(in2 / 8);
        `endif
        next_axi_bready = 1'b0;
        next_axi_rready = 1'b1;
)";
std::cout << "        next_first_read = 1'b1;\n";
std::cout << "        next_axi_araddr = " << _ports_in[in4].name << ";\n";
std::cout << "        next_misalignment = " << _ports_in[in4].name << " % (1 << next_ARSIZE);\n";
std::cout << "        if(next_misalignment > 'b0) begin\n";
std::cout << "          next_ARLEN = 'b1;\n";
std::cout << "          next_ARBURST = 'b1;\n";
std::cout << "          read_mask = -(1 << (next_misalignment * 8));\n";
std::cout << "        end else begin\n";
std::cout << "          next_ARLEN = 'b0;\n";
std::cout << "          next_ARBURST = 'b0;\n";
std::cout << "        end\n";
std::cout << "        next_axi_arvalid = 1'b1;\n";
std::cout << "        _next_state = S_RD_BURST;\n";
std::cout << "      end else if (" << _ports_in[start_port].name << " && " << _ports_in[in1].name << ") begin\n";
std::cout << "        next_axi_awaddr = " << _ports_in[in4].name << ";\n";
std::cout << "        next_axi_awvalid = 1'b1;\n";
std::cout << "        `ifdef _SIM_HAVE_CLOG2\n";
std::cout << "          next_AWSIZE = $clog2(in2 / 8);\n";
std::cout << "        `else\n";
std::cout << "          next_AWSIZE = log2(in2 / 8);\n";
std::cout << "        `endif\n";
/* Compute the misalignment, assert all the bits to the left of the misaligned one */
std::cout << "        next_misalignment = " << _ports_in[in4].name << " % (1 << next_AWSIZE);\n";
std::cout << "        next_WSTRB = -(1 << next_misalignment);\n";
std::cout << "        next_axi_wdata = " << _ports_in[in3].name << ";\n";
std::cout << R"(        next_axi_wvalid = 1'b1;
        next_axi_wlast = !(next_misalignment > 'b0);
        if(next_axi_wlast) begin
          next_AWBURST = 2'b00;
          next_AWLEN = 8'b00000000;
        end else begin
          next_AWBURST = 2'b01;
          next_AWLEN = 8'b00000001;
        end
        next_axi_rready = 1'b0;
        _next_state = S_WR_BURST;
      end else begin
        _next_state = S_IDLE;
      end
    end

    S_RD_BURST: begin
)";
std::cout << "      if(" << _ports_in[ARREADY].name << ") begin\n";
std::cout << R"(        next_axi_arvalid = 1'b0;
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
std::cout << "      if(" << _ports_in[RVALID].name << ") begin\n";
std::cout << "        if(" << _ports_in[RLAST].name << ") begin\n";
std::cout << "          next_acc_rdata = next_acc_rdata + (" << _ports_in[RDATA].name << " & (~read_mask));\n";
std::cout << "          next_axi_rready = 1'b0;\n";
std::cout << "          next_acc_done = 1'b1;\n";
std::cout << "          _next_state = S_IDLE;\n";
std::cout << "        end else if (first_read) begin\n";
std::cout << "          next_acc_rdata = " << _ports_in[RDATA].name << " & read_mask;\n";
std::cout << R"(          next_acc_done = 1'b0;
          next_first_read = 1'b0;
          _next_state = S_RD_BURST;
        end
      end else begin
        _next_state = S_RD_BURST;
      end
    end

    S_WR_BURST : begin 
    _next_state = S_WR_BURST;
)";
std::cout << "      if(!" << _ports_in[WREADY].name << ") begin\n";
std::cout << "        next_axi_wvalid = axi_wvalid;\n";
std::cout << "        next_axi_wlast = axi_wlast;\n";
std::cout << "        next_axi_wdata = axi_wdata;\n";
std::cout << "        next_WSTRB = WSTRB;";
std::cout << "      end\n";
std::cout << "      if(" << _ports_in[AWREADY].name << ") begin";
std::cout << R"(
        next_AWSIZE = 'b0;
        next_AWBURST = 'b0;
        next_AWLEN = 'b0;
        next_axi_awvalid = 'b0;
        next_axi_awaddr = 'b0;
        AWREADY = 1'b1;
      end
)";

std::cout << "      if (AWREADY &&" << _ports_in[WREADY].name << " && !WSTRB[0]) begin";
std::cout << R"(
        /* If the last transfer was not aligned and the slave is ready, transfer the rest */
        next_WSTRB = ~WSTRB;
        next_axi_wdata = axi_wdata;
        next_axi_wvalid = 1'b1;
        next_axi_wlast = 1'b1;
      end
      else if (AWREADY && !WSTRB[0]) begin
        /* If it's an aligned transfer but the slave is not ready, just keep the signals */
        next_axi_wdata = axi_wdata;
        next_axi_wvalid = axi_wvalid;
        next_WSTRB = WSTRB;
        next_axi_wlast = axi_wlast;
      end
      if(!AWREADY) begin
        next_axi_awvalid = axi_awvalid;
        next_axi_awaddr = axi_awaddr;
        next_axi_wvalid = axi_wvalid;
        next_axi_wdata = axi_wdata;
      end 
      /* If the last transfer was complete, deassert the validity bits and check if you can go back to
      IDLE */
)";
std::cout << "      if (" << _ports_in[BVALID].name << ") begin";
std::cout << R"(        next_acc_done = 1'b1;
        next_axi_wvalid = 1'b0;
        next_axi_wdata = 'b0;
        next_WSTRB = 'b0;
        next_axi_wlast = 1'b0;
        _next_state = S_IDLE;
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
  misalignment <= next_misalignment;
  first_read <= next_first_read;

  if (!reset) begin
    _present_state <= S_IDLE;
  end
end)";
