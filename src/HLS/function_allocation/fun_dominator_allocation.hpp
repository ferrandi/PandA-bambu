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
 *              Copyright (C) 2004-2026 Politecnico di Milano
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
 * @file fun_dominator_allocation.hpp
 * @brief Class to allocate function in HLS based on dominators
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef FUN_DOMINATOR_ALLOCATION_HPP
#define FUN_DOMINATOR_ALLOCATION_HPP

#include "function_allocation.hpp"

class fun_dominator_allocation : public function_allocation
{
 protected:
   /// True if this step has yet been executed
   bool already_executed;

   /// list of trivial function that does not require to be proxied
   static const std::set<std::string> simple_functions;

 public:
   fun_dominator_allocation(
       const ParameterConstRef _parameters, const HLS_managerRef HLSMgr, const DesignFlowManager& design_flow_manager,
       const HLSFlowStep_Type hls_flow_step_type = HLSFlowStep_Type::DOMINATOR_FUNCTION_ALLOCATION);

   DesignFlowStep_Status Exec() override;

   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) override;

   bool HasToBeExecuted() const override;
};
#endif
