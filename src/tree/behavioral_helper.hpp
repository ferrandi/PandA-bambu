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
 * @file graph_helper.hpp
 * @brief Helper for reading data about internal representation after graph_manager analysis
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 */

#ifndef BEHAVIORAL_HELPER_HPP
#define BEHAVIORAL_HELPER_HPP

/// Autoheader include
#include "config_HAVE_BAMBU_BUILT.hpp"
#include "config_HAVE_EXPERIMENTAL.hpp"
#include "config_HAVE_FROM_PRAGMA_BUILT.hpp"

#include "custom_map.hpp"
#include "custom_set.hpp" // for set
#include "custom_set.hpp"
#include <list>   // for list
#include <string> // for string
#include <tuple>
#include <utility> // for pair

/// graph include
#include "graph.hpp"

/// Utility include
#include "refcount.hpp"
/**
 * @name Forward declarations.
 */
//@{
REF_FORWARD_DECL(application_manager);
REF_FORWARD_DECL(bloc);
CONSTREF_FORWARD_DECL(OpGraph);
CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_node);
CONSTREF_FORWARD_DECL(var_pp_functor);
//@}

typedef unsigned int tree_class;

#define INTERNAL "internal_"

class BehavioralHelper
{
 protected:
   /// The application manager
   const application_managerRef AppM;

   /// The tree manager
   const tree_managerConstRef TM;

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

   /// Name of the function
   std::string function_name;

   /// Flag to check if behavioral_graph_manager contains the function implementation
   bool body;

   /// Flag to check if a call to this function has to be treated as a TYPE_OPAQUE nodeID
   bool opaque;

   /// Structure which stores initializations
   std::map<unsigned int, unsigned int> initializations;

   /**
    * Return true if the variable is a field_decl and it has a bitfield.
    */
   bool has_bit_field(unsigned int variable) const;

 public:
   /**
    * Constructor
    * @param AppM is the application manager
    * @param index is the index of the function_decl
    * @param body specifies if function body is available
    * @param parameters is the set of input parameters
    */
   BehavioralHelper(const application_managerRef AppM, unsigned int index, bool body, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   virtual ~BehavioralHelper();

   /**
    * Returns where the type index is defined
    * @param index is the index typedef
    * @param bool tells if the type hype has been already recognized as system
    * @return a pair composed by the filename and the line:column of the definition
    */
   virtual std::tuple<std::string, unsigned int, unsigned int> get_definition(unsigned int index, bool& is_system) const;

   /**
    * Print the operations corrisponding to the vertex
    * @param g is the graph
    * @param v is the vertex
    * @param vppf is the functor used to dump the variable var.
    * @param dot tells if the output is a dot graph
    */
   std::string print_vertex(const OpGraphConstRef g, const vertex v, const var_pp_functorConstRef vppf, const bool dot = false) const;

   /**
    * Print the initialization part
    * @param index is the initialization expression.
    * @param vppf is the functor used to dump the variable var.
    */
   virtual std::string print_init(unsigned int index, const var_pp_functorConstRef vppf) const;

   /**
    * Print the attributes associated to a variable
    * @param index is the attribute node
    * @param vppf is the functor used to dump the variable var.
    */
   virtual std::string print_attributes(unsigned int index, const var_pp_functorConstRef vppf, bool first = true) const;

   /**
    * Print the name of the variable associated to the index
    * @param index is the considered variable.
    * @return the name of the variable
    */
   std::string PrintVariable(unsigned int index) const;

   /**
    * Invalidate cached variable name
    * @param index is the variable whose name must be invalidated
    */
   void InvaildateVariableName(const unsigned int index);

   /**
    * Print the constant associated with index.
    * @param index is the constant id.
    * @param vppf is the functor used to dump the variable var.
    */
   virtual std::string print_constant(unsigned int var, const var_pp_functorConstRef vppf = var_pp_functorConstRef()) const;

   /**
    * Print a type and its variable in case var is not zero.
    * @param type is the type of var.
    * @param global tells if the variable is global
    * @param print_qualifiers tells if the qualifiers (i.e. "const") have to be printed
    * @param print_storage tells if the storage (i.e. "static") has to be printed
    * @param var is the variable.
    * @param vppf is the pointer to the functor used to dump the possible variable var
    * @param prefix is the string to be appended at the begining of the printing
    * @return the printed string
    */
   virtual std::string print_type(unsigned int type, bool global = false, bool print_qualifiers = false, bool print_storage = false, unsigned int var = 0, const var_pp_functorConstRef vppf = var_pp_functorConstRef(), const std::string& prefix = "",
                                  const std::string& tail = "") const;

   /**
    * Print the declaration of a non built-in type.
    * @param type is an object type.
    */
   virtual std::string print_type_declaration(unsigned int type) const;

   /**
    * Return the size in bit of a C object
    * @param index is the index of a C object
    * @return the size in bit
    */
   virtual unsigned int get_size(unsigned int index) const;

   /**
    * Return the name of the function
    */
   std::string get_function_name() const;

   /**
    * Return the index of the function
    * @param function_name is the name of the function
    */
   unsigned int get_function_index() const;

   /**
    * Return the index associated with the type of the return of the function
    * @param function_is is the index of the function
    * @return the index of the type
    */
   unsigned int GetFunctionReturnType(unsigned int function) const;

   /**
    * Return true if index is a variable or a type of type bool
    */
   virtual bool is_bool(unsigned int index) const;

   /**
    * Return true if index is a variable grater than or equal to zero
    */
   virtual bool is_natural(unsigned int index) const;

   /**
    * Return true if index is a variable or a type of type int
    */
   virtual bool is_int(unsigned int index) const;

   /**
    * Return true if index is a variable or a type of type enum
    */
   virtual bool is_an_enum(unsigned int index) const;

   /**
    * Return true if index is a variable or a type of type unsigned int
    */
   virtual bool is_unsigned(unsigned int index) const;

   /**
    * Return true if index is a variable or a type of type real
    */
   virtual bool is_real(unsigned int index) const;

   /**
    * Return true if index is a variable or a type of type complex
    */
   virtual bool is_a_complex(unsigned int index) const;

   /**
    * Return true if index is a variable or a type of type struct
    */
   virtual bool is_a_struct(unsigned int index) const;

   /**
    * Return true if index is a variable of a type of type union
    * @param index is the index of the variable or of the type
    * @return true if variable or type is of type union
    */
   virtual bool is_an_union(unsigned int index) const;

   /**
    * Return true if index is a variable or a type of type array
    */
   virtual bool is_an_array(unsigned int index) const;

   /**
    * Return true if index is a variable or a type of type vector
    */
   bool is_a_vector(unsigned int variable) const;

   /**
    * Return true if index is a variable or a type of type pointer
    */
   virtual bool is_a_pointer(unsigned int index) const;

   /**
    * Return true if the index is an indirect ref.
    * @param index is a variable.
    */
   virtual bool is_an_indirect_ref(unsigned int index) const;

   /**
    * Return true if the index is an array ref
    * @param index is a variable.
    */
   virtual bool is_an_array_ref(unsigned int index) const;

   /**
    * Return true if the index is a component ref
    * @param index is the index
    */
   virtual bool is_a_component_ref(unsigned int index) const;

   /**
    * Return true if the index is an addr_expr
    * @param index is the index
    */
   virtual bool is_an_addr_expr(unsigned int index) const;

   /**
    * Return true if the index is a mem_ref
    * @param index is the index
    */
   virtual bool is_a_mem_ref(unsigned int index) const;

   /**
    * check if a given index is a static declaration
    * @param decl is the decl index id
    * @return true whem decl is a static function or variable declaration
    */
   virtual bool is_static(unsigned int decl) const;

   /**
    * check if a given index is a extern declaration
    * @param decl is the decl index id
    * @return true whem decl is an extern function or variable declaration
    */
   virtual bool is_extern(unsigned int decl) const;

   /**
    * Return true if the index is a constant object
    * @param index is a nodeID.
    * @return true if it's a constant object
    */
   virtual bool is_a_constant(unsigned int index) const;

   /**
    * Return true if index is a result_decl
    */
   virtual bool is_a_result_decl(unsigned int index) const;

   /**
    * Return true if index is a realpart_expr
    * @param index is the index of the node
    * @return true if index is a realpart_expr
    */
   virtual bool is_a_realpart_expr(unsigned int index) const;

   /**
    * Return true if index is a imagpart_expr
    * @param index is the index of the node
    * @return true if index is a imagpart_expr
    */
   virtual bool is_a_imagpart_expr(unsigned int index) const;

   /**
    * Return true if function is an operating system function
    * @param index is the index of the node
    * @return true if index is a library system function
    */
   virtual bool is_operating_system_function(const unsigned int obj) const;

   /**
    * Return the nodeID of the first statement of a basic block in case that statement is a label expression.
    * @param block is basic block reference.
    */
   virtual int unsigned start_with_a_label(const blocRef& block) const;

   /**
    * Return the nodeID of the last statement of a basic block in case that statement is a cond or a goto expression.
    * @param block is basic block reference.
    */
   virtual unsigned int end_with_a_cond_or_goto(const blocRef& block) const;

   /**
    * Return the variable of an indirect ref.
    * @param index is an indirect_ref object.
    */
   virtual unsigned int get_indirect_ref_var(unsigned int index) const;

   /**
    * Return the array variable of an array ref.
    * @param index is an indirect_ref object.
    */
   virtual unsigned int get_array_ref_array(unsigned int index) const;

   /**
    * Return the index variable of an array ref.
    * @param index is an indirect_ref object.
    */
   virtual unsigned int get_array_ref_index(unsigned int index) const;

   /**
    * Return the record variable of a component ref.
    * @param index is a component_ref object.
    */
   virtual unsigned int get_component_ref_record(unsigned int index) const;

   /**
    * Return the field index of a component ref.
    * @param index is an component_ref object.
    */
   virtual unsigned int get_component_ref_field(unsigned int index) const;

   /**
    * Return the base of a mem ref.
    * @param index is a mem_ref object.
    */
   virtual unsigned int get_mem_ref_base(unsigned int index) const;

   /**
    * Return the offset of a mem ref.
    * @param index is a mem_ref object.
    */
   virtual unsigned int get_mem_ref_offset(unsigned int index) const;

   /**
    * Return the index of the operand if index is addr_expr, a realpart_expr or a imagpart_expr
    * @param index is the index of the expression
    * @return the index of the operand
    */
   virtual unsigned int get_operand_from_unary_expr(unsigned int index) const;

   /**
    * Return the index of the variable base of a ssa var
    * @param index is the index of the ssa var
    * @return the index of the base variable
    */
   virtual unsigned int GetVarFromSsa(unsigned int index) const;

   /**
    * Return the intermediate variable of an operation. It returns 0 when there is not an intermediate variable.
    * @param index is an operation.
    */
   virtual unsigned int get_intermediate_var(unsigned int index) const;

   /**
    * Return the type of the variable
    * @param var is the index of variable
    */
   virtual unsigned int get_type(const unsigned int var) const;

   /**
    * Return the pointed type of a pointer
    * @param type is a pointer object
    */
   virtual unsigned int get_pointed_type(const unsigned int type) const;

   /**
    * Given an array or a vector return the element type
    * @param index is the type of the array
    * @return the type of the element
    */
   unsigned int GetElements(const unsigned int index) const;

   /**
    * Returns the types of the parameters
    * @return the types of the parameters
    */
   virtual const CustomUnorderedSet<unsigned int> GetParameterTypes() const;

   /**
    * Return the list of index of original parameters of the function
    */
   const std::list<unsigned int> get_parameters() const;

   /**
    * Return true if function has implementation
    */
   bool has_implementation() const;

   /**
    * Returns true if this function is of var args type
    * @return true if this function is of var args type
    */
   virtual bool is_var_args() const;

   /**
    * return true in case stm is a va_start call_expr
    * @param stm is the statement
    */
   virtual bool is_va_start_call(unsigned int stm) const;

   /**
    * return the initialization object associated with the variable.
    * @param var is the index of variable
    * @param list_of_variables is used to return list of variables used during initialization
    * @return the index of the initialization object associated with the variable
    * reference storing at the end true only if the variable has an initialization objetct.
    * In case the variable does not have an initialization object this function returns 0.
    */
   virtual unsigned int GetInit(unsigned int var, CustomUnorderedSet<unsigned int>& list_of_variables) const;

   /**
    * return the attributes associated with the variable.
    * @param var is the index of variable
    * @return the attribute associated to the variable
    */
   virtual unsigned int get_attributes(unsigned int var) const;

   /**
    * return the nodeid of the named pointer (pointer_type with a name different
    * @param index is the index of the variable or the index of the type
    */
   virtual unsigned int is_named_pointer(const unsigned int index) const;

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

   virtual std::string print_phinode_res(unsigned int phi_node_id, vertex v, const var_pp_functorConstRef vppf) const;

   /**
    * Set opaque flag to true
    */
   void set_opaque();

   /**
    * Set opaque flag to false
    */
   void set_not_opaque();

   /**
    * Return the opaque flag
    * @return the opaque flag
    */
   bool get_opaque() const;

   /**
    * return the label name associated with the label expression
    */
   virtual const std::string get_label_name(unsigned int label_expr_nid) const;

   /**
    * create a gimple_assign
    * @param function_decl_nid is the node id of the function where the gimple_assign has to be inserted
    * @param block is the basic block where the gimple_assign has to be inserted
    * @param position is the statement after which gimple_assign has to be inserted
    * @param left_part is the tree_node of the left part
    * @param right_part is tree_node of the right part
    */
   virtual void create_gimple_modify_stmt(unsigned int function_decl_nid, blocRef& block, tree_nodeRef left_part, tree_nodeRef right_part);

   /**
    * Print the declaration of a non built-in type.
    * @param type is an object type.
    */
   virtual std::string print_forward_declaration(unsigned int type) const;

   /**
    * return true in case index is a return expr with an empty operand.
    */
   virtual bool is_empty_return(unsigned int index) const;

   /**
    * return the nodeID of the original gimple_phi in case the statement comes from a splitted phi node
    * @param nodeID is the statement nodeID
    */
   virtual unsigned int is_coming_from_phi_node(unsigned int nodeID) const;

   /**
    * Print the operations corresponding to the node
    * @param index is the index of the node
    * @param v is the vertex of the operation
    * @param vppf is the functor used to dump the variable var.
    * The stream on which string is printed is the one associate with the identer
    * @return the string corrisponding to the node
    */
   virtual std::string print_node(unsigned int index, vertex v, const var_pp_functorConstRef vppf) const;

   /**
    * This method returns true if the node specified in the parameters is
    * a call expression.
    * @param node is the ID of the node to be analyzed
    * @return <b>true</b> if and only if the specified node is a call expression,
    *    <b>false</b> otherwise
    */
   virtual bool isCallExpression(unsigned int nodeID) const;

   /**
    * This method returns the index of the function whose launch code is given as a string parameter.
    * This information will be used to decide whether to insert or not the CUDA execution configuration
    * launch code in the call expression.
    * @param launch_code is the function launch code
    * @return the called-function identifier
    */
   virtual unsigned int getCallExpressionIndex(std::string launch_code) const;

   /**
    * This function prints the declaration of a variable without the closing ";".
    * For example the function prints on the stream os the following piece of C code:
    * \verbatim
    * MYINT a
    * \endverbatim
    * when the variable var is equal to 4(a) and the vppf is an instance of std_var_pp_functor.
    * @param var is the considered variable.
    * @param vppf is the functor used to dump the variable var.
    * @param print_init tells if the init has to be printed
    */
   std::string PrintVarDeclaration(unsigned int var, const var_pp_functorConstRef vppf, bool init_has_to_be_printed) const;

   /**
    * Return the unqualified version of a type
    * @param type is the type
    * @return the unqualified version of the type if it exists, 0 otherwise
    */
   unsigned int GetUnqualified(const unsigned int type) const;

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
    * return the types used in type casting by nodeid
    * @param nodeid is the statement analyzed
    * @param types is the set of types type-casted by nodeid
    */
   virtual void get_typecast(unsigned int nodeid, CustomUnorderedSet<unsigned int>& types) const;

   /**
    * Return true if node is the default ssa_name
    */
   bool IsDefaultSsaName(const unsigned int ssa_name_index) const;

#if HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
   /**
    * Return the degree of parallelism for openmp for wrapper function, 0 otherwise
    */
   size_t GetOmpForDegree() const;

   /**
    * Return true if is the function is atomic
    */
   bool IsOmpFunctionAtomic() const;

   /**
    * Return true if the function is the body of an openmp for
    */
   bool IsOmpBodyLoop() const;
#endif

#if HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
   /**
    * Return true if the function is an omp atomic instruction
    */
   bool IsOmpAtomic() const;
#endif

   /**
    * returns true if the function body has to be printed by the C backend
    */
   bool function_has_to_be_printed(unsigned int f_id) const;

   /**
    * return the string associated with the gimple_asm
    * @param node_index is the gimple_asm node id
    * @return the string associated with gimple_asm
    */
   std::string get_asm_string(const unsigned int node_index) const;

#if HAVE_BAMBU_BUILT
   /**
    * Return true if an operation can be speculated
    * @param node_index is the tree node index of the operation
    */
   bool CanBeSpeculated(const unsigned int node_index) const;

   /**
    * Return if an operation can be moved
    * @param node_index is the tree node index of the operation
    */
   bool CanBeMoved(const unsigned int node_index) const;
#endif

   /**
    * Return if an operation is a store
    * @param statement_index is the index of the tree node
    * @return true if statement is a store
    */
   bool IsStore(const unsigned int statement_index) const;

   /**
    * Return if an operation is a load
    * @param statement_index is the index of the tree node
    * @return true if statement is a load
    */
   bool IsLoad(const unsigned int statement_index) const;

   /**
    * Return if an operation is a lut_expr
    * @param statement_index is the index of the tree node
    * @return true if statement is a lut_expr
    */
   bool IsLut(const unsigned int statement_index) const;
};

/**
 * RefCount type definition of the tree_to_graph class structure
 */
typedef refcount<BehavioralHelper> BehavioralHelperRef;
typedef refcount<const BehavioralHelper> BehavioralHelperConstRef;

#endif
