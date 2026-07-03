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
 * @file Dominance.cpp
 * @brief Unit tests for dominance computation using Semi-NCA algorithm
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "SemiNCADominance.hpp"
#include "dbgPrintHelper.hpp"
#include "graph.hpp"

#include <boost/graph/adjacency_list.hpp>
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>

// Simple graph type for testing - using graph_base wrapper
using BaseGraph = boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS>;
using TestGraph = graph_base<BaseGraph>;

namespace
{
   using Edge = std::pair<unsigned, unsigned>;

   TestGraph makeGraph(unsigned vertex_count, const std::vector<Edge>& edges)
   {
      TestGraph g;
      for(unsigned i = 0; i < vertex_count; ++i)
      {
         boost::add_vertex(g);
      }
      for(const auto& [src, dst] : edges)
      {
         boost::add_edge(src, dst, g);
      }
      return g;
   }

   std::unordered_map<unsigned, unsigned> makeParentMap(const std::vector<Edge>& tree_edges)
   {
      std::unordered_map<unsigned, unsigned> parent_map;
      parent_map.reserve(tree_edges.size());
      for(const auto& [parent, child] : tree_edges)
      {
         const auto inserted = parent_map.emplace(child, parent);
         BOOST_REQUIRE_MESSAGE(inserted.second, "Duplicate parent entry for node " << child);
      }
      return parent_map;
   }

   bool dominates(const dominance<TestGraph>& dom, TestGraph::vertex_descriptor maybe_dom,
                  TestGraph::vertex_descriptor node)
   {
      if(maybe_dom == node)
      {
         return true;
      }

      auto current = node;
      while(true)
      {
         const auto idom = dom.getImmediateDominator(current);
         if(idom == maybe_dom)
         {
            return true;
         }
         if(idom == current)
         {
            return false;
         }
         current = idom;
      }
   }

   constexpr unsigned arith_rand_vertex_count = 23;
   constexpr unsigned arith_rand_entry = 0;
   constexpr unsigned arith_rand_exit = 1;

   const std::vector<Edge> arith_rand_cfg_edges = {
       {0, 1},   {0, 2},   {2, 3},   {3, 5},   {4, 22},  {4, 3},   {5, 7},   {5, 6},   {6, 7},   {6, 5},
       {7, 8},   {8, 10},  {8, 9},   {9, 10},  {9, 8},   {10, 4},  {10, 11}, {11, 12}, {11, 4},  {11, 14},
       {12, 13}, {13, 1},  {14, 15}, {14, 4},  {14, 16}, {15, 13}, {16, 17}, {16, 18}, {17, 18}, {17, 4},
       {17, 19}, {18, 13}, {19, 20}, {19, 21}, {20, 4},  {20, 21}, {21, 13}, {22, 13}};

   const std::vector<Edge> arith_rand_backedges = {
       {4, 3},
       {6, 5},
       {9, 8},
   };

   const std::vector<Edge> arith_rand_dom_tree_edges = {
       {0, 0},   {0, 1},   {0, 2},   {2, 3},   {3, 5},   {4, 22},  {5, 7},   {5, 6},
       {7, 8},   {8, 10},  {8, 9},   {10, 4},  {10, 11}, {11, 12}, {11, 14}, {14, 15},
       {14, 16}, {16, 17}, {16, 18}, {17, 19}, {19, 20}, {19, 21}, {10, 13},
   };

   const std::vector<Edge> arith_rand_post_dom_tree_edges = {
       {1, 1}, {13, 20}, {13, 11}, {1, 13}, {10, 8}, {8, 7},   {13, 21}, {7, 5}, {13, 10}, {13, 16}, {13, 15}, {13, 4},
       {7, 6}, {13, 22}, {13, 17}, {10, 9}, {1, 0},  {13, 19}, {13, 12}, {5, 3}, {13, 14}, {3, 2},   {13, 18},
   };

   constexpr unsigned arith_rand2_vertex_count = 21;
   constexpr unsigned arith_rand2_entry = 0;
   constexpr unsigned arith_rand2_exit = 1;

   const std::vector<Edge> arith_rand2_cfg_edges = {
       {0, 1},   {0, 2},   {2, 3},   {3, 5},   {4, 20},  {4, 3},   {5, 7},   {5, 6},   {6, 7},
       {6, 5},   {7, 8},   {8, 10},  {8, 9},   {9, 10},  {9, 8},   {10, 4},  {10, 11}, {11, 12},
       {11, 4},  {11, 13}, {12, 14}, {13, 15}, {13, 16}, {14, 1},  {15, 16}, {15, 4},  {15, 17},
       {16, 14}, {17, 18}, {17, 19}, {18, 4},  {18, 19}, {19, 14}, {20, 14},
   };

   const std::vector<Edge> arith_rand2_dom_tree_edges = {
       {0, 0},  {0, 1},   {0, 2},   {2, 3},   {3, 5},   {4, 20},  {5, 7},   {5, 6},   {7, 8},   {8, 10},  {8, 9},
       {10, 4}, {10, 11}, {11, 12}, {11, 13}, {13, 15}, {13, 16}, {15, 17}, {17, 18}, {17, 19}, {10, 14},
   };

   const std::vector<Edge> arith_rand2_post_dom_tree_edges = {
       {1, 1},   {7, 6},   {14, 10}, {14, 18}, {14, 15}, {3, 2}, {14, 12}, {10, 9}, {14, 16}, {1, 14},  {10, 8},
       {14, 19}, {14, 17}, {14, 11}, {8, 7},   {1, 0},   {5, 3}, {14, 20}, {7, 5},  {14, 4},  {14, 13},
   };
} // namespace

BOOST_AUTO_TEST_CASE(dominance_simple_graph)
{
   // Create a simple graph:
   //   0 (entry)
   //   |
   //   1
   //   |
   //   2 (exit)
   TestGraph g;
   auto v0 = boost::add_vertex(g);
   auto v1 = boost::add_vertex(g);
   auto v2 = boost::add_vertex(g);
   boost::add_edge(v0, v1, g);
   boost::add_edge(v1, v2, g);

   // Test dominators
   dominance<TestGraph> dom(g, v0, v2);

   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(v0), v0); // Entry dominates itself
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(v1), v0); // 0 dominates 1
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(v2), v1); // 1 dominates 2

   // Test post-dominators
   dominance<TestGraph, true> postdom(g, v0, v2);

   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(v2), v2); // Exit post-dominates itself
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(v1), v2); // 2 post-dominates 1
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(v0), v1); // 1 post-dominates 0
}

BOOST_AUTO_TEST_CASE(dominance_with_branch)
{
   /* Create a graph with branching:
        0 (entry)
       / \
      1   2
       \ /
        3 (exit)
   */
   TestGraph g;
   auto v0 = boost::add_vertex(g);
   auto v1 = boost::add_vertex(g);
   auto v2 = boost::add_vertex(g);
   auto v3 = boost::add_vertex(g);
   boost::add_edge(v0, v1, g);
   boost::add_edge(v0, v2, g);
   boost::add_edge(v1, v3, g);
   boost::add_edge(v2, v3, g);

   // Test dominators
   dominance<TestGraph> dom(g, v0, v3);

   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(v0), v0); // Entry dominates itself
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(v1), v0); // 0 dominates 1
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(v2), v0); // 0 dominates 2
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(v3), v0); // 0 dominates 3 (common dominator)

   // Test post-dominators
   dominance<TestGraph, true> postdom(g, v0, v3);

   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(v3), v3); // Exit post-dominates itself
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(v1), v3); // 3 post-dominates 1
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(v2), v3); // 3 post-dominates 2
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(v0), v3); // 3 post-dominates 0 (common post-dominator)
}

BOOST_AUTO_TEST_CASE(dominance_with_unreachable_node)
{
   // Create a graph with an unreachable node for post-dominators:
   //   0 (entry) -> 1 (no successors - unreachable from exit)
   //   2 (exit, isolated)
   TestGraph g;
   auto v0 = boost::add_vertex(g);
   auto v1 = boost::add_vertex(g);
   auto v2 = boost::add_vertex(g);
   boost::add_edge(v0, v1, g);
   // Node 1 has no successors, so it's unreachable from exit (node 2)

   // Test post-dominators - should handle unreachable nodes gracefully
   dominance<TestGraph, true> postdom(g, v0, v2);

   // The test passes if no exception is thrown - the algorithm should handle
   // unreachable nodes by treating them as dominated by the virtual exit
   BOOST_CHECK_EQUAL(postdom.getImmediateDominator(v2), v2);
}

BOOST_AUTO_TEST_CASE(dominance_complex_cfg)
{
   /* Create a more complex CFG:
          0 (entry)
          |
          1
         / \
        2   3
        |   |
        4   |
         \ /
          5 (exit)
   */
   TestGraph g;
   auto v0 = boost::add_vertex(g);
   auto v1 = boost::add_vertex(g);
   auto v2 = boost::add_vertex(g);
   auto v3 = boost::add_vertex(g);
   auto v4 = boost::add_vertex(g);
   auto v5 = boost::add_vertex(g);
   boost::add_edge(v0, v1, g);
   boost::add_edge(v1, v2, g);
   boost::add_edge(v1, v3, g);
   boost::add_edge(v2, v4, g);
   boost::add_edge(v4, v5, g);
   boost::add_edge(v3, v5, g);

   // Test dominators
   dominance<TestGraph> dom(g, v0, v5);

   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(v0), v0); // Entry dominates itself
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(v1), v0); // 0 dominates 1
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(v2), v1); // 1 dominates 2
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(v3), v1); // 1 dominates 3
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(v4), v2); // 2 dominates 4
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(v5), v1); // 1 is the common dominator of 5

   // Test post-dominators
   dominance<TestGraph, true> postdom(g, v0, v5);

   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(v5), v5); // Exit post-dominates itself
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(v4), v5); // 5 post-dominates 4
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(v3), v5); // 5 post-dominates 3
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(v2), v4); // 4 post-dominates 2
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(v1), v5); // 5 is the common post-dominator of 1
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(v0), v1); // 1 post-dominates 0
}

BOOST_AUTO_TEST_CASE(dominance_20051113_test)
{
   /* Create the graph from 20051113BB_FCFG.dot in the repository root.
    * This is a real-world CFG with 15 basic blocks (0-14) from the GCC test case 20051113-1.
    * The graph represents a memory allocation function with loops and conditional branches.
    *
    * Control Flow Graph edges from 20051113BB_FCFG.dot:
    * 0 -> 1, 12    (entry to exit and to BB15)
    * 2 -> 10, 11   (BB2 branches)
    * 3 -> 2, 4     (BB3 loop body)
    * 4 -> 14, 3    (BB4 loop condition)
    * 5 -> 4        (BB5 to BB4)
    * 6 -> 5        (BB6 to BB5)
    * 7 -> 2, 5     (BB7 branches)
    * 8 -> 13, 3    (BB8 to exit or loop)
    * 9 -> 13       (BB9 to exit)
    * 10 -> 9       (BB10 to BB9)
    * 11 -> 9       (BB11 to BB9)
    * 12 -> 6, 7    (BB15 entry point branches)
    * 13 -> 1       (BB16 to exit)
    * 14 -> 8, 13   (BB17 loop body)
    *
    * This test verifies that the Semi-NCA algorithm implementation in PandA produces
    * the same dominance and post-dominance relationships as previous implementation.
    * The expected values are taken from 20051113BB_dom_tree.dot and
    * 20051113BB_post_dom_tree.dot which were generated by a previous version of
    * PandA-bambu and serve as ground truth.
    */
   TestGraph g;

   // Create vertices 0-14
   std::vector<TestGraph::vertex_descriptor> vertices;
   for(int i = 0; i < 15; ++i)
   {
      vertices.push_back(boost::add_vertex(g));
   }

   // Add edges from 20051113BB_FCFG.dot
   boost::add_edge(vertices[0], vertices[1], g);
   boost::add_edge(vertices[0], vertices[12], g);
   boost::add_edge(vertices[2], vertices[10], g);
   boost::add_edge(vertices[2], vertices[11], g);
   boost::add_edge(vertices[3], vertices[2], g);
   boost::add_edge(vertices[3], vertices[4], g);
   boost::add_edge(vertices[4], vertices[14], g);
   boost::add_edge(vertices[4], vertices[3], g);
   boost::add_edge(vertices[5], vertices[4], g);
   boost::add_edge(vertices[6], vertices[5], g);
   boost::add_edge(vertices[7], vertices[2], g);
   boost::add_edge(vertices[7], vertices[5], g);
   boost::add_edge(vertices[8], vertices[13], g);
   boost::add_edge(vertices[8], vertices[3], g);
   boost::add_edge(vertices[9], vertices[13], g);
   boost::add_edge(vertices[10], vertices[9], g);
   boost::add_edge(vertices[11], vertices[9], g);
   boost::add_edge(vertices[12], vertices[6], g);
   boost::add_edge(vertices[12], vertices[7], g);
   boost::add_edge(vertices[13], vertices[1], g);
   boost::add_edge(vertices[14], vertices[8], g);
   boost::add_edge(vertices[14], vertices[13], g);

   // Test dominators
   dominance<TestGraph> dom(g, vertices[0], vertices[1]);

   // Verify dominator relationships from 20051113BB_dom_tree.dot (PandA-bambu ground truth)
   // Dominator tree edges (parent -> child means parent immediately dominates child):
   // 0 -> 1, 12
   // 2 -> 10, 11, 9
   // 4 -> 14, 3
   // 5 -> 4
   // 12 -> 13, 2, 5, 6, 7
   // 14 -> 8

   // Entry dominates itself
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(vertices[0]), vertices[0]);
   // 0 -> 1, 12
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(vertices[1]), vertices[0]);
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(vertices[12]), vertices[0]);
   // 2 -> 10, 11, 9  ✓ FIXED
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(vertices[10]), vertices[2]);
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(vertices[11]), vertices[2]);
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(vertices[9]), vertices[2]);
   // 4 -> 14, 3
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(vertices[14]), vertices[4]);
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(vertices[3]), vertices[4]);
   // 5 -> 4
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(vertices[4]), vertices[5]);
   // 12 -> 13, 2, 5, 6, 7  ✓ FIXED
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(vertices[13]), vertices[12]);
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(vertices[2]), vertices[12]);
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(vertices[5]), vertices[12]);
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(vertices[6]), vertices[12]);
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(vertices[7]), vertices[12]);

   // 14 -> 8 (FIXED!)
   BOOST_REQUIRE_EQUAL(dom.getImmediateDominator(vertices[8]), vertices[14]);

   // Test post-dominators
   dominance<TestGraph, true> postdom(g, vertices[0], vertices[1]);

   // Verify post-dominator relationships from 20051113BB_post_dom_tree.dot (PandA-bambu ground truth)
   // Post-dominator tree edges (parent -> child means parent immediately post-dominates child):
   // 1 -> 0, 13
   // 4 -> 5
   // 5 -> 6
   // 9 -> 10, 11, 2
   // 13 -> 12, 14, 3, 4, 7, 8, 9

   // Exit post-dominates itself
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(vertices[1]), vertices[1]);
   // 1 -> 0, 13
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(vertices[0]), vertices[1]);
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(vertices[13]), vertices[1]);
   // 4 -> 5
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(vertices[5]), vertices[4]);
   // 5 -> 6
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(vertices[6]), vertices[5]);
   // 9 -> 10, 11, 2
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(vertices[10]), vertices[9]);
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(vertices[11]), vertices[9]);
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(vertices[2]), vertices[9]);
   // 13 -> 12, 14, 3, 4, 7, 8, 9
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(vertices[12]), vertices[13]);
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(vertices[14]), vertices[13]);
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(vertices[3]), vertices[13]);
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(vertices[4]), vertices[13]);
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(vertices[7]), vertices[13]);
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(vertices[8]), vertices[13]);
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(vertices[9]), vertices[13]);
}

BOOST_AUTO_TEST_CASE(dominance_arith_rand_cfg)
{
   const auto golden_dom_parents = makeParentMap(arith_rand_dom_tree_edges);
   const auto golden_post_dom_parents = makeParentMap(arith_rand_post_dom_tree_edges);
   auto cfg_graph = makeGraph(arith_rand_vertex_count, arith_rand_cfg_edges);

   const auto entry_vertex = static_cast<TestGraph::vertex_descriptor>(arith_rand_entry);
   const auto exit_vertex = static_cast<TestGraph::vertex_descriptor>(arith_rand_exit);
   dominance<TestGraph> dom(cfg_graph, entry_vertex, exit_vertex);

   BOOST_TEST_INFO("idom(13)=" << dom.getImmediateDominator(static_cast<TestGraph::vertex_descriptor>(13)));
   std::set<Edge> actual_dom_edges;
   dom.forEachDominanceRelation([&](TestGraph::vertex_descriptor child, TestGraph::vertex_descriptor parent) {
      actual_dom_edges.emplace(static_cast<unsigned>(parent), static_cast<unsigned>(child));
   });
   std::set<Edge> expected_dom_edges(arith_rand_dom_tree_edges.begin(), arith_rand_dom_tree_edges.end());
   BOOST_TEST_INFO("actual dominator edges size " << actual_dom_edges.size());
   BOOST_TEST_INFO("expected dominator edges size " << expected_dom_edges.size());
   if(actual_dom_edges != expected_dom_edges)
   {
      std::cerr << "Actual dominator edges:\n";
      for(const auto& [parent, child] : actual_dom_edges)
      {
         std::cerr << "  getImmediateDominator(" << child << ") -> " << parent << "\n";
      }
      std::cerr << "Expected dominator edges:\n";
      for(const auto& [parent, child] : expected_dom_edges)
      {
         std::cerr << "  getImmediateDominator(" << child << ") -> " << parent << "\n";
      }
      std::vector<Edge> extra;
      std::set_difference(actual_dom_edges.begin(), actual_dom_edges.end(), expected_dom_edges.begin(),
                          expected_dom_edges.end(), std::back_inserter(extra));
      std::vector<Edge> missing;
      std::set_difference(expected_dom_edges.begin(), expected_dom_edges.end(), actual_dom_edges.begin(),
                          actual_dom_edges.end(), std::back_inserter(missing));
      BOOST_TEST_INFO("Actual dominator edges:");
      for(const auto& [parent, child] : actual_dom_edges)
      {
         BOOST_TEST_INFO("  getImmediateDominator(" << child << ") -> " << parent);
      }
      BOOST_TEST_INFO("Expected dominator edges:");
      for(const auto& [parent, child] : expected_dom_edges)
      {
         BOOST_TEST_INFO("  getImmediateDominator(" << child << ") -> " << parent);
      }
      for(const auto& [parent, child] : extra)
      {
         BOOST_TEST_INFO("Extra edge getImmediateDominator(" << child << ") -> " << parent);
      }
      for(const auto& [parent, child] : missing)
      {
         BOOST_TEST_INFO("Missing edge getImmediateDominator(" << child << ") -> " << parent);
      }
   }
   BOOST_CHECK(actual_dom_edges == expected_dom_edges);

   const auto vertex_count = static_cast<unsigned>(cfg_graph.num_vertices());
   for(unsigned v = 0; v < vertex_count; ++v)
   {
      if(v == arith_rand_entry)
      {
         BOOST_CHECK_EQUAL(dom.getImmediateDominator(v), v);
         continue;
      }
      const auto it = golden_dom_parents.find(v);
      BOOST_REQUIRE_MESSAGE(it != golden_dom_parents.end(), "Missing golden dominator for vertex " << v);
      BOOST_TEST_INFO("dominance vertex getImmediateDominator(" << v << ") expected " << it->second);
      BOOST_CHECK_EQUAL(dom.getImmediateDominator(v), it->second);
   }

   dominance<TestGraph, true> postdom(cfg_graph, entry_vertex, exit_vertex);

   for(unsigned v = 0; v < vertex_count; ++v)
   {
      if(v == arith_rand_exit)
      {
         BOOST_CHECK_EQUAL(postdom.getImmediateDominator(v), v);
         continue;
      }
      const auto it = golden_post_dom_parents.find(v);
      BOOST_REQUIRE_MESSAGE(it != golden_post_dom_parents.end(), "Missing golden post-dominator for vertex " << v);
      BOOST_TEST_INFO("post-dominance vertex getImmediateDominator(" << v << ") expected " << it->second);
      BOOST_CHECK_EQUAL(postdom.getImmediateDominator(v), it->second);
   }

   std::set<std::pair<unsigned, unsigned>> expected_backedges(arith_rand_backedges.begin(), arith_rand_backedges.end());
   std::set<std::pair<unsigned, unsigned>> computed_backedges;
   for(const auto& edge : cfg_graph.edges())
   {
      const auto src = cfg_graph.source(edge);
      const auto dst = cfg_graph.target(edge);
      if(dominates(dom, dst, src))
      {
         computed_backedges.emplace(static_cast<unsigned>(src), static_cast<unsigned>(dst));
      }
   }
   BOOST_CHECK(expected_backedges == computed_backedges);
}

BOOST_AUTO_TEST_CASE(dominance_arith_rand2_cfg)
{
   const auto golden_dom_parents = makeParentMap(arith_rand2_dom_tree_edges);
   const auto golden_post_dom_parents = makeParentMap(arith_rand2_post_dom_tree_edges);
   auto cfg_graph = makeGraph(arith_rand2_vertex_count, arith_rand2_cfg_edges);

   const auto entry_vertex = static_cast<TestGraph::vertex_descriptor>(arith_rand2_entry);
   const auto exit_vertex = static_cast<TestGraph::vertex_descriptor>(arith_rand2_exit);
   dominance<TestGraph> dom(cfg_graph, entry_vertex, exit_vertex);

   std::set<Edge> actual_dom_edges;
   dom.forEachDominanceRelation([&](TestGraph::vertex_descriptor child, TestGraph::vertex_descriptor parent) {
      actual_dom_edges.emplace(static_cast<unsigned>(parent), static_cast<unsigned>(child));
   });
   std::set<Edge> expected_dom_edges(arith_rand2_dom_tree_edges.begin(), arith_rand2_dom_tree_edges.end());
   BOOST_TEST_INFO("actual dominator edges size " << actual_dom_edges.size());
   BOOST_TEST_INFO("expected dominator edges size " << expected_dom_edges.size());

   if(actual_dom_edges != expected_dom_edges)
   {
      std::cerr << "Actual dominator edges:\n";
      for(const auto& [parent, child] : actual_dom_edges)
      {
         std::cerr << "  getImmediateDominator(" << child << ") -> " << parent << "\n";
      }
      std::cerr << "Expected dominator edges:\n";
      for(const auto& [parent, child] : expected_dom_edges)
      {
         std::cerr << "  getImmediateDominator(" << child << ") -> " << parent << "\n";
      }
      std::vector<Edge> extra;
      std::set_difference(actual_dom_edges.begin(), actual_dom_edges.end(), expected_dom_edges.begin(),
                          expected_dom_edges.end(), std::back_inserter(extra));
      std::vector<Edge> missing;
      std::set_difference(expected_dom_edges.begin(), expected_dom_edges.end(), actual_dom_edges.begin(),
                          actual_dom_edges.end(), std::back_inserter(missing));
      for(const auto& [parent, child] : extra)
      {
         BOOST_TEST_INFO("Extra edge getImmediateDominator(" << child << ") -> " << parent);
      }
      for(const auto& [parent, child] : missing)
      {
         BOOST_TEST_INFO("Missing edge getImmediateDominator(" << child << ") -> " << parent);
      }
   }
   BOOST_CHECK(actual_dom_edges == expected_dom_edges);

   const auto vertex_count = static_cast<unsigned>(cfg_graph.num_vertices());
   for(unsigned v = 0; v < vertex_count; ++v)
   {
      if(v == arith_rand2_entry)
      {
         BOOST_CHECK_EQUAL(dom.getImmediateDominator(v), v);
         continue;
      }
      const auto it = golden_dom_parents.find(v);
      BOOST_REQUIRE_MESSAGE(it != golden_dom_parents.end(), "Missing golden dominator for vertex " << v);
      BOOST_TEST_INFO("dominance vertex getImmediateDominator(" << v << ") expected " << it->second);
      BOOST_CHECK_EQUAL(dom.getImmediateDominator(v), it->second);
   }

   dominance<TestGraph, true> postdom(cfg_graph, entry_vertex, exit_vertex);

   for(unsigned v = 0; v < vertex_count; ++v)
   {
      if(v == arith_rand2_exit)
      {
         BOOST_CHECK_EQUAL(postdom.getImmediateDominator(v), v);
         continue;
      }
      const auto it = golden_post_dom_parents.find(v);
      BOOST_REQUIRE_MESSAGE(it != golden_post_dom_parents.end(), "Missing golden post-dominator for vertex " << v);
      BOOST_TEST_INFO("post-dominance vertex getImmediateDominator(" << v << ") expected " << it->second);
      BOOST_CHECK_EQUAL(postdom.getImmediateDominator(v), it->second);
   }
}

BOOST_AUTO_TEST_CASE(post_dominance_irr4_graph)
{
   // CFG extracted from irr4_fcfg.dot
   TestGraph g;
   auto entry = boost::add_vertex(g); // 0
   auto exit = boost::add_vertex(g);  // 1
   auto bb2 = boost::add_vertex(g);   // 2
   auto bb3 = boost::add_vertex(g);   // 3
   auto bb4 = boost::add_vertex(g);   // 4

   // Forward edges
   boost::add_edge(entry, exit, g); // 0 -> 1
   boost::add_edge(entry, bb2, g);  // 0 -> 2
   boost::add_edge(bb2, bb3, g);    // 2 -> 3 (true branch)
   boost::add_edge(bb2, bb4, g);    // 2 -> 4 (false branch)
   boost::add_edge(bb3, exit, g);   // 3 -> 1
   boost::add_edge(bb4, bb4, g);    // 4 -> 4 (self-loop)

   dominance<TestGraph, true> postdom(g, entry, exit);

   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(exit), exit);  // Exit post-dominates itself
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(entry), exit); // EXIT is the immediate post-dominator of ENTRY
   // Immediate post-dominator relationships derived from irr4_post_dominator.dot
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(bb2), exit); // EXIT is the immediate post-dominator of BB2
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(bb3), exit); // EXIT post-dominates BB3
   BOOST_REQUIRE_EQUAL(postdom.getImmediateDominator(bb4), exit); // EXIT post-dominates BB4 (loop with no other exits)
}
