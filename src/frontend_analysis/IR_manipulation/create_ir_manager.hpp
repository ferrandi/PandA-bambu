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
 * @file create_ir_manager.hpp
 * @brief Class that creates the ir_manager starting from the source code files
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef _CREATE_IR_MANAGER_HPP_
#define _CREATE_IR_MANAGER_HPP_
#include "application_frontend_flow_step.hpp"

/**
 * Class that creates the ir_manager starting from the source code files
 */
class create_ir_manager : public ApplicationFrontendFlowStep
{
   /**
    * @brief createCostTable: Fill the CostTable starting from the technology files
    */
   std::string createCostTable();

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   /**
    * Constructor
    * @param _parameters is the set of the parameters
    * @param AppM is the reference to the application manager
    * @param design_flow_manager is the design flow manager
    */
   create_ir_manager(const ParameterConstRef _parameters, const application_managerRef AppM,
                     const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status Exec() override;

   bool HasToBeExecuted() const override;
};
#endif
