/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************            
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file hdl_testbench_generation.hpp
 * @brief Generate HDL testbench for the top-level kernel testing
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef HDL_TESTBENCH_GENERATION_HPP
#define HDL_TESTBENCH_GENERATION_HPP
#include "hls_step.hpp"
#include "refcount.hpp"

#include <filesystem>
#include <string>
#include <vector>

class module_o;
REF_FORWARD_DECL(language_writer);
REF_FORWARD_DECL(structural_object);

class HDLTestbenchGeneration final : public HLS_step
{
   const language_writerRef writer;

   structural_objectRef cir;

   const module_o* mod;

   /// output directory
   const std::filesystem::path output_sim_directory;

   /// testbench basename
   std::string hdl_testbench_basename;

   /** This function takes care of printing cache hit/miss counters. Starting from the root module, it visits all
    * submodules and checks if they have axi ports, then recursively visits axi children. If no axi children are found,
    * the current node is the axi controller and can print the axi cache stats.
    * @param rootMod Root module the search must be started from.
    * @return true if the node has at least an axi child.
    */
   bool printCacheStats(const module_o* rootMod) const;

   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    * @param parameters is the set of input parameters
    * @param _HLSMgr is the HLS manager
    * @param design_flow_manager is the design flow manager
    */
   HDLTestbenchGeneration(const ParameterConstRef parameters, const HLS_managerRef _HLSMgr,
                          const DesignFlowManager& design_flow_manager);

   void Initialize() override;

   bool HasToBeExecuted() const override;

   DesignFlowStep_Status Exec() override;
};
#endif
