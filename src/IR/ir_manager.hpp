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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file ir_manager.hpp
 * @brief Class specification of the manager of the IR structures extracted from the raw file.
 *
 * This class specifies the ir_manager node.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef IR_MANAGER_HPP
#define IR_MANAGER_HPP

#include "custom_map.hpp"
#include "custom_set.hpp"
#include "hash_helper.hpp"
#include "panda_types.hpp"
#include "refcount.hpp"

#include <deque>
#include <iosfwd>
#include <string>
#include <utility>

CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(ir_node);
REF_FORWARD_DECL(application_manager);
REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_node);
enum class IRVocabularyTokenTypes_TokenEnum;
enum kind : int;
struct function_val_node;
struct ssa_node;

#define BUILTIN_LOCINFO "<built-in>:0:0"

/**
 * This class manages the IR structures extracted from the raw file.
 */
class ir_manager
{
 private:
   /// cache for ir_manager::find
   CustomUnorderedMapUnstable<std::string, unsigned int> find_cache;

   /**
    * Variable containing set of ir_nodes.
    */
   std::vector<ir_nodeRef> ir_nodes;

   /// partition map
   std::map<kind, std::map<unsigned int, ir_nodeRef>> partitionMap;

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

   CustomUnorderedMap<std::pair<std::string, unsigned int>, ir_nodeRef> unique_cst_map;

   /// Set of parameters
   const ParameterConstRef Param;

   /// Next version number for ssa variables
   unsigned int next_vers;

   /**
    * check for decl_node and return true if not suitable for symbol table or otherwise its symbol_name and
    * symbol_scope.
    * @param tn is the IR node to be examinated
    * @param symbol_name is where symbol name will be stored
    * @param symbol_scope is where symbol scope will be stored
    * @param node_id is the index of the IR node
    * @param global_type_unql_symbol_table is the table of globally unique type symbols used during declaration checks
    * @return true if symbol has not to be inserted into symbol table
    */
   bool check_for_decl(const ir_nodeRef& tn, std::string& symbol_name, std::string& symbol_scope, unsigned int node_id,
                       const CustomUnorderedMap<unsigned int, std::string>& global_type_unql_symbol_table);

   /**
    * check for type and return true if not suitable for symbol table or otherwise its symbol_name and symbol_scope.
    */
   bool check_for_type(const ir_nodeRef& tn, std::string& symbol_name, std::string& symbol_scope,
                       const CustomUnorderedMapUnstable<std::string, unsigned int>& global_type_symbol_table,
                       unsigned int node_id);

   /**
    * Erase the information about variable usage (remove stmt from use_stmts attribute) in ssa variables recursively
    * contained in node tn.
    * @param tn is the node from which the recursion begin.
    * @param stmt is the statement that is removed from the usage vector of ssa variables.
    */
   void erase_usage_info(const ir_nodeRef& tn, const ir_nodeRef& stmt);

   /**
    * Insert the information about variable usage (insert stmt in use_stmts attribute) in ssa variables recursively
    * contained in node tn.
    * @param tn is the node from which the recursion begin.
    * @param stmt is the statement that is inserted in the usage vector of ssa variables.
    */
   void insert_usage_info(const ir_nodeRef& tn, const ir_nodeRef& stmt);

   ir_nodeRef create_unique_const(const std::string& val, const ir_nodeConstRef& type);

 public:
   using IRSchema = std::map<IRVocabularyTokenTypes_TokenEnum, std::string>;
   /**
    * Replace the occurrences of IR node old_node with new_node in statement identified by tn.
    * Operates recursively.
    * NOTE: tn must be a reference since we are replacing the ir_node in other ir_node fields
    * @param tn is the statement from which the recursion begin. It must be a statement.
    * @param old_node is the node whose occurrences have to be replace
    * @param new_node is the node that replaces occurrences of old_node
    * @param stmt is the statement from which the recursion originates (necessary to update ssa_nodes usage information)
    * @param definition is true if old_node is a ssa_node in the left part of a assign_stmt
    * @param use_counting true if use counting must be updated else false
    */
   void RecursiveReplaceIRNode(ir_nodeRef& tn, const ir_nodeRef old_node, const ir_nodeRef& new_node,
                               const ir_nodeRef& stmt, bool definition, bool use_counting);

   /**
    * This is the constructor of the ir_manager which initializes the vector of functions.
    * @param Param is the set of input parameters
    */
   explicit ir_manager(const ParameterConstRef& Param);

   /**
    * @brief Reserve additional space for given amount of IR nodes
    *
    * @param additional_reserved_nodes Number of additional nodes to reserve space for
    */
   void add_reserve(size_t additional_reserved_nodes);

   /**
    * @brief Replace all IR reindex occurrences with the pointed IR node
    *
    */
   void FixIRReindex();

   // ************************+ handlers for ir_nodes structure **************************

   /**
    * Add to the IR manager the current node.
    * @param curr is the added element
    */
   void AddIRNode(const ir_nodeRef& curr);

   /**
    * Return a ir_reindex wrapping the  i-th ir_node.
    * @param i is the index of the ir_node.
    * @return the reference to the ir_node.
    */
   ir_nodeRef GetIRReindex(const unsigned int i);

   /**
    * Return the index-th ir_node (modifiable version)
    * @param index is the index of the IR node to be returned
    * @return the index-the ir_node
    */
   ir_nodeRef GetIRNode(const unsigned int index) const;

   /**
    * Factory method.
    * It creates an ir_node of type ir_node_type by using an ir_node_schema mapping
    * IR node fields to their values.
    * @param node_id is the node id of the created object.
    * @param ir_node_type is the type of the node added to the ir_manager expressed as a irVocabularyTokenTypes.
    * @param ir_node_schema expresses the value of the field of the IR node created.
    * For example the following code:
    * std::map<int, std::string> identifier_schema;
    * int identifier_node_id = TM->new_ir_node_id();
    * identifier_schema[TOK(TOK_STRG)]= "my_identifier";
    * TM->create_ir_node(identifier_node_id, TOK(TOK_IDENTIFIER_NODE), identifier_schema);
    * will add an identifier node to the ir_manager TM.
    */
   ir_nodeRef create_ir_node(const unsigned int node_id, enum kind ir_node_type, const IRSchema& ir_node_schema);

   ir_nodeRef create_ir_node(enum kind ir_node_type, const IRSchema& ir_node_schema);

   /**
    * if there exist return the node id of an IR node compatible with the ir_node_schema and of type ir_node_type.
    * @param ir_node_type is the type of the node added to the ir_manager expressed as a irVocabularyTokenTypes.
    * @param ir_node_schema expresses the value of the field of the IR node created.
    */
   unsigned int find0(enum kind ir_node_type, const IRSchema& ir_node_schema);

   /**
    * Return a new node id in the intermediate representation.
    * @return the new node is
    */
   unsigned int new_ir_node_id();

   /**
    * return the next available IR node id.
    */
   unsigned int get_next_available_ir_node_id() const;

   /**
    * Add the current node to a partition.
    * @param tag is the partition tag
    * @param index is the index of the node
    * @param curr is the added element
    */
   void add_to_partition(enum kind tag, unsigned int index, ir_nodeRef curr);

   /**
    * Returns all the functions in the ir_manager
    * @return all the functions
    */
   const CustomOrderedSet<unsigned int> GetAllFunctions() const;

   /**
    * Determine the index node of "sc_main" function in ir_node vector
    */
   unsigned int find_sc_main_node() const;

   /**
    * Return the index of a function given its name
    * @param function_name is the name of the function
    * @return the IR node_index of the function_val_node
    */
   /// FIXME: to be remove after substitution with GetFunction
   unsigned int function_index(const std::string& function_name) const;

   /**
    * Return the index of a function given its name
    * @param function_name is the name of the function
    * @return the IR node of the function_val_node
    */
   ir_nodeRef GetFunction(const std::string& function_name) const;

   /**
    * Return the index of a function given its mangled name
    * @param function_name is the mangled name of the function
    * @return the IR node_index of the function_val_node
    */
   unsigned int function_index_mngl(const std::string& function_name) const;

   /**
    * Function that prints the class ir_manager.
    * @param os is the stream where ir_manager will be serialized
    */
   void print(std::ostream& os) const;

   /**
    * write the IR as LLVM descripion
    * @param os is the stream where ir_manager will be serialized
    */
   void PrintBambuLLVM(std::ostream& os) const;

   /**
    * Friend definition of the << operator.
    */
   friend std::ostream& operator<<(std::ostream& os, const ir_manager& s)
   {
      s.print(os);
      return os;
   }

   /**
    * Friend definition of the << operator. Pointer version.
    */
   friend std::ostream& operator<<(std::ostream& os, const ir_managerRef& s)
   {
      if(s)
      {
         s->print(os);
      }
      return os;
   }

   /**
    * merge two IR manager: this with TM_source
    * @param source_ir_manager is the IR manager merged into this instance.
    */
   void merge_ir_managers(const ir_managerRef& source_ir_manager);

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

   /**
    * Return the next unused version number for ssa variables
    */
   unsigned int get_next_vers();

   /**
    * Replace the occurrences of IR node old_node with new_node in statement identified by tn.
    * Operates recursively.
    * @param stmt is the statement from which the recursion begin. It must be a statement.
    * @param old_node is the node whose occurrences have to be replace
    * @param new_node is thenode that replaces occurrences of old_node
    * @param use_counting true if use counting must be updated else false
    */
   void ReplaceIRNode(const ir_nodeRef& stmt, const ir_nodeRef& old_node, const ir_nodeRef& new_node,
                      bool use_counting = true);

   /**
    * memoization of integer constants
    * @param value is the integer value
    * @param type is the type of the integer constant
    * @return an IR reindex node for the integer value with as type type
    */
   ir_nodeRef CreateUniqueIntegerCst(integer_cst_t value, const ir_nodeConstRef& type);

   /**
    * memoization of integer constants
    * @param value is the real value
    * @param type is the type of the real constant
    * @return an IR reindex node for the real value with as type type
    */
   ir_nodeRef CreateUniqueRealCst(long double value, const ir_nodeConstRef& type);

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
   bool is_top_function(const function_val_node* fd) const;
};
#endif
