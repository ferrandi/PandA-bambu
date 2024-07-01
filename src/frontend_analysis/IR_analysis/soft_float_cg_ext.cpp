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
 * @file soft_float_cg_ext.cpp
 * @brief Step that extends the call graph with the soft-float calls where appropriate.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "soft_float_cg_ext.hpp"

#include "FunctionCallOpt.hpp"
#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "basic_blocks_graph_constructor.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "custom_map.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "design_flow_step.hpp"
#include "exceptions.hpp"
#include "ext_tree_node.hpp"
#include "function_behavior.hpp"
#include "math.h"
#include "op_graph.hpp"
#include "string_manipulation.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_node_dup.hpp"
#include "utility.hpp"
#include "var_pp_functor.hpp"

#include <boost/multiprecision/integer.hpp>

#include <algorithm>
#include <deque>
#include <list>
#include <regex>
#include <set>
#include <string>

CustomMap<CallGraph::vertex_descriptor, FunctionVersionRef> soft_float_cg_ext::funcFF;
CustomMap<unsigned int, std::array<tree_nodeRef, 8>> soft_float_cg_ext::versioning_args;
bool soft_float_cg_ext::inline_math = false;
bool soft_float_cg_ext::inline_conversion = false;
tree_nodeRef soft_float_cg_ext::float32_type;
tree_nodeRef soft_float_cg_ext::float32_ptr_type;
tree_nodeRef soft_float_cg_ext::float64_type;
tree_nodeRef soft_float_cg_ext::float64_ptr_type;

static const FloatFormatRef float32FF(new FloatFormat(8, 23, -127));
static const FloatFormatRef float64FF(new FloatFormat(11, 52, -1023));

static const std::set<std::string> supported_libm_calls = {
    "copysign", "fabs",       "finite", "fpclassify", "huge_val", "inf",  "infinity", "isfinite",
    "isinf",    "isinf_sign", "isnan",  "isnormal",   "nan",      "nans", "signbit"};
static const std::set<std::string> supported_libm_calls_inlined = {"copysign", "fabs"};

/**
 * @brief List of low level implementation libm functions. Composite functions are not present since fp format can be
 * safely propagated there.
 *
 */
static const std::set<std::string> libm_func = {
    "acos",     "acosh",  "asin",     "asinh",    "atan",      "atanh",      "atan2",      "cbrt",     "ceil",
    "copysign", "cos",    "cosh",     "erf",      "erfc",      "exp",        "exp2",       "expm1",    "fabs",
    "fdim",     "finite", "floor",    "fma",      "fmod",      "fpclassify", "frexp",      "huge_val", "hypot",
    "ilogb",    "inf",    "infinity", "isfinite", "isinf",     "isinf_sign", "isnan",      "isnormal", "ldexp",
    "lgamma",   "llrint", "llround",  "log",      "log10",     "log1p",      "log2",       "logb",     "lrint",
    "lround",   "modf",   "nan",      "nans",     "nearbyint", "nextafter",  "nexttoward", "pow",      "remainder",
    "remquo",   "rint",   "round",    "scalb",    "scalbln",   "scalbn",     "sin",        "signbit",  "sinh",
    "sincos",   "sqrt",   "tan",      "tanh",     "tgamma",    "trunc"};

static std::string strip_fname(std::string fname, bool* single_prec = nullptr)
{
   if(single_prec)
   {
      *single_prec = false;
   }
   if(fname.find("__internal_") == 0)
   {
      fname = fname.substr(sizeof("__internal_") - 1);
   }
   else if(fname.find("__builtin_") == 0)
   {
      fname = fname.substr(sizeof("__builtin_") - 1);
   }
   else if(fname.find("__") == 0)
   {
      fname = fname.substr(sizeof("__") - 1);
   }
   if(fname.back() == 'f' && libm_func.count(fname.substr(0, fname.size() - 1)))
   {
      fname = fname.substr(0, fname.size() - 1);
      if(single_prec)
      {
         *single_prec = true;
      }
   }
   return fname;
}

soft_float_cg_ext::soft_float_cg_ext(const ParameterConstRef _parameters, const application_managerRef _AppM,
                                     unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, SOFT_FLOAT_CG_EXT, _design_flow_manager, _parameters),
      TreeM(_AppM->get_tree_manager()),
      tree_man(new tree_manipulation(TreeM, parameters, _AppM)),
      fd(GetPointer<function_decl>(TreeM->GetTreeNode(function_id))),
      isTopFunction(AppM->CGetCallGraphManager()->GetRootFunctions().count(function_id)),
      bindingCompleted(fd->list_of_args.size() == 0),
      paramBinding(fd->list_of_args.size(), nullptr)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);

   if(!float32_type)
   {
      float32_type = tree_man->GetCustomIntegerType(32, true);
      float32_ptr_type = tree_man->GetPointerType(float32_type, 32);
      float64_type = tree_man->GetCustomIntegerType(64, true);
      float64_ptr_type = tree_man->GetPointerType(float64_type, 64);
      if(parameters->isOption(OPT_fp_subnormal) && parameters->getOption<bool>(OPT_fp_subnormal))
      {
         float32FF->has_subnorm = true;
         float64FF->has_subnorm = true;
      }
      if(parameters->isOption(OPT_fp_rounding_mode))
      {
         const auto rnd_mode = parameters->getOption<std::string>(OPT_fp_rounding_mode);
         if(rnd_mode == "nearest_even")
         {
            float32FF->rounding_mode = FloatFormat::FPRounding_NearestEven;
            float64FF->rounding_mode = FloatFormat::FPRounding_NearestEven;
         }
         else if(rnd_mode == "truncate")
         {
            float32FF->rounding_mode = FloatFormat::FPRounding_Truncate;
            float64FF->rounding_mode = FloatFormat::FPRounding_Truncate;
         }
         else
         {
            THROW_UNREACHABLE("Floating-point rounding mode not supported: " + STR(rnd_mode));
         }
      }
      if(parameters->isOption(OPT_fp_exception_mode))
      {
         const auto exc_mode = parameters->getOption<std::string>(OPT_fp_exception_mode);
         if(exc_mode == "ieee")
         {
            float32FF->exception_mode = FloatFormat::FPException_IEEE;
            float64FF->exception_mode = FloatFormat::FPException_IEEE;
         }
         else if(exc_mode == "saturation")
         {
            float32FF->exception_mode = FloatFormat::FPException_Saturation;
            float64FF->exception_mode = FloatFormat::FPException_Saturation;
         }
         else if(exc_mode == "overflow")
         {
            float32FF->exception_mode = FloatFormat::FPException_Overflow;
            float64FF->exception_mode = FloatFormat::FPException_Overflow;
         }
         else
         {
            THROW_UNREACHABLE("Floating-point exception mode not supported: " + STR(exc_mode));
         }
      }
   }

   if(funcFF.empty() && !parameters->getOption<std::string>(OPT_fp_format).empty())
   {
      const auto CGM = AppM->CGetCallGraphManager();
      auto opts = string_to_container<std::vector<std::string>>(parameters->getOption<std::string>(OPT_fp_format), ",");
      const auto inline_math_it = std::find(opts.begin(), opts.end(), "inline-math");
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "-->Soft-float fp format specialization required:");
      if(inline_math_it != opts.end())
      {
         opts.erase(inline_math_it);
         inline_math = true;
         INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Full inlining of floating-point arithmetic operators");
      }
      const auto inline_conversion_it = std::find(opts.begin(), opts.end(), "inline-conversion");
      if(inline_conversion_it != opts.end())
      {
         opts.erase(inline_conversion_it);
         inline_conversion = true;
         INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Full inlining of floating-point conversion operators");
      }
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "-->");
      for(const auto& opt : opts)
      {
         auto format = string_to_container<std::vector<std::string>>(opt, "*");

         const auto f_index = [&]() {
            if(format[0] == "@")
            {
               const auto top_symbols = parameters->getOption<std::vector<std::string>>(OPT_top_functions_names);
               THROW_ASSERT(top_symbols.size() == 1, "Expected single top function name");
               if(top_symbols.size() > 1)
               {
                  THROW_WARNING("Multiple top functions defined, @ is replaced with first one.");
               }
               format[0] = top_symbols.front();
            }
            const auto f_node = TreeM->GetFunction(format[0]);
            return f_node ? f_node->index : 0;
         }();

         if(!f_index)
         {
            THROW_ERROR("Function " + format[0] + " does not exists. (Maybe it has been inlined)");
         }
         const auto function_v = CGM->GetVertex(f_index);
         if(funcFF.count(function_v))
         {
            THROW_ERROR("Function " + format[0] + " already specialized.");
         }

         const auto userFF = FloatFormat::FromString(format[1]);
         THROW_ASSERT(userFF, "FP format for function " + STR(format[0]) + " not valid");
         funcFF.insert({function_v, FunctionVersionRef(new FunctionVersion(function_v, userFF))});
         INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                        format[0] + " specialized with fp format " + userFF->ToString());
      }
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--");

      // Propagate floating-point format specialization over to called functions
      if(parameters->isOption(OPT_fp_format_propagate) && parameters->getOption<bool>(OPT_fp_format_propagate))
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Soft-float fp format propagation enabled:");
         INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "-->");
         for(const auto& root_func : CGM->GetRootFunctions())
         {
            std::list<CallGraph::vertex_descriptor> func_sort;
            CustomUnorderedSet<CallGraph::vertex_descriptor> reached_v;
            const auto reached_from_top = CGM->GetReachedFunctionsFrom(root_func);
            for(const auto func_id : reached_from_top)
            {
               reached_v.insert(CGM->GetVertex(func_id));
            }
            const auto TopCG = CGM->CGetCallSubGraph(reached_v);
            TopCG->TopologicalSort(func_sort);

            for(const auto func : func_sort)
            {
               // Initialize current function version
               FunctionVersionRef current_v;
               if(static_cast<bool>(funcFF.count(func)))
               {
                  current_v = funcFF.at(func);
               }
               else
               {
                  current_v = FunctionVersionRef(new FunctionVersion(func));
#if HAVE_ASSERTS
                  const auto insertion =
#endif
                      funcFF.insert({func, current_v});
                  THROW_ASSERT(insertion.second, "");
               }

               // Check callers' function version
               FloatFormatRef callers_ff =
                   !current_v->callers.empty() ? current_v->callers.front()->userRequired : nullptr;
               const auto common_null = callers_ff == nullptr;
               for(const auto& caller : current_v->callers)
               {
                  const auto caller_null = caller->userRequired == nullptr;
                  if((caller_null ^ common_null) || (!common_null && *callers_ff != *caller->userRequired))
                  {
                     callers_ff = nullptr;
                     break;
                  }
               }

               // Update current function fp format
               if(current_v->userRequired == nullptr)
               {
                  current_v->userRequired = callers_ff;
                  current_v->internal = true;
               }
               else if(callers_ff == nullptr)
               {
                  current_v->internal = current_v->callers.empty();
               }
               else
               {
                  current_v->internal = *current_v->userRequired == *callers_ff;
               }

               const auto func_id = AppM->CGetCallGraphManager()->get_function(func);
               INDENT_OUT_MEX(
                   OUTPUT_LEVEL_VERBOSE, output_level,
                   "Analysing function " +
                       tree_helper::print_type(TreeM, func_id, false, true, false, 0U,
                                               var_pp_functorConstRef(new std_var_pp_functor(
                                                   AppM->CGetFunctionBehavior(func_id)->CGetBehavioralHelper()))));
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---FP format " + current_v->ToString());

               // Propagate current fp format to the called functions
               CallGraph::out_edge_iterator ei, ei_end;
               for(boost::tie(ei, ei_end) = boost::out_edges(func, *TopCG); ei != ei_end; ++ei)
               {
                  const auto called = boost::target(*ei, *TopCG);
                  const auto fname = tree_helper::print_function_name(
                      TreeM, GetPointerS<const function_decl>(TreeM->GetTreeNode(CGM->get_function(called))));
                  const auto called_fname = strip_fname(fname);
                  if(static_cast<bool>(libm_func.count(called_fname)))
                  {
                     // Do not propagate format to libm functions, specialization will be handled successively
                     continue;
                  }
                  FunctionVersionRef called_v;
                  if(static_cast<bool>(funcFF.count(called)))
                  {
                     called_v = funcFF.at(called);
                  }
                  else
                  {
                     called_v = FunctionVersionRef(new FunctionVersion(called));
#if HAVE_ASSERTS
                     const auto insertion =
#endif
                         funcFF.insert({called, called_v});
                     THROW_ASSERT(insertion.second, "");
                  }
                  called_v->callers.push_back(current_v);
               }
            }
         }
         INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--");
      }
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--");
   }
   THROW_ASSERT(AppM->CGetCallGraphManager()->IsVertex(function_id), "");
   const auto function_v = AppM->CGetCallGraphManager()->GetVertex(function_id);
   if(static_cast<bool>(funcFF.count(function_v)))
   {
      _version = funcFF.at(function_v);
   }
   else
   {
      _version = FunctionVersionRef(new FunctionVersion(function_v));
#if HAVE_ASSERTS
      const auto insertion =
#endif
          funcFF.insert({function_v, _version});
      THROW_ASSERT(insertion.second, "");
   }
   int_type = !_version->ieee_format() ?
                  tree_man->GetCustomIntegerType(
                      static_cast<unsigned int>(static_cast<uint8_t>(_version->userRequired->sign == bit_lattice::U) +
                                                _version->userRequired->exp_bits + _version->userRequired->frac_bits),
                      true) :
                  nullptr;
   int_ptr_type = int_type ? tree_man->GetPointerType(int_type, GetPointer<integer_type>(int_type)->algn) : nullptr;
}

soft_float_cg_ext::~soft_float_cg_ext() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
soft_float_cg_ext::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(DETERMINE_MEMORY_ACCESSES, SAME_FUNCTION));
         relationships.insert(std::make_pair(EXTRACT_GIMPLE_COND_OP, SAME_FUNCTION));
         relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(SOFT_FLOAT_CG_EXT, CALLED_FUNCTIONS));
         relationships.insert(std::make_pair(UN_COMPARISON_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(TREE2FUN, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(INTERFACE_INFER, WHOLE_APPLICATION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         if(GetStatus() == DesignFlowStep_Status::SUCCESS)
         {
            relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, SAME_FUNCTION));
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
   return bb_version == 0 && FunctionFrontendFlowStep::HasToBeExecuted();
}

DesignFlowStep_Status soft_float_cg_ext::InternalExec()
{
   THROW_ASSERT(parameters->isOption(OPT_soft_float) && parameters->getOption<bool>(OPT_soft_float),
                "Floating-point lowering should not be executed");

   static const auto ff_already_propagated =
       parameters->isOption(OPT_fp_format_propagate) && parameters->getOption<bool>(OPT_fp_format_propagate);
   // Check if current function needs IO fp format interface (avoid check if fp format propagation has already been
   // computed)
   if(!ff_already_propagated && !_version->ieee_format())
   {
      const auto CG = AppM->CGetCallGraphManager()->CGetCallGraph();
      InEdgeIterator ie, ie_end;
      for(boost::tie(ie, ie_end) = boost::in_edges(_version->function_vertex, *CG); ie != ie_end; ie++)
      {
         if(static_cast<bool>(funcFF.count(ie->m_source)))
         {
            const auto& funcV = funcFF.at(ie->m_source);
            if(funcV->ieee_format() || *(funcV->userRequired) != *(_version->userRequired))
            {
               // If a caller of current function uses a different float format, current function is not internal to the
               // user specified float format
               _version->internal = false;
               break;
            }
         }
         else
         {
            // If a caller of current function does not have a function version specified, it uses a standard float
            // format for sure, thus current function is not internal
            _version->internal = false;
            break;
         }
      }
   }
   THROW_ASSERT(!_version->ieee_format() || _version->internal,
                "A standard floating-point format function should be internal.");

#ifndef NDEBUG
   const auto fn_name = tree_helper::print_type(
       TreeM, function_id, false, true, false, 0U,
       var_pp_functorConstRef(new std_var_pp_functor(function_behavior->CGetBehavioralHelper())));
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Function " + fn_name + " implementing " + _version->ToString() + " floating-point format");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---IO interface is " + STR((_version->ieee_format() || _version->internal) ? "not " : "") +
                      "necessary");

   const auto sl = GetPointerS<statement_list>(fd->body);
   bool modified = false;

   for(const auto& block : sl->list_of_bloc)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Update recursively BB" + STR(block.first));
      for(const auto& phi : block.second->CGetPhiList())
      {
         modified |= RecursiveExaminate(phi, phi, INTERFACE_TYPE_NONE);
      }

      // RecursiveExaminate could add statements to the statements list, thus it is necessary to iterate over a static
      // copy of the initial statement list
      for(const auto& stmt : block.second->CGetStmtList())
      {
         modified |= RecursiveExaminate(stmt, stmt, INTERFACE_TYPE_NONE);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Updated recursively BB" + STR(block.first));
   }

   // Fix hardware implemented function arguments
   if(hwParam.size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Adding view-convert expressions to support hardware implemented FU call arguments");
      for(const auto& ssa_uses : hwParam)
      {
         const auto ssa = ssa_uses.first;
         const auto ssa_node = TreeM->GetTreeNode(ssa->index);
         const auto out_int = outputInterface.find(ssa);
         std::vector<tree_nodeRef>* out_ssa =
             out_int != outputInterface.end() ? &std::get<1>(out_int->second) : nullptr;
         for(const auto& call_stmt_idx : ssa_uses.second)
         {
            const auto call_stmt = TreeM->GetTreeNode(call_stmt_idx);
            const auto call_node = GetPointerS<const gimple_node>(call_stmt);
            const auto& call_bb = sl->list_of_bloc.at(call_node->bb_index);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---View-convert for " + ssa->ToString() + " in BB" + STR(call_bb->number) + " " +
                               call_node->ToString());
            // At this time ssa->type is still real_type, thus we can exploit that (it will be modified after)
            const auto arg_vc =
                tree_man->create_unary_operation(ssa->type, ssa_node, BUILTIN_SRCP, view_convert_expr_K);
            const auto vc_stmt = tree_man->CreateGimpleAssign(ssa->type, tree_nodeRef(), tree_nodeRef(), arg_vc,
                                                              function_id, BUILTIN_SRCP);
            const auto vc_ssa = GetPointerS<gimple_assign>(vc_stmt)->op0;
            call_bb->PushBefore(vc_stmt, call_stmt, AppM);
            TreeM->ReplaceTreeNode(call_stmt, ssa_node, vc_ssa);
            if(out_ssa)
            {
               std::replace_if(
                   out_ssa->begin(), out_ssa->end(), [&](const tree_nodeRef& t) { return t->index == call_stmt_idx; },
                   vc_stmt);
            }
         }
      }
      modified = true;
      hwParam.clear();
   }

   // Fix hardware implemented function return values
   if(hwReturn.size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Adding view-convert expressions to support hardware implemented FU call return values");
      for(const auto& ssa : hwReturn)
      {
         const auto call_stmt = ssa->CGetDefStmt();
         const auto def_node = GetPointerS<const gimple_node>(call_stmt);
         const auto call_bb = sl->list_of_bloc.at(def_node->bb_index);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->View-convert for " + ssa->ToString() + " in BB" + STR(call_bb->number) + " " +
                            def_node->ToString());
         const auto ssa_node = TreeM->GetTreeNode(ssa->index);
         // Hardware calls are for sure dealing with standard IEEE formats only
         const auto int_ret_type = tree_helper::Size(ssa->type) == 32 ? float32_type : float64_type;
         const auto ret_vc =
             tree_man->create_unary_operation(int_ret_type, ssa_node, BUILTIN_SRCP, view_convert_expr_K);
         const auto vc_stmt = tree_man->CreateGimpleAssign(int_ret_type, tree_nodeRef(), tree_nodeRef(), ret_vc,
                                                           function_id, BUILTIN_SRCP);
         const auto vc_ssa = GetPointerS<const gimple_assign>(vc_stmt)->op0;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Added statement " + vc_stmt->ToString());
         const auto if_info = inputInterface.find(ssa);
         if(if_info != inputInterface.end())
         {
            const auto new_ssa = GetPointer<ssa_name>(vc_ssa);
            THROW_ASSERT(std::get<0>(if_info->second), "");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Input interface " + std::get<0>(if_info->second)->ToString() + " moved");
            inputInterface.insert(std::make_pair(new_ssa, if_info->second));
            inputInterface.erase(ssa);
         }
         const auto ssa_uses = ssa->CGetUseStmts();
         for(const auto& stmt_use : ssa_uses)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Replace use - before: " + stmt_use.first->ToString());
            TreeM->ReplaceTreeNode(stmt_use.first, ssa_node, vc_ssa);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---               after: " + stmt_use.first->ToString());
         }
         call_bb->PushAfter(vc_stmt, call_stmt, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         viewConvert.erase(ssa);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      modified = true;
      hwReturn.clear();
   }

   // Design top function signatures must not be modified, thus a view-convert operation for real_type parameters and
   // return value must be added inside the function body
   if(isTopFunction &&
      (!parameters->isOption(OPT_fp_format_interface) || !parameters->getOption<bool>(OPT_fp_format_interface)))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Parameters binding " + STR(bindingCompleted ? "" : "partially ") + "completed on " +
                         STR(paramBinding.size()) + " arguments");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");

      const auto entry_bb = sl->list_of_bloc.at(bloc::ENTRY_BLOCK_ID);
      const auto first_bb = sl->list_of_bloc.at(entry_bb->list_of_succ.front());
      for(const auto& parm : paramBinding)
      {
         if(parm)
         {
            THROW_ASSERT(parm->get_kind() == ssa_name_K,
                         "Unexpected parameter node type (" + parm->get_kind_text() + ")");
            const auto parmSSA = GetPointerS<ssa_name>(parm);
            if(lowering_needed(parmSSA))
            {
               const auto parm_type = int_type_for(parmSSA->type, false);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "Lowering top function parameter type of " + parmSSA->ToString() + ": " +
                                  parmSSA->type->ToString() + " -> " + parm_type->ToString());
               tree_nodeRef vc_stmt;
               if(parm_type->get_kind() == pointer_type_K)
               {
                  vc_stmt = tree_man->CreateNopExpr(parm, parm_type, tree_nodeRef(), tree_nodeRef(), function_id);
               }
               else
               {
                  const auto vc = tree_man->create_unary_operation(parm_type, parm, BUILTIN_SRCP, view_convert_expr_K);
                  vc_stmt = tree_man->CreateGimpleAssign(parm_type, tree_nodeRef(), tree_nodeRef(), vc, function_id,
                                                         BUILTIN_SRCP);
                  if(!_version->ieee_format())
                  {
                     const auto ssa_ff = tree_helper::Size(parmSSA->type) == 32 ? float32FF : float64FF;
                     const auto ientry =
                         inputInterface.insert({GetPointerS<ssa_name>(GetPointerS<gimple_assign>(vc_stmt)->op0),
                                                {ssa_ff, std::vector<unsigned int>()}});
                     THROW_ASSERT(ientry.second, "");
                     const auto oentry = outputInterface.find(parmSSA);
                     if(oentry != outputInterface.end())
                     {
                        const auto& oentry_list = std::get<1>(oentry->second);
                        auto& ientry_list = std::get<1>(ientry.first->second);
                        for(const auto& e : oentry_list)
                        {
                           ientry_list.push_back(e->index);
                        }
                        outputInterface.erase(oentry);
                     }
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Input interface required for current parameter");
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Lowering statement added to BB" + STR(first_bb->number) + ": " + vc_stmt->ToString());
               const auto lowered_parm = GetPointerS<gimple_assign>(vc_stmt)->op0;
               const auto parm_uses = parmSSA->CGetUseStmts();
               for(const auto& stmt_uses : parm_uses)
               {
                  TreeM->ReplaceTreeNode(stmt_uses.first, parm, lowered_parm);
               }
               first_bb->PushFront(vc_stmt, AppM);
               viewConvert.erase(parmSSA);
               modified = true;
            }
         }
      }
      paramBinding.clear();
      for(const auto& ret_stmt : topReturn)
      {
         const auto gr = GetPointerS<gimple_return>(ret_stmt);
         const auto ret_ssa = GetPointerS<ssa_name>(gr->op);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Return value type restore added for variable " + ret_ssa->ToString());
         const auto bb = sl->list_of_bloc.at(gr->bb_index);
         tree_nodeRef vc_stmt;
         if(ret_ssa->type->get_kind() == pointer_type_K)
         {
            vc_stmt = tree_man->CreateNopExpr(gr->op, ret_ssa->type, tree_nodeRef(), tree_nodeRef(), function_id);
         }
         else
         {
            const auto vc = tree_man->create_unary_operation(ret_ssa->type, gr->op, BUILTIN_SRCP, view_convert_expr_K);
            vc_stmt = tree_man->CreateGimpleAssign(ret_ssa->type, tree_nodeRef(), tree_nodeRef(), vc, function_id,
                                                   BUILTIN_SRCP);
            if(!_version->ieee_format())
            {
               const auto ssa_ff = tree_helper::Size(ret_ssa->type) == 32 ? float32FF : float64FF;
               outputInterface.insert({ret_ssa, {ssa_ff, std::vector<tree_nodeRef>({vc_stmt})}});
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Output interface required for current variable use");
            }
         }
         const auto lowered_ret = GetPointerS<gimple_assign>(vc_stmt)->op0;
         bb->PushBefore(vc_stmt, ret_stmt, AppM);
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
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Parameters binding " + STR(bindingCompleted ? "" : "partially ") + "completed on " +
                            STR(paramBinding.size()) + " arguments");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
         size_t idx;
         for(idx = 0; idx < fd->list_of_args.size(); ++idx)
         {
            const auto& param = paramBinding.at(idx);
            const auto& arg = fd->list_of_args.at(idx);
            const auto pd = GetPointerS<const parm_decl>(arg);
            if(param)
            {
               THROW_ASSERT(param->get_kind() == ssa_name_K,
                            "Unexpected parameter node type (" + param->get_kind_text() + ")");
               const auto parmSSA = GetPointerS<ssa_name>(param);

               if(pd->type->index != parmSSA->type->index)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "Lowering type of " + parmSSA->ToString() + " bound to paremeter " + pd->ToString() +
                                     ": " + parmSSA->type->ToString() + " -> " + pd->type->ToString());
                  parmSSA->type = pd->type;

                  // Remove ssa variable associated to function parameter to avoid multiple type replacement
                  viewConvert.erase(parmSSA);
               }
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "Missing binding for parameter " + pd->ToString());
            }
         }
         paramBinding.clear();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
   }

   if(viewConvert.size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Lowering type for " + STR(viewConvert.size()) + " ssa variables");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(const auto& ssa_var : viewConvert)
      {
         ssa_lowering(ssa_var.first, ssa_var.second);
      }
      modified = true;
      viewConvert.clear();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }

   if(nopConvert.size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Lowering " + STR(nopConvert.size()) + " view-convert expressions to nop expressions");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(const auto& vcStmt : nopConvert)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Before lowering - " + vcStmt->ToString());
         const auto ga = GetPointerS<gimple_assign>(vcStmt);
         THROW_ASSERT(ga, "");
         const auto vc = GetPointerS<view_convert_expr>(ga->op1);
         THROW_ASSERT(vc, "");
         THROW_ASSERT(tree_helper::CGetType(vc->op)->get_kind() == integer_type_K,
                      "At this point " + vc->op->ToString() + " should be of integer type.");
         const auto resType = tree_helper::CGetType(ga->op0);
         THROW_ASSERT(resType->get_kind() == integer_type_K,
                      "Destination variable should of integer type (" + resType->get_kind_text() + ")");
         const auto nop = tree_man->create_unary_operation(resType, vc->op, BUILTIN_SRCP, nop_expr_K);
         TreeM->ReplaceTreeNode(vcStmt, ga->op1, nop);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---After lowering - " + vcStmt->ToString());
      }
      modified = true;
      nopConvert.clear();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }

   if(inputInterface.size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Generating input interface for " + STR(inputInterface.size()) + " variables");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(auto& [SSA, if_info] : inputInterface)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Input interface for " + SSA->ToString());
         const auto ssa = TreeM->GetTreeNode(SSA->index);
         auto& [fformat, exclude] = if_info;
         const auto oentry = outputInterface.find(SSA);
         if(oentry != outputInterface.end())
         {
            const auto& oentry_list = std::get<1>(oentry->second);
            std::transform(oentry_list.begin(), oentry_list.end(), std::back_inserter(exclude),
                           [&](const tree_nodeRef& tn) {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---Skipping replacement for statement " + tn->ToString());
                              return tn->index;
                           });
            outputInterface.erase(oentry);
         }

         // Get ssa uses before renaming to avoid replacement in cast rename operations
         const auto ssaUses = SSA->CGetUseStmts();
         if(ssaUses.size() == exclude.size())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Input interface for " + SSA->ToString() + " has no users, skipping...");
            continue;
         }

         auto defStmt = SSA->CGetDefStmt();
         const auto def = GetPointerS<gimple_node>(defStmt);
         blocRef bb;
         if(def->get_kind() == gimple_assign_K)
         {
            THROW_ASSERT(sl->list_of_bloc.count(def->bb_index),
                         "BB " + STR(def->bb_index) + " not present in current function.");
            bb = sl->list_of_bloc.at(def->bb_index);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Input interface will be inserted in BB" + STR(bb->number));
         }
         else if(def->get_kind() == gimple_phi_K)
         {
            THROW_ASSERT(sl->list_of_bloc.count(def->bb_index),
                         "BB " + STR(def->bb_index) + " not present in current function.");
            bb = sl->list_of_bloc.at(def->bb_index);
            defStmt = nullptr;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Input interface for phi will be inserted in BB" + STR(bb->number));
         }
         else
         {
            THROW_ASSERT(sl->list_of_bloc.at(BB_ENTRY)->list_of_succ.size() == 1,
                         "Multiple successors after entry basic block.");
            const auto realEntryBBIdx = sl->list_of_bloc.at(BB_ENTRY)->list_of_succ.front();
            bb = sl->list_of_bloc.at(realEntryBBIdx);
            defStmt = nullptr;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Input interface for parameter will be inserted in BB" + STR(bb->number));
         }

         const auto convertedSSA = generate_interface(bb, defStmt, ssa, fformat, _version->userRequired);
         const auto convertedSSA_type = tree_helper::CGetType(convertedSSA);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Interface from " + fformat->ToString() + " to " + _version->userRequired->ToString() +
                            " generated output " + convertedSSA->ToString());

         for(const auto& ssaUse : ssaUses)
         {
            const auto& useStmt = ssaUse.first;
            if(std::find(exclude.begin(), exclude.end(), useStmt->index) != exclude.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Skipping replacement for statement " + useStmt->ToString());
               continue;
            }
            TreeM->ReplaceTreeNode(useStmt, ssa, convertedSSA);
            const auto* ga = GetPointer<const gimple_assign>(useStmt);
            if(ga)
            {
               // Unary and binary expression have already been lowered to function calls
               auto* te = GetPointer<ternary_expr>(ga->op1);
               if(te)
               {
                  te->type = TreeM->GetTreeNode(convertedSSA_type->index);
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Replaced in statement " + useStmt->ToString());
            modified = true;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      inputInterface.clear();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }

   if(outputInterface.size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Generating output interface for " + STR(outputInterface.size()) + " variables");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(const auto& [SSA, if_info] : outputInterface)
      {
         const auto ssa = TreeM->GetTreeNode(SSA->index);
         const auto& [fformat, useStmts] = if_info;

         auto defStmt = SSA->CGetDefStmt();
         const auto gn = GetPointerS<gimple_node>(defStmt);
         THROW_ASSERT(sl->list_of_bloc.count(gn->bb_index),
                      "BB" + STR(gn->bb_index) + " not present in current function.");
         auto bb = sl->list_of_bloc.at(gn->bb_index);
         if(gn->get_kind() == gimple_nop_K)
         {
            THROW_ASSERT(bb->number == BB_ENTRY, "Parameter definition should be associated to entry block");
            THROW_ASSERT(bb->list_of_succ.size() == 1, "Multiple successors after entry basic block.");
            THROW_ASSERT(sl->list_of_bloc.count(bb->list_of_succ.front()),
                         "BB " + STR(bb->list_of_succ.front()) + " not present in current function.");
            defStmt = nullptr;
            bb = sl->list_of_bloc.at(bb->list_of_succ.front());
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Output interface for " + SSA->ToString() + " will be inserted in BB" + STR(bb->number));

         const auto convertedSSA = generate_interface(bb, defStmt, ssa, _version->userRequired, fformat);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Interface generated output " + convertedSSA->ToString());
         for(const auto& stmt : useStmts)
         {
            TreeM->ReplaceTreeNode(stmt, ssa, convertedSSA);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Replaced in statement " + stmt->ToString());
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
   return DesignFlowStep_Status::UNCHANGED;
}

bool soft_float_cg_ext::lowering_needed(const ssa_name* ssa)
{
   if(tree_helper::IsPointerType(ssa->type))
   {
      return tree_helper::IsRealType(tree_helper::CGetPointedType(ssa->type));
   }
   return tree_helper::IsRealType(ssa->type);
}

tree_nodeRef soft_float_cg_ext::int_type_for(const tree_nodeRef& type, bool use_internal) const
{
   if(tree_helper::IsPointerType(type))
   {
      return tree_helper::Size(tree_helper::CGetPointedType(type)) == 32 ? float32_ptr_type : float64_ptr_type;
   }
   if(!use_internal || _version->ieee_format())
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
   const auto f_name = tree_helper::print_type(TreeM, f_decl->index, false, true, false, 0U,
                                               var_pp_functorConstRef(new std_var_pp_functor(
                                                   AppM->CGetFunctionBehavior(f_decl->index)->CGetBehavioralHelper())));
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing function signature " + f_name);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   bool changed_parm = false, changed_type = false;
   const auto decl_type = f_decl->type;
   const auto is_ptr_type = decl_type->get_kind() == pointer_type_K;
   // Tree node decoupling is necessary when directly modifying a type node
   CustomUnorderedMapStable<unsigned int, unsigned int> remapping;
   const auto dup_ft = tree_node_dup(remapping, AppM)
                           .create_tree_node(is_ptr_type ? GetPointerS<pointer_type>(decl_type)->ptd : decl_type,
                                             tree_node_dup_mode::RENAME);
   const auto f_type = TreeM->GetTreeNode(dup_ft);

   tree_list* prms = nullptr;
   if(tree_helper::IsFunctionType(f_type))
   {
      const auto ft = GetPointerS<function_type>(f_type);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing return type " + ft->retn->ToString());
      if(tree_helper::IsRealType(ft->retn))
      {
         const auto int_ret = int_type_for(ft->retn, _version->internal);
         const auto ret_type = GetPointerS<const type_node>(int_ret);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Return type lowered to " + ret_type->ToString() + " " + STR(tree_helper::Size(int_ret)));
         ft->retn = int_ret;
         ft->algn = ret_type->algn;
         ft->qual = ret_type->qual;
         ft->size = ret_type->size;
         changed_type = true;
      }
      prms = ft->prms ? GetPointerS<tree_list>(ft->prms) : nullptr;
   }

   for(const auto& arg : f_decl->list_of_args)
   {
      const auto pd = GetPointerS<parm_decl>(arg);
      const auto& parm_type = pd->type;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Analysing parameter " + pd->ToString() + " of type " + STR(parm_type));
      if((tree_helper::IsPointerType(parm_type) && tree_helper::IsRealType(tree_helper::CGetPointedType(parm_type))) ||
         tree_helper::IsRealType(parm_type))
      {
         const auto int_parm_type = int_type_for(parm_type, _version->internal);
         const auto parm_int_type = GetPointerS<const type_node>(int_parm_type);
         pd->algn = parm_int_type->algn;
         pd->argt = int_parm_type;
         pd->packed_flag = parm_int_type->packed_flag;
         pd->type = int_parm_type;
         if(prms)
         {
            prms->valu = int_parm_type;
            changed_type = true;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Parameter type lowered to " + STR(int_parm_type) + " " +
                            STR(tree_helper::Size(int_parm_type)));
         changed_parm = true;
      }
      prms = prms ? (prms->chan ? GetPointerS<tree_list>(prms->chan) : nullptr) : nullptr;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   if(changed_type)
   {
      // Replace function type reference when modifications have been applied
      f_decl->type = is_ptr_type ? tree_man->GetPointerType(f_type) : f_type;
   }
   return changed_parm || changed_type;
}

void soft_float_cg_ext::ssa_lowering(ssa_name* ssa, bool internal_type) const
{
   THROW_ASSERT(lowering_needed(ssa), "Unexpected ssa type - " + ssa->ToString() + " " + STR(ssa->type));
   const auto vc_type = int_type_for(ssa->type, internal_type);
   THROW_ASSERT(vc_type, "");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Lowering " + ssa->ToString() + " type to " + STR(vc_type));

   const auto defStmt = ssa->CGetDefStmt();
   const auto def = GetPointer<gimple_assign>(defStmt);
   if(def)
   {
      const auto ue = GetPointer<unary_expr>(def->op1);
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
            const auto ssa_node = TreeM->GetTreeNode(ssa->index);
            const auto def_bb = GetPointerS<statement_list>(fd->body)->list_of_bloc.at(def->bb_index);
            const auto vc = tree_man->create_unary_operation(vc_type, ssa_node, BUILTIN_SRCP, view_convert_expr_K);
            const auto vc_stmt =
                tree_man->CreateGimpleAssign(vc_type, tree_nodeRef(), tree_nodeRef(), vc, function_id, BUILTIN_SRCP);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Inserting view-convert operation after complex part expression - " +
                               vc_stmt->ToString());
            const auto lowered_ssa = GetPointerS<gimple_assign>(vc_stmt)->op0;

            const auto ssa_uses = ssa->CGetUseStmts();
            for(const auto& stmt_uses : ssa_uses)
            {
               TreeM->ReplaceTreeNode(stmt_uses.first, ssa_node, lowered_ssa);
            }
            def_bb->PushAfter(vc_stmt, defStmt, AppM);
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
   if(ssa->var && ssa->var->get_kind() != parm_decl_K)
   {
      const auto vd = GetPointer<var_decl>(ssa->var);
      THROW_ASSERT(vd, "SSA name associated variable is espected to be a variable declaration " +
                           ssa->var->get_kind_text() + " " + ssa->var->ToString());
      if(vd->type->index != ssa->type->index)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Variable declaration before - " + vd->ToString() + " " + vd->type->ToString());
         const auto var_int_type = GetPointerS<type_node>(vc_type);
         vd->algn = var_int_type->algn;
         vd->packed_flag = var_int_type->packed_flag;
         vd->type = vc_type;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Variable declaration after - " + vd->ToString() + " " + vd->type->ToString());
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Variable declaration - " + vd->ToString() + " " + vd->type->ToString());
      }
   }

   for(const auto& use_count : ssa->CGetUseStmts())
   {
      const auto* ga = GetPointer<const gimple_assign>(use_count.first);
      if(ga)
      {
         // Unary and binary expression have already been lowered to function calls
         auto* te = GetPointer<ternary_expr>(ga->op1);
         if(te)
         {
            te->type = vc_type;
         }
      }
   }
}

tree_nodeRef soft_float_cg_ext::cstCast(uint64_t bits, const FloatFormatRef& inFF, const FloatFormatRef& outFF) const
{
   uint64_t Sign, Exp, Frac;

   Sign = bits >> (inFF->exp_bits + inFF->frac_bits);
   Exp = (bits >> (inFF->frac_bits)) & ((1ULL << inFF->exp_bits) - 1);
   Frac = bits & ((1ULL << inFF->frac_bits) - 1);

   uint64_t FExp, SFrac;
   bool ExpOverflow = false;

   const auto needed_bits = [](int i) -> auto
   {
      int lz;
      if(i > 0)
      {
         lz = 32 - __builtin_clz(static_cast<unsigned int>(i));
      }
      else
      {
         i = -i;
         lz = 32 - __builtin_clz(static_cast<unsigned int>(i)) + ((i & (i - 1)) != 0);
      }
      return static_cast<unsigned int>(lz);
   };
   const auto exp_bits_diff =
       inFF->exp_bits > outFF->exp_bits ? (inFF->exp_bits - outFF->exp_bits) : (outFF->exp_bits - inFF->exp_bits);
   const auto exp_type_size = std::max({static_cast<unsigned int>(inFF->exp_bits) + (exp_bits_diff == 1),
                                        static_cast<unsigned int>(outFF->exp_bits) + (exp_bits_diff == 1),
                                        needed_bits(inFF->exp_bias), needed_bits(outFF->exp_bias)});

   const auto biasDiff = inFF->exp_bias - outFF->exp_bias;
   const auto rangeDiff = ((1 << outFF->exp_bits) - !outFF->has_subnorm) - ((1 << inFF->exp_bits) - !inFF->has_subnorm);
   if((inFF->exp_bits != outFF->exp_bits) || (inFF->exp_bias != outFF->exp_bias))
   {
      FExp = Exp + static_cast<uint64_t>(biasDiff);
      bool ExpUnderflow = false;
      if(biasDiff < 0 || biasDiff > rangeDiff)
      {
         const auto expOverflow = (FExp >> outFF->exp_bits) & ((1ULL << (exp_type_size - outFF->exp_bits - 1)) - 1);
         ExpOverflow = expOverflow != 0ULL;
         ExpUnderflow = (FExp >> (exp_type_size - 1)) & 1;
         THROW_ASSERT((!ExpOverflow && !ExpUnderflow) || bits == 0,
                      "Target FP format can not represent a program constant.");
         const auto ExExp = ExpUnderflow ? 0ULL : ((1ULL << outFF->exp_bits) - 1);
         FExp = FExp & ((1ULL << outFF->exp_bits) - 1);
         FExp = ExpOverflow ? ExExp : FExp;
         Frac = ExpUnderflow ? 0 : Frac;
         ExpOverflow = ExpOverflow ^ ExpUnderflow;
      }

      FExp = FExp & ((1ULL << outFF->exp_bits) - 1);
      const bool ExpNull = Exp == 0;
      const bool FracNull = Frac == 0;
      bool inputZero = ExpNull && FracNull;
      if(biasDiff < 0 || biasDiff > rangeDiff)
      {
         inputZero = inputZero || ExpUnderflow;
      }
      FExp = inputZero ? 0ULL : FExp;
   }
   else
   {
      if(inFF->has_subnorm && !outFF->has_subnorm)
      {
         const bool ExpNull = Exp == 0;
         Frac = ExpNull ? 0ULL : Frac;
      }
      FExp = Exp;
   }

   if(inFF->frac_bits > outFF->frac_bits)
   {
      const auto bits_diff = inFF->frac_bits - outFF->frac_bits;

      SFrac = Frac >> bits_diff;

      if(outFF->rounding_mode == FloatFormat::FPRounding_NearestEven)
      {
         const bool GuardBit = (Frac >> (bits_diff - 1)) & 1;

         bool LSB = 0;
         if(bits_diff > 1)
         {
            const bool RoundBit = (Frac >> (bits_diff - 2)) & 1;
            LSB = LSB | RoundBit;
         }

         if(bits_diff > 2)
         {
            const bool Sticky = (Frac & ((1ULL << (bits_diff - 2)) - 1)) != 0;
            LSB = LSB | Sticky;
         }

         const bool Round = GuardBit & LSB;
         SFrac = SFrac | static_cast<uint64_t>(Round);
      }
   }
   else if(inFF->frac_bits < outFF->frac_bits)
   {
      const auto bits_diff = outFF->frac_bits - inFF->frac_bits;
      SFrac = Frac << bits_diff;
   }
   else
   {
      SFrac = Frac;
   }

   bool out_nan = false;
   if(outFF->sign != bit_lattice::U && inFF->sign != outFF->sign)
   {
      if(inFF->sign == bit_lattice::U)
      {
         out_nan |= Sign != (outFF->sign == bit_lattice::ONE ? 1 : 0);
      }
      else
      {
         THROW_ERROR("Casting from fixed " + STR(inFF->sign == bit_lattice::ONE ? "negative" : "positive") +
                     " type to fixed " + STR(outFF->sign == bit_lattice::ONE ? "negative" : "positive") +
                     " type will always result in a static value.");
         return nullptr;
      }
   }

   if(inFF->exception_mode == FloatFormat::FPException_IEEE)
   {
      out_nan |= Exp == ((1ULL << inFF->exp_bits) - 1);
   }
   uint64_t RExp, NFrac, RFrac;

   RExp = out_nan ? ((1ULL << outFF->exp_bits) - 1) : FExp;
   RExp <<= outFF->frac_bits;

   if(biasDiff < 0 || biasDiff > rangeDiff)
   {
      out_nan |= ExpOverflow;
   }

   if(outFF->exception_mode == FloatFormat::FPException_IEEE)
   {
      if(inFF->exception_mode == FloatFormat::FPException_IEEE)
      {
         const auto in_nan = (Exp == ((1ULL << inFF->exp_bits) - 1)) && (Frac != 0);
         NFrac = in_nan ? ((1ULL << outFF->frac_bits) - 1) : 0;
      }
      else
      {
         NFrac = 0;
      }
   }
   else
   {
      NFrac = ((1ULL << outFF->frac_bits) - 1);
   }

   RFrac = out_nan ? NFrac : SFrac;

   uint64_t out_val = RExp | RFrac;

   if(outFF->sign == bit_lattice::U)
   {
      uint64_t FSign;
      if(inFF->sign != bit_lattice::U)
      {
         FSign = inFF->sign == bit_lattice::ONE ? (1ULL << (outFF->exp_bits + outFF->frac_bits)) : 0;
      }
      else
      {
         FSign = Sign << (outFF->exp_bits + outFF->frac_bits);
      }
      out_val |= FSign;
   }

   return TreeM->CreateUniqueIntegerCst(
       static_cast<int64_t>(out_val),
       tree_man->GetCustomIntegerType(static_cast<unsigned int>(static_cast<uint8_t>(outFF->sign == bit_lattice::U) +
                                                                outFF->exp_bits + outFF->frac_bits),
                                      true));
}

#define FLOAT_CAST_FU_NAME "__float_cast"

tree_nodeRef soft_float_cg_ext::generate_interface(const blocRef& bb, tree_nodeRef stmt, const tree_nodeRef& ssa,
                                                   const FloatFormatRef inFF, const FloatFormatRef outFF) const
{
   static const auto default_bool_type = tree_man->GetBooleanType();
   static const auto default_int_type = tree_man->GetSignedIntegerType();

#if HAVE_ASSERTS
   const auto t_kind = tree_helper::CGetType(ssa)->get_kind();
#endif
   THROW_ASSERT(t_kind == integer_type_K,
                "Cast rename should be applied on integer variables only. " + tree_node::GetString(t_kind));
   THROW_ASSERT(inFF, "Interface input float format must be defined.");
   THROW_ASSERT(outFF, "Interface output float format must be defined.");

   const auto float_cast = TreeM->GetFunction(FLOAT_CAST_FU_NAME);
   THROW_ASSERT(float_cast, "The library miss this function " FLOAT_CAST_FU_NAME);
   const auto spec_suffix = inFF->ToString() + "_to_" + outFF->ToString();
   auto spec_function = TreeM->GetFunction(FLOAT_CAST_FU_NAME + spec_suffix);
   if(!spec_function)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Generating specialized version of " FLOAT_CAST_FU_NAME " (" + STR(float_cast->index) +
                         ") with fp format " + spec_suffix);
      spec_function = tree_man->CloneFunction(float_cast, spec_suffix);
      THROW_ASSERT(spec_function, "Error cloning function " FLOAT_CAST_FU_NAME " (" + STR(float_cast->index) + ").");
   }
   const std::vector<tree_nodeRef> args = {
       ssa,
       TreeM->CreateUniqueIntegerCst(static_cast<long long>(inFF->exp_bits), default_int_type),
       TreeM->CreateUniqueIntegerCst(static_cast<long long>(inFF->frac_bits), default_int_type),
       TreeM->CreateUniqueIntegerCst(static_cast<long long>(inFF->exp_bias), default_int_type),
       TreeM->CreateUniqueIntegerCst(static_cast<long long>(inFF->rounding_mode), default_int_type),
       TreeM->CreateUniqueIntegerCst(static_cast<long long>(inFF->exception_mode), default_int_type),
       TreeM->CreateUniqueIntegerCst(static_cast<long long>(inFF->has_one), default_bool_type),
       TreeM->CreateUniqueIntegerCst(static_cast<long long>(inFF->has_subnorm), default_bool_type),
       TreeM->CreateUniqueIntegerCst(
           static_cast<long long>(inFF->sign == bit_lattice::U ? -1 : (inFF->sign == bit_lattice::ONE ? 1 : 0)),
           default_int_type),
       TreeM->CreateUniqueIntegerCst(static_cast<long long>(outFF->exp_bits), default_int_type),
       TreeM->CreateUniqueIntegerCst(static_cast<long long>(outFF->frac_bits), default_int_type),
       TreeM->CreateUniqueIntegerCst(static_cast<long long>(outFF->exp_bias), default_int_type),
       TreeM->CreateUniqueIntegerCst(static_cast<long long>(outFF->rounding_mode), default_int_type),
       TreeM->CreateUniqueIntegerCst(static_cast<long long>(outFF->exception_mode), default_int_type),
       TreeM->CreateUniqueIntegerCst(static_cast<long long>(outFF->has_one), default_bool_type),
       TreeM->CreateUniqueIntegerCst(static_cast<long long>(outFF->has_subnorm), default_bool_type),
       TreeM->CreateUniqueIntegerCst(
           static_cast<long long>(outFF->sign == bit_lattice::U ? -1 : (outFF->sign == bit_lattice::ONE ? 1 : 0)),
           default_int_type),
   };
   const auto float_cast_call = tree_man->CreateCallExpr(spec_function, args, BUILTIN_SRCP);
   const auto ret_type = tree_helper::GetFunctionReturnType(spec_function);
   const auto cast_stmt = tree_man->CreateGimpleAssign(ret_type, tree_nodeConstRef(), tree_nodeConstRef(),
                                                       float_cast_call, function_id, BUILTIN_SRCP);
   if(stmt == nullptr)
   {
      bb->PushFront(cast_stmt, AppM);
   }
   else
   {
      bb->PushAfter(cast_stmt, stmt, AppM);
   }
   auto out_var = GetPointer<const gimple_assign>(cast_stmt)->op0;
   const auto out_type =
       tree_man->GetCustomIntegerType(static_cast<unsigned int>(static_cast<uint8_t>(outFF->sign == bit_lattice::U) +
                                                                outFF->exp_bits + outFF->frac_bits),
                                      true);
   if(!tree_helper::IsSameType(ret_type, out_type))
   {
      const auto nop_stmt =
          tree_man->CreateNopExpr(out_var, out_type, tree_nodeConstRef(), tree_nodeConstRef(), function_id);
      out_var = GetPointer<const gimple_assign>(nop_stmt)->op0;
      bb->PushAfter(nop_stmt, cast_stmt, AppM);
   }
   if(inline_conversion)
   {
      FunctionCallOpt::RequestCallOpt(cast_stmt, function_id, FunctionOptType::INLINE);
   }

   // Update functions float format map
   const auto called_func_vertex = AppM->CGetCallGraphManager()->GetVertex(spec_function->index);
   const auto calledFF = FunctionVersionRef(new FunctionVersion(called_func_vertex, inFF));
#if HAVE_ASSERTS
   const auto res =
#endif
       funcFF.insert(std::make_pair(called_func_vertex, calledFF));
   THROW_ASSERT(res.second || res.first->second->compare(*calledFF, true) == 0,
                "Same function registered with different formats: " + res.first->second->ToString() + " and " +
                    calledFF->ToString() + " (" FLOAT_CAST_FU_NAME ")");
   return out_var;
}

tree_nodeRef soft_float_cg_ext::floatNegate(const tree_nodeRef& op, const FloatFormatRef& ff) const
{
   if(ff->sign == bit_lattice::U)
   {
      const auto int_ff_type = tree_man->GetCustomIntegerType(1U + ff->exp_bits + ff->frac_bits, true);
      return tree_man->create_binary_operation(
          int_ff_type, op, TreeM->CreateUniqueIntegerCst(1LL << (ff->exp_bits + ff->frac_bits), int_ff_type),
          BUILTIN_SRCP, bit_xor_expr_K);
   }
   else
   {
      THROW_ERROR("Negate operation on fixed sign type will flatten all values.");
      return nullptr;
   }
}

tree_nodeRef soft_float_cg_ext::floatAbs(const tree_nodeRef& op, const FloatFormatRef& ff) const
{
   if(ff->sign == bit_lattice::U)
   {
      const auto int_ff_type = tree_man->GetCustomIntegerType(1U + ff->exp_bits + ff->frac_bits, true);
      return tree_man->create_binary_operation(
          int_ff_type, op, TreeM->CreateUniqueIntegerCst((1LL << (ff->exp_bits + ff->frac_bits)) - 1, int_ff_type),
          BUILTIN_SRCP, bit_and_expr_K);
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

void soft_float_cg_ext::replaceWithCall(const FloatFormatRef& specFF, const std::string& fu_name,
                                        std::vector<tree_nodeRef> args, const tree_nodeRef& current_statement,
                                        const tree_nodeRef& current_tree_node, const std::string& current_srcp)
{
   THROW_ASSERT(specFF, "FP format specialization missing");

   auto called_function = TreeM->GetFunction(fu_name);
   THROW_ASSERT(called_function, "The library miss this function " + fu_name);
   const auto spec_suffix = specFF->ToString();
   auto spec_function = TreeM->GetFunction(fu_name + spec_suffix);
   if(!spec_function)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Generating specialized version of " + fu_name + " (" + STR(called_function->index) +
                         ") with fp format " + spec_suffix);
      spec_function = tree_man->CloneFunction(called_function, spec_suffix);
      THROW_ASSERT(spec_function, "Error cloning function " + fu_name + " (" + STR(called_function->index) + ").");

      auto& version_args = versioning_args[spec_function->index];
      static const auto default_bool_type = tree_man->GetBooleanType();
      static const auto default_int_type = tree_man->GetSignedIntegerType();

      version_args[0] = TreeM->CreateUniqueIntegerCst(static_cast<long long>(specFF->exp_bits), default_int_type);
      version_args[1] = TreeM->CreateUniqueIntegerCst(static_cast<long long>(specFF->frac_bits), default_int_type);
      version_args[2] = TreeM->CreateUniqueIntegerCst(static_cast<long long>(specFF->exp_bias), default_int_type);
      version_args[3] = TreeM->CreateUniqueIntegerCst(static_cast<long long>(specFF->rounding_mode), default_int_type);
      version_args[4] = TreeM->CreateUniqueIntegerCst(static_cast<long long>(specFF->exception_mode), default_int_type);
      version_args[5] = TreeM->CreateUniqueIntegerCst(static_cast<long long>(specFF->has_one), default_bool_type);
      version_args[6] = TreeM->CreateUniqueIntegerCst(static_cast<long long>(specFF->has_subnorm), default_bool_type);
      version_args[7] = TreeM->CreateUniqueIntegerCst(
          static_cast<long long>(specFF->sign == bit_lattice::U ? -1 : (specFF->sign == bit_lattice::ONE ? 1 : 0)),
          default_int_type);
   }
   THROW_ASSERT(static_cast<bool>(versioning_args.count(spec_function->index)),
                "Static arguments for specialization parameters of " + fu_name + spec_suffix + " (" +
                    STR(spec_function->index) + ") where not specified.");
   std::copy(versioning_args.at(spec_function->index).begin(), versioning_args.at(spec_function->index).end(),
             std::back_inserter(args));
   called_function = spec_function;
   TreeM->ReplaceTreeNode(current_statement, current_tree_node,
                          tree_man->CreateCallExpr(called_function, args, current_srcp));
   CallGraphManager::addCallPointAndExpand(already_visited, AppM, function_id, called_function->index,
                                           current_statement->index, FunctionEdgeInfo::CallType::direct_call,
                                           DEBUG_LEVEL_NONE);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Added call point for " + STR(called_function->index));

   // Update functions float format map
   const auto called_func_vertex = AppM->CGetCallGraphManager()->GetVertex(called_function->index);
   const auto calledFF = FunctionVersionRef(new FunctionVersion(called_func_vertex, specFF));
#if HAVE_ASSERTS
   const auto res =
#endif
       funcFF.insert(std::make_pair(called_func_vertex, calledFF));
   THROW_ASSERT(res.second || res.first->second->compare(*calledFF, true) == 0,
                "Same function registered with different formats: " + res.first->second->ToString() + " and " +
                    calledFF->ToString() + " (" + fu_name + ")");
}

bool soft_float_cg_ext::RecursiveExaminate(const tree_nodeRef& current_statement, const tree_nodeRef& current_tree_node,
                                           int type_interface)
{
   THROW_ASSERT((type_interface & 3) == INTERFACE_TYPE_NONE || (type_interface & 3) == INTERFACE_TYPE_INPUT ||
                    (type_interface & 3) == INTERFACE_TYPE_OUTPUT,
                "Required interface type must be unique (" + STR(type_interface) + ")");
   const auto curr_tn = current_tree_node;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Update recursively (ti=" + STR(type_interface) + ") (" + STR(current_tree_node->index) + " " +
                      curr_tn->get_kind_text() + ") " + STR(current_tree_node));
   const auto current_srcp = [curr_tn]() -> std::string {
      const auto srcp_tn = GetPointer<const srcp>(curr_tn);
      if(srcp_tn)
      {
         return srcp_tn->include_name + ":" + STR(srcp_tn->line_number) + ":" + STR(srcp_tn->column_number);
      }
      return "";
   }();
   bool modified = false;
   const auto is_internal_call = [&](const tree_nodeRef& fn) -> bool {
      static const auto mcpy_node = TreeM->GetFunction(MEMCPY);
      static const auto mset_node = TreeM->GetFunction(MEMSET);
      const auto fn_fd = GetPointerS<function_decl>(fn);
      if(!fn_fd->body)
      {
         if(!_version->ieee_format() && tree_helper::print_function_name(TreeM, fn_fd) == BUILTIN_WAIT_CALL)
         {
            THROW_UNREACHABLE("Function pointers not supported from user defined floating point format functions");
            // TODO: maybe it could be possible to only warn the user here to be careful about the pointed function
            // definition and go on
         }
         return _version->ieee_format();
      }
      const auto fn_v = AppM->CGetCallGraphManager()->GetVertex(fn->index);
      const auto ff_it = funcFF.find(fn_v);
      if(ff_it != funcFF.end())
      {
         return ff_it->second->internal && _version->ieee_format() == ff_it->second->ieee_format();
      }
      else if(fn_fd->builtin_flag || fn_fd->index == mcpy_node->index || fn_fd->index == mset_node->index)
      {
         return _version->ieee_format();
      }
      THROW_UNREACHABLE("");
      return false;
   };
   const auto ExaminateFunctionCall = [&](tree_nodeRef fn) -> int {
      const auto ae = GetPointer<addr_expr>(fn);
      if(ae)
      {
         fn = ae->op;
         const auto called_fd = GetPointerS<const function_decl>(fn);
         ae->type = called_fd->type->get_kind() == pointer_type_K ? called_fd->type :
                                                                    tree_man->GetPointerType(called_fd->type);
         modified = true;
      }
      if(tree_helper::print_function_name(TreeM, GetPointerS<const function_decl>(fn)) == BUILTIN_WAIT_CALL)
      {
         if(_version->ieee_format())
         {
            return INTERFACE_TYPE_NONE;
         }
         THROW_UNREACHABLE("Function pointers not supported from user defined floating point format functions");
         // TODO: maybe it could be possible to only warn the user here to be careful about the pointed function
         // definition and go on
      }

      int type_i = is_internal_call(fn) ? INTERFACE_TYPE_NONE : INTERFACE_TYPE_OUTPUT;
      // Hardware implemented functions need arguments to still be real_type, thus it is necessary to add a view_convert
      // operation before
      if(AppM->get_tree_manager()->get_implementation_node(fn->index) == 0)
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
         const auto rhs_type = ga->op1->get_kind();
         if(rhs_type == call_expr_K || rhs_type == aggr_init_expr_K)
         {
            const auto ce = GetPointerS<const call_expr>(ga->op1);
            const auto fn = GetPointer<const addr_expr>(ce->fn) ? GetPointerS<const addr_expr>(ce->fn)->op : ce->fn;
            const auto fname = tree_helper::print_function_name(TreeM, GetPointerS<const function_decl>(fn));
            bool is_f32 = false;
            const auto tf_fname = strip_fname(fname, &is_f32);
            if(supported_libm_calls.count(tf_fname))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Replacing libm call with templatized version");
               // libm function calls may be replaced with their templatized version if available, avoiding conversion
               AppM->GetCallGraphManager()->RemoveCallPoint(function_id, fn->index, current_statement->index);
               is_f32 |= !ce->args.empty() && tree_helper::Size(ce->args.front()) == 32;
               const auto specFF = _version->ieee_format() ? (is_f32 ? float32FF : float64FF) : _version->userRequired;
               replaceWithCall(specFF, "__" + tf_fname, ce->args, current_statement, ga->op1, current_srcp);
               RecursiveExaminate(current_statement, ga->op0, INTERFACE_TYPE_NONE);
               if(supported_libm_calls_inlined.count(tf_fname))
               {
                  FunctionCallOpt::RequestCallOpt(current_statement, function_id, FunctionOptType::INLINE);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Call inlining required");
               }
               modified = true;
            }
            else
            {
               const auto type_i = [&]() {
                  // Return values associated to non-internal calls need to be cast renamed to local float format
                  int ti = is_internal_call(fn) ? type_interface : INTERFACE_TYPE_INPUT;

                  // Hardware implemented functions need the return value to still be real_type, thus it is necessary to
                  // add a view_convert operation after
                  if(fname != BUILTIN_WAIT_CALL && AppM->get_tree_manager()->get_implementation_node(fn->index) == 0)
                  {
                     ti |= INTERFACE_TYPE_REAL;
                  }
                  return ti;
               }();
               RecursiveExaminate(current_statement, ga->op0, type_i);
            }
         }
         else if(tree_helper::IsLoad(current_statement, function_behavior->get_function_mem()))
         {
            // Values loaded from memory need to be cast renamed to local float format
            RecursiveExaminate(current_statement, ga->op0, INTERFACE_TYPE_INPUT);
         }
         else
         {
            RecursiveExaminate(current_statement, ga->op0, type_interface);
         }
         if(tree_helper::IsStore(current_statement, function_behavior->get_function_mem()))
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
         if(lowering_needed(SSA))
         {
            // Real variables must all be converted to unsigned integers after softfloat lowering operations
            viewConvert.insert({SSA, type_interface == INTERFACE_TYPE_NONE});
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Lowering required for current variable");

            if(type_interface & INTERFACE_TYPE_REAL)
            {
               if(SSA->CGetDefStmt()->index == current_statement->index)
               {
                  hwReturn.push_back(SSA);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Internal input interface required");
               }
               else
               {
                  hwParam[SSA].insert(current_statement->index);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Internal output interface required");
               }
            }
         }

         // Search for ssa variables associated to input parameters
         if(!bindingCompleted)
         {
            const auto& args = fd->list_of_args;
            // If ssa_name references a parm_decl and is defined by a gimple_nop, it represents the formal function
            // parameter inside the function body
            if(SSA->var != nullptr && SSA->var->get_kind() == parm_decl_K &&
               SSA->CGetDefStmt()->get_kind() == gimple_nop_K)
            {
               auto argIt = std::find_if(args.begin(), args.end(),
                                         [&](const tree_nodeRef& arg) { return arg->index == SSA->var->index; });
               THROW_ASSERT(argIt != args.end(), "parm_decl associated with ssa_name not found in function parameters");
               const auto arg_pos = static_cast<size_t>(argIt - args.begin());
               THROW_ASSERT(arg_pos < args.size(), "Computed parameter position outside actual parameters number");
               paramBinding[arg_pos] = curr_tn;
               bindingCompleted = std::find(paramBinding.begin(), paramBinding.end(), nullptr) == paramBinding.end();
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Variable " + SSA->ToString() + " is defined from parameter " + STR(arg_pos));
            }
         }

         if(!_version->ieee_format() && tree_helper::IsRealType(SSA->type))
         {
            const auto ssa_ff = tree_helper::Size(SSA->type) == 32 ? float32FF : float64FF;
            if((!_version->internal &&
                std::find(paramBinding.begin(), paramBinding.end(), curr_tn) != paramBinding.end()) ||
               type_interface & INTERFACE_TYPE_INPUT)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Input interface required for current parameter");
               if(type_interface & INTERFACE_TYPE_OUTPUT)
               {
                  // Considered ssa has been discovered to be a function parameter and is used in current statement as a
                  // non-internal function argument, thus conversion can be avoided
                  const auto iif = inputInterface.find(SSA);
                  if(iif == inputInterface.end())
                  {
                     THROW_ASSERT(ssa_ff, "");
                     inputInterface.insert({SSA, {ssa_ff, std::vector<unsigned int>({current_statement->index})}});
                  }
                  else
                  {
                     std::get<1>(iif->second).push_back(current_statement->index);
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Skipping input interface for current parameter");
                  break;
               }
               if(!static_cast<bool>(inputInterface.count(SSA)))
               {
                  // Add current input SSA to the input cast rename list for all its uses if not already present
                  THROW_ASSERT(ssa_ff, "");
                  inputInterface.insert({SSA, {ssa_ff, std::vector<unsigned int>()}});
               }
            }
            else if(type_interface & INTERFACE_TYPE_OUTPUT)
            {
               const auto iif = inputInterface.find(SSA);
               if(iif != inputInterface.end())
               {
                  std::get<1>(iif->second).push_back(current_statement->index);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Uninterfaced value forward required");
               }
               else
               {
                  // Add current output SSA to the output cast rename list for its uses in current statement
                  const auto oif = outputInterface.find(SSA);
                  if(oif == outputInterface.end())
                  {
                     outputInterface.insert({SSA, {ssa_ff, std::vector<tree_nodeRef>({current_statement})}});
                  }
                  else
                  {
                     std::get<1>(oif->second).push_back(current_statement);
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Output interface required for current variable use");
               }
            }
         }

         break;
      }
      case tree_list_K:
      {
         auto current = current_tree_node;
         while(current)
         {
            RecursiveExaminate(current_statement, GetPointerS<const tree_list>(current)->valu, type_interface);
            current = GetPointerS<const tree_list>(current)->chan;
         }
         break;
      }
      case CASE_UNARY_EXPRESSION:
      {
         const auto ue = GetPointerS<const unary_expr>(curr_tn);
         const auto& expr_type = ue->type;
         const auto op_expr_type = tree_helper::CGetType(ue->op);
         // Propagate recursion with INTERFACE_TYPE_NONE to avoid cast rename of internal variables (input parameters
         // and constant will be converted anyway)
         RecursiveExaminate(current_statement, ue->op, INTERFACE_TYPE_NONE);
         if(tree_helper::IsRealType(expr_type))
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
                  FloatFormatRef outFF;
                  if(bitsize_out <= 32)
                  {
                     bitsize_out = 32;
                     outFF = float32FF;
                  }
                  else if(bitsize_out > 32 && bitsize_out <= 64)
                  {
                     bitsize_out = 64;
                     outFF = float64FF;
                  }
                  const auto bitsize_str_in = STR(bitsize_in);
                  const auto bitsize_str_out = bitsize_out == 96 ? "x80" : STR(bitsize_out);
                  if(!tree_helper::IsRealType(op_expr_type))
                  {
                     const auto is_unsigned = tree_helper::IsUnsignedIntegerType(op_expr_type);
                     const auto fu_name = "__" + std::string(is_unsigned ? "u" : "") + "int" + bitsize_str_in +
                                          "_to_float" + bitsize_str_out;
                     THROW_ASSERT(!_version->ieee_format() || outFF, "");
                     replaceWithCall(_version->ieee_format() ? outFF : _version->userRequired, fu_name, {ue->op},
                                     current_statement, current_tree_node, current_srcp);
                     if(inline_conversion)
                     {
                        FunctionCallOpt::RequestCallOpt(current_statement, function_id, FunctionOptType::INLINE);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Call inlining required");
                     }
                     modified = true;
                  }
                  break;
               }
               case view_convert_expr_K:
               {
                  // BEAWARE: this view_convert is from integer to real type, thus it will be a def statement of a real
                  // type ssa_name
                  //          def statements of real type ssa_name variables will be correctly replaced in the next
                  //          phase of this step
                  break;
               }
               case abs_expr_K:
               {
                  const auto float_size = tree_helper::Size(op_expr_type);
                  THROW_ASSERT(float_size == 32 || float_size == 64,
                               "Unhandled floating point format (size = " + STR(float_size) + ")");
                  const auto ff =
                      _version->ieee_format() ? (float_size == 32 ? float32FF : float64FF) : _version->userRequired;
                  THROW_ASSERT(ff, "Float format should be defined here");
                  const auto float_negate = floatAbs(ue->op, ff);
                  TreeM->ReplaceTreeNode(current_statement, current_tree_node, float_negate);
                  modified = true;
                  break;
               }
               case negate_expr_K:
               {
                  const auto float_size = tree_helper::Size(op_expr_type);
                  THROW_ASSERT(float_size == 32 || float_size == 64,
                               "Unhandled floating point format (size = " + STR(float_size) + ")");
                  const auto ff =
                      _version->ieee_format() ? (float_size == 32 ? float32FF : float64FF) : _version->userRequired;
                  THROW_ASSERT(ff, "Float format should be defined here");
                  const auto float_negate = floatNegate(ue->op, ff);
                  TreeM->ReplaceTreeNode(current_statement, current_tree_node, float_negate);
                  modified = true;
                  break;
               }
               case nop_expr_K:
               {
                  if(_version->ieee_format())
                  {
                     auto bitsize_in = tree_helper::Size(op_expr_type);
                     auto bitsize_out = tree_helper::Size(expr_type);
                     THROW_ASSERT(bitsize_in == 32 || bitsize_in == 64,
                                  "Unhandled input floating point format (size = " + STR(bitsize_in) + ")");
                     THROW_ASSERT(bitsize_out == 32 || bitsize_out == 64,
                                  "Unhandled output floating point format (size = " + STR(bitsize_out) + ")");
                     if(tree_helper::IsRealType(op_expr_type))
                     {
                        const auto fu_name = "__float" + STR(bitsize_in) + "_to_float" + STR(bitsize_out) + "_ieee";
                        const auto inFF = bitsize_in == 32 ? float32FF : float64FF;
                        const auto called_function = TreeM->GetFunction(fu_name);
                        THROW_ASSERT(called_function, "The library miss this function " + fu_name);
                        std::vector<tree_nodeRef> args = {
                            ue->op,
                            TreeM->CreateUniqueIntegerCst(static_cast<long long>(inFF->exception_mode),
                                                          tree_man->GetSignedIntegerType()),
                            TreeM->CreateUniqueIntegerCst(inFF->has_subnorm, tree_man->GetBooleanType())};
                        TreeM->ReplaceTreeNode(current_statement, current_tree_node,
                                               tree_man->CreateCallExpr(called_function, args, current_srcp));
                        CallGraphManager::addCallPointAndExpand(
                            already_visited, AppM, function_id, called_function->index, current_statement->index,
                            FunctionEdgeInfo::CallType::direct_call, DEBUG_LEVEL_NONE);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---Added call point for " + STR(called_function->index));
                        if(inline_conversion)
                        {
                           FunctionCallOpt::RequestCallOpt(current_statement, function_id, FunctionOptType::INLINE);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Call inlining required");
                        }
                     }
                     else
                     {
                        const auto vc_expr =
                            tree_man->create_unary_operation(expr_type, ue->op, current_srcp, view_convert_expr_K);
                        TreeM->ReplaceTreeNode(current_statement, current_tree_node, vc_expr);
                     }
                     modified = true;
                  }
                  else
                  {
                     THROW_UNREACHABLE("Operation not yet supported: function with user-defined floating point format "
                                       "should use a unique format type.");
                  }
                  break;
               }
               case indirect_ref_K:
               case imagpart_expr_K:
               case realpart_expr_K:
               case paren_expr_K:
                  break;
               case addr_expr_K:
               case alignof_expr_K:
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
         if(tree_helper::IsRealType(op_expr_type))
         {
            switch(curr_tn->get_kind())
            {
               case fix_trunc_expr_K:
               {
                  const auto bitsize_in = tree_helper::Size(op_expr_type);
                  const auto inFF = bitsize_in == 32 ? float32FF : (bitsize_in == 64 ? float64FF : nullptr);
                  auto bitsize_out = tree_helper::Size(expr_type);
                  if(bitsize_out < 32)
                  {
                     bitsize_out = 32;
                  }
                  else if(bitsize_out > 32 && bitsize_out < 64)
                  {
                     bitsize_out = 64;
                  }
                  const auto is_unsigned = tree_helper::IsUnsignedIntegerType(expr_type);
                  const auto bitsize_str_in = bitsize_in == 96 ? "x80" : STR(bitsize_in);
                  const auto bitsize_str_out = bitsize_out == 96 ? "x80" : STR(bitsize_out);
                  const auto fu_name = "__float" + bitsize_str_in + "_to_" + (is_unsigned ? "u" : "") + "int" +
                                       bitsize_str_out + "_round_to_zero";
                  replaceWithCall(_version->ieee_format() ? inFF : _version->userRequired, fu_name, {ue->op},
                                  current_statement, current_tree_node, current_srcp);
                  if(inline_conversion)
                  {
                     FunctionCallOpt::RequestCallOpt(current_statement, function_id, FunctionOptType::INLINE);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Call inlining required");
                  }
                  modified = true;
                  break;
               }
               case view_convert_expr_K:
               {
                  // This view_convert is from real to integer type variable, thus needs to be stored in the type
                  // conversion set of statements to be converted later if necessary
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
               case alignof_expr_K:
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
         // Get operand type before recursive examination because floating point operands may be converted to unsigned
         // integer during during recursion
         const auto expr_type = tree_helper::CGetType(be->op0);
         // Propagate recursion with INTERFACE_TYPE_NONE to avoid cast rename of internal variables (input parameters
         // and constant will be converted anyway)
         RecursiveExaminate(current_statement, be->op0,
                            (tree_helper::IsRealType(expr_type) &&
                             (curr_tn->get_kind() == unordered_expr_K || curr_tn->get_kind() == ordered_expr_K ||
                              curr_tn->get_kind() == complex_expr_K)) ?
                                INTERFACE_TYPE_REAL :
                                INTERFACE_TYPE_NONE);
         RecursiveExaminate(current_statement, be->op1,
                            (tree_helper::IsRealType(expr_type) &&
                             (curr_tn->get_kind() == unordered_expr_K || curr_tn->get_kind() == ordered_expr_K ||
                              curr_tn->get_kind() == complex_expr_K)) ?
                                INTERFACE_TYPE_REAL :
                                INTERFACE_TYPE_NONE);
         if(tree_helper::IsRealType(expr_type))
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
                        THROW_ERROR("FP-Division algorithm not supported:" +
                                    parameters->getOption<std::string>(OPT_hls_fpdiv));
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
               case extractvalue_expr_K:
               case extractelement_expr_K:
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
               case frem_expr_K:
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
               const auto opFF = bitsize == 32 ? float32FF : (bitsize == 64 ? float64FF : nullptr);
               const auto fu_name = "__float_" + fu_suffix;
               replaceWithCall(_version->ieee_format() ? opFF : _version->userRequired, fu_name, {be->op0, be->op1},
                               current_statement, current_tree_node, current_srcp);
               if(inline_math)
               {
                  FunctionCallOpt::RequestCallOpt(current_statement, function_id, FunctionOptType::INLINE);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Call inlining required");
               }
               modified = true;
            }
         }
         else if(tree_helper::IsRealType(be->type))
         {
            if(curr_tn->get_kind() == mem_ref_K)
            {
               const auto mr = GetPointerS<mem_ref>(curr_tn);
               mr->type = tree_helper::Size(mr->type) == 32 ? float32_type : float64_type;
            }
            else
            {
               THROW_UNREACHABLE("Real type expression not handled: " + curr_tn->get_kind_text() + " " +
                                 curr_tn->ToString());
            }
         }
         else if(tree_helper::IsPointerType(current_tree_node) &&
                 tree_helper::IsRealType(tree_helper::CGetPointedType(be->type)))
         {
            if(curr_tn->get_kind() == mem_ref_K)
            {
               const auto mr = GetPointerS<mem_ref>(curr_tn);
               mr->type = int_type_for(mr->type, true);
            }
            else if(curr_tn->get_kind() == pointer_plus_expr_K)
            {
               const auto pp = GetPointerS<pointer_plus_expr>(curr_tn);
               pp->type = int_type_for(pp->type, true);
            }
            else
            {
               THROW_UNREACHABLE("Real pointer type expression not handled: " + curr_tn->get_kind_text() + " " +
                                 curr_tn->ToString());
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
            if(isTopFunction && re->op->get_kind() == ssa_name_K && lowering_needed(GetPointerS<ssa_name>(re->op)))
            {
               topReturn.push_back(current_statement);
            }
            RecursiveExaminate(current_statement, re->op,
                               _version->internal ? INTERFACE_TYPE_NONE : INTERFACE_TYPE_OUTPUT);
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
            const auto bw = tree_helper::Size(current_tree_node);
            const auto fp_str =
                (cst->valx.front() == '-' && cst->valr.front() != cst->valx.front()) ? ("-" + cst->valr) : cst->valr;
            const auto cst_val = convert_fp_to_bits(fp_str, bw);
            tree_nodeRef int_cst;
            if(type_interface == INTERFACE_TYPE_OUTPUT || _version->ieee_format())
            {
               int_cst = TreeM->CreateUniqueIntegerCst(static_cast<long long>(cst_val),
                                                       tree_man->GetCustomIntegerType(bw, true));
            }
            else
            {
               const auto inFF = bw == 32 ? float32FF : float64FF;
               int_cst = cstCast(cst_val, inFF, _version->userRequired);
            }

            // Perform static constant value cast and replace real type constant with converted unsigned integer type
            // constant
            TreeM->ReplaceTreeNode(current_statement, current_tree_node, int_cst);
            modified = true;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Real type constant " + curr_tn->ToString() + " converted to " + int_cst->ToString());
         }
         break;
      }
      case integer_cst_K:
      //    {
      //       const auto cst_val = tree_helper::GetConstValue(curr_tn);
      //       if(tree_helper::IsPointerType(curr_tn))
      //       {
      //          const auto ptd_type = tree_helper::CGetPointedType(curr_tn);
      //          if(tree_helper::IsRealType(ptd_type))
      //          {
      //             const auto int_ptr_cst = TreeM->CreateUniqueIntegerCst(cst_val, tree_helper::Size(ptd_type) == 32
      //             ? float32_ptr_type : float64_ptr_type); TreeM->ReplaceTreeNode(current_statement,
      //             current_tree_node, int_ptr_cst); INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Real
      //             pointer type constant " + curr_tn->ToString() + " converted to " +
      //             int_ptr_cst->ToString());
      //          }
      //       }
      //       break;
      //    }
      case CASE_PRAGMA_NODES:
      case case_label_expr_K:
      case complex_cst_K:
      case field_decl_K:
      case function_decl_K:
      case gimple_asm_K:
      case gimple_goto_K:
      case gimple_label_K:
      case gimple_pragma_K:
      case label_decl_K:
      case result_decl_K:
      case string_cst_K:
      case tree_vec_K:
      case vector_cst_K:
      case void_cst_K:
         break;
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case binfo_K:
      case block_K:
      case const_decl_K:
      case error_mark_K:
      case gimple_bind_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case identifier_node_K:
      case namespace_decl_K:
      case statement_list_K:
      case target_expr_K:
      case template_decl_K:
      case translation_unit_decl_K:
      case using_decl_K:
      {
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "Not supported node: " + std::string(curr_tn->get_kind_text()));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "<--Updated recursively (" + STR(current_tree_node->index) + ") " + STR(current_tree_node));
   return modified;
}

FloatFormat::FloatFormat(uint8_t _exp_bits, uint8_t _frac_bits, int32_t _exp_bias, FPRounding _rounding_mode,
                         FPException _exception_mode, bool _has_one, bool _has_subnorm, bit_lattice _sign)
    : exp_bits(_exp_bits),
      frac_bits(_frac_bits),
      exp_bias(_exp_bias),
      rounding_mode(_rounding_mode),
      exception_mode(_exception_mode),
      has_one(_has_one),
      has_subnorm(_has_subnorm),
      sign(_sign)
{
}

bool FloatFormat::operator==(const FloatFormat& other) const
{
   return std::tie(exp_bits, frac_bits, exp_bias, rounding_mode, exception_mode, has_one, has_subnorm, sign) ==
          std::tie(other.exp_bits, other.frac_bits, other.exp_bias, other.rounding_mode, other.exception_mode,
                   other.has_one, other.has_subnorm, other.sign);
}

bool FloatFormat::operator!=(const FloatFormat& other) const
{
   return std::tie(exp_bits, frac_bits, exp_bias, rounding_mode, exception_mode, has_one, has_subnorm, sign) !=
          std::tie(other.exp_bits, other.frac_bits, other.exp_bias, other.rounding_mode, other.exception_mode,
                   other.has_one, other.has_subnorm, other.sign);
}

bool FloatFormat::ieee_format() const
{
   return ((exp_bits == 8 && frac_bits == 23 && exp_bias == -127) ||
           (exp_bits == 11 && frac_bits == 52 && exp_bias == -1023)) &&
          (rounding_mode == FPRounding_NearestEven && exception_mode == FPException_IEEE && has_one && !has_subnorm &&
           sign == bit_lattice::U);
}

std::string FloatFormat::ToString() const
{
   std::stringstream ss;
   ss << "e" << +exp_bits;
   ss << "m" << +frac_bits;
   ss << "b" << (exp_bias < 0 ? "_" : "");
   ss << std::abs(exp_bias);
   switch(rounding_mode)
   {
      case FPRounding_NearestEven:
         ss << "n";
         break;
      case FPRounding_Truncate:
         ss << "t";
      default:
         break;
   }
   switch(exception_mode)
   {
      case FPException_IEEE:
         ss << "i";
         break;
      case FPException_Saturation:
         ss << "a";
         break;
      case FPException_Overflow:
         ss << "o";
         break;
      default:
         break;
   }
   if(has_one)
   {
      ss << "h";
   }
   if(has_subnorm)
   {
      ss << "s";
   }
   switch(sign)
   {
      case bit_lattice::ONE:
         ss << "1";
         break;
      case bit_lattice::ZERO:
         ss << "0";
         break;
      case bit_lattice::U:
      case bit_lattice::X:
      default:
         break;
   }
   return ss.str();
}

#define FP_FORMAT_EXP 1
#define FP_FORMAT_SIG 2
#define FP_FORMAT_BIAS 3
#define FP_FORMAT_RND 4
#define FP_FORMAT_EXC 5
#define FP_FORMAT_SPEC 6
#define FP_FORMAT_SIGN 7
FloatFormatRef FloatFormat::FromString(std::string ff_str)
{
   std::replace(ff_str.begin(), ff_str.end(), '_', '-');
   static const std::regex fp_format("^e(\\d+)m(\\d+)b([_-]?\\d+)(\\D)(\\D)(\\D*)(\\d?)$");
   std::cmatch what;
   if(std::regex_search(ff_str.data(), what, fp_format))
   {
      const auto e = std::stoi(std::string(
          what[FP_FORMAT_EXP].first, static_cast<size_t>(what[FP_FORMAT_EXP].second - what[FP_FORMAT_EXP].first)));
      const auto m = std::stoi(std::string(
          what[FP_FORMAT_SIG].first, static_cast<size_t>(what[FP_FORMAT_SIG].second - what[FP_FORMAT_SIG].first)));
      const auto b = std::stoi(std::string(
          what[FP_FORMAT_BIAS].first, static_cast<size_t>(what[FP_FORMAT_BIAS].second - what[FP_FORMAT_BIAS].first)));
      FloatFormatRef ff(new FloatFormat(static_cast<uint8_t>(e), static_cast<uint8_t>(m), b));
      switch(*what[FP_FORMAT_RND].first)
      {
         case 't':
            ff->rounding_mode = FPRounding_Truncate;
            break;
         case 'n':
            ff->rounding_mode = FPRounding_NearestEven;
            break;
         default:
            break;
      }
      switch(*what[FP_FORMAT_EXC].first)
      {
         case 'i':
            ff->exception_mode = FPException_IEEE;
            break;
         case 'a':
            ff->exception_mode = FPException_Saturation;
            break;
         case 'o':
            ff->exception_mode = FPException_Overflow;
            break;
         default:
            break;
      }
      const auto spec = std::string(what[FP_FORMAT_SPEC].first,
                                    static_cast<size_t>(what[FP_FORMAT_SPEC].second - what[FP_FORMAT_SPEC].first));
      for(const auto& s : spec)
      {
         switch(s)
         {
            case 'h':
               ff->has_one = true;
               break;
            case 's':
               ff->has_subnorm = true;
               break;
            default:
               break;
         }
      }
      if(what[FP_FORMAT_SIGN].second - what[FP_FORMAT_SIGN].first)
      {
         const auto sign = static_cast<bool>(std::stoi(std::string(what[FP_FORMAT_SIGN].first, 1)));
         ff->sign = sign ? bit_lattice::ONE : bit_lattice::ZERO;
      }
      return ff;
   }
   return nullptr;
}

FunctionVersion::FunctionVersion() : function_vertex(nullptr), userRequired(nullptr), internal(true)
{
}

FunctionVersion::FunctionVersion(CallGraph::vertex_descriptor func_v, const FloatFormatRef& userFormat)
    : function_vertex(func_v), userRequired(userFormat), internal(true)
{
}

FunctionVersion::FunctionVersion(const FunctionVersion& other)
    : function_vertex(other.function_vertex),
      userRequired(other.ieee_format() ? nullptr : new FloatFormat(*other.userRequired)),
      internal(other.internal)
{
}

FunctionVersion::~FunctionVersion() = default;

int FunctionVersion::compare(const FunctionVersion& other, bool format_only) const
{
   return ((function_vertex != other.function_vertex || internal != other.internal) && !format_only) ||
          !((userRequired == nullptr && other.userRequired == nullptr) ||
            (userRequired != nullptr && other.userRequired != nullptr && *userRequired == *other.userRequired));
}

bool FunctionVersion::operator==(const FunctionVersion& other) const
{
   return compare(other) == 0;
}

bool FunctionVersion::operator!=(const FunctionVersion& other) const
{
   return compare(other) != 0;
}

bool FunctionVersion::ieee_format() const
{
   return userRequired == nullptr /*|| userRequired->ieee_format()*/;
}

std::string FunctionVersion::ToString() const
{
   return STR(function_vertex) + (internal ? "_internal_" : "") + (userRequired ? userRequired->ToString() : "");
}
