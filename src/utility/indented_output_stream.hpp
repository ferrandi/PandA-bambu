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
 * $Revision: $
 * $Date: $
 * Last modified by $Author: $
 *
 */
#ifndef INDENTED_OUTPUT_STREAM_HPP
#define INDENTED_OUTPUT_STREAM_HPP

/// STD include
#include <sstream>
#include <string>

/// utility include
#include "refcount.hpp"

/// Special opening character used to open a new nested level. This character is just a control character and therefore it will not be printed.
#define STD_OPENING_CHAR (static_cast<char>(1))
/// Special closing character used to close the current nested level. This character is just a control character and therefore it will not be printed.
#define STD_CLOSING_CHAR (static_cast<char>(2))

/**
 * Class to print indented code
 */
class IndentedOutputStream
{
 private:
   /// The actual stream
   std::ostringstream output_stream;

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
    * Append the indent spaces
    */
   void AppendIndent();

   /**
    * Append the current char
    */
   void AppendChar(const char& c);

 public:
   /**
    * constructor
    * @param o is the opening character used by simple_indent.
    * @param c is the closing character used by simple_indent.
    * @param d is the number of characters used to indent the code.
    */
   IndentedOutputStream(char o = '{', char c = '}', unsigned int d = 3);

   /**
    * destructor
    */
   ~IndentedOutputStream();

   /**
    * Append a string to the output
    * @param message is the string to be printed
    */
   void Append(const std::string& message);

   /**
    * Manually increase the indenting of the code
    */
   void Indent();

   /**
    * Manually reduce the indenting of the code
    */
   void Deindent();

   /**
    * Write the indented output on a string
    */
   const std::string WriteString();

   /**
    * Write the indented output on a file
    * @param file_name is the name of the file
    */
   void WriteFile(const std::string& file_name);
};
typedef refcount<IndentedOutputStream> IndentedOutputStreamRef;
#endif
