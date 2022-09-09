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
 * @file Write_none_registeredModuleGenerator.cpp
 * @brief
 *
 *
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "Write_none_registeredModuleGenerator.hpp"

#include "language_writer.hpp"

Write_none_registeredModuleGenerator::Write_none_registeredModuleGenerator(const HLS_managerRef& _HLSMgr)
    : Registrar(_HLSMgr)
{
}

void Write_none_registeredModuleGenerator::InternalExec(
    std::ostream& out, const module* /* mod */, unsigned int /* function_id */, vertex /* op_v */,
    const HDLWriter_Language language, const std::vector<ModuleGenerator::parameter>& /* _p */,
    const std::vector<ModuleGenerator::parameter>& _ports_in, const std::vector<ModuleGenerator::parameter>& _ports_out,
    const std::vector<ModuleGenerator::parameter>& /* _ports_inout */)
{
   if(language == HDLWriter_Language::VHDL)
   {
      out << "constant ones : std_logic_vector(\\" << _ports_out[1].name << "\\'range) := (others => '1');\n";
      out << "constant threezeros : std_logic_vector(2 downto 0) := (others => '0');\n";
      out << "begin\n";
      out << "process(clock,reset)\n";
      out << "  variable \\" << _ports_out[1].name << "_0\\ : std_logic_vector(" << _ports_out[1].type_size
          << "-1  downto 0);\n";
      out << "begin\n";
      out << "  if (1RESET_VALUE) then\n";
      out << "    \\" << _ports_out[1].name << "\\ <= (others => '0');\n";
      out << "  elsif (clock'event and clock='1') then\n";
      out << "    if(unsigned(" << _ports_in[2].name << ") /= 0 ) then\n";
      out << "      if(PORTSIZE_" << _ports_in[3].name << " /= 1 ) then\n";
      out << "        \\" << _ports_out[1].name << "_0\\ := (others => '0');\n";
      out << "        for ii0 in 0 to PORTSIZE_" << _ports_in[3].name << "-1 loop\n";
      out << "          if(unsigned(" << _ports_in[3].name << "(BITSIZE_" << _ports_in[3].name
          << "*(ii0+1)-1 downto BITSIZE_" << _ports_in[3].name << "*ii0)) >=" << _ports_out[1].type_size << ") then\n";
      out << "            \\" << _ports_out[1].name << "_0\\ := std_logic_vector(unsigned(resize(unsigned("
          << _ports_in[4].name << "(BITSIZE_" << _ports_in[4].name << "*(ii0+1)-1 downto BITSIZE_" << _ports_in[4].name
          << "*ii0)), " << _ports_out[1].type_size << ")));\n";
      out << "          elsif ((unsigned(" << _ports_in[3].name << "(BITSIZE_" << _ports_in[3].name
          << "*(ii0+1)-1 downto BITSIZE_" << _ports_in[3].name << "*ii0))+unsigned(" << _ports_in[5].name << "(BITSIZE_"
          << _ports_in[5].name << "*(ii0+1)-1 downto BITSIZE_" << _ports_in[5].name << "*ii0) & threezeros))>"
          << _ports_out[1].type_size << ") then \n";
      out << "            \\" << _ports_out[1].name << "_0\\ := \\" << _ports_out[1].name
          << "_0\\ xor (((std_logic_vector(shift_left(unsigned(resize(unsigned(" << _ports_in[4].name << "(BITSIZE_"
          << _ports_in[4].name << "*(ii0+1)-1 downto BITSIZE_" << _ports_in[4].name << "*ii0)),"
          << _ports_out[1].type_size << ")),to_integer(unsigned(" << _ports_in[5].name << "(BITSIZE_"
          << _ports_in[5].name << "*(ii0+1)-1 downto BITSIZE_" << _ports_in[5].name << "*ii0) & threezeros)))) xor \\"
          << _ports_out[1].name
          << "_0\\) and std_logic_vector(shift_left(unsigned(shift_right(unsigned(ones), to_integer(unsigned("
          << _ports_in[5].name << "(BITSIZE_" << _ports_in[5].name << "*(ii0+1)-1 downto BITSIZE_" << _ports_in[5].name
          << "*ii0) & threezeros)))), to_integer(unsigned(" << _ports_in[5].name << "(BITSIZE_" << _ports_in[5].name
          << "*(ii0+1)-1 downto BITSIZE_" << _ports_in[5].name << "*ii0) & threezeros))))));\n";
      out << "          else\n";
      out << "            \\" << _ports_out[1].name << "_0\\ := \\" << _ports_out[1].name
          << "_0\\ xor (((std_logic_vector(shift_left(unsigned(resize(unsigned(" << _ports_in[4].name << "(BITSIZE_"
          << _ports_in[4].name << "*(ii0+1)-1 downto BITSIZE_" << _ports_in[4].name << "*ii0)),"
          << _ports_out[1].type_size << ")),to_integer(unsigned(" << _ports_in[5].name << "(BITSIZE_"
          << _ports_in[5].name << "*(ii0+1)-1 downto BITSIZE_" << _ports_in[5].name << "*ii0) & threezeros)))) xor \\"
          << _ports_out[1].name
          << "_0\\) and "
             "((std_logic_vector(shift_right(unsigned(shift_left(unsigned(shift_left(unsigned(shift_right(unsigned("
             "ones), to_integer(unsigned("
          << _ports_in[5].name << "(BITSIZE_" << _ports_in[5].name << "*(ii0+1)-1 downto BITSIZE_" << _ports_in[5].name
          << "*ii0) & threezeros)))), to_integer(unsigned(" << _ports_in[5].name << "(BITSIZE_" << _ports_in[5].name
          << "*(ii0+1)-1 downto BITSIZE_" << _ports_in[5].name << "*ii0) & threezeros)))), to_integer("
          << _ports_out[1].type_size << "-unsigned(" << _ports_in[3].name << "(BITSIZE_" << _ports_in[3].name
          << "*(ii0+1)-1 downto BITSIZE_" << _ports_in[3].name << "*ii0))-unsigned(" << _ports_in[5].name << "(BITSIZE_"
          << _ports_in[5].name << "*(ii0+1)-1 downto BITSIZE_" << _ports_in[5].name
          << "*ii0) & threezeros)))), to_integer(" << _ports_out[1].type_size << "-unsigned(" << _ports_in[3].name
          << "(BITSIZE_" << _ports_in[3].name << "*(ii0+1)-1 downto BITSIZE_" << _ports_in[3].name << "*ii0))-unsigned("
          << _ports_in[5].name << "(BITSIZE_" << _ports_in[5].name << "*(ii0+1)-1 downto BITSIZE_" << _ports_in[5].name
          << "*ii0) & threezeros))))))));\n";
      out << "          end if;\n";
      out << "        end loop;\n";
      out << "        \\" << _ports_out[1].name << "\\ <= \\" << _ports_out[1].name << "_0\\;\n";
      out << "      else\n";
      out << "        \\" << _ports_out[1].name << "\\ <= std_logic_vector(resize(unsigned(" << _ports_in[4].name
          << "), " << _ports_out[1].type_size << "));\n";
      out << "      end if;\n";
      out << "    end if;\n";
      out << "  end if;\n";
      out << "end process;\n";

      out << "process(clock,reset)\n";
      out << "begin\n";
      out << "  if (1RESET_VALUE) then\n";
      out << "    " << _ports_out[0].name << " <= (others => '0');\n";
      out << "  elsif (clock'event and clock='1') then\n";
      out << "    for ii1 in 0 to PORTSIZE_" << _ports_in[3].name << "-1 loop\n";
      out << "      " << _ports_out[0].name << "(ii1) <= " << _ports_in[2].name << "(ii1);\n";
      out << "    end loop;\n";
      out << "  end if;\n";
      out << "end process;\n";
   }
   else
   {
      out << "integer ii=0;\n";
      out << "reg [" << _ports_out[1].type_size << "-1:0] " << _ports_out[1].name << ";\n";
      out << "reg [" << _ports_out[1].type_size << "-1:0] " << _ports_out[1].name << "_0;\n";
      out << "reg [PORTSIZE_" << _ports_in[3].name << "-1:0] " << _ports_out[0].name << " 1INIT_ZERO_VALUE;\n";

      out << "always @(posedge clock 1RESET_EDGE)\n";
      out << "begin\n";
      out << "  if (1RESET_VALUE)\n";
      out << "    " << _ports_out[1].name << " <= 0;\n";
      out << "  else if(|" << _ports_in[2].name << ")\n";
      out << "    " << _ports_out[1].name << " <= " << _ports_out[1].name << "_0;\n";
      out << "end\n";

      out << "always @(*)\n";
      out << "begin\n";
      out << "  " << _ports_out[1].name << "_0 = 0;\n";
      out << "  for(ii=0; ii<PORTSIZE_" << _ports_in[3].name << "; ii=ii+1)\n";
      out << "  begin\n";
      out << "    " << _ports_out[1].name << "_0 = (" << _ports_in[3].name << "[(BITSIZE_" << _ports_in[3].name
          << ")*ii+:BITSIZE_" << _ports_in[3].name << "]>=" << _ports_out[1].type_size << ")?" << _ports_in[4].name
          << "[(BITSIZE_" << _ports_in[4].name << ")*ii+:BITSIZE_" << _ports_in[4].name << "]:(" << _ports_out[1].name
          << "_0^((((BITSIZE_" << _ports_in[4].name << ">=" << _ports_out[1].type_size << "?" << _ports_in[4].name
          << "[(BITSIZE_" << _ports_in[4].name << ")*ii+:BITSIZE_" << _ports_in[4].name << "]:{{("
          << _ports_out[1].type_size << "<BITSIZE_" << _ports_in[4].name << " ? 1 : " << _ports_out[1].type_size
          << "-BITSIZE_" << _ports_in[4].name << "){1'b0}}," << _ports_in[4].name << "[(BITSIZE_" << _ports_in[4].name
          << ")*ii+:BITSIZE_" << _ports_in[4].name << "]})<<" << _ports_in[5].name << "[(BITSIZE_" << _ports_in[5].name
          << ")*ii+:BITSIZE_" << _ports_in[5].name << "]*8)^" << _ports_out[1].name << "_0) & (((" << _ports_in[3].name
          << "[(BITSIZE_" << _ports_in[3].name << ")*ii+:BITSIZE_" << _ports_in[3].name << "]+" << _ports_in[5].name
          << "[(BITSIZE_" << _ports_in[5].name << ")*ii+:BITSIZE_" << _ports_in[5].name << "]*8)>"
          << _ports_out[1].type_size << ") ? ((({(" << _ports_out[1].type_size << "){1'b1}})>>(" << _ports_in[5].name
          << "[(BITSIZE_" << _ports_in[5].name << ")*ii+:BITSIZE_" << _ports_in[5].name << "]*8))<<("
          << _ports_in[5].name << "[(BITSIZE_" << _ports_in[5].name << ")*ii+:BITSIZE_" << _ports_in[5].name
          << "]*8)) : ((((({(" << _ports_out[1].type_size << "){1'b1}})>>(" << _ports_in[5].name << "[(BITSIZE_"
          << _ports_in[5].name << ")*ii+:BITSIZE_" << _ports_in[5].name << "]*8))<<(" << _ports_in[5].name
          << "[(BITSIZE_" << _ports_in[5].name << ")*ii+:BITSIZE_" << _ports_in[5].name << "]*8))<<("
          << _ports_out[1].type_size << "-" << _ports_in[3].name << "[(BITSIZE_" << _ports_in[3].name
          << ")*ii+:BITSIZE_" << _ports_in[3].name << "]-" << _ports_in[5].name << "[(BITSIZE_" << _ports_in[5].name
          << ")*ii+:BITSIZE_" << _ports_in[5].name << "]*8))>>(" << _ports_out[1].type_size << "-" << _ports_in[3].name
          << "[(BITSIZE_" << _ports_in[3].name << ")*ii+:BITSIZE_" << _ports_in[3].name << "]-" << _ports_in[5].name
          << "[(BITSIZE_" << _ports_in[5].name << ")*ii+:BITSIZE_" << _ports_in[5].name << "]*8)))));\n";
      out << "  end\n";
      out << "end\n";

      out << "always @(posedge clock 1RESET_EDGE)\n";
      out << "begin\n";
      out << "  if (1RESET_VALUE)\n";
      out << "    " << _ports_out[0].name << " <= 0;\n";
      out << "  else\n";
      out << "    for(ii=0; ii<PORTSIZE_" << _ports_in[3].name << "; ii=ii+1)\n";
      out << "    begin\n";
      out << "      " << _ports_out[0].name << "[ii] <= " << _ports_in[2].name << "[ii];\n";
      out << "    end\n";
      out << "end\n";
   }
}