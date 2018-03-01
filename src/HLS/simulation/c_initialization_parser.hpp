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
 *              Copyright (c) 2018 Politecnico di Milano
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
 * @file c_initialization_parser.hpp
 * @brief Interface to parse the initialization of c variable.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
*/
#ifndef C_INITIALIZATION_PARSER_HPP
#define C_INITIALIZATION_PARSER_HPP
///Utility include
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(BehavioralHelper);
REF_FORWARD_DECL(CInitializationParserData);
REF_FORWARD_DECL(CInitializationFlexLexer);
CONSTREF_FORWARD_DECL(Parameter);
enum class TestbenchGeneration_MemoryType;
CONSTREF_FORWARD_DECL(tree_manager);
CONSTREF_FORWARD_DECL(tree_node);

class CInitializationParser
{
   private:
      ///The stream of the output file
      std::ofstream & output_stream;

      ///The tree manager
      const tree_managerConstRef TM;

      ///The behavioral helper
      const BehavioralHelperConstRef behavioral_helper;

      ///The set of input parameters
      const ParameterConstRef parameters;

      ///The debug level
      int debug_level;

      /**
       * Wrapper to yyparse
       * @param data is the global data structure storing the status of the parsing
       * @param lexer is the lexer used to process the initiation string
       */
      void YYParse(const CInitializationParserDataRef data, const CInitializationFlexLexerRef lexer) const;

   public:
      /**
       * Constructor
       * @param output_stream is the stream of the output file
       * @param TM is the tree manager
       * @param behavioral_helper is the behavioral_helper of the top function
       * @param parameters is the set of input parameters
       */
      CInitializationParser(std::ofstream & output_stream, const tree_managerConstRef TM, const BehavioralHelperConstRef behavioral_helper, const ParameterConstRef parameters);

      /**
       * Parse a string to generate the corresponding memory initialization
       * @param initialization_string is the C initialization string of a variable
       * @param reserved_mem_bytes is the number of bytes to be written; this information will be used to check if the initialization is complete
       * @param parameter_type is the type of the parameter whose initialization is analyzed
       */
      void Parse(const std::string & initialization_string, const unsigned long int reserved_mem_bytes, const tree_nodeConstRef parameter_type, const TestbenchGeneration_MemoryType testbench_generation_memory_type) const;

};
typedef refcount<const CInitializationParser> CInitializationParserConstRef;
typedef refcount<CInitializationParser> CInitializationParserRef;
#endif

