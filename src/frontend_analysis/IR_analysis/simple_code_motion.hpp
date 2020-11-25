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
 * @file simple_code_motion.hpp
 * @brief Analysis step that performs some simple code motions over the IR
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef SIMPLE_CODE_MOTION_HPP
#define SIMPLE_CODE_MOTION_HPP

/// Superclass include
#include "function_frontend_flow_step.hpp"

/// utility includes
#include "custom_map.hpp"
#include "refcount.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(Schedule);
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(bloc);
class ssa_name;
class gimple_assign;
//@}

/**
 * Restructure the tree control flow graph
 */
class simple_code_motion : public FunctionFrontendFlowStep
{
 private:
   /// The scheduling solution
   ScheduleRef schedule;

   /// flag to check if initial tree has been dumped
   static bool tree_dumped;

   /// True if only zero delay statement can be moved
   bool conservative;

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * Check if a statement can be moved in a basic block
    * @param bb_index is the index of the basic block
    * @param gn is the statement to be moved
    * @param zero_delay returns if statement has zero delay
    * @param TM is the tree manager
    * @return if the statement can be moved
    */
   FunctionFrontendFlowStep_Movable CheckMovable(const unsigned int dest_bb_index, tree_nodeRef tn, bool& zero_delay, const tree_managerRef TM);

   void loop_pipelined(tree_nodeRef curr_stmt, const tree_managerRef TM, unsigned int curr_bb, unsigned int curr_loop_id, std::list<tree_nodeRef>& to_be_removed, std::list<tree_nodeRef>& to_be_added_back, std::list<tree_nodeRef>& to_be_added_front,
                       std::map<unsigned int, blocRef>& list_of_bloc, std::map<std::pair<unsigned int, blocRef>, std::pair<unsigned int, blocRef>>& dom_diff, unsigned int curr_bb_dom);

 public:
   /**
    * Constructor.
    * @param parameters is the set of input parameters
    * @param AppM is the application manager
    * @param function_id is the identifier of the function
    * @param design_flow_manager is the design flow manager
    */
   simple_code_motion(const ParameterConstRef parameters, const application_managerRef AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager);

   /**
    *  Destructor
    */
   ~simple_code_motion() override;

   /**
    * Updates the tree to have a more compliant CFG
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

   /**
    * Return true if the last run of this step was based on scheduling
    */
   bool IsScheduleBased() const;
};
#endif
