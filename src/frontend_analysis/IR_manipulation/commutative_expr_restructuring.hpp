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
 *              Copyright (C) 2018-2020 Politecnico di Milano
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
 * @file commutative_expr_restructuring.hpp
 * @brief Analysis step restructing tree of commutative expressions to reduce the critical path delay.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef COMMUTATIVE_EXPR_RESTRUCTURING_HPP
#define COMMUTATIVE_EXPR_RESTRUCTURING_HPP

/// Superclass include
#include "function_frontend_flow_step.hpp"

/// utility include
#include "refcount.hpp"

REF_FORWARD_DECL(AllocationInformation);
REF_FORWARD_DECL(Schedule);
REF_FORWARD_DECL(tree_manager);
CONSTREF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(tree_node);

class commutative_expr_restructuring : public FunctionFrontendFlowStep
{
 private:
   /// The schedule
   ScheduleRef schedule;

   /// The allocation information
   AllocationInformationRef allocation_information;

   /// The tree manager
   tree_managerRef TM;

   /**
    * Return true if tree node is a gimple_assign with a mult_expr/plus_expr in the right part
    * @param tn is the tree reindex to be considered
    */
   bool IsCommExprGimple(const tree_nodeConstRef tn) const;

   /**
    * Given a gimple_assign with cond_expr in the right part and one of its operand it checks:
    * - if operand is a ssa_name
    * - if operand is defined in the same basic block
    * - if operand is defined in a gimple_assign whose right a part is another cond_expr
    * - if operand is on the relative critical path (i.e., it delays execution of tn
    * @param tn is the starting gimple_assign
    * @param first is true if first operand has to be considered, false if second oeprand has to be considered
    * @return the chained gimple_assignment if all conditions hold
    */
   tree_nodeRef IsCommExprChain(const tree_nodeConstRef tn, const bool first) const;

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param AppM is the application manager
    * @param function_id is the node id of the function analyzed.
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   commutative_expr_restructuring(const application_managerRef AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    *  Destructor
    */
   ~commutative_expr_restructuring() override;

   /**
    * Performs the loops analysis
    */
   DesignFlowStep_Status InternalExec() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;
};
#endif
