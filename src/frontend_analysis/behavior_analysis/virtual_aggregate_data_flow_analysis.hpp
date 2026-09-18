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
 * @file virtual_aggregate_data_flow_analysis.hpp
 * @brief Analysis step performing aggregate variable computation on the basis of IR virtual operands
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS_HPP
#define VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS_HPP

#include "data_dependence_computation.hpp"

#include "refcount.hpp"

class VirtualAggregateDataFlowAnalysis : public DataDependenceComputation
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
    * Constructor
    * @param AppM is the application manager
    * @param design_flow_manager is the design flow manager
    * @param _function_index is the index of the function
    * @param parameters is the set of the parameters
    */
   VirtualAggregateDataFlowAnalysis(const application_managerRef AppM, const DesignFlowManager& design_flow_manager,
                                    const unsigned int _function_index, const ParameterConstRef parameters);

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;
};
#endif
