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
 * @file operations_cfg_computation.hpp
 * @brief Analysis step creating the control flow graph for the operations.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef OPERATIONS_CFG_COMPUTATION_HPP
#define OPERATIONS_CFG_COMPUTATION_HPP

/// Autoheader include
#include "config_HAVE_BAMBU_BUILT.hpp"

/// Superclass include
#include "function_frontend_flow_step.hpp"

/// STD include
#include <string>

/// STL includes
#include "custom_map.hpp"
#include <list>

/// utility include
#include "refcount.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(operations_cfg_computation);
REF_FORWARD_DECL(operations_graph_constructor);
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_node);
//@}
//
/**
 * Compute the control flow graph for the operations.
 */
class operations_cfg_computation : public FunctionFrontendFlowStep
{
 private:
   unsigned int last_bb_cfg_computation_version;

   /// relation between label declaration and first statement id
   std::map<unsigned int, std::string> label_decl_map;

   /// relation between basic block and first statement id
   std::map<unsigned int, std::string> first_statement;

   /// store the name of the nodes at which the next node should be attached.
   std::list<std::string> start_nodes;

   /// store the name of the current vertex
   std::string actual_name;

   /**
    * Return the name of the first node given a tree node.
    * @param tn is the tree node.
    * @param f_name is the name of the function of the node we are analyzing
    * @return the name of the first node
    */
   std::string get_first_node(const tree_nodeRef& tn, const std::string& f_name) const;

   /**
    * Clean the list of start nodes
    */
   void clean_start_nodes();

   /**
    * Insert a start node to the list of start nodes
    * @param start_node is the starting node
    */
   void insert_start_node(const std::string& start_node);

   /**
    * Return true if start_node is empty
    * @return true if start node is empty
    */
   bool empty_start_nodes() const;

   /**
    * Initialize the list of start nodes
    * @param start_node is the starting node
    */
   void init_start_nodes(const std::string& start_node);

   /**
    * Connect start_node with the next node.
    * @param ogc is the operation graph constructor used to add the edges.
    * @param next is the ending node of the edge that must be added to g.
    * @param true_edge when it is true the control edge is the true branch of an if then else statement.
    * @param false_edge when it is true the control edge is the false branch of an if then else statement.
    * @param nodeid is meaningful only in case true_edge and false_edge are both true and the control edge is associated with a switch statement.
    */
   void connect_start_nodes(const operations_graph_constructorRef ogc, const std::string& next, bool true_edge = false, bool false_edge = false, unsigned int nodeid = 0);

   /**
    * Builds recursively the operation for a given tree node. We assume a one to one mapping between nodeids and vertices
    * @param TM is the tree manager.
    * @param ogc is the operation graph constructor used to add the vertices.
    * @param tn is the reference of the tree node we are currently analyzing
    * @param f_name is the name of the function which the node we are analyzing belongs to
    */
   void build_operation_recursive(const tree_managerRef TM, const operations_graph_constructorRef ogc, const tree_nodeRef tn, const std::string& f_name, unsigned int bb_index);

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
    * @param function_id is the identifier of the function
    */
   operations_cfg_computation(const ParameterConstRef Param, const application_managerRef AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager);

   /**
    *  Destructor
    */
   ~operations_cfg_computation() override;

   /**
    * Computes the operations CFG graph data structure.
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;
};

#endif
