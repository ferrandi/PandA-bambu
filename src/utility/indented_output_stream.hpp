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

/// Special opening character used to open a new nested level. This character is just a control character and therefore
/// it will not be printed.
#define STD_OPENING_CHAR (static_cast<char>(1))
/// Special closing character used to close the current nested level. This character is just a control character and
/// therefore it will not be printed.
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

   ~IndentedOutputStream();

   /**
    * Append a string to the output
    * @param str is the string to be printed
    */
   void Append(const std::string& str);

   /**
    * Append a pre-indented string to the output
    * @param str is the string to be printed
    */
   void AppendIndented(const std::string& str);

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
using IndentedOutputStreamRef = refcount<IndentedOutputStream>;
#endif
