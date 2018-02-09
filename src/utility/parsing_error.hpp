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
 *              Copyright (c) 2004-2018 Politecnico di Milano
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
 * @file parsing_error.hpp
 * @brief function used to print out error stuff for spirit based parser
 *
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
*/
#ifndef PARSING_ERROR_HPP
#define PARSING_ERROR_HPP

#include <boost/tokenizer.hpp>
#include <boost/spirit/iterator/position_iterator.hpp>
#include <string>
#include <iosfwd>

#define STRING_LENGTH 32

inline
std::ostream& operator<<(std::ostream& out, boost::spirit::file_position const& lc)
{
   return out <<
          "|File:" << lc.file <<
          "|Line:" << lc.line <<
          "|Col:" << lc.column << "|";
}


/**
 * print a message and the token where the error is encountered.
 * @param info spirit information returned by the spirit parse function.
 * @param class_name is the name of the calling class
*/
template <class info_t>
void print_parsing_error(info_t &info, const std::string&class_name)
{
   std::string curr_string = std::string(info.stop, info.stop + STRING_LENGTH);
   typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
   boost::char_separator<char> sep(" \t", "\n");
   tokenizer tokens(curr_string, sep);
   std::cerr << class_name << ": Parsing error: " << info.stop.get_position() << "\n\t>>";
   if (*tokens.begin() == "\n")
      std::cerr << "\\n";
   else
      std::cerr << *tokens.begin();

   std::cerr << "<<\n";
}
#endif
