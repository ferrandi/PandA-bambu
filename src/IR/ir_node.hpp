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
 * @file ir_node.hpp
 * @brief Classes specification of the ir_node data structures.
 *
 * Classes used to described the IR nodes imported from the raw file.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef IR_NODE_HPP
#define IR_NODE_HPP
#include "Range.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "exceptions.hpp"
#include "ir_common.hpp"
#include "panda_types.hpp"
#include "refcount.hpp"

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "config_HAVE_UNORDERED.hpp"

REF_FORWARD_DECL(bloc);
REF_FORWARD_DECL(ir_manager);
CONSTREF_FORWARD_DECL(ir_node);
REF_FORWARD_DECL(ir_node);
enum class IRVocabularyTokenTypes_TokenEnum;

/**
 * Macro which defines the get_kind_text function that returns the parameter as a string.
 */
#define GET_KIND_TEXT(meth)                   \
   std::string get_kind_text() const override \
   {                                          \
      return std::string(#meth);              \
   }

#define NON_LEAF_IR_NODES                                                                                          \
   (ir_node)(IR_LocInfo)(PointToSolution)(decl_node)(expr_node)(node_stmt)(unary_node)(binary_node)(ternary_node)( \
       type_node)(cst_node)

/// sequence of all objects
#define VISITED_OBJ_SEQ1 \
   NON_LEAF_IR_NODES UNARY_EXPRESSION_IR_NODES BINARY_EXPRESSION_IR_NODES TERNARY_EXPRESSION_IR_NODES
#define VISITED_OBJ_SEQ2                                                                                   \
   NODE_STMTS MISCELLANEOUS_EXPR_IR_NODES MISCELLANEOUS_OBJ_IR_NODES TYPE_NODE_IR_NODES CONST_OBJ_IR_NODES \
       DECL_NODE_IR_NODES BASIC_BLOCK_IR_NODES

/// sequence of obj that have to be specialized
#define OBJ_SPECIALIZED_SEQ                                                                                            \
   (ir_node)(IR_LocInfo)(decl_node)(expr_node)(node_stmt)(unary_node)(binary_node)(ternary_node)(type_node)(cst_node)( \
       array_ty_node)(call_node)(call_stmt)(constructor_node)(field_val_node)(function_val_node)(function_ty_node)(    \
       assign_stmt)(identifier_node)(constant_int_val_node)(integer_ty_node)(lut_node)(argument_val_node)(phi_stmt)(   \
       pointer_ty_node)(constant_fp_val_node)(real_ty_node)(struct_ty_node)(return_stmt)(ssa_node)(                    \
       statement_list_node)(variable_val_node)(constant_vector_val_node)(vector_ty_node)(bloc)(multi_way_if_stmt)(     \
       ir_reindex)

#define OBJ_NOT_SPECIALIZED_SEQ     \
   (module_unit_node)(void_ty_node) \
       UNARY_EXPRESSION_IR_NODES BINARY_EXPRESSION_IR_NODES TERNARY_EXPRESSION_IR_NODES(nop_stmt)(PointToSolution)

#include "visitor.hpp"

class ir_node_visitor : public object_visitor
{
};

/**
 * Abstract pure class for the IR structure. This node and in particular its refCount type will be used to describe
 * all nodes read from the IR structure.
 */
class ir_node
{
 public:
   /// Represent the index read from the parsed file and the index-1 of the vector of ir_node associated to the
   /// functions vector present in the ir_manager.
   const unsigned int index;

   explicit ir_node(unsigned int i) : index(i)
   {
   }

   virtual ~ir_node() = default;

   /**
    * Virtual function returning the type of the actual class
    */
   virtual enum kind get_kind() const = 0;

   /**
    * Virtual function returning the name of the actual class.
    */
   virtual std::string get_kind_text() const = 0;

   /// @return the string describing the node.
   virtual std::string ToString() const;

   /**
    * Friend definition of the << operator.
    * @param os is the output stream
    * @param tn is the ir_node to be printed
    */
   friend std::ostream& operator<<(std::ostream& os, const ir_node* tn);

   /**
    * Friend definition of the << operator.
    * @param os is the output stream
    * @param tn is the ir_node to be printed
    */
   friend std::ostream& operator<<(std::ostream& os, const ir_nodeRef& tn);

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   virtual void visit(ir_node_visitor* const v) const;

   /**
    * Given a string, return the corresponding kind
    * @param name is the string
    * @return the corresponding kind
    */
   static enum kind get_kind(const std::string& name);

   /**
    * Given a kind, return the corresponding string
    * @param k is the kind
    * @return the name of the kind
    */
   static std::string GetString(const enum kind k);
};

using ir_nodeRef = refcount<ir_node>;
using ir_nodeConstRef = refcount<const ir_node>;

struct IRNodeSorter
{
   bool operator()(const ir_nodeRef& x, const ir_nodeRef& y) const
   {
      return x->index < y->index;
   }
};

struct IRNodeConstSorter
{
   bool operator()(const ir_nodeConstRef& x, const ir_nodeConstRef& y) const
   {
      return x->index < y->index;
   }
};

struct IRNodeEqual
{
   bool operator()(const ir_nodeRef& x, const ir_nodeRef& y) const
   {
      return x->index == y->index;
   }
};

struct IRNodeConstEqual
{
   bool operator()(const ir_nodeConstRef& x, const ir_nodeConstRef& y) const
   {
      return x->index == y->index;
   }
};

#if HAVE_UNORDERED
struct IRNodeHash
{
   size_t operator()(const ir_nodeRef& tn) const
   {
      return tn->index;
   }
};

struct IRNodeConstHash
{
   size_t operator()(const ir_nodeConstRef& tn) const
   {
      return tn->index;
   }
};

using IRNodeSet = CustomUnorderedSet<ir_nodeRef, IRNodeHash, IRNodeEqual>;
using IRNodeConstSet = CustomUnorderedSet<ir_nodeConstRef, IRNodeConstHash, IRNodeConstEqual>;

template <typename T>
using IRNodeMap = CustomUnorderedMap<ir_nodeRef, T, IRNodeHash, IRNodeEqual>;
template <typename T>
using IRNodeConstMap = CustomUnorderedMap<ir_nodeConstRef, T, IRNodeConstHash, IRNodeConstEqual>;
#else
using IRNodeSet = CustomOrderedSet<ir_nodeRef, IRNodeSorter>;
using IRNodeConstSet = CustomOrderedSet<ir_nodeConstRef, IRNodeConstSorter>;

template <typename T>
using IRNodeMap = CustomOrderedMap<ir_nodeRef, T, IRNodeSorter>;
template <typename T>
using IRNodeConstMap = CustomOrderedMap<ir_nodeConstRef, T, IRNodeConstSorter>;
#endif

using OrderedIRNodeSet = CustomOrderedSet<ir_nodeRef, IRNodeSorter>;
using OrderedIRNodeConstSet = CustomOrderedSet<ir_nodeConstRef, IRNodeConstSorter>;

/**
 * Macro used to hide implementation details when accessing an ir_node from another ir_node
 * @param t is the ir_nodeRef to access
 * @return the pointer to t
 */
#define GET_PTD_NODE(t) (((t) && (t)->get_kind() == ir_reindex_K) ? GetPointerS<ir_reindex>(t)->actual_ir_node : (t))
#define GET_CONST_PTD_NODE(t) \
   (((t) && (t)->get_kind() == ir_reindex_K) ? GetPointerS<const ir_reindex>(t)->actual_ir_node : (t))

/**
 * This macro collects all case labels for unary_node objects.
 * Its use it is quite simple: just add the following line in the switch statement
 * case CASE_UNARY_NODES:
 */
#define CASE_UNARY_NODES             \
   abs_node_K:                       \
   case addr_node_K:                 \
   case not_node_K:                  \
   case fptoi_node_K:                \
   case mem_access_node_K:           \
   case unaligned_mem_access_node_K: \
   case neg_node_K:                  \
   case nop_node_K:                  \
   case bitcast_node_K:              \
   case itofp_node_K

/**
 * This macro collects all case labels for unary_node objects.
 * Its use it is quite simple: just add the following line in the switch statement
 * case CASE_UNARY_NODES:
 */
#define CASE_NON_ADDR_UNARY_EXPRESSION \
   abs_node_K:                         \
   case not_node_K:                    \
   case fptoi_node_K:                  \
   case unaligned_mem_access_node_K:   \
   case neg_node_K:                    \
   case nop_node_K:                    \
   case bitcast_node_K:                \
   case itofp_node_K

/**
 * This macro collects all case labels for binary_node objects.
 */
#define CASE_BINARY_NODES      \
   and_node_K:                 \
   case or_node_K:             \
   case xor_node_K:            \
   case eq_node_K:             \
   case ge_node_K:             \
   case gt_node_K:             \
   case le_node_K:             \
   case shl_node_K:            \
   case lt_node_K:             \
   case max_node_K:            \
   case min_node_K:            \
   case sub_node_K:            \
   case mul_node_K:            \
   case ne_node_K:             \
   case add_node_K:            \
   case gep_node_K:            \
   case fdiv_node_K:           \
   case shr_node_K:            \
   case idiv_node_K:           \
   case irem_node_K:           \
   case widen_mul_node_K:      \
   case extract_bit_node_K:    \
   case add_sat_node_K:        \
   case sub_sat_node_K:        \
   case extractvalue_node_K:   \
   case extractelement_node_K: \
   case frem_node_K

/**
 * This macro collects all case labels for ternary_node objects.
 */
#define CASE_TERNARY_NODES    \
   concat_bit_node_K:         \
   case select_node_K:        \
   case shufflevector_node_K: \
   case ternary_add_node_K:   \
   case ternary_as_node_K:    \
   case ternary_sa_node_K:    \
   case ternary_ss_node_K:    \
   case fshl_node_K:          \
   case fshr_node_K:          \
   case insertvalue_node_K:   \
   case insertelement_node_K

/**
 * This macro collects all case labels for type objects
 */
#define CASE_TYPE_NODES     \
   array_ty_node_K:         \
   case function_ty_node_K: \
   case integer_ty_node_K:  \
   case pointer_ty_node_K:  \
   case real_ty_node_K:     \
   case struct_ty_node_K:   \
   case vector_ty_node_K:   \
   case void_ty_node_K

/**
 * This macro collects all case labels for fake or empty nodes
 */
#define CASE_FAKE_NODES \
   last_ir_K:           \
   case ir_reindex_K

/// NOTE that cast_expr is a unary expression but it could not be included in the CASE_UNARY_NODES because the
/// operand could be null

/**
 * This macro collects all case labels for declaration nodes
 */
#define CASE_DECL_NODES      \
   field_val_node_K:         \
   case function_val_node_K: \
   case argument_val_node_K: \
   case module_unit_node_K:  \
   case variable_val_node_K

/**
 * This macro collects all case labels for cast nodes
 */
#define CASE_CST_NODES          \
   constant_int_val_node_K:     \
   case constant_fp_val_node_K: \
   case constant_vector_val_node_K

/**
 * This macro collects all cases labels for node statement
 */
#define CASE_NODE_STMTS      \
   assign_stmt_K:            \
   case multi_way_if_stmt_K: \
   case nop_stmt_K:          \
   case phi_stmt_K:          \
   case return_stmt_K:       \
   case call_stmt_K

/// macro to create simple IR node classes
#define CREATE_IR_NODE_CLASS(class_name, superclass)               \
   struct class_name : public superclass                           \
   {                                                               \
      GET_KIND_TEXT(class_name)                                    \
      GET_KIND(class_name)                                         \
      virtual void visit(ir_node_visitor* const v) const override; \
      enum                                                         \
      {                                                            \
         GETID(superclass) = 0                                     \
      };                                                           \
      explicit class_name(unsigned int i) : superclass(i)          \
      {                                                            \
      }                                                            \
   }

/**
 * struct definition of the source position.
 */
struct IR_LocInfo : public ir_node
{
   /// include_name is a filename string, this can be the location of a reference, if no definition has been seen.
   std::string include_name;

   /**
    * line_number holds a line number.
    */
   unsigned int line_number;

   /**
    * column_number holds the column number.
    */
   unsigned int column_number;

   explicit IR_LocInfo(unsigned int i) : ir_node(i), line_number(0), column_number(0)
   {
   }

   virtual ~IR_LocInfo() = default;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(ir_node) = 0
   };
};

/**
 * struct definition of the declaration node structures.
 */
struct decl_node : public IR_LocInfo
{
   /// it is the name of a declaration node. It is an identifier_node.
   ir_nodeRef name;

   /// it is the name of the object as the assembler will see it.
   /// Often this is the same as name. It is an identifier_node.
   ir_nodeRef mngl;

   /// type of the declaration.
   ir_nodeRef type;

   /// it is where the decl is declared: global (translation unit) or local (function declaration)
   ir_nodeRef parent;

   /// operating system flag: it's true when this is a variable of operating system library
   bool operating_system_flag;

   /// library system flag: it's true when this is a variable of a standard library (e.g libmath)
   bool library_system_flag;

   /// it is true when this is a declared inside libbambu
   bool libbambu_flag;

   explicit decl_node(unsigned int i);

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(IR_LocInfo) = 0,
      GETID(name),
      GETID(mngl),
      GETID(orig),
      GETID(type),
      GETID(parent)
   };
};

/**
 * struct definition of the common part of an expression
 */
struct expr_node : public IR_LocInfo
{
   /// type of the expression
   ir_nodeRef type;

   explicit expr_node(unsigned int i) : IR_LocInfo(i)
   {
   }

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(IR_LocInfo) = 0,
      GETID(type)
   };
};

/**
 * This struct specifies a point-to solution
 */
struct PointToSolution
{
   /// True if it is not possible to determine where the pointer points to
   bool anything;

   /// True if the points to includes the local escaped solution
   bool escaped;

   /// True if the points to includes the IPA escaped solution
   bool ipa_escaped;

   /// True if the points to includes any global memory
   bool nonlocal;

   /// True if the points to includes nothing
   bool null;

   /// Set of variables that this pointer may point to
   std::vector<ir_nodeRef> variables;

   PointToSolution();

   virtual ~PointToSolution() = default;

   /**
    * Add a symbolic variable to this point to set
    * @param variable is the symbolic variable to be added
    */
   void Add(const std::string& variable);

   /**
    * Add a variable to this point to set
    * @param variable is the variable to be added
    */
   void Add(const ir_nodeRef& variable);

   /**
    * this function check if the point-to set is a singleton or not
    * @param nonlocal_std If true, counts function parameters as standard variables
    * @return true in case the point-to is a singleton, false otherwise
    */
   bool is_a_singleton(bool nonlocal_std = false) const;

   /**
    * this function check if the point-to set resolved w.r.t. standard variables
    * @param nonlocal_std If true, counts function parameters as standard variables
    * @return true in case the point-to point-to set resolved w.r.t. standard variables, false otherwise
    */
   bool is_fully_resolved(bool nonlocal_std = false) const;

   /**
    * Print this point-to solution
    * @return the string containing the point-to solution
    */
   std::string ToString() const;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   virtual void visit(ir_node_visitor* const v) const;

   /// visitor enum
   enum
   {
      GETID(variables) = 0
   };
};

/**
 * struct definition of the common part of a statement
 */
struct node_stmt : public IR_LocInfo
{
   /// whole memory operand use
   ir_nodeRef memuse;

   /// whole memory operand def
   ir_nodeRef memdef;

   /// vdef of this statement
   ir_nodeRef vdef;

   /// vuses of this statement
   OrderedIRNodeSet vuses;

   /// vovers of this statement
   OrderedIRNodeSet vovers;

   /// parent of the object.
   ir_nodeRef parent;

   /// The basic block to which this node_stmt belongs
   unsigned int bb_index;

   /// The operation
   std::string operation;

   /// The predicate
   ir_nodeRef predicate;

   /// this field is true if the node_stmt was created artificially to handle some specific situations, like for
   /// example handling functions returning structs by value or accepting structs by value as parameters
   bool artificial;

   /// when true CSE and Bit Value optimization will not remove from the IR
   bool keep;

   /**
    * Constructor
    * @param i is the index of the node
    */
   explicit node_stmt(unsigned int i);

   /**
    * Add a vdef
    * @param vdef is the vdef
    */
   void SetVdef(const ir_nodeRef& vdef);

   /**
    * Add a vuse
    * @param vuse is the vuse
    * @return bool True if vuse has been added, false if vuse already exists
    */
   bool AddVuse(const ir_nodeRef& vuse);

   /**
    * Add a vover
    * @param vover is the vover
    * @return bool True if vover has been added, false if vover already exists
    */
   bool AddVover(const ir_nodeRef& vover);

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(IR_LocInfo) = 0,
      GETID(memuse),
      GETID(memdef),
      GETID(vuses),
      GETID(vdef),
      GETID(vovers),
      GETID(parent),
      GETID(predicate)
   };
};

/**
 * struct definition of the unary node structures.
 */
struct unary_node : public expr_node
{
   /// op field is the operand of the unary expression
   ir_nodeRef op;

   explicit unary_node(unsigned int i) : expr_node(i)
   {
   }

   /// @return the string describing the node.
   std::string ToString() const override;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(expr_node) = 0,
      GETID(op)
   };
};

/**
 * struct definition of the binary node structures.
 */
struct binary_node : public expr_node
{
   /// The first operand of the binary expression
   ir_nodeRef op0;

   /// The second operand of the binary expression
   ir_nodeRef op1;

   explicit binary_node(unsigned int i) : expr_node(i)
   {
   }

   /// @return the string describing the node.
   std::string ToString() const override;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(expr_node) = 0,
      GETID(op0),
      GETID(op1)
   };
};

/**
 * struct definition of the ternary node structures.
 */
struct ternary_node : public expr_node
{
   /// The first operand of the ternary expression
   ir_nodeRef op0;

   /// The second operand of the ternary expression
   ir_nodeRef op1;

   /// The third operand of the ternary expression
   ir_nodeRef op2;

   explicit ternary_node(unsigned int i) : expr_node(i)
   {
   }

   /// @return the string describing the node.
   std::string ToString() const override;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(expr_node) = 0,
      GETID(op0),
      GETID(op1),
      GETID(op2)
   };
};

/**
 * struct definition of the type node structures.
 */
struct type_node : public ir_node
{
   /// bitsizealloc is the allocation bitsize
   unsigned bitsizealloc;

   /// alignment
   unsigned int algn;

   /// system flag: it's true when this is a system variable
   bool system_flag;

   /// it is true when this is a declared inside libbambu
   bool libbambu_flag;

   explicit type_node(unsigned int i) : ir_node(i), bitsizealloc(0), algn(0), system_flag(false), libbambu_flag(false)
   {
   }

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(ir_node) = 0,
   };
};

/**
 * This struct specifies super class for constant nodes
 */
struct cst_node : ir_node
{
   /// type field is the type of the node
   ir_nodeRef type;

   explicit cst_node(unsigned int i) : ir_node(i)
   {
   }

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(ir_node) = 0,
      GETID(type)
   };
};

/**
 * struct definition of the array_ty_node IR node.
 */
struct array_ty_node : public type_node
{
   /// field elts is the type of an array element
   ir_nodeRef elts;

   /// number of elements in the array
   unsigned long long nelements;

   explicit array_ty_node(unsigned int i) : type_node(i), nelements(0)
   {
   }

   /// @return the string describing the node.
   std::string ToString() const override;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(array_ty_node)

   /// Redefinition of get_kind.
   GET_KIND(array_ty_node)

   /// visitor enum
   enum
   {
      GETID(type_node) = 0,
      GETID(elts)
   };
};

/**
 * This struct specifies the call_node node.
 * Function call.  Operand 0 is the function.
 * Operand 1 is the argument list, a list of expressions made out of a chain of nodes.
 * Operand 2 is the static chain argument, or NULL.
 */
struct call_node : public expr_node
{
   /// fn is the operand 0 of the call expression: this is the function
   ir_nodeRef fn;

   /// The arguments of the call_node
   std::vector<ir_nodeRef> args;

   explicit call_node(const unsigned int i) : expr_node(i)
   {
   }

   /**
    * Add an argument to the list of arguments
    * @param arg is the argument to be added
    */
   void AddArg(const ir_nodeRef& arg);

   /// @return the string describing the node.
   std::string ToString() const override;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(call_node)

   /// Redefinition of get_kind.
   GET_KIND(call_node)

   /// visitor enum
   enum
   {
      GETID(expr_node) = 0,
      GETID(fn),
      GETID(args)
   };
};

/**
 * This struct specifies the call_stmt node.
 */
struct call_stmt : public node_stmt
{
   /// fn is the operand 0 of the call expression: this is the function
   ir_nodeRef fn;

   /// The arguments of the call_stmt
   std::vector<ir_nodeRef> args;

   explicit call_stmt(const unsigned int i) : node_stmt(i)
   {
   }

   /// @return the string describing the node.
   std::string ToString() const override;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /**
    * Add an argument to the list of arguments
    * @param arg is the argument to be added
    */
   void AddArg(const ir_nodeRef& arg);

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(call_stmt)

   /// Redefinition of get_kind.
   GET_KIND(call_stmt)

   /// visitor enum
   enum
   {
      GETID(node_stmt) = 0,
      GETID(fn),
      GETID(args)
   };
};

/**
 * Constructor: return an aggregate value made from specified components.
 */
struct constructor_node : public ir_node
{
   /// type field is the type of the node
   ir_nodeRef type;

   /// store the list initializers: (\c index, value)
   std::vector<std::pair<ir_nodeRef, ir_nodeRef>> list_of_idx_valu;

   explicit constructor_node(unsigned int i) : ir_node(i)
   {
   }

   /**
    * Add a pair (\c index, value) to the list of idx_val.
    * @param idx is the index.
    * @param valu is the value.
    */
   void add_idx_valu(const ir_nodeRef& idx, const ir_nodeRef& valu)
   {
      list_of_idx_valu.push_back(std::pair<ir_nodeRef, ir_nodeRef>(idx, valu));
   }

   /**
    * Add a pair <null, value> to the list of idx_val.
    * @param valu is the value.
    */
   void add_valu(const ir_nodeRef& valu)
   {
      list_of_idx_valu.push_back(std::pair<ir_nodeRef, ir_nodeRef>(ir_nodeRef(), valu));
   }

   /// @return the string describing the node.
   std::string ToString() const override;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(constructor_node)

   /// Redefinition of get_kind.
   GET_KIND(constructor_node)

   /// visitor enum
   enum
   {
      GETID(ir_node) = 0,
      GETID(type),
      GETID(list_of_idx_valu)
   };
};

/**
 * struct definition of the field_val_node IR node.
 */
struct field_val_node : public decl_node
{
   /// true when the field is a bitfield
   bool bitfield;

   /// bitsizealloc is the allocation bitsize
   unsigned bitsizealloc;

   /// alignment
   unsigned int algn;

   /// Indicates this field should be bit-packed
   bool packed_flag;

   /// offset in bits of the field
   integer_cst_t offset;

   explicit field_val_node(unsigned int i) : decl_node(i), bitfield(false), algn(0), packed_flag(false), offset(0)
   {
   }

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(field_val_node)

   /// Redefinition of get_kind.
   GET_KIND(field_val_node)

   /// visitor enum
   enum
   {
      GETID(decl_node) = 0
   };
};

/**
 * struct definition of the function_val_node IR node.
 */
struct function_val_node : public decl_node
{
   using pipeline_style_t = enum { STP_STYLE, FLP_STYLE, FRP_STYLE };

   /// True if function write in memory somehow
   bool writing_memory;

   /// True if function read from memory somehow
   bool reading_memory;

   /// True if pipelining is enabled for the function
   bool pipeline_enabled;

   /// Specify function pipelining style
   pipeline_style_t pipeline_style;

   /// initiation time in case function is pipelined
   unsigned initiation_time;

   /// fn field is the initial declaration for this function declaration
   ir_nodeRef fn;

   /// for each bit of the SSA variable tells if it is equal to U,X,0,1
   std::string bit_values;

   /// Range information about bounds of the function return value (valid for real_ty_node too)
   Range range;

   /// args field holds a chain of argument_val_node nodes for the arguments.
   std::vector<ir_nodeRef> list_of_args;

   /// flag true when the function is a builtin
   bool builtin_flag;

   /// static_flag is true if function has been defined
   bool static_flag;

   /// body field is the saved representation of the body of the entire function.
   ir_nodeRef body;

   explicit function_val_node(unsigned int i)
       : decl_node(i),
         writing_memory(false),
         reading_memory(false),
         pipeline_enabled(false),
         pipeline_style(FRP_STYLE),
         initiation_time(1),
         range(Unknown, 32),
         builtin_flag(false),
         static_flag(false)
   {
   }

   /**
    * Add a parameter to the list of the paramters.
    * @param a is the parameter to be added.
    */
   void AddArg(const ir_nodeRef& a);

   /// returns true if is a declaration of a pipelined function
   bool is_pipelined();

   void set_pipelining(bool v);

   void set_pipeline_style(pipeline_style_t ps);

   pipeline_style_t get_pipeline_style() const;

   unsigned get_initiation_time();

   void set_initiation_time(unsigned time);

   /// @return the string describing the node.
   std::string ToString() const override;

   std::string ToStringDef() const;

   std::string ToStringDecl() const;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(function_val_node)

   /// Redefinition of get_kind.
   GET_KIND(function_val_node)

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(decl_node) = 0,
      GETID(fn),
      GETID(list_of_args),
      GETID(body)
   };
};

/**
 * struct definition of the function_ty_node IR node.
 */
struct function_ty_node : public type_node
{
   /// varargs flag: tells if function is of varargs type
   bool varargs_flag;

   /// retn field is the type of value returned.
   ir_nodeRef retn;

   /// args field holds a chain of argument_val_node nodes for the arguments.
   std::vector<ir_nodeRef> list_of_args_type;

   void AddArgType(const ir_nodeRef& a)
   {
      list_of_args_type.push_back(a);
   }

   explicit function_ty_node(unsigned int i) : type_node(i), varargs_flag(false)
   {
   }

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(function_ty_node)

   /// Redefinition of get_kind.
   GET_KIND(function_ty_node)

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(type_node) = 0,
      GETID(retn),
      GETID(list_of_args_type)
   };
};

/**
 * This struct specifies the assignment node.
 */
struct assign_stmt : public node_stmt
{
   /// The first operand of the binary expression
   ir_nodeRef op0;

   /// The second operand of the binary expression
   ir_nodeRef op1;

   bool temporary_address;

   explicit assign_stmt(unsigned int i) : node_stmt(i), temporary_address(false)
   {
   }

   /// @return the string describing the node.
   std::string ToString() const override;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(assign_stmt)

   /// Redefinition of get_kind.
   GET_KIND(assign_stmt)

   /// visitor enum
   enum
   {
      GETID(node_stmt) = 0,
      GETID(op0),
      GETID(op1)
   };
};

struct nop_stmt : public node_stmt
{
   explicit nop_stmt(unsigned int i) : node_stmt(i)
   {
   }

   /// @return the string describing the node.
   std::string ToString() const override;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(nop_stmt)

   /// Redefinition of get_kind.
   GET_KIND(nop_stmt)

   /// visitor enum
   enum
   {
      GETID(node_stmt) = 0
   };
};

/**
 * struct definition of the function_val_node IR node.
 */
struct identifier_node : public ir_node
{
   /// Store the identifier string associated with the identifier_node.
   std::string strg;

   /// constructors
   identifier_node(unsigned int node_id, std::string _strg, ir_manager* TM);

   /// @return the string describing the node.
   std::string ToString() const override;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(identifier_node)

   /// Redefinition of get_kind.
   GET_KIND(identifier_node)

   /// visitor enum
   enum
   {
      GETID(ir_node) = 0
   };
};

/**
 * This struct specifies the constant_int_val_node node.
 */
struct constant_int_val_node : public cst_node
{
   /// The value of the integer cast
   integer_cst_t value;

   explicit constant_int_val_node(unsigned int i) : cst_node(i), value(0)
   {
   }

   /// @return the string describing the node.
   std::string ToString() const override;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(constant_int_val_node)

   /// Redefinition of get_kind.
   GET_KIND(constant_int_val_node)

   /// visitor enum
   enum
   {
      GETID(cst_node) = 0
   };
};

/**
 * struct definition of the integer_ty_node IR node.
 */
struct integer_ty_node : public type_node
{
   /// bitsize of the data type
   unsigned int bitsize;

   /// integer is unsigned
   bool unsigned_flag;

   explicit integer_ty_node(unsigned int i) : type_node(i), bitsize(0), unsigned_flag(false)
   {
   }

   /// @return the string describing the node.
   std::string ToString() const override;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(integer_ty_node)

   /// Redefinition of get_kind.
   GET_KIND(integer_ty_node)

   /// visitor enum
   enum
   {
      GETID(type_node) = 0
   };
};

/**
 * struct definition of the argument_val_node IR node.
 */
struct argument_val_node : public decl_node
{
   /// bitsizealloc is the allocation bitsize
   unsigned bitsizealloc;

   /// alignment
   unsigned int algn;

   /// read-only parameter
   bool readonly_flag;

   /// Range information about bounds of the function parameter (valid for real_ty_node too)
   Range range;

   explicit argument_val_node(unsigned int i)
       : decl_node(i), bitsizealloc(0), algn(0), readonly_flag(false), range(Unknown, 32)
   {
   }

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(argument_val_node)

   /// Redefinition of get_kind.
   GET_KIND(argument_val_node)

   /// visitor enum
   enum
   {
      GETID(decl_node) = 0,
      GETID(argt)
   };
};

/**
 * This struct specifies the PHI node.
 */
struct phi_stmt : public node_stmt
{
   friend class ir_manager;
   friend class parm2ssa;
   friend class ir_reindex_remove;

   /// The type of the def edge
   using DefEdge = std::pair<ir_nodeRef, unsigned int>;

   /// The type of the def edge list
   using DefEdgeList = std::list<DefEdge>;

   /// res is the new SSA_NODE node created by the PHI node.
   ir_nodeRef res;

   /// flag for virtual phi
   bool virtual_flag;

 private:
   /// store the list pairs: <def, edge>. Each tuple contains the incoming reaching definition (SSA_NODE node) and the
   /// edge via which that definition is coming through.
   DefEdgeList list_of_def_edge;

   /// True if SSA uses are updated
   bool updated_ssa_uses;

 public:
   /**
    * Constructor
    * @param i is the index of the node to be created
    */
   explicit phi_stmt(unsigned int i) : node_stmt(i), virtual_flag(false), updated_ssa_uses(false)
   {
   }

   /// @return the string describing the node.
   std::string ToString() const override;

   /**
    * Return the list of def edges
    * @return the list of def edges
    */
   const DefEdgeList& CGetDefEdgesList() const;

   /**
    * Remove a defedge
    * @param TM is the IR manager
    * @param to_be_removed is the def edge to be removed
    */
   void RemoveDefEdge(const ir_managerRef& TM, const DefEdge& to_be_removed);

   /**
    * Add a defedge
    * @param TM is the IR manager
    * @param def_edge is the def edge to be added
    */
   void AddDefEdge(const ir_managerRef& TM, const DefEdge& def_edge);

   /**
    * Replace a defedge
    * @param TM is the IR manager
    * @param old_def_edge is the def edge to be removed
    * @param new_def_edge is the def edge to be added
    */
   void ReplaceDefEdge(const ir_managerRef& TM, const DefEdge& old_def_edge, const DefEdge& new_def_edge);

   /**
    * Set the def edge list removing the ond one
    * @param TM is the IR manager
    * @param new_list_of_def_edge is the new def edge list
    */
   void SetDefEdgeList(const ir_managerRef& TM, DefEdgeList new_list_of_def_edge);

   /**
    * Set that uses of ssa have been computed
    */
   void SetSSAUsesComputed();

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(phi_stmt)

   /// Redefinition of get_kind.
   GET_KIND(phi_stmt)

   /// visitor enum
   enum
   {
      GETID(node_stmt) = 0,
      GETID(res),
      GETID(list_of_def_edge)
   };
};

/**
 * struct definition of the pointer_ty_node node.
 */
struct pointer_ty_node : public type_node
{
   /// ptd field points to the node for the type pointed to.
   ir_nodeRef ptd;

   explicit pointer_ty_node(unsigned int i) : type_node(i)
   {
   }

   /// @return the string describing the node.
   std::string ToString() const override;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(pointer_ty_node)

   /// Redefinition of get_kind.
   GET_KIND(pointer_ty_node)

   /// visitor enum
   enum
   {
      GETID(type_node) = 0,
      GETID(ptd)
   };
};

/**
 * This struct specifies the constant_fp_val_node node.
 */
struct constant_fp_val_node : public cst_node
{
   /// overflow_flag means there was an overflow in folding, and no warning has been issued for this subexpression.
   bool overflow_flag;

   /// valr is the real value
   std::string valr;

   /// valx field
   std::string valx;

   explicit constant_fp_val_node(unsigned int i) : cst_node(i), overflow_flag(false)
   {
   }

   /// @return the string describing the node.
   std::string ToString() const override;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(constant_fp_val_node)

   /// Redefinition of get_kind.
   GET_KIND(constant_fp_val_node)

   /// visitor enum
   enum
   {
      GETID(cst_node) = 0
   };
};

/**
 * struct definition of the real_ty_node node.
 */
struct real_ty_node : public type_node
{
   /// bitsize of the data
   unsigned int bitsize;

   explicit real_ty_node(unsigned int i) : type_node(i), bitsize(0)
   {
   }

   /// @return the string describing the node.
   std::string ToString() const override;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(real_ty_node)

   /// Redefinition of get_kind.
   GET_KIND(real_ty_node)

   /// visitor enum
   enum
   {
      GETID(type_node) = 0
   };
};

/**
 * struct definition of the struct_ty_node node.
 */
struct struct_ty_node : public type_node
{
   /// name of the type
   ir_nodeRef name;

   /// Indicated that objects of this type should be laid out in as compact a way as possible
   bool packed_flag;

   /// list_of_flds is a chain of field_val_node for the fields of the struct,
   std::vector<ir_nodeRef> list_of_flds;

   explicit struct_ty_node(unsigned int i) : type_node(i), packed_flag(false)
   {
   }

   /**
    * Add a field_val_node to list of flds.
    * @param a is a NODE_ID.
    */
   void add_flds(const ir_nodeRef& a)
   {
      list_of_flds.push_back(a);
   }

   /**
    * returns ir_nodeRef of the field specified by offset
    * @param offset is the offset of the field from the base address of the struct_ty_node
    * @return the ir_nodeRef if the offset is valid else null pointer
    */
   ir_nodeRef get_field(integer_cst_t offset);

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(struct_ty_node)

   /// Redefinition of get_kind.
   GET_KIND(struct_ty_node)

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(type_node) = 0,
      GETID(name),
      GETID(list_of_flds)
   };
};

/**
 * This struct specifies the return_stmt node.
 * Evaluates the operand, then returns from the current function.
 * Presumably that operand is an assignment that stores into the RESULT_DECL that hold the value to be returned.
 * The operand may be null. The type should be void and the value should be ignored.
 */
struct return_stmt : public node_stmt
{
   /// op field is the operand of this node
   ir_nodeRef op;

   explicit return_stmt(unsigned int i) : node_stmt(i)
   {
   }

   /// @return the string describing the node.
   std::string ToString() const override;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(return_stmt)

   /// Redefinition of get_kind.
   GET_KIND(return_stmt)

   /// visitor enum
   enum
   {
      GETID(node_stmt) = 0,
      GETID(op)
   };
};

/**
 * This struct specifies the ssa_node node.
 */
struct ssa_node : public ir_node
{
 private:
   /// The uses of this SSA: it is a map since the same SSA can be used multiple times in the same statement
   /// Key is the index of the IR node instead of IR node because in this way node_stmt can updated uses
   /// of a SSA without using the IR manager to get the IR re-index of itself
   IRNodeMap<size_t> use_stmts;

   /// the statement which defines the ssa
   ir_nodeRef def_stmt;

 public:
   ir_nodeRef type;

   /// var is the variable being referenced.
   ir_nodeRef var;

   /// vers is the SSA version number of this SSA name.
   unsigned int vers;

   /// flag for virtual SSA
   bool virtual_flag;

   /**
    * Nonzero if this SSA_NODE is the default definition for the
    * underlying symbol.  A default SSA name is created for symbol S if
    * the very first reference to S in the function is a read operation.
    * Default definitions are always created by an empty statement and
    * belong to no basic block.
    * */
   bool default_flag;

   /// minimum values this SSA may reach
   ir_nodeRef min;

   /// maximum values this SSA may reach
   ir_nodeRef max;

   /// for each bit of the SSA variable tells if it is equal to U,X,0,1
   std::string bit_values;

   /// Range information about numerical values of the SSA variable
   Range range;

   /// point to solution
   PointToSolution use_set;

   explicit ssa_node(unsigned int i) : ir_node(i), vers(0), virtual_flag(false), default_flag(false), range(Unknown, 32)
   {
   }

   /**
    * Add use of this SSA
    * @param use_stmt is the statement which uses this SSA
    */
   void AddUseStmt(const ir_nodeRef& use_stmt);

   /**
    * Return the use stmts
    * @return the use stmts
    */
   const IRNodeMap<size_t>& CGetUseStmts() const
   {
      return use_stmts;
   }

   /**
    * Return the number of uses
    * @return the number of uses
    */
   size_t CGetNumberUses() const;

   /**
    * Remove a use of this SSA
    * @param use_stmt is the statement which uses this SSA
    */
   void RemoveUse(const ir_nodeRef& use_stmt);

   /**
    * Set the def stmt erasing the old definitions
    * @param def is a def statement.
    */
   void SetDefStmt(const ir_nodeRef& def)
   {
      def_stmt = def;
   }

   /**
    * Return the def stmt (checking that is unique)
    * @return the definition statement
    */
   ir_nodeRef GetDefStmt() const
   {
      return def_stmt;
   }

   /// @return the string describing the node.
   std::string ToString() const override;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(ssa_node)

   /// Redefinition of get_kind.
   GET_KIND(ssa_node)

   /// visitor enum
   enum
   {
      GETID(ir_node) = 0,
      GETID(type),
      GETID(var),
      GETID(use_stmts),
      GETID(def_stmt),
      GETID(min),
      GETID(max),
      GETID(use_set)
   };
};

/**
 * This struct specifies the statement_list_node node.
 */
struct statement_list_node : public ir_node
{
   /// list_of_bloc field is the list of basic block.
   std::map<unsigned int, blocRef> list_of_bloc;

   explicit statement_list_node(unsigned int i) : ir_node(i)
   {
   }

   /**
    * Add a value to list of basic block.
    * @param a is a NODE_ID.
    */
   void add_bloc(const blocRef& a);

   /// @return the string describing the node.
   std::string ToString() const override;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(statement_list_node)

   /// Redefinition of get_kind.
   GET_KIND(statement_list_node)

   /// visitor enum
   enum
   {
      GETID(ir_node) = 0,
      GETID(list_of_bloc)
   };
};

/**
 * This struct specifies a multi-way-if construct
 */
struct multi_way_if_stmt : public node_stmt
{
   /// The list of pair condition basic block
   std::list<std::pair<ir_nodeRef, unsigned int>> list_of_cond;

   /**
    * Add a pair <cond, bb_index> to the list of cond.
    * @param cond is the condition.
    * @param bb_ind is the basic block index.
    */
   void add_cond(const ir_nodeRef& cond, unsigned int bb_ind)
   {
      list_of_cond.emplace_back(cond, bb_ind);
   }

   explicit multi_way_if_stmt(unsigned int i) : node_stmt(i)
   {
   }

   /// @return the string describing the node.
   std::string ToString() const override;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(multi_way_if_stmt)

   /// Redefinition of get_kind.
   GET_KIND(multi_way_if_stmt)

   /// visitor enum
   enum
   {
      GETID(node_stmt) = 0,
      GETID(list_of_cond)
   };
};

/**
 * This struct specifies the vector node.
 */

/**
 * struct definition of the variable_val_node node.
 */
struct variable_val_node : public decl_node
{
   /// to manage standard static attribute
   bool static_flag;

   /// a variable can be extern
   bool extern_flag;

   /// True when we are able to prove that its address is not taken and do not escape.
   /// It is defined by LLVM/CLANG and it refers mainly to alloca type variables
   bool addr_not_taken;

   /// possible initialization
   ir_nodeRef init;

   /// bitsizealloc is the allocation bitsize
   unsigned bitsizealloc;

   /// alignment
   unsigned int algn;

   /// readonly variable
   bool readonly_flag;

   /// for each bit of the variable_val_node tells if it is equal to U,X,0,1
   /// meaningful only in case the variable is readonly
   std::string bit_values;

   explicit variable_val_node(unsigned int i)
       : decl_node(i), static_flag(false), extern_flag(false), addr_not_taken(false), algn(0), readonly_flag(false)
   {
   }

   /// @return the string describing the node.
   std::string ToString() const override;

   std::string ToStringDecl() const;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(variable_val_node)

   /// Redefinition of get_kind.
   GET_KIND(variable_val_node)

   /// visitor enum
   enum
   {
      GETID(decl_node) = 0,
      GETID(init)
   };
};

/**
 * This struct specifies the constant_vector_val_node node.
 * Contents are stored in the vector constant elements field.
 */
struct constant_vector_val_node : public cst_node
{
   /// list_of_valu stores the values of the vector constant elements.
   std::vector<ir_nodeRef> list_of_valu;

   explicit constant_vector_val_node(unsigned int i) : cst_node(i)
   {
   }

   /**
    * Add a value to list of value.
    * @param a is a NODE_ID.
    */
   void add_valu(const ir_nodeRef& a)
   {
      list_of_valu.push_back(a);
   }

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(constant_vector_val_node)

   /// Redefinition of get_kind.
   GET_KIND(constant_vector_val_node)

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(cst_node) = 0,
      GETID(list_of_valu)
   };
};

/**
 * struct definition of the vector_ty_node node.
 */
struct vector_ty_node : public type_node
{
   /// field elts is the type of an vector element
   ir_nodeRef elts;

   explicit vector_ty_node(unsigned int i) : type_node(i)
   {
   }

   /// @return the string describing the node.
   std::string ToString() const override;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(vector_ty_node)

   /// Redefinition of get_kind.
   GET_KIND(vector_ty_node)

   /// visitor enum
   enum
   {
      GETID(type_node) = 0,
      GETID(elts)
   };
};

/**
 * This struct specifies the lut_node node.
 */
struct lut_node : public expr_node
{
   /// true table constant
   ir_nodeRef op0;

   /// first operand
   ir_nodeRef op1;

   /// second operand
   ir_nodeRef op2;

   /// third operand
   ir_nodeRef op3;

   /// fourth operand
   ir_nodeRef op4;

   /// fifth operand
   ir_nodeRef op5;

   /// sixth operand
   ir_nodeRef op6;

   /// seventh operand
   ir_nodeRef op7;

   /// eighth operand
   ir_nodeRef op8;

   explicit lut_node(unsigned int i) : expr_node(i)
   {
   }

   /// @return the string describing the node.
   std::string ToString() const override;

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(lut_node)

   /// Redefinition of get_kind.
   GET_KIND(lut_node)

   /// visitor enum
   enum
   {
      GETID(expr_node) = 0,
      GETID(op0),
      GETID(op1),
      GETID(op2),
      GETID(op3),
      GETID(op4),
      GETID(op5),
      GETID(op6),
      GETID(op7),
      GETID(op8)
   };
};

/**
 * Definition of the module_unit_node node.
 */
CREATE_IR_NODE_CLASS(module_unit_node, decl_node);

/**
 * Definition of the void_ty_node node.
 */
CREATE_IR_NODE_CLASS(void_ty_node, type_node);

/**
 * Represents the absolute value of the operand.
 */
CREATE_IR_NODE_CLASS(abs_node, unary_node);

/**
 * Return the address of the operand.
 */
CREATE_IR_NODE_CLASS(addr_node, unary_node);

/**
 * Definition of the not_node node.
 */
CREATE_IR_NODE_CLASS(not_node, unary_node);

/**
 * Conversion of real to integer.
 */
CREATE_IR_NODE_CLASS(fptoi_node, unary_node);

/**
 * Conversion of an integer to a real.
 */
CREATE_IR_NODE_CLASS(itofp_node, unary_node);

/**
 * Load/Store node
 */
CREATE_IR_NODE_CLASS(mem_access_node, unary_node);

/**
 * Definition of the unaligned version of the load/store nodes.
 */
CREATE_IR_NODE_CLASS(unaligned_mem_access_node, unary_node);

/**
 * Unary negation.
 */
CREATE_IR_NODE_CLASS(neg_node, unary_node);

/**
 * Definition of cast/conversion node.
 */
CREATE_IR_NODE_CLASS(nop_node, unary_node);

/**
 * Instruction converting value to a type without changing any bits.
 */
CREATE_IR_NODE_CLASS(bitcast_node, unary_node);

/**
 * Bitwise logical and of its two operands.
 */
CREATE_IR_NODE_CLASS(and_node, binary_node);

/**
 * Bitwise logical inclusive or of its two operands.
 */
CREATE_IR_NODE_CLASS(or_node, binary_node);

/**
 * Bitwise logical exclusive or of its two operands.
 */
CREATE_IR_NODE_CLASS(xor_node, binary_node);

/**
 * Equal comparison node
 */
CREATE_IR_NODE_CLASS(eq_node, binary_node);

/**
 * Extracts a bit value from a SSA/integer const
 */
CREATE_IR_NODE_CLASS(extract_bit_node, binary_node);

/**
 * Extracts a single scalar element from a vector at a specified index.
 */
CREATE_IR_NODE_CLASS(extractelement_node, binary_node);

/**
 * Extracts the value of a member field from an aggregate value.
 */
CREATE_IR_NODE_CLASS(extractvalue_node, binary_node);

/**
 * Remainder of the division of its two operands.
 */
CREATE_IR_NODE_CLASS(frem_node, binary_node);

/**
 * Greater than or equal comparison node.
 */
CREATE_IR_NODE_CLASS(ge_node, binary_node);

/**
 * Greater than comparison node.
 */
CREATE_IR_NODE_CLASS(gt_node, binary_node);

/**
 * Less than or equal comparison node.
 */
CREATE_IR_NODE_CLASS(le_node, binary_node);

/**
 * First operand (signed or not) shifted to the left a specified number of bits.
 */
CREATE_IR_NODE_CLASS(shl_node, binary_node);

/**
 * Less than comparison node.
 */
CREATE_IR_NODE_CLASS(lt_node, binary_node);

/**
 * Return the larger of its operand.
 */
CREATE_IR_NODE_CLASS(max_node, binary_node);

/**
 *  Return the smaller of its operand.
 */
CREATE_IR_NODE_CLASS(min_node, binary_node);

/**
 * Returns the difference of its two operands.
 */
CREATE_IR_NODE_CLASS(sub_node, binary_node);

/**
 * Returns the product of its two operands.
 */
CREATE_IR_NODE_CLASS(mul_node, binary_node);

/**
 * Not equal comparison node.
 */
CREATE_IR_NODE_CLASS(ne_node, binary_node);

/**
 * Returns the sum of its two operands.
 */
CREATE_IR_NODE_CLASS(add_node, binary_node);

/**
 * Pointer addition node.
 */
CREATE_IR_NODE_CLASS(gep_node, binary_node);

/**
 * floating point division.
 */
CREATE_IR_NODE_CLASS(fdiv_node, binary_node);

/**
 * First operand (signed or not) shifted to the right a specified number of bits.
 */
CREATE_IR_NODE_CLASS(shr_node, binary_node);

/**
 * Saturating subtraction on its arguments.
 */
CREATE_IR_NODE_CLASS(sub_sat_node, binary_node);

/**
 * Saturating addition on its arguments.
 */
CREATE_IR_NODE_CLASS(add_sat_node, binary_node);

/**
 * Integer division.
 */
CREATE_IR_NODE_CLASS(idiv_node, binary_node);

/**
 * Remainder from the integer division of its two arguments.
 */
CREATE_IR_NODE_CLASS(irem_node, binary_node);

/**
 * Wide multiplication (A_bw * B_bw -> C_2bw).
 */
CREATE_IR_NODE_CLASS(widen_mul_node, binary_node);

/**
 * Concatenation between in1 and in2 using in3 bits.
 */
CREATE_IR_NODE_CLASS(concat_bit_node, ternary_node);

/**
 * Choose one value based on a condition, without IR-level branching
 */
CREATE_IR_NODE_CLASS(select_node, ternary_node);

/**
 * Funnel shift left node.
 */
CREATE_IR_NODE_CLASS(fshl_node, ternary_node);

/**
 * Funnel shift right node.
 */
CREATE_IR_NODE_CLASS(fshr_node, ternary_node);

/**
 * Inserts a scalar element into a vector at a specified index.
 */
CREATE_IR_NODE_CLASS(insertelement_node, ternary_node);

/**
 * Inserts a value into a member field in an aggregate value.
 */
CREATE_IR_NODE_CLASS(insertvalue_node, ternary_node);

/**
 * The shufflevector node constructs a permutation of elements from two input vectors, returning a vector with
 * the same element type as the input and length that is the same as the shuffle mask.
 */
CREATE_IR_NODE_CLASS(shufflevector_node, ternary_node);

/**
 * Compute a - b - c node.
 */
CREATE_IR_NODE_CLASS(ternary_ss_node, ternary_node);

/**
 * Compute a - b + c node.
 */
CREATE_IR_NODE_CLASS(ternary_sa_node, ternary_node);

/**
 * Compute a + b + c node.
 */
CREATE_IR_NODE_CLASS(ternary_add_node, ternary_node);

/**
 * Compute a + b - c node.
 */
CREATE_IR_NODE_CLASS(ternary_as_node, ternary_node);

#endif
