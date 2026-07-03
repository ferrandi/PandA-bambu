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
 * @file data_dependence_computation.hpp
 * @brief Base class for different data dependence computation
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef DATA_DEPENDENCE_COMPUTATION_HPP
#define DATA_DEPENDENCE_COMPUTATION_HPP

#include "function_frontend_flow_step.hpp"

#include "custom_set.hpp"
#include "graph.hpp"

/// Forward declaration
enum class VariableAccessType;

class DataDependenceComputation : public FunctionFrontendFlowStep
{
 private:
   void do_dependence_reduction();

 protected:
   /**
    * Compute the dependencies
    * @param dfg_selector is the selector to be used for DFG dependence
    * @param fb_dfg_selector is the selector to be used for DFG feedback dependence
    * @param adg_selector is the selector to be used for ADG dependence
    * @param fb_adg_selector is the selector to be used for ADG feedback dependence
    */
   DesignFlowStep_Status Computedependencies(const int dfg_selector, const int fb_dfg_selector, const int adg_selector,
                                             const int fb_adg_selector);

   /**
    * Return the variables accessed in a node
    * It is specialized in the different subclasses of this
    * @param statement is the statement to be considered
    * @param variable_access_type is the type of accesses to be considered
    */
   CustomSet<unsigned int> getVariables(gc_vertex_descriptor statement,
                                        const VariableAccessType variable_access_type) const;

 public:
   /**
    * Constructor.
    * @param _AppM is the application manager
    * @param function_id is the node id of the function analyzed.
    * @param frontend_flow_step_type is the type of data flow analysis
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    * */
   DataDependenceComputation(const application_managerRef _AppM, unsigned int function_id,
                             const FrontendFlowStepType frontend_flow_step_type,
                             const DesignFlowManager& design_flow_manager, const ParameterConstRef parameters);

   /**
    * Cleans the fake data dependencies
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() final;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override = 0;
};
#endif
