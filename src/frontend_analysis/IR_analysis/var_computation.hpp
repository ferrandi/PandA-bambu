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
 * @file var_computation.hpp
 * @brief Analyzes operations and creates the sets of read and written variables
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef VAR_COMPUTATION_HPP
#define VAR_COMPUTATION_HPP
#include "function_frontend_flow_step.hpp"

#include "graph.hpp"
#include "refcount.hpp"

class node_stmt;
class operations_graph_constructor;
CONSTREF_FORWARD_DECL(ir_node);
enum class VariableAccessType;

/**
 *
 */
class VarComputation : public FunctionFrontendFlowStep
{
   /**
    * Recursively analyze an ir_node
    * @param op_vertex is the vertex to which the statement where ir_node is inclued belongs
    * @param ir_node is the IR node to be examined
    * @param access_type is the type of the access
    * @param ogc is the operations graph constructor used to record dependencies
    */
   void RecursivelyAnalyze(gc_vertex_descriptor op_vertex, const ir_nodeConstRef& ir_node,
                           const VariableAccessType access_type,
                           const std::unique_ptr<operations_graph_constructor>& ogc) const;

   /**
    * Analyze virtual operands associated with a node statement
    * @param op_vertex is the vertex to which node statement belongs
    * @param vops is the set of virtual operands to be considered
    * @param ogc is the operations graph constructor used to record dependencies
    */
   void AnalyzeVops(gc_vertex_descriptor op_vertex, const node_stmt* vops,
                    const std::unique_ptr<operations_graph_constructor>& ogc) const;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   VarComputation(const ParameterConstRef _parameters, const application_managerRef AppM, unsigned int function_id,
                  const DesignFlowManager& design_flow_manager);

   void Initialize() override;

   DesignFlowStep_Status InternalExec() override;
};

#endif
