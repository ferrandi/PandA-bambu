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

#include "exceptions.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <cxxabi.h>

void add_escape(std::string& ioString, const std::string& to_be_escaped)
{
   for(std::string::size_type lPos = 0; lPos != ioString.size(); lPos++)
   {
      if(to_be_escaped.find(ioString[lPos]) != std::string::npos)
      {
         char escaped_char[3];
         escaped_char[0] = '\\';
         escaped_char[1] = ioString.at(lPos);
         escaped_char[2] = '\0';
         ioString.replace(lPos, 1, escaped_char);
         lPos++;
      }
   }
}

void remove_escaped(std::string& ioString)
{
   for(std::string::size_type lPos = 0; lPos != ioString.size(); lPos++)
   {
      if(ioString.at(lPos) == '\\')
      {
         if(ioString.at(lPos + 1) == '\\')
         {
            ioString.replace(lPos, 2, "\\");
         }
         else if(ioString.at(lPos + 1) == 'n')
         {
            ioString.replace(lPos, 2, "\n");
         }
         else if(ioString.at(lPos + 1) == 't')
         {
            ioString.replace(lPos, 2, "\t");
         }
      }
   }
}

std::string TrimSpaces(const std::string& value)
{
   std::string temp;
   std::vector<std::string> splitted = SplitString(value, " \n\t\r");
   bool first = true;
   for(auto& i : splitted)
   {
      if(!first and i.size())
         temp += " ";
      if(i.size())
      {
         temp += i;
         first = false;
      }
   }
   return temp;
}
std::string string_demangle(std::string input)
{
   char buf[1024];
   size_t size = 1024;
   int status;
   char* res = abi::__cxa_demangle(input.c_str(), buf, &size, &status);
   return std::string(res);
}
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

std::string convert_fp_to_string(std::string num, unsigned int precision)
{
   union
   {
      unsigned long long ll;
      double d;
      unsigned int i;
      float f;
   } u;
   std::string res;
   char* endptr = nullptr;

   switch(precision)
   {
      case 32:
      {
         if(num == "__Inf")
            u.f = 1.0f / 0.0f;
         else if(num == "-__Inf")
            u.f = -1.0f / 0.0f;
         else if(num == "__Nan")
            u.f = 0.0f / 0.0f;
         else if(num == "-__Nan")
            u.f = -(0.0f / 0.0f);
         else
            u.f = strtof(num.c_str(), &endptr);
         res = "";
         for(unsigned int ind = 0; ind < precision; ind++)
            res = res + (((1U << (precision - ind - 1)) & u.i) ? '1' : '0');
         break;
      }
      case 64:
      {
         if(num == "__Inf")
            u.d = 1.0 / 0.0;
         else if(num == "-__Inf")
            u.d = -1.0 / 0.0;
         else if(num == "__Nan")
            u.d = 0.0 / 0.0;
         else if(num == "-__Nan")
            u.d = -(0.0 / 0.0);
         else
            u.d = strtod(num.c_str(), &endptr);
         res = "";
         for(unsigned int ind = 0; ind < precision; ind++)
            res = res + (((1LLU << (precision - ind - 1)) & u.ll) ? '1' : '0');
         break;
      }
      default:
         throw std::string("not supported precision ") + STR(precision);
   }
   return res;
}

unsigned int ac_type_bitwidth(const std::string& intType, bool& is_signed, bool& is_fixed)
{
   is_fixed = false;
   is_signed = false;
   unsigned int inputBitWidth = 0;
   auto interfaceTypename = intType;
   if(interfaceTypename.find("const ") == 0)
      interfaceTypename = interfaceTypename.substr(std::string("const ").size());
   if(interfaceTypename.find("ac_int<") == 0)
   {
      auto subtypeArg = interfaceTypename.substr(std::string("ac_int<").size());
      auto terminate = subtypeArg.find_first_of(",> ");
      if(subtypeArg.at(terminate) == '>')
         is_signed = true;
      else
      {
         auto signString = subtypeArg.substr(terminate + 2);
         signString = signString.substr(0, signString.find_first_of(",> "));
         if(signString == "true" || signString == "1")
            is_signed = true;
         else
            is_signed = false;
      }
      auto sizeString = subtypeArg.substr(0, terminate);
      inputBitWidth = boost::lexical_cast<unsigned>(sizeString);
   }
   else if(interfaceTypename.find("ac_fixed<") == 0)
   {
      is_fixed = true;
      auto subtypeArg = interfaceTypename.substr(std::string("ac_fixed<").size());
      auto terminate = subtypeArg.find_first_of(",> ");
      auto secondPartType = subtypeArg.substr(terminate + 2);
      auto terminate2 = secondPartType.find_first_of(",> ");
      if(secondPartType.at(terminate2) == '>')
         is_signed = true;
      else
      {
         auto signString = secondPartType.substr(terminate2 + 2);
         signString = signString.substr(0, signString.find_first_of(",> "));
         if(signString == "true" || signString == "1")
            is_signed = true;
         else
            is_signed = false;
      }
      auto sizeString = subtypeArg.substr(0, terminate);
      inputBitWidth = boost::lexical_cast<unsigned>(sizeString);
   }
   else if(interfaceTypename.find("ap_int<") == 0)
   {
      auto subtypeArg = interfaceTypename.substr(std::string("ap_int<").size());
      auto sizeString = subtypeArg.substr(0, subtypeArg.find_first_of(",> "));
      inputBitWidth = boost::lexical_cast<unsigned>(sizeString);
      is_signed = true;
   }
   else if(interfaceTypename.find("ap_uint<") == 0)
   {
      auto subtypeArg = interfaceTypename.substr(std::string("ap_uint<").size());
      auto sizeString = subtypeArg.substr(0, subtypeArg.find_first_of(",> "));
      inputBitWidth = boost::lexical_cast<unsigned>(sizeString);
      is_signed = false;
   }
   else if(interfaceTypename.find("ap_fixed<") == 0)
   {
      is_fixed = true;
      auto subtypeArg = interfaceTypename.substr(std::string("ap_fixed<").size());
      auto sizeString = subtypeArg.substr(0, subtypeArg.find_first_of(",> "));
      inputBitWidth = boost::lexical_cast<unsigned>(sizeString);
      is_signed = true;
   }
   else if(interfaceTypename.find("ap_ufixed<") == 0)
   {
      is_fixed = true;
      auto subtypeArg = interfaceTypename.substr(std::string("ap_ufixed<").size());
      auto sizeString = subtypeArg.substr(0, subtypeArg.find_first_of(",> "));
      inputBitWidth = boost::lexical_cast<unsigned>(sizeString);
      is_signed = false;
   }
   return inputBitWidth;
}
