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
 * @file design_flow.hpp
 * @brief This class contains the base representation for design flow
 *
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef DESIGN_FLOW_HPP
#define DESIGN_FLOW_HPP
#include "design_flow_step.hpp"

#include <string>

enum class DesignFlow_Type
{
   NON_DETERMINISTIC_FLOWS = 0
};

class DesignFlow : public DesignFlowStep
{
 protected:
   /// The type of this design flow
   const DesignFlow_Type design_flow_type;

   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    * @param design_flow_type is the type of the flow
    * @param parameters is the set of the parameters
    */
   DesignFlow(const DesignFlowManager& design_flow_manager, DesignFlow_Type design_flow_type,
              const ParameterConstRef parameters);

   std::string GetName() const override;

   DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory() const override;

   bool HasToBeExecuted() const override;

   /**
    * Compute the signature of a step
    * @param design_flow_type is the type of design flow
    * @return the signature corresponding to the design flow
    */
   static signature_t ComputeSignature(DesignFlow_Type design_flow_type);

   /**
    * Return the name of the type
    * @param design_flow_type is the type of the design flow
    * @return the name
    */
   static std::string EnumToKindText(const DesignFlow_Type design_flow_type);

   /**
    * Given the name of design flow, return the enum
    * @param name is the name of the design flow
    * @return the corresponding enum
    */
   static DesignFlow_Type KindTextToEnum(const std::string& name);
};
#endif
