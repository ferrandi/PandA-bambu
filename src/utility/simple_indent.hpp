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
 * @file simple_indent.hpp
 * @brief Simple pretty print functor.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef SIMPLE_INDENT_HPP
#define SIMPLE_INDENT_HPP
#include <ostream>
#include <string>

#include "refcount.hpp"

/// Special opening character used to open a new nested level. This character is just a control character and therefore
/// it will not be printed.
#define STD_OPENING_CHAR (static_cast<char>(1))
/// Special closing character used to close the current nested level. This character is just a control character and
/// therefore it will not be printed.
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
    * @param os the output stream
    * @param str the string to be printed
    */
   void operator()(std::ostream& os, const std::string& str);

   /**
    * constructor
    * @param o is the opening character used by simple_indent.
    * @param c is the closing character used by simple_indent.
    * @param d is the number of characters used to indent the code.
    */
   simple_indent(char o, char c, unsigned int d);

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

using simple_indentRef = refcount<simple_indent>;
#endif
