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
 *              Copyright (C) 2004-2020 Politecnico di Milano
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
#include <vector>

/**
 * Macro which performs a lexical_cast to a string
 */
#ifndef STR
#define STR(s) boost::lexical_cast<std::string>(s)
#endif

/**
 * Function which splits a string into tokens
 * @param input is the string to be split
 * @param separators is the set of characters to be used to compute the tokens
 * @return the identified tokens
 */
const std::vector<std::string> SplitString(const std::string& input, const std::string& separators);

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

std::string TrimSpaces(const std::string& value);

std::string string_demangle(std::string input);

/**
 * Function with print number in desired format
 * @param number is the number to be printed
 * @param precision is the precision
 * @param size is the size of the string
 */
template <typename numeric_type>
inline std::string NumberToString(const numeric_type number, const size_t precision, const size_t size)
{
   std::stringstream return_stream;
   return_stream.width(static_cast<std::streamsize>(size));
   return_stream.fill(' ');
   return_stream.setf(std::ios::fixed, std::ios::floatfield);
   return_stream.precision(static_cast<std::streamsize>(precision));
   return_stream << boost::lexical_cast<long double>(number);
   return return_stream.str();
}

/**
 * Function with print number in desired format
 * @param number is the number to be printed
 * @param precision is the precision
 */
template <typename numeric_type>
inline std::string NumberToString(const numeric_type number, const size_t precision)
{
   std::stringstream return_stream;
   return_stream.setf(std::ios::fixed, std::ios::floatfield);
   return_stream.precision(static_cast<std::streamsize>(precision));
   return_stream << boost::lexical_cast<long double>(number);
   return return_stream.str();
}

/**
 * Function which print number in binary format
 * @param number is the number to be printed
 * @param precision is the minimum number of digits to be printed
 */
template <typename numeric_type>
inline std::string NumberToBinaryString(const numeric_type number, const size_t precision = 0)
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
 * convert a real number stored in a string into a string o bits with a given precision
 */
std::string convert_fp_to_string(std::string num, unsigned int precision);

/**
 * Macro returning the actual type of an object
 */
#define GET_CLASS(obj) string_demangle(typeid(obj).name())

/**
 * Convert a string storing a number in decimal format into a string in binary format
 * @param C_value is the decimal format
 * @param precision is the precision of the number
 * @param real_type is true if the type of the number is real
 * @param unsigned_type is true if the type of the number is unsigned
 */
std::string ConvertInBinary(const std::string& C_value, const unsigned int precision, const bool real_type, const bool unsigned_type);

unsigned int ac_type_bitwidth(const std::string& intType, bool& is_signed, bool& is_fixed);
#endif
