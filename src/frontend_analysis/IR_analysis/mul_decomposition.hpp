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
 * @file mul_decomposition.hpp
 * @brief Step that replace multiplications with software implementation in case fracturing is requested.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef MUL_DECOMPOSITION_HPP
#define MUL_DECOMPOSITION_HPP
#include "application_frontend_flow_step.hpp"

#include "custom_set.hpp"
#include "refcount.hpp"

REF_FORWARD_DECL(mul_decomposition);
REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_manipulation);
REF_FORWARD_DECL(ir_node);

/**
 * Add to the call graph the function calls associated with the integer multiplication in case multiplication fracturing
 * is requested.
 */
class mul_decomposition : public ApplicationFrontendFlowStep
{
 private:
   /// Already visited IR node (used to avoid infinite recursion)
   CustomUnorderedSet<unsigned int> already_visited;

   const ir_managerRef IRM;

   bool use64bitMul;
   bool use32bitMul;
   CustomOrderedSet<unsigned int> fun_id_to_restart;

   /**
    * Recursive examine IR node
    */
   bool recursive_transform(unsigned int function_id, const ir_nodeRef& current_ir_node,
                            const ir_nodeRef& current_statement, const ir_manipulationRef ir_man);

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   void ComputeRelationships(DesignFlowStepSet& relationships,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   /**
    * Constructor.
    * @param AM is the application manager
    * @param dfm is the design flow manager
    * @param par is the set of the parameters
    */
   mul_decomposition(const application_managerRef AM, const DesignFlowManager& dfm, const ParameterConstRef par);

   DesignFlowStep_Status Exec() override;
};
#endif
