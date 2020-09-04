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
 *              Copyright (c) 2004-2020 Politecnico di Milano
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
 * @file builtin_wait_call_verilog_generator.cpp
 * @brief Snippet for the builtin_wait_call dynamic generator.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
*/

// Signals declarations
if (_np == 3)
std::cout << "reg [0:0] index;\n"
          <<std::endl;
else if(_np > 3)
std::cout << "reg [" << static_cast<size_t>(ceil(log2(_np -2))) << "-1:0] index;\n"
          <<std::endl;
if (_np > 2)
std::cout << "wire [BITSIZE_Mout_addr_ram-1:0] paramAddressRead;\n"
          <<std::endl;
std::cout << "reg [31:0] step 1INIT_ZERO_VALUE;\n"
          << "reg [31:0] next_step;\n"
          << "reg done_port;\n"
             "reg [PORTSIZE_Sout_DataRdy-1:0] Sout_DataRdy;\n"
             "reg [PORTSIZE_Mout_oe_ram-1:0] Mout_oe_ram;\n"
             "reg [PORTSIZE_Mout_we_ram-1:0] Mout_we_ram;\n"
             "reg [PORTSIZE_Mout_addr_ram*BITSIZE_Mout_addr_ram-1:0] Mout_addr_ram;\n"
             "reg [PORTSIZE_Mout_Wdata_ram*BITSIZE_Mout_Wdata_ram-1:0] Mout_Wdata_ram;\n"
             "reg [PORTSIZE_Mout_data_ram_size*BITSIZE_Mout_data_ram_size-1:0] Mout_data_ram_size;\n"
          <<std::endl;
if(_specializing_string != "")
std::cout <<
             "reg [" << _specializing_string << "-1:0] readValue 1INIT_ZERO_VALUE;\n"
             "reg [" << _specializing_string << "-1:0] next_readValue;\n"
          <<std::endl;

if (_np > 2)
  std::cout << "reg [BITSIZE_Mout_addr_ram-1:0] paramAddress [" << _np - 2 << "-1:0];\n"
          <<std::endl;

std::cout << "function [PORTSIZE_S_addr_ram-1:0] check_condition;\n"
             "  input [PORTSIZE_S_addr_ram*BITSIZE_S_addr_ram-1:0] m;\n"
             "  integer i1;\n"
             "begin\n"
             "  for(i1 = 0; i1 < PORTSIZE_S_addr_ram; i1 = i1 + 1)\n"
             "  begin\n"
             "    check_condition[i1] = m[i1*BITSIZE_S_addr_ram +:BITSIZE_S_addr_ram] == unlock_address;\n"
             "  end\n"
             "end\n"
             "endfunction" <<std::endl;

std::cout << "wire [PORTSIZE_S_addr_ram-1:0] internal;"<<std::endl;

unsigned int n_iterations = _specializing_string != "" ? _np + 3 : _np;

std::cout << "parameter [31:0] ";
for (int idx = 0; idx <= n_iterations; ++idx)
   if (idx != n_iterations)
      std::cout << "S_" << idx << " = 32'd" << idx << ",\n";
   else
      std::cout << "S_" << idx << " = 32'd" << idx << ";\n";

if (_np > 2)
std::cout << "initial\n"
             "   begin\n"
             "     $readmemb(MEMORY_INIT_file, paramAddress, 0, "<< _np - 2 << "-1);\n"
             "   end\n\n"
          << std::endl;

if (_np > 2)
{
   std::cout << "assign paramAddressRead = paramAddress[index];" << std::endl;
}
std::cout << "assign Sout_Rdata_ram = Sin_Rdata_ram;" << std::endl;
std::cout << "assign internal = check_condition(S_addr_ram);" << std::endl;

// State machine
std::cout << "always @ (posedge clock 1RESET_EDGE)\n"
             "  if (1RESET_VALUE)\n"
             "  begin\n"
             "    step <= 0;\n";

if(_specializing_string != "")
{
  if (_specializing_string == "1")
    std::cout << "    readValue <= {1'b0};\n";
  else
    std::cout << "    readValue <= {" <<  _specializing_string << " {1'b0}};\n";
  std::cout << "  end else begin\n"
               "    step <= next_step;\n"
               "    readValue <= next_readValue;\n"
               "  end\n"
            << std::endl;
}
else
{
  std::cout << "  end else begin\n"
             "    step <= next_step;\n"
             "  end\n"
          << std::endl;
}

if (_np > 2)
{
   std::cout << "always @(*)\n"
             << "  begin\n"
             << "    index = 0;\n"
             << "    if (step == S_0) begin\n"
             << "        index = 0;\n"
             << "    end\n";
}

int idx = 1;
for (idx = 1; idx <= _np - 3; ++idx)
{
   std::cout << "     else if (step == S_" << idx << ") begin\n"
      << "       index = " << idx - 1 << ";\n"
      << "     end\n";
}

if (_np > 2)
{
   std::cout << "    else if (step == S_" << idx << ") begin\n"
             << "      index = " << idx - 1 << ";\n"
             << "    end\n";
   idx++;
}

idx++;

idx++;

if (_np > 2 && _specializing_string != "")
{
   std::cout << "  else if (step == S_" << idx << ") begin\n"
             << "    index = " << idx - 4 << ";\n"
             << "  end\n";
   idx++;
}
if (_np > 2)
{
   std::cout << "end" << std::endl;
}

std::cout << "always @(*)\n"
          << "  begin\n"
          << "  Sout_DataRdy = Sin_DataRdy;\n"
          << "  done_port = 1'b0;\n"
          << "  next_step = S_0;\n"
          << (_specializing_string != "" ? "  next_readValue = readValue;\n" : "")
          << "  Mout_we_ram = Min_we_ram;\n"
          << "  Mout_Wdata_ram = Min_Wdata_ram;\n"
          << "  Mout_oe_ram = Min_oe_ram;\n"
          << "  Mout_addr_ram = Min_addr_ram;\n"
          << "  Mout_data_ram_size = Min_data_ram_size;\n";

std::cout << "  if (step == S_0) begin\n"
          << "    if (start_port == 1'b1) begin\n";
if (_np == 3)
   std::cout << "      next_step = in2[0] ? S_2 : S_1;\n";
else
   std::cout << "      next_step = S_1;\n";
std::cout << "    end else begin\n"
          << "      next_step = S_0;\n"
          << "    end\n"
          << "  end\n";
idx = 1;
for (idx = 1; idx <= _np - 3; ++idx)
{
   if (idx != _np - 3)
   {
      std::cout << "  else if (step == S_" << idx << ") begin\n"
                << "    Mout_we_ram[0] = 1'b1;\n"
                << "    Mout_addr_ram[BITSIZE_Mout_addr_ram-1:0] = in1 + paramAddressRead;\n"
                << "    Mout_Wdata_ram[BITSIZE_Mout_Wdata_ram-1:0] = " << _p[idx + 1].name << ";\n"
                << "    Mout_data_ram_size[BITSIZE_Mout_data_ram_size-1:0] = "<< _p[idx + 1].type_size << ";\n"
                << "    if (M_DataRdy[0] == 1'b1) begin\n"
                << "      next_step = S_" << idx + 1 << ";\n"
                << "    end else begin\n"
                << "      next_step = S_" << idx << ";\n"
                << "    end\n"
                << "  end\n";
   }
   else
   {
      std::cout << "  else if (step == S_" << idx << ") begin\n"
                << "    Mout_we_ram[0] = 1'b1;\n"
                << "    Mout_addr_ram[BITSIZE_Mout_addr_ram-1:0] = in1 + paramAddressRead;\n"
                << "    Mout_Wdata_ram[BITSIZE_Mout_Wdata_ram-1:0] = " << _p[idx + 1].name << ";\n"
                << "    Mout_data_ram_size[BITSIZE_Mout_data_ram_size-1:0] = "<< _p[idx + 1].type_size << ";\n"
                << "    if (M_DataRdy[0] == 1'b1) begin\n"
                << "      next_step = in2[0] ? S_" << idx + 2 << " : S_" << idx + 1 << ";\n"
                << "    end else begin\n"
                << "      next_step = S_" << idx << ";\n"
                << "    end\n"
                 << "  end\n";
   }
}
if (_np > 2)
{
   std::cout << "  else if (step == S_" << idx << ") begin\n"
             << "     Mout_we_ram[0] = 1'b1;\n"
             << "     Mout_addr_ram[BITSIZE_Mout_addr_ram-1:0] = in1 + paramAddressRead;\n"
             << "     Mout_Wdata_ram[BITSIZE_Mout_Wdata_ram-1:0] = " << _p[idx + 1].name << ";\n"
             << "     Mout_data_ram_size[BITSIZE_Mout_data_ram_size-1:0] = "<< _p[idx + 1].type_size << ";\n"
             << "   if (M_DataRdy[0] == 1'b1) begin\n"
             << "     next_step = S_" << idx + 1 << ";\n"
             << "   end else begin\n"
             << "     next_step = S_" << idx << ";\n"
             << "   end\n"
             << "  end\n";
   idx++;
}

std::cout << "  else if (step == S_" << idx << ") begin\n"
          << "    Mout_we_ram[0] = 1'b1;\n"
          << "    Mout_addr_ram[BITSIZE_Mout_addr_ram-1:0] = in1;\n"
          << "    Mout_Wdata_ram[BITSIZE_Mout_Wdata_ram-1:0] = unlock_address;\n"
          << "    Mout_data_ram_size[BITSIZE_Mout_data_ram_size-1:0] = 32;\n"
          << "    if (M_DataRdy[0] == 1'b1) begin\n"
          << "      next_step = S_" << idx + 1 << ";\n"
          << "    end else begin\n"
          << "      next_step = S_" << idx << ";\n"
          << "    end"
          << "  end\n";
idx++;

std::cout << "  else if (step == S_" << idx << ") begin\n"
          << "    if (|(S_we_ram & internal)) begin\n"
          << "      Sout_DataRdy = (S_we_ram & internal) | Sin_DataRdy;\n"
          << "      next_step = in2[0] ? S_" << (_specializing_string != "" ? idx + 1 : 0) << " : S_0;\n"
          << "      done_port = in2[0] ? 1'b0 : 1'b1;\n"
          << "    end else begin\n";
std::cout << "      next_step = S_" << idx << ";\n";
std::cout << "    end\n"
          << "  end\n";
idx++;

if (_np > 2 && _specializing_string != "")
{
   std::cout << "  else if (step == S_" << idx << ") begin\n"
             << "      Mout_oe_ram[0] = 1'b1;\n"
             << "      Mout_addr_ram[BITSIZE_Mout_addr_ram-1:0] = in1 + paramAddressRead ;\n"
             << "      Mout_data_ram_size[BITSIZE_Mout_data_ram_size-1:0] = " << _specializing_string << ";\n"
             << "    if (M_DataRdy[0] == 1'b1) begin\n"
             << "      next_step = S_" << idx + 1 << ";\n"
             << "      next_readValue = M_Rdata_ram;\n"
             << "    end else begin\n"
             << "      next_step = S_" << idx << ";\n"
             << "    end"
             << "  end\n";
   idx++;

   std::cout << "  else if (step == S_" << idx << ") begin\n"
             << "    Mout_we_ram[0] = 1'b1;\n"
             << "    Mout_addr_ram[BITSIZE_Mout_addr_ram-1:0] = " << _p[_np - 1].name << ";\n"
             << "    Mout_Wdata_ram[BITSIZE_Mout_Wdata_ram-1:0] = readValue;\n"
             << "    Mout_data_ram_size[BITSIZE_Mout_data_ram_size-1:0] = " << _specializing_string << ";\n"
             << "    if (M_DataRdy[0] == 1'b1) begin\n"
             << "      next_step = S_0;\n"
             << "      done_port = 1'b1;\n"
             << "    end else begin\n"
             << "      next_step = S_" << idx << ";\n"
             << "    end"
             << "  end\n";
}
std::cout << "end" << std::endl;
