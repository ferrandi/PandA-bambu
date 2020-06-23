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
 *              Copyright (c) 2019-2020 Politecnico di Milano
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
 * @file memory_initialization_c_writer.hpp
 * @brief Functor used to write c code which writes initialization of the memory
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef MEMORY_INITIALIZATION_C_WRITER_HPP
#define MEMORY_INITIALIZATION_C_WRITER_HPP

/// Superclass include
#include "memory_initialization_writer_base.hpp"

/// STD include
#include <fstream>
#include <string>

/// utility include
#include "utility.hpp"

REF_FORWARD_DECL(IndentedOutputStream);

/**
 * Functor used to write initialization of the memory throw C instructions added to values.c
 */
class MemoryInitializationCWriter : public MemoryInitializationWriterBase
{
 protected:
   /// The stream where C code has to be written
   const IndentedOutputStreamRef indented_output_stream;

   /// temporary file used to store the formatted memory values
   std::ofstream memory_init_file;

 public:
   /**
    * Constructor
    * @param indented_output_stream is where the C code will be written
    * @param TM is the tree manager
    * @param behavioral_helper is the behavioral helper
    * @param reserved_mem_bytes is the number of bytes to be written
    * @param function_parameter is the function parameter whose initialization is being printed
    * @param testbench_generation_memory_type is the type of initialization being printed
    * @param parameters is the set of input parameters
    */
   MemoryInitializationCWriter(const IndentedOutputStreamRef indented_output_stream, const tree_managerConstRef TM, const BehavioralHelperConstRef behavioral_helper, const unsigned long int reserved_mem_bytes, const tree_nodeConstRef function_parameter,
                               const TestbenchGeneration_MemoryType testbench_generation_memory_type, const ParameterConstRef parameters);

   /**
    * Process an element
    * @param content is the string assocated with the string
    */
   void Process(const std::string& content) override;

   /**
    * In case the test_v has a size over a threshold write the tests on a file
    * @param filename is the filename to use
    */
   void ActivateFileInit(const std::string& filename) override;

   /**
    * Copy and close the file
    */
   void FinalizeFileInit() override;
};
#endif
