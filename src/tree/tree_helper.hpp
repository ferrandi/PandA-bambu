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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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

#include "custom_set.hpp"
#include "panda_types.hpp"
#include "refcount.hpp"
#include "tree_node.hpp"

#include <cstddef>
#include <list>
#include <set>
#include <string>
#include <tuple>
#include <vector>

struct binfo;
struct function_decl;
struct integer_cst;
struct ssa_name;
struct statement_list;
enum class TreeVocabularyTokenTypes_TokenEnum;
CONSTREF_FORWARD_DECL(tree_manager);
CONSTREF_FORWARD_DECL(tree_node);
CONSTREF_FORWARD_DECL(var_pp_functor);
REF_FORWARD_DECL(Range);
REF_FORWARD_DECL(bloc);
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_node);

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
    * Return the types to be declared before/after declaring type
    * @param returned_types is the set of type declarations
    * @param type is the starting type
    * @param recursion must be set to true in recursive call
    * @param without_transformation specifies if we are not restructuring the code
    * @param before is true/false if we are computing types which must be declared before/after type
    */
   static void RecursiveGetTypesToBeDeclared(std::set<tree_nodeConstRef, TreeNodeConstSorter>& returned_types,
                                             const tree_nodeConstRef& type, const bool recursion,
                                             const bool without_transformation, const bool before);

   /**
    * recursively compute the references to the ssa_name variables used in a statement
    * @param tn is the statement
    * @param uses is where the uses will be stored
    */
   static void ComputeSsaUses(const tree_nodeRef&, TreeNodeMap<size_t>& uses);

 public:
   /// debug level (set by Parameter)
   static int debug_level;

   /**
    * Return the size of a tree object
    * @param tn is the tree object
    * @return the size of the object
    */
   static unsigned long long Size(const tree_nodeConstRef& tn);

   /**
    * Return the size of a tree object type
    * @param tn is the tree object
    * @return the size of the object
    */
   static unsigned long long TypeSize(const tree_nodeConstRef& tn);

   /**
    * Return the size of a tree object when allocated in memory
    * @param tn is the tree object
    * @return the size of the object
    */
   static unsigned long long SizeAlloc(const tree_nodeConstRef& tn);

   /**
    * Return the range of a tree object
    * @param tn is the tree object
    * @return the range of the object
    */
   static RangeRef Range(const tree_nodeConstRef& tn);

   /**
    * Return the range of a tree object type
    * @param tn is the tree object
    * @param rt range type
    * @return the range of the object
    */
   static RangeRef TypeRange(const tree_nodeConstRef& tn, int rt);
   static RangeRef TypeRange(const tree_nodeConstRef& tn);

   /**
    * Return the name of template without parameters. Ex: sc_signal<T> --> sc_signal
    * @param type is the treenode of the class template (of a record_type)
    * @return name of template without parameters
    */
   static std::string GetTemplateTypeName(const tree_nodeConstRef& type);

   /**
    * Return the name of the class.
    * @param type is the treenode of the class (of a record_type)
    * @return the name of the class
    */
   static std::string GetRecordTypeName(const tree_nodeConstRef& type);

   /**
    * Return the name of the function.
    * @param tm is the tree manager
    * @param index is the treenode_index of the class (is the index of a function_decl)
    */
   static
       /// FIXME: to be remove after substitution with GetFunctionName
       std::string
       name_function(const tree_managerConstRef& tm, const unsigned int index);

   /**
    * Return the name of the function.
    * @param TM is the tree manager instance
    * @param decl is the treenode of the class (of a function_decl)
    * @return the name of the function
    */
   static
       /// FIXME: to be remove after substitution with PrintFunctionName
       std::string
       GetFunctionName(const tree_managerConstRef& TM, const tree_nodeConstRef& decl);

   /**
    * Return where a function or a type is defined
    * @param node type or decl treenode
    * @param is_system stores if function or type has been already recognized as a system one
    * @return include name, line number, and column number tuple
    */
   static std::tuple<std::string, unsigned int, unsigned int> GetSourcePath(const tree_nodeConstRef& node,
                                                                            bool& is_system);

   /**
    * Return true if the function index returns a not void type, false otherwise
    * @param index is the treenode_index of the functions
    */
   static
       /// FIXME: to be remove after substitution with HasReturnType
       bool
       has_function_return(const tree_managerConstRef& tm, const unsigned int index);

   /**
    * Return true if the function index returns a not void type, false otherwise
    * @param type is the treenode of the functions
    * @return true if the function returns a not void type, false otherwise
    */
   static bool HasReturnType(const tree_managerConstRef& tm, const unsigned int index);

   /**
    * Return a string describing the functino type
    * @param Tree node of the function type
    */
   static std::string getFunctionTypeString(const tree_nodeRef& FT);

   /**
    * Return the list of tree nodes associated with the variable used by the node t.
    * @param first_level_only tells if we are performing inlining
    * @param t is a tree node (usually a function declaration).
    * @param list_of_variable list of used variables.
    */
   static void get_used_variables(bool first_level_only, const tree_nodeConstRef& t,
                                  CustomUnorderedSet<unsigned int>& list_of_variable);

   /**
    * Look for inheritance of a class
    * @param b is the binfo to explore
    * @param bcs is the name of the base class
    * @return true if an inheritance is found
    */
   static bool look_for_binfo_inheritance(const binfo* b, const std::string& bcs);

   /**
    * Given the tree_node of an obj_type_ref return the tree_node of the called function
    * @param tn is the tree node of the obj_type_ref.
    * @return the tree_node of the called function
    */
   static tree_nodeRef find_obj_type_ref_function(const tree_nodeConstRef& tn);

   /**
    * Return the types to be declared before declaring index type
    * @param tn is the starting type
    * @param without_transformation specifies if we are not restructuring the code
    * @return the types to be declared
    */
   static std::set<tree_nodeConstRef, TreeNodeConstSorter>
   GetTypesToBeDeclaredBefore(const tree_nodeConstRef& tn, const bool without_transformation);

   /**
    * Return the types to be declared after declaring index type
    * @param tn is the starting type
    * @param without_transformation specifies if we are not restructuring the code
    * @return the types to be declared
    */
   static std::set<tree_nodeConstRef, TreeNodeConstSorter> GetTypesToBeDeclaredAfter(const tree_nodeConstRef& tn,
                                                                                     const bool without_transformation);

   /**
    * Return the treenode index of the type of index
    * @param TM is the tree_manager
    * @param index is the treenode index
    * @param vec_size in case the treenode is a vector return its size
    * @param is_a_pointer in case the treenode is a pointer return true
    * @param is_a_function in case the treenode is a function_decl return true
    */
   static
       /// FIXME: to be remove after substitution with GetType/CGetType and others to get the out variable checks
       unsigned int
       get_type_index(const tree_managerConstRef& TM, const unsigned int index, long long int& vec_size,
                      bool& is_a_pointer, bool& is_a_function);

   /**
    * Same as previous but with two parameters.
    * @param TM is the tree_manager
    * @param index is the treenode
    */
   static
       /// FIXME: to be remove after substitution with GetType/CGetType
       unsigned int
       get_type_index(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return the return type of a function
    * @param function is the function to be considered
    * @param void_as_null if true returns nullptr when return type is void, else return tree node for void type
    * @return the tree node of the return type
    */
   static tree_nodeConstRef GetFunctionReturnType(const tree_nodeConstRef& function, bool void_as_null = true);

   /**
    * Return the tree_node index of the pointed type of a pointer object;
    * @param TM is the tree_manager
    * @param index is the index of the pointer object
    */
   static
       /// FIXME: to be remove after substitution with GetPointedType/CGetPointedType
       unsigned int
       get_pointed_type(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return the pointed type of a pointer object
    * @param pointer is the pointer object
    */
   static tree_nodeConstRef CGetPointedType(const tree_nodeConstRef& pointer);

   /**
    * Given an array or a vector return the element type
    * @param TM is the tree_manager
    * @param index is the type of the array
    * @return the type of the element
    */
   static
       /// FIXME: to be remove after substitution with CGetElements
       unsigned int
       GetElements(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Given an array or a vector return the element type
    * @param type is the type of the array
    * @return the type of the element
    */
   static tree_nodeConstRef CGetElements(const tree_nodeConstRef& type);

   /**
    * Given two nodes tells if they have same base type (const is not considered: const double == double)
    * @param tn0 first node to compare
    * @param tn1 second node to compare
    * @return true if tn0 and tn1 have the same type
    */
   static bool IsSameType(const tree_nodeConstRef& tn0, const tree_nodeConstRef& tn1);

   /**
    * Return name of the type
    * @param TM is the tree manager
    * @param index is the index of the type
    * @return the name of the type
    */
   static
       /// FIXME: to be remove after substitution with GetTypeName
       std::string
       get_type_name(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return name of the type
    * @param type is the type tree node
    * @return the name of the type
    */
   static std::string GetTypeName(const tree_nodeConstRef& type);

   /**
    * Return the tree node of parameter types
    * @param TM is the tree_manager
    * @param ind is the index of the function type
    * @param params is where parameter types are stored
    */
   static
       /// FIXME: to be remove after substitution with GetParameterTypes
       void
       get_parameter_types(const tree_managerConstRef& TM, const unsigned int index, std::list<unsigned int>& params);

   /**
    * Return the tree node of parameter types
    * @param ftype is the function type
    * @return parameters type list
    */
   static std::vector<tree_nodeConstRef> GetParameterTypes(const tree_nodeConstRef& ftype);

   /**
    * Return the fields type of a variable of type struct
    * @param type is the struct type
    */
   static std::vector<tree_nodeConstRef> CGetFieldTypes(const tree_nodeConstRef& type);

   /**
    * Return the idx element of the fields declared in an union or a record type
    * @param TM is the tree_manager
    * @param ind is the index of the record/union type
    * @param idx is the index of the field decl
    */
   static
       /// FIXME: to be remove after substitution with GetFieldIdx
       unsigned int
       get_field_idx(const tree_managerConstRef& TM, const unsigned int index, unsigned int idx);

   /**
    * Return the element of the fields declared in an union or a record type
    * @param type is the record/union type
    * @param idx is the index of the field decl
    * @return the field decl node
    */
   static tree_nodeRef GetFieldIdx(const tree_nodeConstRef& type, unsigned int idx);

   /**
    * Return the treenode of the type of node.
    * @param node is the treenode
    */
   static tree_nodeConstRef CGetType(const tree_nodeConstRef& node);

   /**
    * Return true if variable or type is a system one
    * @param TM is the tree manager
    * @param index is the index of the treenode corresponding to the decl node or to the type node
    * @return true if variable or type is a system one
    */
   static
       /// FIXME: to be remove after substitution with IsSystemType
       bool
       is_system(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if variable or type is a system one
    * @param type is the treenode corresponding to the decl node or to the type node
    * @return true if variable or type is a system one
    */
   static bool IsSystemType(const tree_nodeConstRef& type);

   /**
    * Return true if the decl node or type is in libbambu
    * @param TM is the tree manager
    * @param index is the treenode index
    */
   static
       /// FIXME: to be remove after substitution with IsInLibbambu
       bool
       IsInLibbambu(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if the decl node or type is in libbambu
    * @param type is the treenode
    * @return true if the decl node or type is in libbambu
    */
   static bool IsInLibbambu(const tree_nodeConstRef& type);

   /**
    * Return if treenode index is an enumeral type
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static
       /// FIXME: to be remove after substitution with IsEnumType
       bool
       is_an_enum(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return if treenode index is an enumeral type
    * @param type is the treenode
    * @return if treenode is an enumeral type
    */
   static bool IsEnumType(const tree_nodeConstRef& type);

   /**
    * Return if treenode index is a record
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static
       /// FIXME: to be remove after substitution with IsStructType
       bool
       is_a_struct(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if treenode is a record
    * @param type is the treenode
    * @return true if treenode is a record
    */
   static bool IsStructType(const tree_nodeConstRef& type);

   /**
    * Return if treenode index is an union
    * @param TM is the tree_manager
    * @param index is the treenode index
    * @return true if treenode index is an union
    */
   static
       /// FIXME: to be remove after substitution with IsUnionType
       bool
       is_an_union(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return if treenode is an union
    * @param type is the treenode
    * @return true if treenode is an union
    */
   static bool IsUnionType(const tree_nodeConstRef& type);

   /**
    * Return if treenode index is a complex
    * @param TM is the tree_manager
    * @param index is the treenode index
    * @return true if treenode index is a complex
    */
   static
       /// FIXME: to be remove after substitution with IsComplexType
       bool
       is_a_complex(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return if treenode is a complex
    * @param type is the treenode
    * @return true if treenode is a complex
    */
   static bool IsComplexType(const tree_nodeConstRef& type);

   /**
    * Return if treenode index is an array or it is equivalent to an array (record recursively having a single field
    * ending into a single arrays)
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static
       /// FIXME: to be remove after substitution with IsArrayEquivType
       bool
       is_an_array(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if treenode is an array or it is equivalent to an array (record recursively having a single field
    * ending into a single arrays)
    * @param type is the treenode
    * @return true if treenode is an array or it is equivalent to an array
    */
   static bool IsArrayEquivType(const tree_nodeConstRef& type);

   /**
    * Return true if treenode is an array
    * @param type is the treenode
    * @return true if treenode is an array
    * @return false if treenode is not an array
    */
   static bool IsArrayType(const tree_nodeConstRef& type);

   /**
    * @param type is the treenode
    * @return the basetype of the array in case it is an array
    */
   static tree_nodeConstRef CGetArrayBaseType(const tree_nodeConstRef& type);

   /**
    * Return if treenode index is a pointer
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static
       /// FIXME: to be remove after substitution with IsPointerType
       bool
       is_a_pointer(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if treenode index is a pointer
    * @param typw is the treenode
    * @return true if treenode is a pointer
    */
   static bool IsPointerType(const tree_nodeConstRef& type);

   /**
    * Return if treenode index is a function_decl
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static
       /// FIXME: to be remove after substitution with IsFunctionDeclaration
       bool
       is_a_function(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if treenode is a function_decl
    * @param type is the treenode
    * @return true if treenode is a function_decl
    */
   static bool IsFunctionDeclaration(const tree_nodeConstRef& type);

   /**
    * Return true if the treenode index is a vector
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static
       /// FIXME: to be remove after substitution with IsVectorType
       bool
       is_a_vector(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if the treenode is a vector
    * @param type is the treenode
    * @return true if the treenode is a vector
    */
   static bool IsVectorType(const tree_nodeConstRef& type);

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
    * @param type is the treenode
    * @return if the type has to be declared
    */
   static bool HasToBeDeclared(const tree_managerConstRef& TM, const tree_nodeConstRef& type);

   /**
    * Return if the treenode is of const type
    * @param TM is the tree_manager
    * @param index is the treenode index
    * @return if tree_node is of const type
    */
   static
       /// FIXME: to be remove after substitution with IsConstType
       bool
       is_const_type(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if the treenode is of const type
    * @param type is the treenode
    * @return true if tree_node is of const type
    */
   static bool IsConstType(const tree_nodeConstRef& type);

   /**
    * Return true if the treenode is of bool type
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static
       /// FIXME: to be remove after substitution with IsBooleanType
       bool
       is_bool(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if the treenode is of bool type
    * @param type is the treenode index
    * @return true if the treenode is of bool type
    */
   static bool IsBooleanType(const tree_nodeConstRef& type);

   /**
    * Return true if the treenode is of void type
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static
       /// FIXME: to be remove after substitution with IsVoidType
       bool
       is_a_void(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if the treenode is of void type
    * @param type is the treenode
    * @return true if the treenode is of void type
    */
   static bool IsVoidType(const tree_nodeConstRef& type);

   /**
    * Return true if the treenode is a ssa_name greater or equal to zero
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static
       /// FIXME: to be remove after substitution with IsPositiveIntegerValue
       bool
       is_natural(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if the treenode is a ssa_name greater or equal to zero
    * @param type is the treenode
    * @return true if the treenode is a ssa_name greater or equal to zero
    */
   static bool IsPositiveIntegerValue(const tree_nodeConstRef& type);

   /**
    * Return true if the treenode is of integer type
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static
       /// FIXME: to be remove after substitution with IsSignedIntegerType
       bool
       is_int(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if the treenode is of integer type
    * @param type is the treenode
    * @return true if the treenode is of integer type
    */
   static bool IsSignedIntegerType(const tree_nodeConstRef& type);

   /**
    * Return true if the treenode is of real type
    * @param TM is the tree_manager
    * @param index is the treenode index
    * @return if index is of real type
    */
   static
       /// FIXME: to be remove after substitution with IsRealType
       bool
       is_real(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if the treenode is of real type
    * @param type is the treenode
    * @return true if treenode is of real type
    */
   static bool IsRealType(const tree_nodeConstRef& type);

   /**
    * Return true if the treenode is of unsigned integer type
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static
       /// FIXME: to be remove after substitution with IsUnsignedIntegerType
       bool
       is_unsigned(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if the treenode is of unsigned integer type
    * @param type is the treenode
    * @return true if the treenode is of unsigned integer type
    */
   static bool IsUnsignedIntegerType(const tree_nodeConstRef& type);

   /**
    * Return true if the treenode is an int, an unsigned, a real or a Boolean data type
    * @param TM is the tree_manager
    * @param index is the treenode index
    */
   static
       /// FIXME: to be remove after substitution with IsScalarType
       bool
       is_scalar(const tree_managerConstRef& TM, const unsigned int var);

   /**
    * Return true if the treenode is an int, an unsigned, a real or a Boolean data type
    * @param type is the treenode
    * @return true if the treenode is an int, an unsigned, a real or a Boolean data type
    */
   static bool IsScalarType(const tree_nodeConstRef& type);

   static
       /// FIXME: to be remove after substitution with IsVariableType
       bool
       is_a_variable(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if the treenode is a valid variable
    * @param node the treenode
    * @return true if the treenode is a valid variable
    */
   static bool IsVariableType(const tree_nodeConstRef& node);

   static
       /// FIXME: to be remove after substitution with IsStaticDeclaration
       bool
       is_static(const tree_managerConstRef& TM, const unsigned int index);

   static bool IsStaticDeclaration(const tree_nodeConstRef& decl);

   static
       /// FIXME: to be remove after substitution with IsExternDeclaration
       bool
       is_extern(const tree_managerConstRef& TM, const unsigned int index);

   static bool IsExternDeclaration(const tree_nodeConstRef& decl);

   /**
    * Return true if the treenode is of type function type
    * @param TM is the tree_manager
    * @param index is the treenode_index
    */
   static
       /// FIXME: to be remove after substitution with IsFunctionType
       bool
       is_function_type(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if the treenode is of type function type
    * @param type is the treenode
    * @return true if the treenode is of type function type
    */
   static bool IsFunctionType(const tree_nodeConstRef& type);

   /**
    * Return true if the treenode is of type function pointer type
    * @param TM is the tree_manager
    * @param index is the treenode_index
    */
   static
       /// FIXME: to be remove after substitution with IsFunctionPointerType
       bool
       is_function_pointer_type(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return true if the treenode is of type function pointer type
    * @param type is the treenode
    * @return true if the treenode is of type function pointer type
    */
   static bool IsFunctionPointerType(const tree_nodeConstRef& type);

   /**
    * Retrun the base address of a memory access
    * @param TM is the tree manager
    * @param int is the index of the access
    */
   static
       /// FIXME: to be remove after substitution with GetBaseVariable
       unsigned int
       get_base_index(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Retrun the base variable of a memory access
    * @param mem is the node of the memory access
    * @return the base variable of a memory access
    */
   static tree_nodeRef GetBaseVariable(const tree_nodeRef& mem, std::vector<tree_nodeRef>* field_offset = nullptr);

   static
       /// FIXME: to be remove after substitution with IsPointerResolved
       bool
       is_fully_resolved(const tree_managerConstRef& TM, const unsigned int index,
                         CustomOrderedSet<unsigned int>& res_set);

   static bool IsPointerResolved(const tree_nodeConstRef& ptr, CustomOrderedSet<unsigned int>& res_set);

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
   static
       /// FIXME: to be remove after substitution with IsVolatile
       bool
       is_volatile(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * return true in case the tree node corresponds to a volatile variable
    * @param tn is the tree node
    * @return true in case the variable/ssa_name is a volatile object
    */
   static bool IsVolatile(const tree_nodeConstRef& tn);

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
   static
       /// FIXME: to be remove after substitution with GetConstValue
       integer_cst_t
       get_integer_cst_value(const integer_cst* ic);

   /**
    * Get value from integer constant
    * @param tn Integer constant tree node
    * @param is_signed Return signed value if true, unsigned equivalent if false
    * @return integer_cst_t Integer constant value
    */
   static integer_cst_t GetConstValue(const tree_nodeConstRef& tn, bool is_signed = true);

   /**
    * Return the tree node index of the array variable written or read
    * @param TM is the tree_manager
    * @param index is the index of the gimple_assign
    * @param is_written is true when the array is written false otherwise
    * @param two_dim_p becomes true when the array is two dimensional
    */
   static unsigned int get_array_var(const tree_managerConstRef& TM, const unsigned int index, bool is_written,
                                     bool& two_dim_p);

   /**
    * Return the size (in bits) of the base element of the array
    * @param TM is the tree_manager
    * @param index is the array object
    */
   static
       /// FIXME: to be remove after substitution with GetArrayElementSize
       unsigned long long
       get_array_data_bitsize(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return the size (in bits) of the base element of the array
    * @param node is the array object
    * @return the size (in bits) of the base element of the array
    */
   static unsigned long long GetArrayElementSize(const tree_nodeConstRef& node);

   /**
    * Return the dimension of the array
    * @param TM is the tree_manager
    * @param index is the array object
    * @param dims return for each dimension the number of elements
    */
   static
       /// FIXME: to be remove after substitution with GetArrayDimensions
       void
       get_array_dimensions(const tree_managerConstRef& TM, const unsigned int index,
                            std::vector<unsigned long long>& dims);

   /**
    * Return the dimension of the array
    * @param node is the array object
    * @return for each dimension the number of elements
    */
   static std::vector<unsigned long long> GetArrayDimensions(const tree_nodeConstRef& node);

   /**
    * Return the dimension of the array
    * @param TM is the tree_manager
    * @param index is the array object
    * @param dims return for each dimension the number of elements
    * @param elts_bitsize return the base type bitsize
    */
   static
       /// FIXME: to be remove after substitution with GetArrayDimensions and GetArrayElementSize
       void
       get_array_dim_and_bitsize(const tree_managerConstRef& TM, const unsigned int index,
                                 std::vector<unsigned long long>& dims, unsigned long long& elts_bitsize);

   /**
    * Return the total number of elements of the the base type in the array
    * @param TM is the tree_manager
    * @param index is the array object
    */
   static
       /// FIXME: to be remove after substitution with GetArrayTotalSize
       unsigned long long
       get_array_num_elements(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return the total number of elements of the the base type in the array
    * @param node is the array object
    * @return the total number of elements of the the base type in the array
    */
   static unsigned long long GetArrayTotalSize(const tree_nodeConstRef& node);

   /**
    * Return the indexes of the array_ref
    * @param TM is the tree_manager
    * @param index is the array_ref object
    * @param indexes return the index of the array_ref
    * @param size_indexes return the size of each index
    * @param base_object is the base referenced object
    */
   static void extract_array_indexes(const tree_managerConstRef& TM, const unsigned int index,
                                     std::vector<unsigned long long>& indexes,
                                     std::vector<unsigned long long>& size_indexes, unsigned int& base_object);

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
   static
       /// FIXME: to be remove after substitution with IsConstant
       bool
       is_constant(const tree_managerConstRef& TM, const unsigned int index);

   /**
    * Return if a tree node is a constant object
    * @param node is the tree_node
    * @return true is the object is constant
    */
   static bool IsConstant(const tree_nodeConstRef& node);

   /**
    * Function return the symbol related with the operator op passed as parameter
    * @param op is the tree node of the expression
    * @return the string corresponding to the operator
    */
   static std::string op_symbol(const tree_nodeConstRef& op);

   /**
    * Function return the symbol related with the operator op passed as parameter
    * @param op is the tree node of the expression
    * @return the string corresponding to the operator
    */
   static std::string op_symbol(const tree_node* op);

   /**
    * Return the unqualified version of a type
    * @param TM is the tree_manager
    * @param type is the type
    * @return the unqualified version of the type if it exists, 0 otherwise
    */
   static
       /// FIXME: to be remove after substitution with GetUnqualifiedType
       unsigned int
       GetUnqualified(const tree_managerConstRef& TM, unsigned int type);

   /**
    * Return the unqualified version of a type
    * @param type is the type
    * @return the unqualified version of the type if it exists, nullptr else
    */
   static tree_nodeConstRef GetUnqualifiedType(const tree_nodeConstRef& type);

   /**
    * Return the real type
    * @param TM is the tree manager
    * @param index is the index of the type
    * @return the real type
    */
   static
       /// FIXME: to be remove after substitution with GetRealType
       unsigned int
       GetRealType(const tree_managerConstRef& TM, unsigned int index);

   /**
    * Return the real type (same as GetUnqualifiedType, but return type instead of nullptr)
    * @param type is the type
    * @return the real type
    */
   static tree_nodeConstRef GetRealType(const tree_nodeConstRef& type);

   /**
    * Return true if type has not default alignment
    */
   static bool IsAligned(const tree_managerConstRef& TM, unsigned int type);

   static bool IsAligned(const tree_nodeConstRef& tn);

   /**
    * Return the var alignment
    * @param TM is the tree manager
    * @param index is the index of the variable
    * @return the variable alignment
    */
   static unsigned int get_var_alignment(const tree_managerConstRef& TM, unsigned int var);

   /**
    * Return normalized name of types and variables
    * @param id is the initial typename
    */
   static std::string NormalizeTypename(const std::string& id);

   /**
    * Return the mangled function name
    * @param fd is the function decl
    * @return std::string Mangled function name
    */
   static std::string GetMangledFunctionName(const function_decl* fd);

   /**
    * Return the name of the function in a string
    * @param node is the function_decl
    */
   static std::string print_function_name(const tree_managerConstRef& TM, const function_decl* fd);

   /**
    * Print a type and its variable in case var is not zero.
    * @param type is the type of var.
    * @param global tells if the variable is global
    * @param print_qualifiers tells if the qualifiers (i.e. "const") have to be printed
    * @param print_storage tells if the storage (i.e. "static") has to be printed
    * @param var is the variable.
    * @param vppf is the pointer to the functor used to dump the possible variable var
    * @param prefix is the string to be appended at the begining of the printing
    * @return std::string the printed string
    */
   static
       /// FIXME: to be remove after substitution with PrintType
       std::string
       print_type(const tree_managerConstRef& TM, unsigned int type, bool global = false, bool print_qualifiers = false,
                  bool print_storage = false, unsigned int var = 0,
                  const var_pp_functorConstRef& vppf = var_pp_functorConstRef(), const std::string& prefix = "",
                  const std::string& tail = "");

   /**
    * Print a type and its variable in case var is not zero.
    * @param type is the type of var.
    * @param global tells if the variable is global
    * @param print_qualifiers tells if the qualifiers (i.e. "const") have to be printed
    * @param print_storage tells if the storage (i.e. "static") has to be printed
    * @param var is the variable.
    * @param vppf is the pointer to the functor used to dump the possible variable var
    * @param prefix is the string to be appended at the begining of the printing
    * @return std::string the printed string
    */
   static std::string PrintType(const tree_managerConstRef& TM, const tree_nodeConstRef& type, bool global = false,
                                bool print_qualifiers = false, bool print_storage = false,
                                const tree_nodeConstRef& var = nullptr,
                                const var_pp_functorConstRef& vppf = var_pp_functorConstRef(),
                                const std::string& prefix = "", const std::string& tail = "");

   /**
    * return the type of the ith formal parameter in case index_obj is a call_expr
    */
   static
       /// FIXME: to be remove after substitution with GetFormalIth
       unsigned int
       get_formal_ith(const tree_managerConstRef& TM, unsigned int index_obj, unsigned int parm_index);

   /**
    * Return the type of the ith formal parameter in case index_obj is a call_expr
    *
    * @param obj is the call_expr node
    * @param parm_index is the index of the parameter
    * @return tree_nodeConstRef the type of the ith formal parameter in case index_obj is a call_expr (could be nullptr)
    */
   static tree_nodeConstRef GetFormalIth(const tree_nodeConstRef& obj, unsigned int parm_index);

   static
       /// FIXME: to be remove after substitution with IsPackedType
       bool
       is_packed(const tree_managerConstRef& TreeM, unsigned int node_index);

   static bool IsPackedType(const tree_nodeConstRef& type);

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
   static unsigned long long AccessedMaximumBitsize(const tree_nodeConstRef& type_node, unsigned long long bitsize);

   /**
    * return the minimum bitsize associated with the elements accessible through type_node
    */
   static unsigned long long AccessedMinimunBitsize(const tree_nodeConstRef& type_node, unsigned long long bitsize);

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
   static void compute_ssa_uses_rec_ptr(const tree_nodeConstRef& tn, CustomOrderedSet<const ssa_name*>& ssa_uses);

   /**
    * recursively compute the references to the ssa_name variables used in a statement
    * @param tn is the statement
    * @return the used ssa
    */
   static TreeNodeMap<size_t> ComputeSsaUses(const tree_nodeRef& tn);

   static bool is_a_nop_function_decl(const function_decl* fd);

   static void get_required_values(std::vector<std::tuple<unsigned int, unsigned int>>& required,
                                   const tree_nodeConstRef& tn);

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
    * @param tn is the tree node
    * @param fun_mem_data is the set of memory variables of the function
    * @return true if tn operation is a store
    */
   static bool IsStore(const tree_nodeConstRef& tn, const CustomOrderedSet<unsigned int>& fun_mem_data);

   /**
    * Return true if the tree node is a gimple_assign reading something which will be allocated in memory
    * @param tn is the tree node
    * @param fun_mem_data is the set of memory variables of the function
    * @return true if tn operation is a load
    */
   static bool IsLoad(const tree_nodeConstRef& tn, const CustomOrderedSet<unsigned int>& fun_mem_data);

   /**
    * Return true in case the right operation is a lut_expr
    * @param TM is the tree manager
    * @param tn is the tree node
    * @return if tn operation is a lut_expr
    */
   static bool IsLut(const tree_nodeConstRef& tn);

   /**
    * Check if omp simd pragmas are present in given statement list
    * @param sl statement list to analyse
    * @return true If omp simd pragmas where found
    * @return false If no omp simd pragmas are present
    */
   static bool has_omp_simd(const statement_list* sl);

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
   virtual ~FunctionExpander() = default;
};
using FunctionExpanderRef = refcount<FunctionExpander>;
#endif
