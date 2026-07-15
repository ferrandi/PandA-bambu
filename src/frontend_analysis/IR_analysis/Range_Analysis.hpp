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
 * @file Range_Analysis.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */

#ifndef RANGE_ANALYSIS_HPP
#define RANGE_ANALYSIS_HPP

#include <iostream>

#include "Range.hpp"
#include "application_frontend_flow_step.hpp"
#include "ir_node.hpp"

REF_FORWARD_DECL(ConstraintGraph);

enum SolverType
{
   st_Cousot,
   st_Crop
};

class RangeAnalysis : public ApplicationFrontendFlowStep
{
   SolverType solverType;
   bool computeESSA;
   int execution_mode;

   /* Stores the function ids of the functions whose Dead Code need to be restarted */
   CustomOrderedSet<unsigned int> fun_id_to_restart;

   /* Sum of reached body functions' bb+bitvalue versions after last Exec call */
   unsigned int last_ver_sum;

#ifndef NDEBUG
   int graph_debug;
   uint64_t iteration;
   uint64_t stop_iteration;
   uint64_t stop_transformation;
#endif

   bool finalize(const ConstraintGraphRef& CG);

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   void ComputeRelationships(DesignFlowStepSet& relationships,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   /**
    * Constructor.
    * @param AM is the application manager
    * @param dfm is the design flow manager
    * @param parameters is the set of the parameters
    */
   RangeAnalysis(const application_managerRef AM, const DesignFlowManager& dfm, const ParameterConstRef parameters);

   DesignFlowStep_Status Exec() override;

   void Initialize() override;

   bool HasToBeExecuted() const override;
};

#endif // !RANGE_ANALYSIS_HPP
