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
#include "function_behavior.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "ir_node_dup.hpp"
#include "math_function.hpp"
#include "op_graph.hpp"
#include "range_analysis_helper.hpp"
#include "string_manipulation.hpp"
#include "utility.hpp"
#include "var_pp_functor.hpp"

#include <boost/multiprecision/integer.hpp>

#include <algorithm>
#include <deque>
#include <list>
#include <regex>
#include <set>
#include <string>

#define TF_MATH_PREFIX "tf_"

CustomMap<CallGraph::vertex_descriptor, FunctionVersionRef> soft_float_cg_ext::funcFF;
CustomMap<std::string, std::array<ir_nodeRef, 8>> soft_float_cg_ext::spec_parms_map;
bool soft_float_cg_ext::inline_math = false;
bool soft_float_cg_ext::inline_conversion = false;
ir_nodeRef soft_float_cg_ext::float32_type;
ir_nodeRef soft_float_cg_ext::float32_ptr_type;
ir_nodeRef soft_float_cg_ext::float64_type;
ir_nodeRef soft_float_cg_ext::float64_ptr_type;

static const FloatFormatRef float32FF(new FloatFormat(8, 23, -127));
static const FloatFormatRef float64FF(new FloatFormat(11, 52, -1023));
static std::set<std::string> spec_libm_funcs = {};
static const std::set<std::string> spec_libm_funcs_inlined = {"copysign", "fabs"};

/**
 * @brief List of TrueFloat specializable functions
 */
static const std::set<std::string> truefloat_funcs = {
    "__float_add",        "__float_sub",       "__float_mul",       "__float_divSRT4",
    "__float_divSRT4U",   "__float_divG",      "__float_eq",        "__float_le",
    "__float_lt",         "__float_ge",        "__float_gt",        "__float_is_signaling_nan",
    "__float_ltgt_quiet", "__isunordered",     "__int_to_float",    "__int64_to_float",
    "__int32_to_float",   "__int16_to_float",  "__int8_to_float",   "__uint_to_float",
    "__uint64_to_float",  "__uint32_to_float", "__uint16_to_float", "__uint8_to_float",
    "__float_to_int",     "__float_to_int64",  "__float_to_int32",  "__float_to_uint",
    "__float_to_uint64",  "__float_to_uint32"};

/**
 * @brief List of low level implementation libm functions. Composite functions are not present since fp format can be
 * safely propagated there.
 */
static const std::set<std::string> libm_funcs = {
    "acos",           "acosh",       "asin",       "asinh",   "atan",        "atanh",         "atan2",
    "cbrt",           "ceil",        "copysign",   "cos",     "cosh",        "drem",          "erf",
    "erfc",           "exp",         "exp2",       "expm1",   "fabs",        "fdim",          "finite",
    "floor",          "fmax",        "fmin",       "fma",     "fmod",        "fpclassify",    "frexp",
    "huge_val",       "hypot",       "ilogb",      "inf",     "infinity",    "isfinite",      "isgreater",
    "isgreaterequal", "isinf",       "isinf_sign", "isless",  "islessequal", "islessgreater", "isnan",
    "isnormal",       "isunordered", "j0",         "j1",      "jn",          "ldexp",         "lgamma_r",
    "llrint",         "llround",     "log",        "log10",   "log1p",       "log2",          "logb",
    "lrint",          "lround",      "modf",       "nan",     "nans",        "nearbyint",     "nextafter",
    "nexttoward",     "pow",         "remainder",  "remquo",  "rint",        "round",         "scalb",
    "scalbln",        "scalbn",      "sin",        "signbit", "significand", "sinh",          "sincos",
    "sqrt",           "tan",         "tanh",       "tgamma",  "trunc"};

static std::string strip_fname(std::string fname, bool* single_prec = nullptr)
{
   if(single_prec)
   {
      *single_prec = false;
   }
   if(fname.find("__builtin_") == 0)
   {
      fname = fname.substr(sizeof("__builtin_") - 1);
   }
   else if(fname.find("__") == 0)
   {
      fname = fname.substr(sizeof("__") - 1);
   }
   if(fname.back() == 'f')
   {
      const auto fname_nof = fname.substr(0, fname.size() - 1);
      if(libm_funcs.count(fname_nof))
      {
         fname = fname_nof;
         if(single_prec)
         {
            *single_prec = true;
         }
      }
   }
   return fname;
}

soft_float_cg_ext::soft_float_cg_ext(const ParameterConstRef _parameters, const application_managerRef _AppM,
                                     unsigned int _function_id, const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, SOFT_FLOAT_CG_EXT, _design_flow_manager, _parameters),
      IRM(_AppM->get_ir_manager()),
      ir_man(new ir_manipulation(IRM, parameters, _AppM)),
      fd(GetPointer<function_val_node>(IRM->GetIRNode(function_id))),
      isTopFunction(AppM->CGetCallGraphManager().GetRootFunctions().count(function_id)),
      bindingCompleted(fd->list_of_args.size() == 0),
      paramBinding(fd->list_of_args.size(), nullptr)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);

   if(!float32_type)
   {
      float32_type = ir_man->GetCustomIntegerType(32, true);
      float32_ptr_type = ir_man->GetPointerType(float32_type, 32);
      float64_type = ir_man->GetCustomIntegerType(64, true);
      float64_ptr_type = ir_man->GetPointerType(float64_type, 64);
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
         else if(exc_mode == "nonan")
         {
            float32FF->exception_mode = FloatFormat::FPException_NoNan;
            float64FF->exception_mode = FloatFormat::FPException_NoNan;
         }
         else
         {
            THROW_UNREACHABLE("Floating-point exception mode not supported: " + STR(exc_mode));
         }
      }
      for(const auto& fname : libm_funcs)
      {
         const auto TM = AppM->get_ir_manager();
         const auto spec_fname = TF_MATH_PREFIX + fname;
         const auto spec_fnode = TM->GetFunction(spec_fname);
         if(spec_fnode)
         {
            spec_libm_funcs.insert(fname);
         }
      }
   }

   if(funcFF.empty() && !parameters->getOption<std::string>(OPT_fp_format).empty())
   {
      const auto& CGM = AppM->CGetCallGraphManager();
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
            const auto f_node = IRM->GetFunction(format[0]);
            return f_node ? f_node->index : 0;
         }();

         if(!f_index)
         {
            THROW_ERROR("Function " + format[0] + " does not exists. (Maybe it has been inlined)");
         }
         const auto function_v = CGM.GetVertex(f_index);
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
         for(const auto& root_func : CGM.GetRootFunctions())
         {
            std::list<CallGraph::vertex_descriptor> func_sort;
            CustomUnorderedSet<CallGraph::vertex_descriptor> reached_v;
            const auto reached_from_top = CGM.GetReachedFunctionsFrom(root_func);
            for(const auto func_id : reached_from_top)
            {
               reached_v.insert(CGM.GetVertex(func_id));
            }
            const auto& TopCG = CGM.CGetCallSubGraph(reached_v);
            TopCG.TopologicalSort(func_sort);

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

               const auto func_id = AppM->CGetCallGraphManager().get_function(func);
               INDENT_OUT_MEX(
                   OUTPUT_LEVEL_VERBOSE, output_level,
                   "Analysing function " +
                       ir_helper::PrintType(IRM->GetIRNode(func_id), true, false, ir_nodeRef(),
                                            std::make_unique<std_var_pp_functor>(
                                                AppM->CGetFunctionBehavior(func_id)->CGetBehavioralHelper())));
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---FP format " + current_v->ToString());

               // Propagate current fp format to the called functions
               for(const auto& ei : TopCG.out_edges(func))
               {
                  const auto called = TopCG.target(ei);
                  const auto fname = ir_helper::GetFunctionName(IRM->GetIRNode(CGM.get_function(called)));
                  const auto called_fname = strip_fname(fname);
                  if(static_cast<bool>(libm_funcs.count(called_fname)))
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
   THROW_ASSERT(AppM->CGetCallGraphManager().IsVertex(function_id), "");
   const auto function_v = AppM->CGetCallGraphManager().GetVertex(function_id);
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
                  ir_man->GetCustomIntegerType(
                      static_cast<unsigned int>(static_cast<uint8_t>(_version->userRequired->sign == bit_lattice::U) +
                                                _version->userRequired->exp_bits + _version->userRequired->frac_bits),
                      true) :
                  nullptr;
   int_ptr_type = int_type ? ir_man->GetPointerType(int_type, GetPointer<integer_ty_node>(int_type)->algn) : nullptr;
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
soft_float_cg_ext::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(DETERMINE_MEMORY_ACCESSES, SAME_FUNCTION));
         relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, ALL_FUNCTIONS));
         if(parameters->isOption(OPT_openmp) && parameters->getOption<bool>(OPT_openmp))
         {
            relationships.insert(std::make_pair(OMP_CG_EXT, ALL_FUNCTIONS));
         }
         relationships.insert(std::make_pair(SOFT_FLOAT_CG_EXT, CALLED_FUNCTIONS));
         relationships.insert(std::make_pair(IR2FUN, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
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
   static const auto ff_already_propagated =
       parameters->isOption(OPT_fp_format_propagate) && parameters->getOption<bool>(OPT_fp_format_propagate);
   // Check if current function needs IO fp format interface (avoid check if fp format propagation has already been
   // computed)
   if(!ff_already_propagated && !_version->ieee_format())
   {
      const auto& CG = AppM->CGetCallGraphManager().GetCallGraph();
      for(const auto& ie : CG.in_edges(_version->function_vertex))
      {
         if(static_cast<bool>(funcFF.count(CG.source(ie))))
         {
            const auto& funcV = funcFF.at(CG.source(ie));
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
   const auto fn_name =
       ir_helper::PrintType(IRM->GetIRNode(function_id), true, false, ir_nodeRef(),
                            std::make_unique<std_var_pp_functor>(function_behavior->CGetBehavioralHelper()));
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Function " + fn_name + " implementing " + _version->ToString() + " floating-point format");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---IO interface is " + STR((_version->ieee_format() || _version->internal) ? "not " : "") +
                      "necessary");

   const auto sl = GetPointerS<statement_list_node>(fd->body);
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
                     "Adding bitcast expressions to support hardware implemented FU call arguments");
      for(const auto& ssa_uses : hwParam)
      {
         const auto ssa = ssa_uses.first;
         const auto ssa_ref = IRM->GetIRNode(ssa->index);
         const auto out_int = outputInterface.find(ssa);
         std::vector<ir_nodeRef>* out_ssa = out_int != outputInterface.end() ? &std::get<1>(out_int->second) : nullptr;
         for(const auto& call_stmt_idx : ssa_uses.second)
         {
            const auto call_stmt = IRM->GetIRNode(call_stmt_idx);
            const auto call_node = GetPointerS<const node_stmt>(call_stmt);
            const auto& call_bb = sl->list_of_bloc.at(call_node->bb_index);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Bitcast for " + ssa->ToString() + " in BB" + STR(call_bb->number) + " " +
                               call_node->ToString());
            // At this time ssa->type is still real_ty_node, thus we can exploit that (it will be modified after)
            const auto arg_bitcast =
                ir_man->create_unary_operation(ssa->type, ssa_ref, BUILTIN_LOCINFO, bitcast_node_K);
            const auto bitcast_stmt = ir_man->CreateAssignStmt(ssa->type, ir_nodeRef(), ir_nodeRef(), arg_bitcast,
                                                               function_id, BUILTIN_LOCINFO);
            const auto bitcast_ssa = GetPointerS<assign_stmt>(bitcast_stmt)->op0;
            call_bb->PushBefore(bitcast_stmt, call_stmt, AppM);
            IRM->ReplaceIRNode(call_stmt, ssa_ref, bitcast_ssa);
            if(out_ssa)
            {
               std::replace_if(
                   out_ssa->begin(), out_ssa->end(), [&](const ir_nodeRef& t) { return t->index == call_stmt_idx; },
                   bitcast_stmt);
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
                     "Adding bitcast expressions to support hardware implemented FU call return values");
      for(const auto& ssa : hwReturn)
      {
         const auto call_stmt = ssa->GetDefStmt();
         const auto def_node = GetPointerS<const node_stmt>(call_stmt);
         const auto call_bb = sl->list_of_bloc.at(def_node->bb_index);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Bitcast for " + ssa->ToString() + " in BB" + STR(call_bb->number) + " " +
                            def_node->ToString());
         const auto ssa_ref = IRM->GetIRNode(ssa->index);
         // Hardware calls are for sure dealing with standard IEEE formats only
         const auto int_ret_type = ir_helper::Size(ssa->type) == 32 ? float32_type : float64_type;
         const auto ret_bitcast =
             ir_man->create_unary_operation(int_ret_type, ssa_ref, BUILTIN_LOCINFO, bitcast_node_K);
         const auto bitcast_stmt = ir_man->CreateAssignStmt(int_ret_type, ir_nodeRef(), ir_nodeRef(), ret_bitcast,
                                                            function_id, BUILTIN_LOCINFO);
         const auto bitcast_ssa = GetPointerS<const assign_stmt>(bitcast_stmt)->op0;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Added statement " + bitcast_stmt->ToString());
         const auto if_info = inputInterface.find(ssa);
         if(if_info != inputInterface.end())
         {
            const auto new_ssa = GetPointer<ssa_node>(bitcast_ssa);
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
            IRM->ReplaceIRNode(stmt_use.first, ssa_ref, bitcast_ssa);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---               after: " + stmt_use.first->ToString());
         }
         call_bb->PushAfter(bitcast_stmt, call_stmt, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         bitcastCandidates.erase(ssa);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      modified = true;
      hwReturn.clear();
   }

   // Design top function signatures must not be modified, thus a bitcast operation for real_ty_node parameters and
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
            THROW_ASSERT(parm->get_kind() == ssa_node_K,
                         "Unexpected parameter node type (" + parm->get_kind_text() + ")");
            const auto parmSSA = GetPointerS<ssa_node>(parm);
            if(lowering_needed(parmSSA))
            {
               const auto parm_type = int_type_for(parmSSA->type, false);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "Lowering top function parameter type of " + parmSSA->ToString() + ": " +
                                  parmSSA->type->ToString() + " -> " + parm_type->ToString());
               ir_nodeRef bitcast_stmt;
               if(parm_type->get_kind() == pointer_ty_node_K)
               {
                  bitcast_stmt = ir_man->CreateNopExpr(parm, parm_type, ir_nodeRef(), ir_nodeRef(), function_id);
               }
               else
               {
                  const auto bitcast_expr =
                      ir_man->create_unary_operation(parm_type, parm, BUILTIN_LOCINFO, bitcast_node_K);
                  bitcast_stmt = ir_man->CreateAssignStmt(parm_type, ir_nodeRef(), ir_nodeRef(), bitcast_expr,
                                                          function_id, BUILTIN_LOCINFO);
                  if(!_version->ieee_format())
                  {
                     const auto ssa_ff = ir_helper::Size(parmSSA->type) == 32 ? float32FF : float64FF;
                     const auto ientry =
                         inputInterface.insert({GetPointerS<ssa_node>(GetPointerS<assign_stmt>(bitcast_stmt)->op0),
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
                              "---Lowering statement added to BB" + STR(first_bb->number) + ": " +
                                  bitcast_stmt->ToString());
               const auto lowered_parm = GetPointerS<assign_stmt>(bitcast_stmt)->op0;
               const auto parm_uses = parmSSA->CGetUseStmts();
               for(const auto& stmt_uses : parm_uses)
               {
                  IRM->ReplaceIRNode(stmt_uses.first, parm, lowered_parm);
               }
               first_bb->PushFront(bitcast_stmt, AppM);
               bitcastCandidates.erase(parmSSA);
               modified = true;
            }
         }
      }
      paramBinding.clear();
      for(const auto& ret_stmt : topReturn)
      {
         const auto gr = GetPointerS<return_stmt>(ret_stmt);
         const auto ret_ssa = GetPointerS<ssa_node>(gr->op);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Return value type restore added for variable " + ret_ssa->ToString());
         const auto bb = sl->list_of_bloc.at(gr->bb_index);
         ir_nodeRef bitcast_stmt;
         if(ret_ssa->type->get_kind() == pointer_ty_node_K)
         {
            bitcast_stmt = ir_man->CreateNopExpr(gr->op, ret_ssa->type, ir_nodeRef(), ir_nodeRef(), function_id);
         }
         else
         {
            const auto bitcast_expr =
                ir_man->create_unary_operation(ret_ssa->type, gr->op, BUILTIN_LOCINFO, bitcast_node_K);
            bitcast_stmt = ir_man->CreateAssignStmt(ret_ssa->type, ir_nodeRef(), ir_nodeRef(), bitcast_expr,
                                                    function_id, BUILTIN_LOCINFO);
            if(!_version->ieee_format())
            {
               const auto ssa_ff = ir_helper::Size(ret_ssa->type) == 32 ? float32FF : float64FF;
               outputInterface.insert({ret_ssa, {ssa_ff, std::vector<ir_nodeRef>({bitcast_stmt})}});
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Output interface required for current variable use");
            }
         }
         const auto lowered_ret = GetPointerS<assign_stmt>(bitcast_stmt)->op0;
         bb->PushBefore(bitcast_stmt, ret_stmt, AppM);
         IRM->ReplaceIRNode(ret_stmt, gr->op, lowered_ret);
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
            const auto pd = GetPointerS<const argument_val_node>(arg);
            if(param)
            {
               THROW_ASSERT(param->get_kind() == ssa_node_K,
                            "Unexpected parameter node type (" + param->get_kind_text() + ")");
               const auto parmSSA = GetPointerS<ssa_node>(param);

               if(pd->type->index != parmSSA->type->index)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "Lowering type of " + parmSSA->ToString() + " bound to paremeter " + pd->ToString() +
                                     ": " + parmSSA->type->ToString() + " -> " + pd->type->ToString());
                  parmSSA->type = pd->type;

                  // Remove ssa variable associated to function parameter to avoid multiple type replacement
                  bitcastCandidates.erase(parmSSA);
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

   if(bitcastCandidates.size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Lowering type for " + STR(bitcastCandidates.size()) + " ssa variables");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(const auto& ssa_var : bitcastCandidates)
      {
         ssa_lowering(ssa_var.first, ssa_var.second);
      }
      modified = true;
      bitcastCandidates.clear();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }

   if(nopConvertibleBitcasts.size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Lowering " + STR(nopConvertibleBitcasts.size()) + " bitcast expressions to nop expressions");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(const auto& bitcast_stmt : nopConvertibleBitcasts)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Before lowering - " + bitcast_stmt->ToString());
         const auto ga = GetPointerS<assign_stmt>(bitcast_stmt);
         THROW_ASSERT(ga, "");
         const auto bitcast_expr = GetPointerS<bitcast_node>(ga->op1);
         THROW_ASSERT(bitcast_expr, "");
         THROW_ASSERT(ir_helper::CGetType(bitcast_expr->op)->get_kind() == integer_ty_node_K,
                      "At this point " + bitcast_expr->op->ToString() + " should be of integer type.");
         const auto resType = ir_helper::CGetType(ga->op0);
         THROW_ASSERT(resType->get_kind() == integer_ty_node_K,
                      "Destination variable should of integer type (" + resType->get_kind_text() + ")");
         const auto nop = ir_man->create_unary_operation(resType, bitcast_expr->op, BUILTIN_LOCINFO, nop_node_K);
         IRM->ReplaceIRNode(bitcast_stmt, ga->op1, nop);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---After lowering - " + bitcast_stmt->ToString());
      }
      modified = true;
      nopConvertibleBitcasts.clear();
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
         const auto ssa = IRM->GetIRNode(SSA->index);
         auto& [fformat, exclude] = if_info;
         const auto oentry = outputInterface.find(SSA);
         if(oentry != outputInterface.end())
         {
            const auto& oentry_list = std::get<1>(oentry->second);
            std::transform(oentry_list.begin(), oentry_list.end(), std::back_inserter(exclude),
                           [&](const ir_nodeRef& tn) {
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

         auto defStmt = SSA->GetDefStmt();
         const auto def = GetPointerS<node_stmt>(defStmt);
         blocRef bb;
         if(def->get_kind() == assign_stmt_K)
         {
            THROW_ASSERT(sl->list_of_bloc.count(def->bb_index),
                         "BB " + STR(def->bb_index) + " not present in current function.");
            bb = sl->list_of_bloc.at(def->bb_index);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Input interface will be inserted in BB" + STR(bb->number));
         }
         else if(def->get_kind() == phi_stmt_K)
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
         const auto convertedSSA_type = ir_helper::CGetType(convertedSSA);
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
            IRM->ReplaceIRNode(useStmt, ssa, convertedSSA);
            const auto* ga = GetPointer<const assign_stmt>(useStmt);
            if(ga)
            {
               // Unary and binary expression have already been lowered to function calls
               auto* te = GetPointer<ternary_node>(ga->op1);
               if(te)
               {
                  te->type = IRM->GetIRNode(convertedSSA_type->index);
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
         const auto ssa = IRM->GetIRNode(SSA->index);
         const auto& [fformat, useStmts] = if_info;

         auto defStmt = SSA->GetDefStmt();
         const auto gn = GetPointerS<node_stmt>(defStmt);
         THROW_ASSERT(sl->list_of_bloc.count(gn->bb_index),
                      "BB" + STR(gn->bb_index) + " not present in current function.");
         auto bb = sl->list_of_bloc.at(gn->bb_index);
         if(gn->get_kind() == nop_stmt_K)
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
            IRM->ReplaceIRNode(stmt, ssa, convertedSSA);
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

bool soft_float_cg_ext::lowering_needed(const ssa_node* ssa)
{
   if(ir_helper::IsPointerType(ssa->type))
   {
      return ir_helper::IsRealType(ir_helper::CGetPointedType(ssa->type));
   }
   return ir_helper::IsRealType(ssa->type);
}

ir_nodeRef soft_float_cg_ext::int_type_for(const ir_nodeRef& type, bool use_internal) const
{
   if(ir_helper::IsPointerType(type))
   {
      return ir_helper::Size(ir_helper::CGetPointedType(type)) == 32 ? float32_ptr_type : float64_ptr_type;
   }
   if(!use_internal || _version->ieee_format())
   {
      return ir_helper::Size(type) == 32 ? float32_type : float64_type;
   }
   else
   {
      THROW_ASSERT(int_type, "Internal integer type should have been defined before.");
      return int_type;
   }
}

bool soft_float_cg_ext::signature_lowering(function_val_node* f_decl) const
{
#ifndef NDEBUG
   const auto f_name = ir_helper::PrintType(
       IRM->GetIRNode(f_decl->index), true, false, ir_nodeRef(),
       std::make_unique<std_var_pp_functor>(AppM->CGetFunctionBehavior(f_decl->index)->CGetBehavioralHelper()));
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing function signature " + f_name);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   bool changed_parm = false, changed_type = false;
   THROW_ASSERT(ir_helper::IsFunctionType(f_decl->type) || ir_helper::IsFunctionPointerType(f_decl->type),
                "unexpected pattern");
   const auto is_orig_ftype_ptr = ir_helper::IsPointerType(f_decl->type);
   const auto orig_ftype = is_orig_ftype_ptr ? GetPointerS<const pointer_ty_node>(f_decl->type)->ptd : f_decl->type;
   // IR node decoupling is necessary when directly modifying a type node
   CustomUnorderedMapStable<unsigned int, unsigned int> remapping;
   const auto dup_ft = ir_node_dup(remapping, AppM).create_ir_node(orig_ftype, ir_node_dup_mode::FUNCTION);
   const auto f_type = IRM->GetIRNode(dup_ft);

   auto* ft = GetPointerS<function_ty_node>(f_type);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing return type " + ft->retn->ToString());
   if(ir_helper::IsRealType(ft->retn))
   {
      const auto int_ret = int_type_for(ft->retn, _version->internal);
      const auto ret_type = GetPointerS<const type_node>(int_ret);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Return type lowered to " + ret_type->ToString() + " " + STR(ir_helper::Size(int_ret)));
      ft->retn = int_ret;
      ft->algn = ret_type->algn;
      ft->bitsizealloc = ret_type->bitsizealloc;
      changed_type = true;
   }
   auto args_type_it = ft->list_of_args_type.begin();
   THROW_ASSERT(args_type_it != ft->list_of_args_type.end() || f_decl->list_of_args.empty(), "unexpected pattern");
   for(const auto& arg : f_decl->list_of_args)
   {
      const auto pd = GetPointerS<argument_val_node>(arg);
      const auto& parm_type = pd->type;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Analysing parameter " + pd->ToString() + " of type " + STR(parm_type));
      if((ir_helper::IsPointerType(parm_type) && ir_helper::IsRealType(ir_helper::CGetPointedType(parm_type))) ||
         ir_helper::IsRealType(parm_type))
      {
         const auto int_parm_type = int_type_for(parm_type, _version->internal);
         const auto parm_int_type = GetPointerS<const type_node>(int_parm_type);
         pd->algn = parm_int_type->algn;
         pd->type = int_parm_type;
         *args_type_it = int_parm_type;
         changed_type = true;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Parameter type lowered to " + STR(int_parm_type) + " " +
                            STR(ir_helper::Size(int_parm_type)));
         changed_parm = true;
      }
      ++args_type_it;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   if(changed_type)
   {
      // Replace function type reference when modifications have been applied
      f_decl->type = is_orig_ftype_ptr ? ir_man->GetPointerType(f_type) : f_type;
   }
   return changed_parm || changed_type;
}

void soft_float_cg_ext::ssa_lowering(ssa_node* ssa, bool internal_type) const
{
   THROW_ASSERT(lowering_needed(ssa), "Unexpected ssa type - " + ssa->ToString() + " " + STR(ssa->type));
   const auto vc_type = int_type_for(ssa->type, internal_type);
   THROW_ASSERT(vc_type, "");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Lowering " + ssa->ToString() + " type to " + STR(vc_type));

   const auto defStmt = ssa->GetDefStmt();
   const auto def = GetPointer<assign_stmt>(defStmt);
   if(def)
   {
      const auto ue = GetPointer<unary_node>(def->op1);
      if(ue)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Definition statement before - " + def->ToString());
         if(ue->get_kind() == bitcast_node_K)
         {
            const auto nop = ir_man->create_unary_operation(vc_type, ue->op, BUILTIN_LOCINFO, nop_node_K);
            IRM->ReplaceIRNode(defStmt, def->op1, nop);
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
   if(ssa->var && ssa->var->get_kind() != argument_val_node_K)
   {
      const auto vd = GetPointer<variable_val_node>(ssa->var);
      THROW_ASSERT(vd, "SSA name associated variable is espected to be a variable declaration " +
                           ssa->var->get_kind_text() + " " + ssa->var->ToString());
      if(vd->type->index != ssa->type->index)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Variable declaration before - " + vd->ToString() + " " + vd->type->ToString());
         const auto var_int_type = GetPointerS<type_node>(vc_type);
         vd->algn = var_int_type->algn;
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
      const auto* ga = GetPointer<const assign_stmt>(use_count.first);
      if(ga)
      {
         // Unary and binary expression have already been lowered to function calls
         auto* te = GetPointer<ternary_node>(ga->op1);
         if(te)
         {
            te->type = vc_type;
         }
      }
   }
}

ir_nodeRef soft_float_cg_ext::cstCast(uint64_t bits, const FloatFormatRef& inFF, const FloatFormatRef& outFF) const
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

   return IRM->CreateUniqueIntegerCst(
       static_cast<int64_t>(out_val),
       ir_man->GetCustomIntegerType(static_cast<unsigned int>(static_cast<uint8_t>(outFF->sign == bit_lattice::U) +
                                                              outFF->exp_bits + outFF->frac_bits),
                                    true));
}

#define FLOAT_CAST_FU_NAME "__float_cast"

ir_nodeRef soft_float_cg_ext::generate_interface(const blocRef& bb, ir_nodeRef stmt, const ir_nodeRef& ssa,
                                                 const FloatFormatRef inFF, const FloatFormatRef outFF) const
{
   static const auto default_bool_type = ir_man->GetBooleanType();
   static const auto default_int_type = ir_man->GetSignedIntegerType();

#if HAVE_ASSERTS
   const auto t_kind = ir_helper::CGetType(ssa)->get_kind();
#endif
   THROW_ASSERT(t_kind == integer_ty_node_K,
                "Cast rename should be applied on integer variables only. " + ir_node::GetString(t_kind));
   THROW_ASSERT(inFF, "Interface input float format must be defined.");
   THROW_ASSERT(outFF, "Interface output float format must be defined.");

   const auto float_cast = IRM->GetFunction(FLOAT_CAST_FU_NAME);
   THROW_ASSERT(float_cast, "The library miss this function " FLOAT_CAST_FU_NAME);
   const auto spec_suffix = inFF->ToString() + "_to_" + outFF->ToString();
   auto spec_function = IRM->GetFunction(FLOAT_CAST_FU_NAME + spec_suffix);
   if(!spec_function)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Generating specialized version of " FLOAT_CAST_FU_NAME " (" + STR(float_cast->index) +
                         ") with fp format " + spec_suffix);
      spec_function = ir_man->CloneFunction(float_cast, spec_suffix);
      THROW_ASSERT(spec_function, "Error cloning function " FLOAT_CAST_FU_NAME " (" + STR(float_cast->index) + ").");
   }
   const std::vector<ir_nodeRef> args = {
       ssa,
       IRM->CreateUniqueIntegerCst(static_cast<long long>(inFF->exp_bits), default_int_type),
       IRM->CreateUniqueIntegerCst(static_cast<long long>(inFF->frac_bits), default_int_type),
       IRM->CreateUniqueIntegerCst(static_cast<long long>(inFF->exp_bias), default_int_type),
       IRM->CreateUniqueIntegerCst(static_cast<long long>(inFF->rounding_mode), default_int_type),
       IRM->CreateUniqueIntegerCst(static_cast<long long>(inFF->exception_mode), default_int_type),
       IRM->CreateUniqueIntegerCst(static_cast<long long>(inFF->has_one), default_bool_type),
       IRM->CreateUniqueIntegerCst(static_cast<long long>(inFF->has_subnorm), default_bool_type),
       IRM->CreateUniqueIntegerCst(
           static_cast<long long>(inFF->sign == bit_lattice::U ? -1 : (inFF->sign == bit_lattice::ONE ? 1 : 0)),
           default_int_type),
       IRM->CreateUniqueIntegerCst(static_cast<long long>(outFF->exp_bits), default_int_type),
       IRM->CreateUniqueIntegerCst(static_cast<long long>(outFF->frac_bits), default_int_type),
       IRM->CreateUniqueIntegerCst(static_cast<long long>(outFF->exp_bias), default_int_type),
       IRM->CreateUniqueIntegerCst(static_cast<long long>(outFF->rounding_mode), default_int_type),
       IRM->CreateUniqueIntegerCst(static_cast<long long>(outFF->exception_mode), default_int_type),
       IRM->CreateUniqueIntegerCst(static_cast<long long>(outFF->has_one), default_bool_type),
       IRM->CreateUniqueIntegerCst(static_cast<long long>(outFF->has_subnorm), default_bool_type),
       IRM->CreateUniqueIntegerCst(
           static_cast<long long>(outFF->sign == bit_lattice::U ? -1 : (outFF->sign == bit_lattice::ONE ? 1 : 0)),
           default_int_type),
   };
   const auto float_cast_call = ir_man->CreateCallExpr(spec_function, args, BUILTIN_LOCINFO);
   const auto ret_type = ir_helper::GetFunctionReturnType(spec_function);
   const auto cast_stmt = ir_man->CreateAssignStmt(ret_type, ir_nodeConstRef(), ir_nodeConstRef(), float_cast_call,
                                                   function_id, BUILTIN_LOCINFO);
   if(stmt == nullptr)
   {
      bb->PushFront(cast_stmt, AppM);
   }
   else
   {
      bb->PushAfter(cast_stmt, stmt, AppM);
   }
   auto out_var = GetPointer<const assign_stmt>(cast_stmt)->op0;
   const auto out_type =
       ir_man->GetCustomIntegerType(static_cast<unsigned int>(static_cast<uint8_t>(outFF->sign == bit_lattice::U) +
                                                              outFF->exp_bits + outFF->frac_bits),
                                    true);
   if(!ir_helper::IsSameType(ret_type, out_type))
   {
      const auto nop_stmt = ir_man->CreateNopExpr(out_var, out_type, ir_nodeConstRef(), ir_nodeConstRef(), function_id);
      out_var = GetPointer<const assign_stmt>(nop_stmt)->op0;
      bb->PushAfter(nop_stmt, cast_stmt, AppM);
   }
   if(inline_conversion)
   {
      FunctionCallOpt::RequestCallOpt(cast_stmt, function_id, FunctionOptType::INLINE);
   }

   // Update functions float format map
   const auto called_func_vertex = AppM->CGetCallGraphManager().GetVertex(spec_function->index);
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

ir_nodeRef soft_float_cg_ext::floatNegate(const ir_nodeRef& op, const FloatFormatRef& ff) const
{
   if(ff->sign == bit_lattice::U)
   {
      const auto int_ff_type = ir_man->GetCustomIntegerType(1U + ff->exp_bits + ff->frac_bits, true);
      return ir_man->create_binary_operation(
          int_ff_type, op, IRM->CreateUniqueIntegerCst(1LL << (ff->exp_bits + ff->frac_bits), int_ff_type),
          BUILTIN_LOCINFO, xor_node_K);
   }
   else
   {
      THROW_ERROR("Negate operation on fixed sign type will flatten all values.");
      return nullptr;
   }
}

ir_nodeRef soft_float_cg_ext::floatAbs(const ir_nodeRef& op, const FloatFormatRef& ff) const
{
   if(ff->sign == bit_lattice::U)
   {
      const auto int_ff_type = ir_man->GetCustomIntegerType(1U + ff->exp_bits + ff->frac_bits, true);
      return ir_man->create_binary_operation(
          int_ff_type, op, IRM->CreateUniqueIntegerCst((1LL << (ff->exp_bits + ff->frac_bits)) - 1, int_ff_type),
          BUILTIN_LOCINFO, and_node_K);
   }
   if(ff->sign == bit_lattice::ONE)
   {
      // Fixed positive sign representation is always positive already
      return op;
   }
   THROW_ERROR("Negate operation on fixed negative sign type will flatten all values.");
   return nullptr;
}

ir_nodeRef soft_float_cg_ext::replaceWithCall(const FloatFormatRef& specFF, const std::string& fu_name,
                                              std::vector<ir_nodeRef> args, const ir_nodeRef& current_statement,
                                              const ir_nodeRef& curr_tn, const std::string& current_locinfo)
{
   THROW_ASSERT(specFF, "FP format specialization missing");

   auto called_function = IRM->GetFunction(fu_name);
   if(!called_function)
   {
      THROW_ERROR("undefined symbol: " + fu_name);
   }
   const auto spec_suffix = specFF->ToString();
   auto spec_function = IRM->GetFunction(fu_name + spec_suffix);
   if(!spec_function)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Generating specialized version of " + fu_name + " (" + STR(called_function->index) +
                         ") with fp format " + spec_suffix);
      spec_function = ir_man->CloneFunction(called_function, spec_suffix);
      THROW_ASSERT(spec_function, "Error cloning function " + fu_name + " (" + STR(called_function->index) + ").");
      THROW_ASSERT(ir_helper::IsFunctionImplemented(spec_function), "Cloned function " + fu_name + " has no body");
   }
   auto& spec_parms = spec_parms_map[spec_suffix];
   if(!spec_parms[0])
   {
      const auto default_bool_type = ir_man->GetBooleanType();
      const auto default_int_type = ir_man->GetSignedIntegerType();

      spec_parms[0] = IRM->CreateUniqueIntegerCst(static_cast<long long>(specFF->exp_bits), default_int_type);
      spec_parms[1] = IRM->CreateUniqueIntegerCst(static_cast<long long>(specFF->frac_bits), default_int_type);
      spec_parms[2] = IRM->CreateUniqueIntegerCst(static_cast<long long>(specFF->exp_bias), default_int_type);
      spec_parms[3] = IRM->CreateUniqueIntegerCst(static_cast<long long>(specFF->rounding_mode), default_int_type);
      spec_parms[4] = IRM->CreateUniqueIntegerCst(static_cast<long long>(specFF->exception_mode), default_int_type);
      spec_parms[5] = IRM->CreateUniqueIntegerCst(static_cast<long long>(specFF->has_one), default_bool_type);
      spec_parms[6] = IRM->CreateUniqueIntegerCst(static_cast<long long>(specFF->has_subnorm), default_bool_type);
      spec_parms[7] = IRM->CreateUniqueIntegerCst(
          static_cast<long long>(specFF->sign == bit_lattice::U ? -1 : (specFF->sign == bit_lattice::ONE ? 1 : 0)),
          default_int_type);
   }
   std::copy(spec_parms.begin(), spec_parms.end(), std::back_inserter(args));
   called_function = spec_function;
   const auto stmt = GetPointer<node_stmt>(current_statement);
   if(stmt && !stmt->predicate)
   {
      stmt->predicate = IRM->CreateUniqueIntegerCst(1, ir_man->GetBooleanType());
   }
   IRM->ReplaceIRNode(current_statement, curr_tn, ir_man->CreateCallExpr(called_function, args, current_locinfo));
   CallGraphManager::addCallPointAndExpand(already_visited, AppM, function_id, called_function->index,
                                           current_statement->index, FunctionEdgeInfo::CallType::direct_call,
                                           DEBUG_LEVEL_NONE);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Added call point for " + STR(called_function->index));

   // Update functions float format map
   const auto called_func_vertex = AppM->CGetCallGraphManager().GetVertex(called_function->index);
   const auto calledFF = FunctionVersionRef(new FunctionVersion(called_func_vertex, specFF));
#if HAVE_ASSERTS
   const auto res =
#endif
       funcFF.insert(std::make_pair(called_func_vertex, calledFF));
   THROW_ASSERT(res.second || res.first->second->compare(*calledFF, true) == 0,
                "Same function registered with different formats: " + res.first->second->ToString() + " and " +
                    calledFF->ToString() + " (" + fu_name + ")");
   return called_function;
}

bool soft_float_cg_ext::RecursiveExaminate(const ir_nodeRef& current_statement, const ir_nodeRef& curr_tn,
                                           int type_interface)
{
   THROW_ASSERT((type_interface & 3) == INTERFACE_TYPE_NONE || (type_interface & 3) == INTERFACE_TYPE_INPUT ||
                    (type_interface & 3) == INTERFACE_TYPE_OUTPUT,
                "Required interface type must be unique (" + STR(type_interface) + ")");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Update recursively (ti=" + STR(type_interface) + ") (" + STR(curr_tn->index) + " " +
                      curr_tn->get_kind_text() + ") " + STR(curr_tn));
   const auto current_locinfo = [curr_tn]() -> std::string {
      const auto loc_info_tn = GetPointer<const IR_LocInfo>(curr_tn);
      if(loc_info_tn)
      {
         return loc_info_tn->include_name + ":" + STR(loc_info_tn->line_number) + ":" + STR(loc_info_tn->column_number);
      }
      return "";
   }();
   bool modified = false;
   const auto is_internal_call = [&](const ir_nodeRef& fn) -> bool {
      static const auto mcpy_node = IRM->GetFunction(MEMCPY);
      static const auto mset_node = IRM->GetFunction(MEMSET);
      const auto fn_fd = GetPointerS<function_val_node>(fn);
      if(!fn_fd->body)
      {
         if(!_version->ieee_format() && ir_helper::GetFunctionName(fn) == BUILTIN_WAIT_CALL)
         {
            THROW_UNREACHABLE("Function pointers not supported from user defined floating point format functions");
            // TODO: maybe it could be possible to only warn the user here to be careful about the pointed function
            // definition and go on
         }
         return _version->ieee_format();
      }
      const auto fn_v = AppM->CGetCallGraphManager().GetVertex(fn->index);
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
   const auto ExaminateFunctionCall = [&](ir_nodeRef fn) -> int {
      const auto ae = GetPointer<addr_node>(fn);
      if(ae)
      {
         fn = ae->op;
         const auto called_fd = GetPointerS<const function_val_node>(fn);
         ae->type = called_fd->type->get_kind() == pointer_ty_node_K ? called_fd->type :
                                                                       ir_man->GetPointerType(called_fd->type);
         modified = true;
      }
      if(ir_helper::GetFunctionName(fn) == BUILTIN_WAIT_CALL)
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
      // Hardware implemented functions need arguments to still be real_ty_node, thus it is necessary to add a bitcast
      // operation before
      if(!ir_helper::IsFunctionImplemented(fn))
      {
         type_i |= INTERFACE_TYPE_REAL;
      }
      return type_i;
   };
   switch(curr_tn->get_kind())
   {
      case call_node_K:
      {
         const auto ce = GetPointerS<const call_node>(curr_tn);
         type_interface = ExaminateFunctionCall(ce->fn);
         for(const auto& arg : ce->args)
         {
            RecursiveExaminate(current_statement, arg, type_interface);
         }
         break;
      }
      case call_stmt_K:
      {
         const auto ce = GetPointerS<const call_stmt>(curr_tn);
         type_interface = ExaminateFunctionCall(ce->fn);
         for(const auto& arg : ce->args)
         {
            RecursiveExaminate(current_statement, arg, type_interface);
         }
         break;
      }
      case phi_stmt_K:
      {
         const auto gp = GetPointerS<const phi_stmt>(curr_tn);
         RecursiveExaminate(current_statement, gp->res, type_interface);
         for(const auto& de : gp->CGetDefEdgesList())
         {
            RecursiveExaminate(current_statement, de.first, type_interface);
         }
         break;
      }
      case assign_stmt_K:
      {
         const auto ga = GetPointerS<const assign_stmt>(curr_tn);
         const auto rhs_type = ga->op1->get_kind();
         if(rhs_type == call_node_K)
         {
            const auto ce = GetPointerS<const call_node>(ga->op1);
            const auto fn = GetPointer<const addr_node>(ce->fn) ? GetPointerS<const addr_node>(ce->fn)->op : ce->fn;
            const auto fname = ir_helper::GetFunctionName(fn);
            bool is_f32 = false;
            const auto fname_id = strip_fname(fname, &is_f32);
            if(truefloat_funcs.count(fname))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Replacing TrueFloat API call with specialized version");
               THROW_ASSERT(ce->args.size() >= 8, "unexpected condition");
               AppM->GetCallGraphManager().RemoveCallPoint(function_id, fn->index, current_statement->index);
               const auto specFF = FloatFormat::FromArgs(ce->args);
               const auto tf_name = fname == "__float_divSRT4" && function_behavior->is_function_pipelined() ?
                                        "__float_divSRT4U" :
                                        fname;
               replaceWithCall(specFF, tf_name, std::vector<ir_nodeRef>(ce->args.begin(), ce->args.end() - 8),
                               current_statement, ga->op1, current_locinfo);
               RecursiveExaminate(current_statement, ga->op0, INTERFACE_TYPE_NONE);
               modified = true;
            }
            else if(spec_libm_funcs.count(fname_id))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Replacing libm call with templatized version");
               // libm function calls may be replaced with their templatized version if available, avoiding conversion
               AppM->GetCallGraphManager().RemoveCallPoint(function_id, fn->index, current_statement->index);
               is_f32 |= !ce->args.empty() && ir_helper::Size(ce->args.front()) == 32;
               const auto specFF = _version->ieee_format() ? (is_f32 ? float32FF : float64FF) : _version->userRequired;
               replaceWithCall(specFF, TF_MATH_PREFIX + fname_id, ce->args, current_statement, ga->op1,
                               current_locinfo);
               RecursiveExaminate(current_statement, ga->op0, INTERFACE_TYPE_NONE);
               if(spec_libm_funcs_inlined.count(fname_id))
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

                  // Hardware implemented functions need the return value to still be real_ty_node, thus it is necessary
                  // to add a bitcast operation after
                  if(fname != BUILTIN_WAIT_CALL && !ir_helper::IsFunctionImplemented(fn))
                  {
                     ti |= INTERFACE_TYPE_REAL;
                  }
                  return ti;
               }();
               RecursiveExaminate(current_statement, ga->op0, type_i);
            }
         }
         else if(ir_helper::IsLoad(current_statement, function_behavior->get_function_mem()))
         {
            // Values loaded from memory need to be cast renamed to local float format
            RecursiveExaminate(current_statement, ga->op0, INTERFACE_TYPE_INPUT);
         }
         else
         {
            RecursiveExaminate(current_statement, ga->op0, type_interface);
         }
         if(ir_helper::IsStore(current_statement, function_behavior->get_function_mem()))
         {
            // Values stored to memory need to be cast renamed before the store statement
            RecursiveExaminate(current_statement, ga->op1, INTERFACE_TYPE_OUTPUT);
         }
         else
         {
            RecursiveExaminate(current_statement, ga->op1, type_interface);
         }
         break;
      }
      case nop_stmt_K:
      {
         break;
      }
      case variable_val_node_K:
      case argument_val_node_K:
      {
         break;
      }
      case ssa_node_K:
      {
         const auto SSA = GetPointerS<ssa_node>(curr_tn);
         if(lowering_needed(SSA))
         {
            // Real variables must all be converted to unsigned integers after softfloat lowering operations
            bitcastCandidates.insert({SSA, type_interface == INTERFACE_TYPE_NONE});
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Lowering required for current variable");

            if(type_interface & INTERFACE_TYPE_REAL)
            {
               if(SSA->GetDefStmt()->index == current_statement->index)
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
            // If ssa_node references a argument_val_node and is defined by a nop_stmt, it represents the formal
            // function parameter inside the function body
            if(SSA->var != nullptr && SSA->var->get_kind() == argument_val_node_K &&
               SSA->GetDefStmt()->get_kind() == nop_stmt_K)
            {
               auto argIt = std::find_if(args.begin(), args.end(),
                                         [&](const ir_nodeRef& arg) { return arg->index == SSA->var->index; });
               THROW_ASSERT(argIt != args.end(),
                            "argument_val_node associated with ssa_node not found in function parameters");
               const auto arg_pos = static_cast<size_t>(argIt - args.begin());
               THROW_ASSERT(arg_pos < args.size(), "Computed parameter position outside actual parameters number");
               paramBinding[arg_pos] = curr_tn;
               bindingCompleted = std::find(paramBinding.begin(), paramBinding.end(), nullptr) == paramBinding.end();
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Variable " + SSA->ToString() + " is defined from parameter " + STR(arg_pos));
            }
         }

         if(!_version->ieee_format() && ir_helper::IsRealType(SSA->type))
         {
            const auto ssa_ff = ir_helper::Size(SSA->type) == 32 ? float32FF : float64FF;
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
                     outputInterface.insert({SSA, {ssa_ff, std::vector<ir_nodeRef>({current_statement})}});
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
      case CASE_UNARY_NODES:
      {
         const auto* ue = GetPointerS<const unary_node>(curr_tn);
         const auto& expr_type = ue->type;
         const auto op_expr_type = ir_helper::CGetType(ue->op);
         // Propagate recursion with INTERFACE_TYPE_NONE to avoid cast rename of internal variables (input parameters
         // and constant will be converted anyway)
         RecursiveExaminate(current_statement, ue->op, INTERFACE_TYPE_NONE);
         if(ir_helper::IsRealType(expr_type))
         {
            switch(curr_tn->get_kind())
            {
               case itofp_node_K:
               {
                  const auto bitsize_in = std::max(32ULL, ceil_pow2(ir_helper::Size(op_expr_type)));
                  const auto bitsize_out = std::max(32ULL, ceil_pow2(ir_helper::Size(expr_type)));
                  const auto outFF = bitsize_out == 64 ? float64FF : float32FF;
                  if(!ir_helper::IsRealType(op_expr_type))
                  {
                     const auto is_unsigned = ir_helper::IsUnsignedIntegerType(op_expr_type);
                     const auto fu_name =
                         "__" + std::string(is_unsigned ? "u" : "") + "int" + std::to_string(bitsize_in) + "_to_float";
                     THROW_ASSERT(!_version->ieee_format() || outFF, "");
                     replaceWithCall(_version->ieee_format() ? outFF : _version->userRequired, fu_name, {ue->op},
                                     current_statement, curr_tn, current_locinfo);
                     if(inline_conversion)
                     {
                        FunctionCallOpt::RequestCallOpt(current_statement, function_id, FunctionOptType::INLINE);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Call inlining required");
                     }
                     modified = true;
                  }
                  break;
               }
               case bitcast_node_K:
               {
                  // BEWARE: this bitcast is from integer to real type, thus it will be a def statement of a real
                  // type ssa_node
                  //          def statements of real type ssa_node variables will be correctly replaced in the next
                  //          phase of this step
                  break;
               }
               case abs_node_K:
               {
                  const auto float_size = ir_helper::Size(op_expr_type);
                  THROW_ASSERT(float_size == 32 || float_size == 64,
                               "Unhandled floating point format (size = " + STR(float_size) + ")");
                  const auto ff =
                      _version->ieee_format() ? (float_size == 32 ? float32FF : float64FF) : _version->userRequired;
                  THROW_ASSERT(ff, "Float format should be defined here");
                  const auto float_negate = floatAbs(ue->op, ff);
                  IRM->ReplaceIRNode(current_statement, curr_tn, float_negate);
                  modified = true;
                  break;
               }
               case neg_node_K:
               {
                  const auto float_size = ir_helper::Size(op_expr_type);
                  THROW_ASSERT(float_size == 32 || float_size == 64,
                               "Unhandled floating point format (size = " + STR(float_size) + ")");
                  const auto ff =
                      _version->ieee_format() ? (float_size == 32 ? float32FF : float64FF) : _version->userRequired;
                  THROW_ASSERT(ff, "Float format should be defined here");
                  const auto float_negate = floatNegate(ue->op, ff);
                  IRM->ReplaceIRNode(current_statement, curr_tn, float_negate);
                  modified = true;
                  break;
               }
               case nop_node_K:
               {
                  if(_version->ieee_format())
                  {
                     auto bitsize_in = ir_helper::Size(op_expr_type);
                     auto bitsize_out = ir_helper::Size(expr_type);
                     THROW_ASSERT(bitsize_in == 32 || bitsize_in == 64,
                                  "Unhandled input floating point format (size = " + STR(bitsize_in) + ")");
                     THROW_ASSERT(bitsize_out == 32 || bitsize_out == 64,
                                  "Unhandled output floating point format (size = " + STR(bitsize_out) + ")");
                     if(ir_helper::IsRealType(op_expr_type))
                     {
                        const auto fu_name = "__float" + STR(bitsize_in) + "_to_float" + STR(bitsize_out) + "_ieee";
                        const auto inFF = bitsize_in == 32 ? float32FF : float64FF;
                        const auto called_function = IRM->GetFunction(fu_name);
                        THROW_ASSERT(called_function, "The library miss this function " + fu_name);
                        std::vector<ir_nodeRef> args = {
                            ue->op,
                            IRM->CreateUniqueIntegerCst(static_cast<long long>(inFF->exception_mode),
                                                        ir_man->GetSignedIntegerType()),
                            IRM->CreateUniqueIntegerCst(inFF->has_subnorm, ir_man->GetBooleanType())};
                        IRM->ReplaceIRNode(current_statement, curr_tn,
                                           ir_man->CreateCallExpr(called_function, args, current_locinfo));
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
                            ir_man->create_unary_operation(expr_type, ue->op, current_locinfo, bitcast_node_K);
                        IRM->ReplaceIRNode(current_statement, curr_tn, vc_expr);
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
               case mem_access_node_K:
               {
                  const auto mr = GetPointerS<mem_access_node>(curr_tn);
                  mr->type = ir_helper::Size(mr->type) == 32 ? float32_type : float64_type;
                  break;
               }
               case addr_node_K:
               case not_node_K:
               case fptoi_node_K:
               case unaligned_mem_access_node_K:
               case call_node_K:
               case constructor_node_K:
               case identifier_node_K:
               case ssa_node_K:
               case statement_list_node_K:
               case lut_node_K:
               case CASE_BINARY_NODES:
               case CASE_CST_NODES:
               case CASE_DECL_NODES:
               case CASE_FAKE_NODES:
               case CASE_NODE_STMTS:
               case CASE_TERNARY_NODES:
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
         if(ir_helper::IsRealType(op_expr_type))
         {
            switch(curr_tn->get_kind())
            {
               case fptoi_node_K:
               {
                  const auto bitsize_in = ir_helper::Size(op_expr_type);
                  const auto inFF = bitsize_in == 32 ? float32FF : (bitsize_in == 64 ? float64FF : nullptr);
                  auto bitsize_out = ir_helper::Size(expr_type);
                  if(bitsize_out < 32)
                  {
                     bitsize_out = 32;
                  }
                  else if(bitsize_out > 32 && bitsize_out < 64)
                  {
                     bitsize_out = 64;
                  }
                  const auto is_unsigned = ir_helper::IsUnsignedIntegerType(expr_type);
                  const auto fu_name =
                      std::string(is_unsigned ? "__float_to_uint" : "__float_to_int") + STR(bitsize_out);
                  replaceWithCall(_version->ieee_format() ? inFF : _version->userRequired, fu_name, {ue->op},
                                  current_statement, curr_tn, current_locinfo);
                  if(inline_conversion)
                  {
                     FunctionCallOpt::RequestCallOpt(current_statement, function_id, FunctionOptType::INLINE);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Call inlining required");
                  }
                  modified = true;
                  break;
               }
               case bitcast_node_K:
               {
                  // This bitcast is from real to integer type variable, thus needs to be stored in the type
                  // conversion set of statements to be converted later if necessary
                  nopConvertibleBitcasts.push_back(current_statement);
                  break;
               }
               case mem_access_node_K:
               case call_node_K:
               case constructor_node_K:
               case identifier_node_K:
               case ssa_node_K:
               case statement_list_node_K:
               case lut_node_K:
               case CASE_BINARY_NODES:
               case CASE_CST_NODES:
               case CASE_DECL_NODES:
               case CASE_FAKE_NODES:
               case CASE_NODE_STMTS:
               case CASE_TERNARY_NODES:
               case CASE_TYPE_NODES:
               case abs_node_K:
               case addr_node_K:
               case not_node_K:
               case itofp_node_K:
               case unaligned_mem_access_node_K:
               case neg_node_K:
               case nop_node_K:
                  break;
               default:
               {
                  THROW_UNREACHABLE("");
               }
            }
         }
         else if(curr_tn->get_kind() == mem_access_node_K && ir_helper::IsPointerType(op_expr_type) &&
                 ir_helper::IsRealType(ir_helper::CGetPointedType(op_expr_type)))
         {
            const auto mr = GetPointerS<mem_access_node>(curr_tn);
            mr->type = int_type_for(mr->type, true);
         }
         break;
      }
      case CASE_BINARY_NODES:
      {
         const auto be = GetPointerS<binary_node>(curr_tn);
         // Get operand type before recursive examination because floating point operands may be converted to unsigned
         // integer during during recursion
         const auto expr_type = ir_helper::CGetType(be->op0);
         // Propagate recursion with INTERFACE_TYPE_NONE to avoid cast rename of internal variables (input parameters
         // and constant will be converted anyway)
         RecursiveExaminate(current_statement, be->op0, INTERFACE_TYPE_NONE);
         RecursiveExaminate(current_statement, be->op1, INTERFACE_TYPE_NONE);
         if(ir_helper::IsRealType(expr_type))
         {
            bool add_call = true;
            std::string fu_name;
            switch(curr_tn->get_kind())
            {
               case mul_node_K:
               {
                  fu_name = "__float_mul";
                  break;
               }
               case add_node_K:
               {
                  fu_name = "__float_add";
                  break;
               }
               case sub_node_K:
               {
                  fu_name = "__float_sub";
                  break;
               }
               case fdiv_node_K:
               {
                  fu_name = "__float_div";
                  const auto bitsize = ir_helper::Size(expr_type);
                  if(bitsize == 32 || bitsize == 64)
                  {
                     const auto fpdiv_mode = parameters->getOption<std::string>(OPT_hls_fpdiv);
                     if(fpdiv_mode != "SF")
                     {
                        fu_name += fpdiv_mode;
                     }
                     if(fpdiv_mode == "SRT4" && function_behavior->is_function_pipelined())
                     {
                        fu_name += "U";
                     }
                  }
                  break;
               }
               case gt_node_K:
               {
                  fu_name = "__float_gt";
                  break;
               }
               case ge_node_K:
               {
                  fu_name = "__float_ge";
                  break;
               }
               case lt_node_K:
               {
                  fu_name = "__float_lt";
                  break;
               }
               case le_node_K:
               {
                  fu_name = "__float_le";
                  break;
               }
               case eq_node_K:
               {
                  fu_name = "__float_eq";
                  break;
               }
               case ne_node_K:
               {
                  fu_name = "__float_ltgt_quiet";
                  break;
               }
               case and_node_K:
               case or_node_K:
               case xor_node_K:
               case shl_node_K:
               case lut_node_K:
               case max_node_K:
               case min_node_K:
               case gep_node_K:
               case shr_node_K:
               case idiv_node_K:
               case irem_node_K:
               case widen_mul_node_K:
               case extract_bit_node_K:
               case add_sat_node_K:
               case sub_sat_node_K:
               case extractvalue_node_K:
               case extractelement_node_K:
               {
                  add_call = false;
                  break;
               }
               case call_node_K:
               case constructor_node_K:
               case identifier_node_K:
               case ssa_node_K:
               case statement_list_node_K:
               case frem_node_K:
               case CASE_CST_NODES:
               case CASE_DECL_NODES:
               case CASE_FAKE_NODES:
               case CASE_NODE_STMTS:
               case CASE_TERNARY_NODES:
               case CASE_TYPE_NODES:
               case CASE_UNARY_NODES:
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
               const auto bitsize = ir_helper::Size(expr_type);
               const auto opFF = bitsize == 32 ? float32FF : (bitsize == 64 ? float64FF : nullptr);
               replaceWithCall(_version->ieee_format() ? opFF : _version->userRequired, fu_name, {be->op0, be->op1},
                               current_statement, curr_tn, current_locinfo);
               if(inline_math)
               {
                  FunctionCallOpt::RequestCallOpt(current_statement, function_id, FunctionOptType::INLINE);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Call inlining required");
               }
               modified = true;
            }
         }
         else if(ir_helper::IsPointerType(curr_tn) && ir_helper::IsRealType(ir_helper::CGetPointedType(be->type)))
         {
            if(curr_tn->get_kind() == gep_node_K)
            {
               const auto pp = GetPointerS<gep_node>(curr_tn);
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
      case CASE_TERNARY_NODES:
      {
         const auto te = GetPointerS<ternary_node>(curr_tn);
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
      case lut_node_K:
      {
         const auto le = GetPointerS<lut_node>(curr_tn);
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
      case constructor_node_K:
      {
         const auto co = GetPointerS<constructor_node>(curr_tn);
         for(const auto& idx_valu : co->list_of_idx_valu)
         {
            RecursiveExaminate(current_statement, idx_valu.second, type_interface);
         }
         break;
      }
      case multi_way_if_stmt_K:
      {
         const auto gmwi = GetPointerS<multi_way_if_stmt>(curr_tn);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               RecursiveExaminate(current_statement, cond.first, type_interface);
            }
         }
         break;
      }
      case return_stmt_K:
      {
         const auto re = GetPointerS<return_stmt>(curr_tn);
         if(re->op)
         {
            if(isTopFunction && re->op->get_kind() == ssa_node_K && lowering_needed(GetPointerS<ssa_node>(re->op)))
            {
               topReturn.push_back(current_statement);
            }
            RecursiveExaminate(current_statement, re->op,
                               _version->internal ? INTERFACE_TYPE_NONE : INTERFACE_TYPE_OUTPUT);
         }
         break;
      }
      case CASE_TYPE_NODES:
      {
         break;
      }
      case constant_fp_val_node_K:
      {
         if(~type_interface & INTERFACE_TYPE_REAL)
         {
            const auto cst = GetPointerS<constant_fp_val_node>(curr_tn);
            const auto bw = ir_helper::Size(curr_tn);
            const auto fp_str =
                (cst->valx.front() == '-' && cst->valr.front() != cst->valx.front()) ? ("-" + cst->valr) : cst->valr;
            const auto cst_val = convert_fp_to_bits(fp_str, bw);
            ir_nodeRef int_cst;
            if(type_interface == INTERFACE_TYPE_OUTPUT || _version->ieee_format())
            {
               int_cst =
                   IRM->CreateUniqueIntegerCst(static_cast<long long>(cst_val), ir_man->GetCustomIntegerType(bw, true));
            }
            else
            {
               const auto inFF = bw == 32 ? float32FF : float64FF;
               int_cst = cstCast(cst_val, inFF, _version->userRequired);
            }

            // Perform static constant value cast and replace real type constant with converted unsigned integer type
            // constant
            IRM->ReplaceIRNode(current_statement, curr_tn, int_cst);
            modified = true;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Real type constant " + curr_tn->ToString() + " converted to " + int_cst->ToString());
         }
         break;
      }
      case constant_int_val_node_K:
      //    {
      //       const auto cst_val = ir_helper::GetConstValue(curr_tn);
      //       if(ir_helper::IsPointerType(curr_tn))
      //       {
      //          const auto ptd_type = ir_helper::CGetPointedType(curr_tn);
      //          if(ir_helper::IsRealType(ptd_type))
      //          {
      //             const auto int_ptr_cst = IRM->CreateUniqueIntegerCst(cst_val, ir_helper::Size(ptd_type) == 32
      //             ? float32_ptr_type : float64_ptr_type); IRM->ReplaceIRNode(current_statement,
      //             curr_tn, int_ptr_cst); INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Real
      //             pointer type constant " + curr_tn->ToString() + " converted to " +
      //             int_ptr_cst->ToString());
      //          }
      //       }
      //       break;
      //    }
      case field_val_node_K:
      case function_val_node_K:
      case constant_vector_val_node_K:
         break;
      case CASE_FAKE_NODES:
      case identifier_node_K:
      case statement_list_node_K:
      case module_unit_node_K:
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
                  "<--Updated recursively (" + STR(curr_tn->index) + ") " + STR(curr_tn));
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
      case FPException_NoNan:
         ss << "n";
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
         case 'n':
            ff->exception_mode = FPException_NoNan;
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

FloatFormatRef FloatFormat::FromArgs(const std::vector<ir_nodeRef>& args)
{
   if(args.size() < 8)
   {
      THROW_ERROR("Expected at least 8 TrueFloat specialization arguments.");
   }
   auto spec_it = std::next(args.begin(), static_cast<ptrdiff_t>(args.size() - 8u));
   const auto get_next_constant = [&](bool is_signed = true) -> int {
      auto tn = range_analysis::castTraverse(*spec_it++);
      if(!ir_helper::IsConstant(tn))
      {
         const auto tn_def = GetPointerS<const ssa_node>(tn)->GetDefStmt();
         if(tn_def->get_kind() == assign_stmt_K)
         {
            tn = GetPointerS<const assign_stmt>(tn_def)->op1;
            if(ir_helper::IsConstant(tn))
            {
               return static_cast<int>(ir_helper::GetConstValue(tn, is_signed));
            }
         }
         THROW_ERROR("TrueFloat specialization parameter is not constant: " + tn->ToString());
      }
      return static_cast<int>(ir_helper::GetConstValue(tn, is_signed));
   };
   const auto e = get_next_constant(false);
   const auto m = get_next_constant(false);
   const auto b = get_next_constant();
   FloatFormatRef ff(new FloatFormat(static_cast<uint8_t>(e), static_cast<uint8_t>(m), static_cast<int32_t>(b)));
   ff->rounding_mode = static_cast<enum FPRounding>(get_next_constant(false));
   ff->exception_mode = static_cast<enum FPException>(get_next_constant(false));
   ff->has_one = get_next_constant(false) != 0;
   ff->has_subnorm = get_next_constant(false) != 0;
   switch(get_next_constant())
   {
      case 0:
         ff->sign = bit_lattice::ZERO;
         break;
      case 1:
         ff->sign = bit_lattice::ONE;
         break;
      default:
         ff->sign = bit_lattice::U;
         break;
   }
   return ff;
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
