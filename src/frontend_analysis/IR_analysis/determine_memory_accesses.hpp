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
 * @file determine_memory_accesses.hpp
 * @brief Determine variables to be stored in memory
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$ $
 * Last modified by $Author$
 *
 */

#ifndef DETERMINE_MEMORY_ACCESSES_HPP
#define DETERMINE_MEMORY_ACCESSES_HPP

/// Superclass include
#include "function_frontend_flow_step.hpp"

/// STL include
#include "custom_map.hpp"
#include <list>

/// Utility include
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(BehavioralHelper);
CONSTREF_FORWARD_DECL(tree_manager);

class determine_memory_accesses : public FunctionFrontendFlowStep
{
 private:
   /// The behavioral helper
   const BehavioralHelperConstRef behavioral_helper;

   /// The tree manager
   const tree_managerConstRef TM;

   /// Already visited address expression (used to avoid infinite recursion)
   CustomUnorderedSet<unsigned int> already_visited_ae;

   /// True if already executed
   bool already_executed;

   /**
    * Analyze the given node ID to determine which variables have to be referred in memory
    */
   void analyze_node(unsigned int node_id, bool left_p, bool dynamic_address, bool no_dynamic_address);

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param parameters is the set of input parameters
    * @param AppM is the application manager
    * @param function_id is the node id of the function analyzed.
    * @param design_flow_manager is the design flow manager
    */
   determine_memory_accesses(const ParameterConstRef parameters, const application_managerRef AppM, unsigned int function_idi, const DesignFlowManagerConstRef design_flow_manager);

   /**
    *  Destructor
    */
   ~determine_memory_accesses() override;

   /**
    * Determines the variables that require a memory access
    */
   DesignFlowStep_Status InternalExec() override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;
};
#endif
