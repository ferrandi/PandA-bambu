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
 * @file loops.cpp
 * @brief implementation of loops finding algorithm
 *
 * Loops are detection described in
 * in Vugranam C. Sreedhar, Guang R. Gao, Yong-Fong Lee: Identifying Loops Using DJ Graphs. ACM Trans. Program. Lang. Syst. 18(6): 649-658 (1996)
 *
 * @author Marco Garatti <m.garatti@gmail.com>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "loops.hpp"

/// Autoheader include
#include "config_HAVE_HOST_PROFILING_BUILT.hpp"

#include "custom_map.hpp"
#include "custom_set.hpp"
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/lexical_cast.hpp> // for lexical_cast
#include <iosfwd>
#include <list>
#include <ostream> // for operator<<, basic_o...
#include <utility>
#include <vector>

#include "Dominance.hpp"
#include "Parameter.hpp"
#include "Vertex.hpp"
#include "basic_block.hpp"
#include "basic_blocks_graph_constructor.hpp"
#include "behavioral_helper.hpp"
#include "behavioral_writer_helper.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "hash_helper.hpp"
#include "loop.hpp"
#if HAVE_HOST_PROFILING_BUILT
#include "profiling_information.hpp"
#endif
#include "string_manipulation.hpp" // for STR GET_CLASS
#include "tree_basic_block.hpp"

/**
 * Visitor used during the depth-first search on the DJ graph.
 */
struct djgraph_dfs_tree_visitor : public boost::default_dfs_visitor
{
 private:
   /// topological level of the vertices
   CustomUnorderedMap<vertex, unsigned int>& vertex_level;

   /// dfs order of vertices
   CustomUnorderedMap<vertex, unsigned int>& dfs_order;

   /// last label used during the dfs graph visiting
   unsigned int dfs_number;

   /// maximum level of vertices
   unsigned int& max_level;

   /// spanning tree
   vertex2obj<vertex>& parent_depth_search_spanning_tree;

   /// The debug level
   const int debug_level;

 public:
   /**
    * Constructor of the postorder tree visitor. Used to label the node in postorder way.
    * @param vertex_level will store levels of vertices
    * @param dfs_order will store vertices dfs order
    * @param max_level will store the max level of vertices
    * @param parent_depth_search_spanning_tree is the dfs spanning tree
    * @param parameters is the set of input parameters
    */
   djgraph_dfs_tree_visitor(CustomUnorderedMap<vertex, unsigned int>& _vertex_level, CustomUnorderedMap<vertex, unsigned int>& _dfs_order, unsigned int& _max_level, vertex2obj<vertex>& _parent_depth_search_spanning_tree, const ParameterConstRef parameters)
       : vertex_level(_vertex_level), dfs_order(_dfs_order), dfs_number(0), max_level(_max_level), parent_depth_search_spanning_tree(_parent_depth_search_spanning_tree), debug_level(parameters->get_class_debug_level(GET_CLASS(*this)))
   {
   }

   template <class Vertex, class Graph>
   void discover_vertex(Vertex v, Graph& g)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Discovered vertex BB" + STR(g.CGetBBNodeInfo(v)->block->number));
      /// Set the dfs order
      dfs_order[v] = dfs_number++;
      /// and the vertex level
      if(v == g.CGetBBGraphInfo()->entry_vertex)
      {
         vertex_level[v] = 0;
      }
      else
      {
         /// The level of at least one predecessor must have been set already
         typename boost::graph_traits<Graph>::in_edge_iterator ei, ei_end;
         unsigned int new_level = 0;
         for(boost::tie(ei, ei_end) = boost::in_edges(v, g); ei != ei_end; ++ei)
         {
            vertex source = boost::source(*ei, g);
            if(vertex_level.find(source) != vertex_level.end())
            {
               unsigned int source_level = vertex_level[source];
               if(new_level == 0)
               {
                  new_level = source_level + 1;
               }
               else
               {
                  if(source_level + 1 < new_level)
                  {
                     new_level = source_level + 1;
                  }
               }
            }
         }
         THROW_ASSERT(new_level > 0, "Cannot determine a proper vertex level");
         vertex_level[v] = new_level;
         if(new_level > max_level)
         {
            max_level = new_level;
         }
      }
   }

   template <class Edge, class Graph>
   void tree_edge(Edge e, const Graph& g) const
   {
      vertex src, tgt;
      src = boost::source(e, g);
      tgt = boost::target(e, g);
      parent_depth_search_spanning_tree[tgt] = src;
   }
};

Loops::Loops(const FunctionBehaviorRef _FB, const ParameterConstRef parameters) : FB(FunctionBehaviorRef(_FB.get(), null_deleter())), Param(parameters), debug_level(parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE))
{
   /// Detect loops
   DetectLoops();
   /// Build Zero Loop
   BuildZeroLoop();
   /// compute depth
   computeDepth(GetLoop(0));
   /// compute landing pads
   std::list<LoopRef>::const_iterator loop, loop_end = modifiable_loops_list.end();
   for(loop = modifiable_loops_list.begin(); loop != loop_end; ++loop)
   {
      (*loop)->ComputeLandingPadExits();
   }
}

bool Loops::is_edge_in_list(CustomUnorderedSet<vertex_pair>& l, vertex source, vertex target)
{
   vertex_pair vp(source, target);
   return l.find(vp) != l.end();
}

static bool check_ancestor(vertex x, vertex y, const vertex2obj<vertex>& parent_depth_search_spanning_tree)
{
   return ((x != y && y == parent_depth_search_spanning_tree(x)) || (x != parent_depth_search_spanning_tree(x) && check_ancestor(parent_depth_search_spanning_tree(x), y, parent_depth_search_spanning_tree)));
}

void Loops::DetectLoops()
{
   const BasicBlocksGraphConstructorRef bbcg = FB->bbgc;
   const BBGraphRef cfg = FB->GetBBGraph(FunctionBehavior::FBB);
   const BehavioralHelperConstRef helper = FB->CGetBehavioralHelper();
   const std::string function_name = helper->get_function_name();
   const vertex entry = cfg->CGetBBGraphInfo()->entry_vertex;
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Starting Loops Detection of " + function_name);

   /// Retrieve dominator tree and dominance relation
   const dominance<BBGraph>* dom = FB->dominators;
   const auto full_dom = dom->getAllDominated();

   /// Create the DJ graph. This is done in two steps:
   /// 1) build the dominator tree
   /// 2) add the "J" edges

   /// Add J edges (and detect CJ ones)
   /// D edges are already there since we use boost filtered graphs
   CustomUnorderedSet<vertex_pair> cj_edges;
   CustomUnorderedSet<vertex_pair> bj_edges;
   EdgeIterator ei, ei_end;
   for(boost::tie(ei, ei_end) = boost::edges(*cfg); ei != ei_end; ++ei)
   {
      vertex source = boost::source(*ei, *cfg);
      vertex target = boost::target(*ei, *cfg);

      if(source != dom->get_immediate_dominator(target))
      {
         bbcg->AddEdge(source, target, J_SELECTOR);

         /// Check for CJ and BJ edges
         bool cj;
         if(full_dom.find(target) == full_dom.end())
         {
            cj = true;
         }
         else
         {
            const CustomOrderedSet<vertex>& dom_set = full_dom.find(target)->second;
            if(dom_set.find(source) != dom_set.end())
            {
               /// This is a BJ edge, and we mark it as such
               cj = false;
            }
            else
            {
               /// This is a CJ edge, and we mark it as such
               cj = true;
            }
         }
         if(cj)
         {
            vertex_pair cj_edge(source, target);
            cj_edges.insert(cj_edge);
         }
         else
         {
            vertex_pair bj_edge(source, target);
            bj_edges.insert(bj_edge);
         }
      }
   }

   const BBGraphRef djg = FB->GetBBGraph(FunctionBehavior::DJ);

   if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
   {
      djg->WriteDot("BB_DJ.dot");
   }

   /// Build the spanning tree and detect the sp-back edges
   CustomUnorderedMap<vertex, unsigned int> vertex_level_rel;
   CustomUnorderedMap<vertex, unsigned int> dfs_order;
   unsigned int max_level = 0;

   /// spanning tree ancestor data structure
   vertex2obj<vertex> parent_depth_search_spanning_tree;

   /// initialization
   parent_depth_search_spanning_tree[entry] = entry;

   /// spanning tree construction and vertex labeling
   std::vector<boost::default_color_type> color_vec(boost::num_vertices(*djg));
   djgraph_dfs_tree_visitor vis(vertex_level_rel, dfs_order, max_level, parent_depth_search_spanning_tree, Param);
   boost::depth_first_visit(*djg, entry, vis, boost::make_iterator_property_map(color_vec.begin(), boost::get(boost::vertex_index, *djg), boost::white_color));
   std::vector<std::list<vertex>> level_vertices_rel(max_level + 1);
   /// compute the level_vertices_rel
   for(auto vl_pair : vertex_level_rel)
   {
      auto& curr_list = level_vertices_rel[vl_pair.second];
      auto pos = curr_list.begin();
      const std::list<vertex>::iterator pos_end = curr_list.end();
      while(pos_end != pos && dfs_order.find(vl_pair.first)->second > dfs_order.find(*pos)->second)
         ++pos;
      if(pos == pos_end)
         curr_list.push_back(vl_pair.first);
      else
         curr_list.insert(pos, vl_pair.first);
   }

   /// Detect the sp-back edges
   CustomUnorderedSet<vertex_pair> sp_back_edges;
   for(boost::tie(ei, ei_end) = boost::edges(*djg); ei != ei_end; ++ei)
   {
      vertex source = boost::source(*ei, *djg);
      vertex target = boost::target(*ei, *djg);

      if(source == target || check_ancestor(source, target, parent_depth_search_spanning_tree))
      {
         /// This is a sp-back edge
         vertex_pair sp_edge(source, target);
         sp_back_edges.insert(sp_edge);
      }
   }

   for(unsigned int index = max_level + 1; index > 0; --index)
   {
      unsigned int lev = index - 1;
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Level " + boost::lexical_cast<std::string>(lev));
      bool irreducible = false;
      for(auto v : level_vertices_rel[lev])
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Considering BB" + boost::lexical_cast<std::string>(cfg->CGetBBNodeInfo(v)->block->number));
         const LoopRef loop = LoopRef(new Loop(cfg, v));
         bool reducible = false;
         CustomOrderedSet<vertex> visited;
         graph::in_edge_iterator e_iter, e_iter_end;
         for(boost::tie(e_iter, e_iter_end) = boost::in_edges(v, *djg); e_iter != e_iter_end; ++e_iter)
         {
            vertex m = boost::source(*e_iter, *djg);
            vertex n = boost::target(*e_iter, *djg);
            if(is_edge_in_list(cj_edges, m, n) && is_edge_in_list(sp_back_edges, m, n))
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                             "Irreducible loop found: SP-BACK-EDGE=" + boost::lexical_cast<std::string>(cfg->CGetBBNodeInfo(m)->block->number) + "->" + boost::lexical_cast<std::string>(cfg->CGetBBNodeInfo(n)->block->number));
               irreducible = true;
            }
            if(is_edge_in_list(bj_edges, m, n))
            {
               reducible = true;
               /// Detect this Loop (n is the header)
               block_to_loop[n] = loop;
               DetectReducibleLoop(djg, visited, loop, m, n);

               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                             "Found a reducible Loop: SP-BACK-EDGE=" + boost::lexical_cast<std::string>(cfg->CGetBBNodeInfo(m)->block->number) + "->" + boost::lexical_cast<std::string>(cfg->CGetBBNodeInfo(n)->block->number));
            }
         }
         if(reducible)
         {
            modifiable_loops_list.push_back(loop);
            const_loops_list.push_back(loop);
         }
      }
      if(irreducible)
      {
         /// Identify SCCs for the subgraphs induced by nodes at level greater or equal than lev
         DetectIrreducibleLoop(djg, lev, max_level, level_vertices_rel);
      }
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Ended Loops Detection of " + function_name);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Adding spanning tree back edges at " + function_name);
   for(auto curr_loop : modifiable_loops_list)
   {
      CustomUnorderedSet<vertex> blocks;
      curr_loop->get_recursively_bb(blocks);
      for(auto curr_sp_back_edge : sp_back_edges)
         if(blocks.find(curr_sp_back_edge.first) != blocks.end() && blocks.find(curr_sp_back_edge.second) != blocks.end())
         {
            curr_loop->add_sp_back_edge(curr_sp_back_edge.first, curr_sp_back_edge.second);
            if(!curr_loop->IsReducible())
            {
               curr_loop->add_entry(curr_sp_back_edge.second);
               vertex entry_imm_dom = dom->get_immediate_dominator(curr_sp_back_edge.second);
               for(auto cur_bb : blocks)
                  if(entry_imm_dom == dom->get_immediate_dominator(cur_bb))
                     curr_loop->add_entry(cur_bb);
            }
         }
      THROW_ASSERT(curr_loop->get_sp_back_edges().size() > 0, "wrongly computed loop back edges");
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Added spanning tree back edges at " + function_name);
}

void Loops::DetectReducibleLoop(const BBGraphRef djg, CustomOrderedSet<vertex>& visited, LoopRef l, vertex real_node, vertex header)
{
   graph::in_edge_iterator e_iter, e_iter_end;

   if(real_node == header)
      return;
   visited.insert(real_node);

   if(block_to_loop.find(real_node) == block_to_loop.end())
   {
      // Update the loop
      l->add_block(real_node);
      // Update the block to loop map
      block_to_loop[real_node] = l;
   }
   else if(block_to_loop[real_node] != l)
   {
      // This block belongs to another loop therefore we have a nesting here and we should not
      // add this block to the outermost
      LoopRef innermost_loop = LoopRef(block_to_loop[real_node]);
      if(innermost_loop->Parent() == nullptr)
      {
         l->AddChild(innermost_loop);
         innermost_loop->SetParent(l);
      }
   }
   // else the block already belongs to this loop. Nothing to be done

   // Note that since this is a reducible loop, we cannot have any edge entering the loop
   // body other then the predecessors of the loop header
   for(boost::tie(e_iter, e_iter_end) = boost::in_edges(real_node, *djg); e_iter != e_iter_end; ++e_iter)
   {
      vertex source = boost::source(*e_iter, *djg);
      if(visited.find(source) == visited.end())
         DetectReducibleLoop(djg, visited, l, source, header);
   }
}

void Loops::DetectIrreducibleLoop(const BBGraphRef djg, unsigned int min_level, unsigned int max_level, std::vector<std::list<vertex>>& level_vertices_rel)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Detecting irreducible loop - Min level " + STR(min_level) + " - Max level " + STR(max_level));
   CustomOrderedSet<vertex> u;

   /// Populate u with all the non-visited node whose level is >= min_level
   for(auto level = min_level; level <= max_level; ++level)
   {
      for(auto v : level_vertices_rel[level])
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Inserting BB" + STR(djg->CGetBBNodeInfo(v)->block->number) + " into set to be analyzed");
         u.insert(v);
      }
   }

   THROW_ASSERT(u.size() > 0, "There must be at least one item in u");
   do
   {
      unsigned int max_dfs = 0;
      vertex min_ord_ver = *(u.begin());
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "DetectIrreducibleLoop: starting vertex=" + boost::lexical_cast<std::string>(FB->GetBBGraph(FunctionBehavior::BB)->CGetBBNodeInfo(min_ord_ver)->block->number));
      CustomUnorderedMap<vertex, unsigned int> dfs_order;
      std::list<vertex> s;
      CustomUnorderedMap<vertex, unsigned int> lowlink;
      tarjan_scc(djg, min_ord_ver, dfs_order, lowlink, s, u, max_dfs);
      s.clear();
      lowlink.clear();
      dfs_order.clear();
   } while(!u.empty());
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Detected irreducible loop");
}

bool Loops::stack_contains(std::list<vertex> stack, vertex v)
{
   auto stack_iter = stack.begin();
   auto stack_end = stack.end();

   for(; stack_iter != stack_end; ++stack_iter)
   {
      vertex current_node = *stack_iter;
      if(current_node == v)
         return true;
   }
   return false;
}

void Loops::tarjan_scc(const BBGraphRef djg, vertex v, CustomUnorderedMap<vertex, unsigned int>& dfs_order, CustomUnorderedMap<vertex, unsigned int>& lowlink, std::list<vertex>& s, CustomOrderedSet<vertex>& u, unsigned int& max_dfs)
{
   dfs_order[v] = max_dfs++;
   lowlink[v] = dfs_order[v];
   s.push_back(v);
   u.erase(v);

   graph::out_edge_iterator e_out_iter, e_out_iter_end;
   for(boost::tie(e_out_iter, e_out_iter_end) = boost::out_edges(v, *djg); e_out_iter != e_out_iter_end; ++e_out_iter)
   {
      vertex target = boost::target(*e_out_iter, *djg);

      if(u.find(target) != u.end())
      {
         tarjan_scc(djg, target, dfs_order, lowlink, s, u, max_dfs);
         lowlink[v] = lowlink[v] < lowlink[target] ? lowlink[v] : lowlink[target];
      }
      else if(stack_contains(s, target))
      {
         lowlink[v] = lowlink[v] < dfs_order[target] ? lowlink[v] : dfs_order[target];
      }
   }

   if(s.back() == v && lowlink[v] == dfs_order[v])
      s.pop_back();
   else if(lowlink[v] == dfs_order[v])
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking if we have to build a new irreducible loop");
      // v is the root of a strongly connectec component
      CustomUnorderedSet<vertex> candidate_body_loop;
      vertex v1;
      do
      {
         v1 = s.back();
         s.pop_back();
         // Add v1 to the loop
         vertex real_node;

         real_node = v1;
         candidate_body_loop.insert(real_node);
      } while(v1 != v);
      CustomUnorderedSet<vertex> real_body_loop;
      CustomUnorderedSet<LoopRef> nested_loops;
      for(const auto candidate : candidate_body_loop)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing candidate BB" + STR(djg->CGetBBNodeInfo(candidate)->block->number));
         if(block_to_loop.find(candidate) != block_to_loop.end())
         {
            const auto candidate_nesting_loop = block_to_loop.find(candidate)->second;
            if(candidate_nesting_loop->GetHeader() == candidate and not candidate_nesting_loop->Parent())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--It is the header of a nested loop");
               nested_loops.insert(candidate_nesting_loop);
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Already belongs to a loop");
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--It will be added to the body loop");
            real_body_loop.insert(candidate);
         }
      }
      if(real_body_loop.size())
      {
         LoopRef l = LoopRef(new Loop(djg));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Creating data structure for irreducible loop " + STR(l->GetId()));
         for(const auto body_loop_vertex : real_body_loop)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding BB" + STR(djg->CGetBBNodeInfo(body_loop_vertex)->block->number));
            l->add_block(body_loop_vertex);
            block_to_loop[body_loop_vertex] = l;
         }
         for(const auto& nested_loop : nested_loops)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Setting " + STR(l->GetId()) + " as parent of " + STR(nested_loop->GetId()));
            nested_loop->SetParent(l);
            l->AddChild(nested_loop);
         }
         modifiable_loops_list.push_back(l);
         const_loops_list.push_back(l);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created data structure for irreducible loop " + STR(l->GetId()));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checking if we have to build a new irreducible loop: Yes");
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checking if we have to build a new irreducible loop: No");
      }
   }
}

const std::list<LoopConstRef>& Loops::GetList() const
{
   return const_loops_list;
}

const std::list<LoopRef>& Loops::GetModifiableList() const
{
   return modifiable_loops_list;
}

const LoopConstRef Loops::CGetLoop(unsigned int id) const
{
   std::list<LoopConstRef>::const_iterator it, it_end;
   it_end = const_loops_list.end();
   for(it = const_loops_list.begin(); it != it_end; ++it)
      if((*it)->GetId() == id)
         return *it;
   THROW_UNREACHABLE("Loop with id " + boost::lexical_cast<std::string>(id) + " doesn't exist");
   return LoopConstRef();
}

const LoopRef Loops::GetLoop(unsigned int id)
{
   std::list<LoopRef>::const_iterator it, it_end;
   it_end = modifiable_loops_list.end();
   for(it = modifiable_loops_list.begin(); it != it_end; ++it)
      if((*it)->GetId() == id)
         return *it;
   THROW_UNREACHABLE("Loop with id " + boost::lexical_cast<std::string>(id) + " doesn't exist");
   return LoopRef();
}

size_t Loops::NumLoops() const
{
   return const_loops_list.size();
}

void Loops::WriteDot(const std::string& file_name
#if HAVE_HOST_PROFILING_BUILT
                     ,
                     const ProfilingInformationConstRef profiling_information
#endif
) const
{
   std::string output_directory = Param->getOption<std::string>(OPT_dot_directory);
   output_directory += FB->CGetBehavioralHelper()->get_function_name() + "/";
   const BBGraphRef cfg = FB->GetBBGraph(FunctionBehavior::BB);
   std::ofstream dot((output_directory + file_name).c_str());
   dot << "digraph LoopForest {" << std::endl;
   for(const auto& loop : const_loops_list)
   {
      dot << loop->GetId() << " [label=\"LoopId=" << loop->GetId() << " - Depth: " << loop->depth;
#if HAVE_HOST_PROFILING_BUILT
      if(profiling_information)
         dot << "\\nAvg. Iterations=" << profiling_information->GetLoopAvgIterations(loop) << "- Max Iterations=" << profiling_information->GetLoopMaxIterations(loop->GetId());
#endif
      dot << "\\nType:";
      if(loop->loop_type & SINGLE_EXIT_LOOP)
         dot << " Single-Exit";
      if(loop->loop_type == UNKNOWN_LOOP)
         dot << " Generic";
      if(loop->loop_type & WHILE_LOOP)
         dot << " While";
      if(loop->loop_type & FOR_LOOP)
         dot << " For";
      if(loop->loop_type & DOALL_LOOP)
         dot << " DoAll";
      if(loop->loop_type & DO_WHILE_LOOP)
         dot << " DoWhile";
      if(loop->loop_type & COUNTABLE_LOOP)
      {
         dot << " Countable[" << STR(loop->lower_bound) << ":" << STR(loop->increment) << ":" << STR(loop->upper_bound) << (loop->close_interval ? "]" : ")");
      }
      if(loop->loop_type & PIPELINABLE_LOOP)
         dot << " Pipelinable";
      dot << "\\nBlocks:";
      const CustomUnorderedSet<vertex>& blocks = loop->get_blocks();
      CustomUnorderedSet<vertex>::const_iterator bb, bb_end = blocks.end();
      for(bb = blocks.begin(); bb != bb_end; ++bb)
      {
         dot << " BB" + boost::lexical_cast<std::string>(cfg->CGetBBNodeInfo(*bb)->block->number);
      }
      dot << "\\n\"];" << std::endl;
   }
   for(const auto& loop : const_loops_list)
   {
      for(const auto& child : loop->GetChildren())
      {
         dot << loop->GetId() << "->" << child->GetId() << ";" << std::endl;
      }
   }
   dot << "}" << std::endl;
}

void Loops::computeDepth(const LoopConstRef loop)
{
   const CustomOrderedSet<LoopConstRef> children = loop->GetChildren();
   CustomOrderedSet<LoopConstRef>::const_iterator child, child_end = children.end();
   for(child = children.begin(); child != child_end; ++child)
   {
      auto* child_loop = const_cast<Loop*>(child->get());
      child_loop->depth = loop->depth + 1;
      computeDepth(*child);
   }
}

void Loops::BuildZeroLoop()
{
   const BBGraphRef cfg = FB->GetBBGraph(FunctionBehavior::BB);
   const LoopRef zero_loop(new Loop(cfg, cfg->CGetBBGraphInfo()->entry_vertex));
   /// Putting all the basic blocks in blocks
   VertexIterator v, v_end;
   for(boost::tie(v, v_end) = boost::vertices(*cfg); v != v_end; v++)
   {
      zero_loop->blocks.insert(*v);
   }

   if(not const_loops_list.empty())
      zero_loop->is_innermost_loop = false;
   else
      zero_loop->is_innermost_loop = true;

   std::list<LoopRef>::iterator loop, loop_end = modifiable_loops_list.end();
   for(loop = modifiable_loops_list.begin(); loop != loop_end; ++loop)
   {
      if(not(*loop)->Parent())
      {
         (*loop)->parent_loop = zero_loop;
         zero_loop->children.insert(*loop);
         CustomUnorderedSet<vertex> children_blocks;
         (*loop)->get_recursively_bb(children_blocks);
         CustomUnorderedSet<vertex>::const_iterator child_block, child_block_end = children_blocks.end();
         for(child_block = children_blocks.begin(); child_block != child_block_end; ++child_block)
         {
            if(zero_loop->blocks.find(*child_block) != zero_loop->blocks.end())
               zero_loop->blocks.erase(zero_loop->blocks.find(*child_block));
         }
      }
   }

   modifiable_loops_list.push_front(zero_loop);
   const_loops_list.push_front(zero_loop);
}
