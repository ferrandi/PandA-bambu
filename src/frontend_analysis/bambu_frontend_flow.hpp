/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************            
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file bambu_frontend_flow.hpp
 * @brief The step representing the frontend flow for bambu
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 */

#ifndef BAMBU_FRONTEND_FLOW_HPP
#define BAMBU_FRONTEND_FLOW_HPP
#include "application_frontend_flow_step.hpp"
#include "custom_set.hpp"
#include "design_flow_step.hpp"
#include "frontend_flow_step.hpp"

#include <utility>

class BambuFrontendFlow : public ApplicationFrontendFlowStep
{
 protected:
   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    * @param AppM is the application manager
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   BambuFrontendFlow(const application_managerRef AppM, const DesignFlowManager& design_flow_manager,
                     const ParameterConstRef parameters);

   /**
    * Execute this step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;
};
#endif
