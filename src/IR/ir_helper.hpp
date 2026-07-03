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
 * @file ir_helper.hpp
 * @brief This file collects some utility functions.
 *
 *
 * @author Katia Turati <turati@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef IR_HELPER_HPP
#define IR_HELPER_HPP

#include "Range.hpp"
#include "custom_set.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "panda_types.hpp"
#include "refcount.hpp"

#include <cstddef>
#include <list>
#include <string>
#include <tuple>
#include <vector>

struct function_val_node;
struct constant_int_val_node;
struct ssa_node;
struct statement_list_node;
enum class IRVocabularyTokenTypes_TokenEnum;
CONSTREF_FORWARD_DECL(ir_manager);
CONSTREF_FORWARD_DECL(ir_node);
CONSTREF_FORWARD_DECL(var_pp_functor);
REF_FORWARD_DECL(bloc);
REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_node);

/**
 * This class collects some utility functions used to extract information from IR data structures.
 */
class ir_helper
{
 public:
   /// debug level (set by Parameter)
   static int debug_level;

   /**
    * Return the size of a IR node
    * @param tn is the IR node
    * @return the size of the object
    */
   static unsigned long long Size(const ir_nodeConstRef& tn) __attribute__((pure));

   /**
    * Return the size of a IR node type
    * @param tn is the IR node
    * @return the size of the object
    */
   static unsigned long long TypeSize(const ir_nodeConstRef& tn) __attribute__((pure))
   {
      return Size(CGetType(tn));
   }

   /**
    * Return the size of a IR node when allocated in memory
    * @param tn is the IR node
    * @return the size of the object
    */
   static unsigned long long SizeAlloc(const ir_nodeConstRef& tn);

   /**
    * Return the range of a IR node
    * @param tn is the IR node
    * @return the range of the object
    */
   static Range NodeRange(const ir_nodeConstRef& tn) __attribute__((pure));

   /**
    * Return the range of a IR node type
    * @param tn is the IR node
    * @param rt range type
    * @return the range of the object
    */
   static Range TypeRange(const ir_nodeConstRef& tn, int rt = Regular) __attribute__((pure));

   /**
    * Return the name of the function.
    * @param decl is the IR node of the class (of a function_val_node)
    * @return the name of the function
    */
   static std::string GetFunctionName(const ir_nodeConstRef& decl) __attribute__((pure));

   /**
    * Return where a function or a type is defined
    * @param node type or decl IR node
    * @param is_system stores if function or type has been already recognized as a system one
    * @return include name, line number, and column number tuple
    */
   static std::tuple<std::string, unsigned int, unsigned int> GetSourcePath(const ir_nodeConstRef& node,
                                                                            bool& is_system) __attribute__((pure));

   /**
    * Return the list of IR nodes associated with the variable used by the node t.
    * @param first_level_only tells if we are performing inlining
    * @param t is an IR node (usually a function declaration).
    * @param list_of_variable list of used variables.
    */
   static void get_used_variables(bool first_level_only, const ir_nodeConstRef& t,
                                  CustomUnorderedSet<unsigned int>& list_of_variable);

   /**
    * Return the types to be declared before declaring index type
    * @param tn is the starting type
    * @param without_transformation specifies if we are not restructuring the code
    * @return the types to be declared
    */
   static std::set<ir_nodeConstRef, IRNodeConstSorter> GetTypesToBeDeclaredBefore(const ir_nodeConstRef& tn,
                                                                                  const bool without_transformation);

   /**
    * Return the types to be declared after declaring index type
    * @param tn is the starting type
    * @param without_transformation specifies if we are not restructuring the code
    * @return the types to be declared
    */
   static std::set<ir_nodeConstRef, IRNodeConstSorter> GetTypesToBeDeclaredAfter(const ir_nodeConstRef& tn,
                                                                                 const bool without_transformation);

   /**
    * Return the return type of a function
    * @param function is the function to be considered
    * @param void_as_null if true returns nullptr when return type is void, else return IR node for void type
    * @return the IR node of the return type
    */
   static ir_nodeConstRef GetFunctionReturnType(const ir_nodeConstRef& function, bool void_as_null = true)
       __attribute__((pure));

   /**
    * Return the pointed type of a pointer object
    * @param pointer is the pointer object
    */
   static ir_nodeConstRef CGetPointedType(const ir_nodeConstRef& pointer) __attribute__((pure));

   /**
    * Given an array or a vector return the element type
    * @param type is the type of the array
    * @return the type of the element
    */
   static ir_nodeConstRef CGetElements(const ir_nodeConstRef& type) __attribute__((pure));

   /**
    * Given two nodes tells if they have same base type (const is not considered: const double == double)
    * @param tn0 first node to compare
    * @param tn1 second node to compare
    * @return true if tn0 and tn1 have the same type
    */
   static bool IsSameType(const ir_nodeConstRef& tn0, const ir_nodeConstRef& tn1) __attribute__((pure));

   /**
    * Return the IR node of parameter types
    * @param ftype is the function type
    * @return parameters type list
    */
   static std::vector<ir_nodeConstRef> GetParameterTypes(const ir_nodeConstRef& ftype) __attribute__((pure));

   /**
    * Return the fields type of a variable of type struct
    * @param type is the struct type
    */
   static std::vector<ir_nodeConstRef> CGetFieldTypes(const ir_nodeConstRef& type) __attribute__((pure));

   /**
    * Return the IR node of the type of node.
    * @param node is the IR node
    */
   static ir_nodeConstRef CGetType(const ir_nodeConstRef& node) __attribute__((pure));

   /**
    * Return true if variable or type is a system one
    * @param type is the IR node corresponding to the decl node or to the type node
    * @return true if variable or type is a system one
    */
   static bool IsSystemType(const ir_nodeConstRef& type) __attribute__((pure));

   /**
    * Return true if the decl node or type is in libbambu
    * @param type is the IR node
    * @return true if the decl node or type is in libbambu
    */
   static bool IsInLibbambu(const ir_nodeConstRef& type) __attribute__((pure));

   /**
    * Return true if IR node is a record
    * @param type is the IR node
    * @return true if IR node is a record
    */
   static bool IsStructType(const ir_nodeConstRef& type) __attribute__((pure));

   /**
    * Return true if IR node is an array or it is equivalent to an array (record recursively having a single field
    * ending into a single arrays)
    * @param type is the IR node
    * @return true if IR node is an array or it is equivalent to an array
    */
   static bool IsArrayEquivType(const ir_nodeConstRef& type) __attribute__((pure));

   /**
    * Return true if IR node is an array
    * @param type is the IR node
    * @return true if IR node is an array
    * @return false if IR node is not an array
    */
   static bool IsArrayType(const ir_nodeConstRef& type) __attribute__((pure));

   /**
    * @param type is the IR node
    * @return the basetype of the array in case it is an array
    */
   static ir_nodeConstRef CGetArrayBaseType(const ir_nodeConstRef& type) __attribute__((pure));

   /**
    * Return true if IR node index is a pointer
    * @param type is the IR node
    * @return true if IR node is a pointer
    */
   static bool IsPointerType(const ir_nodeConstRef& type) __attribute__((pure));

   /**
    * Return true if IR node is a function_val_node
    * @param type is the IR node
    * @return true if IR node is a function_val_node
    */
   static bool IsFunctionDeclaration(const ir_nodeConstRef& type) __attribute__((pure));

   /**
    * Check if function implementation is available
    * @param decl_node function_val_node ir_node
    * @return true if decl_node is a function_val_node with body
    */
   static bool IsFunctionImplemented(const ir_nodeConstRef& decl_node) __attribute__((pure));

   /**
    * Return true if the IR node is a vector
    * @param type is the IR node
    * @return true if the IR node is a vector
    */
   static bool IsVectorType(const ir_nodeConstRef& type) __attribute__((pure));

   /**
    * Return true if the IR node index is a a misaligned access to a vector data object
    * @param TM is the ir_manager
    * @param index is the IR node index
    */
   static bool is_a_misaligned_vector(const ir_managerConstRef& TM, const unsigned int index);

   /**
    * Return if IR node index is an address expression
    * @param TM is the IR manager
    * @param index is the IR node index
    * @return true if IR node index is an address expression
    */
   static bool is_an_addr_node(const ir_managerConstRef& TM, const unsigned int index);

   /**
    * Return the function id owning a node statement.
    * @param TM is the IR manager
    * @param op_id is the node statement index
    * @return the function_val_node index or 0 when not available
    */
   static unsigned int GetFunctionIdFromOpId(const ir_managerConstRef& TM, const unsigned int op_id)
       __attribute__((pure));

   /**
    * Return the SSA name node id produced by a node statement.
    * @param TM is the IR manager
    * @param op_id is the node statement index
    * @return the ssa_node node id or 0 when not available
    */
   static unsigned int GetSsaNameNodeIdFromOpId(const ir_managerConstRef& TM, const unsigned int op_id)
       __attribute__((pure));

   /**
    * Return true if the type has to be declared
    * @param type is the IR node
    * @return if the type has to be declared
    */
   static bool HasToBeDeclared(const ir_nodeConstRef& type);

   /**
    * Return true if the IR node is of bool type
    * @param type is the IR node index
    * @return true if the IR node is of bool type
    */
   static bool IsBooleanType(const ir_nodeConstRef& type) __attribute__((pure));

   /**
    * Return true if the IR node is of void type
    * @param type is the IR node
    * @return true if the IR node is of void type
    */
   static bool IsVoidType(const ir_nodeConstRef& type) __attribute__((pure));

   /**
    * Return true if the IR node is a ssa_node greater or equal to zero
    * @param type is the IR node
    * @return true if the IR node is a ssa_node greater or equal to zero
    */
   static bool IsPositiveIntegerValue(const ir_nodeConstRef& type) __attribute__((pure));

   /**
    * Return true if the IR node is of integer type
    * @param type is the IR node
    * @return true if the IR node is of integer type
    */
   static bool IsSignedIntegerType(const ir_nodeConstRef& type) __attribute__((pure));

   /**
    * Return true if the IR node is of real type
    * @param type is the IR node
    * @return true if IR node is of real type
    */
   static bool IsRealType(const ir_nodeConstRef& type) __attribute__((pure));

   /**
    * Return true if the IR node is of unsigned integer type
    * @param type is the IR node
    * @return true if the IR node is of unsigned integer type
    */
   static bool IsUnsignedIntegerType(const ir_nodeConstRef& type) __attribute__((pure));

   /**
    * Return true if the IR node is an int, an unsigned, a real or a Boolean data type
    * @param type is the IR node
    * @return true if the IR node is an int, an unsigned, a real or a Boolean data type
    */
   static bool IsScalarType(const ir_nodeConstRef& type) __attribute__((pure));

   /**
    * Return true if the IR node is a valid variable
    * @param node the IR node
    * @return true if the IR node is a valid variable
    */
   static bool IsVariableType(const ir_nodeConstRef& node) __attribute__((pure));

   static bool IsStaticDeclaration(const ir_nodeConstRef& decl) __attribute__((pure));

   static bool IsExternDeclaration(const ir_nodeConstRef& decl) __attribute__((pure));

   /**
    * Return true if the IR node is of type function type
    * @param type is the IR node
    * @return true if the IR node is of type function type
    */
   static bool IsFunctionType(const ir_nodeConstRef& type) __attribute__((pure));

   /**
    * Return true if the IR node is of type function pointer type
    * @param type is the IR node
    * @return true if the IR node is of type function pointer type
    */
   static bool IsFunctionPointerType(const ir_nodeConstRef& type) __attribute__((pure));

   /**
    * Retrun the base address of a memory access
    * @param TM is the IR manager
    * @param index is the index of the access
    */
   static
       /// FIXME: to be remove after substitution with GetBaseVariable
       unsigned int
       get_base_index(const ir_managerConstRef& TM, const unsigned int index)
   {
      const auto var = GetBaseVariable(TM->GetIRNode(index));
      return var ? var->index : 0;
   }

   /**
    * Retrun the base variable of a memory access
    * @param mem is the node of the memory access
    * @param field_offset optionally collects the sequence of field accesses traversed while finding the base variable
    * @return the base variable of a memory access
    */
   static ir_nodeRef GetBaseVariable(const ir_nodeRef& mem, std::vector<ir_nodeRef>* field_offset = nullptr);

   static const PointToSolution& GetPointToSet(const ir_nodeConstRef& ptr);

   /**
    * return the qualifiers in C format
    * @param quals store the token encoding the qualifiers
    * @param real_const is true when we need a real constant (e.g., const is not commented)
    */
   static std::string return_C_qualifiers(const IRVocabularyTokenTypes_TokenEnum quals, bool real_const)
       __attribute__((pure));

   /**
    * Return true in case the IR node is a ssa_node
    * @param tn is the IR node
    * @return true in case of ssa_node
    */
   static bool IsSsaName(const ir_nodeConstRef& tn) __attribute__((pure));

   /**
    * return true in case tn corresponds to a parameter in ssa form
    * @param tn is the IR node
    * @return true in case tn is a parameter
    */
   static bool IsParameter(const ir_nodeConstRef& tn) __attribute__((pure));

   /**
    * return true in case tn is a virtual ssa_node
    * @param tn is the IR node
    * @return true in case tn is a virtual ssa_node
    */
   static bool IsVirtual(const ir_nodeConstRef& tn) __attribute__((pure));

   /**
    * Convert a constant_int_val_node in a long long value
    * @param ic is the integer costant data.
    */
   static
       /// FIXME: to be remove after substitution with GetConstValue
       integer_cst_t
       get_integer_cst_value(const constant_int_val_node* ic);

   /**
    * Get value from integer constant
    * @param tn Integer constant IR node
    * @param is_signed Return signed value if true, unsigned equivalent if false
    * @return integer_cst_t Integer constant value
    */
   static integer_cst_t GetConstValue(const ir_nodeConstRef& tn, bool is_signed = true) __attribute__((pure));

   /**
    * Return the IR node index of the array variable written or read
    * @param TM is the ir_manager
    * @param index is the index of the assign_stmt
    * @param is_written is true when the array is written false otherwise
    * @param two_dim_p becomes true when the array is two dimensional
    */
   static unsigned int get_array_var(const ir_managerConstRef& TM, const unsigned int index, bool is_written,
                                     bool& two_dim_p);

   /**
    * Return the size (in bits) of the base element of the array
    * @param node is the array object
    * @return the size (in bits) of the base element of the array
    */
   static unsigned long long GetArrayElementSize(const ir_nodeConstRef& node) __attribute__((pure));

   /**
    * Return the dimension of the array
    * @param node is the array object
    * @return for each dimension the number of elements
    */
   static std::vector<unsigned long long> GetArrayDimensions(const ir_nodeConstRef& node) __attribute__((pure));

   /**
    * Return the dimension of the array
    * @param TM is the ir_manager
    * @param index is the array object
    * @param dims return for each dimension the number of elements
    * @param elts_bitsize return the base type bitsize
    */
   static
       /// FIXME: to be remove after substitution with GetArrayDimensions and GetArrayElementSize
       void
       get_array_dim_and_bitsize(const ir_managerConstRef& TM, const unsigned int index,
                                 std::vector<unsigned long long>& dims, unsigned long long& elts_bitsize);

   /**
    * Return the total number of elements of the the base type in the array
    * @param node is the array object
    * @return the total number of elements of the the base type in the array
    */
   static unsigned long long GetArrayTotalSize(const ir_nodeConstRef& node) __attribute__((pure));

   /**
    * check if a given IR node is a concatenation operation
    * @param TM is the ir_manager
    * @param index is the index of the ir_node
    * @return true when index is a or_node and the bit values are mutually exclusive.
    */
   static bool is_concat_or_node(const ir_managerConstRef& TM, const unsigned int index);

   /**
    * Return if an IR node is a constant object
    * @param node is the ir_node
    * @return true is the object is constant
    */
   static bool IsConstant(const ir_nodeConstRef& node) __attribute__((pure));

   /**
    * Function return the symbol related with the operator op passed as parameter
    * @param op is the IR node of the expression
    * @return the string corresponding to the operator
    */
   static std::string op_symbol(const ir_nodeConstRef& op);

   /**
    * Function return the symbol related with the operator op passed as parameter
    * @param op is the IR node of the expression
    * @return the string corresponding to the operator
    */
   static std::string op_symbol(const ir_node* op);

   /**
    * Return the var alignment
    * @param TM is the IR manager
    * @param var is the index of the variable
    * @return the variable alignment
    */
   static unsigned int get_var_alignment(const ir_managerConstRef& TM, unsigned int var) __attribute__((pure));

   /**
    * Return normalized name of types and variables
    * @param id is the initial typename
    */
   static std::string NormalizeTypename(const std::string& id) __attribute__((pure));

   /**
    * Print a type and its variable in case var is not zero.
    * @param type is the type of var.
    * @param print_qualifiers tells if the qualifiers (i.e. "const") have to be printed
    * @param print_storage tells if the storage (i.e. "static") has to be printed
    * @param var is the variable.
    * @param vppf is the pointer to the functor used to dump the possible variable var
    * @param prefix is the string to be appended at the begining of the printing
    * @param tail is the string to be appended at the end of the printing
    * @return std::string the printed string
    */
   static std::string PrintType(const ir_nodeConstRef& type, bool print_qualifiers = false, bool print_storage = false,
                                const ir_nodeConstRef& var = nullptr,
                                const std::unique_ptr<var_pp_functor>& vppf = nullptr, const std::string& prefix = "",
                                const std::string& tail = "");
   /**
    * Return the type of the ith formal parameter in case index_obj is a call_node
    *
    * @param obj is the call_node node
    * @param parm_index is the index of the parameter
    * @return ir_nodeConstRef the type of the ith formal parameter in case index_obj is a call_node (could be nullptr)
    */
   static ir_nodeConstRef GetFormalIth(const ir_nodeConstRef& obj, unsigned int parm_index);

   static bool IsPackedType(const ir_nodeConstRef& type) __attribute__((pure));

   /**
    * Check if the access is on a packed data structure or not
    * @param tn is the IR node
    * @return true in case the access is to a packed data structure or not
    */
   static bool IsPackedAccess(const ir_nodeConstRef& tn) __attribute__((pure));

   /**
    * return the maximum bitsize associated with the elements accessible through type_node
    */
   static unsigned long long AccessedMaximumBitsize(const ir_nodeConstRef& type_node, unsigned long long bitsize);

   /**
    * return the minimum bitsize associated with the elements accessible through type_node
    */
   static unsigned long long AccessedMinimunBitsize(const ir_nodeConstRef& type_node, unsigned long long bitsize);

   /**
    * Compute the memory (in bytes) to be allocated to store a parameter or a variable
    * @param parameter is the actual parameter
    */
   static size_t AllocatedMemorySize(const ir_nodeConstRef& parameter);

   /**
    * Computes how many pointers are included in an IR node
    * @param tn is the IR node to be considered
    * @return the number of pointers included in the IR node
    */
   static size_t CountPointers(const ir_nodeConstRef&);

   /**
    * @brief return the position of con in the multi_way_if_stmt conditions
    * @param TM is the IR manager
    * @param node_id is the node id of the multi_way_if_stmt node
    * @param cond is the condition index
    * @return the position
    */
   static unsigned int get_multi_way_if_pos(const ir_managerConstRef& TM, unsigned int node_id, unsigned int cond);

   /**
    * recursively compute the pointers to the ssa_node variables used in a statement
    * @param tn is the statement
    * @param ssa_uses is the collection of ssa_node tn uses
    */
   static void compute_ssa_uses_rec_ptr(const ir_nodeConstRef& tn, CustomOrderedSet<const ssa_node*>& ssa_uses);

   /**
    * recursively compute the references to the ssa_node variables used in a statement
    * @param tn is the statement
    * @return the used ssa
    */
   static IRNodeMap<size_t> ComputeSsaUses(const ir_nodeRef& tn);

   static bool is_a_nop_function_decl(const function_val_node* fd);

   static void get_required_values(std::vector<std::tuple<unsigned int, unsigned int>>& required,
                                   const ir_nodeConstRef& tn);

   /**
    * Return true if statement must be the last of a basic block
    * @param statement is the statement to be analyzed
    * @return true if statement must be the last of a basic block
    */
   static bool LastStatement(const ir_nodeConstRef& statement) __attribute__((pure));

   /**
    * Return true if the IR node is a assign_stmt writing something which will be allocated in memory
    * @param tn is the IR node
    * @param fun_mem_data is the set of memory variables of the function
    * @return true if tn operation is a store
    */
   static bool IsStore(const ir_nodeConstRef& tn, const CustomOrderedSet<unsigned int>& fun_mem_data)
       __attribute__((pure));

   /**
    * Return true if the IR node is a assign_stmt reading something which will be allocated in memory
    * @param tn is the IR node
    * @param fun_mem_data is the set of memory variables of the function
    * @return true if tn operation is a load
    */
   static bool IsLoad(const ir_nodeConstRef& tn, const CustomOrderedSet<unsigned int>& fun_mem_data)
       __attribute__((pure));

   /**
    * Return true in case the right operation is a lut_node
    * @param tn is the IR node
    * @return if tn operation is a lut_node
    */
   static bool IsLut(const ir_nodeConstRef& tn) __attribute__((pure));
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
   CustomOrderedSet<ir_nodeRef> lib_types;

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
   void check_lib_type(const ir_nodeRef& var);

   /**
    * check membership to c library function
    * @param t is the IR node of the type
    * @return true if t is a c library function type
    */
   bool virtual operator()(const ir_nodeRef& t) const;

   FunctionExpander();

   virtual ~FunctionExpander() = default;
};
#endif
