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
 * @file tree_node.hpp
 * @brief Classes specification of the tree_node data structures.
 *
 * Classes used to described the tree nodes imported from the raw file.
 * The first version of this code is due to Fabrizio Ferrandi, Katia Turati, Giacomo Galbiati
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef TREE_NODE_HPP
#define TREE_NODE_HPP

/// Autoheader include
#include "config_HAVE_BAMBU_BUILT.hpp"
#include "config_HAVE_CODE_ESTIMATION_BUILT.hpp"
#include "config_HAVE_FROM_PRAGMA_BUILT.hpp"
#include "config_HAVE_TUCANO_BUILT.hpp"
#include "config_HAVE_UNORDERED.hpp"

#include <cstddef>    // for size_t
#include <functional> // for binary_function
#include <iosfwd>     // for ostream
#include <list>       // for list
#include <memory>     // for allocator_traits...
#include <string>     // for string
#include <utility>    // for pair
#include <vector>     // for vector

#include "custom_map.hpp" // for CustomMap
#include "custom_set.hpp"
#include "exceptions.hpp"  // for throw_error
#include "refcount.hpp"    // for GetPointer, refc...
#include "tree_common.hpp" // for GET_KIND, BINARY...

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(bloc);
REF_FORWARD_DECL(tree_manager);
CONSTREF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(Range);
template <class value>
class TreeNodeMap;
enum class TreeVocabularyTokenTypes_TokenEnum;
#if HAVE_CODE_ESTIMATION_BUILT
REF_FORWARD_DECL(WeightInformation);
#endif
//@}

/**
 * Macro which defines the get_kind_text function that returns the parameter as a string.
 */
#define GET_KIND_TEXT(meth)                   \
   std::string get_kind_text() const override \
   {                                          \
      return std::string(#meth);              \
   }

#define NON_LEAF_TREE_NODES (tree_node)(WeightedNode)(attr)(srcp)(PointToSolution)(decl_node)(expr_node)(gimple_node)(unary_expr)(binary_expr)(ternary_expr)(quaternary_expr)(type_node)(memory_tag)(cst_node)

/// sequence of all objects
#define VISITED_OBJ_SEQ1 NON_LEAF_TREE_NODES UNARY_EXPRESSION_TREE_NODES BINARY_EXPRESSION_TREE_NODES TERNARY_EXPRESSION_TREE_NODES
#define VISITED_OBJ_SEQ2 QUATERNARY_EXPRESSION_TREE_NODES GIMPLE_NODES MISCELLANEOUS_EXPR_TREE_NODES MISCELLANEOUS_OBJ_TREE_NODES TYPE_NODE_TREE_NODES CONST_OBJ_TREE_NODES DECL_NODE_TREE_NODES BASIC_BLOCK_TREE_NODES PANDA_EXTENSION_TREE_NODES

/// sequence of obj that have to be specialized
#define OBJ_SPECIALIZED_SEQ                                                                                                                                                                                                                                 \
   (tree_node)(WeightedNode)(attr)(srcp)(decl_node)(expr_node)(gimple_node)(unary_expr)(binary_expr)(ternary_expr)(quaternary_expr)(type_node)(memory_tag)(cst_node)(error_mark)(array_type)(gimple_asm)(baselink)(gimple_bind)(binfo)(block)(call_expr)(   \
       aggr_init_expr)(gimple_call)(case_label_expr)(cast_expr)(complex_cst)(complex_type)(gimple_cond)(const_decl)(constructor)(enumeral_type)(expr_stmt)(field_decl)(function_decl)(function_type)(gimple_assign)(gimple_goto)(handler)(identifier_node)( \
       integer_cst)(integer_type)(gimple_label)(lut_expr)(method_type)(namespace_decl)(overload)(parm_decl)(gimple_phi)(pointer_type)(real_cst)(real_type)(record_type)(reference_type)(result_decl)(gimple_return)(return_stmt)(type_pack_expansion)(      \
       expr_pack_expansion)(scope_ref)(ssa_name)(statement_list)(string_cst)(gimple_switch)(template_decl)(template_parm_index)(tree_list)(tree_vec)(try_block)(type_decl)(union_type)(var_decl)(vector_cst)(vector_type)(type_argument_pack)(              \
       nontype_argument_pack)(target_expr)(target_mem_ref)(target_mem_ref461)(bloc)(null_node)(gimple_pragma)(issue_pragma)(blackbox_pragma)(profiling_pragma)(statistical_profiling)(map_pragma)(call_hw_pragma)(call_point_hw_pragma)(omp_pragma)(        \
       omp_critical_pragma)(omp_declare_simd_pragma)(omp_for_pragma)(omp_parallel_pragma)(omp_sections_pragma)(omp_parallel_sections_pragma)(omp_section_pragma)(omp_simd_pragma)(omp_target_pragma)(omp_task_pragma)(gimple_while)(gimple_for)(            \
       gimple_multi_way_if)(tree_reindex)

#define OBJ_NOT_SPECIALIZED_SEQ                                                                                                                                                                                                                     \
   (translation_unit_decl)(label_decl)(void_type)(template_type_parm)(set_type)(qual_union_type)(offset_type)(lang_type)(CharType)(nullptr_type)(boolean_type)(typename_type)(none)(vec_new_expr)                                                   \
       UNARY_EXPRESSION_TREE_NODES BINARY_EXPRESSION_TREE_NODES TERNARY_EXPRESSION_TREE_NODES(ctor_initializer)(trait_expr)(template_id_expr)(placeholder_expr)(new_expr)(gimple_resx)(gimple_predict)(gimple_nop)QUATERNARY_EXPRESSION_TREE_NODES( \
           modop_expr)(PointToSolution)(omp_atomic_pragma)(using_decl)(void_cst)

#include "visitor.hpp"

class tree_node_visitor : public object_visitor
{
};

/**
 * Abstract pure class for the tree structure. This node and in particular its refCount type will be used to describe all
 * nodes readed from the tree gcc raw structure.
 */
class tree_node
{
 private:
   /// Map string to corresponding enum
   static std::map<std::string, enum kind> string_to_kind;

   /// Map kind to string
   static std::map<enum kind, std::string> kind_to_string;

 public:
   /**
    * Represent the index readed from the raw file and the index-1 of the vector
    * of tree_node associated to the functions vector present in the tree_manager.
    */
   const unsigned int index;

   /**
    * Constructor
    */
   explicit tree_node(unsigned int i) : index(i)
   {
   }

   /// Destructor
   virtual ~tree_node()
   {
   }

   /**
    * Virtual function returning the type of the actual class
    */
   virtual enum kind get_kind() const = 0;

   /**
    * Virtual function returning the name of the actual class.
    */
   virtual std::string get_kind_text() const = 0;

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   virtual void visit(tree_node_visitor* const v) const;

   /**
    * Friend definition of the << operator.
    * @param os is the output stream
    * @param tn is the tree_node to be printed
    */
   friend std::ostream& operator<<(std::ostream& os, const tree_node* tn);

   /**
    * Friend definition of the << operator.
    * @param os is the output stream
    * @param tn is the tree_node to be printed
    */
   friend std::ostream& operator<<(std::ostream& os, const tree_nodeRef& tn);

   /**
    * Print this node as string in gimple format
    * @return the string containing the gimple representation of this node
    */
   std::string ToString() const;

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

/**
 * RefCount type definition of the tree_node class structure
 */
typedef refcount<tree_node> tree_nodeRef;
typedef refcount<const tree_node> tree_nodeConstRef;

/**
 * A set of const tree node
 */
#if HAVE_UNORDERED
struct TreeNodeConstHash : public std::unary_function<tree_nodeConstRef, size_t>
{
   size_t operator()(tree_nodeConstRef tn) const
   {
      std::hash<unsigned int> hasher;
      return hasher(tn->index);
   }
};

struct TreeNodeConstEqualTo : public std::binary_function<tree_nodeConstRef, tree_nodeConstRef, bool>
{
 public:
   /**
    * Constructor
    */
   TreeNodeConstEqualTo();

   /**
    * Compare two const tree nodes
    * @param x is the first tree node
    * @param y is the second tree node
    * @return true if index of x is the same of y
    */
   bool operator()(const tree_nodeConstRef x, const tree_nodeConstRef y) const;
};

class TreeNodeConstSet : public CustomUnorderedSet<tree_nodeConstRef, TreeNodeConstHash, TreeNodeConstEqualTo>
{
};
#else
class TreeNodeConstSorter : std::binary_function<tree_nodeConstRef, tree_nodeConstRef, bool>
{
 public:
   /**
    * Constructor
    */
   TreeNodeConstSorter();

   /**
    * Compare position of two const tree nodes
    * @param x is the first tree node
    * @param y is the second tree node
    * @return true if index of x is less than y
    */
   bool operator()(const tree_nodeConstRef& x, const tree_nodeConstRef& y) const;
};

class TreeNodeConstSet : public OrderedSetStd<tree_nodeConstRef, TreeNodeConstSorter>
{
 public:
   /**
    * Constructor
    */
   TreeNodeConstSet();
};
#endif

/**
 * A set of tree node
 */
#if HAVE_UNORDERED
struct TreeNodeHash : public std::unary_function<tree_nodeRef, size_t>
{
   size_t operator()(tree_nodeRef tn) const
   {
      std::hash<unsigned int> hasher;
      return hasher(tn->index);
   }
};

class TreeNodeSet : public UnorderedSetStd<tree_nodeRef, TreeNodeHash, TreeNodeConstEqualTo>
{
};
#else
class TreeNodeSorter : std::binary_function<tree_nodeRef, tree_nodeRef, bool>
{
 public:
   /**
    * Constructor
    */
   TreeNodeSorter();

   /**
    * Compare position of two const tree nodes
    * @param x is the first tree node
    * @param y is the second tree node
    * @return true if index of x is less than y
    */
   bool operator()(const tree_nodeRef& x, const tree_nodeRef& y) const;
};

class TreeNodeSet : public OrderedSetStd<tree_nodeRef, TreeNodeSorter>
{
 public:
   /**
    * Constructor
    */
   TreeNodeSet();
};
#endif

/**
 * A map with key tree_nodeRef
 */
#if HAVE_UNORDERED
template <typename value>
class TreeNodeMap : public UnorderedMapStd<tree_nodeRef, value, TreeNodeHash, TreeNodeConstEqualTo>
{
};
#else
/// FIXME: add third template to custom map
template <typename value>
class TreeNodeMap : public OrderedMapStd<tree_nodeRef, value, TreeNodeSorter>
{
};
#endif

/**
 * Macro used to hide implementation details when accessing a tree_node from another tree_node
 * @param t is the tree_nodeRef to access
 * @return the pointer to t
 */
#ifndef NDEBUG
#define GET_NODE(t) (t ? (GetPointer<tree_reindex>(t) ? (GetPointer<tree_reindex>(t))->actual_tree_node : throw_error(t, #t, __PRETTY_FUNCTION__, __FILE__, __LINE__)) : throw_error(t, #t, __PRETTY_FUNCTION__, __FILE__, __LINE__))
#define GET_CONST_NODE(t) (t ? (GetPointer<const tree_reindex>(t) ? (GetPointer<const tree_reindex>(t))->actual_tree_node : throw_error(t, #t, __PRETTY_FUNCTION__, __FILE__, __LINE__)) : throw_error(t, #t, __PRETTY_FUNCTION__, __FILE__, __LINE__))
#else
#define GET_NODE(t) (GetPointer<tree_reindex>(t))->actual_tree_node
#define GET_CONST_NODE(t) (GetPointer<const tree_reindex>(t))->actual_tree_node
#endif

/**
 * Macro used to hide implementation details when accessing a tree_node from another tree_node
 * @param t is the tree_nodeRef to access
 * @return the index of t in tree_manager
 */
#define GET_INDEX_NODE(t) (GET_NODE(t))->index
#define GET_INDEX_CONST_NODE(t) (GET_CONST_NODE(t))->index

/**
 * This macro collects all case labels for unary_expr objects.
 * Its use it is quite simple: just add the following line in the switch statement
 * case CASE_UNARY_EXPRESSION:
 */
#define CASE_UNARY_EXPRESSION       \
   abs_expr_K:                      \
   case addr_expr_K:                \
   case arrow_expr_K:               \
   case bit_not_expr_K:             \
   case buffer_ref_K:               \
   case card_expr_K:                \
   case cleanup_point_expr_K:       \
   case conj_expr_K:                \
   case convert_expr_K:             \
   case exit_expr_K:                \
   case fix_ceil_expr_K:            \
   case fix_floor_expr_K:           \
   case fix_round_expr_K:           \
   case fix_trunc_expr_K:           \
   case float_expr_K:               \
   case imagpart_expr_K:            \
   case indirect_ref_K:             \
   case misaligned_indirect_ref_K:  \
   case loop_expr_K:                \
   case negate_expr_K:              \
   case non_lvalue_expr_K:          \
   case nop_expr_K:                 \
   case paren_expr_K:               \
   case realpart_expr_K:            \
   case reference_expr_K:           \
   case reinterpret_cast_expr_K:    \
   case sizeof_expr_K:              \
   case static_cast_expr_K:         \
   case throw_expr_K:               \
   case truth_not_expr_K:           \
   case unsave_expr_K:              \
   case va_arg_expr_K:              \
   case view_convert_expr_K:        \
   case reduc_max_expr_K:           \
   case reduc_min_expr_K:           \
   case reduc_plus_expr_K:          \
   case vec_unpack_hi_expr_K:       \
   case vec_unpack_lo_expr_K:       \
   case vec_unpack_float_hi_expr_K: \
   case vec_unpack_float_lo_expr_K

/**
 * This macro collects all case labels for unary_expr objects.
 * Its use it is quite simple: just add the following line in the switch statement
 * case CASE_UNARY_EXPRESSION:
 */
#define CASE_NON_ADDR_UNARY_EXPRESSION \
   abs_expr_K:                         \
   case arrow_expr_K:                  \
   case bit_not_expr_K:                \
   case buffer_ref_K:                  \
   case card_expr_K:                   \
   case cleanup_point_expr_K:          \
   case conj_expr_K:                   \
   case convert_expr_K:                \
   case exit_expr_K:                   \
   case fix_ceil_expr_K:               \
   case fix_floor_expr_K:              \
   case fix_round_expr_K:              \
   case fix_trunc_expr_K:              \
   case float_expr_K:                  \
   case imagpart_expr_K:               \
   case indirect_ref_K:                \
   case misaligned_indirect_ref_K:     \
   case loop_expr_K:                   \
   case negate_expr_K:                 \
   case non_lvalue_expr_K:             \
   case nop_expr_K:                    \
   case realpart_expr_K:               \
   case reference_expr_K:              \
   case reinterpret_cast_expr_K:       \
   case sizeof_expr_K:                 \
   case static_cast_expr_K:            \
   case throw_expr_K:                  \
   case truth_not_expr_K:              \
   case unsave_expr_K:                 \
   case va_arg_expr_K:                 \
   case view_convert_expr_K:           \
   case reduc_max_expr_K:              \
   case reduc_min_expr_K:              \
   case reduc_plus_expr_K:             \
   case vec_unpack_hi_expr_K:          \
   case vec_unpack_lo_expr_K:          \
   case vec_unpack_float_hi_expr_K:    \
   case vec_unpack_float_lo_expr_K

/**
 * This macro collects all case labels for binary_expr objects.
 */
#define CASE_BINARY_EXPRESSION     \
   assert_expr_K:                  \
   case bit_and_expr_K:            \
   case bit_ior_expr_K:            \
   case bit_xor_expr_K:            \
   case catch_expr_K:              \
   case ceil_div_expr_K:           \
   case ceil_mod_expr_K:           \
   case complex_expr_K:            \
   case compound_expr_K:           \
   case eh_filter_expr_K:          \
   case eq_expr_K:                 \
   case exact_div_expr_K:          \
   case fdesc_expr_K:              \
   case floor_div_expr_K:          \
   case floor_mod_expr_K:          \
   case ge_expr_K:                 \
   case gt_expr_K:                 \
   case goto_subroutine_K:         \
   case in_expr_K:                 \
   case init_expr_K:               \
   case le_expr_K:                 \
   case lrotate_expr_K:            \
   case lshift_expr_K:             \
   case lt_expr_K:                 \
   case max_expr_K:                \
   case mem_ref_K:                 \
   case min_expr_K:                \
   case minus_expr_K:              \
   case modify_expr_K:             \
   case mult_expr_K:               \
   case mult_highpart_expr_K:      \
   case ne_expr_K:                 \
   case ordered_expr_K:            \
   case plus_expr_K:               \
   case pointer_plus_expr_K:       \
   case postdecrement_expr_K:      \
   case postincrement_expr_K:      \
   case predecrement_expr_K:       \
   case preincrement_expr_K:       \
   case range_expr_K:              \
   case rdiv_expr_K:               \
   case round_div_expr_K:          \
   case round_mod_expr_K:          \
   case rrotate_expr_K:            \
   case rshift_expr_K:             \
   case set_le_expr_K:             \
   case trunc_div_expr_K:          \
   case trunc_mod_expr_K:          \
   case truth_and_expr_K:          \
   case truth_andif_expr_K:        \
   case truth_or_expr_K:           \
   case truth_orif_expr_K:         \
   case truth_xor_expr_K:          \
   case try_catch_expr_K:          \
   case try_finally_K:             \
   case uneq_expr_K:               \
   case ltgt_expr_K:               \
   case unge_expr_K:               \
   case ungt_expr_K:               \
   case unle_expr_K:               \
   case unlt_expr_K:               \
   case unordered_expr_K:          \
   case widen_sum_expr_K:          \
   case widen_mult_expr_K:         \
   case with_size_expr_K:          \
   case vec_lshift_expr_K:         \
   case vec_rshift_expr_K:         \
   case widen_mult_hi_expr_K:      \
   case widen_mult_lo_expr_K:      \
   case vec_pack_trunc_expr_K:     \
   case vec_pack_sat_expr_K:       \
   case vec_pack_fix_trunc_expr_K: \
   case vec_extracteven_expr_K:    \
   case vec_extractodd_expr_K:     \
   case vec_interleavehigh_expr_K: \
   case vec_interleavelow_expr_K:  \
   case extract_bit_expr_K

/**
 * This macro collects all case labels for ternary_expr objects.
 */
#define CASE_TERNARY_EXPRESSION \
   component_ref_K:             \
   case bit_field_ref_K:        \
   case bit_ior_concat_expr_K:  \
   case vtable_ref_K:           \
   case with_cleanup_expr_K:    \
   case obj_type_ref_K:         \
   case save_expr_K:            \
   case cond_expr_K:            \
   case vec_cond_expr_K:        \
   case vec_perm_expr_K:        \
   case dot_prod_expr_K:        \
   case ternary_plus_expr_K:    \
   case ternary_pm_expr_K:      \
   case ternary_mp_expr_K:      \
   case ternary_mm_expr_K

/**
 * This macro collects all case labels for quaternary_expr objects.
 */
#define CASE_QUATERNARY_EXPRESSION \
   array_range_ref_K:              \
   case array_ref_K

/**
 * This macro collects all case labels for type objects
 */
#define CASE_TYPE_NODES        \
   array_type_K:               \
   case boolean_type_K:        \
   case CharType_K:            \
   case nullptr_type_K:        \
   case type_pack_expansion_K: \
   case complex_type_K:        \
   case enumeral_type_K:       \
   case function_type_K:       \
   case integer_type_K:        \
   case lang_type_K:           \
   case method_type_K:         \
   case offset_type_K:         \
   case pointer_type_K:        \
   case qual_union_type_K:     \
   case real_type_K:           \
   case record_type_K:         \
   case reference_type_K:      \
   case set_type_K:            \
   case template_type_parm_K:  \
   case typename_type_K:       \
   case union_type_K:          \
   case vector_type_K:         \
   case void_type_K:           \
   case type_argument_pack_K

/**
 * This macro collects all case labels for pragma objects
 */
#define CASE_PRAGMA_NODES               \
   issue_pragma_K:                      \
   case blackbox_pragma_K:              \
   case profiling_pragma_K:             \
   case statistical_profiling_K:        \
   case map_pragma_K:                   \
   case call_hw_pragma_K:               \
   case call_point_hw_pragma_K:         \
   case omp_pragma_K:                   \
   case null_node_K:                    \
   case omp_atomic_pragma_K:            \
   case omp_critical_pragma_K:          \
   case omp_declare_simd_pragma_K:      \
   case omp_for_pragma_K:               \
   case omp_parallel_pragma_K:          \
   case omp_sections_pragma_K:          \
   case omp_parallel_sections_pragma_K: \
   case omp_section_pragma_K:           \
   case omp_simd_pragma_K:              \
   case omp_target_pragma_K:            \
   case omp_task_pragma_K

/**
 * This macro collects all case labels for fake or empty nodes
 */
#define CASE_FAKE_NODES     \
   last_tree_K:             \
   case none_K:             \
   case placeholder_expr_K: \
   case tree_reindex_K

/**
 * This macro collects all case labels for cpp nodes
 */
#define CASE_CPP_NODES           \
   baselink_K:                   \
   case ctor_initializer_K:      \
   case do_stmt_K:               \
   case expr_stmt_K:             \
   case if_stmt_K:               \
   case for_stmt_K:              \
   case handler_K:               \
   case modop_expr_K:            \
   case new_expr_K:              \
   case overload_K:              \
   case return_stmt_K:           \
   case scope_ref_K:             \
   case template_id_expr_K:      \
   case template_parm_index_K:   \
   case trait_expr_K:            \
   case try_block_K:             \
   case vec_new_expr_K:          \
   case while_stmt_K:            \
   case nontype_argument_pack_K: \
   case cast_expr_K:             \
   case expr_pack_expansion_K
/// NOTE that cast_expr is a unary expression but it could not be included in the CASE_UNARY_EXPRESSION because the operand could be null

/**
 * This macro collects all case labels for declaration nodes
 */
#define CASE_DECL_NODES          \
   const_decl_K:                 \
   case field_decl_K:            \
   case function_decl_K:         \
   case label_decl_K:            \
   case namespace_decl_K:        \
   case parm_decl_K:             \
   case result_decl_K:           \
   case translation_unit_decl_K: \
   case type_decl_K:             \
   case using_decl_K:            \
   case var_decl_K:              \
   case template_decl_K

/**
 * This macro collects all case labels for cast nodes
 */
#define CASE_CST_NODES \
   complex_cst_K:      \
   case integer_cst_K: \
   case real_cst_K:    \
   case string_cst_K:  \
   case vector_cst_K:  \
   case void_cst_K

/**
 * This macro collects all cases labels for gimple nodes
 */
#define CASE_GIMPLE_NODES      \
   gimple_asm_K:               \
   case gimple_assign_K:       \
   case gimple_bind_K:         \
   case gimple_call_K:         \
   case gimple_cond_K:         \
   case gimple_for_K:          \
   case gimple_goto_K:         \
   case gimple_label_K:        \
   case gimple_multi_way_if_K: \
   case gimple_nop_K:          \
   case gimple_phi_K:          \
   case gimple_pragma_K:       \
   case gimple_predict_K:      \
   case gimple_resx_K:         \
   case gimple_return_K:       \
   case gimple_switch_K:       \
   case gimple_while_K

/// macro to create simple tree classes
#define CREATE_TREE_NODE_CLASS(class_name, superclass)               \
   struct class_name : public superclass                             \
   {                                                                 \
      GET_KIND_TEXT(class_name)                                      \
      GET_KIND(class_name)                                           \
      virtual void visit(tree_node_visitor* const v) const override; \
      enum                                                           \
      {                                                              \
         GETID(superclass) = 0                                       \
      };                                                             \
      explicit class_name(unsigned int i) : superclass(i)            \
      {                                                              \
      }                                                              \
   }

/**
 * struct definition of common part of WeightedNode (gimple_assign, expr_node)
 */
struct WeightedNode : public tree_node
{
#if HAVE_CODE_ESTIMATION_BUILT
   WeightInformationRef weight_information;
#endif
   /// visitor enum
   enum
   {
      GETID(tree_node) = 0
   };

   /**
    * Constructor
    */
   explicit WeightedNode(unsigned int i);

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;
};

/**
 * struct definition of the field attr on function_decl, field_decl, var_decl tree node.
 * The tree walker structure of this field is:
 * (TOK_NEW | TOK_DELETE | TOK_ASSIGN | TOK_MEMBER | TOK_PUBLIC | TOK_PRIVATE | TOK_PROTECTED | TOK_NORETURN | TOK_VOLATILE | TOK_NOINLINE | TOK_ALWAYS_INLINE | TOK_USED |
 *  TOK_UNUSED | TOK_CONST | TOK_TRANSPARENT_UNION | TOK_CONSTRUCTOR | TOK_DESTRUCTOR |
 *  TOK_MODE | TOK_SECTION | TOK_ALIGNED | TOK_WEAK | TOK_ALIAS | TOK_NO_INSTRUMENT_FUNCTION | TOK_MALLOC | TOK_NO_STACK_LIMIT |
 *  TOK_PURE | TOK_DEPRECATED | TOK_VECTOR_SIZE | TOK_VISIBILITY | TOK_TLS_MODEL | TOK_NONNULL | TOK_NOTHROW | TOK_MAY_ALIAS |
 *  TOK_WARN_UNUSED_RES | TOK_FORMAT | TOK_FORMAT_ARG | TOK_NULL |
 *  TOK_GLOBAL_INIT | TOK_GLOBAL_FINI | TOK_CONVERSION | TOK_VIRTUAL | TOK_LSHIFT | TOK_PSEUDO | TOK_TEMPL | TOK_MUTABLE
 *  TOK_VECNEW | TOK_VECDELETE | TOK_POS | TOK_NEG | TOK_ADDR | TOK_DEREF | TOK_NOT | TOK_LNOT | TOK_PREINC | TOK_PREDEC | TOK_PLUSASSIGN | TOK_PLUS |
 *  TOK_MINUSASSIGN | TOK_MINUS | TOK_MULTASSIGN | TOK_MULT | TOK_DIVASSIGN | TOK_DIV | TOK_MODASSIGN | TOK_MOD | TOK_ANDASSIGN | TOK_AND |
 *  TOK_ORASSIGN |TOK_OR | TOK_XORASSIGN | TOK_XOR | TOK_LSHIFTASSIGN | TOK_RSHIFTASSIGN | TOK_RSHIFT | TOK_EQ | TOK_NE | TOK_LT | TOK_GT |
 *  TOK_LE | TOK_GE | TOK_LAND | TOK_LOR | TOK_COMPOUND | TOK_MEMREF | TOK_REF | TOK_SUBS | TOK_POSTINC | TOK_POSTDEC | TOK_CALL |
 *  TOK_THUNK | TOK_THIS_ADJUSTING | TOK_RESULT_ADJUSTING )*
 */
struct attr
{
   /// list of TOKEN, represented as int, associated to the tree_node
   CustomOrderedSet<TreeVocabularyTokenTypes_TokenEnum> list_attr;

   /**
    * Destructor
    */
   virtual ~attr();

   /**
    * Add an attribute to list of attribute.
    * @param a is the token number of the token attribute.
    */
   void add(const TreeVocabularyTokenTypes_TokenEnum a)
   {
      list_attr.insert(a);
   }

   /// Return true if there is TOK_CONSTRUCTOR in the list of attributes
   bool is_constructor();

   /// Return true if there is TOK_DESTRUCTOR in the list of attributes
   bool is_destructor();

   /// Return true if there is TOK_MEMBER in the list of attributes
   bool is_member();

   /// returns true if there is a TOK_CALL in the list of attributes
   bool is_call();

   /// returns true if there is a TOK_NEW in the list of attributes
   bool is_new();

   /// returns true if there is a TOK_PUBLIC in the list of attributes
   bool is_public();

   /// returns true if there is a TOK_PRIVATE in the list of attributes
   bool is_private();

   /// returns true if there is a TOK_PROTECTED in the list of attributes
   bool is_protected();

   /// returns true if there is a TOK_BITFIELD in the list of attributes
   bool is_bitfield();

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   virtual void visit(tree_node_visitor* const v) const;
};

/**
 * struct definition of the source position.
 * The tree walker structure of this node is:
 * #(TOK_SRCP TOK_STRING TOK_NUMBER TOK_NUMBER);
 */
struct srcp
{
   /**
    * include_name is a filename string,
    * this can be the location of a reference, if no definition has been seen.
    * (macro DECL_SOURCE_FILE)
    */
   std::string include_name;

   /**
    * line_number holds a line number.
    * (macro DECL_SOURCE_LINE)
    */
   unsigned int line_number;

   /**
    * column_number holds the column number.
    */
   unsigned int column_number;

   /**
    * constructor
    */
   srcp() : line_number(0), column_number(0)
   {
   }

   /**
    * Destructor
    */
   virtual ~srcp();

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   virtual void visit(tree_node_visitor* const v) const;
};

/**
 * struct definition of the declaration node structures.
 * struct definition of the field decla.
 * The tree walker structure of this field is:
 * (name)? (mngl)? (type)? (scpe)? (srcp)? (TOK_ARTIFICIAL)? (chan)?
 */
struct decl_node : public srcp, public tree_node
{
   /**
    * name field contains an identifier_node used to represent a name.
    * (tree-dump.c use the macro DECL_NAME)
    */
   tree_nodeRef name;

   /**
    * mngl field contains the name of the object as the assembler will see it.
    * Often this is the same as DECL_NAME
    * It is an identifier_node
    */
   tree_nodeRef mngl;

   /**
    * For any sort of a ..._DECL node, this points to the original (abstract)
    * decl node which this decl is an instance of, or else it is NULL indicating
    * that this decl is not an instance of some other decl.  For example,
    * in a nested declaration of an inline function, this points back to the
    * definition.
    */
   tree_nodeRef orig;

   /**
    * type field holds the data type of the object, when relevant.
    * label_decl have no data type.
    * for type_decl type field contents are the type whose name is being declared.
    * (tree-dump.c use the macro TREE_TYPE)
    */
   tree_nodeRef type;

   /// scope declaration
   tree_nodeRef scpe;

   tree_nodeRef attributes;

   /**
    * artificial_flag field is used to indicate that this decl_node represents a compiler-generated entity.
    * (tree-dump.c use the macro DECL_ARTIFICIAL)
    */
   bool artificial_flag;

   /// Indicates this field should be bit-packed
   bool packed_flag;

   /// operating system flag: it's true when this is a variable of operating system library
   bool operating_system_flag;

   /// library system flag: it's true when this is a variable of a standard library (e.g libmath)
   bool library_system_flag;

#if HAVE_BAMBU_BUILT
   /// it is true when this is a declared inside libbambu
   bool libbambu_flag;
#endif

   /**
    * chan field: the decls in one binding context are chained through this field.
    * (tree-dump.c use the macro TREE_CHAIN)
    */
   tree_nodeRef chan;

   /// is true when the declaration has the C attribute
   bool C_flag;

   /// The uid
   unsigned int uid;

   /**
    * Constructor
    */
   explicit decl_node(unsigned int i);

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(srcp) = 0,
      GETID(tree_node),
      GETID(name),
      GETID(mngl),
      GETID(orig),
      GETID(type),
      GETID(scpe),
      GETID(chan),
      GETID(attributes)
   };
};

/**
 * struct definition for PointToInformation
 * This struct contains information about what a pointer points to
 * Since pointer can also be field of a struct or element of an array,
 * this structure is associated also with var_decl nodes
 */
struct PointToInformation
{
   /// The string used in point_to_size map for scalar variables
   static const std::string default_key;

   /// The string used in point_to_size map for deferenced variable
   static const std::string deferenced_key;

   /// the bit size of the largest object to which the pointer points to
   CustomMap<std::string, size_t> point_to_size;
   CustomMap<std::string, size_t> symbolic_point_to_size;

   /**
    * Empty constructor
    */
   PointToInformation();

   /**
    * Destructor
    */
   ~PointToInformation();
};
typedef refcount<PointToInformation> PointToInformationRef;

/**
 * struct definition of the common part of an expression
 */
struct expr_node : public srcp, public WeightedNode
{
   /// constructor
   explicit expr_node(unsigned int i) : WeightedNode(i)
   {
   }

   /// type of the expression
   tree_nodeRef type;

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(srcp) = 0,
      GETID(WeightedNode),
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
   std::vector<tree_nodeRef> variables;

   /**
    * Constructor
    */
   PointToSolution();

   /**
    * Destructor
    */
   virtual ~PointToSolution();

   /**
    * Add a symbolic variable to this point to set
    * @param variable is the symbolic variable to be added
    */
   void Add(const std::string& variable);

   /**
    * Add a variable to this point to set
    * @param variable is the variable to be added
    */
   void Add(const tree_nodeRef& variable);

   /**
    * this function check if the point-to set is a singleton or not
    * @return true in case the point-to is a singleton, false otherwise
    */
   bool is_a_singleton() const;

   /**
    * this function check if the point-to set resolved w.r.t. standard variables
    * @return true in case the point-to point-to set resolved w.r.t. standard variables, false otherwise
    */
   bool is_fully_resolved() const;

   /**
    * Print this point-to solution
    * @return the string containing the point-to solution
    */
   std::string ToString() const;

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   virtual void visit(tree_node_visitor* const v) const;

   /// visitor enum
   enum
   {
      GETID(variables) = 0
   };
};
typedef refcount<PointToSolution> PointToSolutionRef;

/**
 * struct definition of the common part of a gimple  with virtual operands
 */
struct gimple_node : public srcp, public WeightedNode
{
   /**
    * Constructor
    * @param i is the index of the node
    */
   explicit gimple_node(unsigned int i);

   /// whole memory operand use
   tree_nodeRef memuse;

   /// whole memory operand def
   tree_nodeRef memdef;

   /// vuses of this statement
   TreeNodeSet vuses;

   /**
    * Add a vuse
    * @param vuse is the vuse
    */
   void AddVuse(const tree_nodeRef& vuse);

   /// vdef of this statement
   tree_nodeRef vdef;

   /**
    * Add a vdef
    * @param vdef is the vdef
    */
   void AddVdef(const tree_nodeRef& vdef);

   /// vovers of this statement
   TreeNodeSet vovers;

   /**
    * Add a vover
    * @param vover is the vover
    */
   void AddVover(const tree_nodeRef& vover);

   /// list of pragmas associated to the function
   std::vector<tree_nodeRef> pragmas;

   /// The point-to set used by this gimple node
   PointToSolutionRef use_set;

   /// The clobbered set of this gimple node
   PointToSolutionRef clobbered_set;

   /// The function to which this gimple_node belongs
   tree_nodeRef scpe;

   /// The basic block to which this gimple_node belongs
   unsigned int bb_index;

#if HAVE_BAMBU_BUILT || HAVE_TUCANO_BUILT
   /// The operation
   std::string operation;
#endif

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(srcp) = 0,
      GETID(WeightedNode),
      GETID(memuse),
      GETID(memdef),
      GETID(vuses),
      GETID(vdef),
      GETID(vovers),
      GETID(pragmas),
      GETID(use_set),
      GETID(clobbered_set),
      GETID(scpe)
   };

   /**
    * this field is true if the gimple_node was created artificially to handle
    * some specific situations, like for example handling functions returning
    * structs by value or accepting structs by value as parameters
    */
   bool artificial;

   /// when true CSE and Bit Value optimization will not remove from the IR
   bool keep;
};

/**
 * struct definition of the unary node structures.
 * The tree walker structure is for example:
 * node_expr: #(TOK_NODE_EXPR type op)
 */
struct unary_expr : public expr_node
{
   /// constructor
   explicit unary_expr(unsigned int i) : expr_node(i)
   {
   }

   /// op field is the operand of the unary expression
   tree_nodeRef op;

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(expr_node) = 0,
      GETID(op)
   };
};

/**
 * struct definition of the binary node structures.
 * The tree walker structure is for example:
 * node_expr: #(TOK_NODE_EXPR (type)? op op)
 */
struct binary_expr : public expr_node
{
   /// constructor
   explicit binary_expr(unsigned int i) : expr_node(i)
   {
   }

   /// The first operand of the binary expression
   tree_nodeRef op0;

   /// The second operand of the binary expression
   tree_nodeRef op1;

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

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
 * The tree walker structure is for example:
 * node_expr: #(TOK_NODE_EXPR type op op op)
 */
struct ternary_expr : public expr_node
{
   /// constructor
   explicit ternary_expr(unsigned int i) : expr_node(i)
   {
   }

   /// The first operand of the ternary expression
   tree_nodeRef op0;

   /// The second operand of the ternary expression
   tree_nodeRef op1;

   /// The third operand of the ternary expression
   tree_nodeRef op2;

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

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
 * struct definition of the quaternary node structures.
 * The tree walker structure is for example:
 * node_expr: #(TOK_NODE_EXPR type op op op op)
 */
struct quaternary_expr : public expr_node
{
   /// constructor
   explicit quaternary_expr(unsigned int i) : expr_node(i)
   {
   }

   /// The first operand of the quaternary expression
   tree_nodeRef op0;

   /// The second operand of the quaternary expression
   tree_nodeRef op1;

   /// The third operand of the quaternary expression
   tree_nodeRef op2;

   /// The fourth operand of the quaternary expression
   tree_nodeRef op3;

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(expr_node) = 0,
      GETID(op0),
      GETID(op1),
      GETID(op2),
      GETID(op3)
   };
};

/**
 * struct definition of the type node structures.
 * struct definition of the field type_node.
 * The tree walker structure of this field is:
 * (qual)? (name)? (unql)? (size)? (scpe)? (algn)?
 */
struct type_node : public tree_node
{
   /// constructor
   explicit type_node(unsigned int i);

   /**
    * qual is the set of type qualifiers for this type.
    * here is represented as token type: (TOK_QUAL_R | TOK_QUAL_V | TOK_QUAL_VR | TOK_QUAL_C | TOK_QUAL_CR | TOK_QUAL_CV | TOK_QUAL_CVR )
    * (macro: TYPE_QUAL_CONST, TYPE_QUAL_VOLATILE, TYPE_QUAL_RESTRICT)
    */
   TreeVocabularyTokenTypes_TokenEnum qual;

   /**
    * name field contains info on the name used in the program
    * for this type.It is either a TYPE_DECL node, for types that are typedefs, or an IDENTIFIER_NODE
    * in the case of structs, unions or enums that are known with a tag,
    * or zero for types that have no special name.
    * (macro TYPE_NAME)
    */
   tree_nodeRef name;

   /**
    * unql field, in any member of such a chain, points to the start of the chain.
    * (macro TYPE_MAIN_VARIANT)
    */
   tree_nodeRef unql;

   /**
    * size field contains a tree that is an expression for the size in bits.
    * (macro TYPE_SIZE)
    */
   tree_nodeRef size;

   /**
    * context/scope of the type object.
    * (macro TYPE_CONTEXT)
    */
   tree_nodeRef scpe;

   /**
    * algn field is the alignment necessary for objects of this type.
    * The value is an int, measured in bits.
    * (macro TYPE_ALIGN)
    */
   unsigned int algn;

   /// Indicated that objects of this type should be laid out in as compact a way as possible
   bool packed_flag;

   /// system flag: it's true when this is a system variable
   bool system_flag;

#if HAVE_BAMBU_BUILT
   /// it is true when this is a declared inside libbambu
   bool libbambu_flag;
#endif

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(tree_node) = 0,
      GETID(name),
      GETID(unql),
      GETID(size),
      GETID(scpe)
   };
};

/**
 * Memory tags used in tree-ssa to represent memory locations in virtual SSA
 */
struct memory_tag : public decl_node
{
   /// constructor
   explicit memory_tag(unsigned int i) : decl_node(i)
   {
   }

   /**
    * list of aliases associated with the memory tag.
    */
   std::vector<tree_nodeRef> list_of_aliases;

   /**
    * Add an alias to the list of aliases.
    * @param a is a NODE_ID.
    */
   void add_alias(const tree_nodeRef a)
   {
      list_of_aliases.push_back(a);
   }

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(decl_node) = 0,
      GETID(list_of_aliases)
   };
};

/**
 * This struct specifies super class for constant nodes
 */
struct cst_node : tree_node
{
   /// constructor
   explicit cst_node(unsigned int i) : tree_node(i)
   {
   }

   /// type field is the type of the node
   tree_nodeRef type;

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(tree_node) = 0,
      GETID(type)
   };
};

/**
 * Any erroneous construct is parsed into a node of this type.
 * This type of node is accepted without complaint in all contexts
 * by later parsing activities, to avoid multiple error messages
 * for one error.
 * No fields in these nodes are used except the TREE_CODE.
 */
struct error_mark : tree_node
{
   /// constructor
   explicit error_mark(unsigned int i) : tree_node(i)
   {
   }

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(error_mark)

   /// Redefinition of get_kind.
   GET_KIND(error_mark)
   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(tree_node) = 0
   };
};

/**
 * This struct specifies the abs_expr node.
 * Represents the absolute value of the operand.
 * An ABS_EXPR must have either an INTEGER_TYPE or a REAL_TYPE.  The operand of the ABS_EXPR must have the same type.
 */
CREATE_TREE_NODE_CLASS(abs_expr, unary_expr);

/**
 * This struct specifies the addr_expr node.
 * & in C.  Value is the address at which the operand's value resides.
 * Operand may have any mode.  Result mode is Pmode.
 */
CREATE_TREE_NODE_CLASS(addr_expr, unary_expr);

/**
 struct describing an array range. Likewise an array, except that the result is a range ("slice") of the array.  The
   starting index of the resulting array is taken from operand 1 and the size
   of the range is taken from the type of the expression.
 */
CREATE_TREE_NODE_CLASS(array_range_ref, quaternary_expr);

/** Array indexing.
   Operand 0 is the array; operand 1 is a (single) array index.
   Operand 2, if present, is a copy of TYPE_MIN_VALUE of the index.
   Operand 3, if present, is the element size, measured in units of
   the alignment of the element type.  */
CREATE_TREE_NODE_CLASS(array_ref, quaternary_expr);

/**
 * struct definition of the array_type tree node.
 * The tree walker structure of this node is:
 * #(TOK_ARRAY_TYPE type_node elts domn)
 */
struct array_type : public type_node
{
   /// constructor
   explicit array_type(unsigned int i) : type_node(i)
   {
   }

   /*type_node fields are in the parent class: type_node*/

   /// field elts is the type of an array element (tree-dump.c use the macro TREE_TYPE)
   tree_nodeRef elts;

   /// field domn is the type to index by (tree-dump.c use the macro TYPE_DOMAIN). Its range of values specifies the array length.
   tree_nodeRef domn;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(array_type)

   /// Redefinition of get_kind.
   GET_KIND(array_type)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(type_node) = 0,
      GETID(elts),
      GETID(domn)
   };
};

/**
 * This struct represent arrow_expr
 */
CREATE_TREE_NODE_CLASS(arrow_expr, unary_expr);

/**
 * This struct specifies the gimple_asm node.
 * Used to represent an inline assembly statement.  ASM_STRING returns a STRING_CST for the instruction (e.g., "mov x, y").
 * ASM_OUTPUTS, ASM_INPUTS, and ASM_CLOBBERS represent the outputs, inputs, and clobbers for the statement.
 * The tree walker structure of this node is:
 * #(TOK_GIMPLE_ASM type (TOK_VOLATILE)? str (out)? in (clob)?)
 */
struct gimple_asm : public gimple_node
{
   /// constructor
   explicit gimple_asm(unsigned int i) : gimple_node(i), volatile_flag(false)
   {
   }

   /// volatile_flag is true if the node is public: it means that the name is accessible from outside.
   bool volatile_flag;

   /// str is the operand 0: ASM_STRING. ASM_STRING returns a STRING_CST for the instruction (e.g., "mov x, y").
   std::string str;

   /// out is the operand 1: ASM_OUTPUTS, this represents the outputs for the statement.
   tree_nodeRef out;

   /// in is the operand 2: ASM_INPUTS, this represents the inputs for the statement.
   tree_nodeRef in;

   /// clob is the operand 3: ASM_CLOBBERS, this represents the clobbers for the statement.
   tree_nodeRef clob;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(gimple_asm)

   /// Redefinition of get_kind.
   GET_KIND(gimple_asm)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(gimple_node) = 0,
      GETID(out),
      GETID(in),
      GETID(clob)
   };
};

/**
 * This struct specifies the assert_expr node.
 */
CREATE_TREE_NODE_CLASS(assert_expr, binary_expr);

/**
 * This struct represents a reference to a member function or member
 * functions from a base class
 */
struct baselink : public tree_node
{
   /// constructor
   explicit baselink(unsigned int i) : tree_node(i)
   {
   }

   /// is the type of the baselink
   tree_nodeRef type;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(baselink)

   /// Redefinition of get_kind.
   GET_KIND(baselink)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(tree_node) = 0,
      GETID(type)
   };
};

/**
 * GIMPLE_BIND <VARS, BLOCK, BODY> represents a lexical scope.
 * VARS is the set of variables declared in that scope.
 * BLOCK is the symbol binding block used for debug information.
 * BODY is the sequence of statements in the scope.
 * #(TOK_GIMPLE_BIND (vars)? body)
 */
struct gimple_bind : public expr_node
{
   /// constructor
   explicit gimple_bind(unsigned int i) : expr_node(i)
   {
   }

   /// vars is the operand 0 (GIMPLE_BIND_VARS), this is a chain of VAR_DECL nodes for the variables.
   std::vector<tree_nodeRef> list_of_vars;

   /// body is the operand 1 (GIMPLE_BIND_BODY), this is the body, the expression to be computed using the variables. The value of operand 1 becomes that of the GIMPLE_BIND.
   tree_nodeRef body;

   /**
    * Add a var to list of vars.
    * @param a is a NODE_ID.
    */
   void add_vars(const tree_nodeRef& a)
   {
      list_of_vars.push_back(a);
   }

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(gimple_bind)

   /// Redefinition of get_kind.
   GET_KIND(gimple_bind)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(expr_node) = 0,
      GETID(list_of_vars),
      GETID(body)
   };
};

/**
 * This struct specifies the binfo node.
 * The tree walker structure of this node is:
 * #(TOK_BINFO (type)? (TOK_VIRT)? (baseinfo)*
 */
struct binfo : public tree_node
{
   /// constructor
   explicit binfo(unsigned int i) : tree_node(i), virt_flag(false), bases(0)
   {
   }

   /// type field is the actual data type node being inherited in this basetype.(BINFO_TYPE)
   tree_nodeRef type;

   /// virt_flag is true if the node is a virtual declaration (macro TREE_VIA_VIRTUAL)
   bool virt_flag;

   /// The number of basetypes for NODE
   int bases;

   /// is the list of pair access binf: baseinfo vector.
   std::vector<std::pair<TreeVocabularyTokenTypes_TokenEnum, tree_nodeRef>> list_of_access_binf;

   /**
    * return the i-th element of baseinfo
    * @param i is the index of baseinfo vector
    */
   tree_nodeRef get_base(size_t i) const
   {
      return list_of_access_binf[i].second;
   }

   /// return the size of baseinfo vector
   size_t get_baseinfo_size() const
   {
      return list_of_access_binf.size();
   }

   /**
    * Add a pair <access, binf> to the list of access binf.
    * @param binf is the binf.
    * @param access is a token between PROTECTED, PRIVATE and PUBLIC
    */
   void add_access_binf(const tree_nodeRef& binf, TreeVocabularyTokenTypes_TokenEnum access);

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(binfo)

   /// Redefinition of get_kind.
   GET_KIND(binfo)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(tree_node) = 0,
      GETID(type),
      GETID(list_of_access_binf)
   };
};

/**
 * This struct specifies the bit_and_expr node.
 * Bitwise operation.  Operands have same mode as result.
 */
CREATE_TREE_NODE_CLASS(bit_and_expr, binary_expr);

/**
 * This struct specifies the bit_field_ref node.
 * Reference to a group of bits within an object.  Similar to COMPONENT_REF except the position is given explicitly rather than via a FIELD_DECL.
 * Operand 0 is the structure or union expression;
 * operand 1 is a tree giving the number of bits being referenced;
 * operand 2 is a tree giving the position of the first referenced bit.
 * The field can be either a signed or unsigned field; BIT_FIELD_REF_UNSIGNED says which.
 */
CREATE_TREE_NODE_CLASS(bit_field_ref, ternary_expr);

/**
 * This struct specifies the bit_ior_expr node.
 * Bitwise operation.  Operands have same mode as result.
 */
CREATE_TREE_NODE_CLASS(bit_ior_expr, binary_expr);

/**
 * This struct specifies a concatenation between in1 and in2 using in3 bits.
 * Bitwise operation.  Operands have same mode as result.
 */
CREATE_TREE_NODE_CLASS(bit_ior_concat_expr, ternary_expr);

/**
 * This struct specifies the bit_not_expr node.
 * Bitwise operation.  Operands have same mode as result.
 */
CREATE_TREE_NODE_CLASS(bit_not_expr, unary_expr);

/**
 * This struct specifies the bit_xor_expr node.
 * Bitwise operation.  Operands have same mode as result.
 */
CREATE_TREE_NODE_CLASS(bit_xor_expr, binary_expr);

/**
 * This struct specifies the block node.
 * A symbol binding block.  These are arranged in a tree, where the BLOCK_SUBBLOCKS field contains a chain of subblocks chained through the BLOCK_CHAIN field.
 * The tree walker structure of this node is:
 * #(TOK_BLOCK)
 */
struct block : public tree_node
{
   /// constructor
   explicit block(unsigned int i) : tree_node(i), bl_flag(false)
   {
   }

   /// used to know if bl string is set
   bool bl_flag;

   /// May contain the string "block"
   std::string bl;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(block)

   /// Redefinition of get_kind.
   GET_KIND(block)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(tree_node) = 0
   };
};

/**
 * struct definition of the boolean_type tree node.
 * The tree walker structure of this node is:
 * #(TOK_BOOLEAN_TYPE type_node)
 */
CREATE_TREE_NODE_CLASS(boolean_type, type_node);

/**
 * This struct specifies the buffer_ref node.
 * Pascal `^` on a file.  One operand, an expression for the file.
 */
CREATE_TREE_NODE_CLASS(buffer_ref, unary_expr);

/**
 * This struct specifies the call_expr node.
 * Function call.  Operand 0 is the function.
 * Operand 1 is the argument list, a list of expressions made out of a chain of TREE_LIST nodes.
 * Operand 2 is the static chain argument, or NULL.
 * The tree walker structure of this node is:
 * #(TOK_CALL_EXPR type fn (args)?)
 */
struct call_expr : public expr_node
{
   /// constructor
   explicit call_expr(unsigned int i);

   /// fn is the operand 0 of the call expression: this is the function
   tree_nodeRef fn;

   /// The arguments of the call_expr
   std::vector<tree_nodeRef> args;

   /**
    * Add an argument to the list of arguments
    * @param arg is the argument to be added
    */
   void AddArg(const tree_nodeRef& arg);

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(call_expr)

   /// Redefinition of get_kind.
   GET_KIND(call_expr)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(expr_node) = 0,
      GETID(type),
      GETID(fn),
      GETID(args)
   };
};

/**
 * AGGR_INIT_EXPRs have a variably-sized representation similar to
 * that of CALL_EXPRs.  Operand 0 is an INTEGER_CST node containing the
 * operand count, operand 1 is the function which performs initialization,
 * operand 2 is the slot which was allocated for this expression, and
 * the remaining operands are the arguments to the initialization function.
 */
struct aggr_init_expr : public call_expr
{
   /// constructor
   explicit aggr_init_expr(unsigned int i);

   /// operand count
   int ctor;

   /// slot is the slot which was allocated for this expression
   tree_nodeRef slot;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(aggr_init_expr)

   /// Redefinition of get_kind.
   GET_KIND(aggr_init_expr)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(call_expr) = 0,
      GETID(ctor),
      GETID(slot)
   };
};

/**
 * This struct specifies the gimple_call node.
 * Function call.  Operand 0 is the function.
 * Operand 1 is the argument list, a list of expressions made out of a chain of TREE_LIST nodes.
 * Operand 2 is the static chain argument, or NULL.
 * The tree walker structure of this node is:
 * #(TOK_GIMPLE_CALL type fn (args)?)
 */
struct gimple_call : public gimple_node
{
   /// constructor
   explicit gimple_call(unsigned int i);

   /// fn is the operand 0 of the call expression: this is the function
   tree_nodeRef fn;

   /// The arguments of the gimple_call
   std::vector<tree_nodeRef> args;

   /**
    * Add an argument to the list of arguments
    * @param arg is the argument to be added
    */
   void AddArg(const tree_nodeRef& arg);

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(gimple_call)

   /// Redefinition of get_kind.
   GET_KIND(gimple_call)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(gimple_node) = 0,
      GETID(type),
      GETID(fn),
      GETID(args)
   };
};

/**
 * This struct specifies the card_expr node.
 * Operation for Pascal sets.
 */
CREATE_TREE_NODE_CLASS(card_expr, unary_expr);

/**
 * This struct specifies the case_label_expr node.
 * Used to represent a case label. The operands are CASE_LOW (operand 0) and CASE_HIGH (operand 1), respectively.
 * If CASE_LOW is NULL_TREE, the label is a 'default' label. If CASE_HIGH is NULL_TREE, the label is a normal case label.CASE_LABEL (operand 2) is the corresponding LABEL_DECL.
 * The tree walker structure of this node is:
 * #(TOK_CASE_LABEL_EXPR  type op (op)? (TOK_DEFAULT)? got);
 */
struct case_label_expr : public expr_node
{
   /// constructor
   explicit case_label_expr(unsigned int i) : expr_node(i), default_flag(false)
   {
   }

   /// op0 is the operand 0 (macro CASE_LOW) of the case label expression
   tree_nodeRef op0;

   /// op1 is the operand 1 (macro CASE_HIGH) of the case label expression
   tree_nodeRef op1;

   /// default_flag is true if the label is a 'default' label
   bool default_flag;

   /// got field is the operand 2 (macro CASE_LABEL) of the case label expression
   tree_nodeRef got;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(case_label_expr)

   /// Redefinition of get_kind.
   GET_KIND(case_label_expr)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(expr_node) = 0,
      GETID(op0),
      GETID(op1),
      GETID(got)
   };
};

/**
 * This struct specifies the cast_expr node.
 * The walker for this struct is
 *   #(TOK_CAST_EXPR wunary_expr);
 */
struct cast_expr : public expr_node
{
   /// constructor
   explicit cast_expr(unsigned int i) : expr_node(i)
   {
   }

   /// op is casted node
   tree_nodeRef op;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(cast_expr)

   /// Redefinition of get_kind.
   GET_KIND(cast_expr)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(expr_node) = 0,
      GETID(op)
   };
};

/**
 * This struct specifies the catch_expr node.
 * Evaluate operand 1.  If and only if an exception is thrown during the evaluation of operand 1, evaluate operand 2.
 * This differs from TRY_FINALLY_EXPR in that operand 2 is not evaluated on a normal or jump exit, only on an exception.
 */
CREATE_TREE_NODE_CLASS(catch_expr, binary_expr);

/**
 * This struct specifies the ceil_div_expr node.
 * Division for integer result that rounds the quotient toward infinity.
 */
CREATE_TREE_NODE_CLASS(ceil_div_expr, binary_expr);

/**
 * This struct specifies the ceil_mod_expr node.
 * Kind of remainder that go with the kind of division.
 */
CREATE_TREE_NODE_CLASS(ceil_mod_expr, binary_expr);

/**
 * struct definition of the CharType tree node.
 * The tree walker structure of this node is:
 * #(TOK_CHAR_TYPE type_node)
 */
CREATE_TREE_NODE_CLASS(CharType, type_node);

/**
 * The C++ decltype(nullptr) type.
 */
CREATE_TREE_NODE_CLASS(nullptr_type, type_node);

/**
 * Represents a type expression that will be expanded into a list of
 * types when instantiated with one or more argument packs.
 *
 * PACK_EXPANSION_PATTERN retrieves the expansion pattern. This is
 * the type or expression that we will substitute into with each
 * argument in an argument pack.
 *
 * SET_PACK_EXPANSION_PATTERN sets the expansion pattern.
 *
 * PACK_EXPANSION_PARAMETER_PACKS contains a TREE_LIST of the parameter
 * packs that are used in this pack expansion.
 *
 * Example:
 *   template<typename... Values>
 *   struct tied : tuple<Values&...> {
 *     // ...
 *   };
 *
 * The derivation from tuple contains a TYPE_PACK_EXPANSION for the
 * template arguments. Its PACK_EXPANSION_PATTERN is "Values&" and its
 * PACK_EXPANSION_PARAMETER_PACKS will contain "Values".
 */
struct type_pack_expansion : public type_node
{
   /// constructor
   explicit type_pack_expansion(unsigned int i) : type_node(i)
   {
   }

   /// PACK_EXPANSION_PATTERN
   tree_nodeRef op;

   /// PACK_EXPANSION_PARAMETER_PACKS
   tree_nodeRef param_packs;

   /// PACK_EXPANSION_EXTRA_ARGS
   tree_nodeRef arg;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(type_pack_expansion)

   /// Redefinition of get_kind.
   GET_KIND(type_pack_expansion)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(type_node) = 0,
      GETID(op),
      GETID(param_packs),
      GETID(arg)
   };
};

/**
 * Represents an expression that will be expanded into a list of
 * expressions when instantiated with one or more argument packs.
 *
 * EXPR_PACK_EXPANSION plays precisely the same role as TYPE_PACK_EXPANSION,
 * but will be used for expressions.
 */
struct expr_pack_expansion : public expr_node
{
   /// constructor
   explicit expr_pack_expansion(unsigned int i) : expr_node(i)
   {
   }

   /// PACK_EXPANSION_PATTERN
   tree_nodeRef op;

   /// PACK_EXPANSION_PARAMETER_PACKS
   tree_nodeRef param_packs;

   /// PACK_EXPANSION_EXTRA_ARGS
   tree_nodeRef arg;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(expr_pack_expansion)

   /// Redefinition of get_kind.
   GET_KIND(expr_pack_expansion)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(expr_node) = 0,
      GETID(op),
      GETID(param_packs),
      GETID(arg)
   };
};

/**
 * This struct specifies the cleanup_point_expr node.
 * As a consequence, the operand of a CLEANUP_POINT_EXPR must not have
 * BLKmode, because it will not be forced out of memory.
 */
CREATE_TREE_NODE_CLASS(cleanup_point_expr, unary_expr);

/**
 * This struct specifies the complex_cst node.
 * Contents are in TREE_REALPART and TREE_IMAGPART fields, whose contents are other constant nodes.
 * The tree walker structure of this node is:
 * #(TOK_COMPLEX_CST type real imag)
 */
struct complex_cst : public cst_node
{
   /// constructor
   explicit complex_cst(unsigned int i) : cst_node(i)
   {
   }

   /// real is the TREE_REALPART which content is other constant node.
   tree_nodeRef real;

   /// imag is the TREE_IMAGPART which content is other constant node.
   tree_nodeRef imag;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(complex_cst)

   /// Redefinition of get_kind.
   GET_KIND(complex_cst)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(cst_node) = 0,
      GETID(real),
      GETID(imag)
   };
};

/**
 * This struct specifies the complex_expr node.
 * Given two real or integer operands of the same type, returns a complex value of the corresponding complex type.
 */
CREATE_TREE_NODE_CLASS(complex_expr, binary_expr);

/**
 * struct definition of the complex_type tree node.
 * The tree walker structure of this node is:
 * #(TOK_COMPLEX_TYPE type_node);
 */
struct complex_type : public type_node
{
   /// constructor
   explicit complex_type(unsigned int i) : type_node(i), unsigned_flag(false), real_flag(false)
   {
   }

   /**
    * unsigned means an unsigned type
    * (macro TYPE_UNSIGNED)
    */
   bool unsigned_flag;

   /// true when the complex base type is a float (macro COMPLEX_FLOAT_TYPE_P)
   bool real_flag;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(complex_type)

   /// Redefinition of get_kind.
   GET_KIND(complex_type)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(type_node) = 0
   };
};

/** Value is structure or union component.
   Operand 0 is the structure or union (an expression).
   Operand 1 is the field (a node of type FIELD_DECL).
   Operand 2, if present, is the value of DECL_FIELD_OFFSET, measured
   in units of DECL_OFFSET_ALIGN / BITS_PER_UNIT.  */
CREATE_TREE_NODE_CLASS(component_ref, ternary_expr);

/**
 * This struct specifies the compound_expr node.
 * Contains two expressions to compute, one followed by the other. The first value is ignored.  The second one's value is used.
 * The type of the first expression need not agree with the other types.
 */
CREATE_TREE_NODE_CLASS(compound_expr, binary_expr);

/**
 * This struct specifies the cond_expr node.
 * Conditional expression ( ... ? ... : ...  in C).
 * Operand 0 is the condition. Operand 1 is the then-value. Operand 2 is the else-value.
 * Operand 0 may be of any type. Operand 1 must have the same type as the entire expression, unless
 * it unconditionally throws an exception, in which case it should have VOID_TYPE.  The same constraints apply to operand 2.
 */
CREATE_TREE_NODE_CLASS(cond_expr, ternary_expr);

/**
 * This struct specifies the gimple_cond node.
 * Operand 0 is the condition.
 */
struct gimple_cond : public gimple_node
{
   /// constructor
   explicit gimple_cond(unsigned int i) : gimple_node(i)
   {
   }

   /// The first operand of the ternary expression
   tree_nodeRef op0;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(gimple_cond)

   /// Redefinition of get_kind.
   GET_KIND(gimple_cond)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(gimple_node) = 0,
      GETID(op0)
   };
};

/**
 * This struct specifies the conj_expr node.
 * Complex conjugate of operand.  Used only on complex types.
 */
CREATE_TREE_NODE_CLASS(conj_expr, unary_expr);

/**
 * struct definition of the const_decl tree node.
 * The tree walker structure of this node is:
 * #(TOK_CONST_DECL decl_node (cnst)?)
 */
struct const_decl : public decl_node
{
   /// constructor
   explicit const_decl(unsigned int i) : decl_node(i)
   {
   }

   /*decl_node fields are in the parent class*/

   /// field cnst holds the value of a constant (tree-dump.c use the macro DECL_INITIAL)
   tree_nodeRef cnst;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(const_decl)

   /// Redefinition of get_kind.
   GET_KIND(const_decl)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(decl_node) = 0,
      GETID(cnst)
   };
};

/**
 * Constructor: return an aggregate value made from specified components.
   In C, this is used only for structure and array initializers.
   The operand is a sequence of component values made out of a VEC of
   struct constructor_elt.

   For ARRAY_TYPE:
   The field INDEX of each constructor_elt is the corresponding index.
   If the index is a RANGE_EXPR, it is a short-hand for many nodes,
   one for each index in the range.  (If the corresponding field VALUE
   has side-effects, they are evaluated once for each element.  Wrap the
   value in a SAVE_EXPR if you want to evaluate side effects only once.)

   For RECORD_TYPE, UNION_TYPE, or QUAL_UNION_TYPE:
   The field INDEX of each node is a FIELD_DECL.
*/
struct constructor : public tree_node
{
   /// constructor
   explicit constructor(unsigned int i) : tree_node(i)
   {
   }

   /// type field is the type of the node
   tree_nodeRef type;

   /// store the list initializers: <index, value>
   std::vector<std::pair<tree_nodeRef, tree_nodeRef>> list_of_idx_valu;

   /**
    * Add a pair <index value> to the list of idx_val.
    * @param idx is the index.
    * @param val is the value.
    */
   void add_idx_valu(const tree_nodeRef& idx, const tree_nodeRef& valu)
   {
      list_of_idx_valu.push_back(std::pair<tree_nodeRef, tree_nodeRef>(idx, valu));
   }

   /**
    * Add a pair <null, value> to the list of idx_val.
    * @param val is the value.
    */
   void add_valu(const tree_nodeRef& valu)
   {
      list_of_idx_valu.push_back(std::pair<tree_nodeRef, tree_nodeRef>(tree_nodeRef(), valu));
   }

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(constructor)

   /// Redefinition of get_kind.
   GET_KIND(constructor)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(tree_node) = 0,
      GETID(type),
      GETID(list_of_idx_valu)
   };
};

/**
 * This struct specifies the convert_expr node.
 * Represents a conversion of type of a value.
 * All conversions, including implicit ones, must be represented by CONVERT_EXPR or NOP_EXPR nodes.
 */
CREATE_TREE_NODE_CLASS(convert_expr, unary_expr);

/**
 * CTOR_INITIALIZER is a placeholder in template code for a call to
 * setup_vtbl_pointer (and appears in all functions, not just ctors)
 */
CREATE_TREE_NODE_CLASS(ctor_initializer, tree_node);

/**
 * This struct specifies the eh_filter_expr node.
 * Used to represent an exception specification.
 * EH_FILTER_TYPES is a list of allowed types, and EH_FILTER_FAILURE is an expression to evaluate on failure.
 * EH_FILTER_MUST_NOT_THROW controls which range type to use when expanding.
 */
CREATE_TREE_NODE_CLASS(eh_filter_expr, binary_expr);

/**
 * struct definition of the integer_type tree node.
 * The tree walker structure of this node is:
 * #(TOK_ENUMERAL_TYPE type_node prec (TOK_UNSIGNED)? min max (csts)?);
 */
struct enumeral_type : public type_node
{
   /// constructor
   explicit enumeral_type(unsigned int i) : type_node(i), prec(0), unsigned_flag(false)
   {
   }

   /*type_node fields are in the parent class*/

   /**
    * prec field is the number of bits used by this type
    * (tree-dump.c use the macro TYPE_PRECISION)
    */
   unsigned int prec;

   /**
    * unsigned means an unsigned type
    * (tree-dump.c use the macro TYPE_UNSIGNED)
    */
   bool unsigned_flag;

   /// min: tree-dump.c use the macro TYPE_MIN_VALUE
   tree_nodeRef min;

   /// max: tree-dump.c use the macro TYPE_MAX_VALUE
   tree_nodeRef max;

   /**
    * csts is a list in which each element's TREE_PURPOSE is a name
    * and the TREE_VALUE is the value (an integer_cst node)
    * (tree-dump.c use the macro TYPE_VALUES)
    */
   tree_nodeRef csts;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(enumeral_type)

   /// Redefinition of get_kind.
   GET_KIND(enumeral_type)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(type_node) = 0,
      GETID(min),
      GETID(max),
      GETID(csts)
   };
};

/**
 * This struct specifies the eq_expr node.
 * Relational operators. EQ_EXPR is allowed for any types.
 * The operands will have the same type, and the value is always the type used by the language for booleans.
 */
CREATE_TREE_NODE_CLASS(eq_expr, binary_expr);

/**
 * This struct specifies the exact_div_expr node.
 * Division which is not supposed to need rounding. Used for pointer subtraction in C.
 */
CREATE_TREE_NODE_CLASS(exact_div_expr, binary_expr);

/**
 * This struct specifies the exit_expr node.
 * Exit the inner most loop conditionally.  The operand is the condition.
 * The type should be void and the value should be ignored.
 */
CREATE_TREE_NODE_CLASS(exit_expr, unary_expr);

/**
 * This struct represent a statement expression
 */
struct expr_stmt : public tree_node
{
   /// constructor
   explicit expr_stmt(unsigned int i) : tree_node(i), line(-1)
   {
   }

   /// line is the line number where the compound_stmt is defined.
   int line;

   /// Is the statement given by the expression
   tree_nodeRef expr;

   /// Is the next statement
   tree_nodeRef next;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(expr_stmt)

   /// Redefinition of get_kind.
   GET_KIND(expr_stmt)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(tree_node) = 0,
      GETID(expr),
      GETID(next)
   };
};

/**
 * This struct specifies the fdesc_expr node.
 * Operand 0 is a function constant; result is part N of a function descriptor of type ptr_mode.
 */
CREATE_TREE_NODE_CLASS(fdesc_expr, binary_expr);

/**
 * struct definition of the field_decl tree node.
 * The tree walker structure of this node is:
 * #(TOK_FIELD_DECL decl_node attr (init)? (size)? algn (bpos)?)
 */
struct field_decl : public decl_node, public attr
{
   /// constructor
   explicit field_decl(unsigned int i) : decl_node(i), algn(0)
   {
   }

   /*decl_node fields are in the parent class: decl_node*/
   /*attr fields are in the parent class: attr*/

   /**
    * init field holds the value to initialize a variable to.
    * (tree-dump.c use the macro DECL_INITIAL)
    */
   tree_nodeRef init;

   /**
    * size field holds the size of datum, in bits.
    * (tree-dump.c use the macro DECL_SIZE)
    */
   tree_nodeRef size;

   /**
    * algn field holds the alignment required for the datum, in bits.
    * (tree-dump.c use the macro DECL_ALIGN)
    */
   unsigned int algn;

   /**
    * bpos field is the field position, counting in bytes, of the byte
    * containing the bit closest to the beginning of the structure
    * (tree-dump.c use the macro DECL_FIELD_OFFSET
    * and the function bit_position)
    */
   tree_nodeRef bpos;

   /**
    * symbol_memory_tag annotation
    */
   tree_nodeRef smt_ann;

   /**
    + Compute the offset of this field in the struct or structure
    * @return the offset for this field
   */
   long long int offset();

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(field_decl)

   /// Redefinition of get_kind.
   GET_KIND(field_decl)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(decl_node) = 0,
      GETID(attr),
      GETID(init),
      GETID(size),
      GETID(bpos),
      GETID(smt_ann)
   };
};

/**
 * This struct specifies the fix_ceil_expr node.
 * Conversion of real to fixed point.
 */
CREATE_TREE_NODE_CLASS(fix_ceil_expr, unary_expr);

/**
 * This struct specifies the fix_floor_expr node.
 * Conversion of real to fixed point
 */
CREATE_TREE_NODE_CLASS(fix_floor_expr, unary_expr);

/**
 * This struct specifies the fix_round_expr node.
 * Conversion of real to fixed point
 */
CREATE_TREE_NODE_CLASS(fix_round_expr, unary_expr);

/**
 * This struct specifies the fix_trunc_expr node.
 * Conversion of real to fixed point.
 */
CREATE_TREE_NODE_CLASS(fix_trunc_expr, unary_expr);

/**
 * This struct specifies the float_expr node.
 * Conversion of an integer to a real.
 */
CREATE_TREE_NODE_CLASS(float_expr, unary_expr);

/**
 * This struct specifies the floor_div_expr node.
 * Division for integer result that rounds toward minus infinity.
 */
CREATE_TREE_NODE_CLASS(floor_div_expr, binary_expr);

/**
 * This struct specifies the floor_mod_expr node.
 * Kind of remainder that go with the kind of division.
 */
CREATE_TREE_NODE_CLASS(floor_mod_expr, binary_expr);

/**
 * struct definition of the function_decl tree node.
 * The tree walker structure of this node is:
 * #(TOK_FUNCTION_DECL decl_node (TOK_C| TOK_OPERATOR (TOK_STRING)*)? attr (args)? (TOK_UNDEFINED)? (TOK_EXTERN | TOK_STATIC) (body)?)
 */
struct function_decl : public decl_node, public attr
{
   /// constructor
   explicit function_decl(unsigned int i);

   /// it is true when the function_decl is an operator
   bool operator_flag;

   /// store the list of operator names
   std::vector<std::string> list_of_op_names;

   /// flags used to know if fixd is set
   bool fixd_flag;

   /// flags used to know if  virt is set
   bool virt_flag;

   /// True if parameters are not alias after its invocation
   bool reverse_restrict_flag;

   /// True if function write in memory somehow
   bool writing_memory;

   /// True if function read from memory somehow
   bool reading_memory;

   /// True if pipelining is enabled for the function
   bool pipeline_enabled;

   /// True if the pipeline does not contain any unbounded operation
   bool simple_pipeline;

   /// Used for pipelined with unbounded operations
   int initiation_time;

#if HAVE_FROM_PRAGMA_BUILT
   /// If different from zero, the parallel degree of the contained openmp loop
   size_t omp_for_wrapper;

   /// True if function contains only the body of an openmp for
   bool omp_body_loop;

   /// If not empty, the name of the critical session contained in the function
   std::string omp_critical;

   /// True if function corresponds to an omp atomic
   bool omp_atomic;
#endif

   /// Is the result of THUNK_FIXED_OFFSET(t) for this tree node
   int fixd;

   /// Is the result of tree_low_cst (THUNK_VIRTUAL_OFFSET (t), 0) for this node
   int virt;

   /// fn field is the initial declaration for this function declaration
   tree_nodeRef fn;

   /// for each bit of the ssa variable tells if it is equal to U,X,0,1
   std::string bit_values;

   /// Range information about bounds of the function return value (valid for real_type too)
   RangeRef range;

   /**
    * tmpl_parms holds template parameters
    * It is a TREE_LIST, his VALU field is a TREE_VEC whose LIST_OF_OP holds template parameters.
    * The instantion of parameter "list_of_op[i]" is "list_of_op[i]" hold in tmpl_args.
    */
   tree_nodeRef tmpl_parms;

   /**
    * tmpl_args holds template instantiations
    * It is a TREE_VEC whose LIST_OF_OP holds template instantiations.
    * The parameter of  instantiation "list_of_op[i]" is "list_of_op[i]" hold in tmpl_parms.
    */
   tree_nodeRef tmpl_args;

   /**
    * args field holds a chain of parm_decl nodes for the arguments.
    * (tree-dump.c use the macro DECL_ARGUMENTS)
    */
   std::vector<tree_nodeRef> list_of_args;

   /**
    * undefined_flag means external reference:
    * do not allocate storage, and refer to a definition elsewhere.
    * (tree-dump.c use the macro DECL_EXTERNAL)
    */
   bool undefined_flag;

   /**
    * flag true when the function is a builtin
    */
   bool builtin_flag;

   /**
    * flag true when the function is marked as hwcall
    */
   bool hwcall_flag;

   /**
    * static_flag is true if function has been defined
    * (macro TREE_STATIC)
    */
   bool static_flag;

   /**
    * body field is the saved representation of the body of the entire function.
    * (macro DECL_SAVED_TREE)
    */
   tree_nodeRef body;

   /**
    * java inline body
    */
   tree_nodeRef inline_body;

   /**
    * Add a string to list of attribute.
    * @param a is the token number of the token attribute.
    */
   void add(std::string a)
   {
      list_of_op_names.push_back(a);
   }

   /**
    * Add a parameter to the list of the paramters.
    * @param a is the parameter to be added.
    */
   void AddArg(const tree_nodeRef& a);

   /// return true if is a declaration of constructor
   bool is_constructor();

   /// Return true if is a declaration of destructor
   bool is_destructor();

   /// returns true if is a declaration of an operator
   bool is_operator() const;

   /// returns true if is a declaration of a public function
   bool is_public();

   /// returns true if is a declaration of a private function
   bool is_private();

   /// returns true if is a declaration of a protected function
   bool is_protected();

   /// returns true if is a declaration of a pipelined function
   bool is_pipelined();

   void set_pipelining(bool v);

   bool is_simple_pipeline();

   void set_simple_pipeline(bool v);

   int get_initiation_time();

   void set_initiation_time(int time);

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(function_decl)

   /// Redefinition of get_kind.
   GET_KIND(function_decl)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(decl_node) = 0,
      GETID(attr),
      GETID(fn),
      GETID(tmpl_parms),
      GETID(tmpl_args),
      GETID(list_of_args),
      GETID(body),
      GETID(inline_body)
   };
};

/**
 * struct definition of the function_type tree node.
 * Type of functions.  Special fields:
 * TREE_TYPE       type of value returned.
 * TYPE_ARG_TYPES  list of types of arguments expected.
 *                 this list is made of TREE_LIST nodes.
 * Types of "Procedures" in languages where they are different from functions
 * have code FUNCTION_TYPE also, but then TREE_TYPE is zero or void type.
 * The tree walker structure of this node is:
 * #(TOK_FUNCTION_TYPE type_node retn (prms)?)
 */
struct function_type : public type_node
{
   /// constructor
   explicit function_type(unsigned int i) : type_node(i), varargs_flag(false)
   {
   }

   /// varargs flag: tells if function is of varargs type
   bool varargs_flag;

   /**
    * retn field is the type of value returned.
    * (macro TREE_TYPE)
    */
   tree_nodeRef retn;

   /**
    * prms field is a list of types of arguments expected,
    * this list is made of tree_list nodes.
    * (macro TYPE_ARG_TYPES)
    */
   tree_nodeRef prms;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(function_type)

   /// Redefinition of get_kind.
   GET_KIND(function_type)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(type_node) = 0,
      GETID(retn),
      GETID(prms)
   };
};

/**
 * This struct specifies the ge_expr node.
 * Relational operator. GE_EXPR is allowed only for integer (or pointer or enumerate) or real types.
 * The operands will have the same type, and the value is always the type used by the language for booleans.
 */
CREATE_TREE_NODE_CLASS(ge_expr, binary_expr);

/**
 * This struct specifies the gimple_assign node (gcc 4.3 tree node).
 * Assignment expression. The first operand is the what to set; the second, the new value.
 */
struct gimple_assign : public gimple_node
{
   /// constructor
   explicit gimple_assign(unsigned int i);

   /// The first operand of the binary expression
   tree_nodeRef op0;

   /// The second operand of the binary expression
   tree_nodeRef op1;

   /// The predicate
   tree_nodeRef predicate;

   /// in case the statement comes from a phi node split points to the original gimple_phi (PandA extension)
   tree_nodeRef orig;

   bool init_assignment;

   bool clobber;

   bool temporary_address;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(gimple_assign)

   /// Redefinition of get_kind.
   GET_KIND(gimple_assign)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(gimple_node) = 0,
      GETID(op0),
      GETID(op1),
      GETID(orig),
      GETID(predicate)
   };
};

struct gimple_nop : public gimple_node
{
   /// constructor
   explicit gimple_nop(unsigned int i) : gimple_node(i)
   {
   }

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(gimple_nop)

   /// Redefinition of get_kind.
   GET_KIND(gimple_nop)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(gimple_node) = 0
   };
};

/**
 * This struct specifies the gimple_goto node.
 * The operand is a LABEL_DECL node or an expression.
 * The type should be void and the value should be ignored.
 */
struct gimple_goto : public gimple_node
{
   /// constructor
   explicit gimple_goto(unsigned int i) : gimple_node(i)
   {
   }

   /// the label
   tree_nodeRef op;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(gimple_goto)

   /// Redefinition of get_kind.
   GET_KIND(gimple_goto)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(gimple_node) = 0,
      GETID(op)
   };
};

/**
 * This struct specifies the goto_subroutine node.
 * Used internally for cleanups in the implementation of TRY_FINALLY_EXPR.
 * (Specifically, it is created by expand_expr, not front-ends.)
 * Operand 0 is the rtx for the start of the subroutine we need to call.
 * Operand 1 is the rtx for a variable in which to store the address of where the subroutine should return to.
 */
CREATE_TREE_NODE_CLASS(goto_subroutine, binary_expr);

/**
 * This struct specifies the gt_expr node.
 * Relational operator. GT_EXPR is allowed only for integer (or pointer or enumerate) or real types.
 * The operands will have the same type, and the value is always the type used by the language for booleans.
 */
CREATE_TREE_NODE_CLASS(gt_expr, binary_expr);

/**
 * A HANDLER wraps a catch handler for the HANDLER_TYPE.  If this is
 * CATCH_ALL_TYPE, then the handler catches all types.  The declaration of
 * the catch variable is in HANDLER_PARMS, and the body block in
 *  HANDLER_BODY.
 */
struct handler : public tree_node
{
   /// constructor
   explicit handler(unsigned int i) : tree_node(i), line(-1)
   {
   }

   /// line is the line number where the compound_stmt is defined.
   int line;

   /// is the body of the handler
   tree_nodeRef body;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(handler)

   /// Redefinition of get_kind.
   GET_KIND(handler)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(tree_node) = 0,
      GETID(body)
   };
};

/**
 * struct definition of the function_decl tree node.
 * The tree walker structure of this node is:
 * #(TOK_IDENTIFIER_NODE ((strg lngt) | TOK_OPERATOR));
 */
struct identifier_node : public tree_node
{
   /// constructors
   identifier_node(unsigned int node_id, std::string _strg, tree_manager* TM);
   identifier_node(unsigned int node_id, bool _operator_flag, tree_manager* TM);

   /// Store true if the identifier_node is an operator.
   const bool operator_flag;

   /// Store the identifier string associated with the identifier_node.
   std::string strg;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(identifier_node)

   /// Redefinition of get_kind.
   GET_KIND(identifier_node)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(tree_node) = 0
   };
};

/**
 * This struct specifies the imagpart_expr node.
 * Used only on an operand of complex type, this return a value of the corresponding component type.
 */
CREATE_TREE_NODE_CLASS(imagpart_expr, unary_expr);

/**
 * This struct specifies the indirect_ref node.
 * C unary `*' or Pascal `^'.  One operand, an expression for a pointer.
 */
CREATE_TREE_NODE_CLASS(indirect_ref, unary_expr);

/**
 * This struct specifies the misaligned version of the indirect_ref node.
 */
CREATE_TREE_NODE_CLASS(misaligned_indirect_ref, unary_expr);

/**
 * This struct specifies the in_expr node.
 * Operation for Pascal sets.
 */
CREATE_TREE_NODE_CLASS(in_expr, binary_expr);

/**
 * This struct specifies the init_expr node.
 * Initialization expression.  Operand 0 is the variable to initialize; Operand 1 is the initializer.
 */
CREATE_TREE_NODE_CLASS(init_expr, binary_expr);

/**
 * This struct specifies the integer_cst node.
 */
struct integer_cst : public cst_node
{
   /// constructor
   explicit integer_cst(unsigned int i) : cst_node(i), value(0)
   {
   }

   /// The value of the integer cast
   long long int value;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(integer_cst)

   /// Redefinition of get_kind.
   GET_KIND(integer_cst)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(cst_node) = 0
   };
};

/**
 * struct definition of the integer_type tree node.
 * The tree walker structure of this node is:
 * #(TOK_INTEGER_TYPE type_node prec (TOK_STRING)? (TOK_UNSIGNED)? min max );
 */
struct integer_type : public type_node
{
   /// constructor
   explicit integer_type(unsigned int i) : type_node(i), prec(0), unsigned_flag(false)
   {
   }

   /*type_node fields are in the parent class: type_node*/

   /**
    * prec field is the number of bits used by this type
    * (macro TYPE_PRECISION)
    */
   unsigned int prec;

   /// FIXME: add the meaning of this field
   std::string str;

   /**
    * unsigned means an unsigned type
    * (macro TYPE_UNSIGNED)
    */
   bool unsigned_flag;

   /// min: tree-dump.c use the macro TYPE_MIN_VALUE
   tree_nodeRef min;

   /// max: tree-dump.c use the macro TYPE_MAX_VALUE
   tree_nodeRef max;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(integer_type)

   /// Redefinition of get_kind.
   GET_KIND(integer_type)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(type_node) = 0,
      GETID(min),
      GETID(max)
   };
};

/**
 * struct definition of the label_decl tree node.
 * The tree walker structure of this node is:
 * #(TOK_LABEL_DECL decl_node (uid)?)
 */
CREATE_TREE_NODE_CLASS(label_decl, decl_node);

/**
 * This struct specifies the gimple_label node.
 * A label definition, encapsulated as a statement.
 * The operand is the LABEL_DECL node for the label that appears here.
 * The type should be void and the value should be ignored.
 */
struct gimple_label : public gimple_node
{
   /// constructor
   explicit gimple_label(unsigned int i) : gimple_node(i)
   {
   }

   /// the label
   tree_nodeRef op;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(gimple_label)

   /// Redefinition of get_kind.
   GET_KIND(gimple_label)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(gimple_node) = 0,
      GETID(op)
   };
};

/**
 * struct definition of the lang_type tree node.
 * The tree walker structure of this node is:
 * #(TOK_LANG_TYPE type_node);
 */
CREATE_TREE_NODE_CLASS(lang_type, type_node);

/**
 * This struct specifies the le_expr node.
 * Relational operator.LE_EXPR is allowed only for integer (or pointer or enumerate) or real types.
 * The operands will have the same type, and the value is always the type used by the language for booleans.
 */
CREATE_TREE_NODE_CLASS(le_expr, binary_expr);

/**
 * A loop. The operand is the body of the loop.
 * It must contain an EXIT_EXPR or is an infinite loop.
 * The type should be void and the value should be ignored.
 */
CREATE_TREE_NODE_CLASS(loop_expr, unary_expr);

/**
 * This struct specifies the lrotate_expr node.
 * Shift operation for rotate.
 * Shift means logical shift if done on an unsigned type, arithmetic shift if done on a signed type.
 * The second operand is the number of bits to shift by; it need not be the same type as the first operand and result.
 * Note that the result is undefined if the second operand is larger than the first operand's type size.
 */
CREATE_TREE_NODE_CLASS(lrotate_expr, binary_expr);

/**
 * This struct specifies the lshift_expr node.
 * Shift operation for shift.
 * Shift means logical shift if done on an unsigned type, arithmetic shift if done on a signed type.
 * The second operand is the number of bits to shift by; it need not be the same type as the first operand and result.
 * Note that the result is undefined if the second operand is larger than the first operand's type size.
 */
CREATE_TREE_NODE_CLASS(lshift_expr, binary_expr);

/**
 * This struct specifies the lt_expr node.
 * Relational operator. LT_EXPR is allowed only for integer (or pointer or enumerate) or real types.
 * The operands will have the same type, and the value is always the type used by the language for booleans.
 */
CREATE_TREE_NODE_CLASS(lt_expr, binary_expr);

/**
 * This struct specifies the ltgt_expr node.
 * This is the reverse of uneq_expr.
 */
CREATE_TREE_NODE_CLASS(ltgt_expr, binary_expr);

/**
 * This struct specifies the max_expr node.
 */
CREATE_TREE_NODE_CLASS(max_expr, binary_expr);

/**
 * Memory addressing.  Operands are a pointer and a tree constant integer
 * byte offset of the pointer type that when dereferenced yields the
 * type of the base object the pointer points into and which is used for
 * TBAA purposes.
 * The type of the MEM_REF is the type the bytes at the memory location
 * are interpreted as.
 * MEM_REF <p, c> is equivalent to ((typeof(c))p)->x... where x... is a
 * chain of component references offsetting p by c.
 */
CREATE_TREE_NODE_CLASS(mem_ref, binary_expr);

/**
 * struct definition of the method_type tree node.
 * METHOD_TYPE is the type of a function which takes an extra first
 * argument for "self", which is not present in the declared argument list.
 * The TREE_TYPE is the return type of the method.  The TYPE_METHOD_BASETYPE
 * is the type of "self".  TYPE_ARG_TYPES is the real argument list, which
 * includes the hidden argument for "self".
 * The tree walker structure of this node is:
 * #(TOK_METHOD_TYPE type_node (clas)? (retn)? (prms)? )
 */
struct method_type : public function_type
{
   /// constructor
   explicit method_type(unsigned int i) : function_type(i)
   {
   }

   // type_node fields are in the parent class: type_node
   // return type and parameters are in the parent class function_type

   /**
    * clas field is the type of "self".
    * (macro TYPE_METHOD_BASETYPE)
    */
   tree_nodeRef clas;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(method_type)

   /// Redefinition of get_kind.
   GET_KIND(method_type)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(function_type) = 0,
      GETID(clas)
   };
};

/**
 * This struct specifies the min_expr node.
 */
CREATE_TREE_NODE_CLASS(min_expr, binary_expr);

/**
 * This struct specifies the minus_expr node.
 * Simple arithmetic.
 */
CREATE_TREE_NODE_CLASS(minus_expr, binary_expr);

/**
 * This struct specifies the modify_expr node.
 * Assignment expression. The first operand is the what to set; the second, the new value.
 */
CREATE_TREE_NODE_CLASS(modify_expr, binary_expr);

/**
 * This struct represent one of the bunch of tree codes for the initial,
 * superficial parsing of templates
 */
CREATE_TREE_NODE_CLASS(modop_expr, expr_node);

/**
 * This struct specifies the mult_expr node.
 * Simple arithmetic.
 */
CREATE_TREE_NODE_CLASS(mult_expr, binary_expr);

/**
 * This struct specifies the mult_highpart_expr node.
 * Highpart multiplication.  For an integral type with precision B,
 *  returns bits [2B-1, B] of the full 2*B product.
 */
CREATE_TREE_NODE_CLASS(mult_highpart_expr, binary_expr);

/**
 * struct definition of the label_decl tree node.
 * The tree walker structure of this node is:
 * #(TOK_NAMESPACE_DECL decl_node (dcls)?)
 */
struct namespace_decl : public decl_node
{
   /// constructor
   explicit namespace_decl(unsigned int i) : decl_node(i)
   {
   }

   /*decl_node fields are in the parent class*/

   /// declarations
   tree_nodeRef dcls;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(namespace_decl)

   /// Redefinition of get_kind.
   GET_KIND(namespace_decl)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(decl_node) = 0,
      GETID(dcls)
   };
};

/**
 * This struct specifies the ne_expr node.
 * Relational operators. NE_EXPR is allowed for any types.
 * The operands will have the same type, and the value is always the type used by the language for booleans.
 */
CREATE_TREE_NODE_CLASS(ne_expr, binary_expr);

/**
 * This struct specifies the negate_expr node.
 * Unary negation.
 */
CREATE_TREE_NODE_CLASS(negate_expr, unary_expr);

/**
 * This struct represent a 'new' expression
 */
CREATE_TREE_NODE_CLASS(new_expr, expr_node);

/**
 * This struct specifies the non_lvalue_expr node.
 * Value is same as argument, but guaranteed not an lvalue.
 */
CREATE_TREE_NODE_CLASS(non_lvalue_expr, unary_expr);

/**
 * This struct specifies the nop_expr node.
 * Represents a conversion expected to require no code to be generated.
 */
CREATE_TREE_NODE_CLASS(nop_expr, unary_expr);

/**
 * Used to represent lookup of runtime type dependent data.  Often this is
 * a reference to a vtable, but it needn't be.  Operands are:
 * OBJ_TYPE_REF_EXPR: An expression that evaluates the value to use.
 * OBJ_TYPE_REF_OBJECT: Is the object on whose behalf the lookup is
 * being performed.  Through this the optimizers may be able to statically
 * determine the dynamic type of the object.
 * OBJ_TYPE_REF_TOKEN: Something front-end specific used to resolve the
 * reference to something simpler, usually to the address of a DECL.
 * Never touched by the middle-end.  Good choices would be either an
 * identifier or a vtable index.
 */
CREATE_TREE_NODE_CLASS(obj_type_ref, ternary_expr);

/**
 * struct definition of the offset_type tree node.
 * The tree walker structure of this node is:
 * #(TOK_OFFSET_TYPE type_node)
 */
CREATE_TREE_NODE_CLASS(offset_type, type_node);

/**
 * This struct specifies the ordered_expr node.
 * Additional relational operator for floating point unordered.
 */
CREATE_TREE_NODE_CLASS(ordered_expr, binary_expr);

/**
 * This struct represents a list-like node for chaining overloading candidates
 */
struct overload : public tree_node
{
   /// constructor
   explicit overload(unsigned int i) : tree_node(i)
   {
   }

   /// Is the current function declaration
   tree_nodeRef crnt;

   /// Is the chain
   tree_nodeRef chan;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(overload)

   /// Redefinition of get_kind.
   GET_KIND(overload)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(tree_node) = 0,
      GETID(crnt),
      GETID(chan)
   };
};

/**
 * struct definition of the parm_decl tree node.
 * The tree walker structure of this node is:
 * #(TOK_PARM_DECL  decl_node (argt)? (size)? algn used (TOK_REGISTER)?)
 */
struct parm_decl : public decl_node
{
   /// constructor
   explicit parm_decl(unsigned int i);

   /*decl_node fields are in the parent class: decl_node*/

   /**
    * argt field is the type in which the argument is actually passed,
    * which may be different from its type within the function.
    * (macro DECL_ARG_TYPE)
    */
   tree_nodeRef argt;

   /**
    * size field holds the size of datum, in bits.
    * (macro DECL_SIZE)
    */
   tree_nodeRef size;

   /**
    * algn field holds the alignment required for the datum, in bits.
    * (macro DECL_ALIGN)
    */
   unsigned int algn;

   /**
    * used is nonzero if the name is used in its scope
    * (macro TREE_USED)
    */
   int used;

   /**
    * register_flag means declared 'register'
    * (macro DECL_REGISTER)
    */
   bool register_flag;

   /**
    * readonly_flag means readonly parameter
    * (macro TREE_READONLY)
    */
   bool readonly_flag;

   /**
    * symbol_memory_tag annotation
    */
   tree_nodeRef smt_ann;

   /// for each bit of the ssa variable tells if it is equal to U,X,0,1
   std::string bit_values;

   /// PointToInformation associated with this ssa_name if the corresponding variable is a pointer
   const PointToInformationRef point_to_information;

   /// Range information about bounds of the function parameter (valid for real_type too)
   RangeRef range;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(parm_decl)

   /// Redefinition of get_kind.
   GET_KIND(parm_decl)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(decl_node) = 0,
      GETID(argt),
      GETID(size),
      GETID(smt_ann)
   };
};

/**
 * This struct specifies the gimple_phi node.
 * SSA PHI operator.  PHI_RESULT is the new  node created by the PHI node. PHI_ARG_LENGTH is the number of arguments.
 * PHI_ARG_ELT returns the Ith tuple <def, edge> from the argument list.  Each tuple contains the incoming reaching
 * definition (SSA_NAME node) and the edge via which that definition is coming through.
 * The tree walker structure of this node is:
 * #(TOK_GIMPLE_PHI res (tuple)*)
 */
struct gimple_phi : public gimple_node
{
 public:
   friend class tree_manager;
   friend class string_cst_fix;
   friend class parm2ssa;

   /// The type of the def edge
   typedef std::pair<tree_nodeRef, unsigned int> DefEdge;

   /// The type of the def edge list
   typedef std::list<DefEdge> DefEdgeList;

 private:
   /** store the list pairs: <def, edge>. Each tuple contains the incoming reaching
    * definition (SSA_NAME node) and the edge via which that definition is coming through.
    */
   DefEdgeList list_of_def_edge;

   /// True if ssa uses are updated
   bool updated_ssa_uses;

 public:
   /**
    * Constructor
    * @param i is the index of the node to be created
    */
   explicit gimple_phi(unsigned int i);

   /// res is the new SSA_NAME node created by the PHI node.
   tree_nodeRef res;

   /// flag for virtual phi
   bool virtual_flag;

   /**
    * Return the list of def edges
    * @return the list of def edges
    */
   const DefEdgeList& CGetDefEdgesList() const;

   /**
    * Remove a defedge
    * @param TM is the tree manager
    * @param def_edge is the def edge to be removed
    * @param update_uses specifies if the uses have to be updated
    */
   void RemoveDefEdge(const tree_managerRef& TM, const DefEdge& def_edge);

   /**
    * Add a defedge
    * @param TM is the tree manager
    * @param def_edge is the def edge to be added
    * @param update_uses specifies if the uses have to be updated
    */
   void AddDefEdge(const tree_managerRef& TM, const DefEdge& def_edge);

   /**
    * Replace a defedge
    * @param old_def_edge is the def edge to be removed
    * @param new_def_edge is the def edge to be added
    * @param update_uses specifies if the uses have to be updated
    */
   void ReplaceDefEdge(const tree_managerRef& TM, const DefEdge& old_def_edge, const DefEdge& new_def_edge);

   /**
    * Set the def edge list removing the ond one
    * @param TM is the tree manager
    * @param new_def_edge_list is the new def edge list
    * @param update_uses specifies if the uses have to be updated
    */
   void SetDefEdgeList(const tree_managerRef& TM, DefEdgeList new_list_of_def_edge);

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(gimple_phi)

   /// Redefinition of get_kind.
   GET_KIND(gimple_phi)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(gimple_node) = 0,
      GETID(res),
      GETID(list_of_def_edge)
   };

   /**
    * Set that uses of ssa have been computed
    */
   void SetSSAUsesComputed();
};

/**
 * This struct specifies a hint for branch prediction.
   PREDICT is one of the predictors from predict.def.
   OUTCOME is NOT_TAKEN or TAKEN.
*/
struct gimple_predict : public gimple_node
{
   /// Constructor
   explicit gimple_predict(unsigned int index);

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(gimple_predict)

   /// Redefinition of get_kind.
   GET_KIND(gimple_predict)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(gimple_node) = 0
   };
};

/**
 * This struct specifies the placeholder_expr node.
 * Such a "self-references" is done using a PLACEHOLDER_EXPR.  This is a node that will later be replaced with the object being referenced.
 * Its type is that of the object and selects which object to use from a chain of references (see below).  No other slots are used in the PLACEHOLDER_EXPR.
 * This node denotes a record to later be substituted before evaluating this expression.
 * The type of this expression is used to find the record to replace it.
 */
CREATE_TREE_NODE_CLASS(placeholder_expr, expr_node);

/**
 * This struct specifies the plus_expr node.
 * Simple arithmetic.
 */
CREATE_TREE_NODE_CLASS(plus_expr, binary_expr);

/**
 * Pointer addition.  The first operand is always a pointer and the
 * second operand is an integer of type sizetype.
 */
CREATE_TREE_NODE_CLASS(pointer_plus_expr, binary_expr);

/**
 * struct definition of the pointer_type tree node.
 * The tree walker structure of this node is:
 * #(TOK_POINTER_TYPE type_node ptd)
 */
struct pointer_type : public type_node
{
   /// constructor
   explicit pointer_type(unsigned int i) : type_node(i)
   {
   }

   /*type_node fields are in the parent class*/

   /**
    * ptd field points to the node for the type pointed to.
    * (macro TREE_TYPE)
    */
   tree_nodeRef ptd;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(pointer_type)

   /// Redefinition of get_kind.
   GET_KIND(pointer_type)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(type_node) = 0,
      GETID(ptd)
   };
};

/**
 * This struct specifies the postdecrement_expr node.
 * struct for -- in C. The second arg is how much to increment by.
 * For a pointer, it would be the size of the object pointed to.
 */
CREATE_TREE_NODE_CLASS(postdecrement_expr, binary_expr);

/**
 * This struct specifies the postincrement_expr node.
 * struct for ++ in C. The second arg is how much to increment by.
 * For a pointer, it would be the size of the object pointed to.
 */
CREATE_TREE_NODE_CLASS(postincrement_expr, binary_expr);

/**
 * This struct specifies the predecrement_expr node.
 * struct for -- in C. The second arg is how much to increment by.
 * For a pointer, it would be the size of the object pointed to.
 */
CREATE_TREE_NODE_CLASS(predecrement_expr, binary_expr);

/**
 * This struct specifies the preincrement_expr node.
 * struct for ++ in C. The second arg is how much to increment by.
 * For a pointer, it would be the size of the object pointed to.
 */
CREATE_TREE_NODE_CLASS(preincrement_expr, binary_expr);

/**
 * struct definition of the qual_union_type tree node.
 * The tree walker structure of this node is:
 * #(TOK_QUAL_UNION_TYPE type_node);
 */
CREATE_TREE_NODE_CLASS(qual_union_type, type_node);

/**
 * This struct specifies the range_expr node.
 * Operation for Pascal sets.
 */
CREATE_TREE_NODE_CLASS(range_expr, binary_expr);

/**
 * Represents a re-association barrier for floating point expressions
 * like explicit parenthesis in fortran.
 */
CREATE_TREE_NODE_CLASS(paren_expr, unary_expr);

/**
 * This struct specifies the rdiv_expr node.
 * Division for real result.
 */
CREATE_TREE_NODE_CLASS(rdiv_expr, binary_expr);

/**
 * This struct specifies the real_cst node.
 * Contents are in TREE_REAL_CST field.
 * The tree walker structure of this node is:
 * #(TOK_REAL_CST type (TOK_OVERFLOW)? valr valx)
 */
struct real_cst : public cst_node
{
   /// constructor
   explicit real_cst(unsigned int i) : cst_node(i), overflow_flag(false)
   {
   }

   /**
    * overflow_flag  means there was an overflow in folding, and no warning has been issued for this subexpression.
    * TREE_OVERFLOW implies TREE_CONSTANT_OVERFLOW, but not vice versa.
    */
   bool overflow_flag;

   /// valr is the real value
   std::string valr;

   /// valx field
   std::string valx;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(real_cst)

   /// Redefinition of get_kind.
   GET_KIND(real_cst)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(cst_node) = 0
   };
};

/**
 * This struct specifies the realpart_expr node.
 * Used only on an operand of complex type, this return a value of the corresponding component type.
 */
CREATE_TREE_NODE_CLASS(realpart_expr, unary_expr);

/**
 * struct definition of the real_type tree node.
 * The tree walker structure of this node is:
 * #(TOK_REAL_TYPE type_node prec);
 */
struct real_type : public type_node
{
   /// constructor
   explicit real_type(unsigned int i) : type_node(i), prec(0)
   {
   }

   /*type_node fields are in the parent class*/

   /**
    * prec field is the number of bits used by this type.
    * (macro TYPE_PRECISION)
    */
   unsigned int prec;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(real_type)

   /// Redefinition of get_kind.
   GET_KIND(real_type)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(type_node) = 0
   };
};

/**
 * struct definition of the record_type tree node.
 * The tree walker structure of this node is:
 * #(TOK_RECORD_TYPE type_node (vfld)? (TOK_SPEC)? TOK_STRUCT (flds)? (fncs)? (binf)?)
 */
struct record_type : public type_node
{
   /// constructor
   explicit record_type(unsigned int i) : type_node(i), spec_flag(false), ptrmem_flag(false), struct_flag(false)
   {
   }

   /**
    * list_of_flds is a chain of field_decl for the fields of the struct,
    * and var_decl, type_decl and const_decl for record-scope variables, types and enumerators.
    * (macro  TYPE_FIELDS)
    */
   std::vector<tree_nodeRef> list_of_flds;

   /**
    * list_of_fncs is a chain of methods_decl for the fields of the struct.
    * (macro TYPE_METHODS)
    */
   std::vector<tree_nodeRef> list_of_fncs;

   /// If pointer mem point to type
   tree_nodeRef ptd;

   /// If pointer mem struct type
   tree_nodeRef cls;

   /**
    * binf field are information about this type, as a base type for itself.
    * It is a binfo node.
    * (macro TYPE_BINFO)
    */
   tree_nodeRef bfld;

   /// type context
   tree_nodeRef binf;

   /// true when a spec is considered
   bool spec_flag;

   /// true when ptrmem obj is considered
   bool ptrmem_flag;

   /// true when a struct is considered
   bool struct_flag;

   /// FIXME: check the meaning
   tree_nodeRef vfld;

   /** tmpl_parms holds template parameters
    * It is a TREE_LIST, his VALU field is a TREE_VEC whose LIST_OF_OP holds template parameters.
    * The instantion of parameter "list_of_op[i]" is "list_of_op[i]" hold in tmpl_args.
    */
   tree_nodeRef tmpl_parms;

   /** tmpl_args holds template instantiations
    * It is a TREE_VEC whose LIST_OF_OP holds template instantiations.
    * The parameter of  instantiation "list_of_op[i]" is "list_of_op[i]" hold in tmpl_parms.
    */
   tree_nodeRef tmpl_args;

   /**
    * Add a field_decl to list of flds.
    * @param a is a NODE_ID.
    */
   void add_flds(const tree_nodeRef& a)
   {
      list_of_flds.push_back(a);
   }

   /**
    * Add a methods_decl to list of fncs.
    * @param a is a NODE_ID.
    */
   void add_fncs(const tree_nodeRef& a)
   {
      list_of_fncs.push_back(a);
   }

   /**
    * returns tree_nodeRef of the field specified by offset
    * @param offset is the offset of the field from the base address of the record_type
    * @return the tree_nodeRef if the offset is valid else null pointer
    */
   tree_nodeRef get_field(long long int offset);

   /**
    * returns the name of the struct represented by this node if there is one else
    * returns the string \#UNKNOWN\#
    */
   std::string get_maybe_name() const;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(record_type)

   /// Redefinition of get_kind.
   GET_KIND(record_type)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(type_node) = 0,
      GETID(vfld),
      GETID(list_of_flds),
      GETID(list_of_fncs),
      GETID(ptd),
      GETID(cls),
      GETID(bfld),
      GETID(binf),
      GETID(tmpl_parms),
      GETID(tmpl_args)
   };
};

/**
 * Reduction operations.
 * Operations that take a vector of elements and "reduce" it to a scalar
 * result (e.g. summing the elements of the vector, finding the minimum over
 * the vector elements, etc).
 * Operand 0 is a vector; the first element in the vector has the result.
 * Operand 1 is a vector.
 */
CREATE_TREE_NODE_CLASS(reduc_max_expr, unary_expr);
CREATE_TREE_NODE_CLASS(reduc_min_expr, unary_expr);
CREATE_TREE_NODE_CLASS(reduc_plus_expr, unary_expr);

/**
 * This struct specifies the reference_expr node.
 * Non-lvalue reference or pointer to an object.
 */
CREATE_TREE_NODE_CLASS(reference_expr, unary_expr);

/**
 * struct definition of the reference_type tree node.
 * The tree walker structure of this node is:
 * #(TOK_REFERENCE_TYPE type_node refd)
 */
struct reference_type : public type_node
{
   /// constructor
   explicit reference_type(unsigned int i) : type_node(i)
   {
   }

   /*type_node fields are in the parent class*/

   /**
    * refd field references to the node for the type referenced to.
    * (macro TREE_TYPE)
    */
   tree_nodeRef refd;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(reference_type)

   /// Redefinition of get_kind.
   GET_KIND(reference_type)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(type_node) = 0,
      GETID(refd)
   };
};

/**
 * This struct represent one of tree codes for the initial,
 * superficial parsing of templates
 */
CREATE_TREE_NODE_CLASS(reinterpret_cast_expr, unary_expr);

/**
 * struct definition of the result_decl tree node.
 * The tree walker structure of this node is:
 * #(TOK_RESULT_DECL decl_node (init)? size algn)
 */
struct result_decl : public decl_node
{
   /// constructor
   explicit result_decl(unsigned int i) : decl_node(i), algn(0)
   {
   }

   /*decl_node fields are in the parent class*/

   /**
    * init field holds the value to initialize a variable to.
    * (macro DECL_INITIAL)
    */
   tree_nodeRef init;

   /**
    * size field holds the size of datum, in bits.
    * (macro DECL_SIZE)
    */
   tree_nodeRef size;

   /**
    * algn field holds the alignment required for the datum, in bits.
    * (macro DECL_ALIGN)
    */
   unsigned int algn;

   /**
    * symbol_memory_tag annotation
    */
   tree_nodeRef smt_ann;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(result_decl)

   /// Redefinition of get_kind.
   GET_KIND(result_decl)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(decl_node) = 0,
      GETID(init),
      GETID(size),
      GETID(smt_ann)
   };
};

struct gimple_resx : public gimple_node
{
   /// constructor
   explicit gimple_resx(unsigned int i) : gimple_node(i)
   {
   }

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(gimple_resx)

   /// Redefinition of get_kind.
   GET_KIND(gimple_resx)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(gimple_node) = 0
   };
};

/**
 * This struct specifies the gimple_return node.
 * Evaluates the operand, then returns from the current function.
 * Presumably that operand is an assignment that stores into the RESULT_DECL that hold the value to be returned.
 * The operand may be null. The type should be void and the value should be ignored.
 * The tree walker structure of this node is:
 * #(TOK_GIMPLE_RETURN type (op)?)
 */
struct gimple_return : public gimple_node
{
   /// constructor
   explicit gimple_return(unsigned int i) : gimple_node(i)
   {
   }

   /// op field is the operand of this node
   tree_nodeRef op;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(gimple_return)

   /// Redefinition of get_kind.
   GET_KIND(gimple_return)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(gimple_node) = 0,
      GETID(op)
   };
};

/**
 * This struct represent a 'return' statement
 */
struct return_stmt : public tree_node
{
   /// constructor
   explicit return_stmt(unsigned int i) : tree_node(i), line(-1)
   {
   }

   /// line is the line number where the compound_stmt is defined.
   int line;

   /// Is the expression of the statement
   tree_nodeRef expr;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(return_stmt)

   /// Redefinition of get_kind.
   GET_KIND(return_stmt)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(tree_node) = 0,
      GETID(expr)
   };
};

/**
 * This struct specifies the round_div_expr node.
 * Division for integer result that rounds toward nearest integer.
 */
CREATE_TREE_NODE_CLASS(round_div_expr, binary_expr);

/**
 * This struct specifies the round_mod_expr node.
 * Kind of remainder that go with the kind of division.
 */
CREATE_TREE_NODE_CLASS(round_mod_expr, binary_expr);

/**
 * This struct specifies the rrotate_expr node.
 * Shift operation for rotate.
 * Shift means logical shift if done on an unsigned type, arithmetic shift if done on a signed type.
 * The second operand is the number of bits to shift by; it need not be the same type as the first operand and result.
 * Note that the result is undefined if the second operand is larger than the first operand's type size.
 */
CREATE_TREE_NODE_CLASS(rrotate_expr, binary_expr);

/**
 * This struct specifies the rshift_expr node.
 * Shift operation for shift.
 * Shift means logical shift if done on an unsigned type, arithmetic shift if done on a signed type.
 * The second operand is the number of bits to shift by; it need not be the same type as the first operand and result.
 * Note that the result is undefined if the second operand is larger than the first operand's type size.
 */
CREATE_TREE_NODE_CLASS(rshift_expr, binary_expr);

/**
 * This struct specifies the save_expr node.
 * Represents something we computed once and will use multiple times.
 * First operand is that expression.  Second is the function decl in which the SAVE_EXPR was created.
 * The third operand is the RTL, nonzero only after the expression has been computed.
 */
CREATE_TREE_NODE_CLASS(save_expr, ternary_expr);

/**
 * This struct specifies reference to particular overloaded struct method
 * The tree walker structure of this node is:
 * #(TOK_SCOPE_REF)
 */
struct scope_ref : public expr_node
{
   /// constructor
   explicit scope_ref(unsigned int i) : expr_node(i)
   {
   }

   /// The first operand of the binary expression
   tree_nodeRef op0;

   /// The second operand of the binary expression
   tree_nodeRef op1;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(scope_ref)

   /// Redefinition of get_kind.
   GET_KIND(scope_ref)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(expr_node) = 0,
      GETID(op0),
      GETID(op1)
   };
};

/**
 * This struct specifies the set_le_expr node.
 * Operation for Pascal sets.
 */
CREATE_TREE_NODE_CLASS(set_le_expr, binary_expr);

/**
 * struct definition of the set_type tree node.
 * The tree walker structure of this node is:
 * #(TOK_SET_TYPE type_node)
 */
CREATE_TREE_NODE_CLASS(set_type, type_node);

/**
 * This struct specifies the sizeof_expr node.
 * Represents the size value of the operand.
 */
CREATE_TREE_NODE_CLASS(sizeof_expr, unary_expr);

/**
 * This struct specifies the ssa_name node.
 * Variable references for SSA analysis.  New SSA names are created every time a variable is assigned a new value.
 * The SSA builder uses SSA_NAME nodes to implement SSA versioning.
 * The tree walker structure of this node is:
 * #(TOK_SSA_NAME var vers)
 */
struct ssa_name : public tree_node
{
 private:
   /// The uses of this ssa: it is a map since the same ssa can be used multiple times in the same statement
   /// Key is the index of the tree node instad of tree node because in this way gimple_node can updated uses
   /// of a ssa without using the tree manager to get the tree reindex of itself
   TreeNodeMap<size_t> use_stmts;

   /// in case ssa_name is not volatile the statement which defines it; statements could be more than one because of gimple_phi splitting
   TreeNodeSet def_stmts;

 public:
   /// constructor
   explicit ssa_name(unsigned int i);

   /// starting from gcc 4.7.2 ssa_name has a type
   tree_nodeRef type;

   /// var is the variable being referenced (macro SSA_NAME_VAR).
   tree_nodeRef var;

   /// vers is the SSA version number of this SSA name.(macro SSA_NAME_VERSION). Note that in tree SSA, version numbers are not per variable and may be recycled
   unsigned int vers;

   /// original SSA version number from GCC
   unsigned int orig_vers;

   /// in case a ssa_name is never defined this boolean member is true
   bool volatile_flag;

   /// flag for virtual ssa
   bool virtual_flag;

   /**
    * Nonzero if this SSA_NAME is the default definition for the
    * underlying symbol.  A default SSA name is created for symbol S if
    * the very first reference to S in the function is a read operation.
    * Default definitions are always created by an empty statement and
    * belong to no basic block.
    * */
   bool default_flag;

   /**
    * Add use of this ssa
    * @param statement is the statement which used this ssa
    */
   void AddUseStmt(const tree_nodeRef& use_stmt);

   /**
    * Return the use stmts
    * @return the use stmts
    */
   const TreeNodeMap<size_t>& CGetUseStmts() const;

   /**
    * Return the number of uses
    * @return the number of uses
    */
   size_t CGetNumberUses() const;

   /**
    * Remove a use of this ssa
    * @param use_stmt is the statement which uses this ssa
    */
   void RemoveUse(const tree_nodeRef& use_stmt);

   /// minimum values this ssa may reach
   tree_nodeRef min;

   /// maximum values this ssa may reach
   tree_nodeRef max;

   /// for each bit of the ssa variable tells if it is equal to U,X,0,1
   std::string bit_values;

   /// Range information about numerical values of the ssa variable
   RangeRef range;

   /// point to solution
   PointToSolutionRef use_set;

   /// PointToInformation associated with this ssa_name if the corresponding variable is a pointer
   const PointToInformationRef point_to_information;

   /**
    * Set the def stmt erasing the old definitions
    * @param a is a def statement.
    */
   void SetDefStmt(const tree_nodeRef& def);

   /**
    * Add a def stmt
    * @param def is a def statement.
    */
   void AddDefStmt(const tree_nodeRef& def);

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(ssa_name)

   /// Redefinition of get_kind.
   GET_KIND(ssa_name)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /**
    * Return the def stmt (checking that is unique)
    * @return the definition statement
    */
   const tree_nodeRef CGetDefStmt() const;

   /**
    * Return the set of definition statements
    * @return the definition statements
    */
   const TreeNodeSet CGetDefStmts() const;

   /// visitor enum
   enum
   {
      GETID(tree_node) = 0,
      GETID(type),
      GETID(var),
      GETID(use_stmts),
      GETID(def_stmts),
      GETID(min),
      GETID(max),
      GETID(use_set)
   };
};

/**
 * This struct specifies the statement_list node.
 * Used to chain children of container statements together.
 * The tree walker structure of this node is:
 * #(TOK_STATEMENT_LIST( (stmt)+ | (bloc)+ )?)
 */
struct statement_list : public tree_node
{
   /// constructor
   explicit statement_list(unsigned int i) : tree_node(i)
   {
   }

   /// list_of_stmt field is the list of statements. If this field is null then the list_of_bloc field is not null.
   std::list<tree_nodeRef> list_of_stmt;

   /// list_of_bloc field is the list of basic block. If this field is null then the list_of_stmt field is not null.
   std::map<unsigned int, blocRef> list_of_bloc;

   /**
    * Add a value to list of basic block.
    * @param a is a NODE_ID.
    */
   void add_bloc(const blocRef& a);

   /**
    * Add a value to list of stmt.
    * @param a is a NODE_ID.
    */
   void add_stmt(const tree_nodeRef& a)
   {
      list_of_stmt.push_back(a);
   }

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(statement_list)

   /// Redefinition of get_kind.
   GET_KIND(statement_list)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(tree_node) = 0,
      GETID(list_of_stmt),
      GETID(list_of_bloc)
   };
};

/**
 * This struct specifies the static_cast_expr node.
 * The walker for this struct is
 *   #(TOK_STATIC_CAST_EXPR wunary_expr);
 */
CREATE_TREE_NODE_CLASS(static_cast_expr, unary_expr);

/**
 * This struct specifies the string_cst node.
 * Contents are TREE_STRING_LENGTH and TREE_STRING_POINTER fields.
 * The tree walker structure of this node is:
 * #(TOK_STRING_CST type strg lngt)
 */
struct string_cst : public cst_node
{
   /// constructor
   explicit string_cst(unsigned int i) : cst_node(i), lngt(-1)
   {
   }

   /// strg is the TREE_STRING_POINTER.
   std::string strg;

   /// lngt is the TREE_STRING_LENGTH.
   int lngt;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(string_cst)

   /// Redefinition of get_kind.
   GET_KIND(string_cst)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(cst_node) = 0
   };
};

/**
 * GIMPLE_SWITCH <INDEX, DEFAULT_LAB, LAB1, ..., LABN> represents the
 *  multiway branch:
 *
 *  switch (INDEX)
 *  {
 *    case LAB1: ...; break;
 *   ...
 *   case LABN: ...; break;
 *   default: ...
 * }
 *
 * INDEX is the variable evaluated to decide which label to jump to.
 *
 * DEFAULT_LAB, LAB1 ... LABN are the tree nodes representing case labels.
 * They must be CASE_LABEL_EXPR nodes.
 */
struct gimple_switch : public gimple_node
{
   /// constructor
   explicit gimple_switch(unsigned int i) : gimple_node(i)
   {
   }

   /// the branch var
   tree_nodeRef op0;

   /// the vec of CASE_LABEL_EXPRs
   tree_nodeRef op1;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(gimple_switch)

   /// Redefinition of get_kind.
   GET_KIND(gimple_switch)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(gimple_node) = 0,
      GETID(op0),
      GETID(op1)
   };
};

/**
 * This struct implements the target_expr node.
 * For TARGET_EXPR, operand 0 is the target of an initialization; operand 1 is the initializer for the target;
 * and operand 2 is the cleanup for this node, if any; operand 3 is the saved initializer after this node has been expanded once, this is so we can re-expand the tree later.
 * The tree walker structure of this node is:
 * #(TOK_TARGET_EXPR type decl (((init) (clnp)?) | ((clnp) (init)?))?)
 */

struct target_expr : public expr_node
{
   explicit target_expr(unsigned int i) : expr_node(i)
   {
   }

   /// it is the target of an initialization
   tree_nodeRef decl;

   /// it is the initializer for the target
   tree_nodeRef init;

   /// it is the cleanup for this node
   tree_nodeRef clnp;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(target_expr)

   /// Redefinition of get_kind.
   GET_KIND(target_expr)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(expr_node) = 0,
      GETID(decl),
      GETID(init),
      GETID(clnp)
   };
};
/**
 * Low-level memory addressing.  Operands are SYMBOL (static or global
 * variable), BASE (register), INDEX (register), STEP (integer constant),
 * OFFSET (integer constant).  Corresponding address is
 * SYMBOL + BASE + STEP * INDEX + OFFSET.  Only variations and values valid on
 * the target are allowed.
 *
 * The type of STEP, INDEX and OFFSET is sizetype.  The type of BASE is
 * sizetype or a pointer type (if SYMBOL is NULL).
 *
 * The sixth argument is the reference to the original memory access, which
 * is preserved for the purposes of the RTL alias analysis.  The seventh
 * argument is a tag representing results of the tree level alias analysis.
 */
struct target_mem_ref : public WeightedNode
{
   /// constructor
   explicit target_mem_ref(unsigned int i) : WeightedNode(i)
   {
   }

   /// type of the expression
   tree_nodeRef type;

   /// static or global variable
   tree_nodeRef symbol;

   /// BASE register
   tree_nodeRef base;

   /// INDEX register
   tree_nodeRef idx;

   /// STEP integer constant
   tree_nodeRef step;

   /// OFFSET integer constant
   tree_nodeRef offset;

   /// original memory access
   tree_nodeRef orig;

   /// result of the tree level alias analysis
   tree_nodeRef tag;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(target_mem_ref)

   /// Redefinition of get_kind.
   GET_KIND(target_mem_ref)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(WeightedNode) = 0,
      GETID(type),
      GETID(symbol),
      GETID(base),
      GETID(idx),
      GETID(step),
      GETID(offset),
      GETID(orig),
      GETID(tag)
   };
};

/**
 * Low-level memory addressing.  Operands are BASE (address of static or
 * global variable or register), OFFSET (integer constant),
 * INDEX (register), STEP (integer constant), INDEX2 (register),
 * The corresponding address is BASE + STEP * INDEX + INDEX2 + OFFSET.
 * Only variations and values valid on the target are allowed.
 *
 * The type of STEP, INDEX and INDEX2 is sizetype.
 *
 * The type of BASE is a pointer type.  If BASE is not an address of
 * a static or global variable INDEX2 will be NULL.
 *
 * The type of OFFSET is a pointer type and determines TBAA the same as
 * the constant offset operand in MEM_REF.
 */
struct target_mem_ref461 : public WeightedNode
{
   /// constructor
   explicit target_mem_ref461(unsigned int i) : WeightedNode(i)
   {
   }

   /// type of the expression
   tree_nodeRef type;

   /// BASE register
   tree_nodeRef base;

   /// OFFSET integer constant
   tree_nodeRef offset;

   /// INDEX register
   tree_nodeRef idx;

   /// STEP integer constant
   tree_nodeRef step;

   /// INDEX register
   tree_nodeRef idx2;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(target_mem_ref461)

   /// Redefinition of get_kind.
   GET_KIND(target_mem_ref461)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(WeightedNode) = 0,
      GETID(type),
      GETID(base),
      GETID(idx),
      GETID(idx2),
      GETID(step),
      GETID(offset)
   };
};

/**
 * struct definition of the template_decl tree node.
 * The tree walker structure of this node is:
 * #(TOK_TEMPLATE_DECL decl_node (rslt)? (inst)? (spcs)? (prms)?)
 */
struct template_decl : public decl_node
{
   /// constructor
   explicit template_decl(unsigned int i) : decl_node(i)
   {
   }

   /*decl_node fields are in the parent class: decl_node*/

   /// rslt is null for struct templates and declaration for object to be created for non-struct templates
   tree_nodeRef rslt;

   /// inst field holds the template instantiatin vector.
   tree_nodeRef inst;

   /// prms field holds the specialization parameters vector.
   tree_nodeRef spcs;

   /// prms field holds the template parameters vector.
   tree_nodeRef prms;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(template_decl)

   /// Redefinition of get_kind.
   GET_KIND(template_decl)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(decl_node) = 0,
      GETID(rslt),
      GETID(inst),
      GETID(base),
      GETID(spcs),
      GETID(prms)
   };
};

/**
 * This struct represents a template identifier
 */
CREATE_TREE_NODE_CLASS(template_id_expr, expr_node);

/* Index into a template parameter list.  The TEMPLATE_PARM_IDX gives
   the index (from 0) of the parameter, while the TEMPLATE_PARM_LEVEL
   gives the level (from 1) of the parameter.

   Here's an example:

   template <class T> // Index 0, Level 1.
   struct S
   {
      template <class U, // Index 0, Level 2.
      class V> // Index 1, Level 2.
      void f();
   };

   The DESCENDANTS will be a chain of TEMPLATE_PARM_INDEXs descended
   from this one.  The first descendant will have the same IDX, but
   its LEVEL will be one less.  The TREE_CHAIN field is used to chain
   together the descendants.  The TEMPLATE_PARM_DECL is the
   declaration of this parameter, either a TYPE_DECL or CONST_DECL.
   The TEMPLATE_PARM_ORIG_LEVEL is the LEVEL of the most distant
   parent, i.e., the LEVEL that the parameter originally had when it
   was declared.  For example, if we instantiate S<int>, we will have:

   struct S<int>
   {
     template <class U, // Index 0, Level 1, Orig Level 2
          class V> // Index 1, Level 1, Orig Level 2
     void f();
   };

   The LEVEL is the level of the parameter when we are worrying about
   the types of things; the ORIG_LEVEL is the level when we are
   worrying about instantiating things.  */
struct template_parm_index : public tree_node
{
   tree_nodeRef type;
   tree_nodeRef decl;
   bool constant_flag;
   bool readonly_flag;
   int idx;
   int level;
   int orig_level;

   /// constructor
   explicit template_parm_index(unsigned int i) : tree_node(i), constant_flag(false), readonly_flag(false), idx(0), level(0), orig_level(0)
   {
   }

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(template_parm_index)

   /// Redefinition of get_kind.
   GET_KIND(template_parm_index)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(tree_node) = 0,
      GETID(type),
      GETID(decl)
   };
};

/**
 * struct definition of the template_type_parm tree node.
 * The tree walker structure of this node is:
 * #(TOK_TEMPLATE_TYPE_PARM type_node)
 */
CREATE_TREE_NODE_CLASS(template_type_parm, type_node);

/**
 * Represents an argument pack of types (or templates). An argument
 * pack stores zero or more arguments that will be used to instantiate
 * a parameter pack.
 *
 * ARGUMENT_PACK_ARGS retrieves the arguments stored in the argument
 * pack.
 *
 * Example:
 *   template<typename... Values>
 *   class tuple { ... };
 *
 *   tuple<int, float, double> t;
 *
 * Values is a (template) parameter pack. When tuple<int, float,
 * double> is instantiated, the Values parameter pack is instantiated
 * with the argument pack <int, float, double>. ARGUMENT_PACK_ARGS will
   be a TREE_VEC containing int, float, and double.
*/
struct type_argument_pack : public type_node
{
   /// constructor
   explicit type_argument_pack(unsigned int i) : type_node(i)
   {
   }

   /// arguments stored in the argument pack
   tree_nodeRef arg;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(type_argument_pack)

   /// Redefinition of get_kind.
   GET_KIND(type_argument_pack)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(type_node) = 0,
      GETID(arg)
   };
};

/* Represents an argument pack of values, which can be used either for
   non-type template arguments or function call arguments.

   NONTYPE_ARGUMENT_PACK plays precisely the same role as
   TYPE_ARGUMENT_PACK, but will be used for packing non-type template
   arguments (e.g., "int... Dimensions") or function arguments ("const
   Args&... args"). */

struct nontype_argument_pack : public expr_node
{
   /// constructor
   explicit nontype_argument_pack(unsigned int i) : expr_node(i)
   {
   }

   /// arguments stored in the argument pack
   tree_nodeRef arg;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(nontype_argument_pack)

   /// Redefinition of get_kind.
   GET_KIND(nontype_argument_pack)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(expr_node) = 0,
      GETID(arg)
   };
};

/**
 * This struct specifies the a + b + c node.
 * Simple arithmetic.
 */
CREATE_TREE_NODE_CLASS(ternary_plus_expr, ternary_expr);

/**
 * This struct specifies the a + b - c node.
 * Simple arithmetic.
 */
CREATE_TREE_NODE_CLASS(ternary_pm_expr, ternary_expr);

/**
 * This struct specifies the a - b + c node.
 * Simple arithmetic.
 */
CREATE_TREE_NODE_CLASS(ternary_mp_expr, ternary_expr);

/**
 * This struct specifies the a - b - c node.
 * Simple arithmetic.
 */
CREATE_TREE_NODE_CLASS(ternary_mm_expr, ternary_expr);

/**
 * This struct represents a throw expression
 */
CREATE_TREE_NODE_CLASS(throw_expr, unary_expr);

/**
 * Represents a trait expression during template expansion.
 */
CREATE_TREE_NODE_CLASS(trait_expr, tree_node);

/**
 * struct definition of the translation_unit_decl tree node.
 * The tree walker structure of this node is:
 * #(TOK_TRANSLATION_UNIT_DECL decl_node)
 */
CREATE_TREE_NODE_CLASS(translation_unit_decl, decl_node);

/**
 * This struct specifies the tree_list node.
 * This node has the TREE_VALUE and TREE_PURPOSE fields. These nodes are made into lists by chaining through the TREE_CHAIN field.
 * The elements of the list live in the TREE_VALUE fields, while TREE_PURPOSE fields are occasionally used as well to get the effect of Lisp association lists.
 * The tree walker structure of this node is:
 * #(TOK_TREE_LIST  (purp)? valu (chan)?);
 */
struct tree_list : public WeightedNode
{
   /// constructor
   explicit tree_list(unsigned int i) : WeightedNode(i)
   {
   }

   /// purp is the TREE_PURPOSE field occasionally used as well to get the effect of Lisp association lists.
   tree_nodeRef purp;

   /// purp is the TREE_VALUE field which stores the elements of the list.
   tree_nodeRef valu;

   /// purp is the TREE_CHAIN field: tree_list nodes are made into lists by chaining through the TREE_CHAIN field.
   tree_nodeRef chan;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(tree_list)

   /// Redefinition of get_kind.
   GET_KIND(tree_list)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(WeightedNode) = 0,
      GETID(purp),
      GETID(valu),
      GETID(chan)
   };
};

/**
 * This struct specifies the tree_vec node.
 * These nodes contain an array of tree nodes.
 * The tree walker structure of this node is:
 * #(TOK_TREE_VEC lngt ((op)+)?)
 */
struct tree_vec : public tree_node
{
   /// constructor
   explicit tree_vec(unsigned int i) : tree_node(i), lngt(0)
   {
   }

   /// lngt is the lenght of the array (list_of_op) stored in tree_vec node.(macro TREE_VEC_LENGTH)
   size_t lngt;

   /// list_of_op is the array of tree node stored in tree_vec node.(macro TREE_VEC_ELT)
   std::vector<tree_nodeRef> list_of_op;

   /**
    * Add a value to list of operands.
    * @param a is a NODE_ID.
    */
   void add_op(const tree_nodeRef& a)
   {
      list_of_op.push_back(a);
   }

   /// returns the number of operands
   size_t get_number_of_op() const
   {
      return list_of_op.size();
   }

   /// return the n-th (from 0 to size() -1) operand of tree_vec
   tree_nodeRef get_op(size_t n) const
   {
      return list_of_op[n];
   }

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(tree_vec)

   /// Redefinition of get_kind.
   GET_KIND(tree_vec)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(tree_node) = 0,
      GETID(list_of_op)
   };
};

/**
 * This struct specifies the trunc_div_expr node.
 * Division for integer result that rounds the quotient toward zero.
 */
CREATE_TREE_NODE_CLASS(trunc_div_expr, binary_expr);

/**
 * This struct specifies the trunc_mod_expr node.
 * Kind of remainder that go with the kind of division.
 */
CREATE_TREE_NODE_CLASS(trunc_mod_expr, binary_expr);

/**
 * This struct specifies the truth_and_expr node.
 * AND always compute the second operand whether its value is needed or not (for side effects).
 * The operand may have BOOLEAN_TYPE or INTEGER_TYPE. The argument will be either zero or one.
 */
CREATE_TREE_NODE_CLASS(truth_and_expr, binary_expr);

/**
 * This struct specifies the truth_andif_expr node.
 * ANDIF allow the second operand not to be computed if the value of the expression is determined from the first operand.
 * The operand may have BOOLEAN_TYPE or INTEGER_TYPE. The argument will be either zero or one.
 */
CREATE_TREE_NODE_CLASS(truth_andif_expr, binary_expr);

/**
 * This struct specifies the truth_not_expr node.
 * The operand may have BOOLEAN_TYPE or INTEGER_TYPE.  In either case, the argument will be
 * either zero or one. TRUTH_NOT_EXPR will never have an INTEGER_TYPE VAR_DECL as its argument.
 */
CREATE_TREE_NODE_CLASS(truth_not_expr, unary_expr);

/**
 * This struct specifies the truth_or_expr node.
 * The operand may have BOOLEAN_TYPE or INTEGER_TYPE. the argument will be either zero or one.
 */
CREATE_TREE_NODE_CLASS(truth_or_expr, binary_expr);

/**
 * This struct specifies the truth_orif_expr node.
 * ORIF allow the second operand not to be computed if the value of the expression is determined from the first operand.
 * The operand may have BOOLEAN_TYPE or INTEGER_TYPE. The argument will be either zero or one.
 */
CREATE_TREE_NODE_CLASS(truth_orif_expr, binary_expr);

/**
 * This struct specifies the truth_xor_expr node.
 * XOR always compute the second operand whether its value is needed or not (for side effects).
 * The operand may have BOOLEAN_TYPE or INTEGER_TYPE. The argument will be either zero or one.
 */
CREATE_TREE_NODE_CLASS(truth_xor_expr, binary_expr);

/**
 * This struct represents a try-block statement
 */
struct try_block : public tree_node
{
   /// constructor
   explicit try_block(unsigned int i) : tree_node(i), line(-1)
   {
   }

   /// line is the line number where the compound_stmt is defined.
   int line;

   /// Is the body of the statement
   tree_nodeRef body;

   /// Is the handler of the statement
   tree_nodeRef hdlr;

   /// Is the next statement
   tree_nodeRef next;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(try_block)

   /// Redefinition of get_kind.
   GET_KIND(try_block)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(tree_node) = 0,
      GETID(body),
      GETID(hdlr),
      GETID(next)
   };
};

/**
 * This struct specifies the try_catch_expr node.
 * Evaluate operand 1.  If and only if an exception is thrown during the evaluation of operand 1, evaluate operand 2.
 * This differs from TRY_FINALLY_EXPR in that operand 2 is not evaluated on a normal or jump exit, only on an exception.
 */
CREATE_TREE_NODE_CLASS(try_catch_expr, binary_expr);

/**
 * This struct specifies the try_finally node.
 * Evaluate the first operand.
 * The second operand is a cleanup expression which is evaluated on any exit (normal, exception, or jump out) from this expression.
 */
CREATE_TREE_NODE_CLASS(try_finally, binary_expr);

/**
 * struct definition of the type_decl tree node.
 * The tree walker structure of this node is:
 * #(TOK_TYPE_DECL decl_node)
 */
struct type_decl : public decl_node
{
   /// constructor
   explicit type_decl(unsigned int i) : decl_node(i)
   {
   }

   /** tmpl_parms holds template parameters
    * It is a TREE_LIST, his VALU field is a TREE_VEC whose LIST_OF_OP holds template parameters.
    * The instantion of parameter "list_of_op[i]" is "list_of_op[i]" hold in tmpl_args.
    */
   tree_nodeRef tmpl_parms;

   /** tmpl_args holds template instantiations
    * It is a TREE_VEC whose LIST_OF_OP holds template instantiations.
    * The parameter of  instantiation "list_of_op[i]" is "list_of_op[i]" hold in tmpl_parms.
    */
   tree_nodeRef tmpl_args;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(type_decl)

   /// Redefinition of get_kind.
   GET_KIND(type_decl)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(decl_node) = 0,
      GETID(tmpl_parms),
      GETID(tmpl_args)
   };
};

/**
 * struct definition of the typename_type tree node.
 * The tree walker structure of this node is:
 * #(TOK_TYPENAME_TYPE type_node)
 */
CREATE_TREE_NODE_CLASS(typename_type, type_node);

/**
 * This struct specifies the uneq_expr node.
 * This is equivalent to unordered or ...
 */
CREATE_TREE_NODE_CLASS(uneq_expr, binary_expr);

/**
 * This struct specifies the unge_expr node.
 * This is equivalent to unordered or ...
 */
CREATE_TREE_NODE_CLASS(unge_expr, binary_expr);

/**
 * This struct specifies the ungt_expr node.
 * This is equivalent to unordered or ...
 */
CREATE_TREE_NODE_CLASS(ungt_expr, binary_expr);

/**
 * struct definition of the union_type tree node.
 * The tree walker structure of this node is:
 * #(TOK_UNION_TYPE type_node TOK_UNION (flds)? (fncs)? (binf)?);
 */
struct union_type : public type_node
{
   /// constructor
   explicit union_type(unsigned int i) : type_node(i)
   {
   }

   /*type_node fields are in the parent class*/

   /**
    * list_of_flds is a chain of field_decl for the fields of the struct,
    * and var_decl, type_decl and const_decl for record-scope variables, types and enumerators.
    * (macro  TYPE_FIELDS)
    */
   std::vector<tree_nodeRef> list_of_flds;

   /**
    * list_of_fncs is a chain of methods_decl for the fields of the struct.
    * (macro TYPE_METHODS)
    */
   std::vector<tree_nodeRef> list_of_fncs;

   /**
    * binf field are information about this type, as a base type for itself.
    * It is a binfo node.
    * (macro TYPE_BINFO)
    */
   tree_nodeRef binf;

   /**
    * Add a field_decl to list of flds.
    * @param a is a NODE_ID.
    */
   void add_flds(const tree_nodeRef& a)
   {
      list_of_flds.push_back(a);
   }

   /**
    * Add a methods_decl to list of fncs.
    * @param a is a NODE_ID.
    */
   void add_fncs(const tree_nodeRef& a)
   {
      list_of_fncs.push_back(a);
   }

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(union_type)

   /// Redefinition of get_kind.
   GET_KIND(union_type)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(type_node) = 0,
      GETID(list_of_flds),
      GETID(list_of_fncs),
      GETID(binf)
   };
};

/**
 * This struct specifies the unle_expr node.
 * This is equivalent to unordered or ...
 */
CREATE_TREE_NODE_CLASS(unle_expr, binary_expr);

/**
 * This struct specifies the unlt_expr node.
 * This is equivalent to unordered or ...
 */
CREATE_TREE_NODE_CLASS(unlt_expr, binary_expr);

/**
 * This struct specifies the unordered_expr node.
 * Additional relational operator for floating point unordered.
 */
CREATE_TREE_NODE_CLASS(unordered_expr, binary_expr);

/**
 * This struct specifies the unsave_expr node.
 * The operand is the value to unsave.  By unsave, we
 * mean that all _EXPRs such as TARGET_EXPRs, SAVE_EXPRs,
 * CALL_EXPRs and RTL_EXPRs, that are protected
 * from being evaluated more than once should be reset so that a new
 * expand_expr call of this expr will cause those to be re-evaluated.
 * This is useful when we want to reuse a tree in different places,
 * but where we must re-expand.
 */
CREATE_TREE_NODE_CLASS(unsave_expr, unary_expr);

/**
 * A using declaration.  USING_DECL_SCOPE contains the specified
 *  scope.  In a member using decl, unless DECL_DEPENDENT_P is true,
 * USING_DECL_DECLS contains the _DECL or OVERLOAD so named.  This is
 * not an alias, but is later expanded into multiple aliases.
 */
CREATE_TREE_NODE_CLASS(using_decl, decl_node);

/**
 * This struct specifies the va_arg_expr node.
 * Used to implement `va_arg'.
 */
CREATE_TREE_NODE_CLASS(va_arg_expr, unary_expr);

/**
 * struct definition of the label_decl tree node.
 * The tree walker structure of this node is:
 * #(TOK_VAR_DECL decl_node attr (TOK_STATIC TOK_STATIC | TOK_STATIC | TOK_EXTERN)? (init)? (size)? algn used (TOK_REGISTER)?)
 */
struct var_decl : public decl_node, public attr
{
   /// constructor
   explicit var_decl(unsigned int i);

   /*decl_node fields are in the parent class: decl_node*/

   /** use_tmpl indicates whether or not (and how) a template was expanded for this VAR_DECL.
    * 0=normal declaration, e.g. int min (int, int);
    * 1=implicit template instantiation
    * 2=explicit template specialization, e.g. int min<int> (int, int);
    * 3=explicit template instantiation, e.g. template int min<int> (int, int);
    */
   int use_tmpl;

   /// to manage C++ code with static member
   bool static_static_flag;

   /// to manage standard static attribute
   bool static_flag;

   /// a variable can be extern
   bool extern_flag;

   /// True when we are able to prove that its address
   /// is taken and escape from a the function in which is defined.
   /// Defined by LLVM/CLANG and it refers mainly to alloca type variables
   bool addr_taken;

   /// True when we are able to prove that its address is not taken and do not escape.
   /// It is defined by LLVM/CLANG and it refers mainly to alloca type variables
   bool addr_not_taken;

   /*attr fields are in the parent class: attr*/

   /**
    * init field holds the value to initialize a variable to.
    * (macro DECL_INITIAL)
    */
   tree_nodeRef init;

   /**
    * size field holds the size of datum, in bits.
    * (macro DECL_SIZE)
    */
   tree_nodeRef size;

   /**
    * algn field holds the alignment required for the datum, in bits.
    * (macro DECL_ALIGN)
    */
   unsigned int algn;

   /**
    * used is nonzero if the name is used in its scope
    * (macro TREE_USED)
    */
   int used;

   /**
    * register_flag means declared 'register'
    * (macro DECL_REGISTER)
    */
   bool register_flag;

   /**
    * readonly_flag means readonly parameter
    * (macro TREE_READONLY)
    */
   bool readonly_flag;

   /// for each bit of the var_decl tells if it is equal to U,X,0,1
   /// meaningful only in case the variable is readonly
   std::string bit_values;

   /**
    * symbol_memory_tag annotation
    */
   tree_nodeRef smt_ann;

   /// PointToInformation associated with this ssa_name if the corresponding variable is a pointer
   const PointToInformationRef point_to_information;

   /// The set of gimple node which writes this variable
   CustomUnorderedSet<tree_nodeRef> defs;

   /// The set of gimple node which read this variable
   CustomUnorderedSet<tree_nodeRef> uses;

   /// The set of gimple node which addresses this variable
   CustomUnorderedSet<tree_nodeRef> addressings;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(var_decl)

   /// Redefinition of get_kind.
   GET_KIND(var_decl)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(decl_node) = 0,
      GETID(attr),
      GETID(init),
      GETID(size),
      GETID(smt_ann)
   };
};

/**
 * Widening dot-product.
 * The first two arguments are of type t1.
 * The third argument and the result are of type t2, such that t2 is at least
 * twice the size of t1. DOT_PROD_EXPR(arg1,arg2,arg3) is equivalent to:
 * 	tmp = WIDEN_MULT_EXPR(arg1, arg2);
 * 	arg3 = PLUS_EXPR (tmp, arg3);
 * or:
 * tmp = WIDEN_MULT_EXPR(arg1, arg2);
 *      arg3 = WIDEN_SUM_EXPR (tmp, arg3);
 */
CREATE_TREE_NODE_CLASS(dot_prod_expr, ternary_expr);

/**
 * Vector conditional expression. It is like COND_EXPR, but with
 * vector operands.
 *
 * A = VEC_COND_EXPR ( X < Y, B, C)
 *
 * means
 *
 * for (i=0; i<N; i++)
 *   A[i] = X[i] < Y[i] ? B[i] : C[i];
 */
CREATE_TREE_NODE_CLASS(vec_cond_expr, ternary_expr);

/**
 * Vector permutation expression.  A = VEC_PERM_EXPR<v0, v1, mask> means
 *
 * N = length(mask)
 * foreach i in N:
 *   M = mask[i] % (2*N)
 *   A = M < N ? v0[M] : v1[M-N]
 *
 * V0 and V1 are vectors of the same type.  MASK is an integer-typed
 * vector.  The number of MASK elements must be the same with the
 * number of elements in V0 and V1.  The size of the inner type
 * of the MASK and of the V0 and V1 must be the same.
 */
CREATE_TREE_NODE_CLASS(vec_perm_expr, ternary_expr);

/**
 * Whole vector left/right shift in bits.
 * Operand 0 is a vector to be shifted.
 * Operand 1 is an integer shift amount in bits.
 */
CREATE_TREE_NODE_CLASS(vec_lshift_expr, binary_expr);
CREATE_TREE_NODE_CLASS(vec_rshift_expr, binary_expr);

/**
 * Widening vector multiplication.
 * The two operands are vectors with N elements of size S. Multiplying the
 * elements of the two vectors will result in N products of size 2*S.
 * VEC_WIDEN_MULT_HI_EXPR computes the N/2 high products.
 * VEC_WIDEN_MULT_LO_EXPR computes the N/2 low products.
 */
CREATE_TREE_NODE_CLASS(widen_mult_hi_expr, binary_expr);
CREATE_TREE_NODE_CLASS(widen_mult_lo_expr, binary_expr);

/**
 * Unpack (extract and promote/widen) the high/low elements of the input
 * vector into the output vector.  The input vector has twice as many
 * elements as the output vector, that are half the size of the elements
 * of the output vector.  This is used to support type promotion.
 */
CREATE_TREE_NODE_CLASS(vec_unpack_hi_expr, unary_expr);
CREATE_TREE_NODE_CLASS(vec_unpack_lo_expr, unary_expr);

/**
 * Unpack (extract) the high/low elements of the input vector, convert
 * fixed point values to floating point and widen elements into the
 * output vector.  The input vector has twice as many elements as the output
 * vector, that are half the size of the elements of the output vector.
 */
CREATE_TREE_NODE_CLASS(vec_unpack_float_hi_expr, unary_expr);
CREATE_TREE_NODE_CLASS(vec_unpack_float_lo_expr, unary_expr);

/**
 * Pack (demote/narrow and merge) the elements of the two input vectors
 * into the output vector using truncation/saturation.
 * The elements of the input vectors are twice the size of the elements of the
 * output vector.  This is used to support type demotion.
 */
CREATE_TREE_NODE_CLASS(vec_pack_trunc_expr, binary_expr);
CREATE_TREE_NODE_CLASS(vec_pack_sat_expr, binary_expr);

/**
 * Convert floating point values of the two input vectors to integer
 * and pack (narrow and merge) the elements into the output vector. The
 * elements of the input vector are twice the size of the elements of
 * the output vector.
 */
CREATE_TREE_NODE_CLASS(vec_pack_fix_trunc_expr, binary_expr);

/**
 * Extract even/odd fields from vectors.
 */
CREATE_TREE_NODE_CLASS(vec_extracteven_expr, binary_expr);
CREATE_TREE_NODE_CLASS(vec_extractodd_expr, binary_expr);

/**
 * Merge input vectors interleaving their fields.
 */
CREATE_TREE_NODE_CLASS(vec_interleavehigh_expr, binary_expr);
CREATE_TREE_NODE_CLASS(vec_interleavelow_expr, binary_expr);

/**
 * This struct represents a vector new expression
 */
CREATE_TREE_NODE_CLASS(vec_new_expr, expr_node);

/**
 * This struct specifies the vector_cst node.
 * Contents are in TREE_VECTOR_CST_ELTS field.
 * The tree walker structure of this node is:
 * #(TOK_VECTOR_CST type (valu)+)
 */
struct vector_cst : public cst_node
{
   /// constructor
   explicit vector_cst(unsigned int i) : cst_node(i)
   {
   }

   /// list_of_valu is a list of value of the TREE_VECTOR_CST_ELTS vector elements.
   std::vector<tree_nodeRef> list_of_valu;

   /**
    * Add a value to list of value.
    * @param a is a NODE_ID.
    */
   void add_valu(const tree_nodeRef& a)
   {
      list_of_valu.push_back(a);
   }

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(vector_cst)

   /// Redefinition of get_kind.
   GET_KIND(vector_cst)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(cst_node) = 0,
      GETID(list_of_valu)
   };
};

/**
 * VOID_CST node
 */
CREATE_TREE_NODE_CLASS(void_cst, cst_node);

/**
 * struct definition of the vector_type tree node.
 * The tree walker structure of this node is:
 * #(TOK_VECTOR_TYPE type_node elts)
 */

struct vector_type : public type_node
{
   /// constructor
   explicit vector_type(unsigned int i) : type_node(i)
   {
   }

   /// field elts is the type of an vector element (tree-dump.c use the macro TREE_TYPE)
   tree_nodeRef elts;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(vector_type)

   /// Redefinition of get_kind.
   GET_KIND(vector_type)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

   /// visitor enum
   enum
   {
      GETID(type_node) = 0,
      GETID(elts)
   };
};

/**
 * This struct specifies the view_convert_expr node.
 * Represents viewing something of one type as being of a second type. This corresponds to an "Unchecked Conversion" in Ada and roughly to
 * the idiom *(type2 *)&X in C.  The only operand is the value to be viewed as being of another type.  It is undefined if the type of the
 * input and of the expression have different sizes.
 *
 * This code may also be used within the LHS of a MODIFY_EXPR, in which case no actual data motion may occur.  TREE_ADDRESSABLE will be set in
 * this case and GCC must abort if it could not do the operation without generating insns.
 */
CREATE_TREE_NODE_CLASS(view_convert_expr, unary_expr);

/**
 * struct definition of the void_type tree node.
 * The tree walker structure of this node is:
 * #(TOK_VOID_TYPE type_node);
 */
CREATE_TREE_NODE_CLASS(void_type, type_node);

/**
 * This struct specifies the vtable_ref node.
 * Vtable indexing.  Carries data useful for emitting information for vtable garbage collection.
 * Operand 0: an array_ref (or equivalent expression)
 * Operand 1: the vtable base (must be a var_decl)
 * Operand 2: index into vtable (must be an integer_cst).
 */
CREATE_TREE_NODE_CLASS(vtable_ref, ternary_expr);

/**
 * This struct specifies the with_cleanup_expr node.
 * Specify a value to compute along with its corresponding cleanup.
 * Operand 0 argument is an expression whose value needs a cleanup.
 * Operand 1 is the cleanup expression for the object.
 * Operand 2 is an RTL_EXPR which will eventually represent that value. The RTL_EXPR is used in this expression, which is how the expression manages to act on the proper value.
 * The cleanup is executed by the first enclosing CLEANUP_POINT_EXPR, if it exists, otherwise it is the responsibility of the caller to manually
 * call expand_start_target_temps/expand_end_target_temps, as needed.
 * This differs from TRY_CATCH_EXPR in that operand 2 is always evaluated when an exception isn't thrown when cleanups are run.
 */
CREATE_TREE_NODE_CLASS(with_cleanup_expr, ternary_expr);

/**
 * Records the size for an expression of variable size type.  This is
 * for use in contexts in which we are accessing the entire object,
 * such as for a function call, or block copy.
 * Operand 0 is the real expression.
 * Operand 1 is the size of the type in the expression.
 */
CREATE_TREE_NODE_CLASS(with_size_expr, binary_expr);

/**
 *  Widening summation.
 * The first argument is of type t1.
 * The second argument is of type t2, such that t2 is at least twice
 * the size of t1. The type of the entire expression is also t2.
 * WIDEN_SUM_EXPR is equivalent to first widening (promoting)
 * the first argument from type t1 to type t2, and then summing it
 * with the second argument.
 */
CREATE_TREE_NODE_CLASS(widen_sum_expr, binary_expr);

/**
 * Widening multiplication.
 * The two arguments are of type t1.
 * The result is of type t2, such that t2 is at least twice
 * the size of t1. WIDEN_MULT_EXPR is equivalent to first widening (promoting)
 * the arguments from type t1 to type t2, and then multiplying them.
 */
CREATE_TREE_NODE_CLASS(widen_mult_expr, binary_expr);

/**
 * This struct specifies the lut_expr node.
 *
 */
struct lut_expr : public expr_node
{
   /// constructor
   explicit lut_expr(unsigned int i) : expr_node(i)
   {
   }

   /// true table constant
   tree_nodeRef op0;

   /// first operand
   tree_nodeRef op1;

   /// second operand
   tree_nodeRef op2;

   /// third operand
   tree_nodeRef op3;

   /// fourth operand
   tree_nodeRef op4;

   /// fifth operand
   tree_nodeRef op5;

   /// sixth operand
   tree_nodeRef op6;

   /// seventh operand
   tree_nodeRef op7;

   /// eighth operand
   tree_nodeRef op8;

   /// Redefinition of get_kind_text.
   GET_KIND_TEXT(lut_expr)

   /// Redefinition of get_kind.
   GET_KIND(lut_expr)

   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;

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
 * @brief extract_bit_expr extracts a bit value from a ssa/integer const
 * op0 is the ssa variable
 * op1 is the bit position
 * return a boolean value
 */
CREATE_TREE_NODE_CLASS(extract_bit_expr, binary_expr);

#endif
