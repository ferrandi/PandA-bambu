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
 * @file cdfg_edge_info.hpp
 * @brief Data structures used to represent an edge in operation and basic block graphs.
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef CDFG_EDGE_INFO_HPP
#define CDFG_EDGE_INFO_HPP

#include "custom_map.hpp" // for map
#include "custom_set.hpp" // for set
#include "edge_info.hpp"  // for EdgeInfo
#include "refcount.hpp"   // for CONSTREF_FORWARD...
#include <limits>         // for numeric_limits
#include <string>         // for string

CONSTREF_FORWARD_DECL(BehavioralHelper);

/**
 * Constants identifying the type of the edges in both operation and basic block graphs
 */

/// Transitive reducted edge selector
#define TRED_SELECTOR (1 << 0)

/// Control flow graph edge selector
#define CFG_SELECTOR (1 << 1)
/// Feedback control flow edge selector
#define FB_CFG_SELECTOR (1 << 2)
/// Control flow graph with feedback edges
#define FCFG_SELECTOR (CFG_SELECTOR | FB_CFG_SELECTOR)

/// Control dependence edge selector
#define CDG_SELECTOR (1 << 3)
/// Feedback control dependence edge selector
#define FB_CDG_SELECTOR (1 << 4)
/// Control dependence graph selector with feedback edges
#define FCDG_SELECTOR (CDG_SELECTOR | FB_CDG_SELECTOR)

/// Extended control flow graph selector
#define ECFG_SELECTOR (1 << 5)

/// constant used to represent control edges representing a true edge of a conditional statement.
#define T_COND (std::numeric_limits<unsigned int>::max())
/// constant used to represent control edges representing a false edge of a conditional statement.
#define F_COND (std::numeric_limits<unsigned int>::max() - 1)
/// constant used to represent control edges representing a standard control edge.
#define NO_COND (std::numeric_limits<unsigned int>::max() - 2)
/// constant used to represent label "default" of a switch construct
#define default_COND (std::numeric_limits<unsigned int>::max() - 3)

/**
 * Information associated with an operation or basic block graph
 */
struct CdfgEdgeInfo : public EdgeInfo
{
 protected:
   /// edge labels; key is the selector
   std::map<int, CustomOrderedSet<unsigned int>> labels;

 public:
   /// Constructor
   CdfgEdgeInfo() = default;

   /**
    * Function returning true when the edge is a then control dependence edge
    */
   bool CdgEdgeT() const;

   /**
    * Function returning true when the edge is an else control dependence edge
    */
   bool CdgEdgeF() const;

   /**
    * Function returning true when the edge is a then control flow edge
    */
   bool CfgEdgeT() const;

   /**
    * Function returning true when the edge is an else control flow edge
    */
   bool CfgEdgeF() const;

   /**
    * Return true if it is an edge associated with a switch
    */
   bool Switch() const;

   /**
    * Add a nodeID of type type to this edge_info
    * @param nodeID is the nodeID
    * @param type is the type
    */
   void add_nodeID(unsigned int nodeID, const int type);

   /**
    * Return the nodeID of type type associated with this edge information
    * @param type is the type
    * @return the nodeID of type type
    */
   const CustomOrderedSet<unsigned int>& get_nodeID(const int type) const;

   /**
    * Return the string of the labels associated with the edge
    * @param type is the type of labels to be printed
    * @param BH is the helper used to print the labels
    */
   const std::string PrintLabels(const int type, const BehavioralHelperConstRef BH) const;
};

/**
 * check if the edge is a then control dependence edge
 * @param data is the graph.
 * @param edge_index is the index of the cdfg edge.
 */
#define CDG_TRUE_CHECK(data, edge_index) Cget_edge_info<CdfgEdgeInfo>(edge_index, *(data)) and Cget_edge_info<CdfgEdgeInfo>(edge_index, *(data))->CdgEdgeT()

/**
 * check if the edge is a else control dependence edge
 * @param data is the graph.
 * @param edge_index is the index of the cdfg edge.
 */
#define CDG_FALSE_CHECK(data, edge_index) Cget_edge_info<CdfgEdgeInfo>(edge_index, *(data)) and Cget_edge_info<CdfgEdgeInfo>(edge_index, *(data))->CdgEdgeF()

/**
 * check if the edge is a then control flow edge
 * @param data is the graph.
 * @param edge_index is the index of the cdfg edge.
 */
#define CFG_TRUE_CHECK(data, edge_index) Cget_edge_info<CdfgEdgeInfo>(edge_index, *(data)) and Cget_edge_info<CdfgEdgeInfo>(edge_index, *(data))->CfgEdgeT()

/**
 * check if the edge is a else control flow edge
 * @param data is the graph.
 * @param edge_index is the index of the cdfg edge.
 */
#define CFG_FALSE_CHECK(data, edge_index) Cget_edge_info<CdfgEdgeInfo>(edge_index, *(data)) and Cget_edge_info<CdfgEdgeInfo>(edge_index, *(data))->CfgEdgeF()

/**
 * check if the edge is a then flow edge
 * @param data is the graph.
 * @param edge_index is the index of the cdfg edge.
 */
#define FLG_TRUE_CHECK(data, edge_index) Cget_edge_info<CdfgEdgeInfo>(edge_index, *(data)) and Cget_edge_info<CdfgEdgeInfo>(edge_index, *(data))->FlgEdgeT()

/**
 * check if the edge is a else flow edge
 * @param data is the graph.
 * @param edge_index is the index of the cdfg edge.
 */
#define FLG_FALSE_CHECK(data, edge_index) Cget_edge_info<CdfgEdgeInfo>(edge_index, *(data)) and Cget_edge_info<CdfgEdgeInfo>(edge_index, *(data))->FlgEdgeF()

/**
 * Helper macro returning the NodeID of a certain type associated with an edge.
 * @param data is the graph.
 * @param edge_index is the index of the cdfg edge.
 * @param type is the type of wanted nodeID
 */
#define EDGE_GET_NODEID(data, edge_index, type) Cget_edge_info<CdfgEdgeInfo>(edge_index, *(data))->get_nodeID(type)

#endif
