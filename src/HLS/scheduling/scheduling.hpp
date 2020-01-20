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
 * @file scheduling.hpp
 * @brief Base class for all scheduling algorithms.
 *
 * This class is a pure virtual one, that has to be specilized in order to implement a particular scheduling algorithm.
 * It has a internal attribute to check if a vertex is speculate. To specialize this class into an algorithm
 *
 * @author Matteo Barbati <mbarbati@gmail.com>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef SCHEDULING_HPP
#define SCHEDULING_HPP

#include "custom_map.hpp"

#include "graph.hpp"
#include "hls_function_step.hpp"
#include "strong_typedef.hpp"

CONSTREF_FORWARD_DECL(OpGraph);
REF_FORWARD_DECL(DesignFlowStep);
UINT_STRONG_TYPEDEF_FORWARD_DECL(ControlStep);

/**
 * Generic class managing scheduling algorithms.
 */
class Scheduling : public HLSFunctionStep
{
 private:
   /**
    * @name switch statements data structures and functions.
    */
   //@{
   /// store for each switch the number of outgoing branches.
   CustomUnorderedMap<vertex, unsigned int> switch_map_size;

   /// for each controlling vertex, it defines a relation between switch tags and branch tags
   CustomUnorderedMap<vertex, CustomUnorderedMapUnstable<unsigned int, unsigned int>> switch_normalizing_map;
   //@}

 protected:
   /// Map for speculation property of each operation vertex. If true, it means that vertex is speculative executed,
   /// false otherwise
   CustomUnorderedMap<vertex, bool> spec;

   /// flag to check speculation
   const bool speculation;

   /**
    * compute the branch tag given the edge and the control data flow graph.
    * @param e is the edge.
    * @param cdg is the controlling dependency graph.
    * @param switch_set is filled with the switch tags in case the source of e is a switch vertex.
    */
   unsigned int compute_b_tag(const EdgeDescriptor& e, const OpGraphConstRef cdg, CustomOrderedSet<unsigned int>::const_iterator& switch_it, CustomOrderedSet<unsigned int>::const_iterator& switch_it_end) const;

   /**
    * Return the number of branches associated with the controlling vertex.
    * @param cdg is the control dependency graph.
    * @param controlling_vertex is the controlling vertex.
    */
   unsigned int compute_b_tag_size(const OpGraphConstRef cdg, vertex controlling_vertex) const;

   /**
    * return an unsigned int from 0 to compute_b_tag_size(controlling_vertex)-1 given a branch tag not normalized.
    * @param controlling_vertex is the controlling vertex (used only for switch vertex).
    * @param b_tag_not_normalized is the non normalized tag.
    */
   unsigned int b_tag_normalize(vertex controlling_vertex, unsigned int b_tag_not_normalized) const;

   /**
    * initialize the maps associated with the switch vertices.
    * @param controlling_vertex is a switch vertex.
    * @param cdg is the control dependency graph.
    */
   void init_switch_maps(vertex controlling_vertex, const OpGraphConstRef cdg);

   /**
    * Move up zero time operations in the previous step when possible
    * @param HLS is the structure storing all the High level synthesis information
    * @param dependence_graph is the graph to be scheduled
    * @return the new last step in which an operation is scheduled
    */
   ControlStep anticipate_operations(const OpGraphConstRef dependence_graph);

   /**
    * Compute the relationship of this step
    * @param relationship_type is the type of relationship to be considered
    * @return the steps in relationship with this
    */
   const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    * @param hls_flow_step_type is the type of scheduling
    */
   Scheduling(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type hls_flow_step_type,
              const HLSFlowStepSpecializationConstRef hls_flow_step_specialization = HLSFlowStepSpecializationConstRef());

   /**
    * Destructor.
    */
   ~Scheduling() override;

   /**
    * It returns speculation property map
    * @return the map associated
    */
   const CustomUnorderedMap<vertex, bool>& get_spec() const
   {
      return spec;
   }

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;
};
#endif
