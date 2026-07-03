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
 * @file host_profiling.hpp
 * @brief Abstract class for passes performing a dynamic profiling of loops, paths or both by means of predependence
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 */
#ifndef HOST_PROFILING_HPP
#define HOST_PROFILING_HPP

#include "application_frontend_flow_step.hpp"

#include "custom_map.hpp"
#include "refcount.hpp"

REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(loop);

/// Different profiling method
enum class HostProfiling_Method
{
   PM_NONE = 0,                /**< None profiling method selected */
   PM_BBP = 1,                 /**< Basic blocks profiling  */
   PM_HPP = 2,                 /**< Hierarchical Path Profiling */
   PM_TP = 4,                  /**< Tree based Path Profiling */
   PM_MAX_LOOP_ITERATIONS = 8, /**< Maximum number of iteration profiling */
   PM_PATH_PROBABILITY = 16,   /**< Probability based path */
   PM_XML_FILE = 32            /**< Data read from XML file */
};

HostProfiling_Method operator&(const HostProfiling_Method first, const HostProfiling_Method second);

/**
 * Class to perform profiling
 */
class HostProfiling : public ApplicationFrontendFlowStep
{
 private:
   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param AppM is the application manager
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of the parameters
    */
   HostProfiling(const application_managerRef AppM, const DesignFlowManager& design_flow_manager,
                 const ParameterConstRef parameters);

   /**
    * Do nothing
    */
   DesignFlowStep_Status Exec() override;

   /**
    * Normalize path frequency according to execution times of whole function_id
    * @param AppM is the application manger
    * @param loop_instances is how many times each loop is executed
    * @param parameters is the set of input parameters
    */
   static void
   normalize(const application_managerRef AppM,
             const CustomUnorderedMap<unsigned int, CustomUnorderedMapStable<unsigned int, long long unsigned int>>&
                 loop_instances,
             const ParameterConstRef parameters);
};

#endif
