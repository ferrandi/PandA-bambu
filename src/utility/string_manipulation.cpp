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
 *              Copyright (c) 2018-2020 Politecnico di Milano
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
 * @file string_manipulation.cpp
 * @brief Auxiliary methods for manipulating string
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "string_manipulation.hpp"

/// Boost includes
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

/// utility include
#include "exceptions.hpp"

std::string ConvertInBinary(const std::string& C_value, const unsigned int precision, const bool real_type, const bool unsigned_type)
{
   std::string trimmed_value;
   THROW_ASSERT(C_value != "", "Empty string for binary conversion");
   if(real_type)
   {
      trimmed_value = convert_fp_to_string(C_value, precision);
   }
   else
   {
      long long int ll_value;
      if(C_value[0] == '\"')
      {
         trimmed_value = C_value.substr(1);
         trimmed_value = trimmed_value.substr(0, trimmed_value.find('\"'));
         if(trimmed_value[0] == '0' && trimmed_value[1] == 'b')
            trimmed_value = trimmed_value.substr(2);
         else if(trimmed_value[0] == '0' && (trimmed_value[1] == 'x' || trimmed_value[1] == 'o'))
         {
            bool is_hex = trimmed_value[1] == 'x';
            std::string initial_string = trimmed_value.substr(2);
            trimmed_value = "";
            std::string hexTable[16] = {"0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111", "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111"};
            std::string octTable[16] = {"000", "001", "010", "011", "100", "101", "110", "111"};
            for(char curChar : initial_string)
            {
               int off = 0;
               if(is_hex)
               {
                  if(curChar >= '0' && curChar <= '9')
                     off = curChar - '0';
                  else if(curChar >= 'A' && curChar <= 'F')
                     off = curChar - 'A' + 10;
                  else if(curChar >= 'a' && curChar <= 'f')
                     off = curChar - 'a' + 10;
                  else
                     THROW_ERROR("unexpected char in hex string");
               }
               else
               {
                  if(curChar >= '0' && curChar <= '8')
                     off = curChar - '0';
                  else
                     THROW_ERROR("unexpected char in octal string");
               }
               trimmed_value = trimmed_value + (is_hex ? hexTable[off] : octTable[off]);
            }
         }
         else
            THROW_ERROR("unsupported format");

         while(trimmed_value.size() < precision)
            trimmed_value = "0" + trimmed_value;
         while(trimmed_value.size() > precision)
            trimmed_value = trimmed_value.substr(1);
         return trimmed_value;
      }
      else if(C_value[0] == '\'')
      {
         trimmed_value = C_value.substr(1);
         THROW_ASSERT(trimmed_value.find('\'') != std::string::npos, "unxpected case");
         trimmed_value = trimmed_value.substr(0, trimmed_value.find('\''));
         if(trimmed_value[0] == '\\')
            ll_value = boost::lexical_cast<long long int>(trimmed_value.substr(1));
         else
            ll_value = boost::lexical_cast<char>(trimmed_value);
      }
      else if(unsigned_type)
      {
         std::string::size_type sz = 0;
         unsigned long long ull = std::stoull(C_value, &sz, 0);
         ll_value = static_cast<long long int>(ull);
      }
      else
      {
         std::string::size_type sz = 0;
         ll_value = std::stoll(C_value, &sz, 0);
      }
      auto ull_value = static_cast<unsigned long long int>(ll_value);
      trimmed_value = "";
      if(precision <= 64)
      {
         for(unsigned int ind = 0; ind < precision; ind++)
         {
            trimmed_value = trimmed_value + (((1LLU << (precision - ind - 1)) & ull_value) ? '1' : '0');
         }
      }
      else
      {
         for(unsigned int ind = 0; ind < (precision - 64); ind++)
            trimmed_value = trimmed_value + '0';
         for(unsigned int ind = 0; ind < 64; ind++)
            trimmed_value = trimmed_value + (((1LLU << (64 - ind - 1)) & ull_value) ? '1' : '0');
      }
   }
   return trimmed_value;
}

const std::vector<std::string> SplitString(const std::string& input, const std::string& separators)
{
   std::vector<std::string> ret_value;
#ifndef __clang_analyzer__
   boost::algorithm::split(ret_value, input, boost::algorithm::is_any_of(separators));
#endif
   return ret_value;
}
