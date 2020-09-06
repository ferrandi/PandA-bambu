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
 *              Copyright (c) 2016-2020 Politecnico di Milano
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
 * @file asm_vhdl_generator.cpp
 * @brief Snippet for the asm dynamimc generator.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
*/
//void fun()
//{
   __replaceStringInPlace(_specializing_string, "%%", "&percent;");
   __replaceStringInPlace(_specializing_string, "&percent;", "%");
   __replaceStringInPlace(_specializing_string, "\n", "\\n");
   ///remove possible dialects
   std::string res_asm;
   res_asm = "begin\n";
   bool open_curl = false;
   unsigned int count_pipes= 0;
   char prec_char=0;
   for(unsigned int i=0; i < _specializing_string.size(); ++i)
   {
      char current_char = _specializing_string[i];
      if(current_char == '{' && prec_char != '%')
         open_curl = true;
      else if(current_char == '}' && prec_char != '%')
      {
         open_curl = false;
         count_pipes = 0;
      }
      else if(open_curl && current_char == '|' && prec_char != '%')
         ++count_pipes;
      else if(open_curl && count_pipes!=3)
         ;
      else if(open_curl && count_pipes==3)
         res_asm = res_asm + current_char;
      else
         res_asm = res_asm + current_char;
      prec_char = current_char;
   }
   if(_specializing_string.size()==0)
      res_asm = "assign done_port = start_port;";
   std::cout << res_asm;
