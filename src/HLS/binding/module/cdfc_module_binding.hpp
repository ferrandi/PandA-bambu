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
 * @file cdfc_module_binding.hpp
 * @brief Module binding based on the analysis of the control data flow chained graph
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @version $Revision$
 * @date $Date$
 */

#ifndef CDFC_MODULE_BINDING_HPP
#define CDFC_MODULE_BINDING_HPP

#include "fu_binding_creator.hpp"

#include "graph.hpp"

#include "hash_helper.hpp"

/// STD include
#include <string>

/// STL includes
#include <deque>
#include <utility>
#include <vector>

#include "custom_map.hpp"
#include "custom_set.hpp"

//#define HC_APPROACH

CONSTREF_FORWARD_DECL(AllocationInformation);
enum class CliqueCovering_Algorithm;
CONSTREF_FORWARD_DECL(fu_binding);
REF_FORWARD_DECL(fu_binding);
struct spec_hierarchical_clustering;
REF_FORWARD_DECL(CdfcGraph);
CONSTREF_FORWARD_DECL(OpGraph);
class OpVertexSet;
CONSTREF_FORWARD_DECL(Parameter);

class CDFCModuleBindingSpecialization : public HLSFlowStepSpecialization
{
 public:
   /// The cdfc module binding algorithm
   const CliqueCovering_Algorithm clique_covering_algorithm;

   /**
    * Constructor
    * @param cdfc_module_binding_algorithm is the algorithm to be used
    */
   explicit CDFCModuleBindingSpecialization(const CliqueCovering_Algorithm clique_covering_algorithm);

   /**
    * Return the string representation of this
    */
   const std::string GetKindText() const override;

   /**
    * Return the contribution to the signature of a step given by the specialization
    */
   const std::string GetSignature() const override;
};

/// Predicate functor object used to select the proper set of vertices
template <typename Graph>
struct cdfc_graph_vertex_selector
{
 public:
   typedef typename boost::graph_traits<Graph>::vertex_descriptor vertex_descriptor;
   typedef CustomSet<vertex_descriptor> SET_container;
   /// constructor
   cdfc_graph_vertex_selector() : all(true), support(nullptr)
   {
   }
   /// constructor
   explicit cdfc_graph_vertex_selector(const SET_container* _support) : all(false), support(_support)
   {
   }
   /// selector operator
   bool operator()(const vertex_descriptor& v) const
   {
      if(all)
         return true;
      else
         return support->find(v) != support->end();
   }

 private:
   bool all;
   const SET_container* support;
};

/// Predicate functor object used to select the proper set of edges
template <typename Graph>
struct cdfc_graph_edge_selector
{
 private:
   /// The selector associated with the filtered graph
   int selector;

   /// bulk graph
   Graph* g;

 public:
   /**
    * Constructor for filtering only on selector
    * @param _selector is the selector of the filtered graph
    * @param _g is the graph
    */
   cdfc_graph_edge_selector(const int _selector, Graph* _g) : selector(_selector), g(_g)
   {
   }

   /// all edges selector
   cdfc_graph_edge_selector() : selector(0), g(nullptr)
   {
   }

   /// edge selector operator
   template <typename Edge>
   bool operator()(const Edge& e) const
   {
      return selector & (*g)[e].selector;
   }
};

struct edge_cdfc_selector
{
 public:
   /// The selector associated with the edge
   int selector;

   /// edge weight
   int weight;

   /**
    * default constructor
    */
   edge_cdfc_selector() : selector(0), weight(0)
   {
   }

   /**
    * Constructor with selector only
    * @param _selector is the selector to be associated with the edge
    */
   explicit edge_cdfc_selector(int _selector) : selector(_selector), weight(0)
   {
   }

   /**
    * Constructor with selector
    * @param _selector is the selector to be associated with the edge
    * @param _weight is the weight to be associated with the edge
    */
   edge_cdfc_selector(int _selector, int _weight) : selector(_selector), weight(_weight)
   {
   }
};

/// bulk compatibility graph
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, boost::property<boost::vertex_index_t, std::size_t>, edge_cdfc_selector> boost_cdfc_graph;

typedef refcount<boost_cdfc_graph> boost_cdfc_graphRef;
typedef refcount<const boost_cdfc_graph> boost_cdfc_graphConstRef;

/// compatibility graph
typedef boost::filtered_graph<boost_cdfc_graph, cdfc_graph_edge_selector<boost_cdfc_graph>, cdfc_graph_vertex_selector<boost_cdfc_graph>> cdfc_graph;

/// refcount version of cdfc_graph
typedef refcount<cdfc_graph> cdfc_graphRef;
typedef refcount<const cdfc_graph> cdfc_graphConstRef;
/// vertex definition
typedef boost::graph_traits<cdfc_graph>::vertex_descriptor cdfc_vertex;
/// in_edge_iterator definition.
typedef boost::graph_traits<cdfc_graph>::in_edge_iterator cdfc_in_edge_iterator;
/// out_edge_iterator definition.
typedef boost::graph_traits<cdfc_graph>::out_edge_iterator cdfc_out_edge_iterator;
/// edge_iterator definition.
typedef boost::graph_traits<cdfc_graph>::edge_iterator cdfc_edge_iterator;
/// edge definition.
typedef boost::graph_traits<cdfc_graph>::edge_descriptor cdfc_edge;

struct CdfcEdgeInfo : public EdgeInfo
{
   const int edge_weight;

   /**
    * Constructor
    * @param edge_weight is the weight to be set
    */
   explicit CdfcEdgeInfo(const int edge_weight);
};
typedef refcount<CdfcEdgeInfo> CdfcEdgeInfoRef;
typedef refcount<const CdfcEdgeInfo> CdfcEdgeInfoConstRef;

/**
 * The info associated with a cdfc graph
 */
struct CdfcGraphInfo : public GraphInfo
{
   const std::map<vertex, vertex>& c2s;

   /// The operation graph associated with the cdfc graph
   const OpGraphConstRef operation_graph;

   /**
    * Constructor
    */
   CdfcGraphInfo(const std::map<vertex, vertex>& c2s, const OpGraphConstRef operation_graph);
};
typedef refcount<CdfcGraphInfo> CdfcGraphInfoRef;
typedef refcount<const CdfcGraphInfo> CdfcGraphInfoConstRef;

/**
 * Cdfc collection of graphs
 */
class CdfcGraphsCollection : public graphs_collection
{
 public:
   /**
    * Constructor
    * @param cdfc_graph_info is the info to be associated with the graph
    * @param parameters is the set of input parameters
    */
   CdfcGraphsCollection(const CdfcGraphInfoRef cdfc_graph_info, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~CdfcGraphsCollection() override;

   /**
    * Add an edge with a weight
    * @param source is the source of the edge
    * @param target is the target of the edge
    * @param selector is the selector to be added
    * @param weight is the weight to be set
    * @return the created edge
    */
   inline EdgeDescriptor AddEdge(const vertex source, const vertex target, const int selector, const int weight)
   {
      THROW_ASSERT(not ExistsEdge(source, target), "Trying to add an already existing edge");
      return InternalAddEdge(source, target, selector, EdgeInfoRef(new CdfcEdgeInfo(weight)));
   }

   /**
    * Add an edge with empty information associated
    * @param source is the source of the edge
    * @param target is the target of the edge
    * @param selector is the selector to be added
    * @return the created edge
    */
   inline EdgeDescriptor AddEdge(const vertex source, const vertex target, const int selector)
   {
      if(ExistsEdge(source, target))
         return AddSelector(source, target, selector);
      else
         return InternalAddEdge(source, target, selector, EdgeInfoRef(new CdfcEdgeInfo(0)));
   }
};
typedef refcount<CdfcGraphsCollection> CdfcGraphsCollectionRef;
typedef refcount<const CdfcGraphsCollection> CdfcGraphsCollectionConstRef;

/**
 * Cdfc graph
 */
class CdfcGraph : public graph
{
 public:
   /**
    * Constructor
    * @param cdfc_graphs_collection is the collections of graph to which this graph belongs
    * @param selector is the selector which identifies the edges of this graph
    */
   CdfcGraph(const CdfcGraphsCollectionRef cdfc_graphs_collection, const int selector);

   /**
    * Constructor
    * @param cdfc_graphs_collection is the collections of graph to which this graph belongs
    * @param selector is the selector which identifies the edges of this graph
    * @param vertices is the set of vertexes on which the graph is filtered.
    */
   CdfcGraph(const CdfcGraphsCollectionRef cdfc_graphs_collection, const int selector, const CustomUnorderedSet<vertex>& vertices);

   /**
    * Destructor
    */
   ~CdfcGraph() override;

   /**
    * Returns the info associated with an edge
    */
   inline const CdfcEdgeInfoConstRef CGetCdfcEdgeInfo(const EdgeDescriptor e) const
   {
      return RefcountCast<const CdfcEdgeInfo>(graph::CGetEdgeInfo(e));
   }

   /**
    * Writes this graph in dot format
    * @param file_name is the file where the graph has to be printed
    * @param detail_level is the detail level of the printed graph
    */
   void WriteDot(const std::string& file_name, const int detail_level = 0) const;
};
typedef refcount<CdfcGraph> CdfcGraphRef;
typedef refcount<const CdfcGraph> CdfcGraphConstRef;

/// connection code can be
/// no_def - source is not defined because is a parameter or a constan value
///          in this case only the tree_var of the source is relevant
/// no_phi_chained - the source vertex has a defining operation (def_op) that is chained with op_vertex
///          in addition to the tree_var here we have to store even the def_op vertex
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
/// op vertex -> vector of port index -> set of pair < conn code, pair of < tree_var/storage_value, null/vertex> >
typedef CustomUnorderedMap<vertex, std::vector<CustomOrderedSet<std::pair<conn_code, std::pair<unsigned int, vertex>>>>> connection_relation;

/**
 * Class managing the module allocation.
 */
class cdfc_module_binding : public fu_binding_creator
{
 protected:
   /// Threshold used in sharing of functional units
   const double small_normalized_resource_area;

   bool false_loop_search(cdfc_vertex start, unsigned k, const cdfc_graphConstRef& cdfc, const cdfc_graphConstRef& cg, std::deque<cdfc_edge>& candidate_edges);
   bool false_loop_search_cdfc_1(cdfc_vertex src, unsigned int level, unsigned k, cdfc_vertex start, const cdfc_graphConstRef& cdfc, const cdfc_graphConstRef& cg, std::deque<cdfc_edge>& candidate_edges, std::vector<bool>& visited,
                                 std::vector<bool>& cg_visited, std::vector<bool>& cdfc_visited);
   bool false_loop_search_cdfc_more(cdfc_vertex src, unsigned int level, unsigned k, cdfc_vertex start, const cdfc_graphConstRef& cdfc, const cdfc_graphConstRef& cg, std::deque<cdfc_edge>& candidate_edges, std::vector<bool>& visited,
                                    std::vector<bool>& cg_visited, std::vector<bool>& cdfc_visited);
   bool can_be_clustered(vertex v, OpGraphConstRef fdfg, const fu_bindingConstRef fu, const CustomUnorderedMap<vertex, double>& slack_time, const double mux_time);

   int weight_computation(bool cond1, bool cond2, vertex v1, vertex v2, const double mux_time, const OpGraphConstRef fdfg, const fu_bindingConstRef fu, const CustomUnorderedMap<vertex, double>& slack_time, CustomUnorderedMap<vertex, double>& starting_time,
#ifdef HC_APPROACH
                          spec_hierarchical_clustering& hc,
#endif
                          connection_relation& con_rel, double controller_delay, unsigned int prec);

   void update_slack_starting_time(const OpGraphConstRef sdg, OpVertexSet& sorted_vertices, CustomUnorderedMap<vertex, double>& slack_time, CustomUnorderedMap<vertex, double>& starting_time, bool update_starting_time, bool only_backward,
                                   bool only_forward);

   void initialize_connection_relation(connection_relation& con_rel, OpVertexSet& all_candidate_vertices);

   static const int CD_EDGE = 1;
   static const int COMPATIBILITY_EDGE = 2;

   /// record if a vertex has to be clustered or not
   CustomUnorderedMap<vertex, bool> can_be_clustered_table;
   CustomUnorderedMapUnstable<std::pair<vertex, unsigned int>, bool> is_complex;

 public:
   /**
    * This is the constructor of the class.
    */
   cdfc_module_binding(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStepSpecializationConstRef hls_flow_step_specialization);

   /**
    * Destructor.
    */
   ~cdfc_module_binding() override;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;
};

#endif
