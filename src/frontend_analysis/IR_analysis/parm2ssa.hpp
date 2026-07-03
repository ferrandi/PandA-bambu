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
 *              Copyright (C) 2019-2026 Politecnico di Milano
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
 * @file parm2ssa.hpp
 * @brief Pre-analysis step computing the relation between argument_val_node and the associated ssa_node.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef PARM2SSA_HPP
#define PARM2SSA_HPP
#include "function_frontend_flow_step.hpp"

#include "custom_map.hpp"
#include "custom_set.hpp"
#include "refcount.hpp"

REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_node);

/**
 * Pre-analysis step. computing the relation between argument_val_node and the associated ssa_node.
 */
class parm2ssa : public FunctionFrontendFlowStep
{
 protected:
   /**
    * Recursive IR node analysis
    */
   void recursive_analysis(const ir_nodeRef& tn, const std::string& loc_info,
                           CustomUnorderedSet<unsigned int>& already_visited_ae);

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   parm2ssa(const ParameterConstRef _parameters, const application_managerRef AppM, unsigned int _function_id,
            const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;
};
#endif
