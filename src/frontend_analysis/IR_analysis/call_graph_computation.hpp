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
 * @file call_graph_computation.hpp
 * @brief Build call_graph data structure starting from the tree_manager.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef CALL_GRAPH_COMPUTATION_HPP
#define CALL_GRAPH_COMPUTATION_HPP

/// Superclass include
#include "application_frontend_flow_step.hpp"

#include "call_graph.hpp"

#include "refcount.hpp"
#include "utility.hpp"

#include "custom_map.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(BehavioralHelper);
REF_FORWARD_DECL(call_graph_computation);
REF_FORWARD_DECL(CallGraphManager);
CONSTREF_FORWARD_DECL(FunctionExpander);
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_node);
//@}

/**
 * Build call graph structures starting from the tree_manager.
 */
class call_graph_computation : public ApplicationFrontendFlowStep
{
 private:
   /// The CallGraphManager used to modify the call graph
   const CallGraphManagerRef CGM;

   /// Index of current function
   unsigned int current;

   /**
    * Recursive analysis of the tree nodes looking for call expressions.
    * @param TM is the tree manager.
    * @param tn is current tree node.
    * @param node_stmt is the analyzed tree node
    * @param call_type is the type of call to be added
    */
   void call_graph_computation_recursive(const tree_managerRef& TM, const tree_nodeRef& tn, unsigned int node_stmt, enum FunctionEdgeInfo::CallType call_type);

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param Param is the set of the parameters
    * @param AppM is the application manager
    * @param design_flow_manager is the design flow manager
    */
   call_graph_computation(const ParameterConstRef Param, const application_managerRef AppM, const DesignFlowManagerConstRef design_flow_manager);

   /**
    *  Destructor
    */
   ~call_graph_computation() override;

   /**
    * Computes the call graph data structure.
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;
};
#endif
