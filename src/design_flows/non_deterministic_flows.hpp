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
 *   Copyright (C) 2017-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file non_deterministic_flows.hpp
 * @brief Design flow to check different non deterministic flows
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef NON_DETERMINISTIC_FLOWS_HPP
#define NON_DETERMINISTIC_FLOWS_HPP
#include "design_flow.hpp"
#include "design_flow_step.hpp"

#include <cstddef>
#include <string>

/**
 * Class to test non deterministic flows
 */
class NonDeterministicFlows : public DesignFlow
{
   /**
    * Compute the arg list string of the tool
    * @param seed is the seed to be passed
    * @return the argument string
    */
   std::string ComputeArgString(uint_fast32_t seed) const;

   /**
    * Execute tool with non deterministic flow
    * @param seed is the seed to be passed
    * @return true if the execution was successful, false otherwise
    */
   bool ExecuteTool(uint_fast32_t seed) const;

 public:
   NonDeterministicFlows(const DesignFlowManager& design_flow_manager, const ParameterConstRef parameters);

   DesignFlowStep_Status Exec() override;
};
#endif
