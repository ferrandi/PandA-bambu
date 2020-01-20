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
 * @file var_computation.hpp
 * @brief Analyzes operations and creates the sets of read and written variables
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef VAR_COMPUTATION_HPP
#define VAR_COMPUTATION_HPP

/// Superclass include
#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"
#include "utility.hpp"

#include "graph.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(BehavioralHelper);
class gimple_node;
enum class FunctionBehavior_VariableAccessType;
REF_FORWARD_DECL(operations_graph_constructor);
CONSTREF_FORWARD_DECL(OpGraph);
REF_FORWARD_DECL(tree_manager);
CONSTREF_FORWARD_DECL(tree_node);
//@}

/**
 *
 */
class VarComputation : public FunctionFrontendFlowStep
{
 private:
   /// The operation graph constructor
   const operations_graph_constructorRef ogc;

   /// The control flow graph of the function
   const OpGraphConstRef cfg;

   /// The behavioral helper associated with the function
   const BehavioralHelperRef behavioral_helper;

   /**
    * Recursively analyze a tree_node
    * @param op_vertex is the vertex to which the statement where tree_node is inclued belongs
    * @param tree_node is the tree node to be examined
    * @param access_type is the type of the access
    */
   void RecursivelyAnalyze(const vertex op_vertex, const tree_nodeConstRef tree_node, const FunctionBehavior_VariableAccessType access_type) const;

   /**
    * Analyze virtual operands associated with a gimple node
    * @param op_vertex is the vertex to which gimple node belongs
    * @param vops is the set of virtual operands to be considered
    */
   void AnalyzeVops(const vertex op_graph, const gimple_node* vops) const;

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param Param is the set of the parameters
    * @param AppM is the reference to the application manager
    * @param function_id is the index of the function
    * @param design_flow_manager is the design flow manager
    */
   VarComputation(const ParameterConstRef Param, const application_managerRef AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager);

   /**
    * Destructor
    */
   ~VarComputation() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;

   /**
    * Computes the set of read and written variables.
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;
};

#endif
