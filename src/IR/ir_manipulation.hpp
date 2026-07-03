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
 * @file ir_manipulation.hpp
 * @brief Class defining some useful functions to create IR nodes and to manipulate the IR manager.
 *
 * This class defines some useful functions to create IR nodes and to manipulate the IR manager.
 *
 * @author Stefano Viazzi <stefano.viazzi@gmail.com>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef IR_MANIPULATION_HPP
#define IR_MANIPULATION_HPP

#include "custom_map.hpp"
#include "panda_types.hpp"
#include "refcount.hpp"

#include <limits>
#include <string>
#include <utility>
#include <vector>

enum kind : int;
struct function_val_node;
REF_FORWARD_DECL(CallGraphManager);
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(ir_node);
CONSTREF_FORWARD_DECL(ir_node);
REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_manipulation);
REF_FORWARD_DECL(application_manager);
enum class IRVocabularyTokenTypes_TokenEnum;
REF_FORWARD_DECL(bloc);

/**
 * This class creates a layer to add nodes and to manipulate the ir_nodes manager.
 */
class ir_manipulation
{
 private:
   /// IR Manager
   ir_managerRef IRM;

   /// Application manager data structure
   const application_managerRef AppM;

   /// The set of input parameters
   const ParameterConstRef parameters;

   /// debug level.
   const int debug_level;

   /// CONST_OBJ_IR_NODES
   /** Function used to create a constant_int_val_node node.
    * @param  type is the type of the node.
    * @param value is the value of the constant
    * @param  integer_cst_nid is the index node of the object to be created
    * @return the ir_reindex node of the constant_int_val_node created.
    */
   ir_nodeRef CreateIntegerCst(const ir_nodeConstRef& type, integer_cst_t value,
                               const unsigned int integer_cst_nid) const;

   ir_nodeRef GetParametricIntegerType(unsigned algn, unsigned prec_bit, bool unsigned_p, unsigned size_value) const;

 public:
   /**
    * This is the constructor of the ir_manipulation.
    * @param IRM is the IR manager.
    * @param parameters is the set of input parameters
    * @param _AppM is the application manager used to keep application-level metadata in sync
    */
   ir_manipulation(const ir_managerRef& IRM, const ParameterConstRef& parameters, const application_managerRef _AppM);

   /**
    * This is the destructor of the ir_manipulation.
    */

   /// EXPRESSION_IR_NODES

   /**
    * Function used to create an unary expression.
    * @param  type is the type of the expression (ir_reindex).
    * @param  op is the operand of the unary expression (ir_reindex).
    * @param  loc_info is the definition of the source position.
    * @param  operation_kind is the kind of unary expression to create.
    * @return the ir_reindex node of the operation created.
    */
   ir_nodeRef create_unary_operation(const ir_nodeConstRef& type, const ir_nodeRef& op, const std::string& loc_info,
                                     enum kind operation_kind) const;

   /**
    * Function used to create a binary expression.
    * @param  type is the type of the expression (ir_reindex).
    * @param  op0 is the first operand of the binary expression (ir_reindex).
    * @param  op1 is the second operand of the binary expression (ir_reindex).
    * @param  loc_info is the definition of the source position.
    * @param  operation_kind is the kind of binary expression to create.
    * @return the ir_reindex node of the operation created.
    */
   ir_nodeRef create_binary_operation(const ir_nodeConstRef& type, const ir_nodeRef& op0, const ir_nodeRef& op1,
                                      const std::string& loc_info, enum kind operation_kind) const;

   /**
    * Function used to create a ternary expression.
    * @param  type is the type of the expression (ir_reindex).
    * @param  op0 is the first operand of the ternary expression (ir_reindex).
    * @param  op1 is the second operand of the ternary expression (ir_reindex).
    * @param  op2 is the third operand of the ternary expression (ir_reindex).
    * @param  loc_info is the definition of the source position.
    * @param  operation_kind is the kind of ternary expression to create.
    * @return the ir_reindex node of the operation created.
    */
   ir_nodeRef create_ternary_operation(const ir_nodeConstRef& type, const ir_nodeRef& op0, const ir_nodeRef& op1,
                                       const ir_nodeRef& op2, const std::string& loc_info,
                                       enum kind operation_kind) const;

   /**
    * @brief create_lut_node: function used to create a generic lut_node operation
    * @param type is the type of the expression (ir_reindex).
    * @param op0 describes the LUT functionality as a constant integer.
    * @param op1 is the first operand of the lut_node expression (ir_reindex).
    * @param op2 is the second operand of the lut_node expression (ir_reindex).
    * @param op3 is the third operand of the lut_node expression (ir_reindex).
    * @param op4 is the fourth operand of the lut_node expression (ir_reindex).
    * @param op5 is the fifth operand of the lut_node expression (ir_reindex).
    * @param op6 is the sixth operand of the lut_node expression (ir_reindex).
    * @param op7 is the seventh operand of the lut_node expression (ir_reindex).
    * @param op8 is the eighth operand of the lut_node expression (ir_reindex).
    * @param loc_info is the definition of the source position.
    * @return the ir_reindex node of the operation created.
    */
   ir_nodeRef create_lut_node(const ir_nodeConstRef& type, const ir_nodeRef& op0, const ir_nodeRef& op1,
                              const ir_nodeRef& op2, const ir_nodeRef& op3, const ir_nodeRef& op4,
                              const ir_nodeRef& op5, const ir_nodeRef& op6, const ir_nodeRef& op7,
                              const ir_nodeRef& op8, const std::string& loc_info) const;

   ir_nodeRef create_extract_bit_node(const ir_nodeRef& op0, const ir_nodeRef& op1, const std::string& loc_info) const;

   /// IDENTIFIER_IR_NODE
   /**
    * Function used to create an identifier node if it is not already present, otherwise it returns the one that is
    * already in the IR manager.
    * @param  strg is the identifier string associated with the identifier_node.
    * @return the ir_reindex node of the identifier_node created.
    */
   ir_nodeRef create_identifier_node(const std::string& strg) const;

   /// DECL_NODES

   /**
    * Function used to create a argument_val_node.
    * @param  name is the name field containing an identifier_node used to represent a name.
    * @param  type is the type.
    * @param  parent is the parent declaration.
    * @param  init is the init field holding the value to initialize a variable to.
    * @param  loc_info is the definition of the source position.
    * @param  readonly_flag means declared 'readonly'
    * @return the ir_reindex node of the argument_val_node created.
    */
   ir_nodeRef create_parm_decl(const ir_nodeRef& name, const ir_nodeConstRef& type, const ir_nodeRef& parent,
                               const ir_nodeRef& init, const std::string& loc_info, bool readonly_flag = false) const;

   /// create or find the global translation unit
   ir_nodeRef create_translation_unit_decl() const;

   /**
    * Function used to create a variable_val_node.
    * @param  name is the name field containing an identifier_node used to represent a name.
    * @param  type is the type.
    * @param  parent is the scope of the declaration.
    * @param  bitsizealloc is the allocation bitsize of the var decl
    * @param  init is the init field holding the value to initialize a variable to.
    * @param  loc_info is the definition of the source position.
    * @param  algn is the field holding the alignment required for the datum, in bits.
    * @param  extern_flag a variable can be extern (default false).
    * @param  static_flag to manage standard static attribute (default false).
    * @param  readonly_flag to mark the declaration as read-only (default false).
    * @param  bit_values stores known bit-level information for the declaration (default empty).
    * @param  addr_not_taken states whether the address of the object is known not to be taken (default false).
    * @return the ir_reindex node of the variable_val_node created.
    */
   ir_nodeRef create_var_decl(const ir_nodeRef& name, const ir_nodeConstRef& type, const ir_nodeRef& parent,
                              unsigned int bitsizealloc, const ir_nodeRef& init, const std::string& loc_info,
                              unsigned int algn, bool extern_flag = false, bool static_flag = false,
                              bool readonly_flag = false, const std::string& bit_values = "",
                              bool addr_not_taken = false) const;

   /// TYPE_OBJ

   /**
    * Function that creates a void type if it is not already present, otherwise it returns the one that is already in
    * the IR manager.
    * @return the ir_reindex node of the void type.
    */
   ir_nodeRef GetVoidType() const;

   /**
    * Function that creates a boolean type if it is not already present, otherwise it returns the one that is already in
    * the IR manager.
    * @return the ir_reindex node of the boolean type.
    */
   ir_nodeRef GetBooleanType() const;

   /**
    * Function that creates a integer type if it is not already present, otherwise it returns the one that is already in
    * the IR manager.
    * @return the ir_reindex node of the integer type.
    */
   ir_nodeRef GetSignedIntegerType() const;

   /**
    * Function that creates a unsigned integer type if it is not already present, otherwise it returns the one that is
    * already in the IR manager.
    * @return the ir_reindex node of the unsigned integer type.
    */
   ir_nodeRef GetUnsignedIntegerType() const;

   /**
    * Function that creates a long long unsigned int type if it is not already present, otherwise return the existing
    * type
    */
   ir_nodeRef GetUnsignedLongLongType() const;

   /**
    * Function that creates a bit_size type if it is not already present, otherwise it returns the one that is already
    * in the IR manager.
    * @return the ir_reindex node of the bit_size type.
    */
   ir_nodeRef GetBitsizeType() const;

   /**
    * create a sizetype builtin type in case it has not already been created, otherwise it returns the one found in the
    * IR manager.
    */
   ir_nodeRef GetSizeType() const;

   /**
    * Function that creates a pointer type if it is not already present, otherwise it returns the one that is already in
    * the IR manager.
    * @param  ptd type pointed by the pointer type (ir_reindex).
    * @param  algn alignment to assign to the pointer type, or zero to use the default target alignment.
    * @return the ir_reindex node of the pointer type.
    */
   ir_nodeRef GetPointerType(const ir_nodeConstRef& ptd, unsigned long long algn = 0) const;

   /**
    * @brief create an integer type starting from a given prec
    * @param prec is the required integer_ty_node precision
    * @param unsigned_p say if the integer_ty_node required is unsigned or not
    * @return a new integer with a precision equal to prec
    */
   ir_nodeRef GetCustomIntegerType(unsigned long long prec, bool unsigned_p) const;

   /**
    * @brief Create a function type
    *
    * @param returnType is the return type
    * @param argsT is the vector of argument types
    * @return ir_nodeRef is the IR reindex associated with the function type created
    */
   ir_nodeRef GetFunctionType(const ir_nodeConstRef& returnType, const std::vector<ir_nodeConstRef>& argsT) const;

   /// MISCELLANEOUS_OBJ_IR_NODES

   /// SSA_NODE

   /**
    * Function used to create a ssa_node node.
    * @param  var is the variable being referenced.
    * @param  type is the type of the ssa_node
    * @param  min is the minimum value of the ssa_var
    * @param  max is the maximum value of the ssa_var
    * @param  virtual_flag flag for virtual phi (default false).
    * @return the ir_reindex node of the ssa_node.
    *
    */
   ir_nodeRef create_ssa_name(const ir_nodeConstRef& var, const ir_nodeConstRef& type, const ir_nodeConstRef& min,
                              const ir_nodeConstRef& max, bool virtual_flag = false) const;

   /// PHI_STMT

   /**
    * Function used to create a phi_stmt.
    * @param  ssa_res is the SSA result produced by the phi node.
    * @param  list_of_def_edge vector where each tuple contains the incoming reaching definition (ssa_node node) and the
    * edge via which that definition is coming through.
    * @param  function_decl_nid is the identifier of the enclosing function declaration.
    * @param  virtual_flag flag to set if it is a virtual phi_stmt (default false).
    * @return the ir_reindex node of the phi_stmt.
    */
   ir_nodeRef create_phi_node(ir_nodeRef& ssa_res,
                              const std::vector<std::pair<ir_nodeRef, unsigned int>>& list_of_def_edge,
                              unsigned int function_decl_nid, bool virtual_flag = false) const;

   /// ASSIGN_STMT

   /**
    * Function used to create a assign_stmt.
    * @param  op0 is the first operand.
    * @param  op1 is the second operand.
    * @param  function_decl_nid is where the statement is inserted
    * @param  loc_info is the definition of the source position.
    * @return the ir_reindex node of the assign_stmt.
    */
   ir_nodeRef create_assign_stmt(const ir_nodeRef& op0, const ir_nodeRef& op1, unsigned int function_decl_nid,
                                 const std::string& loc_info) const;

   /**
    * Create assign_stmt
    * @param type is the type the assignment
    * @param min is the minimum value of the assigned ssa_var
    * @param max is the maximum value of the assigned ssa_var
    * @param op is the right part
    * @param function_decl_nid is the function where the statement is inserted
    * @param loc_info is the loc_info to be assigned
    */
   ir_nodeRef CreateAssignStmt(const ir_nodeConstRef& type, const ir_nodeConstRef& min, const ir_nodeConstRef& max,
                               const ir_nodeRef& op, unsigned int function_decl_nid, const std::string& loc_info) const;

   /// CALL_STMT
   ir_nodeRef create_call_stmt(const ir_nodeConstRef& called_function, const std::vector<ir_nodeRef>& args,
                               unsigned int function_decl_nid, const std::string& loc_info) const;

   /// RETURN_STMT

   /**
    * Function used to create a new return_stmt.
    * @param  type is the type of the expression returned.
    * @param  expr is the value to return.
    * @param  function_decl_nid is the identifier of the enclosing function declaration.
    * @param  loc_info is the definition of the source position.
    * @return the ir_reindex node of the return_stmt created.
    */
   ir_nodeRef create_return_stmt(const ir_nodeConstRef& type, const ir_nodeConstRef& expr,
                                 unsigned int function_decl_nid, const std::string& loc_info) const;

   /**
    * @brief create the declaration of a function without its body
    * @param function_name is the function name
    * @param parent is the function scope
    * @param argsT is the vector of argument types
    * @param returnType is the return type
    * @param loc_info is the source references
    * @param with_body when true a stub of the body is created
    * @return is the the ir_reindex associated with the function_val_node created.
    */
   ir_nodeRef create_function_decl(const std::string& function_name, const ir_nodeRef& parent,
                                   const std::vector<ir_nodeConstRef>& argsT, const ir_nodeConstRef& returnType,
                                   const std::string& loc_info, bool with_body) const;

   /// UTILITY

   /**
    * Create a not expression if it does not exist in the basic block
    * @param condition is the operand of the not
    * @param block is the basic block in which new statement has tobe added
    * @param function_decl_nid is the function where the statement is inserted
    * @return the ssa in the left part of the created statement
    */
   ir_nodeRef CreateNotExpr(const ir_nodeConstRef& condition, const blocRef& block,
                            unsigned int function_decl_nid) const;

   /**
    * Create an or expression
    * @param first_condition is the first condition
    * @param second_condition is the second condition
    * @param block is the basic block in which new statement has to be added
    * @param function_decl_nid is the function where the statement is inserted
    * @return the ssa in the left part of the created statement
    */
   ir_nodeRef CreateAndExpr(const ir_nodeConstRef& first_condition, const ir_nodeConstRef& second_condition,
                            const blocRef& block, unsigned int function_decl_nid) const;

   /**
    * Create an or expression
    * @param first_condition is the first condition
    * @param second_condition is the second condition
    * @param block is the basic block in which new statement has to be added
    * @param function_decl_nid is the function where the statement is inserted
    * @return the ssa in the left part of the created statement
    */
   ir_nodeRef CreateOrExpr(const ir_nodeConstRef& first_condition, const ir_nodeConstRef& second_condition,
                           const blocRef& block, unsigned int function_decl_nid) const;

   /**
    * Normalize condition
    * @param condition is the raw condition
    * @param block is the basic block in which new possible statement has to be added
    * @param function_decl_nid is the function where any helper statements are inserted
    * @param include_name is the source file name associated with generated helper statements
    * @param line_number is the source line associated with generated helper statements
    * @param column_number is the source column associated with generated helper statements
    */
   ir_nodeRef ExtractCondition(const ir_nodeRef& condition, const blocRef& block, unsigned int function_decl_nid,
                               std::string include_name, unsigned line_number, unsigned column_number) const;

   /**
    * Create a nop_node to perform a conversion
    * @param operand is the operand to be converted
    * @param type is the destination type
    * @param min is the minimum value of the assigned ssa_var
    * @param max is the maximum value of the assigned ssa_var
    * @param function_decl_nid is the function where the statement is inserted
    * @return the assign_stmt containing the nop expr as right part
    */
   ir_nodeRef CreateNopExpr(const ir_nodeConstRef& operand, const ir_nodeConstRef& type, const ir_nodeConstRef& min,
                            const ir_nodeConstRef& max, unsigned int function_decl_nid) const;

   /**
    * Create an unsigned integer type starting from signed type
    */
   ir_nodeRef CreateUnsigned(const ir_nodeConstRef& signed_type) const;

   /**
    * Create an eq_node
    * @param first_operand is the first operand
    * @param second_operand is the second operand
    * @param block is the basic block in which new statement has to be added
    * @param function_decl_nid is the function where the statement is inserted
    * @return the ssa in the left part of the created statement
    */
   ir_nodeRef CreateEqExpr(const ir_nodeConstRef& first_operand, const ir_nodeConstRef& second_operand,
                           const blocRef& block, unsigned int function_decl_nid) const;

   /**
    * Create a call_node
    * @param called_function is the IR reindex of the called function
    * @param args is the arguments of the call
    * @param loc_info is the loc_info to be associated with the call
    * @return the IR reindex of the created node
    */
   ir_nodeRef CreateCallExpr(const ir_nodeConstRef& called_function, const std::vector<ir_nodeRef>& args,
                             const std::string& loc_info) const;

   /**
    * Create an addr_node
    * @param tn is the IR reindex of the object of which you want to take the address
    * @param loc_info is the loc_info to be associated with the call
    * @return the IR reindex of the created node
    */
   ir_nodeRef CreateAddrExpr(const ir_nodeConstRef& tn, const std::string& loc_info) const;

   /**
    * Create a assign_stmt with op0 a new ssa_node, and op1 an addr_node
    * which takes the address of the IR node tn
    * @param tn is the IR reindex of the object of which you want to take the address
    * @param function_decl_nid is the function where the statement is inserted
    * @param loc_info is the loc_info to be associated with the call
    * @return the IR reindex of the created node
    */
   ir_nodeRef CreateAssignStmtAddrExpr(const ir_nodeConstRef& tn, unsigned int function_decl_nid,
                                       const std::string& loc_info) const;

   /**
    * Create a vector type
    * @param elt_type is the type of the element of the vector
    * @param number_of_elements is the number of elements of the vector
    * @return the IR reindex of the created node
    */
   ir_nodeRef CreateVectorType(const ir_nodeConstRef& elt_type, integer_cst_t number_of_elements) const;

   /**
    * @brief CloneFunction duplicates a function
    * @param tn is the IR reindex of the function decl
    * @param funNameSuffix is the suffix added to function_val_node newly created
    * @return ir_reindex of the new function decl
    */
   ir_nodeRef CloneFunction(const ir_nodeRef& tn, const std::string& funNameSuffix);

   /**
    * @brief Execute function call inlining of given call statement (call graph must be recomputed after inlining)
    *
    * @param call_node IR node of the call statement to inline
    * @param caller_node caller function IR node
    * @return unsigned int exit basic block number where statements after inlined call have been moved
    */
   unsigned int InlineFunctionCall(const ir_nodeRef& call_node, const ir_nodeRef& caller_node);

   /**
    * @brief Perform function call versioning
    *
    * @param call_node Call statement IR node
    * @param caller_node Caller function IR node
    * @param version_suffix Suffix applied to the versioned function name
    * @return bool true if versioning happened, false if function was already versioned
    */
   bool VersionFunctionCall(const ir_nodeRef& call_node, const ir_nodeRef& caller_node,
                            const std::string& version_suffix);
};

using ir_manipulationRef = refcount<ir_manipulation>;
using ir_manipulationConstRef = refcount<const ir_manipulation>;

#endif /* IR_MANIPULATION_HPP */
