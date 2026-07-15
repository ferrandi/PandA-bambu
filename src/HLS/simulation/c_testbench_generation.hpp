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
 *   Copyright (C) 2025-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file c_testbench_generation.hpp
 * @brief Generate C testbench for the top-level kernel testing
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef C_TESTBENCH_GENERATION_HPP
#define C_TESTBENCH_GENERATION_HPP
#include "hls_step.hpp"

#include <filesystem>

class CTestbenchGeneration final : public HLS_step
{
   std::filesystem::path output_sim_directory;

   std::filesystem::path bbp_filename;

   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   void ComputeRelationships(DesignFlowStepSet& design_flow_step_set,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   CTestbenchGeneration(const ParameterConstRef parameters, const HLS_managerRef _HLSMgr,
                        const DesignFlowManager& design_flow_manager);

   bool HasToBeExecuted() const override;

   DesignFlowStep_Status Exec() override;
};
#endif // C_TESTBENCH_GENERATION_HPP
