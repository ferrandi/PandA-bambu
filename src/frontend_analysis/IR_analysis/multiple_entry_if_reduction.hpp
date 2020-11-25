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
 * @file multiple_entry_if_reduction.hpp
 * @brief Class performing the reduction of  n input - m output BB by duplicating
 * the BB over all its predecessors, modifing the statements in order to keep the ssa
 * and moving the phi to all the successor changing their results' ssa name
 * NOTE: this works if the BB is composed only conditions and phi statements
 *
 *
 * TESTED ON
 * int function5(int input);
 * int function6(int input);
 * int function(int input, int input2)
 * {
 *    int a;
 *    int b;
 *    if(input)
 *    {
 *       a = function1();
 *       b = function3();
 *    }
 *    else
 *    {
 *       a = function2();
 *       b = function4();
 *    }
 *    if(a)
 *    {
 *       b = function5(b);
 *    }
 *    else
 *    {
 *       b = function6(b);
 *    }
 *    return b;
 * }
 *
 * @author Andrea Caielli <andrea.caielli@mail.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef MULTIPLE_ENTRY_IF_REDUCTION_HPP
#define MULTIPLE_ENTRY_IF_REDUCTION_HPP

#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"
#include "tree_basic_block.hpp"
/**
 * @name forward declarations
 */
//@{
CONSTREF_FORWARD_DECL(AllocationInformation);
REF_FORWARD_DECL(MultipleEntryIfReduction);
REF_FORWARD_DECL(bloc);
REF_FORWARD_DECL(Schedule);
class statement_list;
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_manipulation);
REF_FORWARD_DECL(tree_node);
//@}

/**
 * @brief Class performing some optimizations exploiting the reduction of BB with n inputs and m outputs.
 * The block have to contain only phi or conditional statements (such as if and multi way if)
 */
class MultipleEntryIfReduction : public FunctionFrontendFlowStep
{
 private:
   /// The tree manager
   tree_managerRef TM;

   /// The tree manipulation
   tree_manipulationRef tree_man;

   /// The statement list
   statement_list* sl{nullptr};

   /// Modified file
   bool bb_modified{false};

   /// The scheduling solution
   ScheduleRef schedule;

   /// The allocation information
   AllocationInformationConstRef allocation_information;

   /// Estimate the area cost of the statements of a basic block
   double GetAreaCost(const std::list<tree_nodeRef>& list_of_stmt) const;

   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param _Param is the set of the parameters
    * @param _AppM is the application manager
    * @param function_id is the identifier of the function
    * @param design_flow_manager is the design flow manager
    */
   MultipleEntryIfReduction(const ParameterConstRef _Param, const application_managerRef _AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager);

   /**
    *  Destructor
    */
   ~MultipleEntryIfReduction() override;

   /**
    * Extract patterns from the IR.
    * @return the exit status of this step
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

#endif /* MultipleEntryIfReductionHPP */
