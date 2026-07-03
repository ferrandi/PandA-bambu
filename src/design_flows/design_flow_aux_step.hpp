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
 * @file design_flow_aux_step.hpp
 * @brief Class for describing auxiliary steps in design flow
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef DESIGN_FLOW_AUX_STEP_HPP
#define DESIGN_FLOW_AUX_STEP_HPP
#include "design_flow_step.hpp"

#include <iosfwd>
#include <string>

/// Identifier of the auxiliary design flow steps
using AuxDesignFlowStepType = enum AuxDesignFlowStepType {
   DESIGN_FLOW_ENTRY, //! Entry point for the design flow
   DESIGN_FLOW_EXIT   //! Exit point for the design flow
};

/**
 * Class describing auxiliary steps in design flow
 */
class AuxDesignFlowStep : public DesignFlowStep
{
 private:
   /// The type of this auxiliary design flow step
   const AuxDesignFlowStepType type;

   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   /**
    * Constructor
    * @param type is the type of the step
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   AuxDesignFlowStep(AuxDesignFlowStepType type, const DesignFlowManager& design_flow_manager,
                     const ParameterConstRef parameters);

   DesignFlowStep_Status Exec() override;

   bool HasToBeExecuted() const override;

   std::string GetName() const override;

   void writeDot(std::ostream& out) const override;

   DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory() const override;

   /**
    * Compute the signature of a sdf design flow step
    * @param type is the type of auxiliary step
    * @return the signature corresponding to the analysis/transformation
    */
   static signature_t ComputeSignature(AuxDesignFlowStepType type);
};
#endif
