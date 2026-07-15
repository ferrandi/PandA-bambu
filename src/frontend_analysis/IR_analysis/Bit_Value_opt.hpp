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
 * @file Bit_Value_opt.hpp
 * @brief Class performing some optimizations on the IR exploiting Bit Value analysis.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef BIT_VALUE_OPT_HPP
#define BIT_VALUE_OPT_HPP

#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"

REF_FORWARD_DECL(Bit_Value_opt);
REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_manipulation);
REF_FORWARD_DECL(ir_node);
struct function_val_node;
struct ssa_node;

/**
 * @brief Class performing some optimizations on the IR exploiting Bit Value analysis.
 */
class Bit_Value_opt : public FunctionFrontendFlowStep
{
 private:
   /// when true IR has been modified
   bool modified;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * do bit value based optimization such as:
    * - constant propagation
    * @param fd is the function declaration
    * @param TM is the IR manager
    * @param IRman is the IR manipulation helper used to rewrite the IR
    */
   void optimize(const function_val_node* fd, const ir_managerRef& TM, const ir_manipulationRef& IRman);

   void propagateValue(const ir_managerRef& TM, const ir_nodeRef& old_val, const ir_nodeRef& new_val,
                       const std::string& callSiteString);

 public:
   Bit_Value_opt(const ParameterConstRef _Param, const application_managerRef _AppM, unsigned int function_id,
                 const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;

   bool HasToBeExecuted() const override;

   static void constrainSSA(ssa_node* op_ssa, const ir_managerRef& TM);
};
#endif /* Bit_Value_opt_HPP */
