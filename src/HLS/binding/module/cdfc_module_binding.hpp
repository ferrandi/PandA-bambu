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
 * @file cdfc_module_binding.hpp
 * @brief Module binding based on the analysis of the control data flow chained graph
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef CDFC_MODULE_BINDING_HPP
#define CDFC_MODULE_BINDING_HPP
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "fu_binding_creator.hpp"
#include "graph_facade.hpp"
#include "hash_helper.hpp"
#include "op_graph.hpp"

#include <deque>
#include <string>
#include <utility>
#include <vector>

//#define HC_APPROACH

class OpGraph;
class OpVertexSet;
CONSTREF_FORWARD_DECL(AllocationInformation);
CONSTREF_FORWARD_DECL(fu_binding);
CONSTREF_FORWARD_DECL(Parameter);
enum class CliqueCovering_Algorithm;
REF_FORWARD_DECL(fu_binding);
struct spec_hierarchical_clustering;

class CDFCModuleBindingSpecialization : public HLSFlowStepSpecialization
{
 public:
   /// The cdfc module binding algorithm
   const CliqueCovering_Algorithm clique_covering_algorithm;

   /**
    * Constructor
    * @param clique_covering_algorithm is the algorithm to be used
    */
   explicit CDFCModuleBindingSpecialization(const CliqueCovering_Algorithm clique_covering_algorithm);

   std::string GetName() const override;

   context_t GetSignatureContext() const override;
};

struct CdfcVertexInfo
{
};

struct CdfcEdgeInfo
{
   /// edge weight
   int weight{0};

   CdfcEdgeInfo() = default;

   /**
    * Constructor with selector
    * @param _weight is the weight to be associated with the edge
    */
   CdfcEdgeInfo(int _weight) : weight(_weight)
   {
   }
};

/// CDFC graphs use the shared PandA graph layer so storage/backend choices stay localized.
using cdfc_graph_storage_policy = append_only_vec_graph_storage;
using cdfc_graph_collection =
    graphs_collection<CdfcVertexInfo, CdfcEdgeInfo, graph_no_property, cdfc_graph_storage_policy>;
using cdfc_graph = graph<cdfc_graph_collection>;

/// vertex definition
using cdfc_vertex_descriptor = graph_vertex_descriptor_t<cdfc_graph>;
/// edge definition.
using cdfc_edge_descriptor = graph_edge_descriptor_t<cdfc_graph>;

/// connection code can be
/// no_def - source is not defined because is a parameter or a constan value
///          in this case only the ir_var of the source is relevant
/// no_phi_chained - the source vertex has a defining operation (def_op) that is chained with op_vertex
///          in addition to the ir_var here we have to store even the def_op vertex
/// no_phi_no_chained - the source vertex has a defining operation that is not chained with op_vertex
///          here we have to store the def_op vertex and the storage value id
/// phi - the source vertex is a phi
///          here we need the storage value id and the def_op vertex
enum conn_code
{
   no_def,
   no_phi_chained,
   no_phi_no_chained,
   phi
};

/// put into relation an operation vertex with its sources
/// op vertex -> vector of port index -> set of pair < conn code, pair of < ir_var/storage_value, null/vertex> >
using connection_relation = CustomUnorderedMap<
    OpGraph::vertex_descriptor,
    std::vector<CustomOrderedSet<std::pair<conn_code, std::pair<unsigned int, OpGraph::vertex_descriptor>>>>>;

/**
 * Class managing the module allocation.
 */
class cdfc_module_binding : public fu_binding_creator
{
 protected:
   /// Threshold used in sharing of functional units
   const double small_normalized_resource_area;

   bool false_loop_search(cdfc_vertex_descriptor start, unsigned k, const cdfc_graph& cdfc, const cdfc_graph& cg,
                          std::deque<cdfc_edge_descriptor>& candidate_edges);
   bool false_loop_search_cdfc_1(cdfc_vertex_descriptor src, unsigned int level, unsigned k,
                                 cdfc_vertex_descriptor start, const cdfc_graph& cdfc, const cdfc_graph& cg,
                                 std::deque<cdfc_edge_descriptor>& candidate_edges, std::vector<bool>& visited,
                                 std::vector<bool>& cg_visited, std::vector<bool>& cdfc_visited);
   bool false_loop_search_cdfc_more(cdfc_vertex_descriptor src, unsigned int level, unsigned k,
                                    cdfc_vertex_descriptor start, const cdfc_graph& cdfc, const cdfc_graph& cg,
                                    std::deque<cdfc_edge_descriptor>& candidate_edges, std::vector<bool>& visited,
                                    std::vector<bool>& cg_visited, std::vector<bool>& cdfc_visited);
   bool can_be_clustered(OpGraph::vertex_descriptor v, const OpGraph& fsdg, const fu_bindingConstRef fu,
                         const CustomUnorderedMap<OpGraph::vertex_descriptor, double>& slack_time,
                         const double mux_time);

   int weight_computation(bool cond1, bool cond2, OpGraph::vertex_descriptor v1, OpGraph::vertex_descriptor v2,
                          const double mux_time, const OpGraph& fdfg, const fu_bindingConstRef fu,
                          const CustomUnorderedMap<OpGraph::vertex_descriptor, double>& slack_time,
                          CustomUnorderedMap<OpGraph::vertex_descriptor, double>& starting_time,
#ifdef HC_APPROACH
                          spec_hierarchical_clustering& hc,
#endif
                          connection_relation& con_rel, double controller_delay, unsigned long long prec);

   void update_slack_starting_time(const OpGraph& fdfg, OpVertexSet& sorted_vertices,
                                   CustomUnorderedMap<OpGraph::vertex_descriptor, double>& slack_time,
                                   CustomUnorderedMap<OpGraph::vertex_descriptor, double>& starting_time,
                                   bool update_starting_time, bool only_backward, bool only_forward);

   void initialize_connection_relation(connection_relation& con_rel, const OpVertexSet& all_candidate_vertices);

   static const int CD_EDGE = 1;
   static const int COMPATIBILITY_EDGE = 2;

   /// record if a vertex has to be clustered or not
   CustomUnorderedMap<OpGraph::vertex_descriptor, bool> can_be_clustered_table;
   CustomUnorderedMapUnstable<std::pair<OpGraph::vertex_descriptor, unsigned int>, bool> is_complex;

 public:
   cdfc_module_binding(const ParameterConstRef _parameters, const HLS_managerRef HLSMgr, unsigned int funId,
                       const DesignFlowManager& design_flow_manager,
                       const HLSFlowStepSpecializationConstRef hls_flow_step_specialization);

   DesignFlowStep_Status InternalExec() override;
};

#endif
