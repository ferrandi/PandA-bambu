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
 * @file simple_indent.cpp
 * @brief Implementation of the simple pretty print functor.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include <iostream>
#include <string>

#include "dbgPrintHelper.hpp"
/// Utility include
#include "exceptions.hpp"
/// Header include
#include "simple_indent.hpp"

simple_indent::simple_indent(char o, char c, unsigned int d) : indent_spaces(0), opening_char(o), closing_char(c), delta(d), is_line_start(true)
{
}

simple_indent::~simple_indent()
{
#ifndef NDEBUG
   /// This corresponds to a THROW_ASSERT, however it is against C++ rules throw an error inside a destructor
   if(indent_spaces != 0)
   {
      std::cerr << "Not all indentations have been closed: " << indentation << std::endl;
   }
#endif
}

void simple_indent::operator()(std::ostream& os, const std::string& str)
{
   std::string::const_iterator it_end = str.end();
   /// Specified whether the first character of the string
   /// we are going to write must be indented or not
   bool needToInd = false;

   if(*(str.begin()) == closing_char)
   {
      deindent();
   }

   if(is_line_start)
   {
      write_indent(os);
   }

   for(std::string::const_iterator it = str.begin(); it != it_end; ++it)
   {
      is_line_start = false;
      if(*it == '\n')
      {
         os << *it;
         if((it + 1) != it_end)
         {
            if(*(it + 1) != closing_char)
            {
               write_indent(os);
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
         indent();
         if(opening_char != STD_OPENING_CHAR)
         {
            write_char(os, *it);
         }
      }
      else if(*it == closing_char)
      {
         if(*(str.begin()) != closing_char and it != str.begin())
         {
            deindent();
         }
         if(needToInd)
         {
            write_indent(os);
            needToInd = false;
         }
         if(STD_CLOSING_CHAR != closing_char)
         {
            write_char(os, *it);
         }
      }
      else
      {
         write_char(os, *it);
      }
   }
}

void simple_indent::indent()
{
   indent_spaces += delta;
}

void simple_indent::deindent()
{
   THROW_ASSERT(indent_spaces >= indent_spaces - delta, "Indentation has become negative");
   indent_spaces -= delta;
}

void simple_indent::write_indent(std::ostream& os)
{
   for(unsigned int i = 0; i < indent_spaces; i++)
   {
      os << " ";
   }
}

void simple_indent::write_char(std::ostream& os, const char& c)
{
   os << c;
}
