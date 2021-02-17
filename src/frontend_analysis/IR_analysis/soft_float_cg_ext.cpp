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
 *              Copyright (C) 2004-2021 Politecnico di Milano
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
 * @file soft_float_cg_ext.cpp
 * @brief Step that extends the call graph with the soft-float calls where appropriate.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

/// Header include
#include "soft_float_cg_ext.hpp"

/// design_flows include
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "design_flow_step.hpp"

/// frontend_analysis
#include "symbolic_application_frontend_flow_step.hpp"

/// Behavior include
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

/// Graph include
#include "basic_block.hpp"
#include "basic_blocks_graph_constructor.hpp"
#include "op_graph.hpp"

/// Parameter include
#include "Parameter.hpp"

/// STD include
#include <fstream>

/// STL include
#include "custom_map.hpp"
#include <deque>
#include <string>

/// Tree include
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#include "var_pp_functor.hpp"

/// Utility include
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

CustomMap<CallGraph::vertex_descriptor, refcount<FunctionVersion>> soft_float_cg_ext::funcFF;
tree_nodeRef soft_float_cg_ext::float32_type;
tree_nodeRef soft_float_cg_ext::float32_ptr_type;
tree_nodeRef soft_float_cg_ext::float64_type;
tree_nodeRef soft_float_cg_ext::float64_ptr_type;

static const refcount<FloatFormat> float32FF(new FloatFormat(8, 23, -127));
static const refcount<FloatFormat> float64FF(new FloatFormat(11, 52, -1023));

soft_float_cg_ext::soft_float_cg_ext(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, SOFT_FLOAT_CG_EXT, _design_flow_manager, _parameters),
      TreeM(_AppM->get_tree_manager()),
      tree_man(new tree_manipulation(TreeM, parameters)),
      fd(GetPointer<function_decl>(TreeM->GetTreeNode(function_id))),
      isTopFunction(AppM->CGetCallGraphManager()->GetRootFunctions().count(function_id)),
      bindingCompleted(fd->list_of_args.size() == 0),
      paramBinding(fd->list_of_args.size(), nullptr),
      modified(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);

   if(not float32_type)
   {
      float32_type = tree_man->create_integer_type_with_prec(32, true);
      float32_ptr_type = tree_man->create_pointer_type(float32_type, 32);
      float64_type = tree_man->create_integer_type_with_prec(64, true);
      float64_ptr_type = tree_man->create_pointer_type(float64_type, 64);
   }

   if(funcFF.empty() && !parameters->getOption<std::string>(OPT_mask).empty())
   {
      const auto CGM = AppM->CGetCallGraphManager();
      auto opts = SplitString(parameters->getOption<std::string>(OPT_mask), ",");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(const auto& opt : opts)
      {
         auto format = SplitString(opt, "*");
         if(format.size() < 4 || format.size() > 8)
         {
            THROW_ERROR("Malformed format request: " + opt);
         }

         const auto f_id = TreeM->function_index(format[0]);
         if(f_id == 0)
         {
            THROW_ERROR("Function " + format[0] + " does not exists.");
         }
         const auto function_v = CGM->GetVertex(f_id);
         if(funcFF.count(function_v))
         {
            THROW_ERROR("Function " + format[0] + " already specialized.");
         }

         const auto exp_bits = static_cast<uint8_t>(strtoul(format[1].data(), nullptr, 10));
         const auto frac_bits = static_cast<uint8_t>(strtoul(format[2].data(), nullptr, 10));
         const auto exp_bias = static_cast<int32_t>(strtol(format[3].data(), nullptr, 10));
         const auto userFF = refcount<FloatFormat>(new FloatFormat(exp_bits, frac_bits, exp_bias));

         if(format.size() > 4)
         {
            userFF->has_rounding = format[4] == "1";
         }
         if(format.size() > 5)
         {
            userFF->has_nan = format[5] == "1";
         }
         if(format.size() > 6)
         {
            userFF->has_one = format[6] == "1";
         }
         if(format.size() > 7)
         {
            userFF->has_subnorm = format[7] == "1";
         }
         if(format.size() > 8)
         {
            userFF->sign = format[8] == "U" ? bit_lattice::U : (format[8] == "1" ? bit_lattice::ONE : bit_lattice::ZERO);
         }
         funcFF.insert({function_v, refcount<FunctionVersion>(new FunctionVersion(function_v, userFF))});
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Function " + format[0] + " required specialized arithmetic: " + userFF->mngl());
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");

      // Propagate floating-point arithmetic specialization over to called user's functions
      /*
      const auto ACG = AppM->CGetCallGraphManager()->CGetAcyclicCallGraph();
      std::deque<CallGraph::vertex_descriptor> queue;
      for(const auto& topf : AppM->CGetCallGraphManager()->GetRootFunctions())
      {
         queue.push_back(CGM->GetVertex(topf));
      }
      while(not queue.empty())
      {
         const auto function_v = queue.front();
         queue.pop_front();
         FloatFormat* ff = nullptr;
         if(static_cast<bool>(funcFF.count(function_v)))
         {
            ff = funcFF.at(function_v).userRequired;
         }

         CallGraph::out_edge_iterator ei, ei_end;
         for(boost::tie(ei, ei_end) = boost::out_edges(function_v, *ACG); ei != ei_end; ++ei)
         {
            const auto called = boost::target(*ei, *ACG);
            queue.push_back(called);
            if(ff != nullptr)
            {
               funcFF[called].callersFF.insert(*ff);
            }
         }
      };
      */
   }
   THROW_ASSERT(AppM->CGetCallGraphManager()->IsVertex(function_id), "");
   const auto function_v = AppM->CGetCallGraphManager()->GetVertex(function_id);
   if(static_cast<bool>(funcFF.count(function_v)))
   {
      _version = funcFF.at(function_v);
   }
   else
   {
      _version = refcount<FunctionVersion>(new FunctionVersion(function_v));
#if HAVE_ASSERTS
      const auto insertion =
#endif
          funcFF.insert({function_v, _version});
      THROW_ASSERT(insertion.second, "");
   }
   int_type = !_version->std_format() ? tree_man->create_integer_type_with_prec(static_cast<unsigned int>(static_cast<uint8_t>(_version->userRequired->sign == bit_lattice::U) + _version->userRequired->exp_bits + _version->userRequired->frac_bits), true) :
                                        nullptr;
   int_ptr_type = int_type ? tree_man->create_pointer_type(int_type, GetPointer<integer_type>(GET_NODE(int_type))->algn) : nullptr;
}

soft_float_cg_ext::~soft_float_cg_ext() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> soft_float_cg_ext::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(EXTRACT_GIMPLE_COND_OP, SAME_FUNCTION));
         relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, CALLING_FUNCTIONS));
         relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, SAME_FUNCTION));
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(SOFT_FLOAT_CG_EXT, CALLED_FUNCTIONS));
         relationships.insert(std::make_pair(UN_COMPARISON_LOWERING, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         switch(GetStatus())
         {
            case DesignFlowStep_Status::SUCCESS:
            {
               relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, SAME_FUNCTION));
               break;
            }
            case DesignFlowStep_Status::ABORTED:
            case DesignFlowStep_Status::EMPTY:
            case DesignFlowStep_Status::NONEXISTENT:
            case DesignFlowStep_Status::SKIPPED:
            case DesignFlowStep_Status::UNCHANGED:
            case DesignFlowStep_Status::UNEXECUTED:
            case DesignFlowStep_Status::UNNECESSARY:
            default:
               break;
         }
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

bool soft_float_cg_ext::HasToBeExecuted() const
{
   static const bool is_enabled = parameters->getOption<bool>(OPT_soft_float);
   return is_enabled && FunctionFrontendFlowStep::HasToBeExecuted0() && FunctionFrontendFlowStep::HasToBeExecuted() && !modified;
}

DesignFlowStep_Status soft_float_cg_ext::InternalExec()
{
   // Function using standard float formats are always internal
   if(!_version->std_format())
   {
      const auto CG = AppM->CGetCallGraphManager()->CGetCallGraph();
      InEdgeIterator ie, ie_end;
      for(boost::tie(ie, ie_end) = boost::in_edges(_version->function_vertex, *CG); ie != ie_end; ie++)
      {
         if(static_cast<bool>(funcFF.count(ie->m_source)))
         {
            const auto& funcV = funcFF.at(ie->m_source);
            if(funcV->std_format() || *(funcV->userRequired) != *(_version->userRequired))
            {
               // If a caller of current function uses a different float format, current function is not internal to the user specified float format
               _version->internal = false;
               break;
            }
         }
         else
         {
            // If a caller of current function does not have a function version specified, it uses a standard float format for sure, thus current function is not internal
            _version->internal = false;
            break;
         }
      }
   }
   THROW_ASSERT(!_version->std_format() || _version->internal, "An standard floating-point format function should be internal.");

#ifndef NDEBUG
   const auto fn_name = tree_helper::print_type(TreeM, function_id, false, true, false, 0U, var_pp_functorConstRef(new std_var_pp_functor(function_behavior->CGetBehavioralHelper())));
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Function " + fn_name + " implementing " + _version->ToString() + " floating-point format");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---IO interface is " + STR((not _version->std_format() || _version->internal) ? "not " : "") + "necessary");

   const auto sl = GetPointerS<statement_list>(GET_NODE(fd->body));
   modified = false;

   for(const auto& block : sl->list_of_bloc)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Update recursively BB" + STR(block.first));
      for(const auto& phi : block.second->CGetPhiList())
      {
         RecursiveExaminate(phi, phi, INTERFACE_TYPE_NONE);
      }

      // RecursiveExaminate could add statements to the statements list, thus it is necessary to iterate over a static copy of the initial statement list
      for(const auto& stmt : block.second->CGetStmtList())
      {
         RecursiveExaminate(stmt, stmt, INTERFACE_TYPE_NONE);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Updated recursively BB" + STR(block.first));
   }

   // Fix hardware implemented function arguments
   if(hwParam.size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Adding view-convert expressions to support hardware impelmented FU call arguments");
      for(const auto& ssa_uses : hwParam)
      {
         const auto ssa = ssa_uses.first;
         const auto ssa_ridx = TreeM->GetTreeReindex(ssa->index);
         const auto out_int = outputInterface.find(ssa);
         std::vector<tree_nodeRef>* out_ssa = out_int != outputInterface.end() ? &out_int->second : nullptr;
         for(const auto& call_stmt_idx : ssa_uses.second)
         {
            const auto call_stmt = TreeM->CGetTreeReindex(call_stmt_idx);
            const auto call_node = GetPointerS<const gimple_node>(GET_CONST_NODE(call_stmt));
            const auto& call_bb = sl->list_of_bloc.at(call_node->bb_index);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---View-convert for " + ssa->ToString() + " in BB" + STR(call_bb->number) + " " + call_node->ToString());
            // At this time ssa->type is still real_type, thus we can exploit that (it will be modified after)
            const auto arg_vc = tree_man->create_unary_operation(ssa->type, ssa_ridx, BUILTIN_SRCP, view_convert_expr_K);
            const auto vc_stmt = tree_man->CreateGimpleAssign(ssa->type, tree_nodeRef(), tree_nodeRef(), arg_vc, call_bb->number, BUILTIN_SRCP);
            const auto vc_ssa = GetPointerS<gimple_assign>(GET_NODE(vc_stmt))->op0;
            call_bb->PushBefore(vc_stmt, call_stmt);
            TreeM->ReplaceTreeNode(call_stmt, ssa_ridx, vc_ssa);
            if(out_ssa)
            {
               std::replace_if(
                   out_ssa->begin(), out_ssa->end(), [&](const tree_nodeRef& t) { return GET_INDEX_CONST_NODE(t) == call_stmt_idx; }, vc_stmt);
            }
         }
      }
      modified = true;
      hwParam.clear();
   }

   // Fix hardware implemented function return values
   if(hwReturn.size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Adding view-convert expressions to support hardware impelmented FU call return values");
      for(const auto& ssa : hwReturn)
      {
         const auto call_stmt = ssa->CGetDefStmt();
         const auto def_node = GetPointerS<const gimple_node>(GET_CONST_NODE(call_stmt));
         const auto call_bb = sl->list_of_bloc.at(def_node->bb_index);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---View-convert for " + ssa->ToString() + " in BB" + STR(call_bb->number) + " " + def_node->ToString());
         const auto ssa_ridx = TreeM->GetTreeReindex(ssa->index);
         // Hardware calls are for sure dealing with standard IEEE formats only
         const auto int_ret_type = tree_helper::Size(GET_NODE(ssa->type)) == 32 ? float32_type : float64_type;
         const auto ret_vc = tree_man->create_unary_operation(int_ret_type, ssa_ridx, BUILTIN_SRCP, view_convert_expr_K);
         const auto vc_stmt = tree_man->CreateGimpleAssign(int_ret_type, tree_nodeRef(), tree_nodeRef(), ret_vc, call_bb->number, BUILTIN_SRCP);
         const auto vc_ssa = GetPointerS<const gimple_assign>(GET_CONST_NODE(vc_stmt))->op0;
         const auto ssa_uses = ssa->CGetUseStmts();
         for(const auto& stmt_uses : ssa_uses)
         {
            TreeM->ReplaceTreeNode(stmt_uses.first, ssa_ridx, vc_ssa);
         }
         call_bb->PushAfter(vc_stmt, call_stmt);
         viewConvert.erase(ssa);
      }
      modified = true;
      hwReturn.clear();
   }

   // Design top function signatures must not be modified, thus a view-convert operation for real_type parameters and return value must be added inside the function body
   if(isTopFunction)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameters binding " + STR(bindingCompleted ? "" : "partially ") + "completed on " + STR(paramBinding.size()) + " arguments");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      const auto entry_bb = sl->list_of_bloc.at(bloc::ENTRY_BLOCK_ID);
      const auto first_bb = sl->list_of_bloc.at(entry_bb->list_of_succ.front());
      for(const auto& parm : paramBinding)
      {
         if(parm)
         {
            THROW_ASSERT(parm->get_kind() == ssa_name_K, "Unexpected parameter node type (" + parm->get_kind_text() + ")");
            const auto parmSSA = GetPointerS<ssa_name>(parm);
            if(lowering_needed(TreeM, parmSSA))
            {
               const auto parm_ridx = TreeM->CGetTreeReindex(parmSSA->index);
               const auto parm_type = parm_int_type_for(GET_NODE(parmSSA->type));
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Lowering top function parameter type of " + parmSSA->ToString() + ": " + GET_NODE(parmSSA->type)->ToString() + " -> " + GET_NODE(parm_type)->ToString());
               tree_nodeRef vc_stmt;
               if(GET_NODE(parm_type)->get_kind() == pointer_type_K)
               {
                  vc_stmt = tree_man->CreateNopExpr(parm_ridx, parm_type, tree_nodeRef(), tree_nodeRef());
               }
               else
               {
                  const auto vc = tree_man->create_unary_operation(parm_type, parm_ridx, BUILTIN_SRCP, view_convert_expr_K);
                  vc_stmt = tree_man->CreateGimpleAssign(parm_type, tree_nodeRef(), tree_nodeRef(), vc, first_bb->number, BUILTIN_SRCP);
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Lowering statement added to BB" + STR(first_bb->number) + ": " + GET_NODE(vc_stmt)->ToString());
               const auto lowered_parm = GetPointerS<gimple_assign>(GET_NODE(vc_stmt))->op0;
               const auto parm_uses = parmSSA->CGetUseStmts();
               for(const auto& stmt_uses : parm_uses)
               {
                  TreeM->ReplaceTreeNode(stmt_uses.first, parm_ridx, lowered_parm);
               }
               first_bb->PushFront(vc_stmt);
               viewConvert.erase(parmSSA);
               modified = true;
            }
         }
      }
      paramBinding.clear();
      for(const auto& ret_stmt : topReturn)
      {
         const auto gr = GetPointerS<gimple_return>(GET_NODE(ret_stmt));
         const auto ret_ssa = GetPointerS<ssa_name>(GET_NODE(gr->op));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Return value type restore added for variable " + ret_ssa->ToString());
         const auto bb = sl->list_of_bloc.at(gr->bb_index);
         tree_nodeRef vc_stmt;
         if(GET_NODE(ret_ssa->type)->get_kind() == pointer_type_K)
         {
            vc_stmt = tree_man->CreateNopExpr(gr->op, ret_ssa->type, tree_nodeRef(), tree_nodeRef());
         }
         else
         {
            const auto vc = tree_man->create_unary_operation(ret_ssa->type, gr->op, BUILTIN_SRCP, view_convert_expr_K);
            vc_stmt = tree_man->CreateGimpleAssign(ret_ssa->type, tree_nodeRef(), tree_nodeRef(), vc, bb->number, BUILTIN_SRCP);
         }
         const auto lowered_ret = GetPointerS<gimple_assign>(GET_NODE(vc_stmt))->op0;
         bb->PushBefore(vc_stmt, ret_stmt);
         TreeM->ReplaceTreeNode(ret_stmt, gr->op, lowered_ret);
      }
      modified |= topReturn.size();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
   else
   {
      // Else transform real type parameters and return value in unsigned integer type
      const auto modified_signature = signature_lowering(fd);
      if(modified_signature)
      {
         modified = true;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameters binding " + STR(bindingCompleted ? "" : "partially ") + "completed on " + STR(paramBinding.size()) + " arguments");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
         size_t idx;
         for(idx = 0; idx < fd->list_of_args.size(); ++idx)
         {
            const auto& param = paramBinding.at(idx);
            const auto& arg = fd->list_of_args.at(idx);
            const auto pd = GetPointerS<const parm_decl>(GET_CONST_NODE(arg));
            if(param)
            {
               THROW_ASSERT(param->get_kind() == ssa_name_K, "Unexpected parameter node type (" + param->get_kind_text() + ")");
               const auto parmSSA = GetPointerS<ssa_name>(param);

               if(GET_INDEX_NODE(pd->type) != GET_INDEX_NODE(parmSSA->type))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Lowering type of " + parmSSA->ToString() + " bound to paremeter " + pd->ToString() + ": " + GET_NODE(parmSSA->type)->ToString() + " -> " + GET_NODE(pd->type)->ToString());
                  parmSSA->type = pd->type;

                  // Remove ssa variable associated to function parameter to avoid multiple type replacement
                  viewConvert.erase(parmSSA);
               }
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Missing binding for parameter " + pd->ToString());
            }
         }
         paramBinding.clear();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
   }

   if(viewConvert.size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Lowering type for " + STR(viewConvert.size()) + " ssa variables");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(const auto& ssa_var : viewConvert)
      {
         ssa_lowering(ssa_var);
      }
      modified = true;
      viewConvert.clear();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }

   if(nopConvert.size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Lowering " + STR(nopConvert.size()) + " view-convert expressions to nop expressions");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(const auto& vcStmt : nopConvert)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Before lowering - " + GET_NODE(vcStmt)->ToString());
         const auto ga = GetPointerS<gimple_assign>(GET_NODE(vcStmt));
         THROW_ASSERT(ga, "");
         const auto vc = GetPointerS<view_convert_expr>(GET_NODE(ga->op1));
         THROW_ASSERT(vc, "");
         THROW_ASSERT(tree_helper::CGetType(GET_NODE(vc->op))->get_kind() == integer_type_K, "At this point " + GET_NODE(vc->op)->ToString() + " should be of integer type.");
         const auto resType = TreeM->GetTreeReindex(tree_helper::CGetType(GET_NODE(ga->op0))->index);
         THROW_ASSERT(GET_NODE(resType)->get_kind() == integer_type_K, "Destination variable should of integer type (" + GET_NODE(resType)->get_kind_text() + ")");
         const auto nop = tree_man->create_unary_operation(resType, vc->op, BUILTIN_SRCP, nop_expr_K);
         TreeM->ReplaceTreeNode(vcStmt, ga->op1, nop);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---After lowering - " + GET_NODE(vcStmt)->ToString());
      }
      modified = true;
      nopConvert.clear();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }

   if(inputInterface.size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Generating input interface for " + STR(inputInterface.size()) + " variables");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(const auto& ssaExclude : inputInterface)
      {
         const auto* SSA = ssaExclude.first;
         const auto ssa = TreeM->GetTreeReindex(SSA->index);
         const auto& exclude = ssaExclude.second;

         auto defStmt = SSA->CGetDefStmt();
         const auto def = GetPointerS<gimple_node>(GET_NODE(defStmt));
         blocRef bb;
         if(def->get_kind() == gimple_assign_K)
         {
            THROW_ASSERT(sl->list_of_bloc.count(def->bb_index), "BB " + STR(def->bb_index) + " not present in current function.");
            bb = sl->list_of_bloc.at(def->bb_index);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Input interface for " + SSA->ToString() + " will be inserted in BB" + STR(bb->number));
         }
         else if(def->get_kind() == gimple_phi_K)
         {
            THROW_ASSERT(sl->list_of_bloc.count(def->bb_index), "BB " + STR(def->bb_index) + " not present in current function.");
            bb = sl->list_of_bloc.at(def->bb_index);
            defStmt = nullptr;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Input interface for phi " + SSA->ToString() + " will be inserted in BB" + STR(bb->number));
         }
         else
         {
            THROW_ASSERT(sl->list_of_bloc.at(BB_ENTRY)->list_of_succ.size() == 1, "Multiple successors after entry basic block.");
            const auto realEntryBBIdx = sl->list_of_bloc.at(BB_ENTRY)->list_of_succ.front();
            bb = sl->list_of_bloc.at(realEntryBBIdx);
            defStmt = nullptr;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Input interface for parameter " + SSA->ToString() + " will be inserted in BB" + STR(bb->number));
         }

         // Get ssa uses before renaming to avoid replacement in cast rename operations
         const auto ssaUses = SSA->CGetUseStmts();

         const auto convertedSSA = generate_interface(bb, defStmt, ssa, nullptr, _version->userRequired);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Interface generated output " + GET_NODE(convertedSSA)->ToString());

         for(const auto& ssaUse : ssaUses)
         {
            const auto& useStmt = ssaUse.first;
            if(std::find(exclude.begin(), exclude.end(), GET_INDEX_NODE(useStmt)) != exclude.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipping replacement for statement " + GET_NODE(useStmt)->ToString());
               continue;
            }
            TreeM->ReplaceTreeNode(useStmt, ssa, convertedSSA);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Replaced in statement " + GET_NODE(useStmt)->ToString());
            modified = true;
         }
      }
      inputInterface.clear();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }

   if(outputInterface.size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Generating output interface for " + STR(outputInterface.size()) + " variables");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(const auto& ssaReplace : outputInterface)
      {
         const auto* SSA = ssaReplace.first;
         const auto ssa = TreeM->GetTreeReindex(SSA->index);
         const auto& useStmts = ssaReplace.second;

         const auto defStmt = SSA->CGetDefStmt();
         const auto gn = GetPointerS<gimple_node>(GET_NODE(defStmt));
         THROW_ASSERT(sl->list_of_bloc.count(gn->bb_index), "BB" + STR(gn->bb_index) + " not present in current function.");
         const auto bb = sl->list_of_bloc.at(gn->bb_index);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Output interface for " + SSA->ToString() + " will be inserted in BB" + STR(bb->number));

         const auto convertedSSA = generate_interface(bb, defStmt, ssa, _version->userRequired, nullptr);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Interface generated output " + GET_NODE(convertedSSA)->ToString());
         for(const auto& stmt : useStmts)
         {
            TreeM->ReplaceTreeNode(stmt, ssa, convertedSSA);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Replaced in statement " + GET_NODE(stmt)->ToString());
            modified = true;
         }
      }
      outputInterface.clear();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");

   if(modified)
   {
      function_behavior->UpdateBBVersion();
      return DesignFlowStep_Status::SUCCESS;
   }
   modified = true;
   return DesignFlowStep_Status::UNCHANGED;
}

bool soft_float_cg_ext::lowering_needed(const tree_managerRef& TreeM, const ssa_name* ssa)
{
   const auto ssa_type = GET_CONST_NODE(ssa->type);
   return (tree_helper::is_a_pointer(TreeM, ssa_type->index) && tree_helper::CGetPointedType(ssa_type)->get_kind() == real_type_K) || ssa_type->get_kind() == real_type_K;
}

tree_nodeRef soft_float_cg_ext::parm_int_type_for(const tree_nodeRef& type) const
{
   THROW_ASSERT(type, "");
   if(tree_helper::is_a_pointer(TreeM, type->index))
   {
      return tree_helper::Size(tree_helper::CGetPointedType(type)) == 32 ? float32_ptr_type : float64_ptr_type;
   }
   if(not _version->internal || _version->std_format())
   {
      return tree_helper::Size(type) == 32 ? float32_type : float64_type;
   }
   else
   {
      THROW_ASSERT(int_type, "Internal integer type should have been defined before.");
      return int_type;
   }
}

tree_nodeRef soft_float_cg_ext::int_type_for(const tree_nodeRef& type) const
{
   if(tree_helper::is_a_pointer(TreeM, type->index))
   {
      return tree_helper::Size(tree_helper::CGetPointedType(type)) == 32 ? float32_ptr_type : float64_ptr_type;
   }
   if(_version->std_format())
   {
      return tree_helper::Size(type) == 32 ? float32_type : float64_type;
   }
   else
   {
      THROW_ASSERT(int_type, "Internal integer type should have been defined before.");
      return int_type;
   }
}

bool soft_float_cg_ext::signature_lowering(function_decl* f_decl) const
{
#ifndef NDEBUG
   const auto f_name = tree_helper::print_type(TreeM, f_decl->index, false, true, false, 0U, var_pp_functorConstRef(new std_var_pp_functor(AppM->CGetFunctionBehavior(f_decl->index)->CGetBehavioralHelper())));
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing function signature " + f_name);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   bool changed = false;
   const auto f_type = GET_NODE(f_decl->type)->get_kind() == pointer_type_K ? GET_NODE(GetPointerS<pointer_type>(GET_NODE(f_decl->type))->ptd) : GET_NODE(f_decl->type);
   tree_list* prms = nullptr;
   if(f_type->get_kind() == function_type_K)
   {
      const auto ft = GetPointerS<function_type>(f_type);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing return type " + GET_NODE(ft->retn)->ToString());
      if(GET_NODE(ft->retn)->get_kind() == real_type_K)
      {
         const auto int_ret = parm_int_type_for(ft->retn);
         const auto ret_type = GetPointerS<const type_node>(GET_CONST_NODE(int_ret));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Return type lowered to " + ret_type->ToString() + " " + STR(tree_helper::Size(GET_NODE(int_ret))));
         ft->retn = int_ret;
         ft->algn = ret_type->algn;
         ft->qual = ret_type->qual;
         ft->size = ret_type->size;
         changed = true;
      }
      prms = ft->prms ? GetPointerS<tree_list>(GET_NODE(ft->prms)) : nullptr;
   }
   else if(f_type->get_kind() == method_type_K)
   {
      const auto mt = GetPointerS<method_type>(f_type);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing return type " + GET_NODE(mt->retn)->ToString());
      if(GET_NODE(mt->retn)->get_kind() == real_type_K)
      {
         const auto int_ret = parm_int_type_for(mt->retn);
         const auto ret_type = GetPointerS<const type_node>(GET_CONST_NODE(int_ret));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Return type lowered to " + ret_type->ToString() + " " + STR(tree_helper::Size(GET_NODE(int_ret))));
         mt->retn = int_ret;
         mt->algn = ret_type->algn;
         mt->qual = ret_type->qual;
         mt->size = ret_type->size;
         changed = true;
      }
      prms = mt->prms ? GetPointerS<tree_list>(GET_NODE(mt->prms)) : nullptr;
   }
   for(const auto& arg : f_decl->list_of_args)
   {
      const auto pd = GetPointerS<parm_decl>(GET_NODE(arg));
      const auto parm_type = GET_NODE(pd->type);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing parameter " + pd->ToString() + " of type " + parm_type->ToString());
      if((tree_helper::is_a_pointer(TreeM, parm_type->index) && tree_helper::CGetPointedType(parm_type)->get_kind() == real_type_K) || parm_type->get_kind() == real_type_K)
      {
         const auto int_parm_type = parm_int_type_for(parm_type);
         const auto parm_int_type = GetPointerS<const type_node>(GET_CONST_NODE(int_parm_type));
         pd->algn = parm_int_type->algn;
         pd->argt = int_parm_type;
         pd->packed_flag = parm_int_type->packed_flag;
         pd->type = int_parm_type;
         if(prms)
         {
            prms->valu = int_parm_type;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Parameter type lowered to " + GET_NODE(int_parm_type)->ToString() + " " + STR(tree_helper::Size(GET_NODE(int_parm_type))));
         changed = true;
      }
      prms = prms ? (prms->chan ? GetPointerS<tree_list>(GET_NODE(prms->chan)) : nullptr) : nullptr;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   return changed;
}

void soft_float_cg_ext::ssa_lowering(ssa_name* ssa) const
{
   const auto ssa_type = GET_NODE(ssa->type);
   THROW_ASSERT(lowering_needed(TreeM, ssa), "Unexpected ssa type - " + ssa->ToString() + " " + ssa_type->ToString());
   const auto vc_type = int_type_for(ssa_type);
   THROW_ASSERT(vc_type, "");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Lowering " + ssa->ToString() + " type to " + GET_NODE(vc_type)->ToString());

   const auto defStmt = ssa->CGetDefStmt();
   const auto def = GetPointer<gimple_assign>(GET_NODE(defStmt));
   if(def)
   {
      const auto ue = GetPointer<unary_expr>(GET_NODE(def->op1));
      if(ue)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Definition statement before - " + def->ToString());
         if(ue->get_kind() == view_convert_expr_K)
         {
            const auto nop = tree_man->create_unary_operation(vc_type, ue->op, BUILTIN_SRCP, nop_expr_K);
            TreeM->ReplaceTreeNode(defStmt, def->op1, nop);
         }
         else if(ue->get_kind() == imagpart_expr_K || ue->get_kind() == realpart_expr_K)
         {
            const auto ssa_ridx = TreeM->CGetTreeReindex(ssa->index);
            const auto def_bb = GetPointerS<statement_list>(GET_NODE(fd->body))->list_of_bloc.at(def->bb_index);
            const auto vc = tree_man->create_unary_operation(vc_type, ssa_ridx, BUILTIN_SRCP, view_convert_expr_K);
            const auto vc_stmt = tree_man->CreateGimpleAssign(vc_type, tree_nodeRef(), tree_nodeRef(), vc, def_bb->number, BUILTIN_SRCP);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Inserting view-convert operation after complex part expression - " + GET_NODE(vc_stmt)->ToString());
            const auto lowered_ssa = GetPointerS<gimple_assign>(GET_NODE(vc_stmt))->op0;

            const auto ssa_uses = ssa->CGetUseStmts();
            for(const auto& stmt_uses : ssa_uses)
            {
               TreeM->ReplaceTreeNode(stmt_uses.first, ssa_ridx, lowered_ssa);
            }
            def_bb->PushAfter(vc_stmt, defStmt);
            return;
         }
         else
         {
            ue->type = vc_type;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Definition statement after - " + def->ToString());
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Definition statement - " + def->ToString());
      }
   }

   ssa->type = vc_type;
   if(ssa->var && GET_NODE(ssa->var)->get_kind() != parm_decl_K)
   {
      const auto vd = GetPointer<var_decl>(GET_NODE(ssa->var));
      THROW_ASSERT(vd, "SSA name associated variable is espected to be a variable declaration " + GET_NODE(ssa->var)->get_kind_text() + " " + GET_NODE(ssa->var)->ToString());
      if(GET_INDEX_NODE(vd->type) != GET_INDEX_NODE(ssa->type))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable declaration before - " + vd->ToString() + " " + GET_NODE(vd->type)->ToString());
         const auto var_int_type = GetPointerS<type_node>(GET_NODE(vc_type));
         vd->algn = var_int_type->algn;
         vd->packed_flag = var_int_type->packed_flag;
         vd->type = vc_type;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable declaration after - " + vd->ToString() + " " + GET_NODE(vd->type)->ToString());
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable declaration - " + vd->ToString() + " " + GET_NODE(vd->type)->ToString());
      }
   }
}

tree_nodeRef soft_float_cg_ext::cstCast(uint64_t bits, refcount<FloatFormat> inFF, refcount<FloatFormat> outFF) const
{
   bool Sign = inFF->sign != bit_lattice::U ? (inFF->sign == bit_lattice::ONE) : ((bits >> (inFF->exp_bits + inFF->frac_bits)) & 1ULL);
   uint64_t exp = (bits >> inFF->frac_bits) & ((1ULL << inFF->exp_bits) - 1ULL);
   uint64_t frac = bits & ((1ULL << inFF->frac_bits) - 1ULL);

   bool expMax = exp == ((1ULL << inFF->exp_bits) - 1ULL);
   bool nan = expMax & (bits >> (inFF->frac_bits - 1ULL));
   uint64_t RExp;
   uint64_t RFrac;
   uint64_t out_val;

   bool out_nan;

   int64_t exp_fix;
   int64_t range_diff;
   uint64_t FExp;

   uint8_t bits_diff;
   uint64_t ExpFrac;
   uint64_t RExpFrac;
   bool round;

   if(inFF->exp_bits != outFF->exp_bits || inFF->exp_bias != outFF->exp_bias)
   {
      exp_fix = inFF->exp_bias - outFF->exp_bias;
      range_diff = ((1LL << outFF->exp_bits) - 1LL) - ((1LL << inFF->exp_bits) - 1LL);
      FExp = exp + static_cast<uint64_t>(exp_fix);
   }
   else
   {
      exp_fix = 0;
      range_diff = 0;
      FExp = exp;
   }

   if(inFF->frac_bits > outFF->frac_bits)
   {
      bits_diff = static_cast<uint8_t>(inFF->frac_bits - outFF->frac_bits);
      ExpFrac = (FExp << outFF->frac_bits) | (frac >> bits_diff);
      // round = (frac >> (bits_diff - 1U)) & 1U;
      round = (frac & ((1ULL << bits_diff) - 1)) != 0; // IEEE conversion rounding
      RExpFrac = ExpFrac + static_cast<uint64_t>(round & (inFF->has_rounding | outFF->has_rounding));
   }
   else if(inFF->frac_bits < outFF->frac_bits)
   {
      bits_diff = static_cast<uint8_t>(outFF->frac_bits - inFF->frac_bits);
      RExpFrac = (FExp << (outFF->frac_bits)) | frac << bits_diff;
   }
   else
   {
      RExpFrac = (FExp << outFF->frac_bits) | frac;
   }

   out_nan = (inFF->sign != outFF->sign) & ((outFF->sign == bit_lattice::ONE) != Sign);
   out_nan |= (inFF->has_nan & expMax);
   out_nan |= (((inFF->exp_bits != outFF->exp_bits) | (inFF->exp_bias != outFF->exp_bias)) & (((exp_fix < 0) & (static_cast<int64_t>(FExp) < 0)) | ((exp_fix > range_diff) & (FExp > ((1ULL << outFF->exp_bits) - (outFF->has_nan ? 2ULL : 1ULL))))));
   out_nan |= (inFF->has_rounding | outFF->has_rounding) & ((!outFF->has_nan) & (RExpFrac & (1ULL << (outFF->exp_bits + outFF->frac_bits))));

   RExp = out_nan ? ((1ULL << outFF->exp_bits) - 1ULL) : (RExpFrac >> outFF->frac_bits);
   RFrac = (out_nan | ((inFF->sign != outFF->sign) & ((outFF->sign == bit_lattice::ONE) != Sign))) ? (outFF->has_nan ? (static_cast<uint64_t>(nan) << (outFF->frac_bits - 1ULL)) : ((1ULL << outFF->frac_bits) - 1ULL)) :
                                                                                                     (RExpFrac & ((1ULL << outFF->frac_bits) - 1ULL));

   out_val = (RExp << outFF->frac_bits) | RFrac;
   if(outFF->sign == bit_lattice::U)
   {
      out_val = (static_cast<uint64_t>(Sign) << (outFF->exp_bits + outFF->frac_bits)) | out_val;
   }
   return TreeM->CreateUniqueIntegerCst(static_cast<int64_t>(out_val), GET_INDEX_NODE(tree_man->create_integer_type_with_prec(static_cast<unsigned int>(static_cast<uint8_t>(outFF->sign == bit_lattice::U) + outFF->exp_bits + outFF->frac_bits), true)));
}

tree_nodeRef soft_float_cg_ext::generate_interface(blocRef bb, tree_nodeRef stmt, tree_nodeRef ssa, refcount<FloatFormat> inFF, refcount<FloatFormat> outFF) const
{
#if HAVE_ASSERTS
   const auto t_kind = tree_helper::CGetType(GET_CONST_NODE(ssa))->get_kind();
#endif
   THROW_ASSERT(t_kind == integer_type_K, "Cast rename should be applied on integer variables only. " + tree_node::GetString(t_kind));
   const auto real_size = tree_helper::Size(GET_CONST_NODE(ssa));
   if(not inFF)
   {
      inFF = real_size == 32 ? float32FF : float64FF;
   }
   if(not outFF)
   {
      outFF = real_size == 32 ? float32FF : float64FF;
   }

   const auto createStmt = [&](tree_nodeRef op_type, tree_nodeRef op) {
      if(GET_NODE(op)->get_kind() != gimple_assign_K)
      {
         op = tree_man->CreateGimpleAssign(op_type, tree_nodeRef(), tree_nodeRef(), op, bb->number, BUILTIN_SRCP);
      }
      if(stmt == nullptr)
      {
         bb->PushFront(op);
      }
      else
      {
         bb->PushAfter(op, stmt);
      }
      stmt = op;
      return GetPointerS<gimple_assign>(GET_NODE(op))->op0;
   };

   const auto bool_type = tree_man->create_boolean_type();
   const auto in_type = GetPointerS<ssa_name>(GET_NODE(ssa))->type;
   THROW_ASSERT(tree_helper::Size(GET_NODE(in_type)) == static_cast<unsigned int>(static_cast<uint8_t>(inFF->sign == bit_lattice::U) + inFF->exp_bits + inFF->frac_bits), "");
   const auto in_type_idx = GET_INDEX_NODE(in_type);
   const auto out_type = tree_man->create_integer_type_with_prec(static_cast<unsigned int>(static_cast<uint8_t>(outFF->sign == bit_lattice::U) + outFF->exp_bits + outFF->frac_bits), true);
   const auto out_type_idx = GET_INDEX_NODE(out_type);
   const auto exp_type = tree_man->create_integer_type_with_prec(std::max(inFF->exp_bits, uint8_t(outFF->exp_bits + 1)), false);
   const auto exp_type_idx = GET_INDEX_NODE(exp_type);
   const auto expFrac_type = tree_man->create_integer_type_with_prec(1U + outFF->exp_bits + outFF->frac_bits, true);
   const auto expFrac_type_idx = GET_INDEX_NODE(expFrac_type);

   // Apply fractional bits mask
   const auto fracAnd = tree_man->create_binary_operation(in_type, ssa, TreeM->CreateUniqueIntegerCst((1LL << inFF->frac_bits) - 1, in_type_idx), BUILTIN_SRCP, bit_and_expr_K);
   auto Frac = createStmt(in_type, fracAnd);

   // Right shift input value
   const auto expRShift = tree_man->create_binary_operation(in_type, ssa, TreeM->CreateUniqueIntegerCst(inFF->frac_bits, in_type_idx), BUILTIN_SRCP, rshift_expr_K);
   auto Exp = createStmt(in_type, expRShift);

   // Apply exponent bits mask
   const auto expAnd = tree_man->create_binary_operation(in_type, Exp, TreeM->CreateUniqueIntegerCst((1LL << inFF->exp_bits) - 1, in_type_idx), BUILTIN_SRCP, bit_and_expr_K);
   Exp = createStmt(in_type, expAnd);

   // Compute out exponent bits
   tree_nodeRef FExp;
   if((inFF->exp_bits != outFF->exp_bits) || (inFF->exp_bias != outFF->exp_bias))
   {
      const auto biasDiff = inFF->exp_bias - outFF->exp_bias;
      if(biasDiff + ((int32_t(1) << outFF->exp_bits) - 1) < 0)
      {
         THROW_ERROR("Output float format does not intersect with input float format");
         return nullptr;
      }
      // TODO: add simplified cases for exactly scaled bias values

      // Cast exp bits to shrinked type
      const auto expCast = tree_man->CreateNopExpr(Exp, exp_type, tree_nodeRef(), tree_nodeRef());
      FExp = createStmt(exp_type, expCast);

      const auto expAdd = tree_man->create_binary_operation(exp_type, FExp, TreeM->CreateUniqueIntegerCst(biasDiff, exp_type_idx), BUILTIN_SRCP, plus_expr_K);
      FExp = createStmt(exp_type, expAdd);
   }
   else
   {
      FExp = Exp;
   }

   // Cast exponent to internal operations type
   const auto fexpCast = tree_man->CreateNopExpr(FExp, expFrac_type, tree_nodeRef(), tree_nodeRef());
   auto RExp = createStmt(expFrac_type, fexpCast);

   // Shift exponent left
   const auto expLShift = tree_man->create_binary_operation(expFrac_type, RExp, TreeM->CreateUniqueIntegerCst(outFF->frac_bits, expFrac_type_idx), BUILTIN_SRCP, lshift_expr_K);
   RExp = createStmt(expFrac_type, expLShift);

   // Cast input significand to internal operations type
   const auto fracCast = tree_man->CreateNopExpr(Frac, expFrac_type, tree_nodeRef(), tree_nodeRef());
   auto RExpFrac = createStmt(expFrac_type, fracCast);
   if(inFF->frac_bits > outFF->frac_bits)
   {
      // Shift input significand right
      const auto bits_diff = inFF->frac_bits - outFF->frac_bits;
      const auto fracRShift = tree_man->create_binary_operation(expFrac_type, RExpFrac, TreeM->CreateUniqueIntegerCst(bits_diff, expFrac_type_idx), BUILTIN_SRCP, rshift_expr_K);
      RExpFrac = createStmt(expFrac_type, fracRShift);
   }
   else if(inFF->frac_bits < outFF->frac_bits)
   {
      // Shift input significand left
      const auto bits_diff = outFF->frac_bits - inFF->frac_bits;
      const auto fracLShift = tree_man->create_binary_operation(expFrac_type, RExpFrac, TreeM->CreateUniqueIntegerCst(bits_diff, expFrac_type_idx), BUILTIN_SRCP, lshift_expr_K);
      RExpFrac = createStmt(expFrac_type, fracLShift);
   }

   // Concatenate exponent and significand
   const auto expOrFrac = tree_man->create_binary_operation(expFrac_type, RExp, RExpFrac, BUILTIN_SRCP, bit_ior_expr_K);
   RExpFrac = createStmt(expFrac_type, expOrFrac);

   if(inFF->frac_bits > outFF->frac_bits && (inFF->has_rounding || outFF->has_rounding))
   {
      const auto bits_diff = inFF->frac_bits - outFF->frac_bits;
      //    // Shift to get significand discarded msb
      //    const auto roundRShift = tree_man->create_binary_operation(in_type, Frac, TreeM->CreateUniqueIntegerCst(bits_diff - 1, in_type_idx), BUILTIN_SRCP, rshift_expr_K);
      //    auto Round = createStmt(in_type, roundRShift);
      //
      //    // And to keep only first bit value
      //    const auto roundAnd = tree_man->create_binary_operation(in_type, Round, TreeM->CreateUniqueIntegerCst(1, in_type_idx), BUILTIN_SRCP, bit_and_expr_K);
      //    Round = createStmt(in_type, roundAnd);

      // Mask least significant bits which will be discarded by this type conversion
      const auto roundAnd = tree_man->create_binary_operation(in_type, Frac, TreeM->CreateUniqueIntegerCst((1LL << bits_diff) - 1, in_type_idx), BUILTIN_SRCP, bit_and_expr_K);
      auto Round = createStmt(in_type, roundAnd);

      // Check if discarded bits are different from zero (IEEE type conversion rounding policy)
      const auto roundCmp = tree_man->create_binary_operation(bool_type, Round, TreeM->CreateUniqueIntegerCst(0, in_type_idx), BUILTIN_SRCP, ne_expr_K);
      Round = createStmt(bool_type, roundCmp);

      // Cast round bit to expFrac type
      const auto roundCast = tree_man->CreateNopExpr(Round, expFrac_type, TreeM->CreateUniqueIntegerCst(0, expFrac_type_idx), TreeM->CreateUniqueIntegerCst(1, expFrac_type_idx));
      Round = createStmt(expFrac_type, roundCast);

      // Sum rounding bit
      const auto roundAdd = tree_man->create_binary_operation(expFrac_type, RExpFrac, Round, BUILTIN_SRCP, plus_expr_K);
      RExpFrac = createStmt(expFrac_type, roundAdd);
   }

   auto out_nan = TreeM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(bool_type));
   tree_nodeRef sign_test;
   if(inFF->sign != outFF->sign)
   {
      if(inFF->sign == bit_lattice::U)
      {
         // Shift input value to last bit
         const auto signShift = tree_man->create_binary_operation(in_type, ssa, TreeM->CreateUniqueIntegerCst(inFF->exp_bits + inFF->frac_bits, in_type_idx), BUILTIN_SRCP, rshift_expr_K);
         auto Sign = createStmt(in_type, signShift);

         // Mask input sign bit
         const auto signAnd = tree_man->create_binary_operation(in_type, Sign, TreeM->CreateUniqueIntegerCst(1, in_type_idx), BUILTIN_SRCP, bit_and_expr_K);
         Sign = createStmt(in_type, signAnd);

         // Cast to bool
         const auto signCast = tree_man->CreateNopExpr(Sign, bool_type, tree_nodeRef(), tree_nodeRef());
         sign_test = createStmt(bool_type, signCast);

         // Compare with fixed output sign
         const auto signCmp = tree_man->create_binary_operation(bool_type, sign_test, TreeM->CreateUniqueIntegerCst(outFF->sign == bit_lattice::ONE ? 1 : 0, GET_INDEX_NODE(bool_type)), BUILTIN_SRCP, ne_expr_K);
         out_nan = createStmt(bool_type, signCmp);
      }
      else
      {
         THROW_ERROR("Casting from fixed " + STR(inFF->sign == bit_lattice::ONE ? "negative" : "positive") + " type to fixed " + STR(outFF->sign == bit_lattice::ONE ? "negative" : "positive") + " type will always result in a static value.");
         return nullptr;
      }
   }

   tree_nodeRef expMax;
   if(inFF->has_nan)
   {
      // Check if input exponent is max
      const auto expCmp = tree_man->create_binary_operation(bool_type, Exp, TreeM->CreateUniqueIntegerCst((1 << inFF->exp_bits) - 1, in_type_idx), BUILTIN_SRCP, eq_expr_K);
      expMax = createStmt(bool_type, expCmp);

      // Or with the rest
      const auto nanOr = tree_man->create_binary_operation(bool_type, expMax, out_nan, BUILTIN_SRCP, bit_ior_expr_K);
      out_nan = createStmt(bool_type, nanOr);
   }

   const auto expFix = inFF->exp_bias - outFF->exp_bias;
   const auto rangeDiff = ((1LL << outFF->exp_bits) - 1) - ((1LL << inFF->exp_bits) - 1);
   if((((inFF->exp_bits != outFF->exp_bits) || (inFF->exp_bias != outFF->exp_bias)) && (expFix < 0 || (expFix > rangeDiff && (not outFF->has_nan)))) || ((inFF->has_rounding || outFF->has_rounding) && not outFF->has_nan))
   {
      // Shift right rounded exponent to last bit
      const auto rexpRShift = tree_man->create_binary_operation(exp_type, RExpFrac, TreeM->CreateUniqueIntegerCst(outFF->exp_bits + outFF->frac_bits, exp_type_idx), BUILTIN_SRCP, rshift_expr_K);
      auto UnOvflow = createStmt(exp_type, rexpRShift);

      // Mask last bit of fixed exponent
      // TODO: maybe useless?
      const auto fexpAnd = tree_man->create_binary_operation(exp_type, UnOvflow, TreeM->CreateUniqueIntegerCst(1, exp_type_idx), BUILTIN_SRCP, bit_and_expr_K);
      UnOvflow = createStmt(exp_type, fexpAnd);

      // Cast to bool
      const auto ufCast = tree_man->CreateNopExpr(UnOvflow, bool_type, tree_nodeRef(), tree_nodeRef());
      UnOvflow = createStmt(bool_type, ufCast);

      // Or with the rest
      const auto nanOr = tree_man->create_binary_operation(bool_type, UnOvflow, out_nan, BUILTIN_SRCP, bit_ior_expr_K);
      out_nan = createStmt(bool_type, nanOr);
   }

   if(((inFF->exp_bits != outFF->exp_bits) || (inFF->exp_bias != outFF->exp_bias)) && expFix > rangeDiff && outFF->has_nan)
   {
      // Check if fixed exponent is already greater than max output exponent value minus one
      const auto fexpGT = tree_man->create_binary_operation(bool_type, FExp, TreeM->CreateUniqueIntegerCst((1LL << outFF->exp_bits) - 2, tree_helper::CGetType(GET_NODE(FExp))->index), BUILTIN_SRCP, gt_expr_K);
      auto Overflow = createStmt(bool_type, fexpGT);

      // Or with the rest
      const auto nanOr = tree_man->create_binary_operation(bool_type, Overflow, out_nan, BUILTIN_SRCP, bit_ior_expr_K);
      out_nan = createStmt(bool_type, nanOr);
   }

   // Mask rounded exponent bits
   const auto rexpMask = tree_man->create_binary_operation(expFrac_type, RExpFrac, TreeM->CreateUniqueIntegerCst(((1LL << outFF->exp_bits) - 1) << outFF->frac_bits, expFrac_type_idx), BUILTIN_SRCP, bit_and_expr_K);
   RExp = createStmt(expFrac_type, rexpMask);

   // Cast rounded exponent to output type
   const auto rexpCast = tree_man->CreateNopExpr(RExp, out_type, tree_nodeRef(), tree_nodeRef());
   RExp = createStmt(out_type, rexpCast);

   // Ternary if for exponent nan
   const auto terRExp = tree_man->create_ternary_operation(out_type, out_nan, TreeM->CreateUniqueIntegerCst(((1LL << outFF->exp_bits) - 1) << outFF->frac_bits, out_type_idx), RExp, BUILTIN_SRCP, cond_expr_K);
   RExp = createStmt(out_type, terRExp);

   if(inFF->sign != outFF->sign)
   {
      THROW_ASSERT(sign_test, "Sign test should have been computed here.");

      // Add sign test to out_nan for significand nan test
      const auto nanOr = tree_man->create_binary_operation(bool_type, sign_test, out_nan, BUILTIN_SRCP, bit_ior_expr_K);
      out_nan = createStmt(bool_type, nanOr);
   }
   tree_nodeRef NFrac;
   if(outFF->has_nan)
   {
      if(inFF->has_nan)
      {
         THROW_ASSERT(expMax, "");
         // Shift input significand to get msb
         const auto nfracShift = tree_man->create_binary_operation(in_type, Frac, TreeM->CreateUniqueIntegerCst(inFF->frac_bits - 1, in_type_idx), BUILTIN_SRCP, rshift_expr_K);
         auto nanFrac = createStmt(in_type, nfracShift);

         // Mask remaining significand bit
         const auto nfracMask = tree_man->create_binary_operation(in_type, nanFrac, TreeM->CreateUniqueIntegerCst(1, in_type_idx), BUILTIN_SRCP, bit_and_expr_K);
         nanFrac = createStmt(in_type, nfracMask);

         // Cast bit to bool
         const auto nfracCast = tree_man->CreateNopExpr(nanFrac, bool_type, tree_nodeRef(), tree_nodeRef());
         nanFrac = createStmt(bool_type, nfracCast);

         // Check if input is NaN
         const auto nanFracAndNan = tree_man->create_binary_operation(bool_type, nanFrac, expMax, BUILTIN_SRCP, bit_ior_expr_K);
         auto nan = createStmt(bool_type, nanFracAndNan);

         // TODO: Maybe better to define multiple outputs from nan with left/right shift as 'NFrac = (((int32_t)nan << 31) >> 31) & ((1 << outFF->frac_bits) - 1)'
         const auto nanTer = tree_man->create_ternary_operation(out_type, nan, TreeM->CreateUniqueIntegerCst((1LL << outFF->frac_bits) - 1, out_type_idx), TreeM->CreateUniqueIntegerCst(0, out_type_idx), BUILTIN_SRCP, cond_expr_K);
         NFrac = createStmt(out_type, nanTer);
      }
      else
      {
         NFrac = TreeM->CreateUniqueIntegerCst(0, out_type_idx);
      }
   }
   else
   {
      NFrac = TreeM->CreateUniqueIntegerCst((1LL << outFF->frac_bits) - 1, out_type_idx);
   }

   // Mask rounded significand bits
   const auto maskRFrac = tree_man->create_binary_operation(expFrac_type, RExpFrac, TreeM->CreateUniqueIntegerCst((1 << outFF->frac_bits) - 1, expFrac_type_idx), BUILTIN_SRCP, bit_and_expr_K);
   auto RFrac = createStmt(expFrac_type, maskRFrac);

   // Cast rounded significand to output type
   const auto castRFrac = tree_man->CreateNopExpr(RFrac, out_type, tree_nodeRef(), tree_nodeRef());
   RFrac = createStmt(out_type, castRFrac);

   // Ternary if for significand nan test
   const auto terRFrac = tree_man->create_ternary_operation(out_type, out_nan, NFrac, RFrac, BUILTIN_SRCP, cond_expr_K);
   RFrac = createStmt(out_type, terRFrac);

   // Pack exponent and significand
   const auto expFracOr = tree_man->create_binary_operation(out_type, RExp, RFrac, BUILTIN_SRCP, bit_ior_expr_K);
   auto out = createStmt(out_type, expFracOr);
   if(outFF->sign == bit_lattice::U)
   {
      tree_nodeRef FSign;
      if(inFF->sign != bit_lattice::U)
      {
         FSign = TreeM->CreateUniqueIntegerCst(inFF->sign == bit_lattice::ONE ? (1LL << (outFF->exp_bits + outFF->frac_bits)) : 0, out_type_idx);
      }
      else
      {
         // Shift input value to last bit
         const auto signShift = tree_man->create_binary_operation(in_type, ssa, TreeM->CreateUniqueIntegerCst(inFF->exp_bits + inFF->frac_bits, in_type_idx), BUILTIN_SRCP, rshift_expr_K);
         auto Sign = createStmt(in_type, signShift);

         // Cast sign to output type
         const auto signCast = tree_man->CreateNopExpr(Sign, out_type, tree_nodeRef(), tree_nodeRef());
         FSign = createStmt(out_type, signCast);

         // Shift sign bit left in place
         const auto signLShift = tree_man->create_binary_operation(out_type, FSign, TreeM->CreateUniqueIntegerCst(outFF->exp_bits + outFF->frac_bits, out_type_idx), BUILTIN_SRCP, lshift_expr_K);
         FSign = createStmt(out_type, signLShift);
      }

      // Pack sign
      const auto outOr = tree_man->create_binary_operation(out_type, FSign, out, BUILTIN_SRCP, bit_ior_expr_K);
      out = createStmt(out_type, outOr);
   }

   return out;
}

tree_nodeRef soft_float_cg_ext::floatNegate(tree_nodeRef op, refcount<FloatFormat> ff) const
{
   if(ff->sign == bit_lattice::U)
   {
      const auto int_ff_type = tree_man->create_integer_type_with_prec(1U + ff->exp_bits + ff->frac_bits, true);
      return tree_man->create_binary_operation(int_ff_type, op, TreeM->CreateUniqueIntegerCst(1LL << (ff->exp_bits + ff->frac_bits), GET_INDEX_NODE(int_ff_type)), BUILTIN_SRCP, bit_xor_expr_K);
   }
   else
   {
      THROW_ERROR("Negate operation on fixed sign type will flatten all values.");
      return nullptr;
   }
}

tree_nodeRef soft_float_cg_ext::floatAbs(tree_nodeRef op, refcount<FloatFormat> ff) const
{
   if(ff->sign == bit_lattice::U)
   {
      const auto int_ff_type = tree_man->create_integer_type_with_prec(1U + ff->exp_bits + ff->frac_bits, true);
      return tree_man->create_binary_operation(int_ff_type, op, TreeM->CreateUniqueIntegerCst((1LL << (ff->exp_bits + ff->frac_bits)) - 1, GET_INDEX_NODE(int_ff_type)), BUILTIN_SRCP, bit_and_expr_K);
   }
   else if(ff->sign == bit_lattice::ONE)
   {
      // Fixed positive sign representation is always positive already
      return op;
   }
   else
   {
      THROW_ERROR("Negate operation on fixed negative sign type will flatten all values.");
      return nullptr;
   }
}

void soft_float_cg_ext::replaceWithCall(const std::string& fu_name, const std::vector<tree_nodeRef>& args, tree_nodeRef current_statement, tree_nodeRef current_tree_node, const std::string& current_srcp)
{
   unsigned int called_function_id = TreeM->function_index(fu_name);
   THROW_ASSERT(called_function_id, "The library miss this function " + fu_name);
   THROW_ASSERT(AppM->GetFunctionBehavior(called_function_id)->GetBehavioralHelper()->has_implementation(), "inconsistent behavioral helper");
   if(not _version->std_format())
   {
      const auto spec_suffix = (_version->userRequired != nullptr ? _version->userRequired->mngl() : "");
      auto spec_funcion_id = TreeM->function_index(fu_name + spec_suffix);
      if(spec_funcion_id == 0)
      {
         const auto spec_func = tree_man->CloneFunction(TreeM->GetTreeReindex(called_function_id), spec_suffix);
         spec_funcion_id = GET_INDEX_CONST_NODE(spec_func);
         THROW_ASSERT(spec_funcion_id, "Cloned function not correctly computed");

         // TODO: append specialization arguments to args
      }
      called_function_id = spec_funcion_id;
   }

   TreeM->ReplaceTreeNode(current_statement, current_tree_node, tree_man->CreateCallExpr(TreeM->GetTreeReindex(called_function_id), args, current_srcp));
   if(!AppM->GetCallGraphManager()->IsVertex(called_function_id))
   {
      BehavioralHelperRef helper = BehavioralHelperRef(new BehavioralHelper(AppM, called_function_id, true, parameters));
      FunctionBehaviorRef fb = FunctionBehaviorRef(new FunctionBehavior(AppM, helper, parameters));
      AppM->GetCallGraphManager()->AddFunctionAndCallPoint(function_id, called_function_id, current_statement->index, fb, FunctionEdgeInfo::CallType::direct_call);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Call graph updated with call to " + STR(called_function_id));
   }
   else
   {
      AppM->GetCallGraphManager()->AddCallPoint(function_id, called_function_id, current_statement->index, FunctionEdgeInfo::CallType::direct_call);
   }

   // Update functions float format map
   const auto called_func_vertex = AppM->CGetCallGraphManager()->GetVertex(called_function_id);
   const auto calledFF = refcount<FunctionVersion>(new FunctionVersion(called_func_vertex, _version->userRequired));
#if HAVE_ASSERTS
   const auto res =
#endif
       funcFF.insert(std::make_pair(called_func_vertex, calledFF));
   THROW_ASSERT(res.second || *res.first->second == *calledFF, "Same function registered with different formats: " + res.first->second->ToString() + " and " + calledFF->ToString());
}

void soft_float_cg_ext::RecursiveExaminate(const tree_nodeRef current_statement, const tree_nodeRef current_tree_node, int type_interface)
{
   THROW_ASSERT(current_tree_node->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   THROW_ASSERT((type_interface & 3) == INTERFACE_TYPE_NONE || (type_interface & 3) == INTERFACE_TYPE_INPUT || (type_interface & 3) == INTERFACE_TYPE_OUTPUT, "Required interface type must be unique (" + STR(type_interface) + ")");
   const auto curr_tn = GET_NODE(current_tree_node);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Update recursively (" + STR(current_tree_node->index) + " " + curr_tn->get_kind_text() + ") " + STR(current_tree_node));
   const auto current_srcp = [curr_tn]() -> std::string {
      const auto srcp_tn = GetPointer<const srcp>(curr_tn);
      if(srcp_tn)
      {
         return srcp_tn->include_name + ":" + STR(srcp_tn->line_number) + ":" + STR(srcp_tn->column_number);
      }
      return "";
   }();
   const auto is_internal_call = [&](const tree_nodeRef& fn) -> bool {
      static const auto mcpy_id = TreeM->function_index(MEMCPY);
      static const auto mset_id = TreeM->function_index(MEMSET);
      const auto fn_fd = GetPointerS<function_decl>(GET_NODE(fn));
      if(!AppM->CGetFunctionBehavior(fn_fd->index)->CGetBehavioralHelper()->has_implementation())
      {
         if(!_version->std_format() && tree_helper::print_function_name(TreeM, fn_fd) == BUILTIN_WAIT_CALL)
         {
            THROW_UNREACHABLE("Function pointers not supported from user defined floating point format functions");
            // TODO: maybe it could be possible to only warn the user here to be careful about the pointed function definition and go on
         }
         return _version->std_format();
      }
      const auto fn_v = AppM->CGetCallGraphManager()->GetVertex(GET_INDEX_CONST_NODE(fn));
      if(fn_fd->builtin_flag || fn_fd->index == mcpy_id || fn_fd->index == mset_id)
      {
         return _version->std_format();
      }
      else
      {
         THROW_ASSERT(static_cast<bool>(funcFF.count(fn_v)), "Called function should have been already computed.");
         const auto& fn_FF = funcFF.at(fn_v);
         return fn_FF->internal && _version->std_format() == fn_FF->std_format();
      }
   };
   const auto ExaminateFunctionCall = [&](tree_nodeRef fn) -> int {
      const auto ae = GetPointer<addr_expr>(GET_NODE(fn));
      if(ae)
      {
         fn = ae->op;
         const auto called_fd = GetPointerS<const function_decl>(GET_CONST_NODE(fn));
         ae->type = GET_CONST_NODE(called_fd->type)->get_kind() == pointer_type_K ? called_fd->type : tree_man->create_pointer_type(called_fd->type, ALGN_POINTER);
         modified = true;
      }
      if(tree_helper::print_function_name(TreeM, GetPointerS<const function_decl>(GET_CONST_NODE(fn))) == BUILTIN_WAIT_CALL)
      {
         if(_version->std_format())
         {
            return INTERFACE_TYPE_NONE;
         }
         THROW_UNREACHABLE("Function pointers not supported from user defined floating point format functions");
         // TODO: maybe it could be possible to only warn the user here to be careful about the pointed function definition and go on
      }

      int type_i = is_internal_call(fn) ? INTERFACE_TYPE_NONE : INTERFACE_TYPE_OUTPUT;
      // Hardware implemented functions need arguments to still be real_type, thus it is necessary to add a view_convert operation before
      if(!AppM->CGetFunctionBehavior(GET_INDEX_CONST_NODE(fn))->CGetBehavioralHelper()->has_implementation())
      {
         type_i |= INTERFACE_TYPE_REAL;
      }
      return type_i;
   };
   switch(curr_tn->get_kind())
   {
      case call_expr_K:
      case aggr_init_expr_K:
      {
         const auto ce = GetPointerS<const call_expr>(curr_tn);
         type_interface = ExaminateFunctionCall(ce->fn);
         for(const auto& arg : ce->args)
         {
            RecursiveExaminate(current_statement, arg, type_interface);
         }
         break;
      }
      case gimple_call_K:
      {
         const auto ce = GetPointerS<const gimple_call>(curr_tn);
         type_interface = ExaminateFunctionCall(ce->fn);
         for(const auto& arg : ce->args)
         {
            RecursiveExaminate(current_statement, arg, type_interface);
         }
         break;
      }
      case gimple_phi_K:
      {
         const auto gp = GetPointerS<const gimple_phi>(curr_tn);
         RecursiveExaminate(current_statement, gp->res, type_interface);
         for(const auto& de : gp->CGetDefEdgesList())
         {
            RecursiveExaminate(current_statement, de.first, type_interface);
         }
         break;
      }
      case gimple_assign_K:
      {
         const auto ga = GetPointerS<const gimple_assign>(curr_tn);
         const auto rhs_type = GET_NODE(ga->op1)->get_kind();
         if(rhs_type == call_expr_K || rhs_type == aggr_init_expr_K)
         {
            const auto ce = GetPointerS<const call_expr>(GET_NODE(ga->op1));
            const auto fn = GetPointer<const addr_expr>(GET_CONST_NODE(ce->fn)) ? GetPointerS<const addr_expr>(GET_CONST_NODE(ce->fn))->op : ce->fn;
            const auto type_i = [&]() {
               // Return values associated to non-internal calls need to be cast renamed to local float format
               int ti = is_internal_call(fn) ? type_interface : INTERFACE_TYPE_INPUT;

               // Hardware implemented functions need the return value to still be real_type, thus it is necessary to add a view_convert operation after
               if(tree_helper::print_function_name(TreeM, GetPointerS<const function_decl>(GET_CONST_NODE(fn))) != BUILTIN_WAIT_CALL && !AppM->CGetFunctionBehavior(GET_INDEX_CONST_NODE(fn))->CGetBehavioralHelper()->has_implementation())
               {
                  ti |= INTERFACE_TYPE_REAL;
               }
               return ti;
            }();
            RecursiveExaminate(current_statement, ga->op0, type_i);
         }
         else if(tree_helper::IsLoad(TreeM, current_statement, function_behavior->get_function_mem()))
         {
            // Values loaded from memory need to be cast renamed to local float format
            RecursiveExaminate(current_statement, ga->op0, INTERFACE_TYPE_INPUT);
         }
         else
         {
            RecursiveExaminate(current_statement, ga->op0, type_interface);
         }
         if(tree_helper::IsStore(TreeM, current_statement, function_behavior->get_function_mem()))
         {
            // Values stored to memory need to be cast renamed before the store statement
            RecursiveExaminate(current_statement, ga->op1, INTERFACE_TYPE_OUTPUT);
         }
         else
         {
            RecursiveExaminate(current_statement, ga->op1, type_interface);
         }
         if(ga->predicate)
         {
            RecursiveExaminate(current_statement, ga->predicate, type_interface);
         }
         break;
      }
      case gimple_nop_K:
      {
         break;
      }
      case var_decl_K:
      case parm_decl_K:
      {
         break;
      }
      case ssa_name_K:
      {
         const auto SSA = GetPointerS<ssa_name>(curr_tn);
         if(lowering_needed(TreeM, SSA))
         {
            // Real variables must all be converted to unsigned integers after softfloat lowering operations
            viewConvert.insert(SSA);

            if(type_interface & INTERFACE_TYPE_REAL)
            {
               if(GET_INDEX_CONST_NODE(SSA->CGetDefStmt()) == GET_INDEX_CONST_NODE(current_statement))
               {
                  hwReturn.push_back(SSA);
               }
               else
               {
                  hwParam[SSA].insert(GET_INDEX_CONST_NODE(current_statement));
               }
            }
         }

         // Search for ssa variables associated to input parameters
         if(!bindingCompleted)
         {
            const auto& args = fd->list_of_args;
            // If ssa_name references a parm_decl and is defined by a gimple_nop, it represents the formal function parameter inside the function body
            if(SSA->var != nullptr && GET_CONST_NODE(SSA->var)->get_kind() == parm_decl_K && GET_CONST_NODE(SSA->CGetDefStmt())->get_kind() == gimple_nop_K)
            {
               auto argIt = std::find_if(args.begin(), args.end(), [&](const tree_nodeRef& arg) { return GET_INDEX_CONST_NODE(arg) == GET_INDEX_CONST_NODE(SSA->var); });
               THROW_ASSERT(argIt != args.end(), "parm_decl associated with ssa_name not found in function parameters");
               const auto arg_pos = static_cast<size_t>(argIt - args.begin());
               THROW_ASSERT(arg_pos < args.size(), "Computed parameter position outside actual parameters number");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variable " + SSA->ToString() + " is defined from parameter " + STR(arg_pos));
               paramBinding[arg_pos] = curr_tn;
               bindingCompleted = std::find(paramBinding.begin(), paramBinding.end(), nullptr) == paramBinding.end();
            }
         }

         if(!_version->std_format() && GET_CONST_NODE(SSA->type)->get_kind() == real_type_K)
         {
            if((!_version->internal && std::find(paramBinding.begin(), paramBinding.end(), curr_tn) != paramBinding.end()) || type_interface & INTERFACE_TYPE_INPUT)
            {
               if(type_interface & INTERFACE_TYPE_OUTPUT)
               {
                  // Considered ssa has been discovered to be a function parameter and is used in current statement as a non-internal function argument, thus conversion can be avoided
                  inputInterface[SSA].push_back(GET_INDEX_CONST_NODE(current_statement));
                  break;
               }
               if(!static_cast<bool>(inputInterface.count(SSA)))
               {
                  // Add current input SSA to the input cast rename list for all its uses if not already present
                  inputInterface.insert({SSA, {}});
               }
            }
            else if(type_interface & INTERFACE_TYPE_OUTPUT)
            {
               // Add current output SSA to the output cast rename list for its uses in current statement
               outputInterface[SSA].push_back(current_statement);
            }
         }

         break;
      }
      case tree_list_K:
      {
         auto current = current_tree_node;
         while(current)
         {
            RecursiveExaminate(current_statement, GetPointerS<const tree_list>(GET_CONST_NODE(current))->valu, type_interface);
            current = GetPointerS<const tree_list>(GET_CONST_NODE(current))->chan;
         }
         break;
      }
      case CASE_UNARY_EXPRESSION:
      {
         const auto ue = GetPointerS<const unary_expr>(curr_tn);
         const auto expr_type = GET_CONST_NODE(ue->type);
         const auto op_expr_type = tree_helper::CGetType(GET_CONST_NODE(ue->op));
         // Propagate recursion with INTERFACE_TYPE_NONE to avoid cast rename of internal variables (input parameters and constant will be converted anyway)
         RecursiveExaminate(current_statement, ue->op, INTERFACE_TYPE_NONE);
         if(expr_type->get_kind() == real_type_K)
         {
            switch(curr_tn->get_kind())
            {
               case float_expr_K:
               {
                  auto bitsize_in = tree_helper::Size(op_expr_type);
                  if(bitsize_in < 32)
                  {
                     bitsize_in = 32;
                  }
                  else if(bitsize_in > 32 && bitsize_in < 64)
                  {
                     bitsize_in = 64;
                  }
                  auto bitsize_out = tree_helper::Size(expr_type);
                  if(bitsize_out < 32)
                  {
                     bitsize_out = 32;
                  }
                  else if(bitsize_out > 32 && bitsize_out < 64)
                  {
                     bitsize_out = 64;
                  }
                  const auto bitsize_str_in = bitsize_in == 96 ? "x80" : STR(bitsize_in);
                  const auto bitsize_str_out = bitsize_out == 96 ? "x80" : STR(bitsize_out);
                  std::string fu_name;
                  if(op_expr_type->get_kind() != real_type_K)
                  {
                     if(tree_helper::is_unsigned(TreeM, op_expr_type->index))
                     {
                        fu_name = "__uint" + bitsize_str_in + "_to_float" + bitsize_str_out;
                     }
                     else
                     {
                        fu_name = "__int" + bitsize_str_in + "_to_float" + bitsize_str_out;
                     }
                     replaceWithCall(fu_name, {ue->op}, current_statement, current_tree_node, current_srcp);
                     modified = true;
                  }
                  break;
               }
               case view_convert_expr_K:
               {
                  // BEAWARE: this view_convert is from integer to real type, thus it will be a def statement of a real type ssa_name
                  //          def statements of real type ssa_name variables will be correctly replaced in the next phase of this step
                  break;
               }
               case abs_expr_K:
               {
                  const auto float_size = tree_helper::Size(op_expr_type);
                  THROW_ASSERT(float_size == 32 || float_size == 64, "Unhandled floating point format (size = " + STR(float_size) + ")");
                  const auto ff = _version->std_format() ? (float_size == 32 ? float32FF : float64FF) : _version->userRequired;
                  THROW_ASSERT(ff, "Float format should be defined here");
                  const auto float_negate = floatAbs(ue->op, ff);
                  TreeM->ReplaceTreeNode(current_statement, current_tree_node, float_negate);
                  break;
               }
               case negate_expr_K:
               {
                  const auto float_size = tree_helper::Size(op_expr_type);
                  THROW_ASSERT(float_size == 32 || float_size == 64, "Unhandled floating point format (size = " + STR(float_size) + ")");
                  const auto ff = _version->std_format() ? (float_size == 32 ? float32FF : float64FF) : _version->userRequired;
                  THROW_ASSERT(ff, "Float format should be defined here");
                  const auto float_negate = floatNegate(ue->op, ff);
                  TreeM->ReplaceTreeNode(current_statement, current_tree_node, float_negate);
                  break;
               }
               case nop_expr_K:
               {
                  if(_version->std_format())
                  {
                     unsigned int bitsize_in = tree_helper::Size(op_expr_type);
                     unsigned int bitsize_out = tree_helper::Size(expr_type);
                     THROW_ASSERT(bitsize_in == 32 || bitsize_in == 64, "Unhandled input floating point format (size = " + STR(bitsize_in) + ")");
                     THROW_ASSERT(bitsize_out == 32 || bitsize_out == 64, "Unhandled output floating point format (size = " + STR(bitsize_out) + ")");
                     if(tree_helper::is_real(TreeM, op_expr_type->index))
                     {
                        const auto fu_name = "__float" + STR(bitsize_in) + "_to_float" + STR(bitsize_out);
                        replaceWithCall(fu_name, {ue->op}, current_statement, current_tree_node, current_srcp);
                     }
                     else
                     {
                        const auto vc_expr = tree_man->create_unary_operation(TreeM->CGetTreeReindex(expr_type->index), ue->op, current_srcp, view_convert_expr_K);
                        TreeM->ReplaceTreeNode(current_statement, current_tree_node, vc_expr);
                     }
                     modified = true;
                  }
                  else
                  {
                     THROW_UNREACHABLE("Operation not yet supported: function with user-defined floating point format should use a unique format type.");
                  }
                  break;
               }
               case indirect_ref_K:
               case imagpart_expr_K:
               case realpart_expr_K:
               case paren_expr_K:
                  break;
               case addr_expr_K:
               case arrow_expr_K:
               case bit_not_expr_K:
               case buffer_ref_K:
               case card_expr_K:
               case cleanup_point_expr_K:
               case conj_expr_K:
               case convert_expr_K:
               case exit_expr_K:
               case fix_ceil_expr_K:
               case fix_floor_expr_K:
               case fix_round_expr_K:
               case fix_trunc_expr_K:
               case misaligned_indirect_ref_K:
               case loop_expr_K:
               case non_lvalue_expr_K:
               case reference_expr_K:
               case reinterpret_cast_expr_K:
               case sizeof_expr_K:
               case static_cast_expr_K:
               case throw_expr_K:
               case truth_not_expr_K:
               case unsave_expr_K:
               case va_arg_expr_K:
               case reduc_max_expr_K:
               case reduc_min_expr_K:
               case reduc_plus_expr_K:
               case vec_unpack_hi_expr_K:
               case vec_unpack_lo_expr_K:
               case vec_unpack_float_hi_expr_K:
               case vec_unpack_float_lo_expr_K:
               case binfo_K:
               case block_K:
               case call_expr_K:
               case aggr_init_expr_K:
               case case_label_expr_K:
               case constructor_K:
               case identifier_node_K:
               case ssa_name_K:
               case statement_list_K:
               case target_expr_K:
               case target_mem_ref_K:
               case target_mem_ref461_K:
               case tree_list_K:
               case tree_vec_K:
               case error_mark_K:
               case lut_expr_K:
               case CASE_BINARY_EXPRESSION:
               case CASE_CPP_NODES:
               case CASE_CST_NODES:
               case CASE_DECL_NODES:
               case CASE_FAKE_NODES:
               case CASE_GIMPLE_NODES:
               case CASE_PRAGMA_NODES:
               case CASE_QUATERNARY_EXPRESSION:
               case CASE_TERNARY_EXPRESSION:
               case CASE_TYPE_NODES:
               {
                  THROW_ERROR("not yet supported soft float function: " + curr_tn->get_kind_text());
                  break;
               }
               default:
               {
                  THROW_UNREACHABLE("");
               }
            }
         }
         if(op_expr_type->get_kind() == real_type_K)
         {
            switch(curr_tn->get_kind())
            {
               case fix_trunc_expr_K:
               {
                  const auto bitsize_in = tree_helper::Size(op_expr_type);
                  auto bitsize_out = tree_helper::Size(expr_type);
                  if(bitsize_out < 32)
                  {
                     bitsize_out = 32;
                  }
                  else if(bitsize_out > 32 && bitsize_out < 64)
                  {
                     bitsize_out = 64;
                  }
                  const auto is_unsigned = tree_helper::is_unsigned(TreeM, expr_type->index);
                  const auto bitsize_str_in = bitsize_in == 96 ? "x80" : STR(bitsize_in);
                  const auto bitsize_str_out = bitsize_out == 96 ? "x80" : STR(bitsize_out);
                  const auto fu_name = "__float" + bitsize_str_in + "_to_" + (is_unsigned ? "u" : "") + "int" + bitsize_str_out + "_round_to_zero";
                  replaceWithCall(fu_name, {ue->op}, current_statement, current_tree_node, current_srcp);
                  modified = true;
                  break;
               }
               case view_convert_expr_K:
               {
                  // This view_convert is from real to integer type variable, thus needs to be stored in the type conversion set of statements to be converted later if necessary
                  nopConvert.push_back(current_statement);
                  break;
               }
               case binfo_K:
               case block_K:
               case call_expr_K:
               case aggr_init_expr_K:
               case case_label_expr_K:
               case constructor_K:
               case identifier_node_K:
               case ssa_name_K:
               case statement_list_K:
               case target_expr_K:
               case target_mem_ref_K:
               case target_mem_ref461_K:
               case tree_list_K:
               case tree_vec_K:
               case lut_expr_K:
               case CASE_BINARY_EXPRESSION:
               case CASE_CPP_NODES:
               case CASE_CST_NODES:
               case CASE_DECL_NODES:
               case CASE_FAKE_NODES:
               case CASE_GIMPLE_NODES:
               case CASE_PRAGMA_NODES:
               case CASE_QUATERNARY_EXPRESSION:
               case CASE_TERNARY_EXPRESSION:
               case CASE_TYPE_NODES:
               case abs_expr_K:
               case addr_expr_K:
               case arrow_expr_K:
               case bit_not_expr_K:
               case buffer_ref_K:
               case card_expr_K:
               case cleanup_point_expr_K:
               case conj_expr_K:
               case convert_expr_K:
               case exit_expr_K:
               case fix_ceil_expr_K:
               case fix_floor_expr_K:
               case fix_round_expr_K:
               case float_expr_K:
               case imagpart_expr_K:
               case indirect_ref_K:
               case misaligned_indirect_ref_K:
               case loop_expr_K:
               case negate_expr_K:
               case non_lvalue_expr_K:
               case nop_expr_K:
               case paren_expr_K:
               case realpart_expr_K:
               case reference_expr_K:
               case reinterpret_cast_expr_K:
               case sizeof_expr_K:
               case static_cast_expr_K:
               case throw_expr_K:
               case truth_not_expr_K:
               case unsave_expr_K:
               case va_arg_expr_K:
               case reduc_max_expr_K:
               case reduc_min_expr_K:
               case reduc_plus_expr_K:
               case vec_unpack_hi_expr_K:
               case vec_unpack_lo_expr_K:
               case vec_unpack_float_hi_expr_K:
               case vec_unpack_float_lo_expr_K:
               case error_mark_K:
                  break;
               default:
               {
                  THROW_UNREACHABLE("");
               }
            }
         }
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         const auto be = GetPointerS<binary_expr>(curr_tn);
         // Get operand type before recursive examination because floating point operands may be converted to unsigned integer during during recursion
         const auto expr_type = tree_helper::CGetType(GET_CONST_NODE(be->op0));
         // Propagate recursion with INTERFACE_TYPE_NONE to avoid cast rename of internal variables (input parameters and constant will be converted anyway)
         RecursiveExaminate(current_statement, be->op0,
                            (expr_type->get_kind() == real_type_K && (curr_tn->get_kind() == unordered_expr_K || curr_tn->get_kind() == ordered_expr_K || curr_tn->get_kind() == complex_expr_K)) ? INTERFACE_TYPE_REAL : INTERFACE_TYPE_NONE);
         RecursiveExaminate(current_statement, be->op1,
                            (expr_type->get_kind() == real_type_K && (curr_tn->get_kind() == unordered_expr_K || curr_tn->get_kind() == ordered_expr_K || curr_tn->get_kind() == complex_expr_K)) ? INTERFACE_TYPE_REAL : INTERFACE_TYPE_NONE);
         if(expr_type->get_kind() == real_type_K)
         {
            bool add_call = true;
            std::string fu_suffix;
            switch(curr_tn->get_kind())
            {
               case mult_expr_K:
               {
                  fu_suffix = "mul";
                  break;
               }
               case plus_expr_K:
               {
                  fu_suffix = "add";
                  break;
               }
               case minus_expr_K:
               {
                  fu_suffix = "sub";
                  break;
               }
               case rdiv_expr_K:
               {
                  fu_suffix = "div";
                  const auto bitsize = tree_helper::Size(expr_type);
                  if(bitsize == 32 || bitsize == 64)
                  {
                     THROW_ASSERT(parameters->isOption(OPT_hls_fpdiv), "a default is expected");
                     if(parameters->getOption<std::string>(OPT_hls_fpdiv) == "SRT4")
                     {
                        fu_suffix = fu_suffix + "SRT4";
                     }
                     else if(parameters->getOption<std::string>(OPT_hls_fpdiv) == "G")
                     {
                        fu_suffix = fu_suffix + "G";
                     }
                     else if(parameters->getOption<std::string>(OPT_hls_fpdiv) == "SF")
                     {
                        // do nothing
                     }
                     else
                     {
                        THROW_ERROR("FP-Division algorithm not supported:" + parameters->getOption<std::string>(OPT_hls_fpdiv));
                     }
                  }
                  break;
               }
               case gt_expr_K:
               {
                  fu_suffix = "gt";
                  break;
               }
               case ge_expr_K:
               {
                  fu_suffix = "ge";
                  break;
               }
               case lt_expr_K:
               {
                  fu_suffix = "lt";
                  break;
               }
               case le_expr_K:
               {
                  fu_suffix = "le";
                  break;
               }
               case eq_expr_K:
               {
                  fu_suffix = "eq";
                  break;
               }
               case ne_expr_K:
               case ltgt_expr_K:
               {
                  fu_suffix = "ltgt_quiet";
                  break;
               }
               case uneq_expr_K:
               case unge_expr_K:
               case ungt_expr_K:
               case unle_expr_K:
               case unlt_expr_K:
               {
                  THROW_ERROR("Unsupported tree node " + curr_tn->get_kind_text());
                  break;
               }
               case assert_expr_K:
               case bit_and_expr_K:
               case bit_ior_expr_K:
               case bit_xor_expr_K:
               case catch_expr_K:
               case ceil_div_expr_K:
               case ceil_mod_expr_K:
               case complex_expr_K:
               case compound_expr_K:
               case eh_filter_expr_K:
               case exact_div_expr_K:
               case fdesc_expr_K:
               case floor_div_expr_K:
               case floor_mod_expr_K:
               case goto_subroutine_K:
               case in_expr_K:
               case init_expr_K:
               case lrotate_expr_K:
               case lshift_expr_K:
               case lut_expr_K:
               case max_expr_K:
               case mem_ref_K:
               case min_expr_K:
               case modify_expr_K:
               case mult_highpart_expr_K:
               case ordered_expr_K:
               case pointer_plus_expr_K:
               case postdecrement_expr_K:
               case postincrement_expr_K:
               case predecrement_expr_K:
               case preincrement_expr_K:
               case range_expr_K:
               case round_div_expr_K:
               case round_mod_expr_K:
               case rrotate_expr_K:
               case rshift_expr_K:
               case set_le_expr_K:
               case trunc_div_expr_K:
               case trunc_mod_expr_K:
               case truth_and_expr_K:
               case truth_andif_expr_K:
               case truth_or_expr_K:
               case truth_orif_expr_K:
               case truth_xor_expr_K:
               case try_catch_expr_K:
               case try_finally_K:
               case unordered_expr_K:
               case widen_sum_expr_K:
               case widen_mult_expr_K:
               case with_size_expr_K:
               case vec_lshift_expr_K:
               case vec_rshift_expr_K:
               case widen_mult_hi_expr_K:
               case widen_mult_lo_expr_K:
               case vec_pack_trunc_expr_K:
               case vec_pack_sat_expr_K:
               case vec_pack_fix_trunc_expr_K:
               case vec_extracteven_expr_K:
               case vec_extractodd_expr_K:
               case vec_interleavehigh_expr_K:
               case vec_interleavelow_expr_K:
               case extract_bit_expr_K:
               case sat_plus_expr_K:
               case sat_minus_expr_K:
               {
                  add_call = false;
                  break;
               }
               case binfo_K:
               case block_K:
               case call_expr_K:
               case aggr_init_expr_K:
               case case_label_expr_K:
               case constructor_K:
               case identifier_node_K:
               case ssa_name_K:
               case statement_list_K:
               case target_expr_K:
               case target_mem_ref_K:
               case target_mem_ref461_K:
               case tree_list_K:
               case tree_vec_K:
               case error_mark_K:
               case CASE_CPP_NODES:
               case CASE_CST_NODES:
               case CASE_DECL_NODES:
               case CASE_FAKE_NODES:
               case CASE_GIMPLE_NODES:
               case CASE_PRAGMA_NODES:
               case CASE_QUATERNARY_EXPRESSION:
               case CASE_TERNARY_EXPRESSION:
               case CASE_TYPE_NODES:
               case CASE_UNARY_EXPRESSION:
               {
                  break;
               }
               default:
               {
                  THROW_UNREACHABLE("");
               }
            }
            if(add_call)
            {
               const auto bitsize = tree_helper::Size(expr_type);
               const auto bitsize_str = bitsize == 96 ? "x80" : STR(bitsize);
               const auto fu_name = "__float" + bitsize_str + "_" + fu_suffix;
               replaceWithCall(fu_name, {be->op0, be->op1}, current_statement, current_tree_node, current_srcp);
               modified = true;
            }
         }
         else if(GET_NODE(be->type)->get_kind() == real_type_K)
         {
            if(curr_tn->get_kind() == mem_ref_K)
            {
               const auto mr = GetPointerS<mem_ref>(curr_tn);
               mr->type = tree_helper::Size(GET_CONST_NODE(mr->type)) == 32 ? float32_type : float64_type;
            }
            else
            {
               THROW_UNREACHABLE("Real type expression not handled: " + curr_tn->get_kind_text() + " " + curr_tn->ToString());
            }
         }
         else if(tree_helper::is_a_pointer(TreeM, curr_tn->index) && tree_helper::CGetPointedType(GET_CONST_NODE(be->type))->get_kind() == real_type_K)
         {
            if(curr_tn->get_kind() == mem_ref_K)
            {
               const auto mr = GetPointerS<mem_ref>(curr_tn);
               mr->type = int_type_for(GET_NODE(mr->type));
            }
            else if(curr_tn->get_kind() == pointer_plus_expr_K)
            {
               const auto pp = GetPointerS<pointer_plus_expr>(curr_tn);
               pp->type = int_type_for(GET_NODE(pp->type));
            }
            else
            {
               THROW_UNREACHABLE("Real pointer type expression not handled: " + curr_tn->get_kind_text() + " " + curr_tn->ToString());
            }
         }
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         const auto te = GetPointerS<ternary_expr>(curr_tn);
         RecursiveExaminate(current_statement, te->op0, type_interface);
         if(te->op1)
         {
            RecursiveExaminate(current_statement, te->op1, type_interface);
         }
         if(te->op2)
         {
            RecursiveExaminate(current_statement, te->op2, type_interface);
         }
         break;
      }
      case CASE_QUATERNARY_EXPRESSION:
      {
         const auto qe = GetPointerS<quaternary_expr>(curr_tn);
         RecursiveExaminate(current_statement, qe->op0, type_interface);
         if(qe->op1)
         {
            RecursiveExaminate(current_statement, qe->op1, type_interface);
         }
         if(qe->op2)
         {
            RecursiveExaminate(current_statement, qe->op2, type_interface);
         }
         if(qe->op3)
         {
            RecursiveExaminate(current_statement, qe->op3, type_interface);
         }
         break;
      }
      case lut_expr_K:
      {
         const auto le = GetPointerS<lut_expr>(curr_tn);
         RecursiveExaminate(current_statement, le->op0, type_interface);
         RecursiveExaminate(current_statement, le->op1, type_interface);
         if(le->op2)
         {
            RecursiveExaminate(current_statement, le->op2, type_interface);
         }
         if(le->op3)
         {
            RecursiveExaminate(current_statement, le->op3, type_interface);
         }
         if(le->op4)
         {
            RecursiveExaminate(current_statement, le->op4, type_interface);
         }
         if(le->op5)
         {
            RecursiveExaminate(current_statement, le->op5, type_interface);
         }
         if(le->op6)
         {
            RecursiveExaminate(current_statement, le->op6, type_interface);
         }
         if(le->op7)
         {
            RecursiveExaminate(current_statement, le->op7, type_interface);
         }
         if(le->op8)
         {
            RecursiveExaminate(current_statement, le->op8, type_interface);
         }
         break;
      }
      case constructor_K:
      {
         const auto co = GetPointerS<constructor>(curr_tn);
         for(const auto& idx_valu : co->list_of_idx_valu)
         {
            RecursiveExaminate(current_statement, idx_valu.second, type_interface);
         }
         break;
      }
      case gimple_cond_K:
      {
         const auto gc = GetPointerS<gimple_cond>(curr_tn);
         RecursiveExaminate(current_statement, gc->op0, type_interface);
         break;
      }
      case gimple_switch_K:
      {
         const auto se = GetPointerS<gimple_switch>(curr_tn);
         RecursiveExaminate(current_statement, se->op0, type_interface);
         break;
      }
      case gimple_multi_way_if_K:
      {
         const auto gmwi = GetPointerS<gimple_multi_way_if>(curr_tn);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               RecursiveExaminate(current_statement, cond.first, type_interface);
            }
         }
         break;
      }
      case gimple_return_K:
      {
         const auto re = GetPointerS<gimple_return>(curr_tn);
         if(re->op)
         {
            if(isTopFunction && GET_NODE(re->op)->get_kind() == ssa_name_K && lowering_needed(TreeM, GetPointerS<ssa_name>(GET_NODE(re->op))))
            {
               topReturn.push_back(current_statement);
            }
            RecursiveExaminate(current_statement, re->op, _version->internal ? INTERFACE_TYPE_NONE : INTERFACE_TYPE_OUTPUT);
         }
         break;
      }
      case gimple_for_K:
      {
         const auto gf = GetPointerS<const gimple_for>(curr_tn);
         RecursiveExaminate(current_statement, gf->op0, type_interface);
         RecursiveExaminate(current_statement, gf->op1, type_interface);
         RecursiveExaminate(current_statement, gf->op2, type_interface);
         break;
      }
      case gimple_while_K:
      {
         const auto gw = GetPointerS<gimple_while>(curr_tn);
         RecursiveExaminate(current_statement, gw->op0, type_interface);
         break;
      }
      case CASE_TYPE_NODES:
      case type_decl_K:
      {
         break;
      }
      case target_mem_ref_K:
      {
         const auto tmr = GetPointerS<target_mem_ref>(curr_tn);
         if(tmr->symbol)
         {
            RecursiveExaminate(current_statement, tmr->symbol, type_interface);
         }
         if(tmr->base)
         {
            RecursiveExaminate(current_statement, tmr->base, type_interface);
         }
         if(tmr->idx)
         {
            RecursiveExaminate(current_statement, tmr->idx, type_interface);
         }
         break;
      }
      case target_mem_ref461_K:
      {
         const auto tmr = GetPointerS<target_mem_ref461>(curr_tn);
         if(tmr->base)
         {
            RecursiveExaminate(current_statement, tmr->base, type_interface);
         }
         if(tmr->idx)
         {
            RecursiveExaminate(current_statement, tmr->idx, type_interface);
         }
         if(tmr->idx2)
         {
            RecursiveExaminate(current_statement, tmr->idx2, type_interface);
         }
         break;
      }
      case real_cst_K:
      {
         if(~type_interface & INTERFACE_TYPE_REAL)
         {
            const auto cst = GetPointerS<real_cst>(curr_tn);
            const auto bw = tree_helper::Size(curr_tn);
            const auto fp_str = (cst->valx.front() == '-' && cst->valr.front() != cst->valx.front()) ? ("-" + cst->valr) : cst->valr;
            const auto cst_val = convert_fp_to_bits(fp_str, bw);
            const auto inFF = bw == 32 ? float32FF : float64FF;
            const auto outFF = (_version->internal && not _version->std_format() && type_interface != INTERFACE_TYPE_OUTPUT) ? _version->userRequired : inFF;

            // Perform static constant value cast and replace real type constant with converted unsigned integer type constant
            const auto int_cst = cstCast(cst_val, inFF, outFF);
            TreeM->ReplaceTreeNode(current_statement, current_tree_node, int_cst);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Real type constant " + curr_tn->ToString() + " converted to " + GET_NODE(int_cst)->ToString());
         }
         break;
      }
      case integer_cst_K:
      //    {
      //       const auto ic = GetPointerS<integer_cst>(curr_tn);
      //       if(tree_helper::is_a_pointer(TreeM, GET_CONST_NODE(ic->type)->index))
      //       {
      //          const auto ptd_type = tree_helper::CGetPointedType(GET_CONST_NODE(ic->type));
      //          if(ptd_type->get_kind() == real_type_K)
      //          {
      //             const auto int_ptr_cst = TreeM->CreateUniqueIntegerCst(ic->value, tree_helper::Size(ptd_type) == 32 ? GET_INDEX_CONST_NODE(float32_ptr_type) : GET_INDEX_CONST_NODE(float64_ptr_type));
      //             TreeM->ReplaceTreeNode(current_statement, current_tree_node, int_ptr_cst);
      //             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Real pointer type constant " + curr_tn->ToString() + " converted to " + GET_NODE(int_ptr_cst)->ToString());
      //          }
      //       }
      //       break;
      //    }
      case complex_cst_K:
      case string_cst_K:
      case field_decl_K:
      case function_decl_K:
      case label_decl_K:
      case result_decl_K:
      case vector_cst_K:
      case void_cst_K:
      case tree_vec_K:
      case case_label_expr_K:
      case gimple_label_K:
      case gimple_asm_K:
      case gimple_goto_K:
      case gimple_pragma_K:
      case CASE_PRAGMA_NODES:
         break;
      case binfo_K:
      case block_K:
      case const_decl_K:
      case CASE_CPP_NODES:
      case gimple_bind_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case identifier_node_K:
      case last_tree_K:
      case namespace_decl_K:
      case none_K:
      case placeholder_expr_K:
      case statement_list_K:
      case translation_unit_decl_K:
      case using_decl_K:
      case template_decl_K:
      case tree_reindex_K:
      case target_expr_K:
      case error_mark_K:
      {
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "Not supported node: " + std::string(curr_tn->get_kind_text()));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Updated recursively (" + STR(current_tree_node->index) + ") " + STR(current_tree_node));
   return;
}
