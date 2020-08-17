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
 * @file tree_manager.hpp
 * @brief Class specification of the manager of the tree structures extracted from the raw file.
 *
 * This class specifies the tree_manager node.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef TREE_MANAGER_HPP
#define TREE_MANAGER_HPP

#include "config_HAVE_UNORDERED.hpp"

#include "custom_map.hpp"
#include "custom_set.hpp"
#include "hash_helper.hpp"
/// utility include
#include "refcount.hpp"

/// STL include
#include <deque>
#include <iosfwd>
#include <string>  // for string
#include <utility> // for pair

/**
 * @name forward declarations
 */
//@{
CONSTREF_FORWARD_DECL(Parameter);
struct ssa_name;
struct function_decl;
enum kind : int;
REF_FORWARD_DECL(tree_manager);
CONSTREF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(tree_node);
enum class TreeVocabularyTokenTypes_TokenEnum;
//@}

/**
 * This class manages the tree structures extracted from the raw file.
 */
class tree_manager
{
 private:
   /// map between string and corresponding enum kinds
   CustomUnorderedMap<std::string, enum kind> string_to_kind;

   /// cache for tree_manager::find
   CustomUnorderedMapUnstable<std::string, unsigned int> find_cache;

   /**
    * Variable containing set of tree_nodes.
    */
#if HAVE_UNORDERED
   UnorderedMapStd<unsigned int, tree_nodeRef> tree_nodes;
#else
   OrderedMapStd<unsigned int, tree_nodeRef> tree_nodes;
#endif
   /**
    * Variable containing set of function_declaration with their index node
    */
   std::map<unsigned int, tree_nodeRef> function_decl_nodes;

   /// list of examining statements during collapse_into recursion
   std::deque<tree_nodeRef> stack;

   /// set of already examined addr_expr used to avoid circular recursion
   CustomUnorderedSet<tree_nodeRef> already_visited;

   /// the number of parallel loops
   unsigned int n_pl;

   /// the number of added goto
   unsigned int added_goto;

   /// the number of removed pointer_plus
   unsigned int removed_pointer_plus;

   /// the number of removable pointer_plus
   unsigned int removable_pointer_plus;

   /// the number of unremoved pointer_plus
   unsigned int unremoved_pointer_plus;

   /// The debug level
   int debug_level;

   /// last node_id used
   unsigned int last_node_id;

   /// this table stores all identifier_nodes with their nodeID.
   CustomUnorderedMapUnstable<std::string, unsigned int> identifiers_unique_table;

   CustomUnorderedMap<std::pair<long long int, unsigned int>, tree_nodeRef> unique_integer_cst_map;

   /// Set of parameters
   const ParameterConstRef Param;

   /// Map containing temporary information for ssa_name uses deletion
   CustomUnorderedMap<ssa_name*, tree_nodeRef> uses_erase_temp;

   /// Next version number for ssa variables
   unsigned int next_vers;

   /// Index of current call of collapse_into_counter
   unsigned int collapse_into_counter;

   /**
    * check for decl_node and return true if not suitable for symbol table or otherwise its symbol_name and symbol_scope.
    * @param tn is the tree node to be examinated
    * @param TM is a refcount to this TODO: could be removed?
    * @param symbol_name is where symbol name will be stored
    * @param symobl_scope is where symbol scope will be stored
    * @param node_id is the index of the tree node
    * @return true if symbol has not to be inserted into symbol table
    */
   bool check_for_decl(const tree_nodeRef& tn, const tree_managerRef& TM, std::string& symbol_name, std::string& symbol_scope, unsigned int node_id, const CustomUnorderedMap<unsigned int, std::string>& global_type_unql_symbol_table);

   /**
    * check for type and return true if not suitable for symbol table or otherwise its symbol_name and symbol_scope.
    */
   bool check_for_type(const tree_nodeRef& tn, const tree_managerRef& TM, std::string& symbol_name, std::string& symbol_scope, const CustomUnorderedMapUnstable<std::string, unsigned int>& global_type_symbol_table, unsigned int node_id);

   /**
    * Erase the information about variable usage (remove stmt from use_stmts attribute) in ssa variables recursively contained in node tn.
    * @param tn is the node from which the recursion begin.
    * @param stmt is the statement that is removed from the usage vector of ssa variables.
    */
   void erase_usage_info(const tree_nodeRef& tn, const tree_nodeRef& stmt);

   /**
    * Insert the information about variable usage (insert stmt in use_stmts attribute) in ssa variables recursively contained in node tn.
    * @param tn is the node from which the recursion begin.
    * @param stmt is the statement that is inserted in the usage vector of ssa variables.
    */
   void insert_usage_info(const tree_nodeRef& tn, const tree_nodeRef& stmt);

 public:
   /**
    * Replace the occurrences of tree node old_node with new_node in statement identified by tn.
    * Operates recursively.
    * NOTE: tn must be a reference since we are replacing the tree_node in other tree_node fields
    * @param tn is the statement from which the recursion begin. It must be a statement.
    * @param old_node is the node whose occurrences have to be replace
    * @param new_node is the node that replaces occurrences of old_node
    * @param stmt is the statement from which the recursion originates (necessary to update ssa_nodes usage information)
    * @param definition is true if old_node is a ssa_name in the left part of a gimple_assign
    */
   void RecursiveReplaceTreeNode(tree_nodeRef& tn, const tree_nodeRef old_node, const tree_nodeRef& new_node, const tree_nodeRef& stmt, bool definition); // NOLINT

   /**
    * This is the constructor of the tree_manager which initializes the vector of functions.
    * @param Param is the set of input parameters
    */
   explicit tree_manager(const ParameterConstRef& Param);

   ~tree_manager();

   /**
    * Return the index of function_decl node that implements
    * the declaration node
    * @param decl_node is the index of the declaration node
    + @return the index of implementation node
    */
   unsigned int get_implementation_node(unsigned int decl_node) const;

   // ************************+ handlers for tree_nodes structure **************************

   /**
    * Add to the tree manager the current node.
    * @param i position in the tree_vector. Note that the raw file start from 1
    * @param curr is the added element
    */
   void AddTreeNode(const unsigned int i, const tree_nodeRef& curr);

   /**
    * Return a tree_reindex wrapping the  i-th tree_node.
    * @param i is the index of the tree_node.
    * @return the reference to the tree_node.
    */
   tree_nodeRef GetTreeReindex(const unsigned int i);

   /**
    * Return a tree_reindex wrapping the  i-th tree_node.
    * @param i is the index of the tree_node.
    * @return the reference to the tree_node.
    */
   const tree_nodeRef CGetTreeReindex(const unsigned int i) const;

   /**
    * Return the index-th tree_node (modifiable version)
    * @param index is the index of the tree node to be returned
    * @return the index-the tree_node
    */
   tree_nodeRef GetTreeNode(const unsigned int index) const;

   /**
    * Return the reference to the i-th tree_node Constant version of get_tree_node.
    * @param i is the index of the tree_node of the considered function.
    * @return the reference to the tree_node.
    * FIXME: this should return tree_nodeConstRef
    */
   const tree_nodeRef get_tree_node_const(unsigned int i) const;
   const tree_nodeConstRef CGetTreeNode(const unsigned int i) const;

   /**
    * Return true if there exists a tree node associated with the given id, false otherwise
    * @param i is the index of the tree_node to be checked
    */
   bool is_tree_node(unsigned int i) const;

   /**
    * Factory method.
    * It creates a tree_node of type equal tree_node_type by using a relation (tree_node_schema) between tree node fields and their values.
    * @param node_id is the node id of the created object.
    * @param tree_node_type is the type of the node added to the tree_manager expressed as a treeVocabularyTokenTypes.
    * @param tree_node_schema expresses the value of the field of the tree node created.
    * For example the following code:
    * std::map<int, std::string> identifier_schema;
    * int identifier_node_id = TM->get_tree_node_n();
    * identifier_schema[TOK(TOK_STRG)]= "my_identifier";
    * TM->create_tree_node(identifier_node_id, TOK(TOK_IDENTIFIER_NODE), identifier_schema);
    * will add an identifier node to the tree_manager TM.
    */
   void create_tree_node(const unsigned int node_id, enum kind tree_node_type, std::map<TreeVocabularyTokenTypes_TokenEnum, std::string>& tree_node_schema);

   /**
    * if there exist return the node id of a tree node compatible with the tree_node_schema and of type tree_node_type.
    * @param tree_node_type is the type of the node added to the tree_manager expressed as a treeVocabularyTokenTypes.
    * @param tree_node_schema expresses the value of the field of the tree node created.
    */
   unsigned int find(enum kind tree_node_type, const std::map<TreeVocabularyTokenTypes_TokenEnum, std::string>& tree_node_schema);

   /**
    * Return a new node id in the intermediate representation.
    * @param ask is the requested id; if it is not used will be returned otherwise a new id
    * @return the new node is
    */
   unsigned int new_tree_node_id(const unsigned int ask = 0);

   /**
    * return the next available tree node id.
    */
   unsigned int get_next_available_tree_node_id() const;

   /**
    * Add to the function_decl_nodes the current node.
    * @param index is the index of the node
    * @param curr is the added element
    */
   void add_function(unsigned int index, tree_nodeRef curr);

   /**
    * Return the number of function_decl_node.
    * @return the number of function decl nodes.
    */
   unsigned long get_function_decl_node_n() const;

   /**
    * Returns all the functions in the tree_manager
    * @return all the functions
    */
   const CustomUnorderedSet<unsigned int> GetAllFunctions() const;

   /**
    * Determine the index node of "sc_main" function in tree_node vector
    */
   unsigned int find_sc_main_node() const;

   /**
    * Return the index of a function given its name
    * @param tm is the tree_manager
    * @param function_name is the name of the function
    * @return the treenode_index of the function_decl
    */
   unsigned int function_index(const std::string& function_name) const;

   /**
    * Return the index of a function given its mangled name
    * @param tm is the tree_manager
    * @param function_name is the mangled name of the function
    * @return the treenode_index of the function_decl
    */
   unsigned int function_index_mngl(const std::string& function_name) const;

   /**
    * Function that prints the class tree_manager.
    * @param Param is the set of the parameter
    * @param os is the stream where tree_manager will be printed
    */
   void print(std::ostream& os) const;

   /**
    * Function that prints the bodies of function in gimple format
    * @param os is the stream where bodies will be printed
    * @param use_uid tells if uid of gimple index has to be used
    */
   void PrintGimple(std::ostream& os, const bool use_uid) const;

   /**
    * Friend definition of the << operator.
    */
   friend std::ostream& operator<<(std::ostream& os, const tree_manager& s)
   {
      s.print(os);
      return os;
   }

   /**
    * Friend definition of the << operator. Pointer version.
    */
   friend std::ostream& operator<<(std::ostream& os, const tree_managerRef& s)
   {
      if(s)
         s->print(os);
      return os;
   }

   /**
    * Collapse operations chains into the examinated node
    * @param funID is the index of the function
    * @param stmt_to_block maps each statement to its bloc
    * @param tn is the top tree node of the tree to be collapsed
    * @param removed_nodes is the set of nodes removed during collapsing
    */
   void collapse_into(const unsigned int& funID, CustomUnorderedMapUnstable<unsigned int, unsigned int>& stmt_to_bloc, const tree_nodeRef& tn, CustomUnorderedSet<unsigned int>& removed_nodes);

   /// increment the number a parallel loop
   void add_parallel_loop();

   /// return the number of parallel loops
   unsigned int get_n_pl() const;

   /**
    * merge two tree manager: this with TM_source
    * @param TM_source is one of the tree_manager.
    */
   void merge_tree_managers(const tree_managerRef& TM_source);

   /**
    * Increment the number of added gotos
    */
   void add_goto();

   /**
    * Return the number of added gotos
    */
   unsigned int get_added_goto() const;

   /**
    * Increment the number of removed pointer plus
    */
   void increment_removed_pointer_plus();

   /**
    * Return the number of removed pointer plus
    */
   unsigned int get_removed_pointer_plus() const;

   /**
    * Increment the number of removable pointer plus
    */
   void increment_removable_pointer_plus();

   /**
    * Return the number of not removed pointer plus
    */
   unsigned int get_removable_pointer_plus() const;

   /**
    * Increment the number of not removed pointer plus
    */
   void increment_unremoved_pointer_plus();

   /**
    * Return the number of not removed pointer plus
    */
   unsigned int get_unremoved_pointer_plus() const;

   /**
    * Return the nodeID of the identifier_node representing string str.
    * In case there is not that identifier_node it returns 0.
    * @param str is the identifier we are looking for.
    */
   unsigned int find_identifier_nodeID(const std::string& str) const;

   /**
    * Add an identifier_node to the corresponding unique table
    * @param nodeID is the node id.
    * @param str is the string.
    */
   void add_identifier_node(unsigned int nodeID, const std::string& str)
   {
      identifiers_unique_table[str] = nodeID;
   }
   void add_identifier_node(unsigned int nodeID, const bool& op);

   /**
    * Return the next unused version number for ssa variables
    */
   unsigned int get_next_vers();

   /**
    * Replace the occurrences of tree node old_node with new_node in statement identified by tn.
    * Operates recursively.
    * @param tn is the statement from which the recursion begin. It must be a statement.
    * @param old_node is the node whose occurrences have to be replace
    * @param new_node is thenode that replaces occurrences of old_node
    */
   void ReplaceTreeNode(const tree_nodeRef& tn, const tree_nodeRef& old_node, const tree_nodeRef& new_node);

   /**
    * memoization of integer constants
    * @param value is the integer value
    * @param type_index is the type of the integer constant
    * @return a tree reindex node for the integer value with as type type_index
    */
   tree_nodeRef CreateUniqueIntegerCst(long long int value, unsigned int type_index);

   /**
    * @brief is_CPP return true in case we have at least one CPP source code
    * @return true when at least one translation unit is written in C++
    */
   bool is_CPP() const;

   /**
    * @brief is_top_function checks if a function is one of the application top functions.
    * @param fd is the function decl
    * @return true in case fd is a top function
    */
   bool is_top_function(const function_decl* fd) const;

   /**
    * @brief check_ssa_uses check if the uses of a ssa are correct
    * @return true in case all the ssa uses are correct, false otherwise.
    */
   bool check_ssa_uses(unsigned int fun_id) const;
};
typedef refcount<tree_manager> tree_managerRef;
typedef refcount<const tree_manager> tree_managerConstRef;
#endif
