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
 * @file call_graph_computation.hpp
 * @brief Build call_graph data structure starting from the ir_manager.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef CALL_GRAPH_COMPUTATION_HPP
#define CALL_GRAPH_COMPUTATION_HPP

#include "application_frontend_flow_step.hpp"

#include "custom_set.hpp"

/**
 * Build call graph structures starting from the ir_manager.
 */
class call_graph_computation : public ApplicationFrontendFlowStep
{
 private:
   /// Already visited IR node (used to avoid infinite recursion)
   CustomUnorderedSet<unsigned int> already_visited;

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param _parameters is the set of the parameters
    * @param AppM is the application manager
    * @param design_flow_manager is the design flow manager
    */
   call_graph_computation(const ParameterConstRef _parameters, const application_managerRef AppM,
                          const DesignFlowManager& design_flow_manager);

   /**
    * Computes the call graph data structure.
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;
};
#endif
