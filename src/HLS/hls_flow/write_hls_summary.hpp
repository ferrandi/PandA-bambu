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
 * @file write_hls_summary.hpp
 * @brief Class to dump hls summary
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef WRITE_HLS_SUMMARY_HPP
#define WRITE_HLS_SUMMARY_HPP

/// Superclass include
#include "hls_step.hpp"

class WriteHLSSummary : public HLS_step
{
 protected:
   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    * @param parameters is the set of input parameters
    * @param hls_mgr is the HLS manager
    * @param design_flow_manager is the design flow manager
    */
   WriteHLSSummary(const ParameterConstRef parameters, const HLS_managerRef hls_mgr,
                   const DesignFlowManager& design_flow_manager);

   /**
    * Execute this step
    */
   DesignFlowStep_Status Exec() override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;
};
#endif
