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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file string_manipulation.hpp
 * @brief Auxiliary methods for manipulating string
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef STRING_MANIPULATION_HPP
#define STRING_MANIPULATION_HPP

#include <boost/lexical_cast.hpp>
#include <string>
#include <type_traits>
#include <vector>

/**
 * Macro which performs a lexical_cast to a string
 */
#ifndef STR
#define STR(s) boost::lexical_cast<std::string>(s)
#endif

/**
 * Function which adds escape to selected characters
 * @param ioString is the string to be escaped
 * @param to_be_escaped is the list of characters to be escaped
 */
void add_escape(std::string& ioString, const std::string& to_be_escaped);

/**
 * Function converting all the escaped characters in the associated character
 * @param ioString is the string where the escaped character are changed
 */
void remove_escaped(std::string& ioString);

/**
 * Return a shell-safe representation of one argument using single-quote style escaping.
 */
std::string shell_escape_argument(const std::string& argument);

/**
 * Join argv elements into a shell-safe command line.
 */
std::string shell_escape_argv(int argc, char* const argv[]);

std::string cxa_demangle(const std::string& input);

std::string cxa_rename_mangled(const std::string& symbol, const std::string& new_symbol);

std::string cxa_prefix_mangled(const std::string& symbol, const std::string& prefix);

std::string cxa_suffix_mangled(const std::string& symbol, const std::string& suffix);

std::string capitalize(const std::string& str);

std::string& capitalize(std::string& str);

inline bool starts_with(const std::string& str, const std::string& pattern)
{
   return str.find(pattern) == 0;
}

inline bool ends_with(const std::string& str, const std::string& pattern)
{
   const auto pos = str.rfind(pattern);
   return pos != std::string::npos && (pos + pattern.size()) == str.size();
}

/**
 * Function with print number in desired format
 * @param number is the number to be printed
 * @param precision is the precision
 * @param size is the size of the string
 */
template <typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
inline std::string NumberToString(const T number, const size_t precision, const size_t size)
{
   std::stringstream return_stream;
   return_stream.width(static_cast<std::streamsize>(size));
   return_stream.fill(' ');
   return_stream.setf(std::ios::fixed, std::ios::floatfield);
   return_stream.precision(static_cast<std::streamsize>(precision));
   return_stream << static_cast<long double>(number);
   return return_stream.str();
}

/**
 * Function with print number in desired format
 * @param number is the number to be printed
 * @param precision is the precision
 */
template <typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
inline std::string NumberToString(const T number, const size_t precision)
{
   std::stringstream return_stream;
   return_stream.setf(std::ios::fixed, std::ios::floatfield);
   return_stream.precision(static_cast<std::streamsize>(precision));
   return_stream << static_cast<long double>(number);
   return return_stream.str();
}

/**
 * Function which print number in binary format
 * @param number is the number to be printed
 * @param precision is the minimum number of digits to be printed
 */
template <typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
inline std::string NumberToBinaryString(const T number, const size_t precision = 0)
{
   std::string ret;
   auto temp_number = number;
   while(temp_number > 0)
   {
      auto bit = temp_number % 2;
      if(bit == 0)
      {
         ret = "0" + ret;
      }
      else
      {
         ret = "1" + ret;
      }
      temp_number = temp_number / 2;
   }
   if(precision > 0)
   {
      while(ret.size() < precision)
      {
         ret = "0" + ret;
      }
   }
   return ret;
}

/**
 * convert a real number stored in a string into a string of bits with a given precision
 */
std::string convert_fp_to_string(std::string num, unsigned long long precision);

/**
 * convert a real number stored in a string into bits with a given precision
 */
unsigned long long convert_fp_to_bits(std::string num, unsigned long long precision);

/**
 * Macro returning the actual type of an object
 */
#define GET_CLASS(obj) cxa_demangle(typeid(obj).name())

/**
 * Convert a string storing a number in decimal format into a string in binary format
 * @param C_value is the decimal format
 * @param precision is the precision of the number
 * @param real_type is true if the type of the number is real
 * @param unsigned_type is true if the type of the number is unsigned
 */
std::string ConvertInBinary(const std::string& C_value, unsigned long long precision, const bool real_type,
                            bool unsigned_type);

std::string FixedPointReinterpret(const std::string& FP_vector, const std::string& fp_typename);

unsigned long long ac_type_bitwidth(const std::string& intType, bool& is_signed, bool& is_fixed);

void replace_all_with_restart(const std::string& val, std::string& mangled, const std::string& old_string,
                              const std::string& new_string);
bool is_unsigned_long_long(const std::string& str);

#endif
