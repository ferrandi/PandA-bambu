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
 * @file ext_tree_node.hpp
 * @brief Classes specification of the tree_node data structures not present in the gcc.
 *
 * Classes used to described the tree nodes not imported from the raw file.
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $
 *
 */
#ifndef EXT_TREE_NODE_HPP
#define EXT_TREE_NODE_HPP

#include "custom_map.hpp"  // for unordered_map
#include "tree_common.hpp" // for GET_KIND, blackbox_pragma_K, call_hw_prag...
#include "tree_node.hpp"
#include <list>    // for list
#include <string>  // for string
#include <utility> // for pair

struct null_node : public tree_node
{
   /// constructor
   explicit null_node(unsigned int i) : tree_node(i)
   {
   }

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(null_node)

   /// Redefinition of get_kind.
   GET_KIND(null_node)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(tree_node) = 0
   };
};

struct gimple_pragma : public gimple_node
{
   /// constructor
   explicit gimple_pragma(unsigned int i) : gimple_node(i), is_block(false), is_opening(true)
   {
   }

   /// attribute for pragma: true when the pragma refers to a block
   bool is_block;

   /// attribute for pragma: true when the pragma refers to an opening pragma
   bool is_opening;

   /// this node defines the scope of the pragma
   tree_nodeRef scope;

   /// this node represents the directive of the pragma
   tree_nodeRef directive;

   /// this is the string-based form of the pragma
   std::string line;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(gimple_pragma)

   /// Redefinition of get_kind.
   GET_KIND(gimple_pragma)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(gimple_node) = 0,
      GETID(scope),
      GETID(directive)
   };
};

struct profiling_pragma : public tree_node
{
   /// constructor
   explicit profiling_pragma(unsigned int i) : tree_node(i)
   {
   }

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(profiling_pragma)

   /// Redefinition of get_kind.
   GET_KIND(profiling_pragma)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(tree_node) = 0
   };
};

struct statistical_profiling : public profiling_pragma
{
   /// constructor
   explicit statistical_profiling(unsigned int i) : profiling_pragma(i)
   {
   }

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(statistical_profiling)

   /// Redefinition of get_kind.
   GET_KIND(statistical_profiling)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(profiling_pragma) = 0
   };
};

struct map_pragma : public tree_node
{
   /// constructor
   explicit map_pragma(unsigned int i) : tree_node(i)
   {
   }

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(map_pragma)

   /// Redefinition of get_kind.
   GET_KIND(map_pragma)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(tree_node) = 0
   };
};

/**
 * Directive represinting mapping of a software function on a component
 */
struct call_hw_pragma : public map_pragma
{
   /// constructor
   explicit call_hw_pragma(unsigned int i) : map_pragma(i)
   {
   }

   /// The name of the component
   std::string HW_component;

   /// The identifier of the implementation
   std::string ID_implementation;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(call_hw_pragma)

   /// Redefinition of get_kind.
   GET_KIND(call_hw_pragma)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(map_pragma) = 0
   };
};

/**
 * Directive represinting mapping of a function call on a component
 */
struct call_point_hw_pragma : public map_pragma
{
   /// constructor
   explicit call_point_hw_pragma(unsigned int i) : map_pragma(i), recursive(false)
   {
   }

   /// The name of the component
   std::string HW_component;

   /// The identifier of the implementation
   std::string ID_implementation;

   /// True if this mapping pragma has to be considered recursive
   bool recursive;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(call_point_hw_pragma)

   /// Redefinition of get_kind.
   GET_KIND(call_point_hw_pragma)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(map_pragma) = 0
   };
};

struct issue_pragma : public tree_node
{
   /// constructor
   explicit issue_pragma(unsigned int i) : tree_node(i)
   {
   }

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(issue_pragma)

   /// Redefinition of get_kind.
   GET_KIND(issue_pragma)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(tree_node) = 0
   };
};

struct blackbox_pragma : public issue_pragma
{
   /// constructor
   explicit blackbox_pragma(unsigned int i) : issue_pragma(i)
   {
   }

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(blackbox_pragma)

   /// Redefinition of get_kind.
   GET_KIND(blackbox_pragma)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(issue_pragma) = 0
   };
};

struct omp_pragma : public tree_node
{
   /// constructor
   explicit omp_pragma(unsigned int i) : tree_node(i)
   {
   }

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(omp_pragma)

   /// Redefinition of get_kind.
   GET_KIND(omp_pragma)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(tree_node) = 0
   };
};

struct omp_parallel_pragma : public omp_pragma
{
   /// constructor
   explicit omp_parallel_pragma(unsigned int i) : omp_pragma(i), is_shortcut(false)
   {
   }

   /// map between the clauses that can be associated with the OpenMP parallel pragma and their value (e.g. number of threads)
   CustomUnorderedMapUnstable<std::string, std::string> clauses;

   /// flag to check if this pragma is shortcut with a OpenMP sections pragma
   bool is_shortcut;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(omp_parallel_pragma)

   /// Redefinition of get_kind.
   GET_KIND(omp_parallel_pragma)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(omp_pragma) = 0
   };
};

struct omp_sections_pragma : public omp_pragma
{
   /// constructor
   explicit omp_sections_pragma(unsigned int i) : omp_pragma(i), is_shortcut(false)
   {
   }

   /// flag to check if this pragma is shortcut with a OpenMP parallel pragma
   bool is_shortcut;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(omp_sections_pragma)

   /// Redefinition of get_kind.
   GET_KIND(omp_sections_pragma)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(omp_pragma) = 0
   };
};

struct omp_parallel_sections_pragma : public omp_pragma
{
   /// constructor
   explicit omp_parallel_sections_pragma(unsigned int i) : omp_pragma(i)
   {
   }

   /// The parallel part (the tree_node is a omp_parallel_pragma one)
   tree_nodeRef op0;

   /// The sections part (the tree_node is a omp_sections_pragma one)
   tree_nodeRef op1;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(omp_parallel_sections_pragma)

   /// Redefinition of get_kind.
   GET_KIND(omp_parallel_sections_pragma)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(omp_pragma) = 0,
      GETID(op0),
      GETID(op1)
   };
};

struct omp_section_pragma : public omp_pragma
{
   /// constructor
   explicit omp_section_pragma(unsigned int i) : omp_pragma(i)
   {
   }

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(omp_section_pragma)

   /// Redefinition of get_kind.
   GET_KIND(omp_section_pragma)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(omp_pragma) = 0
   };
};

struct omp_target_pragma : public omp_pragma
{
   /// Clauses associated with the directives
   CustomUnorderedMapUnstable<std::string, std::string> clauses;

   /// constructor
   explicit omp_target_pragma(unsigned int i);

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(omp_target_pragma)

   /// Redefinition of get_kind.
   GET_KIND(omp_target_pragma)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(omp_pragma) = 0
   };
};

struct omp_task_pragma : public omp_pragma
{
   /// Clauses associated with the directives
   CustomUnorderedMapUnstable<std::string, std::string> clauses;

   /// constructor
   explicit omp_task_pragma(unsigned int i);

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(omp_task_pragma)

   /// Redefinition of get_kind.
   GET_KIND(omp_task_pragma)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(omp_pragma) = 0
   };
};

struct omp_critical_pragma : public omp_pragma
{
   /// Clauses associated with the directives
   CustomUnorderedMapUnstable<std::string, std::string> clauses;

   /// constructor
   explicit omp_critical_pragma(unsigned int i);

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(omp_critical_pragma)

   /// Redefinition of get_kind.
   GET_KIND(omp_critical_pragma)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(omp_pragma) = 0
   };
};

struct omp_atomic_pragma : public omp_pragma
{
   /// constructor
   explicit omp_atomic_pragma(unsigned int i) : omp_pragma(i)
   {
   }

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(omp_atomic_pragma)

   /// Redefinition of get_kind.
   GET_KIND(omp_atomic_pragma)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(omp_pragma) = 0
   };
};

struct omp_for_pragma : public omp_pragma
{
   /// constructor
   explicit omp_for_pragma(unsigned int i) : omp_pragma(i)
   {
   }

   /// map between the clauses that can be associated with the OpenMP parallel pragma and their value (e.g. number of threads)
   CustomUnorderedMapUnstable<std::string, std::string> clauses;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(omp_for_pragma)

   /// Redefinition of get_kind.
   GET_KIND(omp_for_pragma)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(omp_pragma) = 0
   };
};

struct omp_simd_pragma : public omp_pragma
{
   /// constructor
   explicit omp_simd_pragma(unsigned int i) : omp_pragma(i)
   {
   }

   /// map between the clauses that can be associated with the OpenMP parallel pragma and their value (e.g. number of threads)
   CustomUnorderedMapUnstable<std::string, std::string> clauses;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(omp_simd_pragma)

   /// Redefinition of get_kind.
   GET_KIND(omp_simd_pragma)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(omp_pragma) = 0
   };
};

struct omp_declare_simd_pragma : public omp_pragma
{
   /// constructor
   explicit omp_declare_simd_pragma(unsigned int i) : omp_pragma(i)
   {
   }

   /// map between the clauses that can be associated with the OpenMP parallel pragma and their value (e.g. number of threads)
   CustomUnorderedMapUnstable<std::string, std::string> clauses;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(omp_declare_simd_pragma)

   /// Redefinition of get_kind.
   GET_KIND(omp_declare_simd_pragma)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(omp_pragma) = 0
   };
};

/**
 * This struct specifies the while expression
 * Used to represent a while construct
 */
struct gimple_while : public gimple_node
{
   /// constructor
   explicit gimple_while(unsigned int i) : gimple_node(i)
   {
   }

   /// The boolean condition
   tree_nodeRef op0;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(gimple_while)

   /// Redefinition of get_kind.
   GET_KIND(gimple_while)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(gimple_node) = 0,
      GETID(op0)
   };
};

/**
 * This struct specifies the for expression
 * Used to represent a for construct
 */
struct gimple_for : public gimple_while
{
   /// constructor
   explicit gimple_for(unsigned int i) : gimple_while(i)
   {
   }

   /// initialization
   tree_nodeRef op1;

   /// postincrement
   tree_nodeRef op2;

   /// omp pragma for
   tree_nodeRef omp_for;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(gimple_for)

   /// Redefinition of get_kind.
   GET_KIND(gimple_for)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(gimple_while) = 0,
      GETID(op1),
      GETID(op2)
   };
};

/**
 * This struct specifies a multi-way-if construct
 */
struct gimple_multi_way_if : public gimple_node
{
   /// constructor
   explicit gimple_multi_way_if(unsigned int i) : gimple_node(i)
   {
   }

   /// The list of pair condition basic block
   std::list<std::pair<tree_nodeRef, unsigned int>> list_of_cond;

   /**
    * Add a pair <cond, bb_index> to the list of cond.
    * @param cond is the condition.
    * @param bb_ind is the basic block index.
    */
   void add_cond(const tree_nodeRef& cond, unsigned int bb_ind);

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(gimple_multi_way_if)

   /// Redefinition of get_kind.
   GET_KIND(gimple_multi_way_if)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(gimple_node) = 0,
      GETID(list_of_cond)
   };
};

#endif
