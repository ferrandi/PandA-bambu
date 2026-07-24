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
 *   Copyright (C) 2025-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file BasicBlockReachability.cpp
 * @brief Unit tests for Basic Block Reachability
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "algorithms/reachability/BasicBlockReachability.hpp"

#include "cdfg_edge_info.hpp"
#include "graph.hpp"

#include <boost/test/unit_test.hpp>

using BulkGraph = graphs_collection<>;
using FilteredGraph = graph<BulkGraph>;

BOOST_AUTO_TEST_CASE(Reachability_LinearPath)
{
   BulkGraph bulk;
   const auto s = bulk.AddVertex();
   const auto t = bulk.AddVertex();
   bulk.AddEdge(s, t, CFG_SELECTOR);

   const FilteredGraph cfg(bulk, CFG_SELECTOR);
   BOOST_CHECK(reachability::HasPath(cfg, s, t));
   BOOST_CHECK(!reachability::HasPath(cfg, t, s));
}

BOOST_AUTO_TEST_CASE(Reachability_Cycle)
{
   BulkGraph bulk;
   const auto header = bulk.AddVertex();
   const auto latch = bulk.AddVertex();
   bulk.AddEdge(header, latch, CFG_SELECTOR);
   bulk.AddEdge(latch, header, CFG_SELECTOR);

   const FilteredGraph cfg(bulk, CFG_SELECTOR);
   BOOST_CHECK(reachability::HasPath(cfg, header, latch));
   BOOST_CHECK(reachability::HasCycleThrough(cfg, header));
}

BOOST_AUTO_TEST_CASE(Reachability_FeedbackEdge)
{
   BulkGraph bulk;
   const auto entry = bulk.AddVertex();
   const auto loop_header = bulk.AddVertex();
   bulk.AddEdge(entry, loop_header, CFG_SELECTOR);
   bulk.AddEdge(loop_header, entry, FB_CFG_SELECTOR);

   const FilteredGraph fcfg(bulk, FCFG_SELECTOR);
   BOOST_CHECK(reachability::HasPath(fcfg, entry, loop_header));
   BOOST_CHECK(reachability::HasCycleThrough(fcfg, entry));
}
