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
 *              Copyright (C) 2017-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
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
