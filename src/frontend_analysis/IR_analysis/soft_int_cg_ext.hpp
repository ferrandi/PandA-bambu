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
 * @file soft_int_cg_ext.hpp
 * @brief Step that extends the call graph with software implementation of integer operators.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef SOFT_INT_CG_EXT_HPP
#define SOFT_INT_CG_EXT_HPP
#include "application_frontend_flow_step.hpp"

#include "custom_set.hpp"
#include "refcount.hpp"

REF_FORWARD_DECL(soft_int_cg_ext);
REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_manipulation);
REF_FORWARD_DECL(ir_node);

/**
 * Add to the call graph the function calls associated with the integer division and modulus operations.
 */
class soft_int_cg_ext : public ApplicationFrontendFlowStep
{
 private:
   /// Already visited IR node (used to avoid infinite recursion)
   CustomUnorderedSet<unsigned int> already_visited;

   const ir_managerRef IRM;

   bool doSoftDiv;

   CustomOrderedSet<unsigned int> fun_id_to_restart;

   /**
    * Recursive examine IR node
    */
   bool recursive_transform(unsigned int function_id, const ir_nodeRef& current_ir_node,
                            const ir_nodeRef& current_statement, const ir_manipulationRef ir_man);

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
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
   soft_int_cg_ext(const application_managerRef AM, const DesignFlowManager& dfm, const ParameterConstRef par);

   /**
    * transform soft int operation into function call
    */
   DesignFlowStep_Status Exec() override;
};
#endif
