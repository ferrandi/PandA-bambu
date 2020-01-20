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
 * @file tree_manipulation.hpp
 * @brief Class defining some useful functions to create tree nodes and to manipulate the tree manager.
 *
 * This class defines some useful functions to create tree nodes and to manipulate the tree manager.
 *
 * @author ste <stefano.viazzi@gmail.com>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef TREE_MANIPULATION_HPP
#define TREE_MANIPULATION_HPP

/// Utility include
#include "refcount.hpp"

#include "custom_map.hpp"
#include <string>  // for string
#include <utility> // for pair
#include <vector>

/**
 * @name forward declarations
 */
//@{
enum kind : int;
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(tree_node);
CONSTREF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_manipulation);
enum class TreeVocabularyTokenTypes_TokenEnum;
REF_FORWARD_DECL(bloc);
//@}

#define ALGN_BIT_SIZE 64
#define ALGN_UNSIGNED_INT 32
#define ALGN_INT 32
#define ALGN_LONG_LONG_INT 64
#define ALGN_UNSIGNED_LONG_LONG_INT 64
#define ALGN_VOID 8
#define ALGN_BOOLEAN 8
#define ALGN_POINTER 32

#define PREC_BIT_SIZE 64
#define PREC_UNSIGNED_INT 32
#define PREC_INT 32
#define PREC_LONG_LONG_INT 64
#define PREC_UNSIGNED_LONG_LONG_INT 64

#define SIZE_VALUE_BIT_SIZE 64
#define SIZE_VALUE_UNSIGNED_INT 32
#define SIZE_VALUE_LONG_LONG_INT 64
#define SIZE_VALUE_UNSIGNED_LONG_LONG_INT 64
#define SIZE_VALUE_INT 32
#define SIZE_VALUE_BOOL 8
#define SIZE_VALUE_FUNCTION 8
#define SIZE_VALUE_POINTER 32

#define MIN_VALUE_BIT_SIZE 0
#define MIN_VALUE_UNSIGNED_INT 0
#define MIN_VALUE_INT -2147483648LL
#define MIN_VALUE_LONG_LONG_INT 0
#define MIN_VALUE_UNSIGNED_LONG_LONG_INT 0

#define MAX_VALUE_BIT_SIZE -1
#define MAX_VALUE_UNSIGNED_INT -1
#define MAX_VALUE_INT 2147483647
#define MAX_VALUE_LONG_LONG_INT -1
#define MAX_VALUE_UNSIGNED_LONG_LONG_INT -1

/**
 * This class creates a layer to add nodes and to manipulate the tree_nodes manager.
 */
class tree_manipulation
{
 private:
   /// Tree Manager
   tree_managerRef TreeM;

   /// True if statements can be reused
   const bool reuse;

   /// The set of input parameters
   const ParameterConstRef parameters;

   /// debug level.
   const int debug_level;

   /// store a unique id used during the creation of the label_decl associated with a gimple_goto.
   static unsigned int goto_label_unique_id;

 public:
   /**
    * This is the constructor of the tree_manipulation.
    * @param TreeM is the tree manager.
    * @param parameters is the set of input parameters
    */
   tree_manipulation(const tree_managerRef& TreeM, const ParameterConstRef& parameters);
   tree_manipulation(const tree_managerRef& TreeM, const ParameterConstRef& parameters, bool _reuse);

   /**
    * This is the destructor of the tree_manipulation.
    */
   ~tree_manipulation();

   /// EXPRESSION_TREE_NODES

   /**
    * Function used to create an unary expression.
    * @param  type is the type of the expression (tree_reindex).
    * @param  op is the operand of the unary expression (tree_reindex).
    * @param  srcp is the definition of the source position.
    * @param  operation_kind is the kind of unary expression to create (bit_not_expr_K, nop_expr_K,...).
    * @return the tree_reindex node of the operation created.
    */
   tree_nodeRef create_unary_operation(const tree_nodeRef& type, const tree_nodeRef& op, const std::string& srcp, enum kind operation_kind) const;

   /**
    * Function used to create a binary expression.
    * @param  type is the type of the expression (tree_reindex).
    * @param  op0 is the first operand of the binary expression (tree_reindex).
    * @param  op1 is the second operand of the binary expression (tree_reindex).
    * @param  srcp is the definition of the source position.
    * @param  operation_kind is the kind of binary expression to create (bit_and_expr_K, plus_expr_K,...).
    * @return the tree_reindex node of the operation created.
    */
   tree_nodeRef create_binary_operation(const tree_nodeRef& type, const tree_nodeRef& op0, const tree_nodeRef& op1, const std::string& srcp, enum kind operation_kind) const;

   /**
    * Function used to create a ternary expression.
    * @param  type is the type of the expression (tree_reindex).
    * @param  op0 is the first operand of the ternary expression (tree_reindex).
    * @param  op1 is the second operand of the ternary expression (tree_reindex).
    * @param  op2 is the third operand of the ternary expression (tree_reindex).
    * @param  srcp is the definition of the source position.
    * @param  operation_kind is the kind of ternary expression to create (component_ref_K, gimple_switch_K,...).
    * @return the tree_reindex node of the operation created.
    */
   tree_nodeRef create_ternary_operation(const tree_nodeRef& type, const tree_nodeRef& op0, const tree_nodeRef& op1, const tree_nodeRef& op2, const std::string& srcp, enum kind operation_kind) const;

   /**
    * Function used to create a quaternary expression.
    * @param  type is the type of the expression (tree_reindex).
    * @param  op0 is the first operand of the quaternary expression (tree_reindex).
    * @param  op1 is the second operand of the quaternary expression (tree_reindex).
    * @param  op2 is the third operand of the quaternary expression (tree_reindex).
    * @param  op3 is the fourth operand of the quaternary expression (tree_reindex).
    * @param  srcp is the definition of the source position.
    * @param  operation_kind is the kind of quaternary expression to create (array_range_ref_K, array_ref_K).
    * @return the tree_reindex node of the operation created.
    */
   tree_nodeRef create_quaternary_operation(const tree_nodeRef& type, const tree_nodeRef& op0, const tree_nodeRef& op1, const tree_nodeRef& op2, const tree_nodeRef& op3, const std::string& srcp, enum kind operation_kind) const;

   /**
    * @brief create_lut_expr: function used to create a generic lut_expr operation
    * @param type is the type of the expression (tree_reindex).
    * @param op0 describes the LUT functionality as a constant integer.
    * @param op1 is the first operand of the lut_expr expression (tree_reindex).
    * @param op2 is the second operand of the lut_expr expression (tree_reindex).
    * @param op3 is the third operand of the lut_expr expression (tree_reindex).
    * @param op4 is the fourth operand of the lut_expr expression (tree_reindex).
    * @param op5 is the fifth operand of the lut_expr expression (tree_reindex).
    * @param op6 is the sixth operand of the lut_expr expression (tree_reindex).
    * @param op7 is the seventh operand of the lut_expr expression (tree_reindex).
    * @param op8 is the eighth operand of the lut_expr expression (tree_reindex).
    * @param srcp is the definition of the source position.
    * @return the tree_reindex node of the operation created.
    */
   tree_nodeRef create_lut_expr(const tree_nodeRef& type, const tree_nodeRef& op0, const tree_nodeRef& op1, const tree_nodeRef& op2, const tree_nodeRef& op3, const tree_nodeRef& op4, const tree_nodeRef& op5, const tree_nodeRef& op6,
                                const tree_nodeRef& op7, const tree_nodeRef& op8, const std::string& srcp) const;

   tree_nodeRef create_extract_bit_expr(const tree_nodeRef& op0, const tree_nodeRef& op1, const std::string& srcp) const;

   /// CONST_OBJ_TREE_NODES
   /** Function used to create a integer_cst node.
    * @param  type is the type of the node.
    * @param value is the value of the constant
    * @param  integer_cst_nid is the index node of the object to be created
    * @return the tree_reindex node of the integer_cst created.
    */
   tree_nodeRef CreateIntegerCst(const tree_nodeConstRef& type, const long long int value, const unsigned int integer_cst_nid) const;

   /** Function used to create a real_cst node.
    * @param  type is the type of the node.
    * @param value is the value of the constant
    * @param  real_cst_nid is the index node of the object to be created
    * @return the tree_reindex node of the real_cst created.
    */
   tree_nodeRef CreateRealCst(const tree_nodeConstRef& type, const long double value, const unsigned int real_cst_nid) const;

   /// IDENTIFIER_TREE_NODE
   /**
    * Function used to create an identifier node if it is not already present, otherwise it returns the one that is already in the tree manager.
    * @param  strg is the identifier string associated with the identifier_node.
    * @return the tree_reindex node of the identifier_node created.
    */
   tree_nodeRef create_identifier_node(const std::string& strg) const;

   /// DECL_NODES

   /**
    * Function used to create a parm_decl.
    * @param  name is the name field containing an identifier_node used to represent a name.
    * @param  type is the type.
    * @param  scpe is the scope declaration.
    * @param  size is the size field holding the size of datum, in bits.
    * @param  argt is the type in which the argument is actually passed.
    * @param  smt_ann is the symbol_memory_tag annotation.
    * @param  init is the init field holding the value to initialize a variable to.
    * @param  srcp is the definition of the source position.
    * @param  algn is the field holding the alignment required for the datum, in bits.
    * @param  used is nonzero if the name is used in its scope
    * @param  register_flag means declared 'register'
    * @param  readonly_flag means declared 'readonly'
    * @return the tree_reindex node of the parm_decl created.
    */
   tree_nodeRef create_parm_decl(const tree_nodeRef& name, const tree_nodeRef& type, const tree_nodeRef& scpe, const tree_nodeRef& size, const tree_nodeRef& argt, const tree_nodeRef& smt_ann, const tree_nodeRef& init, const std::string& srcp,
                                 unsigned int algn, int used, bool register_flag = false, bool readonly_flag = false) const;

   /// create or find a global scope
   tree_nodeRef create_translation_unit_decl() const;

   /**
    * Function used to create a result_decl.
    * @param  name is the name field containing an identifier_node used to represent a name.
    * @param  type is the type.
    * @param  scpe is the scope declaration.
    * @param  size is the size field holding the size of datum, in bits.
    * @param  smt_ann is the symbol_memory_tag annotation (default empty).
    * @param  init is the init field holding the value to initialize a variable to.
    * @param  srcp is the definition of the source position.
    * @param  algn is the field holding the alignment required for the datum, in bits.
    * @param  artificial_flag used to indicate that this decl_node represents a compiler-generated entity (default false).
    * @return the tree_reindex node of the result_decl created.
    */
   tree_nodeRef create_result_decl(const tree_nodeRef& name, const tree_nodeRef& type, const tree_nodeRef& scpe, const tree_nodeRef& size, const tree_nodeRef& smt_ann, const tree_nodeRef& init, const std::string& srcp, unsigned int algn,
                                   bool artificial_flag = false) const;

   /**
    * Function used to create a var_decl.
    * @param  name is the name field containing an identifier_node used to represent a name.
    * @param  type is the type.
    * @param  scpe is the scope declaration.
    * @param  size is the size field holding the size of datum, in bits.
    * @param  smt_ann is the symbol_memory_tag annotation.
    * @param  init is the init field holding the value to initialize a variable to.
    * @param  srcp is the definition of the source position.
    * @param  algn is the field holding the alignment required for the datum, in bits.
    * @param  used is nonzero if the name is used in its scope
    * @param  artificial_flag used to indicate that this decl_node represents a compiler-generated entity (default false).
    * @param  use_tmpl indicates whether or not (and how) a template was expanded for this VAR_DECL (default -1).
    * @param  static_static_flag to manage C++ code with static member (default false).
    * @param  static_flag to manage standard static attribute (default false).
    * @param  extern_flag a variable can be extern (default false).
    * @param  init field holds the value to initialize a variable to (default false).
    * @return the tree_reindex node of the var_decl created.
    */
   tree_nodeRef create_var_decl(const tree_nodeRef& name, const tree_nodeConstRef& type, const tree_nodeRef& scpe, const tree_nodeRef& size, const tree_nodeRef& smt_ann, const tree_nodeRef& init, const std::string& srcp, unsigned int algn, int used,
                                bool artificial_flag = false, int use_tmpl = -1, bool static_static_flag = false, bool extern_flag = false, bool static_flag = false, bool register_flag = false, bool readonly_flag = false,
                                const std::string& bit_values = "", bool addr_taken = false, bool addr_not_taken = false) const;

   /// TYPE_OBJ

   /**
    * Function that creates a void type if it is not already present, otherwise it returns the one that is already in the tree manager.
    * @return the tree_reindex node of the void type.
    */
   tree_nodeRef create_void_type() const;

   /**
    * Function that creates a boolean type if it is not already present, otherwise it returns the one that is already in the tree manager.
    * @return the tree_reindex node of the boolean type.
    */
   tree_nodeRef create_boolean_type() const;

   /**
    * Function that creates a integer type if it is not already present, otherwise it returns the one that is already in the tree manager.
    * @return the tree_reindex node of the integer type.
    */
   tree_nodeRef create_default_integer_type() const;

   /**
    * Function that creates a unsigned integer type if it is not already present, otherwise it returns the one that is already in the tree manager.
    * @return the tree_reindex node of the unsigned integer type.
    */
   tree_nodeRef create_default_unsigned_integer_type() const;

   /**
    * Function that creates a long long unsigned int type if it is not already present, otherwise return the existing type
    */
   tree_nodeRef CreateDefaultUnsignedLongLongInt() const;

   /**
    * Function that creates a bit_size type if it is not already present, otherwise it returns the one that is already in the tree manager.
    * @return the tree_reindex node of the bit_size type.
    */
   tree_nodeRef create_bit_size_type() const;

   /**
    * create a sizetype builtin type in case it has not already been created, otherwise it returns the one found in the tree manager.
    */
   tree_nodeRef create_size_type() const;

   /**
    * Function that creates a pointer type if it is not already present, otherwise it returns the one that is already in the tree manager.
    * @param  ptd type pointed by the pointer type (tree_reindex).
    * @return the tree_reindex node of the pointer type.
    */
   tree_nodeRef create_pointer_type(const tree_nodeConstRef& ptd, unsigned int algn) const;

   /**
    * @brief create an integer type starting from a given prec
    * @param prec is the required integer_type precision
    * @param unsigned_p say if the integer_type required is unsigned or not
    * @return a new integer with a precision equal to prec
    */
   tree_nodeRef create_integer_type_with_prec(unsigned int prec, bool unsigned_p) const;

   /// MISCELLANEOUS_OBJ_TREE_NODES

   /// SSA_NAME

   /**
    * Function used to create a ssa_name node.
    * @param  var is the variable being referenced.
    * @param  type is the type of the ssa_node
    * @param  min is the minimum value of the ssa_var
    * @param  max is the maximum value of the ssa_var
    * @param  volatile_flag flag set to true in case a ssa_name is never defined (default false).
    * @param  virtual_flag flag for virtual phi (default false).
    * @return the tree_reindex node of the ssa_name.
    *
    */
   tree_nodeRef create_ssa_name(const tree_nodeConstRef& var, const tree_nodeConstRef& type, const tree_nodeConstRef& min, const tree_nodeConstRef& max, bool volatile_flag = false, bool virtual_flag = false) const;

   /// GIMPLE_PHI

   /**
    * Function used to create a gimple_phi.
    * @param  ssa_res is the new ssa_name node created by the phi node.
    * @param  list_of_def_edge vector where each tuple contains the incoming reaching definition (ssa_name node) and the edge via which that definition is coming through.
    * @param  bb_index is the basic block index associated with the gimple phi
    * @param  virtual_flag flag to set if it is a virtual gimple_phi (default false).
    * @return the tree_reindex node of the gimple_phi.
    */
   tree_nodeRef create_phi_node(tree_nodeRef& ssa_res, const std::vector<std::pair<tree_nodeRef, unsigned int>>& list_of_def_edge, const tree_nodeRef& scpe, unsigned int bb_index, bool virtual_flag = false) const;

   /// GIMPLE_ASSIGN

   /**
    * Function used to create a gimple_assign.
    * @param  op0 is the first operand.
    * @param  op1 is the second operand.
    * @param  srcp is the definition of the source position.
    * @param  bb_index is the basic block index associated with the gimple statement
    * @return the tree_reindex node of the gimple_assign.
    */
   tree_nodeRef create_gimple_modify_stmt(const tree_nodeRef& op0, const tree_nodeRef& op1, const std::string& srcp, const unsigned int bb_index) const;

   /**
    * Create gimple assignment
    * @param type is the type the assignment
    * @param min is the minimum value of the assigned ssa_var
    * @param max is the maximum value of the assigned ssa_var
    * @param op is the right part
    * @param bb_index is the index of the basic block index
    * @param srcp is the srcp to be assigned
    */
   tree_nodeRef CreateGimpleAssign(const tree_nodeRef& type, const tree_nodeConstRef& min, const tree_nodeConstRef& max, const tree_nodeRef& op, unsigned int bb_index, const std::string& srcp) const;

   /// GIMPLE_CALL
   tree_nodeRef create_gimple_call(const tree_nodeConstRef& called_function, const std::vector<tree_nodeRef>& args, const std::string& srcp, const unsigned int bb_index) const;

   /// GIMPLE_COND

   /**
    * Function used to create a new gimple_cond with only one operand and of void type.
    * @param  expr is the condition and can be of any type.
    * @param  srcp is the definition of the source position.
    * @return the tree_reindex node of the gimple_cond created.
    * @param  bb_index is the basic block index associated with the gimple statement
    * @return the tree_reindex node of the gimple_cond created.
    */
   tree_nodeRef create_gimple_cond(const tree_nodeRef& expr, const std::string& srcp, unsigned int bb_index) const;

   /// GIMPLE_RETURN

   /**
    * Function used to create a new gimple_return.
    * @param  type is the type of the expression returned.
    * @param  expr is the value to return.
    * @param  srcp is the definition of the source position.
    * @param  bb_index is the basic block index associated with the gimple statement
    * @return the tree_reindex node of the gimple_return created.
    */
   tree_nodeRef create_gimple_return(const tree_nodeRef& type, const tree_nodeRef& expr, const std::string& srcp, unsigned int bb_index) const;

   /**
    * create a label expression in a header of a loop.
    * @param block is the basic block where the label expression has to be inserted.
    * @param function_decl_nid is the node id of the function where the label expression is defined.
    */
   void create_label(const blocRef& block, unsigned int function_decl_nid) const;

   /**
    * create a goto expression.
    * @param block is the basic block where the goto expression has to be inserted.
    * @param function_decl_nid is the node id of the function where the goto expression is defined.
    * @param label_expr_nid is the node id of the label expression associated with the goto expression.
    */
   void create_goto(const blocRef& block, unsigned int function_decl_nid, unsigned int label_expr_nid) const;

   /**
    * @brief create the declaration of a function without its body
    * @param function_name is the function name
    * @param scpe is the function scope
    * @param args is the vector of argument types
    * @param srcp is the source references
    * @param with_body when true a stub of the body is created
    * @return is the the tree_reindex associated with the function_decl created.
    */
   tree_nodeRef create_function_decl(const std::string& function_name, const tree_nodeRef& scpe, const std::vector<tree_nodeRef>& argsT, const tree_nodeRef& returnType, const std::string& srcp, bool with_body) const;

   /// BASIC BLOCK

   /**
    * Function used to create a new basic block.
    * @param  list_of_bloc is the list of basic blocks.
    * @param  predecessors is the list of predecessors of the basic block to be created (indexes of the blocks).
    * @param  successors is the list of successors of the basic block to be created (indexes of the blocks).
    * @param  stmt is the list of the stmt node to be inserted in the basic block (tree_reindex nodes).
    * @param  number is the new basic block number
    * @param  true_edge is the index of the basic block it goes if there is a jump expression and the condition is satisfied (default=0).
    * @param  false_edge is the index of the basic block it goes if there is a jump expression and the condition is not satisfied (default=0).
    * @param  phi is the list of the gimple_phi to be inserted (default empty).
    * @return basic block created.
    */
   blocRef create_basic_block(std::map<unsigned int, blocRef>& list_of_bloc, std::vector<unsigned int> predecessors, std::vector<unsigned int> successors, std::vector<tree_nodeRef> stmt, unsigned int number, unsigned int true_edge = 0,
                              unsigned int false_edge = 0, std::vector<tree_nodeRef> phi = std::vector<tree_nodeRef>()) const;

   /**
    * Function that adds in basic block bb the statements listed in vector stmt.
    * @param bb is the basic block.
    * @param stmt is the vector of statements.
    */
   void bb_add_stmts(blocRef& bb, const std::vector<tree_nodeRef>& stmt) const;

   /**
    * Function that adds in basic block bb the statement stmt.
    * @param bb is the basic block.
    * @param stmt is the statement.
    */
   void bb_add_stmt(blocRef& bb, const tree_nodeRef& stmt) const;

   /**
    * Function that adds a list of successors in basic block bb.
    * @param bb is the basic block.
    * @param successors is the vector of successors.
    */
   void bb_add_successors(blocRef& bb, const std::vector<unsigned int>& successors) const;

   /**
    * Function that adds a successor in basic block bb.
    * @param bb is the basic block.
    * @param successor is the successor.
    */
   void bb_add_successor(blocRef& bb, const unsigned int& successor) const;

   /**
    * Function that adds a list of predecessors in basic block bb.
    * @param bb is the basic block.
    * @param predecessors is the vector of predecessors.
    */
   void bb_add_predecessors(blocRef& bb, const std::vector<unsigned int>& predecessors) const;

   /**
    * Function that adds a predecessor in basic block bb.
    * @param bb is the basic block.
    * @param predecessor is the predecessor.
    */
   void bb_add_predecessor(blocRef& bb, const unsigned int& predecessor) const;

   /**
    * Function that removes a list of successors in basic block bb.
    * @param bb is the basic block.
    * @param successors is the vector of successors.
    */
   void bb_remove_successors(blocRef& bb, const std::vector<unsigned int>& successors) const;

   /**
    * Function that removes a successor in basic block bb.
    * @param bb is the basic block.
    * @param successor is the successor.
    */
   void bb_remove_successor(blocRef& bb, const unsigned int& successor) const;

   /**
    * Function that removes a list of predecessors in basic block bb.
    * @param bb is the basic block.
    * @param predecessors is the vector of predecessors.
    */
   void bb_remove_predecessors(blocRef& bb, const std::vector<unsigned int>& predecessors) const;

   /**
    * Function that removes a predecessor in basic block bb.
    * @param bb is the basic block.
    * @param predecessor is the predecessor.
    */
   void bb_remove_predecessor(blocRef& bb, const unsigned int& predecessor) const;

   /**
    * Function that removes all the predecessors of basic block bb.
    * @param bb is the basic block.
    */
   void bb_remove_all_predecessors(blocRef& bb) const;

   /**
    * Function that removes all the successors of basic block bb.
    * @param bb is the basic block.
    */
   void bb_remove_all_successors(blocRef& bb) const;

   /**
    * Function that sets in basic block bb the index of the then basic block in case the last statement is a gimple_cond.
    * @param bb is the basic block.
    * @param false_edge_index is the index of the then basic block.
    */
   void bb_set_false_edge(blocRef& bb, const unsigned int& false_edge_index) const;

   /**
    * Function that sets in basic block bb the index of the if basic block in case the last statement is a gimple_cond.
    * @param bb is the basic block.
    * @param true_edge_index is the index of the if basic block.
    */
   void bb_set_true_edge(blocRef& bb, const unsigned int& true_edge_index) const;

   /// UTILITY

   /**
    * Create a not expression if it does not exist in the basic block
    * @param condition is the operand of the not
    * @param block is the basic block in which new statement has tobe added
    * @return the ssa in the left part of the created statement
    */
   tree_nodeRef CreateNotExpr(const tree_nodeConstRef& condition, const blocRef& block) const;

   /**
    * Create an or expression
    * @param first_condition is the first condition
    * @param second_condition is the second condition
    * @param block is the basic block in which new statement has to be added
    * @return the ssa in the left part of the created statement
    */
   tree_nodeRef CreateAndExpr(const tree_nodeConstRef& first_condition, const tree_nodeConstRef& second_condition, const blocRef& block) const;

   /**
    * Create an or expression
    * @param first_condition is the first condition
    * @param second_condition is the second condition
    * @param block is the basic block in which new statement has to be added
    * @return the ssa in the left part of the created statement
    */
   tree_nodeRef CreateOrExpr(const tree_nodeConstRef& first_condition, const tree_nodeConstRef& second_condition, const blocRef& block) const;

   /**
    * Extract computation of condition from a gimple_cond
    * @param gc is the gimple_cond (already eliminated)
    * @param block is the basic block in which new statement has to be added
    * @return the ssa in the left part of the created statement
    */
   tree_nodeRef ExtractCondition(const tree_nodeRef& gc, const blocRef& block) const;

   /**
    * Create a nop_expr to perform a conversion
    * @param operand is the operand to be converted
    * @param type is the destination type
    * @param min is the minimum value of the assigned ssa_var
    * @param max is the maximum value of the assigned ssa_var
    * @return the gimple assignment containing the nop expr as right part
    */
   tree_nodeRef CreateNopExpr(const tree_nodeConstRef& operand, const tree_nodeConstRef& type, const tree_nodeConstRef& min, const tree_nodeConstRef& max) const;

   /**
    * Create an unsigned integer type starting from signed type
    */
   tree_nodeRef CreateUnsigned(const tree_nodeConstRef& signed_type) const;

   /**
    * Create an eq_expr
    * @param first_operand is the first operand
    * @param second_operand is the second operand
    * @param block is the basic block in which new statement has to be added
    * @return the ssa in the left part of the created statement
    */
   tree_nodeRef CreateEqExpr(const tree_nodeConstRef& first_operand, const tree_nodeConstRef& second_operand, const blocRef& block) const;

   /**
    * Create a call_expr
    * @param called_function is the tree reindex of the called function
    * @param args is the arguments of the call
    * @param srcp is the srcp to be associated with the call
    * @return the tree reindex of the created node
    */
   tree_nodeRef CreateCallExpr(const tree_nodeConstRef& called_function, const std::vector<tree_nodeRef>& args, const std::string& srcp) const;

   /**
    * Create an addr_expr
    * @param tn is the tree reindex of the object of which you want to take the address
    * @param srcp is the srcp to be associated with the call
    * @return the tree reindex of the created node
    */
   tree_nodeRef CreateAddrExpr(const tree_nodeConstRef& tn, const std::string& srcp) const;

   /**
    * Create a gimple_assign with op0 a new ssa_name, and op1 an addr_expr
    * which takes the address of the tree node tn
    * @param tn is the tree reindex of the object of which you want to take the address
    * @param bb_index is the basic block index associated with the gimple_assign
    * @param srcp is the srcp to be associated with the call
    * @return the tree reindex of the created node
    */
   tree_nodeRef CreateGimpleAssignAddrExpr(const tree_nodeConstRef& tn, const unsigned int bb_index, const std::string& srcp) const;

   /**
    * Create a vector bool type
    * @param number_of_elements is the number of elements of a vector
    * @return the tree reindex of the created node
    */
   tree_nodeRef CreateVectorBooleanType(const unsigned int number_of_elements) const;
};

typedef refcount<tree_manipulation> tree_manipulationRef;
typedef refcount<const tree_manipulation> tree_manipulationConstRef;

#endif /* TREE_MANIPULATION_HPP */
