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
 * @file determine_memory_accesses.hpp
 * @brief Determine variables to be stored in memory
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef DETERMINE_MEMORY_ACCESSES_HPP
#define DETERMINE_MEMORY_ACCESSES_HPP
#include "function_frontend_flow_step.hpp"

#include "custom_set.hpp"
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(BehavioralHelper);
CONSTREF_FORWARD_DECL(ir_manager);
CONSTREF_FORWARD_DECL(ir_node);
REF_FORWARD_DECL(ir_node);

class determine_memory_accesses : public FunctionFrontendFlowStep
{
 private:
   /// The behavioral helper
   const BehavioralHelperConstRef behavioral_helper;

   /// The IR manager
   const ir_managerConstRef TM;

   /// Already visited address expression (used to avoid infinite recursion)
   CustomUnorderedSet<unsigned int> already_visited_ae;
   CustomUnorderedSet<unsigned int> already_visited;

   void analyze_call(const ir_nodeConstRef& tn, const ir_nodeRef& fnode, const std::vector<ir_nodeRef>& args);

   /**
    * Analyze the given node ID to determine which variables have to be referred in memory
    */
   void analyze_node(const ir_nodeConstRef& tn, bool left_p, bool dynamic_address, bool no_dynamic_address);

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   determine_memory_accesses(const ParameterConstRef parameters, const application_managerRef AppM,
                             unsigned int _function_id, const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;
};
#endif
