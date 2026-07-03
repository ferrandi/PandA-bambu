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
 * @file check_system_type.hpp
 * @brief analyse loc_info of variables and types to detect system ones; the identified one are flagged
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef CHECK_SYSTEM_TYPE_HPP
#define CHECK_SYSTEM_TYPE_HPP

#include "custom_map.hpp"
#include "custom_set.hpp"
#include "function_frontend_flow_step.hpp"
#include "refcount.hpp"

#include <string>
#include <vector>

CONSTREF_FORWARD_DECL(BehavioralHelper);
REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_node);

/**
 * Class which system_flag to ir_node of variables and types when necessary
 */
class CheckSystemType : public FunctionFrontendFlowStep
{
 private:
   /// The helper associated with the current function
   const BehavioralHelperConstRef behavioral_helper;

   /// The IR manager
   const ir_managerRef TM;

   /**
    * Examinate recursively the IR to detect system types and system variables
    * @param tn is the root of the IR subtree to be examinated; it must be a ir_reindex
    * @param already_visited stores the IR nodes already visited during the recursive walk
    */
   void recursive_examinate(const ir_nodeRef& tn, CustomUnorderedSet<unsigned int>& already_visited) const;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   CheckSystemType(const ParameterConstRef _parameters, const application_managerRef AppM, unsigned int function_id,
                   const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;

   bool HasToBeExecuted() const override;
};
#endif
