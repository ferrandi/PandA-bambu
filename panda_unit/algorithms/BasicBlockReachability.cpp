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
 *                Copyright (C) 2025-2026 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License with BAMBU exceptions, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   A copy of the License can be found in the root directory of this repository.
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
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
