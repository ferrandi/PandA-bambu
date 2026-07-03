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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file operations_cfg_computation.hpp
 * @brief Analysis step creating the control flow graph for the operations.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef OPERATIONS_CFG_COMPUTATION_HPP
#define OPERATIONS_CFG_COMPUTATION_HPP

#include "custom_map.hpp"
#include "function_frontend_flow_step.hpp"
#include "refcount.hpp"
#include <list>
#include <string>

REF_FORWARD_DECL(operations_cfg_computation);
REF_FORWARD_DECL(operations_graph_constructor);
REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_node);

/**
 * Compute the control flow graph for the operations.
 */
class operations_cfg_computation : public FunctionFrontendFlowStep
{
 private:
   /// relation between basic block and first statement id
   std::map<unsigned int, std::string> first_statement;

   /// store the name of the nodes at which the next node should be attached.
   std::list<std::string> start_nodes;

   /// store the name of the current vertex
   std::string actual_name;

   /**
    * Return the name of the first node given an IR node.
    * @param tn is the IR node.
    * @param f_name is the name of the function of the node we are analyzing
    * @return the name of the first node
    */
   std::string get_first_node(const ir_nodeRef& tn, const std::string& f_name) const;

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
    */
   void connect_start_nodes(const std::unique_ptr<operations_graph_constructor>& ogc, const std::string& next);

   /**
    * Builds recursively the operation for a given IR node. We assume a one to one mapping between nodeids and
    * vertices
    * @param TM is the IR manager.
    * @param ogc is the operation graph constructor used to add the vertices.
    * @param tn is the reference of the IR node we are currently analyzing
    * @param f_name is the name of the function which the node we are analyzing belongs to
    * @param bb_index is the basic block index of the operation being built
    */
   void build_operation_recursive(const ir_managerRef TM, const std::unique_ptr<operations_graph_constructor>& ogc,
                                  const ir_nodeRef tn, const std::string& f_name, unsigned int bb_index);

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   operations_cfg_computation(const ParameterConstRef _parameters, const application_managerRef AppM,
                              unsigned int function_id, const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;

   void Initialize() override;
};

#endif
