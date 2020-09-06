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
 *              Copyright (c) 2018-2020 Politecnico di Milano
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
 * @file Write_none_registered_VHDL.cpp
 * @brief Snippet for the Write_none_registered dynamic generator.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
*/
std::cout << "constant ones : std_logic_vector(\\" << _ports_out[1].name << "\\'range) := (others => '1');\n";
std::cout << "constant threezeros : std_logic_vector(2 downto 0) := (others => '0');\n";
std::cout << "begin\n";
std::cout << "process(clock,reset)\n";
std::cout << "  variable \\" << _ports_out[1].name << "_0\\ : std_logic_vector(" << _ports_out[1].type_size << "-1  downto 0);\n";
std::cout << "begin\n";
std::cout << "  if (1RESET_VALUE) then\n";
std::cout << "    \\" << _ports_out[1].name << "\\ <= (others => '0');\n";
std::cout << "  elsif (clock'event and clock='1') then\n";
std::cout << "    if(unsigned("<<_ports_in[2].name<<") /= 0 ) then\n";
std::cout << "      if(PORTSIZE_"<< _ports_in[3].name << " /= 1 ) then\n";
std::cout << "        \\" << _ports_out[1].name << "_0\\ := (others => '0');\n";
std::cout << "        for ii0 in 0 to PORTSIZE_"<< _ports_in[3].name << "-1 loop\n";
std::cout << "          if(unsigned(" << _ports_in[3].name <<"(BITSIZE_" << _ports_in[3].name <<"*(ii0+1)-1 downto BITSIZE_" << _ports_in[3].name <<"*ii0)) >=" << _ports_out[1].type_size << ") then\n";
std::cout << "            \\" << _ports_out[1].name << "_0\\ := std_logic_vector(unsigned(resize(unsigned(" << _ports_in[4].name <<"(BITSIZE_" << _ports_in[4].name <<"*(ii0+1)-1 downto BITSIZE_" << _ports_in[4].name <<"*ii0)), " << _ports_out[1].type_size << ")));\n";
std::cout << "          elsif ((unsigned(" << _ports_in[3].name <<"(BITSIZE_" << _ports_in[3].name <<"*(ii0+1)-1 downto BITSIZE_" << _ports_in[3].name <<"*ii0))+unsigned(" << _ports_in[5].name <<"(BITSIZE_" << _ports_in[5].name <<"*(ii0+1)-1 downto BITSIZE_" << _ports_in[5].name <<"*ii0) & threezeros))>" << _ports_out[1].type_size << ") then \n";
std::cout << "            \\" << _ports_out[1].name << "_0\\ := \\" << _ports_out[1].name << "_0\\ xor (((std_logic_vector(shift_left(unsigned(resize(unsigned(" << _ports_in[4].name <<"(BITSIZE_" << _ports_in[4].name <<"*(ii0+1)-1 downto BITSIZE_" << _ports_in[4].name <<"*ii0)),"<< _ports_out[1].type_size <<")),to_integer(unsigned("<< _ports_in[5].name <<"(BITSIZE_" << _ports_in[5].name <<"*(ii0+1)-1 downto BITSIZE_" << _ports_in[5].name <<"*ii0) & threezeros)))) xor \\" << _ports_out[1].name << "_0\\) and std_logic_vector(shift_left(unsigned(shift_right(unsigned(ones), to_integer(unsigned("<< _ports_in[5].name <<"(BITSIZE_" << _ports_in[5].name <<"*(ii0+1)-1 downto BITSIZE_" << _ports_in[5].name <<"*ii0) & threezeros)))), to_integer(unsigned(" << _ports_in[5].name <<"(BITSIZE_" << _ports_in[5].name <<"*(ii0+1)-1 downto BITSIZE_" << _ports_in[5].name <<"*ii0) & threezeros))))));\n";
std::cout << "          else\n";
std::cout << "            \\" << _ports_out[1].name << "_0\\ := \\" << _ports_out[1].name << "_0\\ xor (((std_logic_vector(shift_left(unsigned(resize(unsigned(" << _ports_in[4].name <<"(BITSIZE_" << _ports_in[4].name <<"*(ii0+1)-1 downto BITSIZE_" << _ports_in[4].name <<"*ii0)),"<< _ports_out[1].type_size <<")),to_integer(unsigned("<< _ports_in[5].name <<"(BITSIZE_" << _ports_in[5].name <<"*(ii0+1)-1 downto BITSIZE_" << _ports_in[5].name <<"*ii0) & threezeros)))) xor \\" << _ports_out[1].name << "_0\\) and ((std_logic_vector(shift_right(unsigned(shift_left(unsigned(shift_left(unsigned(shift_right(unsigned(ones), to_integer(unsigned(" << _ports_in[5].name <<"(BITSIZE_" << _ports_in[5].name <<"*(ii0+1)-1 downto BITSIZE_" << _ports_in[5].name <<"*ii0) & threezeros)))), to_integer(unsigned(" << _ports_in[5].name <<"(BITSIZE_" << _ports_in[5].name <<"*(ii0+1)-1 downto BITSIZE_" << _ports_in[5].name <<"*ii0) & threezeros)))), to_integer(" << _ports_out[1].type_size << "-unsigned(" << _ports_in[3].name <<"(BITSIZE_" << _ports_in[3].name <<"*(ii0+1)-1 downto BITSIZE_" << _ports_in[3].name <<"*ii0))-unsigned(" << _ports_in[5].name <<"(BITSIZE_" << _ports_in[5].name <<"*(ii0+1)-1 downto BITSIZE_" << _ports_in[5].name <<"*ii0) & threezeros)))), to_integer(" << _ports_out[1].type_size << "-unsigned(" << _ports_in[3].name <<"(BITSIZE_" << _ports_in[3].name <<"*(ii0+1)-1 downto BITSIZE_" << _ports_in[3].name <<"*ii0))-unsigned(" << _ports_in[5].name <<"(BITSIZE_" << _ports_in[5].name <<"*(ii0+1)-1 downto BITSIZE_" << _ports_in[5].name <<"*ii0) & threezeros))))))));\n";
std::cout << "          end if;\n";
std::cout << "        end loop;\n";
std::cout << "        \\" << _ports_out[1].name << "\\ <= \\" << _ports_out[1].name << "_0\\;\n";
std::cout << "      else\n";
std::cout << "        \\" << _ports_out[1].name << "\\ <= std_logic_vector(resize(unsigned("<< _ports_in[4].name <<"), "<< _ports_out[1].type_size << "));" <<std::endl;
std::cout << "      end if;\n";
std::cout << "    end if;\n";
std::cout << "  end if;\n";
std::cout << "end process;\n";


std::cout << "process(clock,reset)\n";
std::cout << "begin\n";
std::cout << "  if (1RESET_VALUE) then\n";
std::cout << "    " << _ports_out[0].name << " <= (others => '0');\n";
std::cout << "  elsif (clock'event and clock='1') then\n";
std::cout << "    for ii1 in 0 to PORTSIZE_"<< _ports_in[3].name << "-1 loop\n";
std::cout << "      " << _ports_out[0].name << "(ii1) <= "<< _ports_in[2].name <<"(ii1);" <<std::endl;
std::cout << "    end loop;\n";
std::cout << "  end if;\n";
std::cout << "end process;\n";


