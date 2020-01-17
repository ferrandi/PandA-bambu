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
 * @file PragmaParser.hpp
 * @brief Parsing pragma from C sources.
 *
 * A object for retrieving information about pragma directives in a C/C++ program.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef PRAGMAPARSER_HPP
#define PRAGMAPARSER_HPP

/// STD include
#include <iosfwd>

/// STL include
#include "custom_map.hpp"
#include "custom_set.hpp"
#include <list>

/// Utility include
#include "dbgPrintHelper.hpp"
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(pragma_manager);

/**
 * @class PragmaParser
 * Main class for parsing: contains the context associated with a parsing action
 * and the methods to retrieve all the information
 */
class PragmaParser
{
 private:
   /// counter of generic pragma
   static unsigned int number;

   /// pointer to the pragma manager datastructure
   const pragma_managerRef PM;

   /// current debugging level
   const int debug_level;

   /// reference to the parameter datastructure
   const ParameterConstRef Param;

   /**
    * Retrieve information about a pragma directive when the token "#pragma" has been found
    */
   bool analyze_pragma(std::string& Line);

   /**
    * Retrieve information about a pragma directive when a OpenMP (parallelism) pragma has been found
    */
   bool recognize_omp_pragma(std::string& Line);

   /**
    * Retrieve information about a pragma directive when a mapping pragma has been found
    */
   bool recognize_mapping_pragma(std::string& Line);

   /**
    * Retrieve information about a pragma directive of type pragma map call_point_hw
    * @param line is the line containing the pragma; it will be replaced con a function call
    */
   bool recognize_call_point_hw_pragma(std::string& line) const;

   /**
    * Retrieve information about a pragma directive when an issue pragma has been found
    */
   bool recognize_issue_pragma(std::string& Line);

   /**
    * Retrieve information about a pragma directive when a profiling pragma has been found
    */
   bool recognize_profiling_pragma(std::string& Line);

   /**
    * Retrieve information about a generic pragma directive (i.e., none of the known ones has been detected)
    */
   bool recognize_generic_pragma(std::string& Line);

   /// counter for nesting level
   unsigned int level;

   std::list<std::string> FloatingPragmas;

   CustomUnorderedSet<std::string> FunctionPragmas;

   std::map<unsigned int, std::list<std::string>> OpenPragmas;

   bool search_function;

   std::string name_function;

   /// Counter of examined files
   static unsigned int file_counter;

 public:
   /**
    * Constructor
    * @param PM is the pragma manager
    * @param Param is the set of input parameters
    */
   PragmaParser(const pragma_managerRef PM, const ParameterConstRef Param);

   /**
    * Destructor
    */
   ~PragmaParser();

   /**
    * Substitute the pragmas with proper functions
    * @param OldFile is the name of the file to be analyzed and substituted
    * @return the name of the new file coming from the substitution
    */
   std::string substitutePragmas(const std::string& OldFile);
};

/// Refcount definition for the PragmaParser class
typedef refcount<PragmaParser> PragmaParserRef;

#endif
