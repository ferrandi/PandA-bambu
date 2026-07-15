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
 * @file rebuild_initialization2.hpp
 * @brief rebuild initialization where it is possible
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef _REBUILD_INITIALIZATION2_HPP
#define _REBUILD_INITIALIZATION2_HPP
#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"

REF_FORWARD_DECL(ir_node);
class mem_access_node;

/**
 * Rebuild initialization function flow front-end step done after IR_lowering
 */
class rebuild_initialization2 : public FunctionFrontendFlowStep
{
 private:
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * @brief extract_var_decl_ppe
    * @param addr_assign_op1 is the pointer expression used by the address assignment
    * @param vd_index is the variable decl index
    * @param vd_node is the variable decl IR re-index node
    */
   bool extract_var_decl_ppe(ir_nodeRef addr_assign_op1, unsigned& vd_index, ir_nodeRef& vd_node);

   /**
    * @brief extract_var_decl return the variable decl referred by the mem_access_node given it is resolvable
    * @param me is the memory reference node
    * @param vd_index is the variable decl index
    * @param vd_node is the variable decl IR re-index node
    * @param addr_assign_op1 is the pointer expression used by the mem_access_node
    * @return true in case it is possible to compute the variable decl referred, false otherwise
    */
   bool extract_var_decl(const mem_access_node* me, unsigned& vd_index, ir_nodeRef& vd_node,
                         ir_nodeRef& addr_assign_op1);

   /**
    * @brief look_for_ROMs transforms the IR by looking for an initial sequence of writes followed
    * by read only instructions. In case the writes have constant offset and
    * the written values are constants a constant array may be defined.
    */
   bool look_for_ROMs();

 public:
   rebuild_initialization2(const ParameterConstRef Param, const application_managerRef AppM, unsigned int function_id,
                           const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;
};

#endif // _REBUILD_INITIALIZATION2_HPP
