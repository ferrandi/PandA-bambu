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
 *              Copyright (c) 2018-2024 Politecnico di Milano
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
#include "panda_types.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <cxxabi.h>
#include <regex>

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

std::string cxa_demangle(const std::string& input)
{
   int status;
   std::unique_ptr<char, void (*)(void*)> res(abi::__cxa_demangle(input.data(), nullptr, nullptr, &status), std::free);
   return status == 0 ? std::string(res.get()) : "";
}

std::string cxa_rename_mangled(const std::string& signature, const std::string& new_fname)
{
   auto z_pos = signature.find('Z');
   if(z_pos != std::string::npos)
   {
      const char* z_start = signature.data() + z_pos + 1;
      char* z_end;
      auto z_len = std::strtoul(z_start, &z_end, 10);
      if(z_start != z_end)
      {
         return signature.substr(0, z_pos + 1) + std::to_string(new_fname.size()) + new_fname +
                signature.substr(static_cast<size_t>(std::distance(signature.data(), static_cast<const char*>(z_end))) +
                                 z_len);
      }
   }
   return new_fname;
}

std::string cxa_prefix_mangled(const std::string& signature, const std::string& prefix)
{
   auto z_pos = signature.find('Z');
   if(z_pos != std::string::npos)
   {
      const char* z_start = signature.data() + z_pos + 1;
      char* z_end;
      auto z_len = std::strtoul(z_start, &z_end, 10);
      if(z_start != z_end)
      {
         return signature.substr(0, z_pos + 1) + std::to_string(prefix.size() + z_len) + prefix +
                signature.substr(static_cast<size_t>(std::distance(signature.data(), static_cast<const char*>(z_end))));
      }
   }
   return prefix + signature;
}

std::string capitalize(const std::string& str)
{
   std::string s(str);
   return capitalize(s);
}

std::string& capitalize(std::string& str)
{
   str[0] = static_cast<char>(toupper(str[0]));
   return str;
}

static const std::regex fixed_def("a[cp]_(u)?fixed<\\s*(\\d+)\\s*,\\s*(\\d+),?\\s*(\\w+)?[^>]*>[^\\d-]*");
#define FD_GROUP_U 1
#define FD_GROUP_W 2
#define FD_GROUP_D 3
#define FD_GROUP_SIGN 4

std::string ConvertInBinary(const std::string& C_value, unsigned long long precision, const bool real_type,
                            bool unsigned_type)
{
   std::string trimmed_value = C_value;
   THROW_ASSERT(C_value != "", "Empty string for binary conversion");

   bool is_signed, is_fixed;
   const auto ac_bw = ac_type_bitwidth(C_value, is_signed, is_fixed);
   if(ac_bw)
   {
      unsigned_type = !is_signed;
      trimmed_value = trimmed_value.substr(trimmed_value.find('>') + 1);
   }

   if(real_type)
   {
      trimmed_value = convert_fp_to_string(C_value, precision);
   }
   else if(is_fixed)
   {
      std::cmatch what;
#if HAVE_ASSERTS
      const auto is_match =
#endif
          std::regex_search(C_value.c_str(), what, fixed_def);
      THROW_ASSERT(is_match, "");
      const auto w = std::stoul(
          std::string(what[FD_GROUP_W].first, static_cast<size_t>(what[FD_GROUP_W].second - what[FD_GROUP_W].first)));
      const auto d = std::stoul(
          std::string(what[FD_GROUP_D].first, static_cast<size_t>(what[FD_GROUP_D].second - what[FD_GROUP_D].first)));
      is_signed = (what[FD_GROUP_U].second - what[FD_GROUP_U].first) == 0 &&
                  ((what[FD_GROUP_SIGN].second - what[FD_GROUP_SIGN].first) == 0 ||
                   strncmp(what[FD_GROUP_SIGN].first, "true", 4) == 0);
      THROW_ASSERT(d < w, "Decimal part should be smaller then total length");
      const long double val = strtold(what[0].second, nullptr) * powl(2, w - d);
      // TODO: update regex to handle overflow correctly
      auto fixp = integer_cst_t(val);
      is_signed &= val < 0;
      trimmed_value.clear();
      while(trimmed_value.size() < w)
      {
         trimmed_value = ((fixp & 1) ? "1" : "0") + trimmed_value;
         fixp >>= 1;
      }
      while(trimmed_value.size() < precision)
      {
         trimmed_value = (is_signed ? trimmed_value.front() : '0') + trimmed_value;
      }
   }
   else
   {
      long long int ll_value;
      if(trimmed_value[0] == '\"')
      {
         trimmed_value = trimmed_value.substr(1);
         trimmed_value = trimmed_value.substr(0, trimmed_value.find('\"'));
         if(trimmed_value[0] == '0' && trimmed_value[1] == 'b')
         {
            trimmed_value = trimmed_value.substr(2);
         }
         else if(trimmed_value[0] == '0' && (trimmed_value[1] == 'x' || trimmed_value[1] == 'o'))
         {
            bool is_hex = trimmed_value[1] == 'x';
            std::string initial_string = trimmed_value.substr(2);
            trimmed_value = "";
            std::string hexTable[16] = {"0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111",
                                        "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111"};
            std::string octTable[16] = {"000", "001", "010", "011", "100", "101", "110", "111"};
            for(char curChar : initial_string)
            {
               int off = 0;
               if(is_hex)
               {
                  if(curChar >= '0' && curChar <= '9')
                  {
                     off = curChar - '0';
                  }
                  else if(curChar >= 'A' && curChar <= 'F')
                  {
                     off = curChar - 'A' + 10;
                  }
                  else if(curChar >= 'a' && curChar <= 'f')
                  {
                     off = curChar - 'a' + 10;
                  }
                  else
                  {
                     THROW_ERROR("unexpected char in hex string");
                  }
               }
               else
               {
                  if(curChar >= '0' && curChar <= '8')
                  {
                     off = curChar - '0';
                  }
                  else
                  {
                     THROW_ERROR("unexpected char in octal string");
                  }
               }
               trimmed_value = trimmed_value + (is_hex ? hexTable[off] : octTable[off]);
            }
         }
         else
         {
            THROW_ERROR("unsupported format");
         }

         while(trimmed_value.size() < precision)
         {
            trimmed_value = "0" + trimmed_value;
         }
         while(trimmed_value.size() > precision)
         {
            trimmed_value = trimmed_value.substr(1);
         }
         return trimmed_value;
      }
      else if(trimmed_value[0] == '\'')
      {
         trimmed_value = trimmed_value.substr(1);
         THROW_ASSERT(trimmed_value.find('\'') != std::string::npos, "unxpected case");
         trimmed_value = trimmed_value.substr(0, trimmed_value.find('\''));
         if(trimmed_value[0] == '\\')
         {
            ll_value = std::stoll(trimmed_value.substr(1));
         }
         else
         {
            ll_value = boost::lexical_cast<char>(trimmed_value);
         }
      }
      else if(unsigned_type)
      {
         std::string::size_type sz = 0;
         unsigned long long ull = std::stoull(trimmed_value, &sz, 0);
         ll_value = static_cast<long long int>(ull);
      }
      else
      {
         std::string::size_type sz = 0;
         ll_value = std::stoll(trimmed_value, &sz, 0);
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
         {
            trimmed_value = trimmed_value + '0';
         }
         for(unsigned int ind = 0; ind < 64; ind++)
         {
            trimmed_value = trimmed_value + (((1LLU << (64 - ind - 1)) & ull_value) ? '1' : '0');
         }
      }
   }
   return trimmed_value;
}

static const std::regex fixp_val("(\\d+\\.?\\d*)");

std::string FixedPointReinterpret(const std::string& FP_vector, const std::string& fp_typename)
{
   std::cmatch what;
   if(std::regex_search(fp_typename.c_str(), what, fixed_def))
   {
      const auto w = std::stoul(
          std::string(what[FD_GROUP_W].first, static_cast<size_t>(what[FD_GROUP_W].second - what[FD_GROUP_W].first)));
      const auto d = std::stoul(
          std::string(what[FD_GROUP_D].first, static_cast<size_t>(what[FD_GROUP_D].second - what[FD_GROUP_D].first)));
      THROW_ASSERT(d < w, "Decimal part should be smaller then total length");
      std::sregex_token_iterator fix_val_it(FP_vector.begin(), FP_vector.end(), fixp_val), end;
      std::string new_vector = "{";
      while(fix_val_it != end)
      {
         const long double val = strtold(fix_val_it->str().c_str(), nullptr) * powl(2, w - d);
         // TODO: update regex to handle overflow correctly
         const auto fixp = static_cast<long long>(val);
         new_vector += "{{{" + STR(fixp) + "}}}, ";
         ++fix_val_it;
      }
      new_vector.erase(new_vector.size() - 2, 2);
      new_vector += "}";
      return new_vector;
   }
   return FP_vector;
}

std::string convert_fp_to_string(std::string num, unsigned long long precision)
{
   union
   {
      unsigned long long ll;
      double d;
      unsigned int i;
      float f;
   } u = {};
   std::string res;
   char* endptr = nullptr;

   switch(precision)
   {
      case 32:
      {
         if(num == "__Inf")
         {
            u.f = 1.0f / 0.0f;
         }
         else if(num == "-__Inf")
         {
            u.f = -1.0f / 0.0f;
         }
         else if(num == "__Nan")
         {
            u.f = 0.0f / 0.0f;
         }
         else if(num == "-__Nan")
         {
            u.f = -(0.0f / 0.0f);
         }
         else
         {
            u.f = strtof(num.c_str(), &endptr);
         }
         res = "";
         for(unsigned int ind = 0; ind < precision; ind++)
         {
            res = res + (((1U << (precision - ind - 1)) & u.i) ? '1' : '0');
         }
         break;
      }
      case 64:
      {
         if(num == "__Inf")
         {
            u.d = 1.0 / 0.0;
         }
         else if(num == "-__Inf")
         {
            u.d = -1.0 / 0.0;
         }
         else if(num == "__Nan")
         {
            u.d = 0.0 / 0.0;
         }
         else if(num == "-__Nan")
         {
            u.d = -(0.0 / 0.0);
         }
         else
         {
            u.d = strtod(num.c_str(), &endptr);
         }
         res = "";
         for(unsigned int ind = 0; ind < precision; ind++)
         {
            res = res + (((1LLU << (precision - ind - 1)) & u.ll) ? '1' : '0');
         }
         break;
      }
      default:
         throw std::string("not supported precision ") + STR(precision);
   }
   return res;
}

unsigned long long convert_fp_to_bits(std::string num, unsigned long long precision)
{
   union
   {
      unsigned long long ll;
      double d;
      unsigned int i;
      float f;
   } u;
   char* endptr = nullptr;

   switch(precision)
   {
      case 32:
      {
         if(num == "__Inf")
         {
            u.f = 1.0f / 0.0f;
         }
         else if(num == "-__Inf")
         {
            u.f = -1.0f / 0.0f;
         }
         else if(num == "__Nan")
         {
            u.f = 0.0f / 0.0f;
         }
         else if(num == "-__Nan")
         {
            u.f = -(0.0f / 0.0f);
         }
         else
         {
            u.f = strtof(num.c_str(), &endptr);
         }
         return u.i;
      }
      case 64:
      {
         if(num == "__Inf")
         {
            u.d = 1.0 / 0.0;
         }
         else if(num == "-__Inf")
         {
            u.d = -1.0 / 0.0;
         }
         else if(num == "__Nan")
         {
            u.d = 0.0 / 0.0;
         }
         else if(num == "-__Nan")
         {
            u.d = -(0.0 / 0.0);
         }
         else
         {
            u.d = strtod(num.c_str(), &endptr);
         }
         return u.ll;
      }
      default:
         throw std::string("not supported precision ") + STR(precision);
   }
   return 0;
}

static const std::regex ac_type_def("a[cp]_(u)?(\\w+)<\\s*(\\d+)\\s*,?\\s*(\\d+)?,?\\s*(\\w+)?[^>]*>");
#define AC_GROUP_U 1
#define AC_GROUP_T 2
#define AC_GROUP_W 3
#define AC_GROUP_SIGN 4

unsigned long long ac_type_bitwidth(const std::string& intType, bool& is_signed, bool& is_fixed)
{
   std::cmatch what;
   is_signed = false;
   is_fixed = false;
   if(std::regex_search(intType.c_str(), what, ac_type_def))
   {
      auto w = std::stoull(
          std::string(what[AC_GROUP_W].first, static_cast<size_t>(what[AC_GROUP_W].second - what[AC_GROUP_W].first)));
      is_signed = (what[AC_GROUP_U].second - what[AC_GROUP_U].first) == 0 &&
                  ((what[AC_GROUP_SIGN].second - what[AC_GROUP_SIGN].first) == 0 ||
                   strncmp(what[AC_GROUP_SIGN].first, "true", 4) == 0);
      is_fixed =
          std::string(what[AC_GROUP_T].first, static_cast<size_t>(what[AC_GROUP_T].second - what[AC_GROUP_T].first))
              .find("fixed") != std::string::npos;
      return w;
   }
   return 0;
}
