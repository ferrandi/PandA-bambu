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
 * @file design_flow_graph.hpp
 * @brief Classes to describe design flow graph
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef DESIGN_FLOW_GRAPH_HPP
#define DESIGN_FLOW_GRAPH_HPP

#include "edge_info.hpp"  // for EdgeInfo, EdgeIn...
#include "graph.hpp"      // for vertex, EdgeDesc...
#include "graph_info.hpp" // for GraphInfo
#include "node_info.hpp"  // for NodeInfo
#include "refcount.hpp"   // for refcount, Refcou...
#include <cstddef>        // for size_t
#include <iosfwd>         // for ostream
#include <string>         // for string

#include "custom_map.hpp" // for unordered_map

CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(DesignFlowGraphsCollection);
REF_FORWARD_DECL(DesignFlowStep);
enum class DesignFlowStep_Status;
class SdfGraph;

struct DesignFlowStepInfo : public NodeInfo
{
 public:
   /// The step corresponding to a vertex
   const DesignFlowStepRef design_flow_step;

   /// Status of a step
   DesignFlowStep_Status status;

   /**
    * Constructor
    */
   DesignFlowStepInfo(const DesignFlowStepRef step_ref, const bool unnecessary);
};
typedef refcount<DesignFlowStepInfo> DesignFlowStepInfoRef;
typedef refcount<const DesignFlowStepInfo> DesignFlowStepInfoConstRef;

struct DesignFlowDependenceInfo : public EdgeInfo
{
 public:
   /**
    * Constructor
    */
   DesignFlowDependenceInfo();

   /**
    * Desturctor
    */
   ~DesignFlowDependenceInfo() override;
};
typedef refcount<DesignFlowDependenceInfo> DesignFlowDependenceInfoRef;
typedef refcount<const DesignFlowDependenceInfo> DesignFlowDependenceInfoConstRef;

struct DesignFlowGraphInfo : public GraphInfo
{
 public:
   /// The entry vertex of the graph
   vertex entry;

   /// The exit vertex of the graph
   vertex exit;
};
typedef refcount<DesignFlowGraphInfo> DesignFlowGraphInfoRef;
typedef refcount<const DesignFlowGraphInfo> DesignFlowGraphInfoConstRef;

class DesignFlowGraphsCollection : public graphs_collection
{
 protected:
   /// Map a signature of a step to the corresponding vertex
   CustomUnorderedMap<std::string, vertex> signature_to_vertex;

 public:
   /**
    * Constructor
    */
   explicit DesignFlowGraphsCollection(const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~DesignFlowGraphsCollection() override;

   /**
    * Return the vertex associated with a design step if exists, NULL_VERTEX otherwise
    * @param signature is the signature of the design step
    */
   vertex GetDesignFlowStep(const std::string& signature) const;

   /**
    * Add a design flow dependence
    * @param source is the source of the dependence
    * @param target is the target of the dependence
    */
   inline EdgeDescriptor AddDesignFlowDependence(const vertex source, const vertex target, const int selector)
   {
      if(ExistsEdge(source, target))
         return AddSelector(source, target, selector);
      else
         return InternalAddEdge(source, target, selector, EdgeInfoRef(new DesignFlowDependenceInfo()));
   }

   /**
    * Add a design step
    * @param design_flow_step is the step to be added
    * @param unnecessary specifiy is the step is necessary or not
    */
   vertex AddDesignFlowStep(const DesignFlowStepRef design_flow_step, const bool unnecessary);
};
typedef refcount<DesignFlowGraphsCollection> DesignFlowGraphsCollectionRef;
typedef refcount<const DesignFlowGraphsCollection> DesignFlowGraphsCollectionConstRef;

class DesignFlowGraph : public graph
{
 public:
   /// The dependence selector
   static const int DEPENDENCE_SELECTOR;

   /// The condition selector
   static const int PRECEDENCE_SELECTOR;

   /// The auxiliary selector
   static const int AUX_SELECTOR;

   /// The dependence feedback selector
   static const int DEPENDENCE_FEEDBACK_SELECTOR;

   /**
    * The type of view of design flow graph
    */
   enum Type
   {
      ALL, /**< Graph with all the edges */
   };

   /**
    * Constructor
    * @param design_flow_graphs_collection is the graph collection
    * @param selector is the selector used in this view
    */
   DesignFlowGraph(const DesignFlowGraphsCollectionRef design_flow_graphs_collection, const int _selector);

   /**
    * Destructor
    */
   ~DesignFlowGraph() override;

   /**
    * Return the vertex associated with a design step if exists, NULL_VERTEX otherwise
    * @param signature is the signature of the design step
    */
   vertex GetDesignFlowStep(const std::string& signature) const;

   /**
    * @param step is the vertex
    * @return the info associated with the vertex
    */
   inline DesignFlowStepInfoRef GetDesignFlowStepInfo(const vertex step)
   {
      return RefcountCast<DesignFlowStepInfo>(graph::GetNodeInfo(step));
   }

   /**
    * Return the info associated with a step
    * @param step is the vertex
    * @return the info associated with the vertex
    */
   inline const DesignFlowStepInfoConstRef CGetDesignFlowStepInfo(const vertex step) const
   {
      return RefcountCast<const DesignFlowStepInfo>(graph::CGetNodeInfo(step));
   }

   /**
    * Return the info associated with the graph
    * @return the info associated with the graph
    */
   inline DesignFlowGraphInfoRef GetDesignFlowGraphInfo()
   {
      return RefcountCast<DesignFlowGraphInfo>(graph::GetGraphInfo());
   }

   /**
    * Return the info associated with the graph
    * @return the info associated with the graph
    */
   inline const DesignFlowGraphInfoConstRef CGetDesignFlowGraphInfo() const
   {
      return RefcountCast<const DesignFlowGraphInfo>(graph::CGetGraphInfo());
   }

   /**
    * Write this graph in dot format
    * @param file_name is the file where the graph has to be printed
    * @param detail_level is the detail level of the printed graph
    */
   void WriteDot(const std::string& file_name, const int detail_level = 0) const;

#ifndef NDEBUG
   /**
    * Write this graph in dot format considering situation during a given iteration
    * @param file_name is the file where graph has to be printed
    * @param vertex_history tells which vertices are present in each iteration
    * @param edge_history tells which edges are present in each iteration
    * @param vertex_names is the name of each vertex (name of old vertices could be not more computable)
    */
   void WriteDot(const std::string& file_name, const CustomMap<size_t, CustomMap<vertex, DesignFlowStep_Status>>& vertex_history, const CustomMap<size_t, CustomUnorderedMapStable<EdgeDescriptor, int>>& edge_history,
                 const CustomMap<vertex, std::string>& vertex_names, const size_t writing_step_counter) const;
#endif
};
typedef refcount<DesignFlowGraph> DesignFlowGraphRef;
typedef refcount<const DesignFlowGraph> DesignFlowGraphConstRef;

/**
 * Functor used to write the content of the design flow step to dotty file
 */
class DesignFlowStepWriter : public VertexWriter
{
 private:
   /// Actors which have to be printed (empty means all)
   const CustomMap<vertex, DesignFlowStep_Status>& vertex_history;

   /// The name of the actors (when they cannot be taken from graph
   const CustomMap<vertex, std::string>& actor_names;

 public:
   /**
    * Constructor
    * @param design_flow_graph is the graph to be printed
    * @param vertex_history are the vertices which have to be printed
    * @param detail_level is the detail level
    */
   DesignFlowStepWriter(const DesignFlowGraph* design_flow_graph, const CustomMap<vertex, DesignFlowStep_Status>& vertex_history = CustomMap<vertex, DesignFlowStep_Status>(),
                        const CustomMap<vertex, std::string>& actor_names = CustomMap<vertex, std::string>(), const int detail_level = 0);

   /**
    * Destructor
    */
   ~DesignFlowStepWriter() override;

   /**
    * Functor actually called by the boost library to perform the writing
    * @param out is the stream where the nodes have to be printed
    * @param v is the vertex to be printed
    */
   void operator()(std::ostream& out, const vertex& v) const override;
};

/**
 * Functor used to write the content of the design flow edge to dotty file
 */
class DesignFlowEdgeWriter : public EdgeWriter
{
 private:
   /// Actors which have to be printed (empty means all)
   const CustomMap<vertex, DesignFlowStep_Status>& vertex_history;

   /// Edges which have to be printed (empty means all)
   const CustomUnorderedMapStable<EdgeDescriptor, int>& edge_history;

 public:
   /**
    * Constructor
    * @param design_flow_graph is the graph to be printed
    * @param vertex_history are the vertices which have to be printed
    * @param edge_history are the edges which have to be printed
    * @param detail_level is the detail level
    */
   DesignFlowEdgeWriter(const DesignFlowGraph* design_flow_graph, const CustomMap<vertex, DesignFlowStep_Status>& vertex_history = CustomMap<vertex, DesignFlowStep_Status>(),
                        const CustomUnorderedMapStable<EdgeDescriptor, int>& edge_history = CustomUnorderedMapStable<EdgeDescriptor, int>(), const int detail_level = 0);

   /**
    * Functor actually called by the boost library to perform the writing
    * @param out is the stream where the edges have to be printed
    * @param edge is the edge to be printed
    */
   void operator()(std::ostream& out, const EdgeDescriptor& edge) const override;
};

#endif
