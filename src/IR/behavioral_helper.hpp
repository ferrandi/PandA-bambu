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
 * @file behavioral_helper.hpp
 * @brief Helper for reading data about internal representation after graph_manager analysis
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef BEHAVIORAL_HELPER_HPP
#define BEHAVIORAL_HELPER_HPP

#include "custom_map.hpp"
#include "custom_set.hpp"
#include "graph.hpp"
#include "ir_node.hpp"
#include "refcount.hpp"

#include <list>
#include <string>
#include <tuple>
#include <utility>

class OpGraph;
CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(ir_manager);
CONSTREF_FORWARD_DECL(ir_node);
REF_FORWARD_DECL(application_manager);
REF_FORWARD_DECL(bloc);
REF_FORWARD_DECL(ir_node);
struct var_pp_functor;

using ir_class = unsigned int;

#define INTERNAL "internal_"

class BehavioralHelper
{
 protected:
   /// The application manager
   const application_managerRef AppM;

   /// The IR manager
   const ir_managerConstRef TM;

   /// The set of input parameters
   const ParameterConstRef Param;

   /// the debug level
   int debug_level;

   /// Maps between unqualified type and definition of a corresponding qualified type
   static std::map<unsigned int, std::pair<std::string, int>> definitions;

   /// Set of variables name already used
   static std::map<std::string, unsigned int> used_name;

   /// The var symbol table
   static std::map<unsigned int, std::string> vars_symbol_table;

   /// Variable renaming table
   static std::map<unsigned int, std::string> vars_renaming_table;

   /// Index of the function
   unsigned int function_index;

   /// Structure which stores initializations
   std::map<unsigned int, unsigned int> initializations;

   /**
    * Return true if the variable is a field_val_node and it has a bitfield.
    */
   bool has_bit_field(unsigned int variable) const;

 public:
   /**
    * Constructor
    * @param AppM is the application manager
    * @param index is the index of the function_val_node
    * @param parameters is the set of input parameters
    */
   BehavioralHelper(const application_managerRef AppM, unsigned int index, const ParameterConstRef parameters);

   /**
    * Print the operations corrisponding to the vertex
    * @param g is the graph
    * @param v is the vertex
    * @param vppf is the functor used to dump the variable var.
    * @param dot tells if the output is a dot graph
    */
   std::string print_vertex(const OpGraph& g, gc_vertex_descriptor v, const std::unique_ptr<var_pp_functor>& vppf,
                            const bool dot = false) const;

   /**
    * Print the initialization part
    * @param var is the initialization expression.
    * @param vppf is the functor used to dump the variable var.
    */
   std::string PrintInit(const ir_nodeConstRef& var, const std::unique_ptr<var_pp_functor>& vppf) const;

   /**
    * Print the name of the variable associated to the index
    * @param var is the considered variable.
    * @return the name of the variable
    */
   std::string PrintVariable(unsigned int var) const;

   /**
    * Invalidate cached variable name
    * @param index is the variable whose name must be invalidated
    */
   void InvaildateVariableName(const unsigned int index);

   /**
    * Print the constant associated with var
    * @param var is the constant IR node
    * @param vppf is the functor used to dump the variable var.
    */
   std::string PrintConstant(const ir_nodeConstRef& var, const std::unique_ptr<var_pp_functor>& vppf = nullptr) const;

   /**
    * Print a type.
    * @param type is the type to print.
    * @return the printed string
    */
   std::string print_type(unsigned int type) const;

   /**
    * Print the declaration of a non built-in type.
    * @param type is an object type.
    */
   std::string print_type_declaration(unsigned int type) const;

   /**
    * Return the size in bit of a C object
    * @param var is the index of a C object
    * @return the size in bit
    */
   unsigned long long get_size(unsigned int var) const;

   /**
    * Return the name of the function
    */
   std::string GetFunctionName() const;

   /**
    * Return the index of the function
    */
   unsigned int get_function_index() const;

   /**
    * Return the index associated with the type of the return of the current function
    * @return the index of the type
    */
   unsigned int
   /// FIXME: to be remove after substitution with ir_helper::GetFunctionReturnType
   GetFunctionReturnType() const;

   /**
    * Return true if index is a variable or a type of type bool
    */
   bool is_bool(unsigned int index) const;

   /**
    * Return true if index is a variable or a type of type int
    */
   bool is_int(unsigned int index) const;

   /**
    * Return true if index is a variable or a type of type unsigned int
    */
   bool is_unsigned(unsigned int index) const;

   /**
    * Return true if index is a variable or a type of type real
    */
   bool is_real(unsigned int index) const;

   /**
    * Return true if index is a variable or a type of type struct
    */
   bool is_a_struct(unsigned int variable) const;

   /**
    * Return true if index is a variable or a type of type array
    */
   bool is_an_array(unsigned int variable) const;

   /**
    * Return true if index is a variable or a type of type vector
    */
   bool is_a_vector(unsigned int variable) const;

   /**
    * Return true if index is a variable or a type of type pointer
    */
   bool is_a_pointer(unsigned int variable) const;

   /**
    * Return true if the index is an indirect ref.
    * @param variable is a variable.
    */
   bool is_an_indirect_ref(unsigned int variable) const;

   /**
    * Return true if the index is an addr_node
    * @param variable is the index
    */
   bool is_an_addr_node(unsigned int variable) const;

   /**
    * Return true if the index is a mem_access_node
    * @param variable is the index
    */
   bool is_a_mem_access(unsigned int variable) const;

   /**
    * Return true if the index is a constant object
    * @param obj is a nodeID.
    * @return true if it's a constant object
    */
   bool is_a_constant(unsigned int obj) const;

   /**
    * Return true if function is an operating system function
    * @param obj is the index of the node
    * @return true if obj is a library system function
    */
   bool is_operating_system_function(const unsigned int obj) const;

   /**
    * Return the nodeID of the last statement of a basic block in case that statement is a cond or a goto expression.
    * @param block is basic block reference.
    */
   unsigned int end_with_a_cond_or_goto(const blocRef& block) const;

   /**
    * Return the variable of an indirect ref.
    * @param obj is an indirect_ref object.
    */
   unsigned int get_indirect_ref_var(unsigned int obj) const;

   /**
    * Return the base of a mem ref.
    * @param obj is a mem_access_node object.
    */
   unsigned int get_mem_access_base(unsigned int obj) const;

   /**
    * Return the index of the operand if index is addr_node
    * @param obj is the index of the expression
    * @return the index of the operand
    */
   unsigned int get_operand_from_unary_node(unsigned int obj) const;

   /**
    * Return the index of the variable base of a ssa var
    * @param index is the index of the ssa var
    * @return the index of the base variable
    */
   unsigned int GetVarFromSsa(unsigned int index) const;

   /**
    * Return the type of the variable
    * @param var is the index of variable
    */
   unsigned int get_type(const unsigned int var) const;

   /**
    * Return the pointed type of a pointer
    * @param type is a pointer object
    */
   unsigned int get_pointed_type(const unsigned int type) const;

   /**
    * Given an array or a vector return the element type
    * @param type is the type of the array
    * @return the type of the element
    */
   unsigned int GetElements(const unsigned int type) const;

   /**
    * Returns the types of the parameters
    * @return the types of the parameters
    */
   IRNodeConstSet GetParameterTypes() const;

   /**
    * Return the list of index of original parameters of the function
    */
   const std::list<unsigned int> get_parameters() const;

   /**
    * Return the list of index of original parameters of the function
    */
   std::vector<ir_nodeRef> GetParameters() const;

   /**
    * Return true if function has implementation
    */
   bool has_implementation() const;

   /**
    * Returns true if this function is of var args type
    * @return true if this function is of var args type
    */
   bool is_var_args() const;

   /**
    * return true in case stm is a va_start call_node
    * @param stm is the statement
    */
   bool is_va_start_call(unsigned int stm) const;

   /**
    * return the initialization object associated with the variable.
    * @param var is the index of variable
    * @param list_of_variables is used to return list of variables used during initialization
    * @return the index of the initialization object associated with the variable
    * reference storing at the end true only if the variable has an initialization objetct.
    * In case the variable does not have an initialization object this function returns 0.
    */
   unsigned int GetInit(unsigned int var, CustomUnorderedSet<unsigned int>& list_of_variables) const;

   /**
    * return the nodeid of the named pointer (pointer_ty_node with a name different
    * @param index is the index of the variable or the index of the type
    */
   unsigned int is_named_pointer(const unsigned int index) const;

   /**
    * Return if a variable with this name already exists
    * @param var is the name of the variable
    * @return 0 if is name hasn't been used, index of the existing variable with that name otherwise
    */
   unsigned int is_name_used(std::string var) const;

   /**
    * Add a read of a global variable
    * @param variable is the global variable read
    */
   void add_read_global_variable(unsigned int variable);

   /**
    * Add a writing of a global variables
    * @param variable is the global variable written
    */
   void add_written_global_variable(unsigned int variable);

   /**
    * Get the global variables read by the correspondent function
    */
   CustomOrderedSet<unsigned int> get_read_global_variables() const;

   /**
    * Get the global variables written by the correspondent function
    */
   CustomOrderedSet<unsigned int> get_written_global_variables() const;

   /**
    * Add the initialization of a variables
    * @param var is the variable
    * @param init is the index of the initialization
    */
   void add_initialization(unsigned int var, unsigned int init);

   std::string print_phinode_res(unsigned int phi_node_id, const std::unique_ptr<var_pp_functor>& vppf) const;

   /**
    * Print the declaration of a non built-in type.
    * @param type is an object type.
    */
   std::string print_forward_declaration(unsigned int type) const;

   /**
    * Print the operations corresponding to the node
    * @param node is the node to print
    * @param vppf is the functor used to dump the variable var.
    * The stream on which string is printed is the one associate with the identer
    * @return the string corrisponding to the node
    */
   std::string PrintNode(const ir_nodeConstRef& node, const std::unique_ptr<var_pp_functor>& vppf) const;

   std::string PrintNode(unsigned int node_id, const std::unique_ptr<var_pp_functor>& vppf) const;

   /**
    * This function prints the declaration of a variable without the closing ";".
    * For example the function prints on the stream os the following piece of C code:
    * \verbatim
    * MYINT a
    * \endverbatim
    * when the variable var is equal to 4(a) and the vppf is an instance of std_var_pp_functor.
    * @param var is the considered variable.
    * @param vppf is the functor used to dump the variable var.
    * @param init_has_to_be_printed tells if the init has to be printed
    */
   std::string PrintVarDeclaration(unsigned int var, const std::unique_ptr<var_pp_functor>& vppf,
                                   bool init_has_to_be_printed) const;

   /**
    * rename a variable
    * @param var is the index of the variable renamed
    * @param new_name is the new variable name
    */
   static void rename_a_variable(unsigned int var, const std::string& new_name);

   /**
    * remove all the entries from the renaming table
    */
   static void clear_renaming_table();

   /**
    * return the types used in type casting by tn
    * @param tn is the statement analyzed
    * @param types is the set of types type-casted by tn
    */
   void GetTypecast(const ir_nodeConstRef& tn, IRNodeConstSet& types) const;

   /**
    * Return true if node is the default ssa_node
    */
   bool IsDefaultSsaName(const unsigned int ssa_name_index) const;

   /**
    * returns true if the function body has to be printed by the C backend
    */
   bool function_has_to_be_printed(unsigned int f_id) const;

   /**
    * Return true if an operation can be moved across a control dependence, either because it is safe to anticipate
    * without any guard or because it can be protected by a predicate.
    * @param node_index is the IR node index of the operation
    */
   bool CanBeMovedAcrossControlDependence(const unsigned int node_index) const;

   /**
    * Return true if an operation requires predication to be moved across a control dependence.
    * @param node_index is the IR node index of the operation
    */
   bool RequiresPredicationForControlMotion(const unsigned int node_index) const;

   /**
    * Return if an operation can be moved
    * @param node_index is the IR node index of the operation
    */
   bool CanBeMoved(const unsigned int node_index) const;

   /**
    * Return if an operation is a store
    * @param statement_index is the index of the IR node
    * @return true if statement is a store
    */
   bool IsStore(const unsigned int statement_index) const;

   /**
    * Return if an operation is a load
    * @param statement_index is the index of the IR node
    * @return true if statement is a load
    */
   bool IsLoad(const unsigned int statement_index) const;

   /**
    * Return if an operation is a lut_node
    * @param statement_index is the index of the IR node
    * @return true if statement is a lut_node
    */
   bool IsLut(const unsigned int statement_index) const;
};

/**
 * RefCount type definition of the IR-to-graph class structure
 */
using BehavioralHelperRef = refcount<BehavioralHelper>;
using BehavioralHelperConstRef = refcount<const BehavioralHelper>;

#endif
