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

/// Utility include
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

unsigned int soft_float_cg_ext::unique_id = 0;
CustomMap<CallGraph::vertex_descriptor, FunctionVersion> soft_float_cg_ext::funcFF;

soft_float_cg_ext::soft_float_cg_ext(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, SOFT_FLOAT_CG_EXT, _design_flow_manager, _parameters), TreeM(_AppM->get_tree_manager()), tree_man(new tree_manipulation(TreeM, parameters)), modified(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);

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

         const auto function_id = TreeM->function_index(format[0]);
         if(function_id == 0)
         {
            THROW_ERROR("Function " + format[0] + " does not exists.");
         }
         const auto function_v = CGM->GetVertex(function_id);
         if(funcFF.count(function_v))
         {
            THROW_ERROR("Function " + format[0] + " already specialized.");
         }

         const auto exp_bits = static_cast<uint8_t>(strtoul(format[1].data(), nullptr, 10));
         const auto frac_bits = static_cast<uint8_t>(strtoul(format[2].data(), nullptr, 10));
         const auto exp_bias = static_cast<int32_t>(strtol(format[3].data(), nullptr, 10));
         const auto insertion = funcFF.insert({function_v, FunctionVersion(function_v, new FloatFormat(exp_bits, frac_bits, exp_bias))});
         auto& funcVersion = insertion.first->second;

         if(format.size() > 4)
         {
            funcVersion.userRequired->has_rounding = format[4] == "1";
         }
         if(format.size() > 5)
         {
            funcVersion.userRequired->has_nan = format[5] == "1";
         }
         if(format.size() > 6)
         {
            funcVersion.userRequired->has_one = format[6] == "1";
         }
         if(format.size() > 7)
         {
            funcVersion.userRequired->has_subnorm = format[7] == "1";
         }
         if(format.size() > 8)
         {
            funcVersion.userRequired->sign = format[8] == "U" ? bit_lattice::U : (format[8] == "1" ? bit_lattice::ONE : bit_lattice::ZERO);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Function " + format[0] + " required specialized arithmetic: " + funcVersion.userRequired->mngl());
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");

      // Propagate floating-point arithmetic specialization over to called functions
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
      _version = &funcFF.at(function_v);
   }
   else
   {
      const auto insertion = funcFF.insert({function_v, FunctionVersion(function_v)});
      THROW_ASSERT(insertion.second, "");
      _version = &insertion.first->second;
   }
   const auto curr_tn = TreeM->GetTreeNode(function_id);
   fd = GetPointer<function_decl>(curr_tn);
   FB = AppM->CGetFunctionBehavior(function_id);
   paramBinding = std::vector<tree_nodeRef>(fd->list_of_args.size(), nullptr);
   bindingCompleted = false;
}

soft_float_cg_ext::~soft_float_cg_ext() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> soft_float_cg_ext::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SOFT_FLOAT_CG_EXT, CALLED_FUNCTIONS));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(UN_COMPARISON_LOWERING, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(EXTRACT_GIMPLE_COND_OP, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

DesignFlowStep_Status soft_float_cg_ext::InternalExec()
{
   // Function using standard float formats are always internal
   if(not _version->std_format())
   {
      const auto CG = AppM->CGetCallGraphManager()->CGetCallGraph();
      InEdgeIterator ie, ie_end;
      for(boost::tie(ie, ie_end) = boost::in_edges(_version->function_vertex, *CG); ie != ie_end; ie++)
      {
         if(static_cast<bool>(funcFF.count(ie->m_source)))
         {
            const auto& funcV = funcFF.at(ie->m_source);
            if(funcV.std_format() || *funcV.userRequired != *_version->userRequired)
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

   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
   modified = false;

   for(const auto& block : sl->list_of_bloc)
   {
      // RecursiveExaminate could add statements to the statements list, thus it is necessary to iterate over a static copy of the initial statement list
      std::list<tree_nodeRef> stmtList = block.second->CGetStmtList();
      for(const auto& stmt : stmtList)
      {
         RecursiveExaminate(stmt, stmt, INTERFACE_TYPE_NONE);
      }
   }

   for(const auto& ssaExclude : inputCastRename)
   {
      const auto* ssa = ssaExclude.first;
      const auto& exclude = ssaExclude.second;

      // TODO: Add cast rename operations converting from standrd to user-defined float format after ssa definition statement (or at the beginning of function body for input parameters)
      tree_nodeRef convertedSSA;
      const auto& ssaUses = ssa->CGetUseStmts();
      for(const auto& ssaUse : ssaUses)
      {
         const auto& useStmt = ssaUse.first;
         if(std::find(exclude.begin(), exclude.end(), GET_INDEX_NODE(useStmt)) != exclude.end())
         {
            continue;
         }
         TreeM->ReplaceTreeNode(useStmt, TreeM->GetTreeReindex(ssa->index), convertedSSA);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "");
         modified = true;
      }
   }
   inputCastRename.clear();

   for(const auto& ssaReplace : outputCastRename)
   {
      const auto* ssa = ssaReplace.first;
      const auto& useStmts = ssaReplace.second;

      // TODO: Add cast rename operations converting from user-defined to standard float-format after ssa definition statement
      tree_nodeRef convertedSSA;
      for(const auto& stmt : useStmts)
      {
         TreeM->ReplaceTreeNode(stmt, TreeM->GetTreeReindex(ssa->index), convertedSSA);
         modified = true;
      }
   }
   outputCastRename.clear();

   if(modified)
   {
      function_behavior->UpdateBBVersion();
      return DesignFlowStep_Status::SUCCESS;
   }
   return DesignFlowStep_Status::UNCHANGED;
}

void soft_float_cg_ext::replaceWithCall(const std::string& fu_name, const std::vector<tree_nodeRef>& args, tree_nodeRef current_statement, tree_nodeRef current_tree_node, const std::string& current_srcp)
{
   unsigned int called_function_id = TreeM->function_index(fu_name);
   THROW_ASSERT(called_function_id, "The library miss this function " + fu_name);
   THROW_ASSERT(AppM->GetFunctionBehavior(called_function_id)->GetBehavioralHelper()->has_implementation(), "inconsistent behavioral helper");
   if(not _version->std_format())
   {
      const auto spec_name = fu_name + (_version->userRequired != nullptr ? _version->userRequired->mngl() : "");
      auto spec_funcion_id = TreeM->function_index(spec_name);
      if(spec_funcion_id == 0)
      {
         const auto spec_func = tree_man->CloneFunction(TreeM->GetTreeReindex(called_function_id), spec_name);
         called_function_id = GET_INDEX_CONST_NODE(spec_func);
         THROW_ASSERT(called_function_id, "Cloned function not correctly computed");

         // TODO: append specialization arguments to args
      }
   }

   TreeM->ReplaceTreeNode(current_statement, current_tree_node, tree_man->CreateCallExpr(TreeM->GetTreeReindex(called_function_id), args, current_srcp));
   if(!AppM->GetCallGraphManager()->IsVertex(called_function_id))
   {
      BehavioralHelperRef helper = BehavioralHelperRef(new BehavioralHelper(AppM, called_function_id, true, parameters));
      FunctionBehaviorRef fb = FunctionBehaviorRef(new FunctionBehavior(AppM, helper, parameters));
      AppM->GetCallGraphManager()->AddFunctionAndCallPoint(function_id, called_function_id, current_statement->index, fb, FunctionEdgeInfo::CallType::direct_call);
   }
   else
   {
      AppM->GetCallGraphManager()->AddCallPoint(function_id, called_function_id, current_statement->index, FunctionEdgeInfo::CallType::direct_call);
   }
}

void soft_float_cg_ext::RecursiveExaminate(const tree_nodeRef current_statement, const tree_nodeRef current_tree_node, InterfaceType castRename)
{
   THROW_ASSERT(current_tree_node->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Update recursively (" + STR(current_tree_node->index) + ") " + STR(current_tree_node));
   const tree_nodeRef curr_tn = GET_NODE(current_tree_node);
   const std::string current_srcp = [curr_tn]() -> std::string {
      const auto srcp_tn = GetPointer<const srcp>(curr_tn);
      if(srcp_tn)
      {
         return srcp_tn->include_name + ":" + STR(srcp_tn->line_number) + ":" + STR(srcp_tn->column_number);
      }
      return "";
   }();
   switch(curr_tn->get_kind())
   {
      case call_expr_K:
      case aggr_init_expr_K:
      {
         const call_expr* ce = GetPointer<call_expr>(curr_tn);
         const auto called_v = AppM->CGetCallGraphManager()->GetVertex(GET_INDEX_CONST_NODE(ce->fn));
         THROW_ASSERT(static_cast<bool>(funcFF.count(called_v)), "Called function should have been already computed.");
         const auto& calledFV = funcFF.at(called_v);
         const auto internal_call = calledFV.internal && _version->std_format() == calledFV.std_format();
         const std::vector<tree_nodeRef>& args = ce->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         unsigned int parm_index = 0;
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            RecursiveExaminate(current_statement, *arg, internal_call ? INTERFACE_TYPE_NONE : INTERFACE_TYPE_OUTPUT);
            ++parm_index;
         }
         break;
      }
      case gimple_call_K:
      {
         const gimple_call* ce = GetPointer<gimple_call>(curr_tn);
         const auto called_v = AppM->CGetCallGraphManager()->GetVertex(GET_INDEX_CONST_NODE(ce->fn));
         THROW_ASSERT(static_cast<bool>(funcFF.count(called_v)), "Called function should have been already computed.");
         const auto& calledFV = funcFF.at(called_v);
         const auto internal_call = calledFV.internal && _version->std_format() == calledFV.std_format();
         const std::vector<tree_nodeRef>& args = ce->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         unsigned int parm_index = 0;
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            RecursiveExaminate(current_statement, *arg, internal_call ? INTERFACE_TYPE_NONE : INTERFACE_TYPE_OUTPUT);
            ++parm_index;
         }
         break;
      }
      case gimple_phi_K:
      {
         auto* gp = GetPointer<gimple_phi>(curr_tn);
         for(const auto& de : gp->CGetDefEdgesList())
         {
            RecursiveExaminate(current_statement, de.first, castRename);
         }
         break;
      }
      case gimple_assign_K:
      {
         auto* gm = GetPointer<gimple_assign>(curr_tn);
         const auto rhs_type = GET_NODE(gm->op1)->get_kind();
         if(rhs_type == call_expr_K || rhs_type == aggr_init_expr_K)
         {
            const call_expr* ce = GetPointer<call_expr>(curr_tn);
            const auto called_v = AppM->CGetCallGraphManager()->GetVertex(GET_INDEX_CONST_NODE(ce->fn));
            THROW_ASSERT(static_cast<bool>(funcFF.count(called_v)), "Called function should have been already computed.");
            const auto& calledFV = funcFF.at(called_v);
            const auto internal_call = calledFV.internal && _version->std_format() == calledFV.std_format();
            // Return values associated to non-internal calls need to be cast renamed to local float format
            RecursiveExaminate(current_statement, gm->op0, internal_call ? castRename : INTERFACE_TYPE_INPUT);
         }
         else if(tree_helper::IsLoad(TreeM, current_statement, FB->get_function_mem()))
         {
            // Values loaded from memory need to be cast renamed to local float format
            RecursiveExaminate(current_statement, gm->op0, INTERFACE_TYPE_INPUT);
         }
         else
         {
            RecursiveExaminate(current_statement, gm->op0, castRename);
         }
         if(tree_helper::IsStore(TreeM, current_statement, FB->get_function_mem()))
         {
            // Values stored to memory need to be cast renamed before the store statement
            RecursiveExaminate(current_statement, gm->op1, INTERFACE_TYPE_OUTPUT);
         }
         else
         {
            RecursiveExaminate(current_statement, gm->op1, castRename);
         }
         if(gm->predicate)
         {
            RecursiveExaminate(current_statement, gm->predicate, castRename);
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
         auto* SSA = GetPointer<ssa_name>(GET_NODE(curr_tn));

         // Non-internal functions need to implement an input interface
         if(not(_version->internal || bindingCompleted))
         {
            const auto& args = fd->list_of_args;
            // If ssa_name references a parm_decl and is defined by a gimple_nop, it represents the formal function parameter inside the function body
            if(SSA->var != nullptr && GET_CONST_NODE(SSA->var)->get_kind() == parm_decl_K && GET_CONST_NODE(SSA->CGetDefStmt())->get_kind() == gimple_nop_K)
            {
               auto argIt = std::find_if(args.begin(), args.end(), [&](const tree_nodeRef& arg) { return GET_INDEX_CONST_NODE(arg) == GET_INDEX_CONST_NODE(SSA->var); });
               THROW_ASSERT(argIt != args.end(), "parm_decl associated with ssa_name not found in function parameters");
               size_t arg_pos = static_cast<size_t>(argIt - args.begin());
               THROW_ASSERT(arg_pos < args.size(), "Computed parameter position outside actual parameters number");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variable " + SSA->ToString() + " is defined from parameter " + STR(arg_pos));
               paramBinding[arg_pos] = curr_tn;
               bindingCompleted = std::find(paramBinding.begin(), paramBinding.end(), nullptr) == paramBinding.end();
            }
         }

         if(not _version->std_format() && tree_helper::CGetType(curr_tn)->get_kind() == real_type_K)
         {
            if((not _version->internal && std::find(paramBinding.begin(), paramBinding.end(), curr_tn) != paramBinding.end()) || castRename == INTERFACE_TYPE_INPUT)
            {
               if(castRename == INTERFACE_TYPE_OUTPUT)
               {
                  // Considered ssa has been discovered to be a function parameter and is used in current statement as a non-internal function argument, thus conversion can be avoided
                  inputCastRename[SSA].push_back(GET_INDEX_CONST_NODE(current_statement));
                  break;
               }
               if(not static_cast<bool>(inputCastRename.count(SSA)))
               {
                  // Add current input SSA to the input cast rename list for all its uses if not already present
                  inputCastRename.insert({SSA, {}});
               }
            }
            else if(castRename == INTERFACE_TYPE_OUTPUT)
            {
               // Add current output SSA to the output cast rename list for its uses in current statement
               outputCastRename[SSA].push_back(current_statement);
            }
         }

         break;
      }
      case tree_list_K:
      {
         tree_nodeRef current = current_tree_node;
         while(current)
         {
            RecursiveExaminate(current_statement, GetPointer<tree_list>(GET_NODE(current))->valu, castRename);
            current = GetPointer<tree_list>(GET_NODE(current))->chan;
         }
         break;
      }
      case CASE_UNARY_EXPRESSION:
      {
         const unary_expr* ue = GetPointer<unary_expr>(curr_tn);
         // Propagate recursion with INTERFACE_TYPE_NONE to avoid cast rename of internal variables (input parameters and constant will be converted anyway)
         RecursiveExaminate(current_statement, ue->op, INTERFACE_TYPE_NONE);
         tree_nodeRef expr_type = GET_NODE(ue->type);
         unsigned int op_expr_type_index;
         tree_nodeRef op_expr_type = tree_helper::get_type_node(GET_NODE(ue->op), op_expr_type_index);
         if(expr_type->get_kind() == real_type_K)
         {
            switch(curr_tn->get_kind())
            {
               case float_expr_K:
               {
                  unsigned int bitsize_in = tree_helper::size(TreeM, op_expr_type_index);
                  if(bitsize_in < 32)
                  {
                     bitsize_in = 32;
                  }
                  else if(bitsize_in > 32 && bitsize_in < 64)
                  {
                     bitsize_in = 64;
                  }
                  unsigned int bitsize_out = tree_helper::size(TreeM, GET_INDEX_NODE(ue->type));
                  if(bitsize_in < 32)
                  {
                     bitsize_in = 32;
                  }
                  else if(bitsize_in > 32 && bitsize_in < 64)
                  {
                     bitsize_in = 64;
                  }
                  std::string bitsize_str_in = bitsize_in == 96 ? "x80" : STR(bitsize_in);
                  std::string bitsize_str_out = bitsize_out == 96 ? "x80" : STR(bitsize_out);
                  std::string fu_name;
                  if(op_expr_type->get_kind() != real_type_K)
                  {
                     if(tree_helper::is_unsigned(TreeM, op_expr_type_index))
                     {
                        fu_name = "__uint" + bitsize_str_in + "_to_float" + bitsize_str_out + "if";
                     }
                     else
                     {
                        fu_name = "__int" + bitsize_str_in + "_to_float" + bitsize_str_out + "if";
                     }
                     replaceWithCall(fu_name, {ue->op}, current_statement, current_tree_node, current_srcp);
                     modified = true;
                  }
                  break;
               }
               case nop_expr_K:
               case abs_expr_K:
               case negate_expr_K:
               case view_convert_expr_K:
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
                  unsigned int bitsize_in = tree_helper::size(TreeM, op_expr_type_index);
                  unsigned int bitsize_out = tree_helper::size(TreeM, GET_INDEX_NODE(ue->type));
                  if(bitsize_out < 32)
                  {
                     bitsize_out = 32;
                  }
                  else if(bitsize_out > 32 && bitsize_out < 64)
                  {
                     bitsize_out = 64;
                  }
                  bool is_unsigned = tree_helper::is_unsigned(TreeM, GET_INDEX_NODE(ue->type));
                  std::string bitsize_str_in = bitsize_in == 96 ? "x80" : STR(bitsize_in);
                  std::string bitsize_str_out = bitsize_out == 96 ? "x80" : STR(bitsize_out);
                  std::string fu_name = "__float" + bitsize_str_in + "_to_" + (is_unsigned ? "u" : "") + "int" + bitsize_str_out + "_round_to_zeroif";
                  replaceWithCall(fu_name, {ue->op}, current_statement, current_tree_node, current_srcp);
                  modified = true;
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
               case view_convert_expr_K:
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
         const binary_expr* be = GetPointer<binary_expr>(curr_tn);
         // Propagate recursion with INTERFACE_TYPE_NONE to avoid cast rename of internal variables (input parameters and constant will be converted anyway)
         RecursiveExaminate(current_statement, be->op0, INTERFACE_TYPE_NONE);
         RecursiveExaminate(current_statement, be->op1, INTERFACE_TYPE_NONE);
         unsigned int expr_type_index;
         tree_nodeRef expr_type = tree_helper::get_type_node(GET_NODE(be->op0), expr_type_index);
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
                  unsigned int bitsize = tree_helper::size(TreeM, expr_type_index);
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
                        ; // do nothing
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
               case eq_expr_K:
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
               case ne_expr_K:
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
               unsigned int bitsize = tree_helper::size(TreeM, expr_type_index);
               std::string bitsize_str = bitsize == 96 ? "x80" : STR(bitsize);
               std::string fu_name = "__float" + bitsize_str + "_" + fu_suffix + "if";
               replaceWithCall(fu_name, {be->op0, be->op1}, current_statement, current_tree_node, current_srcp);
               modified = true;
            }
         }
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         const ternary_expr* te = GetPointer<ternary_expr>(curr_tn);
         RecursiveExaminate(current_statement, te->op0, castRename);
         if(te->op1)
         {
            RecursiveExaminate(current_statement, te->op1, castRename);
         }
         if(te->op2)
         {
            RecursiveExaminate(current_statement, te->op2, castRename);
         }
         break;
      }
      case CASE_QUATERNARY_EXPRESSION:
      {
         const quaternary_expr* qe = GetPointer<quaternary_expr>(curr_tn);
         RecursiveExaminate(current_statement, qe->op0, castRename);
         if(qe->op1)
         {
            RecursiveExaminate(current_statement, qe->op1, castRename);
         }
         if(qe->op2)
         {
            RecursiveExaminate(current_statement, qe->op2, castRename);
         }
         if(qe->op3)
         {
            RecursiveExaminate(current_statement, qe->op3, castRename);
         }
         break;
      }
      case lut_expr_K:
      {
         auto* le = GetPointer<lut_expr>(curr_tn);
         RecursiveExaminate(current_statement, le->op0, castRename);
         RecursiveExaminate(current_statement, le->op1, castRename);
         if(le->op2)
         {
            RecursiveExaminate(current_statement, le->op2, castRename);
         }
         if(le->op3)
         {
            RecursiveExaminate(current_statement, le->op3, castRename);
         }
         if(le->op4)
         {
            RecursiveExaminate(current_statement, le->op4, castRename);
         }
         if(le->op5)
         {
            RecursiveExaminate(current_statement, le->op5, castRename);
         }
         if(le->op6)
         {
            RecursiveExaminate(current_statement, le->op6, castRename);
         }
         if(le->op7)
         {
            RecursiveExaminate(current_statement, le->op7, castRename);
         }
         if(le->op8)
         {
            RecursiveExaminate(current_statement, le->op8, castRename);
         }
         break;
      }
      case constructor_K:
      {
         const constructor* co = GetPointer<constructor>(curr_tn);
         const std::vector<std::pair<tree_nodeRef, tree_nodeRef>>& list_of_idx_valu = co->list_of_idx_valu;
         std::vector<std::pair<tree_nodeRef, tree_nodeRef>>::const_iterator it, it_end = list_of_idx_valu.end();
         for(it = list_of_idx_valu.begin(); it != it_end; ++it)
         {
            RecursiveExaminate(current_statement, it->second, castRename);
         }
         break;
      }
      case gimple_cond_K:
      {
         const gimple_cond* gc = GetPointer<gimple_cond>(curr_tn);
         RecursiveExaminate(current_statement, gc->op0, castRename);
         break;
      }
      case gimple_switch_K:
      {
         const gimple_switch* se = GetPointer<gimple_switch>(curr_tn);
         RecursiveExaminate(current_statement, se->op0, castRename);
         break;
      }
      case gimple_multi_way_if_K:
      {
         auto* gmwi = GetPointer<gimple_multi_way_if>(curr_tn);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               RecursiveExaminate(current_statement, cond.first, castRename);
            }
         }
         break;
      }
      case gimple_return_K:
      {
         const gimple_return* re = GetPointer<gimple_return>(curr_tn);
         if(re->op)
         {
            RecursiveExaminate(current_statement, re->op, _version->internal ? INTERFACE_TYPE_NONE : INTERFACE_TYPE_OUTPUT);
         }
         break;
      }
      case gimple_for_K:
      {
         const auto* gf = GetPointer<const gimple_for>(curr_tn);
         RecursiveExaminate(current_statement, gf->op0, castRename);
         RecursiveExaminate(current_statement, gf->op1, castRename);
         RecursiveExaminate(current_statement, gf->op2, castRename);
         break;
      }
      case gimple_while_K:
      {
         const gimple_while* gw = GetPointer<gimple_while>(curr_tn);
         RecursiveExaminate(current_statement, gw->op0, castRename);
         break;
      }
      case CASE_TYPE_NODES:
      case type_decl_K:
      {
         break;
      }
      case target_mem_ref_K:
      {
         const target_mem_ref* tmr = GetPointer<target_mem_ref>(curr_tn);
         if(tmr->symbol)
         {
            RecursiveExaminate(current_statement, tmr->symbol, castRename);
         }
         if(tmr->base)
         {
            RecursiveExaminate(current_statement, tmr->base, castRename);
         }
         if(tmr->idx)
         {
            RecursiveExaminate(current_statement, tmr->idx, castRename);
         }
         break;
      }
      case target_mem_ref461_K:
      {
         const target_mem_ref461* tmr = GetPointer<target_mem_ref461>(curr_tn);
         if(tmr->base)
         {
            RecursiveExaminate(current_statement, tmr->base, castRename);
         }
         if(tmr->idx)
         {
            RecursiveExaminate(current_statement, tmr->idx, castRename);
         }
         if(tmr->idx2)
         {
            RecursiveExaminate(current_statement, tmr->idx2, castRename);
         }
         break;
      }
      case real_cst_K:
      {
         if(_version->internal && not _version->std_format() && castRename != INTERFACE_TYPE_OUTPUT)
         {
            // TODO: rewrite constant value using user required float format
            //       simply add conversion statements as if real_cst was an ssa_name with INTERFACE_TYPE_INPUT, constant propagation will do the rest
         }
         break;
      }
      case complex_cst_K:
      case string_cst_K:
      case integer_cst_K:
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
