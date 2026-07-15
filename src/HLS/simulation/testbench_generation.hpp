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
 * @file testbench_generation.hpp
 * @brief Generate testbench for the top-level kernel testing
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef TESTBENCH_GENERATION_HPP
#define TESTBENCH_GENERATION_HPP
#include "hls_step.hpp"

#include "refcount.hpp"

class memory;
CONSTREF_FORWARD_DECL(ir_manager);

class TestbenchGeneration final : public HLS_step
{
   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   void ComputeRelationships(DesignFlowStepSet& design_flow_step_set,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   TestbenchGeneration(const ParameterConstRef parameters, const HLS_managerRef _HLSMgr,
                       const DesignFlowManager& design_flow_manager);

   bool HasToBeExecuted() const override;

   DesignFlowStep_Status Exec() override;

   static std::vector<std::string> print_var_init(const ir_managerConstRef IRM, unsigned int var,
                                                  const std::unique_ptr<memory>& mem);

   static unsigned long long generate_init_file(const std::string& dat_filename, const ir_managerConstRef IRM,
                                                unsigned int var, const std::unique_ptr<memory>& mem);
};
#endif // TESTBENCH_GENERATION_HPP
