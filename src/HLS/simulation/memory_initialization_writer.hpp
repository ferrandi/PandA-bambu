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
 * @file memory_initialization_writer.hpp
 * @brief Functor used to write initialization of the memory
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef MEMORY_INITIALIZATION_WRITER_HPP
#define MEMORY_INITIALIZATION_WRITER_HPP

/// Superclass include
#include "memory_initialization_writer_base.hpp"

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
class MemoryInitializationWriter : public MemoryInitializationWriterBase
{
 protected:
   /// The stream corresponding to the memory initialization file
   std::ofstream& output_stream;

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
   MemoryInitializationWriter(std::ofstream& output_stream, const tree_managerConstRef TM, const BehavioralHelperConstRef behavioral_helper, const unsigned long int reserved_mem_bytes, const tree_nodeConstRef function_parameter,
                              const TestbenchGeneration_MemoryType testbench_generation_memory_type, const ParameterConstRef parameters);

   /**
    * Process an element
    */
   void Process(const std::string& content) override;

   /**
    * In this case the function does not activate anything
    */
   void ActivateFileInit(const std::string&) override
   {
   }

   /**
    * do nothing
    */
   void FinalizeFileInit() override
   {
   }
};
#endif
