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
 * @file cdfg_edge_info.hpp
 * @brief Data structures used to represent an edge in operation and basic block graphs.
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef CDFG_EDGE_INFO_HPP
#define CDFG_EDGE_INFO_HPP

#include "custom_map.hpp"
#include "custom_set.hpp"
#include "edge_info.hpp"
#include "refcount.hpp"

#include <limits>
#include <string>

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

/// constant used to represent control edges representing a standard control edge.
#define NO_COND (std::numeric_limits<unsigned int>::max() - 2)
/// constant used to represent label "default" of a ifelseif construct
#define default_COND (std::numeric_limits<unsigned int>::max() - 3)

/**
 * Information associated with an operation or basic block graph
 */
struct CdfgEdgeInfo : public EdgeInfo
{
   virtual ~CdfgEdgeInfo() = default;

   /**
    * Return true if it is an edge associated with a IfElseIf
    */
   bool IfElseIf() const;

   /**
    * Add a nodeID of type type to this edge_info
    * @param nodeID is the nodeID
    * @param type is the type
    */
   void add_nodeID(unsigned int nodeID, const int type);

   /**
    * Return the nodeID of type type associated with this edge information
    * @param selector is the selector
    * @return the nodeID of type type
    */
   const CustomOrderedSet<unsigned int>& get_nodeID(const int selector) const;

   /**
    * Return the string of the labels associated with the edge
    * @param selector is the type of labels to be printed
    * @param BH is the helper used to print the labels
    */
   std::string PrintLabels(const int selector, const BehavioralHelperConstRef& BH) const;

 protected:
   /// edge labels; key is the selector
   std::map<int, CustomOrderedSet<unsigned int>> labels;
};

#endif
