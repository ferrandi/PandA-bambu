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
 * @file scalar_ssa_data_dependence_computation.hpp
 * @brief Analysis step performing data flow analysis based on ssa variables
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef SCALAR_SSA_DATA_DEPENDENCE_COMPUTATION_HPP
#define SCALAR_SSA_DATA_DEPENDENCE_COMPUTATION_HPP

#include "data_dependence_computation.hpp"

#include "refcount.hpp"

REF_FORWARD_DECL(ir_manager);

/**
 * ssa data flow analysis step
 */
class ScalarSsaDataDependenceComputation : public DataDependenceComputation
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
    * @param _Param is the set of the parameters
    * @param _AppM is the application manager
    * @param function_id is the node id of the function analyzed.
    * @param design_flow_manager is the design flow manager
    */
   ScalarSsaDataDependenceComputation(const ParameterConstRef _Param, const application_managerRef _AppM,
                                      unsigned int function_id, const DesignFlowManager& design_flow_manager);

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;
};
#endif
