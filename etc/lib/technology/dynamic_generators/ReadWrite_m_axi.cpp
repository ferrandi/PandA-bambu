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
std::cout << "  assign " << _ports_out[AWLEN].name << " = 8'b00000000;\n";
std::cout << "  assign " << _ports_out[AWSIZE].name << " = 3'b100;\n";
std::cout << "  assign " << _ports_out[AWBURST].name << " = 2'b00;\n";
std::cout << "  assign " << _ports_out[WSTRB].name << " = 2'b11;\n";
std::cout << "  assign " << _ports_out[ARLEN].name << " = 8'b00000000;\n";
std::cout << "  assign " << _ports_out[ARSIZE].name << " = 3'b100;\n";
std::cout << "  assign " << _ports_out[ARBURST].name << " = 2'b00;\n";
std::cout << "endgenerate\n";

std::cout << R"(
reg [2:0] _present_state, _next_state;
)";
std::cout << "reg [BITSIZE_" + _ports_out[AWADDR].name + "+(-1):0] axi_awaddr, next_axi_awaddr;\n";
std::cout << "reg [BITSIZE_" + _ports_out[ARADDR].name + "+(-1):0] axi_araddr, next_axi_araddr;\n";
std::cout << "reg [BITSIZE_" + _ports_out[WDATA].name + "+(-1):0] axi_wdata, next_axi_wdata;";
std::cout << R"(
reg axi_awvalid, next_axi_awvalid;
reg axi_wlast, next_axi_wlast;
reg axi_wvalid, next_axi_wvalid;
reg axi_bready, next_axi_bready;
reg axi_arvalid, next_axi_arvalid;
reg axi_rready, next_axi_rready;

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
end
  
always @(*) begin
  _next_state = S_IDLE;
  next_axi_awaddr = axi_awaddr;
  next_axi_awvalid = axi_awvalid;
  next_axi_wdata = axi_wdata;
  next_axi_wlast = axi_wlast;
  next_axi_wvalid = axi_wvalid;
  next_axi_bready = 1'b0;
  next_axi_araddr = axi_araddr;
  next_axi_arvalid = axi_arvalid;
  next_axi_rready = 1'b0;
  next_acc_done = acc_done;
  next_acc_rdata = acc_rdata;

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
)";
std::cout << "      if (" << _ports_in[start_port].name << " && !" << _ports_in[in1].name << ") begin\n";
std::cout << "        next_axi_bready = 1'b0;\n";
std::cout << "        next_axi_araddr = " << _ports_in[in4].name << ";\n";
std::cout << "        next_axi_arvalid = 1'b1;\n";
std::cout << "        _next_state = S_RD_BURST;\n";
std::cout << "      end else if (" << _ports_in[start_port].name << " && " << _ports_in[in1].name << ") begin\n";
std::cout << "        next_axi_awaddr = " << _ports_in[in4].name << ";\n";
std::cout << "        next_axi_awvalid = 1'b1;\n";
std::cout << "        next_axi_wdata = " << _ports_in[in3].name << ";\n";
std::cout << R"(        next_axi_wvalid = 1'b1;
        next_axi_wlast = 1'b1;
        next_axi_rready = 1'b0;
        _next_state = S_WR_BURST;
      end else begin
        _next_state = S_IDLE;
      end
    end

    S_RD_BURST: begin
      next_axi_arvalid = 1'b0;
      next_axi_rready = 1'b1;
      next_acc_done = 1'b0;
)";
std::cout << "      if (axi_rready && " << _ports_in[RVALID].name << ") begin\n";
std::cout << "        next_acc_rdata = " << _ports_in[RDATA].name << ";\n";
std::cout << R"(        next_acc_done = 1'b1;
        next_axi_araddr = 'b0;
        _next_state = S_IDLE;
      end else begin
        _next_state = S_RD_BURST;
      end
    end

    S_WR_BURST: begin
      next_axi_awvalid = 1'b0;
      next_axi_wvalid = 1'b0;
)";
std::cout << "      if (" << _ports_in[BVALID].name << ") begin\n";
std::cout << R"(        next_acc_done = 1'b1;
        _next_state = S_IDLE;
      end else begin  
        _next_state = S_WR_BURST;
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

  if (!reset) begin
    _present_state <= S_IDLE;
  end
end)";
