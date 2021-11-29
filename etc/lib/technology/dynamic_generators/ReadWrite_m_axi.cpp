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
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

/*
// IN
  input clock;
  input reset;
  input [0:0] start_port;
  input [(PORTSIZE_in1*BITSIZE_in1)+(-1):0] in1;
  input [(PORTSIZE_in2*BITSIZE_in2)+(-1):0] in2;
  input [(PORTSIZE_in3*BITSIZE_in3)+(-1):0] in3;
  input [31:0] in4;
  input _m_axi_common_AWREADY;
  input _m_axi_common_WREADY;
  input _m_axi_common_BID;
  input [1:0] _m_axi_common_BRESP;
  input _m_axi_common_BUSER;
  input _m_axi_common_BVALID;
  input _m_axi_common_ARREADY;
  input _m_axi_common_RID;
  input [15:0] _m_axi_common_RDATA;
  input [1:0] _m_axi_common_RRESP;
  input _m_axi_common_RLAST;
  input _m_axi_common_RUSER;
  input _m_axi_common_RVALID;
  input [31:0] _a;
  input [31:0] _n_ptr;
  // OUT
  output [0:0] done_port;
  output [(PORTSIZE_out1*BITSIZE_out1)+(-1):0] out1;
  output _m_axi_common_AWID;
  output [31:0] _m_axi_common_AWADDR;
  output [7:0] _m_axi_common_AWLEN;
  output [2:0] _m_axi_common_AWSIZE;
  output [1:0] _m_axi_common_AWBURST;
  output [1:0] _m_axi_common_AWLOCK;
  output [3:0] _m_axi_common_AWCACHE;
  output [2:0] _m_axi_common_AWPROT;
  output [3:0] _m_axi_common_AWQOS;
  output [3:0] _m_axi_common_AWREGION;
  output _m_axi_common_AWUSER;
  output _m_axi_common_AWVALID;
  output _m_axi_common_WID;
  output [15:0] _m_axi_common_WDATA;
  output [1:0] _m_axi_common_WSTRB;
  output _m_axi_common_WLAST;
  output _m_axi_common_WUSER;
  output _m_axi_common_WVALID;
  output _m_axi_common_BREADY;
  output _m_axi_common_ARID;
  output [31:0] _m_axi_common_ARADDR;
  output [7:0] _m_axi_common_ARLEN;
  output [2:0] _m_axi_common_ARSIZE;
  output [1:0] _m_axi_common_ARBURST;
  output [1:0] _m_axi_common_ARLOCK;
  output [3:0] _m_axi_common_ARCACHE;
  output [2:0] _m_axi_common_ARPROT;
  output [3:0] _m_axi_common_ARQOS;
  output [3:0] _m_axi_common_ARREGION;
  output _m_axi_common_ARUSER;
  output _m_axi_common_ARVALID;
  output _m_axi_common_RREADY;
*/

const bool isAlignedPowerOfTwo = _ports_out[3].alignment == RUPNP2(_ports_out[3].alignment);
std::cout << "//" << (isAlignedPowerOfTwo ? "T" : "F") << "\n";
std::cout << "integer ii=0;\n";
std::cout << "reg [BITSIZE_" << _ports_out[1].name << "-1:0] " << _ports_out[1].name << ";\n";

const unsigned int log2nbyte =
    _ports_out[3].alignment == 1 ? 0 : (32u - static_cast<unsigned>(__builtin_clz(_ports_out[3].alignment - 1)));

const unsigned int addressMaxValue =
    _ports_out[3].alignment * static_cast<unsigned>(atoi(_specializing_string.data())) - 1;
const unsigned int nbitAddress =
    addressMaxValue == 1 ? 1 : (32u - static_cast<unsigned>(__builtin_clz(addressMaxValue)));

std::cout << "// TODO: add BRAM to AXI4 bridge implementation here\n";
