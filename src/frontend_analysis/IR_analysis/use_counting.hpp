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
 * @file use_counting.hpp
 * @brief Analysis step counting how many time a ssa_name is used
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef USE_COUNTING_HPP
#define USE_COUNTING_HPP

/// Superclass include
#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(BehavioralHelper);
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_node);
//@}

#include "custom_map.hpp"
#include "custom_set.hpp"
#include <list>
#include <string>
#include <vector>

/**
 * Update ssa_name with information about how many times it is used
 */
class use_counting : public FunctionFrontendFlowStep
{
 private:
   /**
    * Update counters with information concerning tree_node tn
    *@param tn is the tree_node to be analyzed
    *@param ssa_uses is the vector containing the ssa_nodes "used" by the tree_node tn
    */
   void analyze_node(tree_nodeRef& tn, CustomOrderedSet<tree_nodeRef>& ssa_uses);

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * Compute the relationships of a step with other steps
    * @param dependencies is where relationships will be stored
    * @param relationship_type is the type of relationship to be computed
    */
   void ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   /**
    * Constructor.
    * @param Param is the set of the parameters
    * @param AppM is the application manager
    * @param function_id is the identifier of the function
    * @param design_flow_manager is the design flow manager
    */
   use_counting(const ParameterConstRef Param, const application_managerRef AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager);

   /**
    *  Destructor
    */
   ~use_counting() override;

   /**
    * Counts how many time a ssa_name is read
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;
};

#endif
