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
 * @file c_initialization_parser_data.hpp
 * @brief Specification of the global data structure used during parsing of C initialization string
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
*/
#ifndef C_INITIALIZATION_PARSER_DATA_HPP
#define C_INITIALIZATION_PARSER_DATA_HPP

///STD include
#include <stack>

///utility include
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(BehavioralHelper);
CONSTREF_FORWARD_DECL(Parameter);
enum class TestbenchGeneration_MemoryType;
CONSTREF_FORWARD_DECL(tree_manager);
CONSTREF_FORWARD_DECL(tree_node);

/**
 * Data associated with ent status of the parsing of C initializaiton string
*/
class CInitializationParserData
{
   protected:
      ///The tree manager
      const tree_managerConstRef TM;

      ///The behavioral helper
      const BehavioralHelperConstRef behavioral_helper;

      ///The number of bytes to be written
      const unsigned long int reserved_mem_bytes;

      ///The number of bytes currently written
      unsigned long int written_bytes;

      ///The stream corresponding to the memory initialization file
      std::ofstream & output_stream;

      ///The stack representing the current status of th parser; the content is the next element to be read and dumped
      ///First element of the pair is the tree node describing the type, the second element of the pair is the number of the field (for struct/union) or of the element (for array)
      std::deque<std::pair<const tree_nodeConstRef, size_t> > status;

      ///The variable/parameter being printed
      const tree_nodeConstRef function_parameter;

      ///The type of initialization being written
      const TestbenchGeneration_MemoryType testbench_generation_memory_type;

      ///The debug level
      const int debug_level;

   public:
      /**
       * Constructor
       * @param output_stream is where memory initialization will be written
       * @param TM is the tree manager
       * @param behavioral_helper is the behavioral helper
       * @param reserved_mem_bytes is the number of bytes to be written
       * @param function_parameter is the function parameter whose initialization is being printed
       * @param testbench_generation_memory_type is the type of initialization being printed
       * @param parameters is the set of input parameters
       */
      CInitializationParserData(std::ofstream & output_stream, const tree_managerConstRef TM, const BehavioralHelperConstRef behavioral_helper, const unsigned long int reserved_mem_bytes, const tree_nodeConstRef function_parameter, const TestbenchGeneration_MemoryType testbench_generation_memory_type, const ParameterConstRef parameters);

      /**
       * Check that all the necessary information was present in the initialization string
       */
      void CheckEnd();

      /**
       * Start the initialization of a new composed data structure
       */
      void GoDown();

      /**
       * Ends the initialization of the current nested data structure
       */
      void GoUp();

      /**
       * Count an element
       */
      void Write(const std::string & content);
};
typedef refcount<CInitializationParserData> CInitializationParserDataRef;
#endif
