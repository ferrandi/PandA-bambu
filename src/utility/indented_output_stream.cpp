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
 * @file indented_output_stream.hpp
 * @brief Class to print indented code
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision: 10608 $
 * $Date: 2012-03-12 10:46:42 +0100 (Mon, 12 Mar 2012) $
 * Last modified by $Author: ferrandi $
 *
 */

#include <cstddef>
#include <fstream>
#include <iostream>

/// Utility include
#include "exceptions.hpp"
/// Header include
#include "indented_output_stream.hpp"

/// In global_variables.hpp
extern size_t indentation;

IndentedOutputStream::IndentedOutputStream(char o, char c, unsigned int d) : indent_spaces(0), opening_char(o), closing_char(c), delta(d), is_line_start(true)
{
}

IndentedOutputStream::~IndentedOutputStream()
{
#ifndef NDEBUG
   /// This corresponds to a THROW_ASSERT, however it is against C++ rules throw an errror inside a destructor
   if(indent_spaces != 0)
   {
      std::cerr << "Not all indentations have been closed: " << indentation << std::endl;
   }
#endif
}

void IndentedOutputStream::Append(const std::string& str)
{
   std::string::const_iterator it_end = str.end();
   /// Specified whether the first character of the string
   /// we are going to write must be indented or not
   bool needToInd = false;

   if(*(str.begin()) == closing_char)
   {
#if 0
      if(delta > indent_spaces)
      {
         WriteFile("Error");
         THROW_UNREACHABLE("Indentation becomes negative");
      }
#endif
      Deindent();
   }

   if(is_line_start)
   {
      AppendIndent();
   }

   for(std::string::const_iterator it = str.begin(); it != it_end; ++it)
   {
      is_line_start = false;
      if(*it == '\n')
      {
         output_stream << *it;
         if((it + 1) != it_end)
         {
            if(*(it + 1) != closing_char)
            {
               AppendIndent();
            }
            else
            {
               needToInd = true;
            }
         }
         else
         {
            is_line_start = true;
         }
      }
      else if(*it == opening_char)
      {
         indent_spaces += delta;
         if(opening_char != STD_OPENING_CHAR)
         {
            AppendChar(*it);
         }
      }
      else if(*it == closing_char)
      {
         if(*(str.begin()) != closing_char and it != str.begin())
         {
            Deindent();
         }
         if(needToInd)
         {
            AppendIndent();
            needToInd = false;
         }
         if(STD_CLOSING_CHAR != closing_char)
         {
            AppendChar(*it);
         }
      }
      else
      {
         AppendChar(*it);
      }
   }
}

void IndentedOutputStream::AppendIndent()
{
   for(unsigned int i = 0; i < indent_spaces; i++)
   {
      output_stream << " ";
   }
}

void IndentedOutputStream::AppendChar(const char& c)
{
   output_stream << c;
}

void IndentedOutputStream::Indent()
{
   indent_spaces += delta;
}

void IndentedOutputStream::Deindent()
{
   THROW_ASSERT(indent_spaces >= indent_spaces - delta, "Indentation has become negative");
   indent_spaces -= delta;
}

const std::string IndentedOutputStream::WriteString()
{
   std::string ret;
   ret = output_stream.str();
   return ret;
}

void IndentedOutputStream::WriteFile(const std::string& file_name)
{
   std::ofstream file_out(file_name.c_str(), std::ios::out);
   file_out << output_stream.str() << std::endl;
   file_out.close();
   output_stream.str(std::string());
}
