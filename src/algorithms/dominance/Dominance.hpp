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
 * @file dominance.hpp
 * @brief Class computing (post)dominators.
 *
 * This C++ source code deeply resemble to the C source code used by GCC to
 * quickly compute (post)dominators.
 * The GCC source code has been written by Michael Matz (matz@ifh.de) and it
 * is copyrighted by Free Software Foundation, Inc.
 * This file implements the well known algorithm from Lengauer and Tarjan
 * to compute the dominators in a control flow graph.  A basic block D is said
 * to dominate another block X, when all paths from the entry node of the CFG
 * to X go also over D.  The dominance relation is a transitive reflexive
 * relation and its minimal transitive reduction is a tree, called the
 * dominator tree.  So for each block X besides the entry block exists a
 * block I(X), called the immediate dominator of X, which is the parent of X
 * in the dominator tree.
 *
 * The algorithm computes this dominator tree implicitly by computing for
 * each block its immediate dominator.  We use tree balancing and path
 * compression, so it's the O(e*a(e,v)) variant, where a(e,v) is the very
 * slowly growing functional inverse of the Ackerman function.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef DOMINANCE_HPP
#define DOMINANCE_HPP

/// Parameter include
#include "Parameter.hpp"

/// STL include
#include "custom_map.hpp"
#include "custom_set.hpp"
#include <vector>

/// Utility include
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_NONE
#include "exceptions.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/version.hpp>

/// Type of Basic Block aka TBB. In this case each basic block is represented with its index.
typedef unsigned long int TBB;

/**
 * Store the intermediate information used to compute dominator or post dominator information.
 * It holds various arrays reflecting the (sub)structure of the flowgraph.
 * Most of them are of type TBB and are also indexed by TBB.
 */
template <typename GraphObj>
class dom_info
{
   /// The algorithm uses two different indices for each Vertex: a generic index given by boost and the index in the dfs search
 private:
   /// Definition of Vertex
   typedef typename boost::graph_traits<const GraphObj>::vertex_descriptor Vertex;
   /// edge_iterator definition.
   typedef typename boost::graph_traits<const GraphObj>::in_edge_iterator in_edge_iterator;
   /// edge_iterator definition.
   typedef typename boost::graph_traits<const GraphObj>::out_edge_iterator out_edge_iterator;
   /// edge definition.
   typedef typename boost::graph_traits<const GraphObj>::edge_descriptor edge;
   /// Vertex iterator definition
   typedef typename boost::graph_traits<const GraphObj>::vertex_iterator Vertex_iterator;

   /// The parent of a node in the DFS tree.
   std::vector<TBB> dfs_parent;

   /**
    * For a node x key[x] is roughly the node nearest to the root from which
    * exists a way to x only over nodes behind x.  Such a node is also called
    * semidominator.
    */
   std::vector<TBB> key;

   /// The value in path_min[x] is the node y on the path from x to the root of the tree x is in with the smallest key[y]
   std::vector<TBB> path_min;

   /// bucket[x] points to the first node of the set of nodes having x as key.
   std::vector<TBB> bucket;

   /// And next_bucket[x] points to the next node.
   std::vector<TBB> next_bucket;

   /**
    * set_chain[x] is the next node on the path from x to the representant
    * of the set containing x.  If set_chain[x]==0 then x is a root.
    * It's similar to a reversed link list
    */
   std::vector<TBB> set_chain;

   /// set_size[x] is the number of elements in the set named by x.
   std::vector<unsigned int> set_size;
   /**
    * set_child[x] is used for balancing the tree representing a set. It can
    * be understood as the next sibling of x.
    */
   std::vector<TBB> set_child;

   /// This is the next free DFS number when creating the DFS tree.
   unsigned int dfsnum;

   /// The number of nodes in the DFS tree (==dfsnum-1).
   unsigned int nodes;

   /**
    * Blocks with bits set here have a fake edge to EXIT. These are used
    * to turn a DFS forest into a proper tree.
    */
   CustomOrderedSet<TBB> fake_exit_edge;

   /// index of the last basic block
   const unsigned long int last_basic_block;

   /// Start block (ENTRY_BLOCK_PTR for forward problem, EXIT_BLOCK for backward problem).
   const Vertex& en_block;

   /// Ending block.
   const Vertex& ex_block;

   /// Flow graph reference
   const GraphObj& g;

   /// After the algorithm is done, dom[x] contains the immediate dominator of x.
   std::vector<TBB> dom;

   /**
    * If b is the number of a basic block (index_map[bb_Vertex]), dfs_order[b] is the
    * number of that node in DFS order counted from 1.  This is an index
    * into most of the other arrays in this structure.
    */
   std::vector<TBB> dfs_order;
   /**
    * If x is the DFS-index of a node which corresponds with a basic block,
    * dfs_to_bb[x] is that basic block.  Note, that in our structure there are
    * more nodes that basic blocks, so only dfs_to_bb[dfs_order[index_map[bb]]]==bb
    * is true for every basic block bb, but not the opposite.
    */
   std::vector<Vertex> dfs_to_bb;

   /// index map putting into relation vertices and arbitrary indexes.
   std::map<Vertex, size_t> index_map;

   /// debug_level
   int debug_level;

   /**
    * The recursive variant of creating a DFS tree.
    * After this is done all nodes reachable from BB were visited, have
    * assigned their dfs number and are linked together to form a tree.
    * @param bb is the basic block Vertex BB.
    */
   void calc_dfs_tree_rec(Vertex bb)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing dfs tree starting from v_" + boost::lexical_cast<std::string>(bb));
      // bb is the parent Vertex
      /* We call this _only_ if bb is not already visited.  */
      edge e;
      TBB child_i, my_i = 0;
      typename boost::graph_traits<GraphObj>::out_edge_iterator ei, ei_end;

      for(boost::tie(ei, ei_end) = boost::out_edges(bb, g); ei != ei_end; ei++)
      {
         // bn is the child Vertex
         Vertex bn = boost::target(*ei, g);
         // if it has yet been visited we skip it
         if(dfs_order[index_map[bn]])
         {
            continue;
         }

         /* Fill the DFS tree info calculatable _before_ recursing.  */
         if(bb != en_block)
            my_i = dfs_order[index_map[bb]];
         else
            // In this way dfs_parent of entry is itself
            my_i = dfs_order[last_basic_block];
         // Computed a new dfs index for this Vertex
         child_i = dfs_order[index_map[bn]] = dfsnum++;
         dfs_to_bb[child_i] = bn;
         dfs_parent[child_i] = my_i;
         // recursive call
         calc_dfs_tree_rec(bn);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed dfs tree starting from v_" + boost::lexical_cast<std::string>(bb));
   }
   /**
    * Compress the path from V to the root of its set and update path_min at the
    * same time.  After compress(V) set_chain[V] is the root of the set V is
    * in and path_min[V] is the node with the smallest key[] value on the path
    * from V to that root.
    * @param v is the basic block ID of V.
    */
   void compress(TBB v)
   {
      /* Btw. It's not worth to unrecurse compress() as the depth is usually not
      greater than 5 even for huge graphs (I've not seen call depth > 4).
      Also performance wise compress() ranges _far_ behind eval().  */
      TBB parent = set_chain[v];
      if(set_chain[parent])
      {
         compress(parent);
         if(key[path_min[parent]] < key[path_min[v]])
            path_min[v] = path_min[parent];
         set_chain[v] = set_chain[parent];
      }
   }

   /**
    * Compress the path from V to the set root of V if needed (when the root
    * has changed since the last call).  Returns the node with the smallest
    * key[] value on the path from V to the root.
    * @param v is the basic block ID of V.
    */
   TBB eval(TBB v)
   {
      /* The representant of the set V is in, also called root (as the set
      representation is a tree).  */
      TBB rep = set_chain[v];
      /* V itself is the root.  */
      if(!rep)
      {
         return path_min[v];
      }

      /* Compress only if necessary.  */
      if(set_chain[rep])
      {
         compress(v);
         rep = set_chain[v];
      }

      if(key[path_min[rep]] >= key[path_min[v]])
         return path_min[v];
      else
         return path_min[rep];
   }

   /**
    * This essentially merges the two sets of V and W, giving a single set with the new root V.
    * The internal representation of these disjoint sets is a balanced tree.
    * Currently link(V,W) is only used with V being the
    * parent of W.
    */
   void link_roots(TBB v, TBB w)
   {
      TBB s = w;
      /* Rebalance the tree.  */
      while(key[path_min[w]] < key[path_min[set_child[s]]])
      {
         if(set_size[s] + set_size[set_child[set_child[s]]] >= 2 * set_size[set_child[s]])
         {
            set_chain[set_child[s]] = s;
            set_child[s] = set_child[set_child[s]];
         }
         else
         {
            set_size[set_child[s]] = set_size[s];
            s = set_chain[s] = set_child[s];
         }
      }
      path_min[s] = path_min[w];
      set_size[v] += set_size[w];
      if(set_size[v] < 2 * set_size[w])
      {
         TBB tmp = s;
         s = set_child[v];
         set_child[v] = tmp;
      }
      /* Merge all subtrees.  */
      while(s)
      {
         set_chain[s] = v;
         s = set_child[s];
      }
   }

 public:
   /**
    * Constructor
    * @param g is the starting graph
    * @param en_block is the entry block
    * @param ex_block is the exit_block,
    * @param param is the set of input parameters
    */
   dom_info(const GraphObj& _g, const Vertex& _en_block, const Vertex& _ex_block, const ParameterConstRef param)
       : dfs_parent(boost::num_vertices(_g) + 1, 0),
         key(boost::num_vertices(_g) + 1, 0),
         path_min(boost::num_vertices(_g) + 1, 0),
         bucket(boost::num_vertices(_g) + 1, 0),
         next_bucket(boost::num_vertices(_g) + 1, 0),
         set_chain(boost::num_vertices(_g) + 1, 0),
         set_size(boost::num_vertices(_g) + 1, 1),
         set_child(boost::num_vertices(_g) + 1, 0),
         dfsnum(1),
         nodes(0),
         last_basic_block(boost::num_vertices(_g)),
         en_block(_en_block),
         ex_block(_ex_block),
         g(_g),
         dom(boost::num_vertices(_g) + 1, 0),
         dfs_order(boost::num_vertices(_g) + 1, 0),
         dfs_to_bb(boost::num_vertices(_g) + 1, boost::graph_traits<GraphObj>::null_vertex()),
         debug_level(param->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE))
   {
      size_t index_counter = 0;
      Vertex_iterator v, v_end;
      for(boost::tie(v, v_end) = boost::vertices(g); v != v_end; v++, index_counter++)
      {
         index_map[*v] = index_counter;
      }
      for(unsigned int i = 0; i < boost::num_vertices(_g) + 1; i++)
      {
         path_min[i] = i;
         key[i] = i;
      }
   }

   /**
    * The main entry for calculating the DFS tree or forest.
    * @param reverse is true, if we are interested in the reverse flow graph.
    *        In that case the result is not necessarily a tree but a forest,
    *        because there may be nodes from which the EXIT_BLOCK is unreachable.
    * @param source is the ENTRY_BLOCK when reverse is false
    *        while it is the EXIT_BLOCK if reverse is true
    */
   void calc_dfs_tree(bool reverse)
   {
      Vertex begin = en_block;
      dfs_order[last_basic_block] = dfsnum;
      dfs_to_bb[dfsnum] = begin;
      dfsnum++;
      calc_dfs_tree_rec(begin);
      if(reverse)
      {
         /* In the post-dom case we may have nodes without a path to EXIT_BLOCK.
            They are reverse-unreachable.  In the dom-case we disallow such
            nodes, but in post-dom we have to deal with them.

            There are two situations in which this occurs.  First, noreturn
            functions.  Second, infinite loops.  In the first case we need to
            pretend that there is an edge to the exit block.  In the second
            case, we wind up with a forest.  We need to process all noreturn
            blocks before we know if we've got any infinite loops.  */

         Vertex b;
         bool saw_unconnected = false;

         Vertex_iterator vi, vi_end;
         for(boost::tie(vi, vi_end) = boost::vertices(g); vi != vi_end; vi++)
         {
            b = *vi;
            // skip entry
            if(en_block == b)
               continue;
            if(boost::in_degree(b, g) > 0)
            {
               if(dfs_order[index_map[b]] == 0)
                  saw_unconnected = true;
               continue;
            }
            fake_exit_edge.insert(index_map[b]);
            dfs_order[index_map[b]] = dfsnum;
            dfs_to_bb[dfsnum] = b;
            dfs_parent[dfsnum] = dfs_order[last_basic_block];
            dfsnum++;
            calc_dfs_tree_rec(b);
         }

         if(saw_unconnected)
         {
            for(boost::tie(vi, vi_end) = boost::vertices(g); vi != vi_end; vi++)
            {
               b = *vi;
               // skip entry
               if(en_block == b)
                  continue;
               if(dfs_order[index_map[b]])
                  continue;
               fake_exit_edge.insert(index_map[b]);
               dfs_order[index_map[b]] = dfsnum;
               dfs_to_bb[dfsnum] = b;
               dfs_parent[dfsnum] = dfs_order[last_basic_block];
               dfsnum++;
               calc_dfs_tree_rec(b);
            }
         }
      }

      nodes = dfsnum - 1;

      Vertex_iterator i, i_end;
#if HAVE_ASSERTS
      unsigned int counter = 0;
      for(boost::tie(i, i_end) = boost::vertices(g); i != i_end; i++)
         ++counter;
#endif
      /* This aborts e.g. when there is _no_ path from ENTRY to EXIT at all.  */
      THROW_ASSERT(nodes == counter,
                   "there is _no_ path from ENTRY to EXIT at all. Number of vertices in the graph: " + boost::lexical_cast<std::string>(num_vertices(g)) + " Number of reachable from entry vertices " + boost::lexical_cast<std::string>(nodes));
#ifndef NDEBUG
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Vertices in dfs order");
      for(size_t index = 0; index < dfs_to_bb.size(); index++)
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + boost::lexical_cast<std::string>(index) + " v_" + boost::lexical_cast<std::string>(dfs_to_bb[index]));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
#endif
   }

   /**
    * This calculates the immediate dominators (or post-dominators
    * if REVERSE is true).
    * On return the immediate dominator to node V is in dom[V].
    */
   void calc_idoms(bool reverse)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing immediate dominators");
      TBB v, w;
      typename boost::graph_traits<GraphObj>::in_edge_iterator ei, einext, ei_end, einext_end;

      /* Go backwards in DFS order, to first look at the leafs.  */
      v = nodes;
      while(v > 1)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing node in position " + boost::lexical_cast<std::string>(v) + " v_" + boost::lexical_cast<std::string>(dfs_to_bb[v]));
         Vertex bb = dfs_to_bb[v];
         edge e;
         bool do_fake_exit_edge = false;

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Parent is " + boost::lexical_cast<std::string>(dfs_parent[v]));
         TBB par = dfs_parent[v];
         TBB k = v;

         boost::tie(ei, ei_end) = boost::in_edges(bb, g);

         if(reverse)
         {
            /* If this block has a fake edge to exit, process that first.  */
            if(fake_exit_edge.find(index_map[bb]) != fake_exit_edge.end())
            {
               einext = ei;
               do_fake_exit_edge = true;
            }
         }

         /* Search all direct predecessors for the smallest node with a path
         to them.  That way we have the smallest node with also a path to
         us only over nodes behind us.  In effect we search for our
         semidominator.  */
         while(do_fake_exit_edge || ei != ei_end)
         {
            TBB k1;
            Vertex b;
            if(do_fake_exit_edge)
            {
               k1 = dfs_order[last_basic_block];
               do_fake_exit_edge = false;
            }
            else
            {
               e = *ei;
               b = boost::source(e, g);
               einext = ei;
               einext++;

               if(b == en_block)
                  k1 = dfs_order[last_basic_block];
               else
                  k1 = dfs_order[index_map[b]];
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Predecessor is " + boost::lexical_cast<std::string>(k1) + " v_" + boost::lexical_cast<std::string>(b));
            /* Call eval() only if really needed.  If k1 is above V in DFS tree,
            then we know, that eval(k1) == k1 and key[k1] == k1.  */
            // k1 is the dfs index of the predecessor, v is the dfs of this Vertex
            if(k1 > v)
               k1 = key[eval(k1)];
            if(k1 < k)
               k = k1;

            ei = einext;
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Key is " + boost::lexical_cast<std::string>(k));

         // k becomes the key for v, so we have to add v to the bucket of k. It becomes the first element because we are reverse counting v
         key[v] = k;
         link_roots(par, v);
         next_bucket[v] = bucket[k];
         bucket[k] = v;

         /* Transform semidominators into dominators.  */
         for(w = bucket[par]; w; w = next_bucket[w])
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---w is " + boost::lexical_cast<std::string>(w));
            k = eval(w);
            if(key[k] < key[w])
            {
               dom[w] = k;
            }
            else
            {
               dom[w] = par;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---dom[w] is " + boost::lexical_cast<std::string>(dom[w]));
         }
         /* We don't need to cleanup next_bucket[].  */
         bucket[par] = 0;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed node in position " + boost::lexical_cast<std::string>(v));
         v--;
      }

      /* Explicitly define the dominators.  */
      for(v = 1; v <= nodes; v++)
      {
         if(dom[v] != key[v])
         {
            dom[v] = dom[dom[v]];
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed immediate dominators");
   }

   /**
    * fill the dominator map after its computation
    * @param dom_map is the dominator map.
    */
   void fill_dom_map(CustomUnorderedMapStable<Vertex, Vertex>& dom_map)
   {
      Vertex_iterator vi, vi_end;
      dom_map[en_block] = en_block;
      for(boost::tie(vi, vi_end) = boost::vertices(g); vi != vi_end; vi++)
      {
         TBB d = dom[dfs_order[index_map[*vi]]];
         if(dfs_to_bb[d] != boost::graph_traits<GraphObj>::null_vertex())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---dom is " + boost::lexical_cast<std::string>(dfs_to_bb[d]));
            dom_map[*vi] = dfs_to_bb[d];
         }
      }
   }
};

/**
 * Dominator o post-dominator data structure.
 */
template <typename GraphObj>
class dominance
{
 public:
   /**
    * type of computation.
    */
   enum cdi_direction
   {
      CDI_DOMINATORS,
      CDI_POST_DOMINATORS,
      CDI_NONE
   };
   /// type of information stored in dom_info data structure
   enum dom_state
   {
      DOM_NONE,          /**< Not computed at all.  */
      DOM_NO_FAST_QUERY, /**< The data is OK, but the fast query data are not usable.  */
      DOM_OK             /**< Everything is ok.  */
   };

 private:
   /**
    * @name some friend classes
    */
   //@{
   friend class loop_regions_computation;
   friend class add_loop_nop;
   //@}

   /// Definition of Vertex
   typedef typename boost::graph_traits<const GraphObj>::vertex_descriptor Vertex;

   /// Vertex_iterator definition.
   typedef typename boost::graph_traits<const GraphObj>::vertex_iterator Vertex_iterator;

   /**
    * This variable store the direction for which the calculus is done
    */
   enum cdi_direction dom_dir;

   /// Whether the dominators and the postdominators are available.  */
   enum dom_state dom_computed;

   /// Flow graph reference
   const GraphObj& g;

   /// Start block (ENTRY_BLOCK_PTR for forward problem, EXIT_BLOCK for backward problem).
   const Vertex en_block;
   /// Ending block.
   const Vertex ex_block;

   /// After the algorithm is done, dom[x] contains the immediate dominator of x.
   CustomUnorderedMapStable<Vertex, Vertex> dom;

   /// The set of input parameters
   const ParameterConstRef param;

   /// Debug level
   int debug_level;

 public:
   /**
    * Deleted copy constructor for Weffc++
    */
   dominance(const dominance<GraphObj>&) = delete;

   /**
    * Deleted assignment operator for Weffc++
    */
   dominance<GraphObj> operator=(const dominance<GraphObj>&) = delete;
   /**
    * The main entry point into this module.
    * @param dir is set depending on whether we want to compute dominators or postdominators.
    */
   void calculate_dominance_info(enum dominance::cdi_direction dir)
   {
      if(dom_computed == DOM_OK)
         return;
      if(dom_computed == DOM_NONE)
      {
         bool reverse = (dir == CDI_POST_DOMINATORS) ? true : false;
         if(reverse)
         {
            boost::reverse_graph<GraphObj, GraphObj> Rcfg(g);
            /// store the intermediate information used to compute dominator or post dominator information
            dom_info<boost::reverse_graph<GraphObj, GraphObj>> di(Rcfg, ex_block, en_block, param);
            di.calc_dfs_tree(reverse);
            di.calc_idoms(reverse);
            di.fill_dom_map(dom);
         }
         else
         {
            /// store the intermediate information used to compute dominator or post dominator information
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing dominators");
            dom_info<GraphObj> di(g, en_block, ex_block, param);
            di.calc_dfs_tree(reverse);
            di.calc_idoms(reverse);
            di.fill_dom_map(dom);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed dominators");
         }
         dom_computed = DOM_NO_FAST_QUERY;
      }
      /// fast query not yet supported
      // compute_dom_fast_query (dir);
   }
   /**
    * Constructor for the dominance class.
    * @param _g is the starting graph
    * @param _en_block is the entry Vertex
    * @param _ex_block is the exit_Vertex
    * @param dl is the debug_level
    */
   dominance(const GraphObj& _g, const Vertex _en_block, const Vertex _ex_block, const ParameterConstRef _param)
       : dom_dir(CDI_NONE), dom_computed(DOM_NONE), g(_g), en_block(_en_block), ex_block(_ex_block), param(_param), debug_level(_param->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE))
   {
      THROW_ASSERT(_en_block != _ex_block, "incorrect entry and exit basic blocks");
   }
   /**
    * Return the immediate dominator of a Vertex
    * @param v is the node considered.
    * @return the node of the immediate dominator.
    */
   Vertex get_immediate_dominator(Vertex v) const
   {
      return dom.find(v)->second;
   }

   /**
    * Returns a map containing, for each Vertex, a set formed by all the
    * vertices dominated by it (all the dominator relationships, not
    * only the direct one are considered).
    */
   const CustomUnorderedMapStable<Vertex, CustomOrderedSet<Vertex>> getAllDominated() const
   {
      CustomUnorderedMapStable<Vertex, CustomOrderedSet<Vertex>> dominated;
      // These are the immediate dominated nodes
      auto dom_it_end = dom.end();
      for(auto dom_it = dom.begin(); dom_it != dom_it_end; ++dom_it)
      {
         dominated[dom_it->second].insert(dom_it->first);
         dominated[dom_it->first].insert(dom_it->first);
      }

      // Now I have to consider also the non immediate dominators
      bool changed = false;
      do
      {
         changed = false;
         // for(domBeg = this->dom.begin(), domEnd = this->dom.end(); domBeg != domEnd; domBeg++)
         for(auto dom_it = dom.begin(); dom_it != dom_it_end; ++dom_it)
         {
            typedef typename CustomUnorderedMapStable<Vertex, CustomOrderedSet<Vertex>>::iterator mSetIter;
            mSetIter mSetBeg, mSetEnd;
            for(mSetBeg = dominated.begin(), mSetEnd = dominated.end(); mSetBeg != mSetEnd; ++mSetBeg)
            {
               if((mSetBeg->second).find(dom_it->second) != (mSetBeg->second).end() && (mSetBeg->second).find(dom_it->first) == (mSetBeg->second).end())
               {
                  dominated[mSetBeg->first].insert(dom_it->first);
                  changed = true;
               }
            }
         }
      } while(changed);

      return dominated;
   }

   /**
    * Return the dominators tree as a map between Vertex and its immediate dominator
    */
   const CustomUnorderedMapStable<Vertex, Vertex>& get_dominator_map() const
   {
      return dom;
   }
};
#endif
