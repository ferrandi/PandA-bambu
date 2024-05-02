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
 * @file tree_node.cpp
 * @brief Class implementation of the tree_node structures.
 *
 * This file implements some of the tree_node member functions and define a global variable.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "tree_node.hpp"

#include "dbgPrintHelper.hpp"
#include "gimple_writer.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_reindex.hpp"

#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#include <iostream>
#include <ostream>
#include <unordered_map>
#include <utility>

#include "config_HAVE_ASSERTS.hpp"
#include "config_HAVE_TREE_MANIPULATION_BUILT.hpp"
#include "config_HAVE_TREE_PARSER_BUILT.hpp"

// #define CHECK_VIRTUAL_USES

/// forward declaration macro
#define VISIT_TREE_NODE_MACRO(r, data, elem)          \
   void elem::visit(tree_node_visitor* const v) const \
   {                                                  \
      unsigned int mask = ALL_VISIT;                  \
      (*v)(this, mask);                               \
      VISIT_SC(mask, data, visit(v));                 \
   }

enum kind tree_node::get_kind(const std::string& input_name)
{
   static std::unordered_map<std::string, enum kind> to_kind = []() {
      std::unordered_map<std::string, enum kind> out;
      std::string name;

#define NAME_KIND(r, data, elem)                                                    \
   name = #elem;                                                                    \
   name = name.substr(19);                                                          \
   name = name.substr(name.front() == ' ', name.find(')') - (name.front() == ' ')); \
   out[name] = BOOST_PP_CAT(elem, _K);

      BOOST_PP_SEQ_FOR_EACH(NAME_KIND, BOOST_PP_EMPTY, BINARY_EXPRESSION_TREE_NODES);
      BOOST_PP_SEQ_FOR_EACH(NAME_KIND, BOOST_PP_EMPTY, CONST_OBJ_TREE_NODES);
      BOOST_PP_SEQ_FOR_EACH(NAME_KIND, BOOST_PP_EMPTY, CPP_STMT_NODES);
      BOOST_PP_SEQ_FOR_EACH(NAME_KIND, BOOST_PP_EMPTY, DECL_NODE_TREE_NODES);
      BOOST_PP_SEQ_FOR_EACH(NAME_KIND, BOOST_PP_EMPTY, GIMPLE_NODES);
      BOOST_PP_SEQ_FOR_EACH(NAME_KIND, BOOST_PP_EMPTY, MISCELLANEOUS_EXPR_TREE_NODES);
      BOOST_PP_SEQ_FOR_EACH(NAME_KIND, BOOST_PP_EMPTY, MISCELLANEOUS_OBJ_TREE_NODES);
      BOOST_PP_SEQ_FOR_EACH(NAME_KIND, BOOST_PP_EMPTY, PANDA_EXTENSION_TREE_NODES);
      BOOST_PP_SEQ_FOR_EACH(NAME_KIND, BOOST_PP_EMPTY, QUATERNARY_EXPRESSION_TREE_NODES);
      BOOST_PP_SEQ_FOR_EACH(NAME_KIND, BOOST_PP_EMPTY, TERNARY_EXPRESSION_TREE_NODES);
      BOOST_PP_SEQ_FOR_EACH(NAME_KIND, BOOST_PP_EMPTY, TYPE_NODE_TREE_NODES);
      BOOST_PP_SEQ_FOR_EACH(NAME_KIND, BOOST_PP_EMPTY, UNARY_EXPRESSION_TREE_NODES(last_tree));
#if HAVE_ASSERTS
      for(const auto& sk : out)
      {
         THROW_ASSERT(sk.first.find(' ') == std::string::npos,
                      "Kind name string should not contain spaces: '" + sk.first + "'");
      }
#endif
      return out;
   }();
   return to_kind[input_name];
}

std::string tree_node::GetString(enum kind k)
{
   static std::unordered_map<enum kind, std::string> to_string = []() {
      std::unordered_map<enum kind, std::string> out;
      std::string name;

#define KIND_NAME(r, data, elem)                                                    \
   name = #elem;                                                                    \
   name = name.substr(19);                                                          \
   name = name.substr(name.front() == ' ', name.find(')') - (name.front() == ' ')); \
   out[BOOST_PP_CAT(elem, _K)] = name;

      BOOST_PP_SEQ_FOR_EACH(KIND_NAME, BOOST_PP_EMPTY, BINARY_EXPRESSION_TREE_NODES);
      BOOST_PP_SEQ_FOR_EACH(KIND_NAME, BOOST_PP_EMPTY, CONST_OBJ_TREE_NODES);
      BOOST_PP_SEQ_FOR_EACH(KIND_NAME, BOOST_PP_EMPTY, CPP_STMT_NODES);
      BOOST_PP_SEQ_FOR_EACH(KIND_NAME, BOOST_PP_EMPTY, DECL_NODE_TREE_NODES);
      BOOST_PP_SEQ_FOR_EACH(KIND_NAME, BOOST_PP_EMPTY, GIMPLE_NODES);
      BOOST_PP_SEQ_FOR_EACH(KIND_NAME, BOOST_PP_EMPTY, MISCELLANEOUS_EXPR_TREE_NODES);
      BOOST_PP_SEQ_FOR_EACH(KIND_NAME, BOOST_PP_EMPTY, MISCELLANEOUS_OBJ_TREE_NODES);
      BOOST_PP_SEQ_FOR_EACH(KIND_NAME, BOOST_PP_EMPTY, PANDA_EXTENSION_TREE_NODES);
      BOOST_PP_SEQ_FOR_EACH(KIND_NAME, BOOST_PP_EMPTY, QUATERNARY_EXPRESSION_TREE_NODES);
      BOOST_PP_SEQ_FOR_EACH(KIND_NAME, BOOST_PP_EMPTY, TERNARY_EXPRESSION_TREE_NODES);
      BOOST_PP_SEQ_FOR_EACH(KIND_NAME, BOOST_PP_EMPTY, TYPE_NODE_TREE_NODES);
      BOOST_PP_SEQ_FOR_EACH(KIND_NAME, BOOST_PP_EMPTY, UNARY_EXPRESSION_TREE_NODES(last_tree))
#if HAVE_ASSERTS
      for(const auto& ks : out)
      {
         THROW_ASSERT(ks.second.find(' ') == std::string::npos,
                      "Kind name string should not contain spaces: '" + ks.second + "'");
      }
#endif

      return out;
   }();
   return to_string[k];
}

BOOST_PP_SEQ_FOR_EACH(VISIT_TREE_NODE_MACRO, unary_expr, UNARY_EXPRESSION_TREE_NODES)
BOOST_PP_SEQ_FOR_EACH(VISIT_TREE_NODE_MACRO, binary_expr, BINARY_EXPRESSION_TREE_NODES)
BOOST_PP_SEQ_FOR_EACH(VISIT_TREE_NODE_MACRO, ternary_expr, TERNARY_EXPRESSION_TREE_NODES)
BOOST_PP_SEQ_FOR_EACH(VISIT_TREE_NODE_MACRO, quaternary_expr, QUATERNARY_EXPRESSION_TREE_NODES)
BOOST_PP_SEQ_FOR_EACH(VISIT_TREE_NODE_MACRO, cst_node, (void_cst))
BOOST_PP_SEQ_FOR_EACH(VISIT_TREE_NODE_MACRO, tree_node, (ctor_initializer)(trait_expr))
BOOST_PP_SEQ_FOR_EACH(VISIT_TREE_NODE_MACRO, decl_node, (label_decl)(using_decl)(translation_unit_decl))
BOOST_PP_SEQ_FOR_EACH(VISIT_TREE_NODE_MACRO, expr_node,
                      (modop_expr)(new_expr)(placeholder_expr)(template_id_expr)(vec_new_expr))
BOOST_PP_SEQ_FOR_EACH(VISIT_TREE_NODE_MACRO, type_node,
                      (boolean_type)(CharType)(nullptr_type)(lang_type)(offset_type)(qual_union_type)(set_type)(
                          template_type_parm)(typename_type)(void_type))
#undef VISIT_TREE_NODE_MACRO

void tree_node::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
}

std::string tree_node::ToString() const
{
   std::stringstream temp;
   temp << "@" << index << " ";
   temp << this;
   return temp.str();
}

std::ostream& operator<<(std::ostream& os, const tree_node* tn)
{
   GimpleWriter gimple_writer(os, false);
   tn->visit(&gimple_writer);
   return os;
}

std::ostream& operator<<(std::ostream& os, const tree_nodeRef& tn)
{
   os << tn.get();
   return os;
}

WeightedNode::WeightedNode(unsigned int i) : tree_node(i)
{
}

void WeightedNode::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   //    VISIT_SC(mask, tree_node, visit(v));
}

attr::~attr() = default;

void attr::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
}

bool attr::is_constructor() const
{
   return list_attr.count(TreeVocabularyTokenTypes_TokenEnum::TOK_CONSTRUCTOR);
}

bool attr::is_destructor() const
{
   return list_attr.count(TreeVocabularyTokenTypes_TokenEnum::TOK_DESTRUCTOR);
}

bool attr::is_member() const
{
   return list_attr.count(TreeVocabularyTokenTypes_TokenEnum::TOK_MEMBER);
}

bool attr::is_call() const
{
   return list_attr.count(TreeVocabularyTokenTypes_TokenEnum::TOK_CALL);
}

bool attr::is_new() const
{
   return list_attr.count(TreeVocabularyTokenTypes_TokenEnum::TOK_NEW);
}

bool attr::is_public() const
{
   return list_attr.count(TreeVocabularyTokenTypes_TokenEnum::TOK_PUBLIC);
}

bool attr::is_protected() const
{
   return list_attr.count(TreeVocabularyTokenTypes_TokenEnum::TOK_PROTECTED);
}

bool attr::is_private() const
{
   return list_attr.count(TreeVocabularyTokenTypes_TokenEnum::TOK_PRIVATE);
}

bool attr::is_bitfield() const
{
   return list_attr.count(TreeVocabularyTokenTypes_TokenEnum::TOK_BITFIELD);
}

srcp::~srcp() = default;

void srcp::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
}

decl_node::decl_node(unsigned int i)
    : tree_node(i),
      artificial_flag(false),
      packed_flag(false),
      operating_system_flag(false),
      library_system_flag(false),
      libbambu_flag(false),
      C_flag(false),
      uid(0)
{
}

void decl_node::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, tree_node, visit(v));
   VISIT_SC(mask, srcp, visit(v));
   VISIT_MEMBER(mask, name, visit(v));
   VISIT_MEMBER(mask, mngl, visit(v));
   VISIT_MEMBER(mask, orig, visit(v));
   VISIT_MEMBER(mask, type, visit(v));
   VISIT_MEMBER(mask, scpe, visit(v));
   VISIT_MEMBER(mask, attributes, visit(v));
   VISIT_MEMBER(mask, chan, visit(v));
}

void expr_node::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, WeightedNode, visit(v));
   VISIT_SC(mask, srcp, visit(v));
   VISIT_MEMBER(mask, type, visit(v));
}

gimple_node::gimple_node(unsigned int i)
    : WeightedNode(i),
      use_set(new PointToSolution()),
      clobbered_set(new PointToSolution()),
      bb_index(0),
      artificial(false),
      keep(false)
{
}

void gimple_node::SetVdef(const tree_nodeRef& _vdef)
{
   THROW_ASSERT(!GET_CONST_PTD_NODE(_vdef) ||
                    (_vdef->get_kind() == ssa_name_K && GetPointerS<const ssa_name>(_vdef)->virtual_flag),
                "");
   vdef = _vdef;
}

bool gimple_node::AddVuse(const tree_nodeRef& vuse)
{
   THROW_ASSERT(!GET_CONST_PTD_NODE(vuse) ||
                    (vuse->get_kind() == ssa_name_K && GetPointerS<const ssa_name>(vuse)->virtual_flag),
                "");
   return vuses.insert(vuse).second;
}

bool gimple_node::AddVover(const tree_nodeRef& vover)
{
   THROW_ASSERT(!GET_CONST_PTD_NODE(vover) ||
                    (vover->get_kind() == ssa_name_K && GetPointerS<const ssa_name>(vover)->virtual_flag),
                "");
   return vovers.insert(vover).second;
}

void gimple_node::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, WeightedNode, visit(v));
   VISIT_SC(mask, srcp, visit(v));
   VISIT_MEMBER(mask, memuse, visit(v));
   VISIT_MEMBER(mask, memdef, visit(v));
   SEQ_VISIT_MEMBER(mask, vuses, visit(v));
   VISIT_MEMBER(mask, vdef, visit(v));
   SEQ_VISIT_MEMBER(mask, vovers, visit(v));
   VISIT_MEMBER(mask, use_set, visit(v));
   VISIT_MEMBER(mask, clobbered_set, visit(v));
   VISIT_MEMBER(mask, scpe, visit(v));
}

void unary_expr::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, expr_node, visit(v));
   VISIT_MEMBER(mask, op, visit(v));
}

void binary_expr::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, expr_node, visit(v));
   VISIT_MEMBER(mask, op0, visit(v));
   VISIT_MEMBER(mask, op1, visit(v));
}

void ternary_expr::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, expr_node, visit(v));
   VISIT_MEMBER(mask, op0, visit(v));
   VISIT_MEMBER(mask, op1, visit(v));
   VISIT_MEMBER(mask, op2, visit(v));
}

void quaternary_expr::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, expr_node, visit(v));
   VISIT_MEMBER(mask, op0, visit(v));
   VISIT_MEMBER(mask, op1, visit(v));
   VISIT_MEMBER(mask, op2, visit(v));
   VISIT_MEMBER(mask, op3, visit(v));
}

type_node::type_node(unsigned int i)
    : tree_node(i),
      qual(TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN),
      algn(0),
      packed_flag(false),
      system_flag(false),
      libbambu_flag(false)
{
}

void type_node::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, tree_node, visit(v));
   VISIT_MEMBER(mask, name, visit(v));
   VISIT_MEMBER(mask, unql, visit(v));
   VISIT_MEMBER(mask, size, visit(v));
   VISIT_MEMBER(mask, scpe, visit(v));
}

void cst_node::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, tree_node, visit(v));
   VISIT_MEMBER(mask, type, visit(v));
}

void error_mark::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, tree_node, visit(v));
}

void array_type::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, type_node, visit(v));
   VISIT_MEMBER(mask, elts, visit(v));
   VISIT_MEMBER(mask, domn, visit(v));
}

void gimple_asm::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, gimple_node, visit(v));
   VISIT_MEMBER(mask, out, visit(v));
   VISIT_MEMBER(mask, in, visit(v));
   VISIT_MEMBER(mask, clob, visit(v));
}

void baselink::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, tree_node, visit(v));
   VISIT_MEMBER(mask, type, visit(v));
}

void gimple_bind::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, expr_node, visit(v));
   SEQ_VISIT_MEMBER(mask, list_of_vars, visit(v));
   VISIT_MEMBER(mask, body, visit(v));
}

void binfo::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, tree_node, visit(v));
   VISIT_MEMBER(mask, type, visit(v));
   auto vend = list_of_access_binf.end();
   for(auto i = list_of_access_binf.begin(); i != vend; ++i)
   {
      VISIT_MEMBER_NAMED(list_of_access_binf, mask, i->second, visit(v));
   }
}

void binfo::add_access_binf(const tree_nodeRef& binf, TreeVocabularyTokenTypes_TokenEnum access)
{
   list_of_access_binf.emplace_back(access, binf);
}

void block::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, tree_node, visit(v));
}

PointToSolution::PointToSolution() : anything(false), escaped(false), ipa_escaped(false), nonlocal(false), null(false)
{
}

PointToSolution::~PointToSolution() = default;

void PointToSolution::Add(const std::string& variable)
{
   if(variable == "anything")
   {
      anything = true;
   }
   else if(variable == "escaped")
   {
      escaped = true;
   }
   else if(variable == "ipa_escaped")
   {
      ipa_escaped = true;
   }
   else if(variable == "nonlocal")
   {
      nonlocal = true;
   }
   else if(variable == "null")
   {
      null = true;
   }
   else
   {
      THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "Symbolic variable " + variable + " of point to set unknown");
   }
}

void PointToSolution::Add(const tree_nodeRef& variable)
{
   variables.push_back(variable);
}

bool PointToSolution::is_a_singleton() const
{
   return !anything && !escaped && !ipa_escaped && !nonlocal && variables.size() == 1;
}

bool PointToSolution::is_fully_resolved() const
{
   return !anything && !escaped && !ipa_escaped && !nonlocal && !variables.empty();
}

std::string PointToSolution::ToString() const
{
   std::string res;
   if(anything)
   {
      res += "anything ";
   }
   if(escaped)
   {
      res += "escaped ";
   }
   if(ipa_escaped)
   {
      res += "ipa_escaped ";
   }
   if(nonlocal)
   {
      res += "nonlocal ";
   }
   if(null)
   {
      res += "null ";
   }
   for(const auto& var : variables)
   {
      res += var->ToString() + " ";
   }
   return res;
}

void PointToSolution::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   SEQ_VISIT_MEMBER(mask, variables, visit(v));
}

void call_expr::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, expr_node, visit(v));
   VISIT_MEMBER(mask, fn, visit(v));
   SEQ_VISIT_MEMBER(mask, args, visit(v));
}

call_expr::call_expr(const unsigned int i) : expr_node(i)
{
}

void call_expr::AddArg(const tree_nodeRef& arg)
{
   this->args.push_back(arg);
}

void aggr_init_expr::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, call_expr, visit(v));
   VISIT_MEMBER(mask, slot, visit(v));
}

aggr_init_expr::aggr_init_expr(const unsigned int i) : call_expr(i), ctor(0)
{
}

gimple_call::gimple_call(const unsigned int i) : gimple_node(i)
{
}

void gimple_call::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, gimple_node, visit(v));
   VISIT_MEMBER(mask, fn, visit(v));
   SEQ_VISIT_MEMBER(mask, args, visit(v));
}

void gimple_call::AddArg(const tree_nodeRef& arg)
{
   this->args.push_back(arg);
}

void case_label_expr::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, expr_node, visit(v));
   VISIT_MEMBER(mask, op0, visit(v));
   VISIT_MEMBER(mask, op1, visit(v));
   VISIT_MEMBER(mask, got, visit(v));
}

void cast_expr::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, expr_node, visit(v));
   VISIT_MEMBER(mask, op, visit(v));
}

void complex_cst::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, cst_node, visit(v));
   VISIT_MEMBER(mask, real, visit(v));
   VISIT_MEMBER(mask, imag, visit(v));
}

void complex_type::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, type_node, visit(v));
}

void gimple_cond::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, gimple_node, visit(v));
   VISIT_MEMBER(mask, op0, visit(v));
}

void const_decl::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, decl_node, visit(v));
   VISIT_MEMBER(mask, cnst, visit(v));
}

void constructor::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_MEMBER(mask, type, visit(v));
   auto vend = list_of_idx_valu.end();
   for(auto i = list_of_idx_valu.begin(); i != vend; ++i)
   {
      VISIT_MEMBER_NAMED(list_of_idx_valu, mask, i->first, visit(v));
      VISIT_MEMBER_NAMED(list_of_idx_valu, mask, i->second, visit(v));
   }
}

void enumeral_type::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, type_node, visit(v));
   VISIT_MEMBER(mask, min, visit(v));
   VISIT_MEMBER(mask, max, visit(v));
   VISIT_MEMBER(mask, csts, visit(v));
}

void expr_stmt::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, tree_node, visit(v));
   VISIT_MEMBER(mask, expr, visit(v));
   VISIT_MEMBER(mask, next, visit(v));
}

integer_cst_t field_decl::offset()
{
   if(bpos)
   {
      return tree_helper::GetConstValue(bpos);
   }
   return 0;
}

void field_decl::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, decl_node, visit(v));
   VISIT_SC(mask, attr, visit(v));
   VISIT_MEMBER(mask, init, visit(v));
   VISIT_MEMBER(mask, size, visit(v));
   VISIT_MEMBER(mask, bpos, visit(v));
   VISIT_MEMBER(mask, smt_ann, visit(v));
}

function_decl::function_decl(unsigned int i)
    : decl_node(i),
      attr(),
      operator_flag(false),
      fixd_flag(false),
      virt_flag(false),
      reverse_restrict_flag(false),
      writing_memory(false),
      reading_memory(false),
      pipeline_enabled(false),
      simple_pipeline(false),
      initiation_time(1),
#if HAVE_FROM_PRAGMA_BUILT
      omp_for_wrapper(0),
      omp_body_loop(false),
      omp_critical(""),
      omp_atomic(false),
#endif
      fixd(0),
      virt(0),
      undefined_flag(false),
      builtin_flag(false),
      hwcall_flag(false),
      static_flag(false)
{
}

void function_decl::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, decl_node, visit(v));
   VISIT_SC(mask, attr, visit(v));
   VISIT_MEMBER(mask, fn, visit(v));
   VISIT_MEMBER(mask, tmpl_parms, visit(v));
   VISIT_MEMBER(mask, tmpl_args, visit(v));
   SEQ_VISIT_MEMBER(mask, list_of_args, visit(v));
   VISIT_MEMBER(mask, body, visit(v));
   VISIT_MEMBER(mask, inline_body, visit(v));
}

void function_decl::AddArg(const tree_nodeRef& a)
{
   list_of_args.push_back(a);
}

bool function_decl::is_constructor()
{
   return attr::is_constructor();
}
bool function_decl::is_destructor()
{
   return attr::is_destructor();
}

bool function_decl::is_operator() const
{
   return operator_flag;
}

bool function_decl::is_public()
{
   return attr::is_public();
}

bool function_decl::is_private()
{
   return attr::is_private();
}

bool function_decl::is_protected()
{
   return attr::is_protected();
}

bool function_decl::is_pipelined()
{
   return pipeline_enabled;
}

void function_decl::set_pipelining(bool v)
{
   pipeline_enabled = v;
}

bool function_decl::is_simple_pipeline()
{
   return simple_pipeline;
}

void function_decl::set_simple_pipeline(bool v)
{
   simple_pipeline = v;
}

int function_decl::get_initiation_time()
{
   return initiation_time;
}

void function_decl::set_initiation_time(int time)
{
   initiation_time = time;
}

void function_type::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, type_node, visit(v));
   VISIT_MEMBER(mask, retn, visit(v));
   VISIT_MEMBER(mask, prms, visit(v));
}

gimple_assign::gimple_assign(unsigned int i)
    : gimple_node(i), init_assignment(false), clobber(false), temporary_address(false)
{
}

void gimple_assign::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, gimple_node, visit(v));
   VISIT_MEMBER(mask, op0, visit(v));
   VISIT_MEMBER(mask, op1, visit(v));
   VISIT_MEMBER(mask, predicate, visit(v));
}

void gimple_nop::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, gimple_node, visit(v));
}

void gimple_goto::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, gimple_node, visit(v));
   VISIT_MEMBER(mask, op, visit(v));
}

void handler::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, tree_node, visit(v));
   VISIT_MEMBER(mask, body, visit(v));
}

#if HAVE_TREE_MANIPULATION_BUILT
identifier_node::identifier_node(unsigned int node_id, std::string _strg, tree_manager* TM)
    : tree_node(node_id), operator_flag(false), strg(std::move(_strg))
{
   TM->add_identifier_node(node_id, strg);
}

identifier_node::identifier_node(unsigned int node_id, bool _operator_flag, tree_manager* TM)
    : tree_node(node_id), operator_flag(_operator_flag)
{
   TM->add_identifier_node(node_id, operator_flag);
}
#else
identifier_node::identifier_node(unsigned int node_id, const std::string& _strg, tree_manager*)
    : tree_node(node_id), operator_flag(false), strg(_strg)
{
}

identifier_node::identifier_node(unsigned int node_id, bool _operator_flag, tree_manager*)
    : tree_node(node_id), operator_flag(_operator_flag)
{
}
#endif

void identifier_node::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, tree_node, visit(v));
}

void integer_cst::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, cst_node, visit(v));
}

void integer_type::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, type_node, visit(v));
   VISIT_MEMBER(mask, min, visit(v));
   VISIT_MEMBER(mask, max, visit(v));
}

void gimple_label::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, gimple_node, visit(v));
   VISIT_MEMBER(mask, op, visit(v));
}

void method_type::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, function_type, visit(v));
   VISIT_MEMBER(mask, clas, visit(v));
}

void namespace_decl::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, decl_node, visit(v));
   VISIT_MEMBER(mask, dcls, visit(v));
}

void overload::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, tree_node, visit(v));
   VISIT_MEMBER(mask, crnt, visit(v));
   VISIT_MEMBER(mask, chan, visit(v));
}

parm_decl::parm_decl(unsigned int i) : decl_node(i), algn(0), used(0), register_flag(false), readonly_flag(false)
{
}

void parm_decl::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, decl_node, visit(v));
   VISIT_MEMBER(mask, argt, visit(v));
   VISIT_MEMBER(mask, size, visit(v));
   VISIT_MEMBER(mask, smt_ann, visit(v));
}

gimple_phi::gimple_phi(unsigned int i) : gimple_node(i), updated_ssa_uses(false), virtual_flag(false)
{
}

void gimple_phi::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, gimple_node, visit(v));
   VISIT_MEMBER(mask, res, visit(v));
   for(const auto& def_edge : list_of_def_edge)
   {
      VISIT_MEMBER_NAMED(list_of_def_edge, mask, def_edge.first, visit(v));
   }
}

void gimple_phi::AddDefEdge(const tree_managerRef& TM, const DefEdge& def_edge)
{
   list_of_def_edge.push_back(def_edge);
   if(updated_ssa_uses && bb_index != 0)
   {
      const auto sn = GetPointer<ssa_name>(def_edge.first);
      if(sn)
      {
         sn->AddUseStmt(TM->GetTreeNode(index));
      }
   }
}

const gimple_phi::DefEdgeList& gimple_phi::CGetDefEdgesList() const
{
   return list_of_def_edge;
}

void gimple_phi::ReplaceDefEdge(const tree_managerRef& TM, const DefEdge& old_def_edge, const DefEdge& new_def_edge)
{
   for(auto& def_edge : list_of_def_edge)
   {
      if(def_edge == old_def_edge)
      {
         if(updated_ssa_uses && bb_index != 0)
         {
            auto sn = GetPointer<ssa_name>(def_edge.first);
            if(sn)
            {
               sn->RemoveUse(TM->GetTreeNode(index));
            }
         }
         def_edge = new_def_edge;
         if(updated_ssa_uses && bb_index != 0)
         {
            auto sn = GetPointer<ssa_name>(def_edge.first);
            if(sn)
            {
               sn->AddUseStmt(TM->GetTreeNode(index));
            }
         }
         break;
      }
   }
}

void gimple_phi::SetDefEdgeList(const tree_managerRef& TM, DefEdgeList new_list_of_def_edge)
{
   while(!list_of_def_edge.empty())
   {
      RemoveDefEdge(TM, list_of_def_edge.front());
   }
   for(const auto& def_edge : new_list_of_def_edge)
   {
      AddDefEdge(TM, def_edge);
   }
}

void gimple_phi::RemoveDefEdge(const tree_managerRef& TM, const DefEdge& to_be_removed)
{
#if HAVE_ASSERTS
   auto initial_size = list_of_def_edge.size();
#endif
   for(auto def_edge = list_of_def_edge.begin(); def_edge != list_of_def_edge.end(); def_edge++)
   {
      if(*def_edge == to_be_removed)
      {
         if(updated_ssa_uses && bb_index != 0)
         {
            const auto sn = GetPointer<ssa_name>(to_be_removed.first);
            if(sn)
            {
               sn->RemoveUse(TM->GetTreeNode(index));
            }
         }
         list_of_def_edge.erase(def_edge);
         break;
      }
   }
   THROW_ASSERT(list_of_def_edge.size() != initial_size,
                to_be_removed.first->ToString() + "(" + STR(to_be_removed.second) + ") not found in " + ToString());
}

void gimple_phi::SetSSAUsesComputed()
{
   THROW_ASSERT(!updated_ssa_uses, "SSA uses already set as updated");
   updated_ssa_uses = true;
}

gimple_predict::gimple_predict(unsigned int _index) : gimple_node(_index)
{
}

void gimple_predict::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, gimple_node, visit(v));
}

void pointer_type::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, type_node, visit(v));
   VISIT_MEMBER(mask, ptd, visit(v));
}

void real_cst::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, cst_node, visit(v));
}

void real_type::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, type_node, visit(v));
}

void record_type::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, type_node, visit(v));
   VISIT_MEMBER(mask, vfld, visit(v));
   SEQ_VISIT_MEMBER(mask, list_of_flds, visit(v));
   SEQ_VISIT_MEMBER(mask, list_of_fncs, visit(v));
   VISIT_MEMBER(mask, ptd, visit(v));
   VISIT_MEMBER(mask, cls, visit(v));
   VISIT_MEMBER(mask, bfld, visit(v));
   VISIT_MEMBER(mask, binf, visit(v));
   VISIT_MEMBER(mask, tmpl_parms, visit(v));
   VISIT_MEMBER(mask, tmpl_args, visit(v));
}

std::string record_type::get_maybe_name() const
{
   type_decl* td = nullptr;
   if(name)
   {
      td = GetPointer<type_decl>(name);
   }
   if(td)
   {
      identifier_node* in = nullptr;
      if(td->name)
      {
         in = GetPointer<identifier_node>(td->name);
      }
      if(in)
      {
         return in->strg;
      }
   }
   return "#UNKNOWN#";
}

tree_nodeRef record_type::get_field(integer_cst_t offset)
{
   unsigned int i;
   integer_cst_t fld_offset;
   field_decl* fd;
   for(i = 0; i < list_of_flds.size(); i++)
   {
      fd = GetPointer<field_decl>(list_of_flds[i]);
      if(fd)
      {
         fld_offset = fd->offset();
      }
      else
      {
         return tree_nodeRef();
      }
      if(fld_offset == offset)
      {
         return list_of_flds[i];
      }
   }
   return tree_nodeRef();
}

void reference_type::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, type_node, visit(v));
   VISIT_MEMBER(mask, refd, visit(v));
}

void result_decl::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, decl_node, visit(v));
   VISIT_MEMBER(mask, init, visit(v));
   VISIT_MEMBER(mask, size, visit(v));
   VISIT_MEMBER(mask, smt_ann, visit(v));
}

void gimple_resx::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, gimple_node, visit(v));
}

void gimple_return::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, gimple_node, visit(v));
   VISIT_MEMBER(mask, op, visit(v));
}

void return_stmt::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, tree_node, visit(v));
   VISIT_MEMBER(mask, expr, visit(v));
}

void scope_ref::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, expr_node, visit(v));
   VISIT_MEMBER(mask, op0, visit(v));
   VISIT_MEMBER(mask, op1, visit(v));
}

ssa_name::ssa_name(unsigned int i)
    : tree_node(i),
      vers(0),
      orig_vers(0),
      volatile_flag(false),
      virtual_flag(false),
      default_flag(false),
      use_set(new PointToSolution())
{
}

void ssa_name::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, tree_node, visit(v));
   VISIT_MEMBER(mask, type, visit(v));
   VISIT_MEMBER(mask, var, visit(v));
   SEQ_VISIT_MEMBER(mask, def_stmts, visit(v));
   VISIT_MEMBER(mask, min, visit(v));
   VISIT_MEMBER(mask, max, visit(v));
   VISIT_MEMBER(mask, use_set, visit(v));
}

tree_nodeRef ssa_name::CGetDefStmt() const
{
#if HAVE_ASSERTS
   if(def_stmts.size() != 1)
   {
      std::string error_message;
      error_message += "There are " + STR(def_stmts.size()) + " definitions for " + ToString() + ":";
      for(const auto& def_stmt : def_stmts)
      {
         error_message += "\n" + STR(def_stmt);
      }
      THROW_UNREACHABLE(error_message);
   }
#endif
   THROW_ASSERT(def_stmts.size() == 1, "There are " + STR(def_stmts.size()) + " definitions for " + ToString());
   return *(def_stmts.begin());
}

TreeNodeSet ssa_name::CGetDefStmts() const
{
   return def_stmts;
}

void ssa_name::AddUseStmt(const tree_nodeRef& use_stmt)
{
   use_stmts[use_stmt]++;
#ifdef CHECK_VIRTUAL_USES
   if(virtual_flag)
   {
      size_t vuse_count = 0;
      const auto gn = GetPointerS<const gimple_node>(use_stmt);
      vuse_count += gn->vuses.count(tree_nodeRef(this, null_deleter()));
      vuse_count += gn->vovers.count(tree_nodeRef(this, null_deleter()));
      vuse_count += static_cast<size_t>(gn->memuse && gn->memuse->index == index);
      if(use_stmt->get_kind() == gimple_phi_K)
      {
         const auto gp = GetPointerS<const gimple_phi>(use_stmt);
         vuse_count += static_cast<size_t>(
             std::count_if(gp->CGetDefEdgesList().begin(), gp->CGetDefEdgesList().end(),
                           [&](const gimple_phi::DefEdge& de) { return de.first->index == index; }));
      }
      if(use_stmts.count(use_stmt) > vuse_count)
      {
         std::cerr << "vssa: " << ToString() << std::endl;
         const auto gn = GetPointerS<const gimple_node>(use_stmt);
         if(gn->vdef)
         {
            std::cerr << "vdef: " << gn->vdef->ToString() << std::endl;
         }
         for(const auto& vuse : gn->vuses)
         {
            std::cerr << "vuse: " << vuse->ToString() << std::endl;
         }
         for(const auto& vover : gn->vovers)
         {
            std::cerr << "vover: " << vover->ToString() << std::endl;
         }
         if(gn->memdef)
         {
            std::cerr << "memdef: " << gn->memdef->ToString() << std::endl;
         }
         if(gn->memuse)
         {
            std::cerr << "memuse: " << gn->memuse->ToString() << std::endl;
         }
         THROW_UNREACHABLE("Virtual ssa used more than " + STR(vuse_count) + " time - " + use_stmt->ToString());
      }
   }
#endif
}

void ssa_name::AddDefStmt(const tree_nodeRef& def)
{
   def_stmts.insert(def);
}

void ssa_name::SetDefStmt(const tree_nodeRef& def)
{
   def_stmts.clear();
   def_stmts.insert(def);
}

const TreeNodeMap<size_t>& ssa_name::CGetUseStmts() const
{
   return use_stmts;
}

size_t ssa_name::CGetNumberUses() const
{
   size_t ret_value = 0;
   for(const auto& use_stmt : use_stmts)
   {
      ret_value += use_stmt.second;
   }
   return ret_value;
}

void ssa_name::RemoveUse(const tree_nodeRef& use_stmt)
{
#ifndef NDEBUG
   if(use_stmts.find(use_stmt) == use_stmts.end() || use_stmts.find(use_stmt)->second == 0)
   {
      INDENT_DBG_MEX(0, 0, use_stmt->ToString() + " is not in the use_stmts of " + ToString());
      for(const auto& current_use_stmt : use_stmts)
      {
         INDENT_DBG_MEX(0, 0,
                        STR(current_use_stmt.second) + " uses in (" + STR(current_use_stmt.first->index) + ") " +
                            STR(current_use_stmt.first));
      }
      THROW_UNREACHABLE(STR(use_stmt) + " is not in the use statements of " + ToString());
   }
#endif
   use_stmts[use_stmt]--;
   if(use_stmts[use_stmt] == 0)
   {
      use_stmts.erase(use_stmt);
   }
}

void statement_list::add_bloc(const blocRef& a)
{
   list_of_bloc[a->number] = a;
}

void statement_list::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, tree_node, visit(v));
   SEQ_VISIT_MEMBER(mask, list_of_stmt, visit(v));
   auto mend = list_of_bloc.end();
   for(auto i = list_of_bloc.begin(); i != mend; ++i)
   {
      VISIT_MEMBER_NAMED(list_of_bloc, mask, i->second, visit(v));
   }
}

void string_cst::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, cst_node, visit(v));
}

void gimple_switch::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, gimple_node, visit(v));
   VISIT_MEMBER(mask, op0, visit(v));
   VISIT_MEMBER(mask, op1, visit(v));
}

void target_expr::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, expr_node, visit(v));
   VISIT_MEMBER(mask, decl, visit(v));
   VISIT_MEMBER(mask, init, visit(v));
   VISIT_MEMBER(mask, clnp, visit(v));
}

void lut_expr::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, expr_node, visit(v));
   VISIT_MEMBER(mask, op0, visit(v));
   VISIT_MEMBER(mask, op1, visit(v));
   VISIT_MEMBER(mask, op2, visit(v));
   VISIT_MEMBER(mask, op3, visit(v));
   VISIT_MEMBER(mask, op4, visit(v));
   VISIT_MEMBER(mask, op5, visit(v));
   VISIT_MEMBER(mask, op6, visit(v));
   VISIT_MEMBER(mask, op7, visit(v));
   VISIT_MEMBER(mask, op8, visit(v));
}

void template_decl::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, decl_node, visit(v));
   VISIT_MEMBER(mask, rslt, visit(v));
   VISIT_MEMBER(mask, inst, visit(v));
   VISIT_MEMBER(mask, spcs, visit(v));
   VISIT_MEMBER(mask, prms, visit(v));
}

void template_parm_index::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, tree_node, visit(v));
   VISIT_MEMBER(mask, type, visit(v));
   VISIT_MEMBER(mask, decl, visit(v));
}

void tree_list::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_MEMBER(mask, purp, visit(v));
   VISIT_MEMBER(mask, valu, visit(v));
   VISIT_MEMBER(mask, chan, visit(v));
}

void tree_vec::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, tree_node, visit(v));
   SEQ_VISIT_MEMBER(mask, list_of_op, visit(v));
}

void try_block::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, tree_node, visit(v));
   VISIT_MEMBER(mask, body, visit(v));
   VISIT_MEMBER(mask, hdlr, visit(v));
   VISIT_MEMBER(mask, next, visit(v));
}

void type_decl::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, decl_node, visit(v));
   VISIT_MEMBER(mask, tmpl_parms, visit(v));
   VISIT_MEMBER(mask, tmpl_args, visit(v));
}

void union_type::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, type_node, visit(v));
   SEQ_VISIT_MEMBER(mask, list_of_flds, visit(v));
   SEQ_VISIT_MEMBER(mask, list_of_fncs, visit(v));
   VISIT_MEMBER(mask, binf, visit(v));
}

var_decl::var_decl(unsigned int i)
    : decl_node(i),
      use_tmpl(-1),
      static_static_flag(false),
      static_flag(false),
      extern_flag(false),
      addr_taken(false),
      addr_not_taken(false),
      algn(0),
      used(0),
      register_flag(false),
      readonly_flag(false)
{
}

void var_decl::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, decl_node, visit(v));
   VISIT_SC(mask, attr, visit(v));
   VISIT_MEMBER(mask, init, visit(v));
   VISIT_MEMBER(mask, size, visit(v));
   VISIT_MEMBER(mask, smt_ann, visit(v));
}

void vector_cst::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, cst_node, visit(v));
   SEQ_VISIT_MEMBER(mask, list_of_valu, visit(v));
}

void vector_type::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, type_node, visit(v));
   VISIT_MEMBER(mask, elts, visit(v));
}

void target_mem_ref::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, WeightedNode, visit(v));
   VISIT_MEMBER(mask, type, visit(v));
   VISIT_MEMBER(mask, symbol, visit(v));
   VISIT_MEMBER(mask, base, visit(v));
   VISIT_MEMBER(mask, idx, visit(v));
   VISIT_MEMBER(mask, step, visit(v));
   VISIT_MEMBER(mask, offset, visit(v));
   VISIT_MEMBER(mask, orig, visit(v));
   VISIT_MEMBER(mask, tag, visit(v));
}

void target_mem_ref461::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, WeightedNode, visit(v));
   VISIT_MEMBER(mask, type, visit(v));
   VISIT_MEMBER(mask, base, visit(v));
   VISIT_MEMBER(mask, idx, visit(v));
   VISIT_MEMBER(mask, idx2, visit(v));
   VISIT_MEMBER(mask, step, visit(v));
   VISIT_MEMBER(mask, offset, visit(v));
}

void type_argument_pack::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, type_node, visit(v));
   VISIT_MEMBER(mask, arg, visit(v));
}

void nontype_argument_pack::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, expr_node, visit(v));
   VISIT_MEMBER(mask, arg, visit(v));
}

void type_pack_expansion::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, type_node, visit(v));
   VISIT_MEMBER(mask, op, visit(v));
   VISIT_MEMBER(mask, param_packs, visit(v));
   VISIT_MEMBER(mask, arg, visit(v));
}

void expr_pack_expansion::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, expr_node, visit(v));
   VISIT_MEMBER(mask, op, visit(v));
   VISIT_MEMBER(mask, param_packs, visit(v));
   VISIT_MEMBER(mask, arg, visit(v));
}
