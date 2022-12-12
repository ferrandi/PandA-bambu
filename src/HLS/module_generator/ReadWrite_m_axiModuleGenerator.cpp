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
 *              Copyright (C) 2022-2022 Politecnico di Milano
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

enum in_port
{
   clock_s = 0,
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

ReadWrite_m_axiModuleGenerator::ReadWrite_m_axiModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void ReadWrite_m_axiModuleGenerator::InternalExec(std::ostream& out, const module* /* mod */,
                                                  unsigned int /* function_id */, vertex /* op_v */,
                                                  const HDLWriter_Language /* language */,
                                                  const std::vector<ModuleGenerator::parameter>& /* _p */,
                                                  const std::vector<ModuleGenerator::parameter>& _ports_in,
                                                  const std::vector<ModuleGenerator::parameter>& _ports_out,
                                                  const std::vector<ModuleGenerator::parameter>& /* _ports_inout */)
{
   const auto addr_bitsize = STR(_ports_out[AWADDR].type_size);
   const auto data_bitsize = STR(_ports_out[WDATA].type_size);

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

   out << "assign " << _ports_out[AWID].name << " = 'b0;\n";
   out << "assign " << _ports_out[AWLOCK].name << " = 'b0;\n";
   out << "assign " << _ports_out[AWCACHE].name << " = 'b0;\n";
   out << "assign " << _ports_out[AWPROT].name << " = 'b0;\n";
   out << "assign " << _ports_out[AWQOS].name << " = 'b0;\n";
   out << "assign " << _ports_out[AWREGION].name << " = 'b0;\n";
   out << "assign " << _ports_out[AWUSER].name << " = 'b0;\n";
   out << "assign " << _ports_out[WID].name << " = 'b0;\n";
   out << "assign " << _ports_out[WUSER].name << " = 'b0;\n";
   out << "assign " << _ports_out[ARID].name << " = 'b0;\n";
   out << "assign " << _ports_out[ARLOCK].name << " = 'b0;\n";
   out << "assign " << _ports_out[ARCACHE].name << " = 'b0;\n";
   out << "assign " << _ports_out[ARPROT].name << " = 'b0;\n";
   out << "assign " << _ports_out[ARQOS].name << " = 'b0;\n";
   out << "assign " << _ports_out[ARREGION].name << " = 'b0;\n";
   out << "assign " << _ports_out[ARUSER].name << " = 'b0;\n";

   out << R"(
localparam [2:0] S_IDLE = 3'b000,
  S_RD_BURST = 3'b001,
  S_WR_BURST = 3'b101 `ifdef CACHE , S_RD_CACHE = 3'b111, S_STALL_CACHE = 3'b010 `endif;
)";

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
   /* This register will most likely be oversized. The actual size would be the logarithm of the size of WDATA in bytes,
    * but I don't think that it's possible to use the log when declaring a reg */
   out << "reg [(" + data_bitsize + "/8)+(-1):0] misalignment;\n";
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

   out << R"(
   `ifdef CACHE 
     reg   [)" +
              addr_bitsize + R"(+(-1):0]     M_address, next_M_address;
     wire  [)" +
              data_bitsize + R"(+(-1):0]    M_rddata;
     reg   [)" +
              data_bitsize + R"(+(-1):0]    M_wrdata, next_M_wrdata;
     reg               M_write, next_M_write;
     reg               M_read, next_M_read;
     wire              M_wait;
    
     wire  [)" +
              addr_bitsize + R"(+(-1):0]    S_address;
     wire  [)" +
              data_bitsize + R"(+(-1):0]    S_rddata;
     wire  [)" +
              data_bitsize + R"(+(-1):0]    S_wrdata;
     wire              S_write;
     wire              S_read;
     wire              S_wait;
    
     cache_top cache(
       .clk(clock),
       .M_address(M_address),
       .M_rddata(M_rddata),
       .M_wrdata(M_wrdata),
       .M_write(M_write),
       .M_read(M_read),
       .M_wait (M_wait),
       .S_address(S_address),
       .S_rddata(S_rddata),
       .S_wrdata(S_wrdata),
       .S_write (S_write),
       .S_read(S_read),
       .S_wait(S_wait)
     );
   `endif 
   )";

   out << "generate\n";
   out << "  assign " << _ports_out[AWLEN].name << " = AWLEN;\n";
   out << "  assign " << _ports_out[AWSIZE].name << " = AWSIZE;\n";
   out << "  assign " << _ports_out[AWBURST].name << " = AWBURST;\n";
   out << "  assign " << _ports_out[WSTRB].name << " = WSTRB;\n";
   out << "  assign " << _ports_out[ARLEN].name << " = ARLEN;\n";
   out << "  assign " << _ports_out[ARSIZE].name << " = ARSIZE;\n";
   out << "  assign " << _ports_out[ARBURST].name << " = ARBURST;\n";
   out << "endgenerate\n";
   out << "`ifdef CACHE\n";
   out << "  assign S_wait = S_read                                    ? !" << _ports_out[RREADY].name << " || !"
       << _ports_in[RVALID].name << " :\n";
   out << "                  S_write && _present_state == S_WR_BURST   ? !" << _ports_in[WREADY].name << " || !"
       << _ports_out[WVALID].name << " :\n";
   out << "                  1'b0;\n";
   out << "  assign S_rddata = " << _ports_in[RVALID].name << " ? " << _ports_in[RDATA].name << " : 'b0;\n";
   out << "`endif\n";

   // Assign reg values
   out << "assign " << _ports_out[AWADDR].name << " = axi_awaddr;\n";
   out << "assign " << _ports_out[AWVALID].name << " = axi_awvalid;\n";
   out << "assign " << _ports_out[WDATA].name << " = axi_wdata;\n";
   out << "assign " << _ports_out[WLAST].name << " = axi_wlast;\n";
   out << "assign " << _ports_out[WVALID].name << " = axi_wvalid;\n";
   out << "assign " << _ports_out[BREADY].name << " = axi_bready;\n";
   out << "assign " << _ports_out[ARADDR].name << " = axi_araddr;\n";
   out << "assign " << _ports_out[ARVALID].name << " = axi_arvalid;\n";
   out << "assign " << _ports_out[RREADY].name << " = axi_rready;\n";
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
  `ifdef CACHE
    next_M_address = 'b0;
    next_M_read = 1'b0;
    next_M_write = 1'b0;
    next_M_wrdata = 'b0;
  `endif  

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
      next_read_mask = 'b0;
      next_acc_rdata = 'b0;
      next_AWREADY = 'b0;
)";
   out << "      if (" << _ports_in[start_port].name << " && !" << _ports_in[in1].name << ") begin\n";
   out << R"(        
        `ifdef CACHE
          next_M_read = 1'b1;
          next_first_read = 1'b1;
          `ifndef _SIM_HAVE_CLOG2
            misalignment = in4 % (1 << `CLOG2(in2/8));
          `else
            misalignment = in4 % (1 << $clog2(in2/8));
          `endif
          
          next_M_address = in4 - misalignment;
          next_read_mask = -(1 << (misalignment * 8));
          _next_state = S_RD_CACHE;
        `else
          `ifdef _SIM_HAVE_CLOG2
            next_ARSIZE = $clog2(in2 / 8);
          `else
            next_ARSIZE = `CLOG2(in2 / 8);
          `endif
          next_axi_bready = 1'b0;
          next_axi_rready = 1'b1;
)";
   out << "        next_first_read = 1'b1;\n";
   out << "        next_axi_araddr = " << _ports_in[in4].name << ";\n";
   out << "        misalignment = " << _ports_in[in4].name << " % (1 << next_ARSIZE);\n";
   out << "        if(misalignment > 'b0) begin\n";
   out << "          next_ARLEN = 'b1;\n";
   out << "          next_ARBURST = 'b1;\n";
   out << "          next_read_mask = -(1 << (misalignment * 8));\n";
   out << "        end else begin\n";
   out << "          next_ARLEN = 'b0;\n";
   out << "          next_ARBURST = 'b0;\n";
   out << "          next_read_mask = -1;\n";
   out << "        end\n";
   out << "        next_axi_arvalid = 1'b1;\n";
   out << "        _next_state = S_RD_BURST;\n";
   out << "      `endif\n";
   out << "      end else if (" << _ports_in[start_port].name << " && " << _ports_in[in1].name << ") begin\n";
   out << "        next_axi_awaddr = " << _ports_in[in4].name << ";\n";
   out << "        next_axi_awvalid = 1'b1;\n";
   out << "        `ifdef _SIM_HAVE_CLOG2\n";
   out << "          next_AWSIZE = $clog2(in2 / 8);\n";
   out << "        `else\n";
   out << "          next_AWSIZE = `CLOG2(in2 / 8);\n";
   out << "        `endif\n";
   /* Compute the misalignment, assert all the bits to the left of the misaligned one */
   out << "        misalignment = " << _ports_in[in4].name << " % (1 << next_AWSIZE);\n";
   out << "        next_WSTRB = -(1 << misalignment);\n";
   out << "        next_axi_wdata = " << _ports_in[in3].name << ";\n";
   out << R"(        next_axi_wvalid = 1'b1;
        next_axi_wlast = !(misalignment > 'b0);
        if(next_axi_wlast) begin
          next_AWBURST = 2'b00;
          next_AWLEN = 8'b00000000;
        end else begin
          next_AWBURST = 2'b01;
          next_AWLEN = 8'b00000001;
        end
        next_axi_rready = 1'b0;
        `ifdef CACHE
          next_M_address = in4 - misalignment;
          next_M_write = 1'b1;
          next_M_wrdata = in3;
        `endif
        _next_state = S_WR_BURST;
      end else begin
        _next_state = S_IDLE;
      end
    end

    `ifdef CACHE
      S_RD_CACHE : begin
        next_M_address = M_address;
        next_M_read = M_read;
        if(M_wait && !S_wait)
          _next_state = S_RD_CACHE;
        else if(S_wait) begin   /* Cache miss */
          `ifdef _SIM_HAVE_CLOG2
            next_ARSIZE = $clog2(in2 / 8);
          `else
            next_ARSIZE = `CLOG2(in2 / 8);
          `endif
          next_axi_bready = 1'b0;
          next_axi_rready = 1'b1;
          if(read_mask != -1 && first_read) begin
            next_ARLEN = 'b1;
            next_ARBURST = 'b1;
          end else begin
            next_ARLEN = 'b0;
            next_ARBURST = 'b0;
            if(read_mask != -1) begin
              next_axi_arvalid = 1'b1;
              next_axi_araddr = M_address;
            end
          end
          if(first_read) begin
            next_axi_arvalid = 1'b1;
            next_axi_araddr = in4;      
          end
          _next_state = S_RD_BURST;
        end else begin  /* Cache hit */
          /* Data is ready */
          if(!M_wait) begin
            if(first_read) begin
              next_acc_rdata = M_rddata & read_mask;
              next_first_read = 1'b0;
              if(read_mask != -1) begin
                next_M_address = M_address + (1 << `ifdef _SIM_HAVE_CLOG2 $clog2(in2 / 8) `else `CLOG2(in2 / 8) `endif);
                _next_state = S_RD_CACHE;
              end
              else begin
                next_acc_done = 1'b1;
                next_M_address = 'b0;
                next_M_read = 1'b0;
                _next_state = S_IDLE;
              end
            end
            else begin
              next_acc_rdata = acc_rdata | ()" +
              _ports_in[RDATA].name + R"( & (~read_mask));
              next_acc_done = 1'b1;
              next_M_address = 'b0;
              next_M_read = 1'b0;
              _next_state = S_IDLE; 
            end
          end
        end
      end

      S_STALL_CACHE: begin
        _next_state = S_RD_BURST;
        next_axi_rready = 1'b0;
        next_M_address = M_address;
        next_M_read = M_read;
      end
    `endif

    S_RD_BURST: begin
)";
   out << R"(
      `ifdef CACHE 
        next_M_address = M_address;
        next_M_read = M_read;
      `endif
   )";
   out << "      if(" << _ports_in[ARREADY].name << ") begin\n";
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
   out << "      if(" << _ports_in[RVALID].name << " && axi_rready) begin\n";
   out << R"(          if(!first_read) begin
            next_acc_rdata = acc_rdata | ()" +
              _ports_in[RDATA].name + R"( & (~read_mask));
            next_axi_rready = 1'b0;
            next_acc_done = 1'b1;
            `ifdef CACHE
              next_M_address = 'b0;
              next_M_read = 1'b0;
            `endif
            _next_state = S_IDLE;
          end else if()" +
              _ports_in[RLAST].name + R"() begin
            next_acc_rdata = )" +
              _ports_in[RDATA].name + R"( & read_mask;
            `ifdef CACHE
              next_M_address = 'b0;
              next_M_read = 1'b0;
            `endif
)";
   out << "            next_axi_rready = 1'b0;\n";
   out << "            next_acc_done = 1'b1;\n";
   out << "            _next_state = S_IDLE;\n";
   out << "          end else if (first_read) begin\n";
   out << "            next_acc_rdata = " << _ports_in[RDATA].name << " & read_mask;\n";
   out << R"(            `ifdef CACHE
              next_M_address = M_address + (1 << `ifdef _SIM_HAVE_CLOG2 $clog2(in2 / 8) `else `CLOG2(in2 / 8) `endif);
              next_M_read = 1'b1;
              next_axi_rready = 1'b0;
              _next_state = S_STALL_CACHE;
            `else
              _next_state = S_RD_BURST;
            `endif
            next_acc_done = 1'b0;
            next_first_read = 1'b0;
          end
      end else begin
        _next_state = S_RD_BURST;
      end
    end

    S_WR_BURST : begin 
    _next_state = S_WR_BURST;
    `ifdef CACHE
      next_M_address = M_address;
      next_M_write = M_write;
      next_M_wrdata = M_wrdata;
    `endif
)";
   out << "      if(!" << _ports_in[WREADY].name << ") begin\n";
   out << "        next_axi_wvalid = axi_wvalid;\n";
   out << "        next_axi_wlast = axi_wlast;\n";
   out << "        next_axi_wdata = axi_wdata;\n";
   out << "      end\n";
   out << "      if(" << _ports_in[AWREADY].name << ") begin";
   out << R"(
        next_AWSIZE = 'b0;
        next_AWBURST = 'b0;
        next_AWLEN = 'b0;
        next_axi_awvalid = 'b0;
        next_axi_awaddr = 'b0;
        next_AWREADY = 1'b1;
      end
)";

   out << "      if (next_AWREADY &&" << _ports_in[WREADY].name << " && !WSTRB[0]) begin";
   out << R"(
        /* If the last transfer was not aligned and the slave is ready, transfer the rest */
        next_WSTRB = ~WSTRB;
        next_axi_wdata = axi_wdata;
        next_axi_wvalid = 1'b1;
        next_axi_wlast = 1'b1;
        `ifdef CACHE
          next_M_address = M_address + (1 << `ifdef _SIM_HAVE_CLOG2 $clog2(in2 / 8) `else `CLOG2(in2 / 8) `endif);
        `endif
      end
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
   out << "      if (" << _ports_in[BVALID].name << ") begin\n";
   out << R"(        next_acc_done = 1'b1;
        next_axi_wvalid = 1'b0;
        next_axi_wdata = 'b0;
        next_WSTRB = 'b0;
        next_axi_wlast = 1'b0;
        `ifdef CACHE
          next_M_wrdata = 'b0;
          next_M_write = 1'b0;
          next_M_address = 'b0;
        `endif
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
  first_read <= next_first_read;
  read_mask <= next_read_mask;
  AWREADY <= next_AWREADY;
  `ifdef CACHE
    M_address <= next_M_address;
    M_wrdata <= next_M_wrdata;
    M_write <= next_M_write;
    M_read <= next_M_read;
  `endif

  if (!reset) begin
    _present_state <= S_IDLE;
  end
end)";
}