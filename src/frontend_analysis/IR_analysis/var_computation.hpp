/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
