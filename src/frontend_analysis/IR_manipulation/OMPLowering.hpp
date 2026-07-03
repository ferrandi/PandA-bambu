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
 *              Copyright (C) 2022-2026 Politecnico di Milano
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
 * @file OMPLowering.hpp
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */

#ifndef OMP_LOWERING_HPP
#define OMP_LOWERING_HPP

#include "function_frontend_flow_step.hpp"

#include "OMPCGExt.hpp"
#include "custom_map.hpp"
#include "refcount.hpp"
#include <map>
#include <stack>
#include <string>
#include <vector>

REF_FORWARD_DECL(OMPInfo);
REF_FORWARD_DECL(bloc);
REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_manipulation);
REF_FORWARD_DECL(ir_node);

class OMPLowering : public OMPCGExt
{
 private:
   static std::map<unsigned int, CustomSet<unsigned int>> shared_infos;

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * Get OMP runtime function related to the given fork information structure
    * @param fname OMP runtime function name
    * @param function_info Funtion information
    * @return ir_nodeRef OMP runtime versioned function
    */
   ir_nodeRef ForkFunction(const std::string& fname, const OMPInfoRef& function_info) const;

 public:
   /**
    * Constructor.
    * @param parameters is the set of input parameters
    * @param AppM is the application manager
    * @param function_id is the identifier of the function
    * @param design_flow_manager is the design flow manager
    */
   OMPLowering(const ParameterConstRef parameters, const application_managerRef AppM, unsigned int function_id,
               const DesignFlowManager& design_flow_manager);

   /**
    * Updates the IR to have a more compliant CFG
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;

   bool HasToBeExecuted() const override;
};
#endif
