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
 * @file dead_code_eliminationIPA.hpp
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef DEAD_CODE_ELIMINATION_IPA_HPP
#define DEAD_CODE_ELIMINATION_IPA_HPP
#include "application_frontend_flow_step.hpp"

REF_FORWARD_DECL(ir_manager);
struct function_val_node;

/**
 * @brief Inter-procedural dead code elimination analysis
 */
class dead_code_eliminationIPA : public ApplicationFrontendFlowStep
{
 protected:
   /**
    * stores the function ids of the functions whose Bit_Value intra procedural steps have to be invalidated by this
    * step
    */
   CustomOrderedSet<unsigned int> fun_id_to_restart;
   /**
    * stores the function ids of the functions whose Parm2SSA intra procedural steps have to be invalidated by this step
    */
   CustomOrderedSet<unsigned int> fun_id_to_restartParm;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   void ComputeRelationships(DesignFlowStepSet& relationships,
                             const DesignFlowStep::RelationshipType relationship_type) override;

   bool signature_opt(const ir_managerRef& TM, function_val_node* fd, unsigned int function_id,
                      const CustomOrderedSet<unsigned int>& rFunctions);

 public:
   dead_code_eliminationIPA(const application_managerRef AM, const DesignFlowManager& dfm,
                            const ParameterConstRef parameters);

   /**
    * Execute the step
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
