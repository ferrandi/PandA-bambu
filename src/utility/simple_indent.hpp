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
 * @file simple_indent.hpp
 * @brief Simple pretty print functor.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef SIMPLE_INDENT_HPP
#define SIMPLE_INDENT_HPP
#include <ostream>
#include <string>

#include "refcount.hpp"

/// Special opening character used to open a new nested level. This character is just a control character and therefore it will not be printed.
#define STD_OPENING_CHAR (static_cast<char>(1))
/// Special closing character used to close the current nested level. This character is just a control character and therefore it will not be printed.
#define STD_CLOSING_CHAR (static_cast<char>(2))

/**
 * Very simple pretty printer functor.
 */
class simple_indent
{
 private:
   /// number of spaces used to indent after a new line print
   unsigned int indent_spaces;

   /// char that increments the indent space by a delta
   char opening_char;

   /// char that increments the indent space by a delta
   char closing_char;

   /// delta indent space
   unsigned int delta;

   bool is_line_start;

   /**
    * Write the indent spaces
    */
   void write_indent(std::ostream& os);

   /**
    * Write the current char
    */
   void write_char(std::ostream& os, const char& c);

 public:
   /**
    * pretty print functor
    * @param is the output stream
    * @param is the string to be printed
    */
   void operator()(std::ostream& os, const std::string& str);

   /**
    * constructor
    * @param o is the opening character used by simple_indent.
    * @param c is the closing character used by simple_indent.
    * @param d is the number of characters used to indent the code.
    */
   simple_indent(char o, char c, unsigned int d);

   /**
    * destructor
    */
   ~simple_indent();

   /**
    * Manually increase the indenting of the code
    */
   void indent();

   /**
    * Manually reduce the indenting of the code
    */
   void deindent();
};

typedef refcount<simple_indent> simple_indentRef;
#endif
