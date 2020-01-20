/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *              Copyright (C) 2004-2020 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/**
 * @file IR_lowering.hpp
 * @brief Decompose some complex gimple statements into set of simple operations.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef IR_LOWERING_HPP
#define IR_LOWERING_HPP
#include "custom_set.hpp"                  // for unordered_set
#include "design_flow_step.hpp"            // for DesignFlowStep
#include "frontend_flow_step.hpp"          // for FrontendFlowStep...
#include "function_frontend_flow_step.hpp" // for DesignFlowManage...
#include "refcount.hpp"                    // for REF_FORWARD_DECL
#include <list>                            // for list, list<>::co...
#include <string>                          // for string
#include <utility>                         // for pair

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(bloc);
class integer_cst;
class target_mem_ref461;
class array_ref;
enum kind : int;
REF_FORWARD_DECL(IR_lowering);
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_manipulation);
REF_FORWARD_DECL(tree_node);
struct gimple_assign;
//@}

/** Indicates the type of fixup needed after a constant multiplication.
   BASIC_VARIANT means no fixup is needed, NEGATE_VARIANT means that
   the result should be negated, and ADD_VARIANT means that the
   multiplicand should be added to the result.  */
enum mult_variant
{
   basic_variant,
   negate_variant,
   add_variant
};

/**
 * Compute the control flow graph for the operations.
 */
class IR_lowering : public FunctionFrontendFlowStep
{
 private:
   /// The tree manager
   tree_managerRef TM;

   /// The IR manipulation
   tree_manipulationRef tree_man;

   /**
    * A subroutine of expand_mult, used for constant multiplications.
    * Multiply OP0 by VAL in mode MODE, storing the result in TARGET if
    * convenient.  Use the shift/add sequence described by ALG and apply
    * the final fixup specified by VARIANT.
    */
   tree_nodeRef expand_mult_const(tree_nodeRef op0, unsigned long long int val, const struct algorithm& alg, enum mult_variant& variant, const tree_nodeRef stmt, const blocRef block, tree_nodeRef& type, const std::string& srcp_default);

   /**
    * Expand signed modulus of OP0 by a power of two D in mode MODE.
    */
   tree_nodeRef expand_smod_pow2(tree_nodeRef op0, unsigned long long int d, const tree_nodeRef stmt, const blocRef block, tree_nodeRef& type, const std::string& srcp_default);

   /**
    * Expand signed division of OP0 by a power of two D in mode MODE.
    * This routine is only called for positive values of D.
    */
   tree_nodeRef expand_sdiv_pow2(tree_nodeRef op0, unsigned long long int d, const tree_nodeRef stmt, const blocRef block, tree_nodeRef& type, const std::string& srcp_default);

   tree_nodeRef expand_MC(tree_nodeRef op0, integer_cst* ic_node, tree_nodeRef old_target, const tree_nodeRef stmt, const blocRef block, tree_nodeRef& type_expr, const std::string& srcp_default);

   bool expand_target_mem_ref(target_mem_ref461* tmr, const tree_nodeRef stmt, const blocRef block, const std::string& srcp_default, bool temp_addr);

   tree_nodeRef expand_mult_highpart(tree_nodeRef op0, unsigned long long int ml, tree_nodeRef type_expr, int data_bitsize, const std::list<tree_nodeRef>::const_iterator it_los, const blocRef block, const std::string& srcp_default);

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   tree_nodeRef array_ref_lowering(array_ref* AR, const std::string& srcp_default, std::pair<unsigned int, blocRef> block, std::list<tree_nodeRef>::const_iterator it_los, bool temp_addr);

   /**
    * @brief check if the max transformation limit has been reached
    * @param stmt is the current statement
    * @return true in case all the next transformations have to be skipped
    */
   bool reached_max_transformation_limit(tree_nodeRef stmt);

   void division_by_a_constant(const std::pair<unsigned int, blocRef>& block, std::list<tree_nodeRef>::const_iterator& it_los, gimple_assign* ga, tree_nodeRef op1, enum kind code1, bool& restart_analysis, const std::string& srcp_default,
                               const std::string& step_name);

 public:
   /**
    * Constructor.
    * @param Param is the set of the parameters
    * @param AppM is the application manager
    * @param function_id is the identifier of the function
    * @param DesignFlowManagerConstRef is the design flow manager
    */
   IR_lowering(const ParameterConstRef Param, const application_managerRef AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager);

   /**
    *  Destructor
    */
   ~IR_lowering() override;

   /**
    * Computes the operations CFG graph data structure.
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;

   /**
    * Compute the relationships of a step with other steps
    * @param dependencies is where relationships will be stored
    * @param relationship_type is the type of relationship to be computed
    */
   void ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type) override;
};
#endif
