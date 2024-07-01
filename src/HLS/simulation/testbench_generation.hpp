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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file testbench_generation.hpp
 * @brief Generate HDL testbench for the top-level kernel testing
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef _TESTBENCH_GENERATION_HPP_
#define _TESTBENCH_GENERATION_HPP_
#include "hls_step.hpp"
#include "refcount.hpp"

#include <filesystem>
#include <string>
#include <vector>

class module;
CONSTREF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(language_writer);
REF_FORWARD_DECL(memory);
REF_FORWARD_DECL(structural_object);

/**
 * Enum class used to specify which type of content has to be printed for memory initialization
 */
enum class TestbenchGeneration_MemoryType
{
   INPUT_PARAMETER,
   MEMORY_INITIALIZATION,
   OUTPUT_PARAMETER,
   RETURN
};

class TestbenchGeneration
#if(__GNUC__ == 4 && __GNUC_MINOR__ >= 7)
    final
#endif
    : public HLS_step
{
   const language_writerRef writer;

   structural_objectRef cir;

   const module* mod;

   /// output directory
   const std::filesystem::path output_directory;

   /// testbench basename
   std::string hdl_testbench_basename;

   /** This function takes care of printing cache hit/miss counters. Starting from the root module, it visits all
    * submodules and checks if they have axi ports, then recursively visits axi children. If no axi children are found,
    * the current node is the axi controller and can print the axi cache stats.
    * @param rootMod Root module the search must be started from.
    * @return true if the node has at least an axi child.
    */
   bool printCacheStats(const module* rootMod) const;

   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    * @param parameters is the set of input parameters
    * @param hls_mgr is the HLS manager
    * @param design_flow_manager is the design flow manager
    */
   TestbenchGeneration(const ParameterConstRef parameters, const HLS_managerRef _HLSMgr,
                       const DesignFlowManagerConstRef design_flow_manager);

   void Initialize() override;

   bool HasToBeExecuted() const override;

   DesignFlowStep_Status Exec() override;

   static std::vector<std::string> print_var_init(const tree_managerConstRef TreeM, unsigned int var,
                                                  const memoryRef mem);

   static unsigned long long generate_init_file(const std::string& dat_filename, const tree_managerConstRef TreeM,
                                                unsigned int var, const memoryRef mem);
};
#endif
