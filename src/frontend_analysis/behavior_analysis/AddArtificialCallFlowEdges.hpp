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
 * @file AddArtificialCallFlowEdges.hpp
 * @brief Analysis step which adds flow edges to builtin bambu time functions
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef ADD_ARTIFICIAL_CALL_FLOW_EDGES_HPP
#define ADD_ARTIFICIAL_CALL_FLOW_EDGES_HPP

#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"

/**
 * Class to perform adding of flow edges to operation graph
 */
class AddArtificialCallFlowEdges : public FunctionFrontendFlowStep
{
 private:
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   AddArtificialCallFlowEdges(const application_managerRef AppM, unsigned int function_id,
                              const DesignFlowManager& design_flow_manager, const ParameterConstRef parameters);

   DesignFlowStep_Status InternalExec() override;
};
#endif
