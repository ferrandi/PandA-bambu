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
 * @file ir_node.cpp
 * @brief Class implementation of the ir_node structures.
 *
 * This file implements some of the ir_node member functions and define a global variable.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "ir_node.hpp"

#include "dbgPrintHelper.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_reindex.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"

#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#include <iostream>
#include <ostream>
#include <unordered_map>
#include <utility>

#include "config_HAVE_ASSERTS.hpp"
#include "config_HAVE_IR_MANIPULATION_BUILT.hpp"
#include "config_HAVE_IR_PARSER_BUILT.hpp"

// #define CHECK_VIRTUAL_USES

/// forward declaration macro
#define VISIT_IR_NODE_MACRO(r, data, elem)          \
   void elem::visit(ir_node_visitor* const v) const \
   {                                                \
      unsigned int mask = ALL_VISIT;                \
      (*v)(this, mask);                             \
      VISIT_SC(mask, data, visit(v));               \
   }

enum kind ir_node::get_kind(const std::string& input_name)
{
   static std::unordered_map<std::string, enum kind> to_kind = []() {
      std::unordered_map<std::string, enum kind> out;
      std::string name;

#define NAME_KIND(r, data, elem)                                                    \
   name = #elem;                                                                    \
   name = name.substr(19);                                                          \
   name = name.substr(name.front() == ' ', name.find(')') - (name.front() == ' ')); \
   out[name] = BOOST_PP_CAT(elem, _K);

      BOOST_PP_SEQ_FOR_EACH(NAME_KIND, BOOST_PP_EMPTY, BINARY_EXPRESSION_IR_NODES);
      BOOST_PP_SEQ_FOR_EACH(NAME_KIND, BOOST_PP_EMPTY, CONST_OBJ_IR_NODES);
      BOOST_PP_SEQ_FOR_EACH(NAME_KIND, BOOST_PP_EMPTY, DECL_NODE_IR_NODES);
      BOOST_PP_SEQ_FOR_EACH(NAME_KIND, BOOST_PP_EMPTY, NODE_STMTS);
      BOOST_PP_SEQ_FOR_EACH(NAME_KIND, BOOST_PP_EMPTY, MISCELLANEOUS_EXPR_IR_NODES);
      BOOST_PP_SEQ_FOR_EACH(NAME_KIND, BOOST_PP_EMPTY, MISCELLANEOUS_OBJ_IR_NODES);
      BOOST_PP_SEQ_FOR_EACH(NAME_KIND, BOOST_PP_EMPTY, TERNARY_EXPRESSION_IR_NODES);
      BOOST_PP_SEQ_FOR_EACH(NAME_KIND, BOOST_PP_EMPTY, TYPE_NODE_IR_NODES);
      BOOST_PP_SEQ_FOR_EACH(NAME_KIND, BOOST_PP_EMPTY, UNARY_EXPRESSION_IR_NODES(last_ir));
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

std::string ir_node::GetString(enum kind k)
{
   static std::unordered_map<enum kind, std::string> to_string = []() {
      std::unordered_map<enum kind, std::string> out;
      std::string name;

#define KIND_NAME(r, data, elem)                                                    \
   name = #elem;                                                                    \
   name = name.substr(19);                                                          \
   name = name.substr(name.front() == ' ', name.find(')') - (name.front() == ' ')); \
   out[BOOST_PP_CAT(elem, _K)] = name;

      BOOST_PP_SEQ_FOR_EACH(KIND_NAME, BOOST_PP_EMPTY, BINARY_EXPRESSION_IR_NODES);
      BOOST_PP_SEQ_FOR_EACH(KIND_NAME, BOOST_PP_EMPTY, CONST_OBJ_IR_NODES);
      BOOST_PP_SEQ_FOR_EACH(KIND_NAME, BOOST_PP_EMPTY, DECL_NODE_IR_NODES);
      BOOST_PP_SEQ_FOR_EACH(KIND_NAME, BOOST_PP_EMPTY, NODE_STMTS);
      BOOST_PP_SEQ_FOR_EACH(KIND_NAME, BOOST_PP_EMPTY, MISCELLANEOUS_EXPR_IR_NODES);
      BOOST_PP_SEQ_FOR_EACH(KIND_NAME, BOOST_PP_EMPTY, MISCELLANEOUS_OBJ_IR_NODES);
      BOOST_PP_SEQ_FOR_EACH(KIND_NAME, BOOST_PP_EMPTY, TERNARY_EXPRESSION_IR_NODES);
      BOOST_PP_SEQ_FOR_EACH(KIND_NAME, BOOST_PP_EMPTY, TYPE_NODE_IR_NODES);
      BOOST_PP_SEQ_FOR_EACH(KIND_NAME, BOOST_PP_EMPTY, UNARY_EXPRESSION_IR_NODES(last_ir))
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

BOOST_PP_SEQ_FOR_EACH(VISIT_IR_NODE_MACRO, unary_node, UNARY_EXPRESSION_IR_NODES)
BOOST_PP_SEQ_FOR_EACH(VISIT_IR_NODE_MACRO, binary_node, BINARY_EXPRESSION_IR_NODES)
BOOST_PP_SEQ_FOR_EACH(VISIT_IR_NODE_MACRO, ternary_node, TERNARY_EXPRESSION_IR_NODES)
BOOST_PP_SEQ_FOR_EACH(VISIT_IR_NODE_MACRO, decl_node, (module_unit_node))
BOOST_PP_SEQ_FOR_EACH(VISIT_IR_NODE_MACRO, type_node, (void_ty_node))
#undef VISIT_IR_NODE_MACRO

void ir_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
}

std::string ir_node::ToString() const
{
   auto k = get_kind();
   if(k == void_ty_node_K)
   {
      return "void";
   }
   std::stringstream temp;
   temp << "@" << index << " ";
   temp << get_kind_text();
   return temp.str();
}

std::ostream& operator<<(std::ostream& os, const ir_node* tn)
{
   os << tn->ToString();
   return os;
}

std::ostream& operator<<(std::ostream& os, const ir_nodeRef& tn)
{
   os << tn.get();
   return os;
}

void IR_LocInfo::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, ir_node, visit(v));
}

decl_node::decl_node(unsigned int i)
    : IR_LocInfo(i), operating_system_flag(false), library_system_flag(false), libbambu_flag(false)
{
}

void decl_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, IR_LocInfo, visit(v));
   VISIT_MEMBER(mask, name, visit(v));
   VISIT_MEMBER(mask, mngl, visit(v));
   VISIT_MEMBER(mask, type, visit(v));
   VISIT_MEMBER(mask, parent, visit(v));
}

void expr_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, IR_LocInfo, visit(v));
   VISIT_MEMBER(mask, type, visit(v));
}

node_stmt::node_stmt(unsigned int i) : IR_LocInfo(i), bb_index(0), artificial(false), keep(false)
{
}

void node_stmt::SetVdef(const ir_nodeRef& _vdef)
{
   THROW_ASSERT(!GET_CONST_PTD_NODE(_vdef) ||
                    (_vdef->get_kind() == ssa_node_K && GetPointerS<const ssa_node>(_vdef)->virtual_flag),
                "");
   vdef = _vdef;
}

bool node_stmt::AddVuse(const ir_nodeRef& vuse)
{
   THROW_ASSERT(!GET_CONST_PTD_NODE(vuse) ||
                    (vuse->get_kind() == ssa_node_K && GetPointerS<const ssa_node>(vuse)->virtual_flag),
                "");
   return vuses.insert(vuse).second;
}

bool node_stmt::AddVover(const ir_nodeRef& vover)
{
   THROW_ASSERT(!GET_CONST_PTD_NODE(vover) ||
                    (vover->get_kind() == ssa_node_K && GetPointerS<const ssa_node>(vover)->virtual_flag),
                "");
   return vovers.insert(vover).second;
}

void node_stmt::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, IR_LocInfo, visit(v));
   VISIT_MEMBER(mask, memuse, visit(v));
   VISIT_MEMBER(mask, memdef, visit(v));
   SEQ_VISIT_MEMBER(mask, vuses, visit(v));
   VISIT_MEMBER(mask, vdef, visit(v));
   SEQ_VISIT_MEMBER(mask, vovers, visit(v));
   VISIT_MEMBER(mask, parent, visit(v));
   VISIT_MEMBER(mask, predicate, visit(v));
}

std::string unary_node::ToString() const
{
   return get_kind_text() + " " + type->ToString() + " " + op->ToString();
}

void unary_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, expr_node, visit(v));
   VISIT_MEMBER(mask, op, visit(v));
}

std::string binary_node::ToString() const
{
   return get_kind_text() + " " + type->ToString() + " " + op0->ToString() + ", " + op1->ToString();
}

void binary_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, expr_node, visit(v));
   VISIT_MEMBER(mask, op0, visit(v));
   VISIT_MEMBER(mask, op1, visit(v));
}

std::string ternary_node::ToString() const
{
   return get_kind_text() + " " + type->ToString() + " " + op0->ToString() + ", " + op1->ToString() + ", " +
          op2->ToString();
}

void ternary_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, expr_node, visit(v));
   VISIT_MEMBER(mask, op0, visit(v));
   VISIT_MEMBER(mask, op1, visit(v));
   VISIT_MEMBER(mask, op2, visit(v));
}

void type_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, ir_node, visit(v));
}

void cst_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, ir_node, visit(v));
   VISIT_MEMBER(mask, type, visit(v));
}

std::string array_ty_node::ToString() const
{
   return "[" + STR(nelements) + " x " + elts->ToString() + "]";
}

void array_ty_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, type_node, visit(v));
   VISIT_MEMBER(mask, elts, visit(v));
}

PointToSolution::PointToSolution() : anything(false), escaped(false), ipa_escaped(false), nonlocal(false), null(false)
{
}

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

void PointToSolution::Add(const ir_nodeRef& variable)
{
   variables.push_back(variable);
}

bool PointToSolution::is_a_singleton(bool nonlocal_std) const
{
   return is_fully_resolved(nonlocal_std) && variables.size() == 1;
}

bool PointToSolution::is_fully_resolved(bool nonlocal_std) const
{
   return !anything && !escaped && !ipa_escaped && !nonlocal && !variables.empty() &&
          (nonlocal_std || !std::any_of(variables.begin(), variables.end(),
                                        [](const ir_nodeRef& tn) { return tn->get_kind() == argument_val_node_K; }));
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

void PointToSolution::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   SEQ_VISIT_MEMBER(mask, variables, visit(v));
}

std::string call_node::ToString() const
{
   std::string res = "call ";
   res += type->ToString() + " ";
   if(fn->get_kind() == addr_node_K)
   {
      const auto ue = GetPointerS<const unary_node>(fn);
      auto fd = GetPointerS<const function_val_node>(ue->op);
      THROW_ASSERT(fd, "unexpected pattern");
      res += fd->ToString();
   }
   else
   {
      res += "(*" + fn->ToString() + ")";
   }
   res += "(";
   bool first_p = false;
   for(const auto& p : args)
   {
      if(!first_p)
      {
         first_p = true;
      }
      else
      {
         res += ", ";
      }
      res += ir_helper::CGetType(p)->ToString() + " " + p->ToString();
   }
   res += ")";

   return res;
}

void call_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, expr_node, visit(v));
   VISIT_MEMBER(mask, fn, visit(v));
   SEQ_VISIT_MEMBER(mask, args, visit(v));
}

void call_node::AddArg(const ir_nodeRef& arg)
{
   this->args.push_back(arg);
}

std::string call_stmt::ToString() const
{
   std::string res = "call void ";
   if(fn->get_kind() == addr_node_K)
   {
      const auto ue = GetPointerS<const unary_node>(fn);
      auto fd = GetPointerS<const function_val_node>(ue->op);
      THROW_ASSERT(fd, "unexpected pattern");
      res += fd->ToString();
   }
   else
   {
      res += "(*" + fn->ToString() + ")";
   }
   res += "(";
   bool first_p = false;
   for(const auto& p : args)
   {
      if(!first_p)
      {
         first_p = true;
      }
      else
      {
         res += ", ";
      }
      res += ir_helper::CGetType(p)->ToString() + " " + p->ToString();
   }
   res += ")";

   return res;
}

void call_stmt::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, node_stmt, visit(v));
   VISIT_MEMBER(mask, fn, visit(v));
   SEQ_VISIT_MEMBER(mask, args, visit(v));
}

void call_stmt::AddArg(const ir_nodeRef& arg)
{
   this->args.push_back(arg);
}

std::string constructor_node::ToString() const
{
   std::string res = type->ToString() + " {";
   bool first_p = false;
   for(const auto& idx_value : list_of_idx_valu)
   {
      if(!first_p)
      {
         first_p = true;
      }
      else
      {
         res += ", ";
      }

      res += ir_helper::CGetType(idx_value.second)->ToString() + " " + idx_value.second->ToString();
   }
   res += "}";
   return res;
}

void constructor_node::visit(ir_node_visitor* const v) const
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

void field_val_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, decl_node, visit(v));
}

std::string function_val_node::ToString() const
{
   return "@" + (mngl ? mngl->ToString() : name->ToString());
}

std::string function_val_node::ToStringDef() const
{
   std::string res = "define ";
   auto ft = GetPointerS<function_ty_node>(type);
   res += ft->retn->ToString() + " ";
   res += this->ToString();
   res += "(";
   bool first_p = false;
   for(const auto& p : ft->list_of_args_type)
   {
      if(!first_p)
      {
         first_p = true;
      }
      else
      {
         res += ", ";
      }
      res += p->ToString();
   }
   res += ") {\n";

   res += body->ToString();
   res += "}\n";
   return res;
}

std::string function_val_node::ToStringDecl() const
{
   std::string res = "declare ";
   auto ft = GetPointerS<function_ty_node>(type);
   res += ft->retn->ToString() + " ";
   res += this->ToString();
   res += "(";
   bool first_p = false;
   for(const auto& p : ft->list_of_args_type)
   {
      if(!first_p)
      {
         first_p = true;
      }
      else
      {
         res += ", ";
      }
      res += p->ToString();
   }
   res += ")";
   res += "\n";
   return res;
}

void function_val_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, decl_node, visit(v));
   VISIT_MEMBER(mask, fn, visit(v));
   SEQ_VISIT_MEMBER(mask, list_of_args, visit(v));
   VISIT_MEMBER(mask, body, visit(v));
}

void function_val_node::AddArg(const ir_nodeRef& a)
{
   list_of_args.push_back(a);
}

bool function_val_node::is_pipelined()
{
   return pipeline_enabled;
}

void function_val_node::set_pipelining(bool v)
{
   pipeline_enabled = v;
}

void function_val_node::set_pipeline_style(pipeline_style_t ps)
{
   pipeline_style = ps;
}

function_val_node::pipeline_style_t function_val_node::get_pipeline_style() const
{
   return pipeline_style;
}

unsigned function_val_node::get_initiation_time()
{
   return initiation_time;
}

void function_val_node::set_initiation_time(unsigned time)
{
   initiation_time = time;
}

void function_ty_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, type_node, visit(v));
   VISIT_MEMBER(mask, retn, visit(v));
   SEQ_VISIT_MEMBER(mask, list_of_args_type, visit(v));
}

std::string assign_stmt::ToString() const
{
   if(op0->get_kind() == unaligned_mem_access_node_K)
   {
      auto ma = GetPointerS<unaligned_mem_access_node>(op0);
      auto maOptype = GetPointerS<const type_node>(ir_helper::CGetType(ma->op));
      return "store volatile " + ir_helper::CGetType(op1)->ToString() + " " + op1->ToString() + ", " +
             maOptype->ToString() + " " + ma->op->ToString() + ", " + STR(maOptype->algn);
   }
   else if(op0->get_kind() == mem_access_node_K)
   {
      auto mr = GetPointerS<mem_access_node>(op0);
      auto mrOptype = GetPointerS<const type_node>(ir_helper::CGetType(mr->op));

      return "store " + ir_helper::CGetType(op1)->ToString() + " " + op1->ToString() + "," + mrOptype->ToString() +
             " " + mr->op->ToString() + ", " + STR(mrOptype->algn);
   }
   else if(op1->get_kind() == unaligned_mem_access_node_K)
   {
      auto ma = GetPointerS<unaligned_mem_access_node>(op1);
      auto maOptype = GetPointerS<const type_node>(ir_helper::CGetType(ma->op));
      return op0->ToString() + " = load volatile " + ir_helper::CGetType(op0)->ToString() + "," + maOptype->ToString() +
             " " + ma->op->ToString() + ", " + STR(maOptype->algn);
   }
   else if(op1->get_kind() == mem_access_node_K)
   {
      auto mr = GetPointerS<mem_access_node>(op1);
      auto mrOptype = GetPointerS<const type_node>(ir_helper::CGetType(mr->op));

      return op0->ToString() + " = load " + ir_helper::CGetType(op0)->ToString() + "," + mrOptype->ToString() + " " +
             mr->op->ToString() + ", " + STR(mrOptype->algn);
   }
   return op0->ToString() + " = " + op1->ToString();
}

void assign_stmt::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, node_stmt, visit(v));
   VISIT_MEMBER(mask, op0, visit(v));
   VISIT_MEMBER(mask, op1, visit(v));
}

std::string nop_stmt::ToString() const
{
   return "/*nop_stmt*/";
}

void nop_stmt::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, node_stmt, visit(v));
}

#if HAVE_IR_MANIPULATION_BUILT
identifier_node::identifier_node(unsigned int node_id, std::string _strg, ir_manager* TM)
    : ir_node(node_id), strg(std::move(_strg))
{
   TM->add_identifier_node(node_id, strg);
}
#else
identifier_node::identifier_node(unsigned int node_id, const std::string& _strg, ir_manager*)
    : ir_node(node_id), strg(_strg)
{
}
#endif

std::string identifier_node::ToString() const
{
   return strg;
}

void identifier_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, ir_node, visit(v));
}

std::string constant_int_val_node::ToString() const
{
   return type->ToString() + " " + STR(value);
}

void constant_int_val_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, cst_node, visit(v));
}

void integer_ty_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, type_node, visit(v));
}

std::string integer_ty_node::ToString() const
{
   return "i" + STR(bitsize);
}

void argument_val_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, decl_node, visit(v));
}

void phi_stmt::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, node_stmt, visit(v));
   VISIT_MEMBER(mask, res, visit(v));
   for(const auto& def_edge : list_of_def_edge)
   {
      VISIT_MEMBER_NAMED(list_of_def_edge, mask, def_edge.first, visit(v));
   }
}

void phi_stmt::AddDefEdge(const ir_managerRef& TM, const DefEdge& def_edge)
{
   list_of_def_edge.push_back(def_edge);
   if(updated_ssa_uses && bb_index != 0)
   {
      const auto sn = GetPointer<ssa_node>(def_edge.first);
      if(sn)
      {
         sn->AddUseStmt(TM->GetIRNode(index));
      }
   }
}

std::string phi_stmt::ToString() const
{
   std::string res0 = res->ToString() + " = phi " + ir_helper::CGetType(res)->ToString() + " ";
   bool first_p = false;
   for(const auto& p : list_of_def_edge)
   {
      if(!first_p)
      {
         first_p = true;
      }
      else
      {
         res0 += ", ";
      }
      res0 += "[" + p.first->ToString() + ", BB" + STR(p.second) + "]";
   }

   return res0;
}

const phi_stmt::DefEdgeList& phi_stmt::CGetDefEdgesList() const
{
   return list_of_def_edge;
}

void phi_stmt::ReplaceDefEdge(const ir_managerRef& TM, const DefEdge& old_def_edge, const DefEdge& new_def_edge)
{
   for(auto& def_edge : list_of_def_edge)
   {
      if(def_edge == old_def_edge)
      {
         if(updated_ssa_uses && bb_index != 0)
         {
            auto sn = GetPointer<ssa_node>(def_edge.first);
            if(sn)
            {
               sn->RemoveUse(TM->GetIRNode(index));
            }
         }
         def_edge = new_def_edge;
         if(updated_ssa_uses && bb_index != 0)
         {
            auto sn = GetPointer<ssa_node>(def_edge.first);
            if(sn)
            {
               sn->AddUseStmt(TM->GetIRNode(index));
            }
         }
         break;
      }
   }
}

void phi_stmt::SetDefEdgeList(const ir_managerRef& TM, DefEdgeList new_list_of_def_edge)
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

void phi_stmt::RemoveDefEdge(const ir_managerRef& TM, const DefEdge& to_be_removed)
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
            const auto sn = GetPointer<ssa_node>(to_be_removed.first);
            if(sn)
            {
               sn->RemoveUse(TM->GetIRNode(index));
            }
         }
         list_of_def_edge.erase(def_edge);
         break;
      }
   }
   THROW_ASSERT(list_of_def_edge.size() != initial_size,
                to_be_removed.first->ToString() + "(" + STR(to_be_removed.second) + ") not found in " + ToString());
}

void phi_stmt::SetSSAUsesComputed()
{
   THROW_ASSERT(!updated_ssa_uses, "SSA uses already set as updated");
   updated_ssa_uses = true;
}

void pointer_ty_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, type_node, visit(v));
   VISIT_MEMBER(mask, ptd, visit(v));
}

std::string pointer_ty_node::ToString() const
{
   return ptd->ToString() + "*";
}

std::string constant_fp_val_node::ToString() const
{
   return type->ToString() + " " + valr;
}

void constant_fp_val_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, cst_node, visit(v));
}

std::string real_ty_node::ToString() const
{
   if(bitsize == 16)
   {
      return "half";
   }
   /// add support for C/C++ data type: __bf16 -> bfloat
   else if(bitsize == 32)
   {
      return "float";
   }
   else if(bitsize == 64)
   {
      return "double";
   }
   else if(bitsize == 80)
   {
      return "x86_fp80";
   }
   else if(bitsize == 128)
   {
      return "fp128";
   }
   else
   {
      THROW_ERROR("unsupported real type");
      return "";
   }
}

void real_ty_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, type_node, visit(v));
}

void struct_ty_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, type_node, visit(v));
   VISIT_MEMBER(mask, name, visit(v));
   SEQ_VISIT_MEMBER(mask, list_of_flds, visit(v));
}

ir_nodeRef struct_ty_node::get_field(integer_cst_t offset)
{
   unsigned int i;
   integer_cst_t fld_offset;
   field_val_node* fd;
   for(i = 0; i < list_of_flds.size(); i++)
   {
      fd = GetPointer<field_val_node>(list_of_flds[i]);
      if(fd)
      {
         fld_offset = fd->offset;
      }
      else
      {
         return ir_nodeRef();
      }
      if(fld_offset == offset)
      {
         return list_of_flds[i];
      }
   }
   return ir_nodeRef();
}

std::string return_stmt::ToString() const
{
   return op ? "ret " + op->ToString() : "void";
}

void return_stmt::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, node_stmt, visit(v));
   VISIT_MEMBER(mask, op, visit(v));
}

void ssa_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, ir_node, visit(v));
   VISIT_MEMBER(mask, type, visit(v));
   VISIT_MEMBER(mask, var, visit(v));
   VISIT_MEMBER(mask, def_stmt, visit(v));
   VISIT_MEMBER(mask, min, visit(v));
   VISIT_MEMBER(mask, max, visit(v));
   if(((1 << use_set_ID) & (mask)) == 0)
      use_set.visit(v);
}

std::string ssa_node::ToString() const
{
   return "%" + STR(vers);
}

void ssa_node::AddUseStmt(const ir_nodeRef& use_stmt)
{
   use_stmts[use_stmt]++;
#ifdef CHECK_VIRTUAL_USES
   if(virtual_flag)
   {
      size_t vuse_count = 0;
      const auto gn = GetPointerS<const node_stmt>(use_stmt);
      vuse_count += gn->vuses.count(ir_nodeRef(this, null_deleter()));
      vuse_count += gn->vovers.count(ir_nodeRef(this, null_deleter()));
      vuse_count += static_cast<size_t>(gn->memuse && gn->memuse->index == index);
      if(use_stmt->get_kind() == phi_stmt_K)
      {
         const auto gp = GetPointerS<const phi_stmt>(use_stmt);
         vuse_count +=
             static_cast<size_t>(std::count_if(gp->CGetDefEdgesList().begin(), gp->CGetDefEdgesList().end(),
                                               [&](const phi_stmt::DefEdge& de) { return de.first->index == index; }));
      }
      if(use_stmts.count(use_stmt) > vuse_count)
      {
         std::cerr << "vssa: " << ToString() << std::endl;
         const auto gn = GetPointerS<const node_stmt>(use_stmt);
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

size_t ssa_node::CGetNumberUses() const
{
   size_t ret_value = 0;
   for(const auto& use_stmt : use_stmts)
   {
      ret_value += use_stmt.second;
   }
   return ret_value;
}

void ssa_node::RemoveUse(const ir_nodeRef& use_stmt)
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

void statement_list_node::add_bloc(const blocRef& a)
{
   list_of_bloc[a->number] = a;
}

std::string statement_list_node::ToString() const
{
   std::string res;
   for(const auto& b : list_of_bloc)
   {
      res += b.second->ToString();
   }
   return res;
}

void statement_list_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, ir_node, visit(v));
   auto mend = list_of_bloc.end();
   for(auto i = list_of_bloc.begin(); i != mend; ++i)
   {
      VISIT_MEMBER_NAMED(list_of_bloc, mask, i->second, visit(v));
   }
}

std::string multi_way_if_stmt::ToString() const
{
   std::string res;
   bool first_p = false;
   for(const auto& cond_bb_pair : list_of_cond)
   {
      if(!first_p)
      {
         first_p = true;
      }
      else
      {
         res += ", ";
      }
      if(cond_bb_pair.first)
      {
         res += "br i1 " + cond_bb_pair.first->ToString() + " label BB" + STR(cond_bb_pair.second);
      }
      else
      {
         res += "default label BB" + STR(cond_bb_pair.second);
      }
   }
   return res;
}

void multi_way_if_stmt::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, node_stmt, visit(v));
   for(const auto& cond : list_of_cond)
   {
      if(cond.first)
      {
         VISIT_MEMBER_NAMED(list_of_cond, mask, cond.first, visit(v));
      }
   }
}

std::string lut_node::ToString() const
{
   std::string res = get_kind_text() + " " + op0->ToString() + ", " + op1->ToString();
   if(op2)
   {
      res += ", " + op2->ToString();
   }
   if(op3)
   {
      res += ", " + op3->ToString();
   }
   if(op4)
   {
      res += ", " + op4->ToString();
   }
   if(op5)
   {
      res += ", " + op5->ToString();
   }
   if(op6)
   {
      res += ", " + op6->ToString();
   }
   if(op7)
   {
      res += ", " + op7->ToString();
   }
   if(op8)
   {
      res += ", " + op8->ToString();
   }

   return res;
}

void lut_node::visit(ir_node_visitor* const v) const
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

std::string variable_val_node::ToString() const
{
   const auto symbol_name = mngl ? mngl->ToString() : (name ? name->ToString() : ("unnamed_var_" + STR(index)));
   if(parent->get_kind() == module_unit_node_K)
   {
      return "@" + symbol_name;
   }
   else
   {
      return "alloca " + type->ToString() + ",align " + STR(algn / 8);
   }
}

void variable_val_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, decl_node, visit(v));
   VISIT_MEMBER(mask, init, visit(v));
}

std::string variable_val_node::ToStringDecl() const
{
   auto tn = GetPointerS<type_node>(type);
   const auto symbol_name = mngl ? mngl->ToString() : (name ? name->ToString() : ("unnamed_var_" + STR(index)));
   return "@" + symbol_name + " " + (static_flag ? "internal " : "") + (extern_flag ? "external " : "") + "global " +
          type->ToString() + (init ? " " + init->ToString() : "") + ", align " + STR(tn->algn);
}

void constant_vector_val_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, cst_node, visit(v));
   SEQ_VISIT_MEMBER(mask, list_of_valu, visit(v));
}

std::string vector_ty_node::ToString() const
{
   return "<" + STR(1) + " x " + STR(elts) + ">";
}

void vector_ty_node::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_SC(mask, type_node, visit(v));
   VISIT_MEMBER(mask, elts, visit(v));
}
