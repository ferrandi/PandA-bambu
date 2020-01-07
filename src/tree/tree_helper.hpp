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
 * @file tree_helper.hpp
 * @brief This file collects some utility functions.
 *
 *
 * @author Katia Turati <turati@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef TREE_HELPER_HPP
#define TREE_HELPER_HPP

#include "config_HAVE_BAMBU_BUILT.hpp"

#include "custom_set.hpp" // for set
#include "custom_set.hpp" // for unordered_set
#include <cstddef>        // for size_t
#include <list>           // for list
#include <string>         // for string
#include <tuple>          // for tuple
#include <vector>         // for vector

/// Utility include
#include "refcount.hpp"

/**
 * @name Forward declarations.
 */
//@{
struct binfo;
struct integer_cst;
struct function_decl;
struct ssa_name;
template <typename value>
class TreeNodeMap;
enum class TreeVocabularyTokenTypes_TokenEnum;
CONSTREF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_manager);
CONSTREF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(bloc);
CONSTREF_FORWARD_DECL(var_pp_functor);

//@}

/**
 * This class collects some utility functions used to extract information from tree-based data structures.
 */
class tree_helper
{
 private:
   /// store the set of SystemC class for which the type correspond to the template parameter
   static const std::set<std::string> SC_tmpl_class;

   /// store the set of SystemC class for which the template parameter correspond to the bit size of the type
   static const std::set<std::string> SC_builtin_scalar_type;

   /// store the set of TLM SystemC class for which the size corresponds to the sum of the size of template parameters
   static const std::set<std::string> TLM_SC_builtin_scalar_type;

   /**
    * Return the type to be declared before declaring index type
    * @param TM is the tree_manager
    * @param index is the starting type
    * @param recursive must be set to true in recursive call
    * @param without_transformation specifies if we are not restructuring the code
    * @param before is true if we are computing types which must be declared before index
    * @return the types to be declared
    */
   static const CustomUnorderedSet<unsigned int> RecursiveGetTypesToBeDeclared(const tree_managerConstRef TM, const unsigned int index, const bool recursive, const bool without_transformation, const bool before);

   /**
    * recursively compute the references to the ssa_name variables used in a statement
    * @param tn is the statement
    * @param uses is where the uses will be stored
    */
   static void ComputeSsaUses(const tree_nodeRef, TreeNodeMap<size_t>& uses);

 public:
   /// debug level (set by Parameter)
   static int debug_level;

   /**
    * Return the size of a tree object
    * @param tm is the tree manager
    * @param index is the treenode
    */
   static unsigned int size(const tree_managerConstRef tm, const unsigned int index);

   /**
    * Return the size of a tree object
    * @param tn is the tree object
    * @return the size of the object
    */
   static unsigned int Size(const tree_nodeConstRef tn);

   /**
    * Return the type name of a signal or a port
    * @param tm is the tree manager
    * @param index is the treenode_index
    */
   static std::string name_type(const tree_managerConstRef tm, const unsigned int index);

   /**
    * Return the name of template without parameters. Ex: sc_signal<T> --> sc_signal
    * @param tm is the tree manager
    * @param index is the treenode_index of the class template (is the index of a record_type)
    */
   static std::string name_tmpl(const tree_managerConstRef tm, const unsigned int index);

   /**
    * Return the name of the class.
    * @param tm is the tree manager
    * @param index is the treenode_index of the class (is the index of a record_type)
    */
   static std::string record_name(const tree_managerConstRef tm, unsigned int index);

   /**
    * Return the name of the function.
    * @param tm is the tree manager
    * @param index is the treenode_index of the class (is the index of a function_decl)
    */
   static std::string name_function(const tree_managerConstRef tm, const unsigned int index);

   /**
    * Return where a function or a type is defined
    * @param tm is the tree manager
    * @param index is the index
    * @param is_system stores if function or type has been already recognized as a system one
    */
   static std::tuple<std::string, unsigned int, unsigned int> get_definition(const tree_managerConstRef tm, const unsigned int index, bool& is_system);

   /**
    * Return true if the function index returns a not void type, false otherwise
    * @param index is the treenode_index of the functions
    */
   static bool has_function_return(const tree_managerConstRef tm, const unsigned int index);

   /**
    * Return a string describing the functino type
    * @param Tree node of the function type
    */
   static std::string getFunctionTypeString(tree_nodeRef FT);

   /**
    * Return the list of tree nodes associated with the variable used by the node t.
    * @param first_level_only tells if we are performing inlining
    * @param t is a tree node (usually a function declaration).
    * @param list_of_variable list of used variables.
    */
   static void get_used_variables(bool first_level_only, const tree_nodeRef t, CustomUnorderedSet<unsigned int>& list_of_variable);

   /**
    * Look for inheritance of a class
    * @param b is the binfo to explore
    * @param bcs is the name of the base class
    * @return true if an inheritance is found
    */
   static bool look_for_binfo_inheritance(binfo* b, const std::string& bcs);

   /**
    * Given the tree_node of an obj_type_ref return the tree_node of the called function
    * @param tn is the tree node of the obj_type_ref.
    * @return the tree_node of the called function
    */
   static tree_nodeRef find_obj_type_ref_function(const tree_nodeRef tn);

   /**
    * Return the types to be declared before declaring index type
    * @param TM is the tree_manager
    * @param index is the starting type
    * @param without_transformation specifies if we are not restructuring the code
    * @return the types to be declared
    */
   static const CustomUnorderedSet<unsigned int> GetTypesToBeDeclaredBefore(const tree_managerConstRef TM, const unsigned int index, const bool without_transformation);

   /**
    * Return the types to be declared after declaring index type
    * @param TM is the tree_manager
    * @param index is the starting type
    * @param without_transformation specifies if we are not restructuring the code
    * @return the types to be declared
    */
   static const CustomUnorderedSet<unsigned int> GetTypesToBeDeclaredAfter(const tree_managerConstRef TM, const unsigned int index, const bool without_transformation);

   /**
    * Return the treenode index of the type of index
    * @param TM is the tree_manager
    * @param index is the treenode index
    * @param vec_size in case the treenode is a vector return its size
    * @param is_a_pointer in case the treenode is a pointer return true
    * @param is_a_function in case the treenode is a function_decl return true
    */
   static unsigned int get_type_index(const tree_managerConstRef TM, const unsigned int index, long long int& vec_size, bool& is_a_pointer, bool& is_a_function);

   /**
    * Same as previous but with two parameters.
    * @param TM is the tree_manager
    * @param index is the treenode
    */
   static unsigned int get_type_index(const tree_managerConstRef TM, const unsigned int index);

   /**
    * Return the return type of a function
    * @param function is the function to be considered
    * @return the tree node of the return type
    */
   static tree_nodeRef GetFunctionReturnType(const tree_nodeRef function);

   /**
    * Return the tree_node index of the pointed type of a pointer object;
    * @param TM is the tree_manager
    * @param index is the index of the pointer object
    */
   static
       /// FIXME: to be remove after substitution with GetPointedType/CGetPointedType
       unsigned int
       get_pointed_type(const tree_managerConstRef TM, const unsigned int index);

   /**
    * Return the pointed type of a pointer object
    * @param pointer is the pointer object
    */
   static const tree_nodeConstRef CGetPointedType(const tree_nodeConstRef pointer);

   /**
    * Given an array or a vector return the element type
    * @param TM is the tree_manager
    * @param index is the type of the array
    * @return the type of the element
    */
   static unsigned int GetElements(const tree_managerConstRef TM, const unsigned int index);

   /**
    * Given an array or a vector return the element type
    * @param type is the type of the array
    * @return the type of the element
    */
   static const tree_nodeConstRef CGetElements(const tree_nodeConstRef type);

   /**
    * Return name of the type
    * @param TM is the tree manager
    * @param index is the index of the type
    * @return the name of the type
    */
   static std::string get_type_name(const tree_managerConstRef TM, const unsigned int index);

   /**
    * Return the tree node of parameter types
    * @param TM is the tree_manager
    * @param ind is the index of the function type
    * @param params is where parameter types are stored
    */
   static void get_parameter_types(const tree_managerConstRef TM, const unsigned int index, std::list<unsigned int>& params);

   /**
    * Return the fields type of a variable of type struct
    * @param type is the struct type
    */
   static const std::vector<tree_nodeConstRef> CGetFieldTypes(const tree_nodeConstRef type);

   /**
    * Return the idx element of the fields declared in an union or a record type
    * @param TM is the tree_manager
    * @param ind is the index of the record/union type
    * @param idx is the index of the field decl
    */
   static unsigned int get_field_idx(const tree_managerConstRef TM, const unsigned int index, unsigned int idx);

   /**
    * Return the treenode of the type of node.
    * @param node is the treenode
    */
   static unsigned int local_return_index;
   /// FIXME to be removed after complete substitution with GetType
   static tree_nodeRef get_type_node(const tree_nodeRef& node, unsigned int& return_index = local_return_index);
   static const tree_nodeConstRef CGetType(const tree_nodeConstRef node);

   /**
    * Return true if variable or type is a system one
    * @param TM is the tree manager
    * @param index is the index of the treenode corresponding to the decl node or to the type node
    * @return true if variable or type is a system one
    */
   static bool is_system(const tree_managerConstRef TM, const unsigned int index);

#if HAVE_BAMBU_BUILT
   /**
    * Return true if the decl node or type is in libbambu
    * @param TM is the tree manager
    * @param index is the treenode index
    */
   static bool IsInLibbambu(const tree_managerConstRef TM, const unsigned int index);
#endif

   /**
    * Return if treenode index is an enumeral type
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static bool is_an_enum(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return if treenode index is a record
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static bool is_a_struct(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return if treenode index is an union
    * @param TM is the tree_manager
    * @param index is the treenode index
    * @return true if treenode index is an union
    */
   static bool is_an_union(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return if treenode index is a complex
    * @param TM is the tree_manager
    * @param index is the treenode index
    * @return true if treenode index is a complex
    */
   static bool is_a_complex(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return if treenode index is an array or it is equivalent to an array (record recursively having a single field ending into a single arrays)
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static bool is_an_array(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return if treenode index is a pointer
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static bool is_a_pointer(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return if treenode index is a function_decl
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static bool is_a_function(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if the treenode index is a vector
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static bool is_a_vector(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if the treenode index is a a misaligned access to a vector data object
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static bool is_a_misaligned_vector(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return if treenode index is an address expression
    * @param TM is the tree manager
    * @param index is the treenode index
    * @return true if treenode index is an address expression
    */
   static bool is_an_addr_expr(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if the type has to be declared
    * @param TM is the tree_manager
    * @param index is the treenode index
    * @return if the type has to be declared
    */
   static bool HasToBeDeclared(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return if the treenode is of const type
    * @param TM is the tree_manager
    * @param index is the treenode index
    * @return if tree_node is of const type
    */
   static bool is_const_type(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if the treenode is of bool type
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static bool is_bool(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if the treenode is of void type
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static bool is_a_void(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if the treenode is a ssa_name greater or equal to zero
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static bool is_natural(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if the treenode is of integer type
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static bool is_int(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if the treenode is of real type
    * @param TM is the tree_manager
    * @param index is the treenode index
    * @return if index is of real type
    */
   static bool is_real(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if the treenode is of unsigned integer type
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static bool is_unsigned(const tree_managerConstRef& TM, const unsigned int index);

   static bool is_a_variable(const tree_managerConstRef& TM, const unsigned int index);

   static bool is_static(const tree_managerConstRef& TM, const unsigned int index);

   static bool is_extern(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if the treenode is of type function type
    * @param TM is the tree_manager
    * @param index is the treenode_index
    */
   static bool is_function_type(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if the treenode is of type function pointer type
    * @param TM is the tree_manager
    * @param index is the treenode_index
    */
   static bool is_function_pointer_type(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Retrun the base address of a memory access
    * @param TM is the tree manager
    * @param int is the index of the access
    */
   static unsigned int get_base_index(const tree_managerConstRef& TM, const unsigned int index);

   static bool is_fully_resolved(const tree_managerConstRef& TM, const unsigned int index, CustomOrderedSet<unsigned int>& res_set);

   /**
    * return the prefix given a qualifier
    * @param quals store the token encoding the qualifiers
    */
   static std::string return_qualifier_prefix(const TreeVocabularyTokenTypes_TokenEnum quals);

   /**
    * return the qualifiers in C format
    * @param quals store the token encoding the qualifiers
    * @param real_const is true when we need a real constant (e.g., const is not commented)
    */
   static std::string return_C_qualifiers(const TreeVocabularyTokenTypes_TokenEnum quals, bool real_const);

   /**
    * @name SystemC related functions
    */
   //@{
   /**
    * This function test if a given index is a SystemC module.
    * @param TM is the tree_manager
    * @param index is the treenode
    * @return true if index is a sc_module, false otherwise
    */
   static bool is_module(const tree_managerConstRef& TM, unsigned int index);

   /**
    * This function test if a given index is a SystemC channel.
    * @param TM is the tree_manager
    * @param index is the treenode
    * @return true if index is a sc_channel, false otherwise
    */
   static bool is_channel(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * This function test if a given index is a SystemC builtin channel.
    * @param TM is the tree_manager
    * @param index is the treenode
    * @return true if index is a SystemC builtin channel, false otherwise
    */
   static bool is_builtin_channel(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * This function test if a given index is a SystemC sc_signal.
    * @param TM is the tree_manager
    * @param index is the treenode
    * @return true if index is a sc_signal, false otherwise
    */
   static bool is_signal(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * This function test if a given index is a SystemC port.
    * @param TM is the tree_manager
    * @param index is the treenode
    * @return true if index is a port, false otherwise
    */
   static bool is_port(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * This function test if a given index is a SystemC input port.
    * @param TM is the tree_manager
    * @param index is the treenode
    * @return true if index is an input port, false otherwise
    */
   static bool is_in_port(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * This function test if a given index is a SystemC output port.
    * @param TM is the tree_manager
    * @param index is the treenode
    * @return true if index is an output port, false otherwise
    */
   static bool is_out_port(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * This function test if a given index is a SystemC in-output port.
    * @param TM is the tree_manager
    * @param index is the treenode
    * @return true if index is an inoutput port, false otherwise
    */
   static bool is_inout_port(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * This function test if a given index is a SystemC event.
    * @param TM is the tree_manager
    * @param index is the treenode
    * @return true if index is an event, false otherwise
    */
   static bool is_event(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * This function test if a given index as parm is a SystemC sc_clock.
    * @param TM is the tree_manager
    * @param index is the treenode
    * @return true if index is a sc_clock, false otherwise
    */
   static bool is_clock(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * This function test if a given index as parm is a SC_BIND_PROXY_NIL
    * @param TM is the tree_manager
    * @param index is the treenode
    * @return true if index is a SC_BIND_PROXY_NIL, false otherwise
    */
   static bool is_SC_BIND_PROXY_NIL(const tree_managerConstRef& TM, const unsigned int index);

   //@}

   /**
    * Return true in case index is a ssa_name
    * @param TM is the tree manager
    * @param index is the index of a possible ssa_name
    * @return true in case of ssa_name
    */
   static bool is_ssa_name(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * return true in case the index corresponds to a volatile variable
    * @param TM is the tree manager
    * @param index is the id of a variable/ssa_name
    * @return true in case the variable/ssa_name is a volatile object
    */
   static bool is_volatile(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * return true in case the index corresponds to a parameter in ssa form or not
    * @param TM is the tree manager
    * @param index is the id of a potential parameter
    * @return true in case the index is a parameter
    */
   static bool is_parameter(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * return true in case the ssa_name is virtual
    * @param TM is the tree manager
    * @param index is the id of a potential ssa_name
    * @return true in case the index is a virtual ssa_name
    */
   static bool is_virtual(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Convert a integer_cst in a long long value
    * @param ic is the integer costant data.
    */
   static long long get_integer_cst_value(const integer_cst* ic);

   /**
    * Return the tree node index of the array variable written or read
    * @param TM is the tree_manager
    * @param index is the index of the gimple_assign
    * @param is_written is true when the array is written false otherwise
    * @param two_dim_p becomes true when the array is two dimensional
    */
   static unsigned int get_array_var(const tree_managerConstRef& TM, const unsigned int index, bool is_written, bool& two_dim_p);

   /**
    * Return the size (in bits) of the base element of the array
    * @param TM is the tree_manager
    * @param index is the array object
    */
   static unsigned int get_array_data_bitsize(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return the dimension of the array
    * @param TM is the tree_manager
    * @param index is the array object
    * @param dims return for each dimension the number of elements
    */
   static void get_array_dimensions(const tree_managerConstRef& TM, const unsigned int index, std::vector<unsigned int>& dims);

   /**
    * Return the dimension of the array
    * @param TM is the tree_manager
    * @param index is the array object
    * @param dims return for each dimension the number of elements
    * @param elts_bitsize return the base type bitsize
    */
   static void get_array_dim_and_bitsize(const tree_managerConstRef& TM, const unsigned int index, std::vector<unsigned int>& dims, unsigned int& elts_bitsize);

   /**
    * Return the total number of elements of the the base type in the array
    * @param TM is the tree_manager
    * @param index is the array object
    */
   static unsigned int get_array_num_elements(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return the indexes of the array_ref
    * @param TM is the tree_manager
    * @param index is the array_ref object
    * @param indexes return the index of the array_ref
    * @param size_indexes return the size of each index
    * @param base_object is the base referenced object
    */
   static void extract_array_indexes(const tree_managerConstRef& TM, const unsigned int index, std::vector<unsigned int>& indexes, std::vector<unsigned int>& size_indexes, unsigned int& base_object);

   /**
    * check if a given tree node is a concatenation operation
    * @param TM is the tree_manager
    * @param index is the index of the tree_node
    * @return true when index is a bit_ior_expr and the bit values are mutually exclusive.
    */
   static bool is_concat_bit_ior_expr(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * check if a given tree node is a simple pointer plus expression
    * @param TM is the tree_manager
    * @param index is the index of the tree_node
    * @return true when index is a simple pointer plus expression between two constants
    */
   static bool is_simple_pointer_plus_test(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return if a tree node is a constant object
    * @param TM is the tree_manager
    * @param index is the index of the tree_node
    * @return true is the object is constant
    */
   static bool is_constant(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Function return the symbol related with the operator op passed as parameter
    * @param op is the tree node of the expression
    * @return the string correspoinding to the operator
    */
   static std::string op_symbol(const tree_nodeRef& op);

   /**
    * Function return the symbol related with the operator op passed as parameter
    * @param op is the tree node of the expression
    * @return the string correspoinding to the operator
    */
   static std::string op_symbol(const tree_node* op);

   /**
    * Return the unqualified version of a type
    * @param TM is the tree_manager
    * @param type is the type
    * @return the unqualified version of the type if it exists, 0 otherwise
    */
   static unsigned int GetUnqualified(const tree_managerConstRef& TM, unsigned int type);

   /**
    * Return the real type
    * @param TM is the tree manager
    * @param index is the index of the type
    * @return the real type
    */
   static unsigned int GetRealType(const tree_managerConstRef& TM, unsigned int index);

   /**
    * Return true if type has not default alignment
    */
   static bool IsAligned(const tree_managerConstRef& TM, unsigned int type);

   /**
    * Return the var alignment
    * @param TM is the tree manager
    * @param index is the index of the variable
    * @return the variable alignment
    */
   static unsigned int get_var_alignment(const tree_managerConstRef& TM, unsigned int var);

   /**
    * Return the normalized the name of types and variables
    * @param id is the initial ID
    */
   static std::string normalized_ID(const std::string& id);

   /**
    * Return the mangled function name
    * @param fd is the function decl
    * @param fname is the returned function name
    */
   static void get_mangled_fname(const function_decl* fd, std::string& fname);

   /**
    * Return the name of the function in a string
    * @param node is the function_decl
    */
   static std::string print_function_name(const tree_managerConstRef TM, const function_decl* fd);

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
   static std::string print_type(const tree_managerConstRef& TM, unsigned int type, bool global = false, bool print_qualifiers = false, bool print_storage = false, unsigned int var = 0, const var_pp_functorConstRef& vppf = var_pp_functorConstRef(),
                                 const std::string& prefix = "", const std::string& tail = "");

   /**
    * return the type of the ith formal parameter in case index_obj is a call_expr
    */
   static unsigned int get_formal_ith(const tree_managerConstRef& TM, unsigned int index_obj, unsigned int parm_index);

   static bool is_packed(const tree_managerConstRef& TreeM, unsigned int node_index);

   /**
    * Check if the access is on a packed data structure or not
    * @param TreeM is the tree manager
    * @param node_index is the node id under check
    * @return true in case the access is to a packed data structure or not
    */
   static bool is_packed_access(const tree_managerConstRef& TreeM, unsigned int node_index);

   /**
    * return the maximum bitsize associated with the elements accessible through type_node
    */
   static void accessed_greatest_bitsize(const tree_managerConstRef& TreeM, const tree_nodeRef& type_node, unsigned int type_index, unsigned int& bitsize);

   /**
    * return the minimum bitsize associated with the elements accessible through type_node
    */
   static void accessed_minimum_bitsize(const tree_managerConstRef& TreeM, const tree_nodeRef& type_node, unsigned int type_index, unsigned int& bitsize);

   /**
    * Compute the memory (in bytes) to be allocated to store a parameter or a variable
    * @param parameter is the actual parameter
    */
   static size_t AllocatedMemorySize(const tree_nodeConstRef& parameter);

   /**
    * Computes how many pointers are included in a tree node
    * @param tn is the tree ndoe to be considered
    * @return the number of pointers included in the tree node
    */
   static size_t CountPointers(const tree_nodeConstRef&);

   /**
    * @brief return the position of con in the gimple_multi_way_if conditions
    * @param TM is the tree manager
    * @param node_id is the node id of the gimple_multi_way_if node
    * @param cond is the condition index
    * @return the position
    */
   static unsigned int get_multi_way_if_pos(const tree_managerConstRef& TM, unsigned int node_id, unsigned int cond);

   /**
    * recursively compute the pointers to the ssa_name variables used in a statement
    * @param tn is the statement
    * @param ssa_uses is the collection of ssa_name tn uses
    */
   static void compute_ssa_uses_rec_ptr(const tree_nodeRef& tn, CustomOrderedSet<ssa_name*>& ssa_uses);

   /**
    * recursively compute the references to the ssa_name variables used in a statement
    * @param tn is the statement
    * @return the used ssa
    */
   static TreeNodeMap<size_t> ComputeSsaUses(const tree_nodeRef& tn);

   static bool is_a_nop_function_decl(function_decl* fd);

   static void get_required_values(const tree_managerConstRef& TM, std::vector<std::tuple<unsigned int, unsigned int>>& required, const tree_nodeRef& tn, unsigned int index);

   /**
    * Return true if statement must be the last of a basic block
    * @param statement is the statement to be analyzed
    * @return true if statement must be the last of a basic block
    */
   static bool LastStatement(const tree_nodeConstRef& statement);

   /**
    * Return the number of statement + the number of phi in the function
    * @param TM is the tree manager
    * @param function_index is the index of the function to be analyzed
    * @return the size of a function
    */
   static size_t GetFunctionSize(const tree_managerConstRef& TM, const unsigned int function_index);

   /**
    * return the string associated with the gimple_asm
    * @param TM is the tree manager
    * @param node_index is the gimple_asm node id
    * @return the string associated with gimple_asm
    */
   static std::string get_asm_string(const tree_managerConstRef& TM, const unsigned int node_index);

   /**
    * Return true if the tree node is a gimple_assign writing something which will be allocated in memory
    * @param TM is the tree manager
    * @param tn is the tree node
    * @param fun_mem_data is the set of memory variables of the function
    * @return true if tn operation is a store
    */
   static bool IsStore(const tree_managerConstRef& TM, const tree_nodeConstRef& tn, const CustomOrderedSet<unsigned int>& fun_mem_data);

   /**
    * Return true if the tree node is a gimple_assign reading something which will be allocated in memory
    * @param TM is the tree manager
    * @param tn is the tree node
    * @param fun_mem_data is the set of memory variables of the function
    * @return true if tn operation is a load
    */
   static bool IsLoad(const tree_managerConstRef& TM, const tree_nodeConstRef& tn, const CustomOrderedSet<unsigned int>& fun_mem_data);

   /**
    * Return true in case the right operation is a lut_expr
    * @param TM is the tree manager
    * @param tn is the tree node
    * @return if tn operation is a lut_expr
    */
   static bool IsLut(const tree_managerConstRef& TM, const tree_nodeConstRef& tn);

   /// Constructor
   tree_helper();

   /// Destructor
   ~tree_helper();
};

/// used to avoid expansion of c library function or type
class FunctionExpander
{
 private:
   /// Set of functions which don't need serialization
   CustomOrderedSet<std::string> no_serialize;

   /// Set of functions which need only internal serialization
   CustomOrderedSet<std::string> internal_serialize;

   /// Set of not opaque functions
   CustomOrderedSet<std::string> transparent;

   /// Set of headers file containg standard and system types
   CustomOrderedSet<std::string> headers;

   /// Set of types which are in c system library
   CustomOrderedSet<tree_nodeRef> lib_types;

 public:
   /**
    * Specify the type of serialization that a function must have
    */
   enum serialization
   {
      none,     /**< No serialization */
      internal, /**< Serialization between call of the same function */
      total     /**< Serialization between call of all funcions */
   };

   /**
    * Return which type of serialization the given function must have
    * @param name is the name of the function
    * @return the type of serialization
    */
   serialization get_serialization(const std::string& name) const;

   /**
    * Return if function has to be considered transparent even if we haven't body
    * @param name is the name of the function
    * @return if the function is transparent
    */
   bool is_transparent(const std::string& name) const;

   /**
    * Check if variable is defined in a c system library header;
    * if yes adds its type to library type
    * @param var is the node of the variable
    */
   void check_lib_type(const tree_nodeRef& var);

   /**
    * check membership to c library function
    * @param t is the treenode of the type
    * @return true if t is a c library function type
    */
   bool virtual operator()(const tree_nodeRef& t) const;

   /// Constructor
   FunctionExpander();

   /// Destructor
   virtual ~FunctionExpander()
   {
   }
};
typedef refcount<FunctionExpander> FunctionExpanderRef;
#endif
