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
 * @file data_xml_parser.hpp
 * @brief Parse xml file containing generic data
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Date$
 *
 */

#ifndef DATA_XML_PARSER_HPP
#define DATA_XML_PARSER_HPP

/// STD include
#include <string>

/// Utility include
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(Parameter);

/**
 * This class collects information about generic data
 */
class DataXmlParser
{
 private:
   /// The set of input paraemeters
   const ParameterConstRef parameters;

   /// The debug level
   const int debug_level;

 public:
   /**
    * Constructor
    * @param parameters is the set of input parameters
    */
   explicit DataXmlParser(const ParameterConstRef& parameters);

   /**
    * Destructor
    */
   ~DataXmlParser();

   /**
    * Parse xml file
    * @param file_names is the input file
    * @param output is where data will be saved: first key is the benchmark name (sorted by name thanks to map),
    * second key is the characteristic, value is the actual value
    */
   void Parse(const CustomSet<std::string>& file_names, std::map<std::string, CustomMap<std::string, std::string>>& output) const;
};
/// Refcount definition for the class ParseProfilingAnalysis
typedef refcount<const DataXmlParser> DataXmlParserConstRef;
typedef refcount<DataXmlParser> DataXmlParserRef;
#endif
