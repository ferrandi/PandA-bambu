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
 *              Copyright (c) 2004-2017 Politecnico di Milano
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
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
*/
#ifndef STRING_MANIPULATION_HPP
#define STRING_MANIPULATION_HPP

///STD include
#include <cxxabi.h>

///STL include
#include <vector>

///Utility include
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>

/**
 * Macro which performs a lexical_cast to a string
 * Temporary is duplicated in utility.hpp and string_manipulation.hpp
 */
#ifndef STR
#define STR(s) boost::lexical_cast<std::string>(s)
#endif

/**
 * Function which adds escape to selected characters
 * @param ioString is the string to be escaped
 * @param to_be_escaped is the list of characters to be escaped
 */
inline
void add_escape(std::string& ioString, const std::string to_be_escaped)
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

/**
 * Function converting all the escaped characters in the associated character
 * @param ioString is the string where the escaped character are changed
 */
inline void remove_escaped(std::string& ioString)
{

   for(std::string::size_type lPos = 0; lPos != ioString.size(); lPos++)
   {
      if(ioString.at(lPos) == '\\')
      {
         if(ioString.at(lPos+1) == '\\')
         {
            ioString.replace(lPos, 2, "\\");
         }
         else if(ioString.at(lPos+1) == 'n')
         {
            ioString.replace(lPos, 2, "\n");
         }
         else if(ioString.at(lPos+1) == 't')
         {
            ioString.replace(lPos, 2, "\t");
         }
      }
   }
}

inline
std::string TrimSpaces(const std::string & value)
{
   std::string temp;
   std::vector<std::string> splitted;
   boost::algorithm::split(splitted, value, boost::algorithm::is_any_of(" \n\t\r"));
   bool first = true;
   for (unsigned int i = 0; i < splitted.size(); i++)
   {
      if (!first and splitted[i].size())
         temp += " ";
      if (splitted[i].size())
      {
         temp += splitted[i];
         first = false;
      }
   }
   return temp;
}

inline
std::string string_demangle(std::string input)
{
   char buf[1024];
   size_t size=1024;
   int status;
   char* res = abi::__cxa_demangle (input.c_str(), buf, &size, &status);
   return std::string(res);
}

/**
 * Function with print number in desired format
 * @param number is the number to be printed
 * @param precision is the precision
 * @param size is the size of the string
 */
template <typename numeric_type>
inline
std::string NumberToString(const numeric_type number, const size_t precision, const size_t size)
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
inline
std::string NumberToString(const numeric_type number, const size_t precision)
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
inline
std::string NumberToBinaryString(const numeric_type number, const size_t precision = 0)
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
inline std::string convert_fp_to_string(std::string num, unsigned int precision)
{
   union
   {
         unsigned long long ll;
         double d;
         unsigned int i;
         float f;
   } u;
   std::string res;
   char *endptr=nullptr;

   switch(precision)
   {
      case 32:
      {
         if(num == "__Inf")
            u.f = 1.0f/0.0f;
         else if(num == "-__Inf")
            u.f = -1.0f/0.0f;
         else if(num == "__Nan")
            u.f = 0.0f/0.0f;
         else
            u.f = strtof(num.c_str(), &endptr);
         res = "";
         for(unsigned int ind = 0; ind < precision; ind++)
            res = res + (((1U << (precision-ind-1)) & u.i) ? '1' : '0');
         break;
      }
      case 64:
      {
         if(num == "__Inf")
            u.d = 1.0/0.0;
         else if(num == "-__Inf")
            u.d = -1.0/0.0;
         else if(num == "__Nan")
            u.d = 0.0/0.0;
         else
            u.d = strtod(num.c_str(), &endptr);
         res = "";
         for(unsigned int ind = 0; ind < precision; ind++)
            res = res + (((1LLU << (precision-ind-1)) & u.ll) ? '1' : '0');
         break;
      }
      default:
         throw std::string("not supported precision ") + STR(precision);
   }
   return res;
}
#endif
