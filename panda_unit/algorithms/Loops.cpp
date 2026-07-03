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
 * @file Loops.cpp
 * @brief Unit tests for loop detection using templated Loops class
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */

#include "algorithms/loops_detection/loops.hpp"
#include "SemiNCADominance.hpp"
#include "dbgPrintHelper.hpp"
#include "graph.hpp"

#include <boost/graph/adjacency_list.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Simple graph type for testing
using BaseGraph = boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS>;
using TestGraph = graph_base<BaseGraph>;

template <typename GraphT>
auto makeDominates(const dominance<GraphT>& domTree)
{
   return [&domTree](auto maybeDom, auto node) {
      if(maybeDom == node)
      {
         return true;
      }

      auto current = node;
      while(true)
      {
         const auto idom = domTree.getImmediateDominator(current);
         if(idom == maybeDom)
         {
            return true;
         }
         if(idom == current)
         {
            return false;
         }
         current = idom;
         if(current == maybeDom)
         {
            return true;
         }
      }
   };
}

struct TestGraphTraits
{
   using GraphType = TestGraph;
   using Vertex = TestGraph::vertex_descriptor;

   static unsigned loopId(const TestGraph&, Vertex v)
   {
      return static_cast<unsigned>(v);
   }

   static std::string nodeLabel(const TestGraph&, Vertex v)
   {
      return std::string("N") + std::to_string(static_cast<unsigned>(v));
   }
};

using TestLoops = LoopsTemplate<TestGraph, TestGraphTraits>;

static std::vector<TestLoops::LoopConstRefType> collectNaturalLoops(const TestLoops& loops)
{
   std::vector<TestLoops::LoopConstRefType> result;
   if(loops.getList().empty())
   {
      return result;
   }
   const auto zero_id = loops.getList().front()->getLoopId();
   for(const auto& loop : loops.getList())
   {
      if(loop->getLoopId() == zero_id)
      {
         continue;
      }
      result.push_back(loop);
   }
   return result;
}

BOOST_AUTO_TEST_CASE(loop_simple_backedge)
{
   TestGraph g;
   auto entry = boost::add_vertex(g);
   auto H = boost::add_vertex(g);
   auto Body = boost::add_vertex(g);
   auto exit = boost::add_vertex(g);

   boost::add_edge(entry, H, g);
   boost::add_edge(H, Body, g);
   boost::add_edge(Body, H, g);
   boost::add_edge(H, exit, g);

   dominance<TestGraph> dom(g, entry, exit);
   auto dominates = makeDominates(dom);
   BOOST_CHECK(dominates(H, Body));

   std::unique_ptr<TestLoops> loopsPtr;

   loopsPtr = std::make_unique<TestLoops>(g, entry, dom);

   TestLoops& loops = *loopsPtr;
   const auto natural = collectNaturalLoops(loops);
   BOOST_REQUIRE_EQUAL(natural.size(), 1U);
   const auto& loop = natural.front();
   BOOST_CHECK(loop->isReducible());
   BOOST_CHECK_EQUAL(loop->getHeader(), H);
   BOOST_CHECK(loop->getBlocks().count(Body) == 1U);

   const auto& latches = loop->getBackEdges();
   BOOST_REQUIRE_EQUAL(latches.size(), 1U);
   BOOST_CHECK(latches.find({Body, H}) != latches.end());
}

BOOST_AUTO_TEST_CASE(loop_nested)
{
   TestGraph g;
   auto entry = boost::add_vertex(g);
   auto OuterH = boost::add_vertex(g);
   auto InnerH = boost::add_vertex(g);
   auto InnerB = boost::add_vertex(g);
   auto OuterB = boost::add_vertex(g);
   auto exit = boost::add_vertex(g);

   boost::add_edge(entry, OuterH, g);
   boost::add_edge(OuterH, InnerH, g);
   boost::add_edge(InnerH, InnerB, g);
   boost::add_edge(InnerB, InnerH, g);
   boost::add_edge(InnerH, OuterB, g);
   boost::add_edge(OuterB, OuterH, g);
   boost::add_edge(OuterH, exit, g);

   dominance<TestGraph> dom(g, entry, exit);
   auto dominates = makeDominates(dom);
   BOOST_CHECK(dominates(InnerH, InnerB));
   BOOST_CHECK(dominates(OuterH, OuterB));

   BOOST_REQUIRE_NO_THROW(TestLoops(g, entry, dom));
   TestLoops loops(g, entry, dom);
   const auto natural = collectNaturalLoops(loops);
   BOOST_REQUIRE_EQUAL(natural.size(), 2U);

   const auto findByHeader = [&](TestGraph::vertex_descriptor header) {
      for(const auto& loop : natural)
      {
         if(loop->getHeader() == header)
         {
            return loop;
         }
      }
      return TestLoops::LoopConstRefType();
   };

   const auto innerLoop = findByHeader(InnerH);
   BOOST_REQUIRE(innerLoop);
   BOOST_CHECK(innerLoop->isReducible());
   BOOST_CHECK(innerLoop->getBlocks().count(InnerB) == 1U);

   const auto outerLoop = findByHeader(OuterH);
   BOOST_REQUIRE(outerLoop);
   BOOST_CHECK(outerLoop->isReducible());
   BOOST_CHECK(outerLoop->getBlocks().count(OuterB) == 1U);
}

BOOST_AUTO_TEST_CASE(loop_no_backedge)
{
   TestGraph g;
   auto entry = boost::add_vertex(g);
   auto N1 = boost::add_vertex(g);
   auto N2 = boost::add_vertex(g);
   auto exit = boost::add_vertex(g);

   boost::add_edge(entry, N1, g);
   boost::add_edge(N1, N2, g);
   boost::add_edge(N2, exit, g);

   dominance<TestGraph> dom(g, entry, exit);
   auto dominates = makeDominates(dom);

   int backedge_count = 0;
   for(const auto& edge : g.edges())
   {
      if(dominates(g.target(edge), g.source(edge)))
      {
         ++backedge_count;
      }
   }
   BOOST_CHECK_EQUAL(backedge_count, 0);

   BOOST_REQUIRE_NO_THROW(TestLoops(g, entry, dom));
   TestLoops loops(g, entry, dom);
   const auto natural = collectNaturalLoops(loops);
   BOOST_CHECK(natural.empty());
}

BOOST_AUTO_TEST_CASE(loop_multiple_latches)
{
   TestGraph g;
   auto entry = boost::add_vertex(g);
   auto H = boost::add_vertex(g);
   auto L1 = boost::add_vertex(g);
   auto L2 = boost::add_vertex(g);
   auto exit = boost::add_vertex(g);

   boost::add_edge(entry, H, g);
   boost::add_edge(H, L1, g);
   boost::add_edge(L1, L2, g);
   boost::add_edge(L2, H, g);
   boost::add_edge(L1, H, g);
   boost::add_edge(H, exit, g);

   dominance<TestGraph> dom(g, entry, exit);

   BOOST_REQUIRE_NO_THROW(TestLoops(g, entry, dom));
   TestLoops loops(g, entry, dom);
   const auto natural = collectNaturalLoops(loops);
   BOOST_REQUIRE_EQUAL(natural.size(), 1U);
   const auto& loop = natural.front();
   BOOST_CHECK(loop->isReducible());
   BOOST_CHECK_EQUAL(loop->getHeader(), H);

   std::set<std::pair<TestGraph::vertex_descriptor, TestGraph::vertex_descriptor>> latches;
   for(const auto& edge : loop->getBackEdges())
   {
      latches.insert(edge);
   }
   BOOST_CHECK_EQUAL(latches.size(), 2U);
   BOOST_CHECK(latches.count({L1, H}) == 1U);
   BOOST_CHECK(latches.count({L2, H}) == 1U);
}

BOOST_AUTO_TEST_CASE(loop_irreducible_from_dot)
{
   TestGraph g;
   std::vector<TestGraph::vertex_descriptor> nodes;
   nodes.reserve(12);
   for(int i = 0; i <= 11; ++i)
   {
      nodes.push_back(boost::add_vertex(g));
   }

   const std::vector<std::pair<int, int>> edges = {{0, 1}, {2, 5}, {0, 3}, {3, 11}, {3, 2},  {3, 5},  {3, 6},
                                                   {3, 7}, {3, 8}, {3, 9}, {3, 10}, {3, 4},  {4, 2},  {4, 11},
                                                   {5, 6}, {6, 7}, {7, 8}, {8, 9},  {9, 10}, {10, 4}, {11, 1}};

   for(const auto& [u, v] : edges)
   {
      const auto ui = static_cast<std::size_t>(u);
      const auto vi = static_cast<std::size_t>(v);
      boost::add_edge(nodes[ui], nodes[vi], g);
   }

   dominance<TestGraph> dom(g, nodes[0], nodes[1]);

   BOOST_REQUIRE_NO_THROW(TestLoops(g, nodes[0], dom));
   TestLoops loops(g, nodes[0], dom);
   const auto natural = collectNaturalLoops(loops);
   BOOST_REQUIRE_EQUAL(natural.size(), 1U);
   const auto& loop = natural.front();
   BOOST_CHECK(!loop->isReducible());

   const std::set<TestGraph::vertex_descriptor> expected = {nodes[2], nodes[4], nodes[5], nodes[6],
                                                            nodes[7], nodes[8], nodes[9], nodes[10]};
   std::set<TestGraph::vertex_descriptor> actual(loop->getBlocks().begin(), loop->getBlocks().end());
   BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());

   const auto& back_edges = loop->getBackEdges();
   BOOST_REQUIRE_EQUAL(back_edges.size(), 1U);
   const auto& back_edge = *back_edges.begin();
   BOOST_CHECK(back_edge.first == nodes[4]);
   BOOST_CHECK(back_edge.second == nodes[2]);
}

BOOST_AUTO_TEST_CASE(loop_arith_rand2_cfg)
{
   TestGraph g;
   std::vector<TestGraph::vertex_descriptor> nodes;
   nodes.reserve(21);
   for(int i = 0; i <= 20; ++i)
   {
      nodes.push_back(boost::add_vertex(g));
   }

   const std::vector<std::pair<int, int>> edges = {
       {0, 1},   {0, 2},   {2, 3},   {3, 5},   {4, 20},  {4, 3},   {5, 7},   {5, 6},   {6, 7},
       {6, 5},   {7, 8},   {8, 10},  {8, 9},   {9, 10},  {9, 8},   {10, 4},  {10, 11}, {11, 12},
       {11, 4},  {11, 13}, {12, 14}, {13, 15}, {13, 16}, {14, 1},  {15, 16}, {15, 4},  {15, 17},
       {16, 14}, {17, 18}, {17, 19}, {18, 4},  {18, 19}, {19, 14}, {20, 14},
   };
   for(const auto& [u, v] : edges)
   {
      const auto src = static_cast<std::size_t>(u);
      const auto dst = static_cast<std::size_t>(v);
      boost::add_edge(nodes[src], nodes[dst], g);
   }

   dominance<TestGraph> dom(g, nodes[0], nodes[1]);

   TestLoops loops(g, nodes[0], dom);
   const auto natural = collectNaturalLoops(loops);
   BOOST_REQUIRE_EQUAL(natural.size(), 3U);

   const auto findLoopByHeader = [&](TestGraph::vertex_descriptor header) {
      for(const auto& loop : natural)
      {
         if(loop->isReducible() && loop->getHeader() == header)
         {
            return loop;
         }
      }
      return TestLoops::LoopConstRefType();
   };

   const auto outer = findLoopByHeader(nodes[3]);
   BOOST_REQUIRE(outer);
   const std::set<TestGraph::vertex_descriptor> expected_outer = {nodes[3],  nodes[4],  nodes[7],  nodes[10], nodes[11],
                                                                  nodes[13], nodes[15], nodes[17], nodes[18]};
   std::set<TestGraph::vertex_descriptor> actual_outer(outer->getBlocks().begin(), outer->getBlocks().end());
   BOOST_CHECK_EQUAL_COLLECTIONS(actual_outer.begin(), actual_outer.end(), expected_outer.begin(),
                                 expected_outer.end());
   BOOST_CHECK(outer->isReducible());

   const auto child_headers = [&]() {
      std::set<TestGraph::vertex_descriptor> headers;
      for(const auto& child : outer->getSubLoops())
      {
         headers.insert(child->getHeader());
      }
      return headers;
   }();
   const std::set<TestGraph::vertex_descriptor> expected_children = {nodes[5], nodes[8]};
   BOOST_CHECK_EQUAL_COLLECTIONS(child_headers.begin(), child_headers.end(), expected_children.begin(),
                                 expected_children.end());

   const auto loop_5 = findLoopByHeader(nodes[5]);
   BOOST_REQUIRE(loop_5);
   const std::set<TestGraph::vertex_descriptor> expected_loop5 = {nodes[5], nodes[6]};
   std::set<TestGraph::vertex_descriptor> actual_loop5(loop_5->getBlocks().begin(), loop_5->getBlocks().end());
   BOOST_CHECK_EQUAL_COLLECTIONS(actual_loop5.begin(), actual_loop5.end(), expected_loop5.begin(),
                                 expected_loop5.end());
   BOOST_CHECK(loop_5->isReducible());

   const auto loop_8 = findLoopByHeader(nodes[8]);
   BOOST_REQUIRE(loop_8);
   const std::set<TestGraph::vertex_descriptor> expected_loop8 = {nodes[8], nodes[9]};
   std::set<TestGraph::vertex_descriptor> actual_loop8(loop_8->getBlocks().begin(), loop_8->getBlocks().end());
   BOOST_CHECK_EQUAL_COLLECTIONS(actual_loop8.begin(), actual_loop8.end(), expected_loop8.begin(),
                                 expected_loop8.end());
   BOOST_CHECK(loop_8->isReducible());

   BOOST_CHECK(loop_5->getParent() == outer);
   BOOST_CHECK(loop_8->getParent() == outer);
}
