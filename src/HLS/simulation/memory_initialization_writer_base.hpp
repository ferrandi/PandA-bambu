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
 *              Copyright (c) 2018-2020 Politecnico di Milano
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
 * @file memory_initialization_writer_base.hpp
 * @brief Abstract base class for functor used to write initialization of the memory
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef MEMORY_INITIALIZATION_WRITER_BASE_HPP
#define MEMORY_INITIALIZATION_WRITER_BASE_HPP

/// Superclass include
#include "c_initialization_parser_functor.hpp"

/// STD include
#include <string>

/// STL includes
#include <utility>
#include <vector>

/// utility include
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(BehavioralHelper);
CONSTREF_FORWARD_DECL(Parameter);
enum class TestbenchGeneration_MemoryType;
CONSTREF_FORWARD_DECL(tree_manager);
CONSTREF_FORWARD_DECL(tree_node);

/**
 * Functor used to write initialization of the memory writer
 */
class MemoryInitializationWriterBase : public CInitializationParserFunctor
{
 protected:
   /// The tree manager
   const tree_managerConstRef TM;

   /// The behavioral helper
   const BehavioralHelperConstRef behavioral_helper;

   /// The number of bytes to be written
   const unsigned long int reserved_mem_bytes;

   /// The number of bytes currently written
   unsigned long int written_bytes;

   /// The stack representing the current status of the parser; the content is the last dumped element.
   /// First element of the pair is the tree node describing the type, the second element of the pair is the number of the field (for struct/union) or of the element (for array)
   /// Second element is the number of seen elements (index of the last element + 1)
   /// Note that storing last element dumped is equivalent to store next element to be dumped, but this approach make easier check of closes parenthesis
   std::vector<std::pair<const tree_nodeConstRef, size_t>> status;

   /// The variable/parameter being printed
   const tree_nodeConstRef function_parameter;

   /// The type of initialization being written
   const TestbenchGeneration_MemoryType testbench_generation_memory_type;

   /**
    * Print the current status
    */
   const std::string PrintStatus() const;

 public:
   /**
    * Constructor
    * @param TM is the tree manager
    * @param behavioral_helper is the behavioral helper
    * @param reserved_mem_bytes is the number of bytes to be written
    * @param function_parameter is the function parameter whose initialization is being printed
    * @param testbench_generation_memory_type is the type of initialization being printed
    * @param parameters is the set of input parameters
    */
   MemoryInitializationWriterBase(const tree_managerConstRef TM, const BehavioralHelperConstRef behavioral_helper, const unsigned long int reserved_mem_bytes, const tree_nodeConstRef function_parameter,
                                  const TestbenchGeneration_MemoryType testbench_generation_memory_type, const ParameterConstRef parameters);

   /**
    * Check that all the necessary information was present in the initialization string
    */
   void CheckEnd() override;

   /**
    * Start the initialization of a new aggregated data structure
    */
   void GoDown() override;

   /**
    * Consume an element of an aggregated data structure
    */
   void GoNext() override;

   /**
    * Ends the initialization of the current aggregated  data structure
    */
   void GoUp() override;
};
#endif
