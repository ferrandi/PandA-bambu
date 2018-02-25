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
 *              Copyright (c) 2018 Politecnico di Milano
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

///Header include
#include "string_manipulation.hpp"

///utility include
#include "exceptions.hpp"

std::string ConvertInBinary(const std::string & C_value, const unsigned int precision, const bool real_type, const bool unsigned_type)
{
   std::string trimmed_value;
   THROW_ASSERT(C_value!= "", "Empty string for binary conversion");
   if (real_type)
   {
      trimmed_value = convert_fp_to_string(C_value, precision);
   }
   else
   {
      long long int ll_value;
      if (C_value[0] == '\'')
      {
         trimmed_value = C_value.substr(1);
         THROW_ASSERT(trimmed_value.find('\'') != std::string::npos, "unxpected case");
         trimmed_value = trimmed_value.substr(0, trimmed_value.find('\''));
         if (trimmed_value[0] == '\\')
            ll_value = boost::lexical_cast<long long int>(trimmed_value.substr(1));
         else
            ll_value = boost::lexical_cast<char>(trimmed_value);
      }
      else if (unsigned_type)
      {
         std::string::size_type sz = 0;
         unsigned long long ull = std::stoull (C_value,&sz,0);
         ll_value = static_cast<long long int>(ull);
      }
      else
      {
         std::string::size_type sz = 0;
         ll_value = std::stoll (C_value,&sz,0);
      }
      unsigned long long int ull_value = static_cast<unsigned long long int>(ll_value);
      trimmed_value = "";
      for (unsigned int ind = 0; ind < precision; ind++)
         trimmed_value = trimmed_value + (((1LLU << (precision-ind-1)) & ull_value) ? '1' : '0');
   }
   return trimmed_value;
}

