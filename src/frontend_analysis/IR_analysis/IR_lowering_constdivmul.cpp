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
 * @file IR_lowering_constdivmul.cpp
 * @brief Constant division/multiplication lowering support for IR_lowering.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#include "IR_lowering.hpp"

#include "Discrepancy.hpp"
#include "Parameter.hpp"
#include "application_manager.hpp"
#include "area_info.hpp"
#include "constdiv_magic.hpp"
#include "custom_map.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_step.hpp"
#include "exceptions.hpp"
#include "frontend_flow_step.hpp"
#include "function_behavior.hpp"
#include "function_frontend_flow_step.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "ir_basic_block.hpp"
#include "ir_common.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "kcm_constmul.hpp"
#include "math_function.hpp"
#include "module_allocation/area_estimation.hpp"
#include "refcount.hpp"
#include "string_manipulation.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "time_info.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
#define FMT_HEADER_ONLY 1
#define PHMAP_BIDIRECTIONAL 0
#define MCDBGQ_NOLOCKFREE_FREELIST 0
#define MCDBGQ_TRACKMEM 0
#define MCDBGQ_NOLOCKFREE_IMPLICITPRODBLOCKINDEX 0
#define MCDBGQ_NOLOCKFREE_IMPLICITPRODHASH 0
#define MCDBGQ_USEDEBUGFREELIST 0
#define DISABLE_NAUTY

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operations.hpp>
#include <kitty/print.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/collapse_mapped.hpp>
#include <mockturtle/algorithms/lut_mapping.hpp>
#include <mockturtle/algorithms/node_resynthesis.hpp>
#include <mockturtle/algorithms/node_resynthesis/dsd.hpp>
#include <mockturtle/algorithms/node_resynthesis/shannon.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <mockturtle/views/mapping_view.hpp>
#include <mockturtle/views/topo_view.hpp>
#pragma GCC diagnostic pop

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "config_HAVE_ASSERTS.hpp"
#include "config_HAVE_FROM_DISCREPANCY_BUILT.hpp"

double IR_lowering::getConstMultDivScore(double latency, double area) const
{
   if(constmultdiv_score == "area")
   {
      return area;
   }
   if(constmultdiv_score == "delay")
   {
      return latency;
   }
   if(constmultdiv_score == "atp")
   {
      return latency * area;
   }
   return constmultdiv_w_latency * latency + constmultdiv_w_area * area;
}

std::string IR_lowering::getEffectiveConstMultDivDecisionMetric() const
{
   if(constmultdiv_decision_metric != "auto")
   {
      return constmultdiv_decision_metric;
   }

   if(constmultdiv_score == "area")
   {
      return "proxy";
   }
   if(constmultdiv_score == "delay")
   {
      return "score";
   }
   if(constmultdiv_score == "atp")
   {
      return "hybrid";
   }
   if(constmultdiv_score == "weighted")
   {
      const double eps = 1e-9;
      if(constmultdiv_w_latency > constmultdiv_w_area + eps)
      {
         return "score";
      }
      return "proxy";
   }
   return "score";
}

static bool is_proxy_decision_metric(const std::string& decision_metric)
{
   return decision_metric == "proxy";
}

static bool is_hybrid_decision_metric(const std::string& decision_metric)
{
   return decision_metric == "hybrid";
}

static bool better_multdiv_candidate(const std::string& decision_metric, double cand_score, double cand_proxy,
                                     double best_score, double best_proxy)
{
   const double eps = 1e-9;
   if(is_proxy_decision_metric(decision_metric))
   {
      if(cand_proxy + eps < best_proxy)
      {
         return true;
      }
      if(std::abs(cand_proxy - best_proxy) <= eps)
      {
         return cand_score + eps < best_score;
      }
      return false;
   }
   if(is_hybrid_decision_metric(decision_metric))
   {
      if(cand_score + eps < best_score)
      {
         return true;
      }
      if(std::abs(cand_score - best_score) <= eps)
      {
         return cand_proxy + eps < best_proxy;
      }
      return false;
   }
   return cand_score + eps < best_score;
}

static HLS_deviceRef getHlsDeviceFromApp(const application_managerRef& AppM)
{
   const auto* hls_manager = GetPointer<HLS_manager>(AppM);
   return hls_manager ? hls_manager->get_HLS_device() : HLS_deviceRef();
}

static unsigned resolveRequiredMaxLutSize(const application_managerRef& AppM, const HLS_deviceRef& hls_d)
{
   unsigned resolved_lut_size = 0U;
   const bool found = AppM->TryGetParameterFromParameterOrDevice<unsigned>("max_lut_size", hls_d, resolved_lut_size);
   if(!found)
   {
      THROW_ERROR_USAGE("Missing required parameter \"max_lut_size\": define it in parameter or device parameter "
                        "(required also for ASIC targets)");
   }
   if(resolved_lut_size == 0U)
   {
      THROW_ERROR_USAGE("Invalid parameter \"max_lut_size\": expected value > 0");
   }
   return resolved_lut_size;
}

static ir_nodeRef createShiftOperationWithCostGuard(const ir_manipulationRef& ir_man, const ir_nodeRef& type_expr,
                                                    const ir_nodeRef& lhs, const ir_nodeRef& rhs,
                                                    const std::string& loc_info, enum kind shift_kind,
                                                    const std::string& ASSERT_PARAMETER(context))
{
   THROW_ASSERT(shift_kind == shl_node_K || shift_kind == shr_node_K, "Expected shift operation kind");
   THROW_ASSERT(ir_helper::IsConstant(rhs),
                context + ": shift amount is not constant. Dynamic shifts require explicit shift area/delay modeling.");
   return ir_man->create_binary_operation(type_expr, lhs, rhs, loc_info, shift_kind);
}

ir_nodeRef IR_lowering::expand_smod_pow2(const ir_nodeRef& op0, unsigned long long int d, const ir_nodeRef& stmt,
                                         const blocRef& block, const ir_nodeRef& type,
                                         const std::string& loc_info_default)
{
   unsigned long long int masklow;
   const auto logd = floor_log2(d);
   const auto bt = ir_man->GetBooleanType();

   const auto const0 = TM->CreateUniqueIntegerCst(0, type);
   const auto constm1 = TM->CreateUniqueIntegerCst(-1, type);
   const auto cond_op0 = ir_man->create_binary_operation(bt, op0, const0, loc_info_default, lt_node_K);
   const auto signmask_ga =
       ir_man->CreateAssignStmt(bt, TM->CreateUniqueIntegerCst(0, bt), TM->CreateUniqueIntegerCst(1, bt), cond_op0,
                                function_id, loc_info_default);
   AppM->RegisterTransformation(GetName(), signmask_ga);
   block->PushBefore(signmask_ga, stmt, AppM);
   const auto select_node0 = ir_man->create_ternary_operation(type, GetPointer<assign_stmt>(signmask_ga)->op0, constm1,
                                                              const0, loc_info_default, select_node_K);

   const auto signmask_select_node =
       ir_man->CreateAssignStmt(type, nullptr, nullptr, select_node0, function_id, loc_info_default);

   AppM->RegisterTransformation(GetName(), signmask_select_node);
   block->PushBefore(signmask_select_node, stmt, AppM);

   auto signmask_var = GetPointer<assign_stmt>(signmask_select_node)->op0;
   const auto size = ir_helper::Size(type);

   if(logd > 63 || size < logd)
   {
      THROW_ERROR("unexpected condition");
   }
   masklow = (1ULL << logd) - 1;
   const auto Constmasklow = TM->CreateUniqueIntegerCst(static_cast<long long int>(masklow), type);

   if(!ir_helper::IsUnsignedIntegerType(type))
   {
      auto unsignedType = ir_man->CreateUnsigned(type);
      const auto constshift = TM->CreateUniqueIntegerCst(static_cast<long long>(size - logd), unsignedType);
      auto ga_nop = ir_man->CreateNopExpr(signmask_var, unsignedType, ir_nodeRef(), ir_nodeRef(), function_id);
      auto nop_vd = GetPointer<assign_stmt>(ga_nop)->op0;
      block->PushBefore(ga_nop, stmt, AppM);
      auto temp = createShiftOperationWithCostGuard(ir_man, unsignedType, nop_vd, constshift, loc_info_default,
                                                    shr_node_K, "expand_smod_pow2");
      auto temp_ga = ir_man->CreateAssignStmt(unsignedType, nullptr, nullptr, temp, function_id, loc_info_default);
      AppM->RegisterTransformation(GetName(), temp_ga);
      block->PushBefore(temp_ga, stmt, AppM);
      nop_vd = GetPointer<assign_stmt>(temp_ga)->op0;
      ga_nop = ir_man->CreateNopExpr(nop_vd, type, ir_nodeRef(), ir_nodeRef(), function_id);
      block->PushBefore(ga_nop, stmt, AppM);
      signmask_var = GetPointer<assign_stmt>(ga_nop)->op0;
   }
   else
   {
      const auto constshift = TM->CreateUniqueIntegerCst(static_cast<long long>(size - logd), type);
      auto temp = createShiftOperationWithCostGuard(ir_man, type, signmask_var, constshift, loc_info_default,
                                                    shr_node_K, "expand_smod_pow2");
      auto temp_ga = ir_man->CreateAssignStmt(type, nullptr, nullptr, temp, function_id, loc_info_default);
      AppM->RegisterTransformation(GetName(), temp_ga);
      block->PushBefore(temp_ga, stmt, AppM);
      signmask_var = GetPointer<assign_stmt>(temp_ga)->op0;
   }

   auto temp = ir_man->create_binary_operation(type, op0, signmask_var, loc_info_default, add_node_K);
   auto temp_ga = ir_man->CreateAssignStmt(type, nullptr, nullptr, temp, function_id, loc_info_default);
   AppM->RegisterTransformation(GetName(), temp_ga);
   block->PushBefore(temp_ga, stmt, AppM);
   auto temp_var = GetPointer<assign_stmt>(temp_ga)->op0;

   temp = ir_man->create_binary_operation(type, temp_var, Constmasklow, loc_info_default, and_node_K);
   temp_ga = ir_man->CreateAssignStmt(type, nullptr, nullptr, temp, function_id, loc_info_default);
   AppM->RegisterTransformation(GetName(), temp_ga);
   block->PushBefore(temp_ga, stmt, AppM);
   temp_var = GetPointer<assign_stmt>(temp_ga)->op0;

   return ir_man->create_binary_operation(type, temp_var, signmask_var, loc_info_default, sub_node_K);
}

ir_nodeRef IR_lowering::expand_sdiv_pow2(const ir_nodeRef& op0, unsigned long long int d, const ir_nodeRef& stmt,
                                         const blocRef& block, const ir_nodeRef& type,
                                         const std::string& loc_info_default)
{
   const auto logd = floor_log2(d);
   const auto bt = ir_man->GetBooleanType();
   const auto const0 = TM->CreateUniqueIntegerCst(0, type);

   const auto cond_op0 = ir_man->create_binary_operation(bt, op0, const0, loc_info_default, lt_node_K);
   const auto cond_op0_ga =
       ir_man->CreateAssignStmt(bt, TM->CreateUniqueIntegerCst(0, bt), TM->CreateUniqueIntegerCst(1, bt), cond_op0,
                                function_id, loc_info_default);
   block->PushBefore(cond_op0_ga, stmt, AppM);
   const auto cond_op0_ga_var = GetPointer<assign_stmt>(cond_op0_ga)->op0;
   ir_nodeRef t_ga;
   ir_nodeRef t2_ga;

   if(d == 2)
   {
      const auto const1 = TM->CreateUniqueIntegerCst(1, type);
      const auto cond_op =
          ir_man->create_ternary_operation(type, cond_op0_ga_var, const1, const0, loc_info_default, select_node_K);
      t_ga = ir_man->CreateAssignStmt(type, nullptr, nullptr, cond_op, function_id, loc_info_default);
      block->PushBefore(t_ga, stmt, AppM);
      const auto cond_ga_var = GetPointer<assign_stmt>(t_ga)->op0;

      const auto sum_expr = ir_man->create_binary_operation(type, op0, cond_ga_var, loc_info_default, add_node_K);
      t2_ga = ir_man->CreateAssignStmt(type, nullptr, nullptr, sum_expr, function_id, loc_info_default);
   }
   else
   {
      const auto d_m1 = TM->CreateUniqueIntegerCst(static_cast<long long int>(d - 1), type);
      const auto t_expr = ir_man->create_binary_operation(type, op0, d_m1, loc_info_default, add_node_K);
      t_ga = ir_man->CreateAssignStmt(type, nullptr, nullptr, t_expr, function_id, loc_info_default);
      block->PushBefore(t_ga, stmt, AppM);
      const auto t_ga_var = GetPointer<assign_stmt>(t_ga)->op0;

      const auto cond_op =
          ir_man->create_ternary_operation(type, cond_op0_ga_var, t_ga_var, op0, loc_info_default, select_node_K);
      t2_ga = ir_man->CreateAssignStmt(type, nullptr, nullptr, cond_op, function_id, loc_info_default);
   }
   block->PushBefore(t2_ga, stmt, AppM);

   const auto t2_ga_var = GetPointer<assign_stmt>(t2_ga)->op0;
   const auto logdConst = TM->CreateUniqueIntegerCst(static_cast<long long>(logd), type);
   return createShiftOperationWithCostGuard(ir_man, type, t2_ga_var, logdConst, loc_info_default, shr_node_K,
                                            "expand_sdiv_pow2");
}

namespace
{
   struct MCShiftAddOp
   {
      bool isAdd;     // true => +, false => -
      unsigned shamt; // left shift amount
   };

   static ir_nodeRef emitShiftedTerm(const ir_manipulationRef& ir_man, const ir_managerRef& TM,
                                     unsigned int function_id, const application_managerRef& AppM,
                                     const ir_nodeRef& op0, unsigned shamt, const ir_nodeRef& stmt,
                                     const blocRef& block, const ir_nodeRef& type_expr,
                                     const std::string& loc_info_default);
   static ir_nodeRef reduceChainAdd(const ir_manipulationRef& ir_man, unsigned int function_id,
                                    const application_managerRef& AppM, const std::vector<ir_nodeRef>& terms,
                                    const ir_nodeRef& stmt, const blocRef& block, const ir_nodeRef& type_expr,
                                    const std::string& loc_info_default);

   // Compute NAF (Non-Adjacent Form) as done in LLVM's RISC-V backend.
   static void computeNAF(unsigned long long coeff, unsigned bitWidth, std::vector<MCShiftAddOp>& ops)
   {
      ops.clear();
      if(coeff == 0)
      {
         return;
      }

      unsigned long long e = coeff;
      unsigned i = 0;

      while(e > 0 && i < bitWidth)
      {
         if(e & 1ULL)
         {
            const int z = ((e & 3ULL) == 1ULL) ? 1 : -1;
            ops.push_back(MCShiftAddOp{z == 1, i});
            const long long e_signed = static_cast<long long>(e) - static_cast<long long>(z);
            e = static_cast<unsigned long long>(e_signed);
         }
         e >>= 1;
         ++i;
      }
   }

   static bool isPower2(unsigned long long value)
   {
      return value != 0 && POWER2_OR_0(value);
   }

   static void appendShiftedOps(const std::vector<MCShiftAddOp>& src, unsigned shift, bool invert,
                                std::vector<MCShiftAddOp>& dst)
   {
      for(const auto& op : src)
      {
         dst.push_back(MCShiftAddOp{invert ? !op.isAdd : op.isAdd, op.shamt + shift});
      }
   }

   static ir_nodeRef emitMulConstOpsNAF(const ir_manipulationRef& ir_man, const ir_managerRef& TM,
                                        unsigned int function_id, const application_managerRef& AppM,
                                        const ir_nodeRef& op0, const std::vector<MCShiftAddOp>& ops,
                                        const ir_nodeRef& stmt, const blocRef& block, const ir_nodeRef& type_expr,
                                        const std::string& loc_info_default)
   {
      std::vector<ir_nodeRef> pos_terms;
      std::vector<ir_nodeRef> neg_terms;
      pos_terms.reserve(ops.size());
      neg_terms.reserve(ops.size());

      for(const auto& o : ops)
      {
         const auto term =
             emitShiftedTerm(ir_man, TM, function_id, AppM, op0, o.shamt, stmt, block, type_expr, loc_info_default);
         if(o.isAdd)
         {
            pos_terms.push_back(term);
         }
         else
         {
            neg_terms.push_back(term);
         }
      }

      if(pos_terms.empty() && neg_terms.empty())
      {
         return TM->CreateUniqueIntegerCst(0, type_expr);
      }

      ir_nodeRef pos_sum;
      ir_nodeRef neg_sum;
      if(!pos_terms.empty())
      {
         pos_sum = reduceChainAdd(ir_man, function_id, AppM, pos_terms, stmt, block, type_expr, loc_info_default);
      }
      if(!neg_terms.empty())
      {
         neg_sum = reduceChainAdd(ir_man, function_id, AppM, neg_terms, stmt, block, type_expr, loc_info_default);
      }

      if(neg_terms.empty())
      {
         return pos_sum;
      }

      if(pos_terms.empty())
      {
         const auto neg_expr = ir_man->create_unary_operation(type_expr, neg_sum, loc_info_default, neg_node_K);
         const auto ga =
             ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), neg_expr, function_id, loc_info_default);
         block->PushBefore(ga, stmt, AppM);
         return GetPointer<assign_stmt>(ga)->op0;
      }

      const auto sub_expr = ir_man->create_binary_operation(type_expr, pos_sum, neg_sum, loc_info_default, sub_node_K);
      const auto ga =
          ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), sub_expr, function_id, loc_info_default);
      block->PushBefore(ga, stmt, AppM);
      return GetPointer<assign_stmt>(ga)->op0;
   }

   static ir_nodeRef emitShiftedTerm(const ir_manipulationRef& ir_man, const ir_managerRef& TM,
                                     unsigned int function_id, const application_managerRef& AppM,
                                     const ir_nodeRef& op0, unsigned shamt, const ir_nodeRef& stmt,
                                     const blocRef& block, const ir_nodeRef& type_expr,
                                     const std::string& loc_info_default)
   {
      if(shamt == 0)
      {
         return op0;
      }

      const auto sh = TM->CreateUniqueIntegerCst(static_cast<long long>(shamt), type_expr);
      const auto sh_expr = createShiftOperationWithCostGuard(ir_man, type_expr, op0, sh, loc_info_default, shl_node_K,
                                                             "emitShiftedTerm");
      const auto sh_ga =
          ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), sh_expr, function_id, loc_info_default);
      block->PushBefore(sh_ga, stmt, AppM);
      return GetPointer<assign_stmt>(sh_ga)->op0;
   }

   static ir_nodeRef reduceChainAdd(const ir_manipulationRef& ir_man, unsigned int function_id,
                                    const application_managerRef& AppM, const std::vector<ir_nodeRef>& terms,
                                    const ir_nodeRef& stmt, const blocRef& block, const ir_nodeRef& type_expr,
                                    const std::string& loc_info_default)
   {
      THROW_ASSERT(!terms.empty(), "reduceChainAdd called with empty terms");
      ir_nodeRef accum = terms.front();

      for(size_t i = 1; i < terms.size(); ++i)
      {
         const auto add_expr =
             ir_man->create_binary_operation(type_expr, accum, terms[i], loc_info_default, add_node_K);
         const auto ga =
             ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), add_expr, function_id, loc_info_default);
         block->PushBefore(ga, stmt, AppM);
         accum = GetPointer<assign_stmt>(ga)->op0;
      }

      return accum;
   }

   static ir_nodeRef emitBalancedAddTree(const ir_manipulationRef& ir_man, unsigned int function_id,
                                         const application_managerRef& AppM, const std::vector<ir_nodeRef>& terms,
                                         const ir_nodeRef& stmt, const blocRef& block, const ir_nodeRef& type_expr,
                                         const std::string& loc_info_default)
   {
      THROW_ASSERT(!terms.empty(), "emitBalancedAddTree called with empty terms");
      std::vector<ir_nodeRef> cur = terms;

      while(cur.size() > 1)
      {
         std::vector<ir_nodeRef> next;
         next.reserve((cur.size() + 1) / 2);

         for(size_t i = 0; i < cur.size(); i += 2)
         {
            if(i + 1 == cur.size())
            {
               next.push_back(cur[i]);
               continue;
            }

            const auto add_expr =
                ir_man->create_binary_operation(type_expr, cur[i], cur[i + 1], loc_info_default, add_node_K);
            const auto ga = ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), add_expr, function_id,
                                                     loc_info_default);
            block->PushBefore(ga, stmt, AppM);
            next.push_back(GetPointer<assign_stmt>(ga)->op0);
         }

         cur.swap(next);
      }

      return cur.front();
   }

   struct KcmEstimate
   {
      double area;
      double delay;
      unsigned q;
      unsigned alpha_eff;
      bool valid;
   };

   using KlutSignal = mockturtle::klut_network::signal;

   static KlutSignal createKlutNodeFromConst(mockturtle::klut_network& ntk, const std::vector<KlutSignal>& fanins,
                                             uint64_t lut_const)
   {
      if(fanins.empty())
      {
         return ntk.get_constant((lut_const & 1ULL) != 0ULL);
      }

      const auto vars = static_cast<unsigned>(fanins.size());
      kitty::dynamic_truth_table tt(vars);
      const auto entries = 1U << vars;
      for(unsigned v = 0; v < entries; ++v)
      {
         if(((lut_const >> v) & 1ULL) != 0ULL)
         {
            kitty::set_bit(tt, v);
         }
      }
      return ntk.create_node(fanins, tt);
   }

   static KlutSignal createKlutNot(mockturtle::klut_network& ntk, KlutSignal in)
   {
      return createKlutNodeFromConst(ntk, {in}, 0x1ULL);
   }

   static KlutSignal createKlutAnd(mockturtle::klut_network& ntk, KlutSignal a, KlutSignal b)
   {
      return createKlutNodeFromConst(ntk, {a, b}, 0x8ULL);
   }

   static KlutSignal createKlutOr(mockturtle::klut_network& ntk, KlutSignal a, KlutSignal b)
   {
      return createKlutNodeFromConst(ntk, {a, b}, 0xEULL);
   }

   static KlutSignal createKlutXor(mockturtle::klut_network& ntk, KlutSignal a, KlutSignal b)
   {
      return createKlutNodeFromConst(ntk, {a, b}, 0x6ULL);
   }

   static std::vector<KlutSignal> makeZeroWord(mockturtle::klut_network& ntk, unsigned width)
   {
      return std::vector<KlutSignal>(width, ntk.get_constant(false));
   }

   static std::vector<KlutSignal> makeOneWord(mockturtle::klut_network& ntk, unsigned width)
   {
      auto one = makeZeroWord(ntk, width);
      if(width > 0)
      {
         one[0] = ntk.get_constant(true);
      }
      return one;
   }

   static std::vector<KlutSignal> addKlutWords(mockturtle::klut_network& ntk, const std::vector<KlutSignal>& lhs,
                                               const std::vector<KlutSignal>& rhs)
   {
      THROW_ASSERT(lhs.size() == rhs.size(), "Mismatched widths in addKlutWords");
      const auto width = lhs.size();
      std::vector<KlutSignal> sum(width, ntk.get_constant(false));
      auto carry = ntk.get_constant(false);

      for(size_t bit = 0; bit < width; ++bit)
      {
         const auto ab_xor = createKlutXor(ntk, lhs[bit], rhs[bit]);
         sum[bit] = createKlutXor(ntk, ab_xor, carry);
         const auto ab_and = createKlutAnd(ntk, lhs[bit], rhs[bit]);
         const auto carry_and = createKlutAnd(ntk, ab_xor, carry);
         carry = createKlutOr(ntk, ab_and, carry_and);
      }

      return sum;
   }

   static std::vector<KlutSignal> negateKlutWord(mockturtle::klut_network& ntk, const std::vector<KlutSignal>& in)
   {
      std::vector<KlutSignal> inverted;
      inverted.reserve(in.size());
      for(const auto bit : in)
      {
         inverted.push_back(createKlutNot(ntk, bit));
      }
      return addKlutWords(ntk, inverted, makeOneWord(ntk, static_cast<unsigned>(in.size())));
   }

   static std::vector<KlutSignal> shiftKlutWord(mockturtle::klut_network& ntk, const std::vector<KlutSignal>& in,
                                                unsigned shift, unsigned width)
   {
      auto out = makeZeroWord(ntk, width);
      for(unsigned bit = 0; bit < width; ++bit)
      {
         const unsigned src = bit >= shift ? (bit - shift) : width;
         if(src < width)
         {
            out[bit] = in[src];
         }
      }
      return out;
   }

   static std::vector<KlutSignal> reduceKlutWords(mockturtle::klut_network& ntk,
                                                  std::vector<std::vector<KlutSignal>> words, bool balanced)
   {
      if(words.empty())
      {
         return {};
      }
      if(words.size() == 1)
      {
         return words.front();
      }
      if(!balanced)
      {
         auto accum = words.front();
         for(size_t i = 1; i < words.size(); ++i)
         {
            accum = addKlutWords(ntk, accum, words[i]);
         }
         return accum;
      }

      while(words.size() > 1)
      {
         std::vector<std::vector<KlutSignal>> next;
         next.reserve((words.size() + 1) / 2);
         for(size_t i = 0; i < words.size(); i += 2)
         {
            if(i + 1 == words.size())
            {
               next.push_back(words[i]);
               continue;
            }
            next.push_back(addKlutWords(ntk, words[i], words[i + 1]));
         }
         words.swap(next);
      }
      return words.front();
   }

   static unsigned maxLutConstInputs()
   {
      return static_cast<unsigned>(floor_log2(static_cast<unsigned long long>(std::numeric_limits<uint64_t>::digits)));
   }

   static mockturtle::klut_network buildKcmKlutNetwork(unsigned wX, unsigned wR, unsigned long long coeff,
                                                       bool negative, unsigned alpha_eff,
                                                       const std::string& sum_strategy)
   {
      mockturtle::klut_network ntk;
      std::vector<KlutSignal> x_bits;
      x_bits.reserve(wX);
      for(unsigned bit = 0; bit < wX; ++bit)
      {
         x_bits.push_back(ntk.create_pi());
      }

      const unsigned q = (wX + alpha_eff - 1) / alpha_eff;
      std::vector<std::vector<KlutSignal>> partial_words;
      partial_words.reserve(q);

      for(unsigned chunk = 0; chunk < q; ++chunk)
      {
         const unsigned base_bit = chunk * alpha_eff;
         std::vector<KlutSignal> chunk_inputs;
         chunk_inputs.reserve(alpha_eff);
         for(unsigned j = 0; j < alpha_eff; ++j)
         {
            const unsigned bit_index = base_bit + j;
            if(bit_index < wX)
            {
               chunk_inputs.push_back(x_bits[bit_index]);
            }
            else
            {
               chunk_inputs.push_back(ntk.get_constant(false));
            }
         }

         std::vector<KlutSignal> partial = makeZeroWord(ntk, wR);
         for(unsigned b = 0; b < wR; ++b)
         {
            const auto lut_const = kcm_constmul::buildKcmLutConstant(coeff, alpha_eff, b, wR);
            partial[b] = createKlutNodeFromConst(ntk, chunk_inputs, lut_const);
         }

         partial_words.push_back(shiftKlutWord(ntk, partial, base_bit, wR));
      }

      auto result = reduceKlutWords(ntk, std::move(partial_words), sum_strategy == "tree");
      if(result.empty())
      {
         result = makeZeroWord(ntk, wR);
      }
      if(negative)
      {
         result = negateKlutWord(ntk, result);
      }
      for(const auto bit : result)
      {
         ntk.create_po(bit);
      }
      return ntk;
   }

   static mockturtle::klut_network buildKcmKlutNetworkMerged(unsigned wX, unsigned wR, unsigned long long coeff,
                                                             bool negative, unsigned alpha_eff)
   {
      mockturtle::klut_network ntk;
      std::vector<KlutSignal> x_bits;
      x_bits.reserve(wX);
      for(unsigned bit = 0; bit < wX; ++bit)
      {
         x_bits.push_back(ntk.create_pi());
      }

      const unsigned q = (wX + alpha_eff - 1) / alpha_eff;
      const unsigned wC = coeff == 0 ? 1U : static_cast<unsigned>(floor_log2(coeff)) + 1U;
      const unsigned max_unshifted_bits = std::min<unsigned>(wR, alpha_eff + wC);
      const unsigned lut_entries = 1U << alpha_eff;
      const uint64_t lut_all_ones = lut_entries >= 64U ? ~0ULL : ((1ULL << lut_entries) - 1ULL);
      auto acc = makeZeroWord(ntk, wR);

      for(unsigned chunk = 0; chunk < q; ++chunk)
      {
         const unsigned base_bit = chunk * alpha_eff;
         if(base_bit >= wR)
         {
            break;
         }
         const unsigned max_bits_after_shift = wR - base_bit;
         const unsigned chunk_active_bits = std::min(max_unshifted_bits, max_bits_after_shift);
         if(chunk_active_bits == 0)
         {
            continue;
         }

         std::vector<KlutSignal> chunk_inputs;
         chunk_inputs.reserve(alpha_eff);
         for(unsigned j = 0; j < alpha_eff; ++j)
         {
            const unsigned bit_index = base_bit + j;
            chunk_inputs.push_back(bit_index < wX ? x_bits[bit_index] : ntk.get_constant(false));
         }

         std::vector<KlutSignal> out = makeZeroWord(ntk, wR);
         for(unsigned bit = 0; bit < base_bit; ++bit)
         {
            out[bit] = acc[bit];
         }

         KlutSignal carry = ntk.get_constant(false);
         for(unsigned bit = base_bit; bit < wR; ++bit)
         {
            const unsigned local_bit = bit - base_bit;
            const auto acc_bit = acc[bit];
            KlutSignal s_bit = acc_bit;

            if(local_bit < chunk_active_bits)
            {
               const auto lut_const = kcm_constmul::buildKcmLutConstant(coeff, alpha_eff, local_bit, wR);
               if(lut_const == 0)
               {
                  s_bit = acc_bit;
               }
               else if(lut_const == lut_all_ones)
               {
                  s_bit = createKlutNot(ntk, acc_bit);
               }
               else
               {
                  const uint64_t lut_const_inv = (~lut_const) & lut_all_ones;
                  const uint64_t merged_lut_const = lut_const | (lut_const_inv << lut_entries);
                  auto s_inputs = chunk_inputs;
                  s_inputs.push_back(acc_bit);
                  s_bit = createKlutNodeFromConst(ntk, s_inputs, merged_lut_const);
               }
            }

            const auto sum_bit = createKlutXor(ntk, s_bit, carry);
            const auto not_s = createKlutNot(ntk, s_bit);
            const auto carry_if_s = createKlutAnd(ntk, s_bit, carry);
            const auto carry_if_not_s = createKlutAnd(ntk, not_s, acc_bit);
            carry = createKlutOr(ntk, carry_if_s, carry_if_not_s);
            out[bit] = sum_bit;
         }
         acc.swap(out);
      }

      if(negative)
      {
         acc = negateKlutWord(ntk, acc);
      }
      for(const auto bit : acc)
      {
         ntk.create_po(bit);
      }
      return ntk;
   }

   static bool mapKlutToCollapsedNetwork(const mockturtle::klut_network& klut, unsigned max_lut_size,
                                         mockturtle::klut_network& collapsed)
   {
      if(max_lut_size == 0)
      {
         return false;
      }
      try
      {
         mockturtle::shannon_resynthesis<mockturtle::aig_network> fallback;
         mockturtle::dsd_resynthesis<mockturtle::aig_network, decltype(fallback)> aig_resyn(fallback);
         auto aig = mockturtle::node_resynthesis<mockturtle::aig_network>(klut, aig_resyn);
         aig = mockturtle::cleanup_dangling(aig);

         mockturtle::mapping_view<mockturtle::aig_network, true> mapped_klut{aig};
         mockturtle::lut_mapping_params mp;
         mp.cut_enumeration_ps.cut_size = static_cast<uint32_t>(max_lut_size);
         mp.cut_enumeration_ps.cut_limit = 16;
#ifndef NDEBUG
         mp.verbose = false;
         mp.cut_enumeration_ps.very_verbose = false;
#endif
         mockturtle::lut_mapping<decltype(mapped_klut), true>(mapped_klut, mp);
         collapsed = *mockturtle::collapse_mapped_network<mockturtle::klut_network>(mapped_klut);
         collapsed = mockturtle::cleanup_luts(collapsed);
         return true;
      }
      catch(...)
      {
         return false;
      }
   }

   static KcmEstimate estimateKcmCostMockturtle(unsigned wX, unsigned wR, unsigned long long coeff, bool negative,
                                                unsigned alpha_eff, unsigned max_lut_size, bool merge_mode,
                                                const std::string& sum_strategy, double lut_delay_base,
                                                double lut_area_base)
   {
      KcmEstimate est{0.0, 0.0, 0U, alpha_eff, false};
      if(alpha_eff == 0 || wX == 0 || wR == 0 || max_lut_size == 0)
      {
         return est;
      }
      est.q = (wX + alpha_eff - 1) / alpha_eff;

      try
      {
         const auto kcm_klut = merge_mode ? buildKcmKlutNetworkMerged(wX, wR, coeff, negative, alpha_eff) :
                                            buildKcmKlutNetwork(wX, wR, coeff, negative, alpha_eff, sum_strategy);
         mockturtle::klut_network collapsed;
         if(!mapKlutToCollapsedNetwork(kcm_klut, max_lut_size, collapsed))
         {
            return est;
         }
         const auto lut_count = collapsed.num_gates();
         mockturtle::depth_view depth_view{collapsed};
         const auto lut_depth = std::max<unsigned>(1U, depth_view.depth());
         est.area = static_cast<double>(lut_count) * lut_area_base;
         est.delay = static_cast<double>(lut_depth) * lut_delay_base;
         est.valid = true;
      }
      catch(...)
      {
         est.valid = false;
      }
      return est;
   }

   static unsigned bitWidthUnsigned(unsigned long long value)
   {
      if(value == 0)
      {
         return 1U;
      }
      return static_cast<unsigned>(floor_log2(value)) + 1U;
   }

   static KcmEstimate estimateKcmCost(const IR_lowering& lowering, const HLS_deviceRef& HLS_D, unsigned wX, unsigned wC,
                                      unsigned wR, unsigned long long coeff, unsigned alpha_eff, unsigned max_lut_size,
                                      const std::string& sum_strategy, bool merge_mode, double add_delay,
                                      double add_area, double lut_delay, double lut_area)
   {
      KcmEstimate est{0.0, 0.0, 0U, alpha_eff, false};
      (void)lowering;
      (void)HLS_D;
      (void)max_lut_size;
      if(alpha_eff == 0 || wX == 0)
      {
         return est;
      }

      const unsigned q = (wX + alpha_eff - 1) / alpha_eff;
      est.q = q;

      if(merge_mode)
      {
         const unsigned max_unshifted_bits = std::min<unsigned>(wR, alpha_eff + wC);
         const unsigned lut_entries = 1U << alpha_eff;
         const uint64_t lut_all_ones = lut_entries >= 64U ? ~0ULL : ((1ULL << lut_entries) - 1ULL);
         const auto fu_prec = std::max(8U, static_cast<unsigned>(ceil_pow2(static_cast<unsigned long long>(wR))));
         const double add_per_bit_area = add_area / static_cast<double>(fu_prec);
         const double add_per_bit_delay = add_delay / static_cast<double>(fu_prec);

         unsigned nontrivial_luts = 0;
         unsigned carry_bits_total = 0;
         double delay_total = 0.0;
         for(unsigned chunk = 0; chunk < q; ++chunk)
         {
            const unsigned base_bit = chunk * alpha_eff;
            if(base_bit >= wR)
            {
               break;
            }
            const unsigned max_bits_after_shift = wR - base_bit;
            const unsigned chunk_active_bits = std::min(max_unshifted_bits, max_bits_after_shift);
            if(chunk_active_bits == 0)
            {
               continue;
            }
            for(unsigned b = 0; b < chunk_active_bits; ++b)
            {
               const auto lut_const = kcm_constmul::buildKcmLutConstant(coeff, alpha_eff, b, wR);
               if(lut_const != 0 && lut_const != lut_all_ones)
               {
                  ++nontrivial_luts;
               }
            }
            const unsigned carry_span = wR - base_bit;
            carry_bits_total += carry_span;
            const double stage_lut_delay = chunk_active_bits > 0 ? lut_delay : 0.0;
            const double stage_carry_delay = static_cast<double>(carry_span) * add_per_bit_delay;
            delay_total += stage_lut_delay + stage_carry_delay;
         }

         est.area =
             static_cast<double>(nontrivial_luts) * lut_area + static_cast<double>(carry_bits_total) * add_per_bit_area;
         est.delay = delay_total;
         est.valid = true;
         return est;
      }

      unsigned depth_add = 0;
      if(q > 1)
      {
         if(sum_strategy == "rake")
         {
            depth_add = q - 1;
         }
         else
         {
            depth_add = static_cast<unsigned>(ceil_log2(static_cast<unsigned long long>(q)));
         }
      }

      const double table_cost_lut = static_cast<double>(alpha_eff + wC) * lut_area;
      const double table_area = table_cost_lut * static_cast<double>(q);
      const double adder_area = q > 1 ? static_cast<double>(q - 1U) * add_area : 0.0;
      const double sum_area = q > 1 ? std::max(table_area, adder_area) : 0.0;
      est.area = table_area + sum_area;
      est.delay = std::max(lut_delay, static_cast<double>(depth_add) * add_delay);
      est.valid = true;
      return est;
   }

   static ir_nodeRef emitMulConstOpsKcm(const ir_manipulationRef& ir_man, const ir_managerRef& TM,
                                        unsigned int function_id, const application_managerRef& AppM,
                                        const ir_nodeRef& op0, unsigned long long coeff, bool negative,
                                        unsigned alpha_eff, unsigned ASSERT_PARAMETER(max_lut_node_inputs),
                                        const std::string& sum_strategy, const ir_nodeRef& stmt, const blocRef& block,
                                        const ir_nodeRef& type_expr, const std::string& loc_info_default)
   {
      const auto boolType = ir_man->GetBooleanType();
      const auto indexType = ir_man->GetUnsignedLongLongType();
      const auto lutConstType = ir_man->GetUnsignedLongLongType();
      const unsigned wX = static_cast<unsigned>(ir_helper::Size(op0));
      const unsigned wR = static_cast<unsigned>(ir_helper::Size(type_expr));

      if(alpha_eff == 0 || wR == 0)
      {
         return TM->CreateUniqueIntegerCst(0, type_expr);
      }
      THROW_ASSERT(alpha_eff <= max_lut_node_inputs, "Unexpected KCM LUT arity: " + std::to_string(alpha_eff) + " > " +
                                                         std::to_string(max_lut_node_inputs));

      const unsigned q = (wX + alpha_eff - 1) / alpha_eff;
      const unsigned wC = bitWidthUnsigned(coeff);
      const unsigned max_unshifted_bits = std::min<unsigned>(wR, alpha_eff + wC);
      const unsigned lut_entries = 1U << alpha_eff;
      const uint64_t lut_all_ones = lut_entries >= 64U ? ~0ULL : ((1ULL << lut_entries) - 1ULL);
      std::vector<ir_nodeRef> partials;
      partials.reserve(q);

      for(unsigned chunk = 0; chunk < q; ++chunk)
      {
         const unsigned base_bit = chunk * alpha_eff;
         if(base_bit >= wR)
         {
            break;
         }
         const unsigned max_bits_after_shift = wR - base_bit;
         const unsigned chunk_active_bits = std::min(max_unshifted_bits, max_bits_after_shift);
         if(chunk_active_bits == 0)
         {
            continue;
         }
         std::vector<ir_nodeRef> lut_inputs;
         lut_inputs.reserve(alpha_eff);

         for(unsigned j = 0; j < alpha_eff; ++j)
         {
            const unsigned bit_index = base_bit + j;
            if(bit_index < wX)
            {
               const auto bit_pos = TM->CreateUniqueIntegerCst(static_cast<long long>(bit_index), indexType);
               const auto eb_op = ir_man->create_extract_bit_node(op0, bit_pos, loc_info_default);
               const auto eb_ga = ir_man->CreateAssignStmt(boolType, TM->CreateUniqueIntegerCst(0, boolType),
                                                           TM->CreateUniqueIntegerCst(1, boolType), eb_op, function_id,
                                                           loc_info_default);
               block->PushBefore(eb_ga, stmt, AppM);
               lut_inputs.push_back(GetPointer<assign_stmt>(eb_ga)->op0);
            }
            else
            {
               lut_inputs.push_back(TM->CreateUniqueIntegerCst(0, boolType));
            }
         }

         ir_nodeRef partial = TM->CreateUniqueIntegerCst(0, type_expr);
         bool partial_has_term = false;

         for(unsigned b = 0; b < chunk_active_bits; ++b)
         {
            const uint64_t lut_const =
                kcm_constmul::buildKcmLutConstant(static_cast<uint64_t>(coeff), alpha_eff, b, wR);
            if(lut_const == 0)
            {
               continue;
            }

            ir_nodeRef term;
            if(lut_const == lut_all_ones)
            {
               term = TM->CreateUniqueIntegerCst(1, type_expr);
            }
            else
            {
               const auto lut_const_node = TM->CreateUniqueIntegerCst(static_cast<long long>(lut_const), lutConstType);

               ir_nodeRef op1 = lut_inputs.size() > 0 ? lut_inputs[0] : TM->CreateUniqueIntegerCst(0, boolType);
               ir_nodeRef op2 = lut_inputs.size() > 1 ? lut_inputs[1] : ir_nodeRef();
               ir_nodeRef op3 = lut_inputs.size() > 2 ? lut_inputs[2] : ir_nodeRef();
               ir_nodeRef op4 = lut_inputs.size() > 3 ? lut_inputs[3] : ir_nodeRef();
               ir_nodeRef op5 = lut_inputs.size() > 4 ? lut_inputs[4] : ir_nodeRef();
               ir_nodeRef op6 = lut_inputs.size() > 5 ? lut_inputs[5] : ir_nodeRef();

               const auto lut_node = ir_man->create_lut_node(boolType, lut_const_node, op1, op2, op3, op4, op5, op6,
                                                             ir_nodeRef(), ir_nodeRef(), loc_info_default);
               const auto lut_ga = ir_man->CreateAssignStmt(boolType, TM->CreateUniqueIntegerCst(0, boolType),
                                                            TM->CreateUniqueIntegerCst(1, boolType), lut_node,
                                                            function_id, loc_info_default);
               block->PushBefore(lut_ga, stmt, AppM);
               const auto lut_var = GetPointer<assign_stmt>(lut_ga)->op0;

               const auto cast_expr = ir_man->create_unary_operation(type_expr, lut_var, loc_info_default, nop_node_K);
               const auto cast_ga = ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), cast_expr,
                                                             function_id, loc_info_default);
               block->PushBefore(cast_ga, stmt, AppM);
               term = GetPointer<assign_stmt>(cast_ga)->op0;
            }

            if(b > 0)
            {
               term = emitShiftedTerm(ir_man, TM, function_id, AppM, term, b, stmt, block, type_expr, loc_info_default);
            }

            if(!partial_has_term)
            {
               partial = term;
               partial_has_term = true;
            }
            else
            {
               const auto or_expr =
                   ir_man->create_binary_operation(type_expr, partial, term, loc_info_default, or_node_K);
               const auto or_ga = ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), or_expr, function_id,
                                                           loc_info_default);
               block->PushBefore(or_ga, stmt, AppM);
               partial = GetPointer<assign_stmt>(or_ga)->op0;
            }
         }

         if(!partial_has_term)
         {
            continue;
         }

         if(base_bit > 0)
         {
            partial = emitShiftedTerm(ir_man, TM, function_id, AppM, partial, base_bit, stmt, block, type_expr,
                                      loc_info_default);
         }
         partials.push_back(partial);
      }

      ir_nodeRef result;
      if(partials.empty())
      {
         result = TM->CreateUniqueIntegerCst(0, type_expr);
      }
      else if(partials.size() == 1)
      {
         result = partials.front();
      }
      else if(sum_strategy == "rake")
      {
         result = reduceChainAdd(ir_man, function_id, AppM, partials, stmt, block, type_expr, loc_info_default);
      }
      else
      {
         result = emitBalancedAddTree(ir_man, function_id, AppM, partials, stmt, block, type_expr, loc_info_default);
      }

      if(negative)
      {
         const auto neg_expr = ir_man->create_unary_operation(type_expr, result, loc_info_default, neg_node_K);
         const auto ga =
             ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), neg_expr, function_id, loc_info_default);
         block->PushBefore(ga, stmt, AppM);
         return GetPointer<assign_stmt>(ga)->op0;
      }

      return result;
   }

   static ir_nodeRef emitMulConstOpsKcmMerged(const ir_manipulationRef& ir_man, const ir_managerRef& TM,
                                              unsigned int function_id, const application_managerRef& AppM,
                                              const ir_nodeRef& op0, unsigned long long coeff, bool negative,
                                              unsigned alpha_eff, unsigned ASSERT_PARAMETER(max_lut_node_inputs),
                                              const std::string& sum_strategy, const ir_nodeRef& stmt,
                                              const blocRef& block, const ir_nodeRef& type_expr,
                                              const std::string& loc_info_default)
   {
      (void)sum_strategy;
      const auto boolType = ir_man->GetBooleanType();
      const auto indexType = ir_man->GetUnsignedLongLongType();
      const auto lutConstType = ir_man->GetUnsignedLongLongType();
      const unsigned wX = static_cast<unsigned>(ir_helper::Size(op0));
      const unsigned wR = static_cast<unsigned>(ir_helper::Size(type_expr));

      if(alpha_eff == 0 || wR == 0)
      {
         return TM->CreateUniqueIntegerCst(0, type_expr);
      }
      THROW_ASSERT(alpha_eff + 1U <= max_lut_node_inputs,
                   "Unexpected merged KCM LUT arity: " + std::to_string(alpha_eff + 1U) + " > " +
                       std::to_string(max_lut_node_inputs));

      const auto bool_zero = TM->CreateUniqueIntegerCst(0, boolType);
      const auto bool_one = TM->CreateUniqueIntegerCst(1, boolType);
      const unsigned q = (wX + alpha_eff - 1) / alpha_eff;
      const unsigned wC = bitWidthUnsigned(coeff);
      const unsigned max_unshifted_bits = std::min<unsigned>(wR, alpha_eff + wC);
      const unsigned lut_entries = 1U << alpha_eff;
      const uint64_t lut_all_ones = lut_entries >= 64U ? ~0ULL : ((1ULL << lut_entries) - 1ULL);
      ir_nodeRef acc = TM->CreateUniqueIntegerCst(0, type_expr);

      auto extract_word_bit = [&](const ir_nodeRef& word, unsigned bit_index) -> ir_nodeRef {
         if(bit_index >= wR)
         {
            return bool_zero;
         }
         const auto bit_pos = TM->CreateUniqueIntegerCst(static_cast<long long>(bit_index), indexType);
         const auto eb_op = ir_man->create_extract_bit_node(word, bit_pos, loc_info_default);
         const auto eb_ga =
             ir_man->CreateAssignStmt(boolType, bool_zero, bool_one, eb_op, function_id, loc_info_default);
         block->PushBefore(eb_ga, stmt, AppM);
         return GetPointer<assign_stmt>(eb_ga)->op0;
      };

      auto emit_bool_binary = [&](const ir_nodeRef& lhs, const ir_nodeRef& rhs, enum kind code) -> ir_nodeRef {
         const auto expr = ir_man->create_binary_operation(boolType, lhs, rhs, loc_info_default, code);
         const auto ga = ir_man->CreateAssignStmt(boolType, bool_zero, bool_one, expr, function_id, loc_info_default);
         block->PushBefore(ga, stmt, AppM);
         return GetPointer<assign_stmt>(ga)->op0;
      };

      auto pack_word_bits = [&](const std::vector<ir_nodeRef>& bits) -> ir_nodeRef {
         ir_nodeRef packed = TM->CreateUniqueIntegerCst(0, type_expr);
         bool has_bits = false;
         for(unsigned bit = 0; bit < bits.size(); ++bit)
         {
            auto bit_word = ir_man->create_unary_operation(type_expr, bits[bit], loc_info_default, nop_node_K);
            auto bit_ga = ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), bit_word, function_id,
                                                   loc_info_default);
            block->PushBefore(bit_ga, stmt, AppM);
            ir_nodeRef term = GetPointer<assign_stmt>(bit_ga)->op0;
            if(bit > 0)
            {
               term =
                   emitShiftedTerm(ir_man, TM, function_id, AppM, term, bit, stmt, block, type_expr, loc_info_default);
            }
            if(!has_bits)
            {
               packed = term;
               has_bits = true;
            }
            else
            {
               const auto or_expr =
                   ir_man->create_binary_operation(type_expr, packed, term, loc_info_default, or_node_K);
               const auto or_ga = ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), or_expr, function_id,
                                                           loc_info_default);
               block->PushBefore(or_ga, stmt, AppM);
               packed = GetPointer<assign_stmt>(or_ga)->op0;
            }
         }
         return has_bits ? packed : TM->CreateUniqueIntegerCst(0, type_expr);
      };

      for(unsigned chunk = 0; chunk < q; ++chunk)
      {
         const unsigned base_bit = chunk * alpha_eff;
         if(base_bit >= wR)
         {
            break;
         }
         const unsigned max_bits_after_shift = wR - base_bit;
         const unsigned chunk_active_bits = std::min(max_unshifted_bits, max_bits_after_shift);
         if(chunk_active_bits == 0)
         {
            continue;
         }

         std::vector<ir_nodeRef> lut_inputs;
         lut_inputs.reserve(alpha_eff);
         for(unsigned j = 0; j < alpha_eff; ++j)
         {
            const unsigned bit_index = base_bit + j;
            if(bit_index < wX)
            {
               const auto bit_pos = TM->CreateUniqueIntegerCst(static_cast<long long>(bit_index), indexType);
               const auto eb_op = ir_man->create_extract_bit_node(op0, bit_pos, loc_info_default);
               const auto eb_ga =
                   ir_man->CreateAssignStmt(boolType, bool_zero, bool_one, eb_op, function_id, loc_info_default);
               block->PushBefore(eb_ga, stmt, AppM);
               lut_inputs.push_back(GetPointer<assign_stmt>(eb_ga)->op0);
            }
            else
            {
               lut_inputs.push_back(bool_zero);
            }
         }

         std::vector<ir_nodeRef> out_bits;
         out_bits.reserve(wR);
         for(unsigned bit = 0; bit < base_bit; ++bit)
         {
            out_bits.push_back(extract_word_bit(acc, bit));
         }

         ir_nodeRef carry = bool_zero;
         for(unsigned bit = base_bit; bit < wR; ++bit)
         {
            const unsigned local_bit = bit - base_bit;
            const auto acc_bit = extract_word_bit(acc, bit);
            ir_nodeRef s_bit = acc_bit;

            if(local_bit < chunk_active_bits)
            {
               const uint64_t lut_const =
                   kcm_constmul::buildKcmLutConstant(static_cast<uint64_t>(coeff), alpha_eff, local_bit, wR);
               if(lut_const == 0)
               {
                  s_bit = acc_bit;
               }
               else if(lut_const == lut_all_ones)
               {
                  s_bit = emit_bool_binary(acc_bit, bool_one, xor_node_K);
               }
               else
               {
                  const uint64_t lut_const_inv = (~lut_const) & lut_all_ones;
                  const uint64_t merged_lut_const = lut_const | (lut_const_inv << lut_entries);
                  const auto lut_const_node =
                      TM->CreateUniqueIntegerCst(static_cast<long long>(merged_lut_const), lutConstType);
                  std::vector<ir_nodeRef> lut_args = lut_inputs;
                  lut_args.push_back(acc_bit);
                  THROW_ASSERT(!lut_args.empty() && lut_args.size() <= max_lut_node_inputs,
                               "Unexpected merged KCM LUT arity: " + std::to_string(lut_args.size()));
                  ir_nodeRef op1 = lut_args.size() > 0 ? lut_args[0] : bool_zero;
                  ir_nodeRef op2 = lut_args.size() > 1 ? lut_args[1] : ir_nodeRef();
                  ir_nodeRef op3 = lut_args.size() > 2 ? lut_args[2] : ir_nodeRef();
                  ir_nodeRef op4 = lut_args.size() > 3 ? lut_args[3] : ir_nodeRef();
                  ir_nodeRef op5 = lut_args.size() > 4 ? lut_args[4] : ir_nodeRef();
                  ir_nodeRef op6 = lut_args.size() > 5 ? lut_args[5] : ir_nodeRef();

                  const auto lut_node = ir_man->create_lut_node(boolType, lut_const_node, op1, op2, op3, op4, op5, op6,
                                                                ir_nodeRef(), ir_nodeRef(), loc_info_default);
                  const auto lut_ga =
                      ir_man->CreateAssignStmt(boolType, bool_zero, bool_one, lut_node, function_id, loc_info_default);
                  block->PushBefore(lut_ga, stmt, AppM);
                  s_bit = GetPointer<assign_stmt>(lut_ga)->op0;
               }
            }

            const auto sum_bit = emit_bool_binary(s_bit, carry, xor_node_K);
            const auto not_s_bit = emit_bool_binary(s_bit, bool_one, xor_node_K);
            const auto carry_if_s = emit_bool_binary(s_bit, carry, and_node_K);
            const auto carry_if_not_s = emit_bool_binary(not_s_bit, acc_bit, and_node_K);
            carry = emit_bool_binary(carry_if_s, carry_if_not_s, or_node_K);
            out_bits.push_back(sum_bit);
         }

         acc = pack_word_bits(out_bits);
      }

      if(negative)
      {
         const auto neg_expr = ir_man->create_unary_operation(type_expr, acc, loc_info_default, neg_node_K);
         const auto ga =
             ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), neg_expr, function_id, loc_info_default);
         block->PushBefore(ga, stmt, AppM);
         return GetPointer<assign_stmt>(ga)->op0;
      }

      return acc;
   }

   static ir_nodeRef emitMappedKlutNetworkToIR(const ir_manipulationRef& ir_man, const ir_managerRef& TM,
                                               unsigned int function_id, const application_managerRef& AppM,
                                               const mockturtle::klut_network& mapped_klut, const ir_nodeRef& op0,
                                               unsigned ASSERT_PARAMETER(max_lut_node_inputs), const ir_nodeRef& stmt,
                                               const blocRef& block, const ir_nodeRef& type_expr,
                                               const std::string& loc_info_default)
   {
      const auto boolType = ir_man->GetBooleanType();
      const auto indexType = ir_man->GetUnsignedLongLongType();
      const auto lutConstType = ir_man->GetUnsignedLongLongType();
      const auto bool_zero = TM->CreateUniqueIntegerCst(0, boolType);
      const auto bool_one = TM->CreateUniqueIntegerCst(1, boolType);
      const unsigned wX = static_cast<unsigned>(ir_helper::Size(op0));
      const unsigned wR = static_cast<unsigned>(ir_helper::Size(type_expr));

      std::map<mockturtle::klut_network::node, ir_nodeRef> node_values;
      unsigned pi_index = 0U;
      mapped_klut.foreach_pi([&](const auto& node) {
         ir_nodeRef pi_signal = bool_zero;
         if(pi_index < wX)
         {
            const auto bit_pos = TM->CreateUniqueIntegerCst(static_cast<long long>(pi_index), indexType);
            const auto eb_op = ir_man->create_extract_bit_node(op0, bit_pos, loc_info_default);
            const auto eb_ga =
                ir_man->CreateAssignStmt(boolType, bool_zero, bool_one, eb_op, function_id, loc_info_default);
            block->PushBefore(eb_ga, stmt, AppM);
            pi_signal = GetPointer<assign_stmt>(eb_ga)->op0;
         }
         node_values[node] = pi_signal;
         ++pi_index;
      });

      auto emit_bool_not = [&](const ir_nodeRef& in) -> ir_nodeRef {
         const auto expr = ir_man->create_binary_operation(boolType, in, bool_one, loc_info_default, xor_node_K);
         const auto ga = ir_man->CreateAssignStmt(boolType, bool_zero, bool_one, expr, function_id, loc_info_default);
         block->PushBefore(ga, stmt, AppM);
         return GetPointer<assign_stmt>(ga)->op0;
      };

      auto resolve_signal = [&](mockturtle::klut_network::signal s) -> ir_nodeRef {
         const auto node = mapped_klut.get_node(s);
         if(mapped_klut.is_constant(node))
         {
            bool value = mapped_klut.constant_value(node);
            if(mapped_klut.is_complemented(s))
            {
               value = !value;
            }
            return value ? bool_one : bool_zero;
         }

         const auto it = node_values.find(node);
         THROW_ASSERT(it != node_values.end(), "Mapped KLUT node value not found while emitting KCM merge network");
         auto value = it->second;
         if(mapped_klut.is_complemented(s))
         {
            value = emit_bool_not(value);
         }
         return value;
      };

      mockturtle::topo_view topo{mapped_klut};
      topo.foreach_node([&](const auto& node) {
         if(topo.is_constant(node) || topo.is_pi(node))
         {
            return;
         }

         std::vector<ir_nodeRef> fanins;
         topo.foreach_fanin(node, [&](const auto& fi) { fanins.push_back(resolve_signal(fi)); });
         THROW_ASSERT(
             fanins.size() <= maxLutConstInputs(),
             "Mapped KCM LUT arity exceeds uint64 LUT-constant capacity and cannot be represented as lut_node: " +
                 std::to_string(fanins.size()));
         THROW_ASSERT(fanins.size() <= max_lut_node_inputs,
                      "Mapped KCM LUT arity exceeds max_lut_size: " + std::to_string(fanins.size()) + " > " +
                          std::to_string(max_lut_node_inputs));

         const auto func = topo.node_function(node);
         uint64_t lut_const = 0ULL;
         const uint64_t entries = fanins.empty() ? 1ULL : (1ULL << fanins.size());
         for(uint64_t i = 0; i < entries; ++i)
         {
            if(kitty::get_bit(func, i))
            {
               lut_const |= (1ULL << i);
            }
         }

         if(fanins.empty())
         {
            node_values[node] = (lut_const & 1ULL) ? bool_one : bool_zero;
            return;
         }

         const auto lut_const_node = TM->CreateUniqueIntegerCst(static_cast<long long>(lut_const), lutConstType);
         ir_nodeRef op1 = fanins.size() > 0 ? fanins[0] : ir_nodeRef();
         ir_nodeRef op2 = fanins.size() > 1 ? fanins[1] : ir_nodeRef();
         ir_nodeRef op3 = fanins.size() > 2 ? fanins[2] : ir_nodeRef();
         ir_nodeRef op4 = fanins.size() > 3 ? fanins[3] : ir_nodeRef();
         ir_nodeRef op5 = fanins.size() > 4 ? fanins[4] : ir_nodeRef();
         ir_nodeRef op6 = fanins.size() > 5 ? fanins[5] : ir_nodeRef();

         const auto lut_node = ir_man->create_lut_node(boolType, lut_const_node, op1, op2, op3, op4, op5, op6,
                                                       ir_nodeRef(), ir_nodeRef(), loc_info_default);
         const auto lut_ga =
             ir_man->CreateAssignStmt(boolType, bool_zero, bool_one, lut_node, function_id, loc_info_default);
         block->PushBefore(lut_ga, stmt, AppM);
         node_values[node] = GetPointer<assign_stmt>(lut_ga)->op0;
      });

      std::vector<ir_nodeRef> out_bits;
      out_bits.reserve(wR);
      mapped_klut.foreach_po([&](const auto& s) { out_bits.push_back(resolve_signal(s)); });
      THROW_ASSERT(out_bits.size() <= wR, "Mapped KCM PO width exceeds destination width");
      while(out_bits.size() < wR)
      {
         out_bits.push_back(bool_zero);
      }

      ir_nodeRef result = TM->CreateUniqueIntegerCst(0, type_expr);
      bool has_bits = false;
      for(unsigned bit = 0; bit < wR; ++bit)
      {
         const auto cast_expr = ir_man->create_unary_operation(type_expr, out_bits[bit], loc_info_default, nop_node_K);
         const auto cast_ga =
             ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), cast_expr, function_id, loc_info_default);
         block->PushBefore(cast_ga, stmt, AppM);
         ir_nodeRef term = GetPointer<assign_stmt>(cast_ga)->op0;
         if(bit > 0)
         {
            term = emitShiftedTerm(ir_man, TM, function_id, AppM, term, bit, stmt, block, type_expr, loc_info_default);
         }

         if(!has_bits)
         {
            result = term;
            has_bits = true;
         }
         else
         {
            const auto or_expr = ir_man->create_binary_operation(type_expr, result, term, loc_info_default, or_node_K);
            const auto or_ga =
                ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), or_expr, function_id, loc_info_default);
            block->PushBefore(or_ga, stmt, AppM);
            result = GetPointer<assign_stmt>(or_ga)->op0;
         }
      }
      return has_bits ? result : TM->CreateUniqueIntegerCst(0, type_expr);
   }

   struct MappedKcmMergedCacheEntry
   {
      bool valid = false;
      mockturtle::klut_network klut;
   };

   static const MappedKcmMergedCacheEntry& getMappedKcmMergedNetwork(unsigned wX, unsigned wR, unsigned long long coeff,
                                                                     bool negative, unsigned alpha_eff,
                                                                     unsigned max_lut_size)
   {
      using key_t = std::tuple<unsigned, unsigned, unsigned long long, bool, unsigned, unsigned>;
      static std::map<key_t, MappedKcmMergedCacheEntry> cache;

      const auto key = std::make_tuple(wX, wR, coeff, negative, alpha_eff, max_lut_size);
      const auto it = cache.find(key);
      if(it != cache.end())
      {
         return it->second;
      }

      MappedKcmMergedCacheEntry entry;
      try
      {
         const auto klut = buildKcmKlutNetworkMerged(wX, wR, coeff, negative, alpha_eff);
         entry.valid = mapKlutToCollapsedNetwork(klut, max_lut_size, entry.klut);
      }
      catch(...)
      {
         entry.valid = false;
      }

      const auto inserted = cache.emplace(key, std::move(entry));
      return inserted.first->second;
   }

   static ir_nodeRef emitMulConstOpsKcmMergedMapped(const ir_manipulationRef& ir_man, const ir_managerRef& TM,
                                                    unsigned int function_id, const application_managerRef& AppM,
                                                    const ir_nodeRef& op0, unsigned long long coeff, bool negative,
                                                    unsigned alpha_eff, unsigned max_lut_node_inputs,
                                                    const ir_nodeRef& stmt, const blocRef& block,
                                                    const ir_nodeRef& type_expr, const std::string& loc_info_default)
   {
      const unsigned wX = static_cast<unsigned>(ir_helper::Size(op0));
      const unsigned wR = static_cast<unsigned>(ir_helper::Size(type_expr));
      if(alpha_eff == 0 || wR == 0 || max_lut_node_inputs == 0)
      {
         return ir_nodeRef();
      }
      if(alpha_eff + 1U > max_lut_node_inputs)
      {
         return ir_nodeRef();
      }

      const unsigned mapping_lut_size = std::min(max_lut_node_inputs, maxLutConstInputs());
      const auto& mapped = getMappedKcmMergedNetwork(wX, wR, coeff, negative, alpha_eff, mapping_lut_size);
      if(!mapped.valid)
      {
         return ir_nodeRef();
      }
      return emitMappedKlutNetworkToIR(ir_man, TM, function_id, AppM, mapped.klut, op0, max_lut_node_inputs, stmt,
                                       block, type_expr, loc_info_default);
   }

   static ir_nodeRef emitMulConstOpsNAF_Balanced(const ir_manipulationRef& ir_man, const ir_managerRef& TM,
                                                 unsigned int function_id, const application_managerRef& AppM,
                                                 const ir_nodeRef& op0, const std::vector<MCShiftAddOp>& ops,
                                                 const ir_nodeRef& stmt, const blocRef& block,
                                                 const ir_nodeRef& type_expr, const std::string& loc_info_default)
   {
      std::vector<ir_nodeRef> pos_terms;
      std::vector<ir_nodeRef> neg_terms;
      pos_terms.reserve(ops.size());
      neg_terms.reserve(ops.size());

      for(const auto& o : ops)
      {
         const auto term =
             emitShiftedTerm(ir_man, TM, function_id, AppM, op0, o.shamt, stmt, block, type_expr, loc_info_default);
         if(o.isAdd)
         {
            pos_terms.push_back(term);
         }
         else
         {
            neg_terms.push_back(term);
         }
      }

      if(pos_terms.empty() && neg_terms.empty())
      {
         return TM->CreateUniqueIntegerCst(0, type_expr);
      }

      ir_nodeRef pos_sum;
      ir_nodeRef neg_sum;
      if(!pos_terms.empty())
      {
         pos_sum = emitBalancedAddTree(ir_man, function_id, AppM, pos_terms, stmt, block, type_expr, loc_info_default);
      }
      if(!neg_terms.empty())
      {
         neg_sum = emitBalancedAddTree(ir_man, function_id, AppM, neg_terms, stmt, block, type_expr, loc_info_default);
      }

      if(neg_terms.empty())
      {
         return pos_sum;
      }

      if(pos_terms.empty())
      {
         const auto neg_expr = ir_man->create_unary_operation(type_expr, neg_sum, loc_info_default, neg_node_K);
         const auto ga =
             ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), neg_expr, function_id, loc_info_default);
         block->PushBefore(ga, stmt, AppM);
         return GetPointer<assign_stmt>(ga)->op0;
      }

      const auto sub_expr = ir_man->create_binary_operation(type_expr, pos_sum, neg_sum, loc_info_default, sub_node_K);
      const auto ga =
          ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), sub_expr, function_id, loc_info_default);
      block->PushBefore(ga, stmt, AppM);
      return GetPointer<assign_stmt>(ga)->op0;
   }
} // namespace

ir_nodeRef IR_lowering::decomposeMultiplicationByConstant(const ir_nodeRef& op0, const constant_int_val_node* ic_node,
                                                          const ir_nodeRef& old_target, const ir_nodeRef& stmt,
                                                          const blocRef& block, const ir_nodeRef& type_expr,
                                                          const std::string& loc_info_default)
{
   if(!AppM->ApplyNewTransformation())
   {
      return old_target;
   }

   long long int ext_op1 = static_cast<long long>(ir_helper::get_integer_cst_value(ic_node));
   const unsigned type_size = static_cast<unsigned>(ir_helper::Size(type_expr));
   const bool type_is_signed = ir_helper::IsSignedIntegerType(type_expr);
   const std::string type_info_embed =
       " [" + std::to_string(type_size) + " bits, " + (type_is_signed ? "signed" : "unsigned") + "]";
   if(type_size < 64)
   {
      ext_op1 <<= 64 - type_size;
      ext_op1 >>= 64 - type_size;
   }

   const HLS_deviceRef HLS_D = getHlsDeviceFromApp(AppM);
   initConstDivMulLoweringParams();
   if(!constmul_enable)
   {
      return old_target;
   }

   if(ext_op1 == 0)
   {
      return TM->CreateUniqueIntegerCst(0, type_expr);
   }
   if(ext_op1 == 1)
   {
      return op0;
   }
   if(ext_op1 == -1)
   {
      return ir_man->create_unary_operation(type_expr, op0, loc_info_default, neg_node_K);
   }

   const bool negative = (ext_op1 < 0);
   const auto coeff = static_cast<unsigned long long>(negative ? -ext_op1 : ext_op1);
   const unsigned wX = static_cast<unsigned>(ir_helper::Size(op0));
   const unsigned wC = bitWidthUnsigned(coeff);

   if(isPower2(coeff))
   {
      const auto l_shift = floor_log2(coeff);
      const auto l_shift_node = TM->CreateUniqueIntegerCst(static_cast<long long>(l_shift), type_expr);
      const auto sh_expr = createShiftOperationWithCostGuard(ir_man, type_expr, op0, l_shift_node, loc_info_default,
                                                             shl_node_K, "decomposeMultiplicationByConstant");

      if(!negative)
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                        "---constmul: coeff=" + std::to_string(ext_op1) + type_info_embed + " -> shift by " +
                            std::to_string(l_shift));
         return sh_expr;
      }

      const auto sh_ga =
          ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), sh_expr, function_id, loc_info_default);
      block->PushBefore(sh_ga, stmt, AppM);
      const auto sh_var = GetPointer<assign_stmt>(sh_ga)->op0;
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "---constmul: coeff=" + std::to_string(ext_op1) + type_info_embed + " -> shift by " +
                         std::to_string(l_shift) + " and negate");
      return ir_man->create_unary_operation(type_expr, sh_var, loc_info_default, neg_node_K);
   }

   const auto decision_metric = getEffectiveConstMultDivDecisionMetric();
   const double weight_fanout = 0.0;

   double add_delay = 1.0;
   double sub_delay = 1.0;
   double mult_delay = 3.0;
   double shift_delay = 1.0;
   double add_area = 1.0;
   double mult_area = 1.0;
   double mult_lut_area = 1.0;
   double mult_dsp_count = 0.0;
   double shift_area = 1.0;
   getAddSubMultCosts(HLS_D, wX, add_delay, sub_delay, mult_delay, shift_delay, add_area, mult_area, mult_lut_area,
                      mult_dsp_count, shift_area, HLS_D != nullptr);

   const std::string kcm_sum_strategy =
       constmul_kcm_merge_table_add ? "rake" : (constmul_kcm_sum_strategy == "rake" ? "rake" : "tree");
   const std::string kcm_cost_model = constmul_kcm_cost_model == "mockturtle" ? "mockturtle" : "heuristic";
   const unsigned max_lut_node_inputs = std::min(resolveRequiredMaxLutSize(AppM, HLS_D), maxLutConstInputs());
   KcmEstimate kcm_est{0.0, 0.0, 0U, 0U, false};
   double kcm_score = std::numeric_limits<double>::infinity();
   if(constmul_kcm_enable)
   {
      unsigned alpha_eff = std::min(constmul_kcm_alpha, max_lut_node_inputs);
      if(constmul_kcm_merge_table_add && alpha_eff > 1)
      {
         alpha_eff -= 1;
      }
      if(alpha_eff >= 2)
      {
         double kcm_lut_delay = add_delay;
         double kcm_lut_area = 1.0;
         if(!getLutCost(HLS_D, alpha_eff, max_lut_node_inputs, kcm_lut_delay, kcm_lut_area))
         {
            kcm_lut_delay = add_delay;
            kcm_lut_area = 1.0;
         }
         if(kcm_cost_model == "mockturtle")
         {
            double base_lut_delay = kcm_lut_delay;
            double base_lut_area = kcm_lut_area;
            if(!getLutCost(HLS_D, max_lut_node_inputs, max_lut_node_inputs, base_lut_delay, base_lut_area))
            {
               base_lut_delay = kcm_lut_delay;
               base_lut_area = kcm_lut_area;
            }
            kcm_est = estimateKcmCostMockturtle(wX, type_size, coeff, negative, alpha_eff, max_lut_node_inputs,
                                                constmul_kcm_merge_table_add, kcm_sum_strategy, base_lut_delay,
                                                base_lut_area);
         }
         if(!kcm_est.valid)
         {
            kcm_est = estimateKcmCost(*this, HLS_D, wX, wC, type_size, coeff, alpha_eff, max_lut_node_inputs,
                                      kcm_sum_strategy, constmul_kcm_merge_table_add, add_delay, add_area,
                                      kcm_lut_delay, kcm_lut_area);
         }
         if(kcm_est.valid)
         {
            kcm_score = getConstMultDivScore(kcm_est.delay, kcm_est.area);
         }
      }
   }

   std::vector<std::vector<MCShiftAddOp>> candidates;
   auto add_candidate = [&candidates](std::vector<MCShiftAddOp> ops) {
      if(ops.empty())
      {
         return;
      }
      std::sort(ops.begin(), ops.end(), [](const MCShiftAddOp& a, const MCShiftAddOp& b) { return a.shamt < b.shamt; });
      candidates.push_back(std::move(ops));
   };

   if(__builtin_popcountll(coeff) == 2)
   {
      std::vector<MCShiftAddOp> ops;
      unsigned long long temp = coeff;
      while(temp)
      {
         const auto bit = static_cast<unsigned>(__builtin_ctzll(temp));
         ops.push_back(MCShiftAddOp{true, bit});
         temp &= (temp - 1);
      }
      add_candidate(std::move(ops));
   }

   const unsigned long long lowbit = coeff & (~coeff + 1ULL);
   if(isPower2(coeff + lowbit))
   {
      const auto a = static_cast<unsigned>(floor_log2(coeff + lowbit));
      const auto b = static_cast<unsigned>(floor_log2(lowbit));
      add_candidate({MCShiftAddOp{true, a}, MCShiftAddOp{false, b}});
   }
   if(coeff > lowbit && isPower2(coeff - lowbit))
   {
      const auto a = static_cast<unsigned>(floor_log2(coeff - lowbit));
      const auto b = static_cast<unsigned>(floor_log2(lowbit));
      add_candidate({MCShiftAddOp{true, a}, MCShiftAddOp{true, b}});
   }

   if(coeff > 1 && isPower2(coeff - 1))
   {
      const auto a = static_cast<unsigned>(floor_log2(coeff - 1));
      add_candidate({MCShiftAddOp{true, a}, MCShiftAddOp{true, 0}});
   }
   if(isPower2(coeff + 1))
   {
      const auto a = static_cast<unsigned>(floor_log2(coeff + 1));
      add_candidate({MCShiftAddOp{true, a}, MCShiftAddOp{false, 0}});
   }

   if(constmul_try_factor_forms)
   {
      const auto max_m = std::min<unsigned>(static_cast<unsigned>(type_size - 1), 12U);
      for(unsigned m = 2; m <= max_m; ++m)
      {
         const unsigned long long base = 1ULL << m;
         const unsigned long long factor_plus = base + 1ULL;
         if(factor_plus != 0 && coeff % factor_plus == 0)
         {
            const auto q = coeff / factor_plus;
            std::vector<MCShiftAddOp> q_ops;
            computeNAF(q, type_size, q_ops);
            std::vector<MCShiftAddOp> ops;
            appendShiftedOps(q_ops, m, false, ops);
            appendShiftedOps(q_ops, 0, false, ops);
            add_candidate(std::move(ops));
         }

         const unsigned long long factor_minus = base - 1ULL;
         if(factor_minus != 0 && coeff % factor_minus == 0)
         {
            const auto q = coeff / factor_minus;
            std::vector<MCShiftAddOp> q_ops;
            computeNAF(q, type_size, q_ops);
            std::vector<MCShiftAddOp> ops;
            appendShiftedOps(q_ops, m, false, ops);
            appendShiftedOps(q_ops, 0, true, ops);
            add_candidate(std::move(ops));
         }
      }
   }

   if(constmul_enable_small_factor_chains)
   {
      const unsigned long long factors[] = {3ULL, 5ULL, 9ULL};
      auto factor_shift = [](unsigned long long factor) -> unsigned {
         if(factor == 3ULL)
         {
            return 1;
         }
         if(factor == 5ULL)
         {
            return 2;
         }
         if(factor == 9ULL)
         {
            return 3;
         }
         return 0;
      };
      auto apply_factor = [&](const std::vector<MCShiftAddOp>& base, unsigned long long factor,
                              std::vector<MCShiftAddOp>& out) {
         const unsigned shift = factor_shift(factor);
         if(shift == 0)
         {
            return;
         }
         appendShiftedOps(base, shift, false, out);
         appendShiftedOps(base, 0, false, out);
      };

      for(const auto f1 : factors)
      {
         if(coeff % f1 != 0)
         {
            continue;
         }
         const auto q1 = coeff / f1;
         std::vector<MCShiftAddOp> q1_ops;
         computeNAF(q1, type_size, q1_ops);
         std::vector<MCShiftAddOp> ops_single;
         apply_factor(q1_ops, f1, ops_single);
         add_candidate(std::move(ops_single));

         for(const auto f2 : factors)
         {
            if(q1 % f2 != 0)
            {
               continue;
            }
            const auto q2 = q1 / f2;
            std::vector<MCShiftAddOp> q2_ops;
            computeNAF(q2, type_size, q2_ops);
            std::vector<MCShiftAddOp> ops_mid;
            apply_factor(q2_ops, f2, ops_mid);
            std::vector<MCShiftAddOp> ops_chain;
            apply_factor(ops_mid, f1, ops_chain);
            add_candidate(std::move(ops_chain));
         }
      }
   }

   std::vector<MCShiftAddOp> naf_ops;
   computeNAF(coeff, type_size, naf_ops);
   add_candidate(std::move(naf_ops));

   double best_score = std::numeric_limits<double>::infinity();
   double best_proxy = std::numeric_limits<double>::infinity();
   size_t best_index = 0;
   bool best_use_balanced = false;
   bool found = false;

   for(size_t idx = 0; idx < candidates.size(); ++idx)
   {
      const auto& ops = candidates[idx];
      const auto n_terms = ops.size();
      if(n_terms == 0 || n_terms > constmul_max_terms)
      {
         continue;
      }
      size_t npos = 0;
      for(const auto& o : ops)
      {
         npos += o.isAdd ? 1 : 0;
      }
      const size_t nneg = n_terms - npos;
      const bool use_balanced = constmul_balance_tree && n_terms >= constmul_balance_tree_min_terms;
      const unsigned depth_pos =
          npos == 0 ? 0U : static_cast<unsigned>(ceil_log2(static_cast<unsigned long long>(npos)));
      const unsigned depth_neg =
          nneg == 0 ? 0U : static_cast<unsigned>(ceil_log2(static_cast<unsigned long long>(nneg)));
      const unsigned chain_depth_pos = npos == 0 ? 0U : static_cast<unsigned>(npos - 1);
      const unsigned chain_depth_neg = nneg == 0 ? 0U : static_cast<unsigned>(nneg - 1);
      const unsigned eff_depth_pos = use_balanced ? depth_pos : chain_depth_pos;
      const unsigned eff_depth_neg = use_balanced ? depth_neg : chain_depth_neg;
      const unsigned depth = std::max(eff_depth_pos, eff_depth_neg) + ((npos > 0 && nneg > 0) ? 1U : 0U);
      if(depth > constmul_max_depth)
      {
         continue;
      }

      const unsigned n_add =
          (npos > 0 ? static_cast<unsigned>(npos - 1) : 0U) + (nneg > 0 ? static_cast<unsigned>(nneg - 1) : 0U);
      const unsigned n_sub = (npos > 0 && nneg > 0) ? 1U : 0U;
      const double latency_seq =
          static_cast<double>(eff_depth_pos + eff_depth_neg) * add_delay + (n_sub ? sub_delay : 0.0);
      const double area_seq = static_cast<double>(n_add + n_sub) * add_area;
      const double proxy_seq = static_cast<double>(n_add + n_sub);
      const double fanout_seq = static_cast<double>(n_terms);
      const double score_seq = getConstMultDivScore(latency_seq, area_seq) + weight_fanout * fanout_seq;

      if(better_multdiv_candidate(decision_metric, score_seq, proxy_seq, best_score, best_proxy))
      {
         best_score = score_seq;
         best_proxy = proxy_seq;
         best_index = idx;
         best_use_balanced = use_balanced;
         found = true;
      }
   }

   const double mul_area_scaled =
       area_estimation::get_lut_equivalent_area_weighted(HLS_D, mult_lut_area, mult_dsp_count, constmul_dsp_scale_k);
   const double score_mul = getConstMultDivScore(mult_delay, mul_area_scaled) + weight_fanout;
   const double proxy_kcm = kcm_est.valid ? kcm_est.area : std::numeric_limits<double>::infinity();
   double best_candidate_score = std::numeric_limits<double>::infinity();
   if(found)
   {
      best_candidate_score = std::min(best_candidate_score, best_score);
   }
   if(kcm_est.valid)
   {
      best_candidate_score = std::min(best_candidate_score, kcm_score);
   }

   enum class ConstMulChoice
   {
      Mul,
      ShiftAdd,
      Kcm
   };

   ConstMulChoice choice = ConstMulChoice::Mul;
   double choice_score = score_mul;
   double choice_proxy = mul_area_scaled;
   if(found && better_multdiv_candidate(decision_metric, best_score, best_proxy, choice_score, choice_proxy))
   {
      choice_score = best_score;
      choice_proxy = best_proxy;
      choice = ConstMulChoice::ShiftAdd;
   }
   if(kcm_est.valid && better_multdiv_candidate(decision_metric, kcm_score, proxy_kcm, choice_score, choice_proxy))
   {
      choice_score = kcm_score;
      choice_proxy = proxy_kcm;
      choice = ConstMulChoice::Kcm;
   }

   if(choice == ConstMulChoice::Mul)
   {
      if(best_candidate_score == std::numeric_limits<double>::infinity())
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                        "---constmul: coeff=" + std::to_string(ext_op1) + type_info_embed +
                            " -> keep mul (no candidate)");
      }
      else
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                        "---constmul: coeff=" + std::to_string(ext_op1) + type_info_embed + " -> keep mul (score " +
                            std::to_string(best_candidate_score) + " >= mul " + std::to_string(score_mul) +
                            ", metric=" + decision_metric + ", mul_area_lut_eq=" + std::to_string(mult_area) +
                            ", mul_area_scaled=" + std::to_string(mul_area_scaled) + ")");
      }
      return old_target;
   }

   if(choice == ConstMulChoice::Kcm)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "---constmul: coeff=" + std::to_string(ext_op1) + type_info_embed + " -> kcm (model " +
                         kcm_cost_model + ", alpha " + std::to_string(kcm_est.alpha_eff) + ", q " +
                         std::to_string(kcm_est.q) + ", merged " + (constmul_kcm_merge_table_add ? "yes" : "no") +
                         ", score " + std::to_string(kcm_score) + ", proxy " + std::to_string(proxy_kcm) +
                         ", metric=" + decision_metric + ")");
      if(constmul_kcm_merge_table_add)
      {
         const auto mapped_kcm =
             emitMulConstOpsKcmMergedMapped(ir_man, TM, function_id, AppM, op0, coeff, negative, kcm_est.alpha_eff,
                                            max_lut_node_inputs, stmt, block, type_expr, loc_info_default);
         if(mapped_kcm)
         {
            return mapped_kcm;
         }
         return emitMulConstOpsKcmMerged(ir_man, TM, function_id, AppM, op0, coeff, negative, kcm_est.alpha_eff,
                                         max_lut_node_inputs, kcm_sum_strategy, stmt, block, type_expr,
                                         loc_info_default);
      }
      return emitMulConstOpsKcm(ir_man, TM, function_id, AppM, op0, coeff, negative, kcm_est.alpha_eff,
                                max_lut_node_inputs, kcm_sum_strategy, stmt, block, type_expr, loc_info_default);
   }

   auto best_ops = candidates[best_index];
   if(negative)
   {
      for(auto& o : best_ops)
      {
         o.isAdd = !o.isAdd;
      }
   }

   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  "---constmul: coeff=" + std::to_string(ext_op1) + type_info_embed + " -> expand (terms " +
                      std::to_string(best_ops.size()) + ", balanced " + std::string(best_use_balanced ? "yes" : "no") +
                      ", score " + std::to_string(best_score) + ", proxy " + std::to_string(best_proxy) +
                      ", metric=" + decision_metric + ")");
   if(best_use_balanced)
   {
      return emitMulConstOpsNAF_Balanced(ir_man, TM, function_id, AppM, op0, best_ops, stmt, block, type_expr,
                                         loc_info_default);
   }

   return emitMulConstOpsNAF(ir_man, TM, function_id, AppM, op0, best_ops, stmt, block, type_expr, loc_info_default);
}

ir_nodeRef IR_lowering::expand_mult_highpart(const ir_nodeRef& op0, unsigned long long int ml,
                                             const ir_nodeRef& type_expr, int data_bitsize,
                                             const std::list<ir_nodeRef>::const_iterator it_los, const blocRef& block,
                                             const std::string& loc_info_default)
{
   /**
    long long int u0, v0, u1, v1, u0v0, u0v0h, u1v0, u0v0hu1v0, u0v1, u0v0hu1v0u0v1, u0v0hu1v0u0v1h, u1v1;
    u0 = u & ((1LL<<32)-1);
    u1 = u >>32;
    v0 = v & ((1LL<<32)-1);
    v1 = v >>32;
    u0v0 = u0 * v0;
    u0v0h = u0v0>>32;
    u0v0hU = u0v0h & ((1LL<<32)-1);///only for signed computation
    u1v0 = u1 * v0;
    u0v0hu1v0 = u0v0hU + u1v0;
    w1 = u0v0hu1v0 & ((1LL<<32)-1);
    w2 = u0v0hu1v0 >>32;
    u0v1 = u0 * v1;
    w1u0v1 = w1+u0v1;
    w1u0v1h = w1u0v1 >> 32;
    u1v1 = u1 * v1;
    w1u0v1hw2 = w1u0v1h + w2;
    return w1u0v1hw2 + u1v1;
   */
   int half_data_bitsize = data_bitsize / 2;
   ir_nodeRef mask_node =
       TM->CreateUniqueIntegerCst(static_cast<long long int>((1LL << half_data_bitsize) - 1), type_expr);
   ir_nodeRef half_data_bitsize_node =
       TM->CreateUniqueIntegerCst(static_cast<long long int>(half_data_bitsize), type_expr);

   ir_nodeRef u0_expr = ir_man->create_binary_operation(type_expr, op0, mask_node, loc_info_default, and_node_K);
   ir_nodeRef u0_ga =
       ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), u0_expr, function_id, loc_info_default);
   block->PushBefore(u0_ga, *it_los, AppM);
   ir_nodeRef u0_ga_var = GetPointer<assign_stmt>(u0_ga)->op0;

   ir_nodeRef u1_expr = createShiftOperationWithCostGuard(ir_man, type_expr, op0, half_data_bitsize_node,
                                                          loc_info_default, shr_node_K, "expand_mult_highpart");
   ir_nodeRef u1_ga =
       ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), u1_expr, function_id, loc_info_default);
   block->PushBefore(u1_ga, *it_los, AppM);
   ir_nodeRef u1_ga_var = GetPointer<assign_stmt>(u1_ga)->op0;

   long long int v0 = static_cast<long long int>(ml) & ((1LL << half_data_bitsize) - 1);
   long long int v1;
   bool unsignedp = ir_helper::IsUnsignedIntegerType(type_expr);
   if(unsignedp)
   {
      v1 = static_cast<long long int>(ml >> half_data_bitsize);
   }
   else
   {
      v1 = static_cast<long long int>(ml) >> half_data_bitsize;
   }

   ir_nodeRef u0v0_ga_var;
   ir_nodeRef v0_node;
   ir_nodeRef v1_node;
   if(v0 != 0)
   {
      if(v0 == 1)
      {
         u0v0_ga_var = u0_ga_var;
      }
      else
      {
         v0_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(v0), type_expr);
         ir_nodeRef u0v0_expr =
             ir_man->create_binary_operation(type_expr, u0_ga_var, v0_node, loc_info_default, mul_node_K);
         ir_nodeRef u0v0_ga =
             ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), u0v0_expr, function_id, loc_info_default);
         block->PushBefore(u0v0_ga, *it_los, AppM);
         u0v0_ga_var = GetPointer<assign_stmt>(u0v0_ga)->op0;
      }
   }
   ir_nodeRef u0v0h_ga_var;
   if(u0v0_ga_var)
   {
      ir_nodeRef u0v0h_expr = createShiftOperationWithCostGuard(ir_man, type_expr, u0v0_ga_var, half_data_bitsize_node,
                                                                loc_info_default, shr_node_K, "expand_mult_highpart");
      ir_nodeRef u0v0h_ga =
          ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), u0v0h_expr, function_id, loc_info_default);
      block->PushBefore(u0v0h_ga, *it_los, AppM);
      u0v0h_ga_var = GetPointer<assign_stmt>(u0v0h_ga)->op0;
   }
   ir_nodeRef u0v0hU_ga_var;
   if(u0v0h_ga_var && !unsignedp)
   {
      ir_nodeRef u0v0hU_expr =
          ir_man->create_binary_operation(type_expr, u0v0h_ga_var, mask_node, loc_info_default, and_node_K);
      ir_nodeRef u0v0hU_ga =
          ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), u0v0hU_expr, function_id, loc_info_default);
      block->PushBefore(u0v0hU_ga, *it_los, AppM);
      u0v0hU_ga_var = GetPointer<assign_stmt>(u0v0hU_ga)->op0;
   }
   else
   {
      u0v0hU_ga_var = u0v0h_ga_var;
   }
   ir_nodeRef u1v0_ga_var;
   if(v0 != 0)
   {
      if(v0 == 1)
      {
         u1v0_ga_var = u1_ga_var;
      }
      else
      {
         ir_nodeRef u1v0_expr =
             ir_man->create_binary_operation(type_expr, u1_ga_var, v0_node, loc_info_default, mul_node_K);
         ir_nodeRef u1v0_ga =
             ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), u1v0_expr, function_id, loc_info_default);
         block->PushBefore(u1v0_ga, *it_los, AppM);
         u1v0_ga_var = GetPointer<assign_stmt>(u1v0_ga)->op0;
      }
   }
   ir_nodeRef u0v0hu1v0_ga_var;
   if(u0v0hU_ga_var)
   {
      THROW_ASSERT(u1v0_ga_var, "unexpected condition");
      ir_nodeRef u0v0hu1v0_expr =
          ir_man->create_binary_operation(type_expr, u0v0hU_ga_var, u1v0_ga_var, loc_info_default, add_node_K);
      ir_nodeRef u0v0hu1v0_ga = ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), u0v0hu1v0_expr,
                                                         function_id, loc_info_default);
      block->PushBefore(u0v0hu1v0_ga, *it_los, AppM);
      u0v0hu1v0_ga_var = GetPointer<assign_stmt>(u0v0hu1v0_ga)->op0;
   }
   ir_nodeRef w1_ga_var;
   if(u0v0hu1v0_ga_var)
   {
      ir_nodeRef w1_expr =
          ir_man->create_binary_operation(type_expr, u0v0hu1v0_ga_var, mask_node, loc_info_default, and_node_K);
      ir_nodeRef w1_ga =
          ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), w1_expr, function_id, loc_info_default);
      block->PushBefore(w1_ga, *it_los, AppM);
      w1_ga_var = GetPointer<assign_stmt>(w1_ga)->op0;
   }

   ir_nodeRef w2_ga_var;
   if(u0v0hu1v0_ga_var)
   {
      ir_nodeRef w2_expr =
          createShiftOperationWithCostGuard(ir_man, type_expr, u0v0hu1v0_ga_var, half_data_bitsize_node,
                                            loc_info_default, shr_node_K, "expand_mult_highpart");
      ir_nodeRef w2_ga =
          ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), w2_expr, function_id, loc_info_default);
      block->PushBefore(w2_ga, *it_los, AppM);
      w2_ga_var = GetPointer<assign_stmt>(w2_ga)->op0;
   }

   ir_nodeRef u0v1_ga_var;
   if(v1 != 0)
   {
      if(v1 == 1)
      {
         u0v1_ga_var = u0_ga_var;
      }
      else
      {
         v1_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(v1), type_expr);
         ir_nodeRef u0v1_expr =
             ir_man->create_binary_operation(type_expr, u0_ga_var, v1_node, loc_info_default, mul_node_K);
         ir_nodeRef u0v1_ga =
             ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), u0v1_expr, function_id, loc_info_default);
         block->PushBefore(u0v1_ga, *it_los, AppM);
         u0v1_ga_var = GetPointer<assign_stmt>(u0v1_ga)->op0;
      }
   }
   ir_nodeRef w1u0v1_ga_var;
   if(w1_ga_var)
   {
      if(u0v1_ga_var)
      {
         ir_nodeRef w1u0v1_expr =
             ir_man->create_binary_operation(type_expr, w1_ga_var, u0v1_ga_var, loc_info_default, add_node_K);
         ir_nodeRef w1u0v1_ga = ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), w1u0v1_expr,
                                                         function_id, loc_info_default);
         block->PushBefore(w1u0v1_ga, *it_los, AppM);
         w1u0v1_ga_var = GetPointer<assign_stmt>(w1u0v1_ga)->op0;
      }
      else
      {
         w1u0v1_ga_var = w1_ga_var;
      }
   }
   else if(u0v1_ga_var)
   {
      w1u0v1_ga_var = u0v1_ga_var;
   }

   ir_nodeRef w1u0v1h_ga_var;
   if(w1u0v1_ga_var)
   {
      ir_nodeRef w1u0v1h_expr =
          createShiftOperationWithCostGuard(ir_man, type_expr, w1u0v1_ga_var, half_data_bitsize_node, loc_info_default,
                                            shr_node_K, "expand_mult_highpart");
      ir_nodeRef w1u0v1h_ga =
          ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), w1u0v1h_expr, function_id, loc_info_default);
      block->PushBefore(w1u0v1h_ga, *it_los, AppM);
      w1u0v1h_ga_var = GetPointer<assign_stmt>(w1u0v1h_ga)->op0;
   }
   ir_nodeRef u1v1_ga_var;
   if(v1 != 0)
   {
      if(v1 == 1)
      {
         u1v1_ga_var = u1_ga_var;
      }
      else
      {
         ir_nodeRef u1v1_expr =
             ir_man->create_binary_operation(type_expr, u1_ga_var, v1_node, loc_info_default, mul_node_K);
         ir_nodeRef u1v1_ga =
             ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), u1v1_expr, function_id, loc_info_default);
         block->PushBefore(u1v1_ga, *it_los, AppM);
         u1v1_ga_var = GetPointer<assign_stmt>(u1v1_ga)->op0;
      }
   }
   ir_nodeRef w1u0v1hw2_ga_var;
   if(w1u0v1h_ga_var)
   {
      if(w2_ga_var)
      {
         ir_nodeRef w1u0v1hw2_expr =
             ir_man->create_binary_operation(type_expr, w1u0v1h_ga_var, w2_ga_var, loc_info_default, add_node_K);
         ir_nodeRef w1u0v1hw2_ga = ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), w1u0v1hw2_expr,
                                                            function_id, loc_info_default);
         block->PushBefore(w1u0v1hw2_ga, *it_los, AppM);
         w1u0v1hw2_ga_var = GetPointer<assign_stmt>(w1u0v1hw2_ga)->op0;
      }
      else
      {
         w1u0v1hw2_ga_var = w1u0v1h_ga_var;
      }
   }
   else if(w2_ga_var)
   {
      w1u0v1hw2_ga_var = w2_ga_var;
   }

   ir_nodeRef res_ga_var;
   if(w1u0v1hw2_ga_var)
   {
      if(u1v1_ga_var)
      {
         ir_nodeRef res_expr =
             ir_man->create_binary_operation(type_expr, w1u0v1hw2_ga_var, u1v1_ga_var, loc_info_default, add_node_K);
         ir_nodeRef res_ga =
             ir_man->CreateAssignStmt(type_expr, ir_nodeRef(), ir_nodeRef(), res_expr, function_id, loc_info_default);
         block->PushBefore(res_ga, *it_los, AppM);
         res_ga_var = GetPointer<assign_stmt>(res_ga)->op0;
      }
      else
      {
         res_ga_var = w1u0v1hw2_ga_var;
      }
   }
   else if(u1v1_ga_var)
   {
      res_ga_var = u1v1_ga_var;
   }
   else
   {
      res_ga_var = TM->CreateUniqueIntegerCst(static_cast<long long int>(0), type_expr);
   }
   return res_ga_var;
}

namespace
{
   using constdiv_magic::computeSDivMagic64;
   using constdiv_magic::computeUDivMagic64;
   using constdiv_magic::maskForWidth;
   using constdiv_magic::SDivMagic64;
   using constdiv_magic::UDivMagic64;
   using constdiv_magic::uint128_t;

   // Upper bound for truth-table input size when using mockturtle-based LUT estimation.
   // This keeps runtime bounded while allowing evaluation beyond device max_lut_size.
   static constexpr unsigned CONSTDIV_MOCKTURTLE_MAX_TT_INPUTS = 16U;

   static bool is_mockturtle_lut_cost_model(const std::string& lut_cost_model)
   {
      return lut_cost_model == "mockturtle" || lut_cost_model == "mockturtle_full";
   }

   static bool is_auto_lut_cost_model(const std::string& lut_cost_model)
   {
      return lut_cost_model == "auto";
   }

   static std::string select_lut_cost_model(const std::string& lut_cost_model, unsigned in_bits, unsigned max_lut_size)
   {
      if(!is_auto_lut_cost_model(lut_cost_model))
      {
         return lut_cost_model;
      }
      return in_bits <= max_lut_size ? "analytic" : "mockturtle";
   }

   static bool is_mockturtle_full_lut_cost_model(const std::string& lut_cost_model)
   {
      return lut_cost_model == "mockturtle_full";
   }

   static unsigned estimateKnownLeadingZeros(const ir_nodeRef& op0, unsigned width)
   {
      if(width == 0 || width > 64)
      {
         return 0;
      }

      if(GetPointer<constant_int_val_node>(op0))
      {
         const uint64_t value = static_cast<uint64_t>(ir_helper::GetConstValue(op0)) & maskForWidth(width);
         if(value == 0)
         {
            return width;
         }
         const unsigned clz = static_cast<unsigned>(__builtin_clzll(value));
         const unsigned adjust = 64 - width;
         return clz >= adjust ? clz - adjust : 0;
      }

      if(const auto* ssa = GetPointer<ssa_node>(op0))
      {
         const auto def_stmt = ssa->GetDefStmt();
         if(def_stmt && def_stmt->get_kind() == assign_stmt_K)
         {
            const auto* ga = GetPointer<assign_stmt>(def_stmt);
            const auto* ue = GetPointer<unary_node>(ga->op1);
            if(ue && (ga->op1->get_kind() == nop_node_K || ga->op1->get_kind() == bitcast_node_K))
            {
               const auto src_type = ir_helper::CGetType(ue->op);
               const auto dst_type = ir_helper::CGetType(ga->op0);
               if(ir_helper::IsUnsignedIntegerType(src_type) && ir_helper::IsUnsignedIntegerType(dst_type))
               {
                  const auto src_bits = static_cast<unsigned>(ir_helper::Size(src_type));
                  const auto dst_bits = static_cast<unsigned>(ir_helper::Size(dst_type));
                  if(dst_bits == width && dst_bits >= src_bits)
                  {
                     return dst_bits - src_bits;
                  }
               }
            }
         }
      }

      // TODO: derive leading-zero information from bit-level analysis / range info.
      return 0;
   }

   struct LinArchStepNetwork
   {
      unsigned long long d;
      unsigned k;
      unsigned t;
      unsigned in_bits;
      unsigned out_bits;
      mockturtle::aig_network aig;
   };

   static mockturtle::aig_network buildAigFromTruthTables(unsigned in_bits,
                                                          const std::vector<kitty::dynamic_truth_table>& outputs)
   {
      mockturtle::klut_network klut;
      std::vector<mockturtle::klut_network::signal> pis;
      pis.reserve(in_bits);
      for(unsigned i = 0; i < in_bits; ++i)
      {
         pis.push_back(klut.create_pi());
      }
      for(const auto& output : outputs)
      {
         const auto node = klut.create_node(pis, output);
         klut.create_po(node);
      }

      mockturtle::shannon_resynthesis<mockturtle::aig_network> fallback;
      mockturtle::dsd_resynthesis<mockturtle::aig_network, decltype(fallback)> aig_resyn(fallback);
      auto aig = mockturtle::node_resynthesis<mockturtle::aig_network>(klut, aig_resyn);
      return mockturtle::cleanup_dangling(aig);
   }

   static const LinArchStepNetwork& getLinArchStepNetwork(unsigned long long d, unsigned k, unsigned t)
   {
      using key_t = std::tuple<unsigned long long, unsigned, unsigned>;
      static std::map<key_t, LinArchStepNetwork> cache;

      const auto key = std::make_tuple(d, k, t);
      const auto it = cache.find(key);
      if(it != cache.end())
      {
         return it->second;
      }

      LinArchStepNetwork net;
      net.d = d;
      net.k = k;
      net.t = t;
      net.in_bits = t + k;
      net.out_bits = t + k;

      std::vector<kitty::dynamic_truth_table> outputs;
      outputs.reserve(net.out_bits);
      for(unsigned i = 0; i < net.out_bits; ++i)
      {
         outputs.emplace_back(net.in_bits);
         kitty::clear(outputs.back());
      }

      const unsigned long long beta = 1ULL << k;
      const unsigned long long max_val = 1ULL << net.in_bits;
      const unsigned long long r_mask = (1ULL << t) - 1ULL;
      for(unsigned long long val = 0; val < max_val; ++val)
      {
         const unsigned long long r = val & r_mask;
         const unsigned long long x = val >> t;
         const unsigned long long u = r * beta + x;
         const unsigned long long qdig = u / d;
         const unsigned long long rnext = u % d;
         for(unsigned i = 0; i < k; ++i)
         {
            if(qdig & (1ULL << i))
            {
               kitty::set_bit(outputs[i], val);
            }
         }
         for(unsigned i = 0; i < t; ++i)
         {
            if(rnext & (1ULL << i))
            {
               kitty::set_bit(outputs[k + i], val);
            }
         }
      }

      net.aig = buildAigFromTruthTables(net.in_bits, outputs);

      auto inserted = cache.emplace(key, std::move(net));
      return inserted.first->second;
   }

   struct BtcdILutNetwork
   {
      unsigned long long d;
      unsigned k;
      unsigned r;
      unsigned in_bits;
      unsigned out_bits;
      mockturtle::aig_network aig;
   };

   struct BtcdRLutNetwork
   {
      unsigned long long d;
      unsigned r;
      unsigned shift_bits;
      unsigned in_bits;
      unsigned out_bits;
      mockturtle::aig_network aig;
   };

   static const BtcdILutNetwork& getBtcdILutNetwork(unsigned long long d, unsigned k, unsigned r)
   {
      using key_t = std::tuple<unsigned long long, unsigned, unsigned>;
      static std::map<key_t, BtcdILutNetwork> cache;

      const auto key = std::make_tuple(d, k, r);
      const auto it = cache.find(key);
      if(it != cache.end())
      {
         return it->second;
      }

      BtcdILutNetwork net;
      net.d = d;
      net.k = k;
      net.r = r;
      net.in_bits = k;
      net.out_bits = k + r;

      std::vector<kitty::dynamic_truth_table> outputs;
      outputs.reserve(net.out_bits);
      for(unsigned i = 0; i < net.out_bits; ++i)
      {
         outputs.emplace_back(net.in_bits);
         kitty::clear(outputs.back());
      }

      const unsigned long long max_val = 1ULL << net.in_bits;
      for(unsigned long long val = 0; val < max_val; ++val)
      {
         const unsigned long long x = val;
         const unsigned long long qdig = d != 0 ? x / d : 0;
         const unsigned long long rdig = d != 0 ? x % d : 0;
         for(unsigned i = 0; i < k; ++i)
         {
            if(qdig & (1ULL << i))
            {
               kitty::set_bit(outputs[i], val);
            }
         }
         for(unsigned i = 0; i < r; ++i)
         {
            if(rdig & (1ULL << i))
            {
               kitty::set_bit(outputs[k + i], val);
            }
         }
      }

      net.aig = buildAigFromTruthTables(net.in_bits, outputs);
      auto inserted = cache.emplace(key, std::move(net));
      return inserted.first->second;
   }

   static const BtcdRLutNetwork& getBtcdRLutNetwork(unsigned long long d, unsigned r, unsigned shift_bits)
   {
      using key_t = std::tuple<unsigned long long, unsigned, unsigned>;
      static std::map<key_t, BtcdRLutNetwork> cache;

      const auto key = std::make_tuple(d, r, shift_bits);
      const auto it = cache.find(key);
      if(it != cache.end())
      {
         return it->second;
      }

      BtcdRLutNetwork net;
      net.d = d;
      net.r = r;
      net.shift_bits = shift_bits;
      net.in_bits = 2U * r;
      net.out_bits = r + shift_bits;

      std::vector<kitty::dynamic_truth_table> outputs;
      outputs.reserve(net.out_bits);
      for(unsigned i = 0; i < net.out_bits; ++i)
      {
         outputs.emplace_back(net.in_bits);
         kitty::clear(outputs.back());
      }

      const unsigned long long max_val = 1ULL << net.in_bits;
      const unsigned long long r_mask = (1ULL << r) - 1ULL;
      for(unsigned long long val = 0; val < max_val; ++val)
      {
         const unsigned long long r_lo = val & r_mask;
         const unsigned long long r_hi = val >> r;
         const uint128_t t = (static_cast<uint128_t>(r_hi) << shift_bits) + r_lo;
         const uint128_t qrem = d != 0 ? t / d : 0;
         const unsigned long long r_out = d != 0 ? static_cast<unsigned long long>(t % d) : 0;
         for(unsigned i = 0; i < shift_bits; ++i)
         {
            if(qrem & (static_cast<uint128_t>(1) << i))
            {
               kitty::set_bit(outputs[i], val);
            }
         }
         for(unsigned i = 0; i < r; ++i)
         {
            if(r_out & (1ULL << i))
            {
               kitty::set_bit(outputs[shift_bits + i], val);
            }
         }
      }

      net.aig = buildAigFromTruthTables(net.in_bits, outputs);
      auto inserted = cache.emplace(key, std::move(net));
      return inserted.first->second;
   }

   struct ConstDivOpCosts
   {
      double add_delay;
      double add_area;
      double mult_delay;
      double mult_area;
      double mult_lut_area;
      double mult_dsp_count;
      double shift_delay;
      double shift_area;
      double div_delay;
      double div_area;
      bool div_available;
   };

   static double getWeightedMultArea(const HLS_deviceConstRef& HLS_D, const ConstDivOpCosts& costs, double dsp_weight_k)
   {
      return area_estimation::get_lut_equivalent_area_weighted(HLS_D, costs.mult_lut_area, costs.mult_dsp_count,
                                                               dsp_weight_k);
   }

   struct LinArchCosts
   {
      double delay;
      double area;
      unsigned digits;
      unsigned in_bits;
      bool valid;
   };

   struct LutBitCost
   {
      double delay;
      double area;
      bool valid;
   };

   struct MappedLutStats
   {
      unsigned lut_count;
      unsigned depth;
      bool valid;
   };

   struct BtcdCosts
   {
      double delay;
      double area;
      unsigned digits;
      unsigned k;
      unsigned r;
      bool valid;
   };

   static bool getOpCost(const HLS_deviceRef& HLS_D, const std::string& component_name, const std::string& op_name,
                         double& delay, double& area, double* lut_area = nullptr, double* dsp_count = nullptr)
   {
      if(!HLS_D)
      {
         return false;
      }
      const auto TechManager = HLS_D->get_technology_manager();
      if(!TechManager)
      {
         return false;
      }
      const technology_nodeRef op_f_unit = TechManager->get_fu(component_name, LIBRARY_STD_FU);
      if(!op_f_unit)
      {
         return false;
      }
      const auto* op_fu = GetPointer<functional_unit>(op_f_unit);
      const technology_nodeRef op_node = op_fu->get_operation(op_name);
      if(!op_node)
      {
         return false;
      }
      const auto* op = GetPointer<operation>(op_node);
      delay = op->time_m->get_execution_time();
      THROW_ASSERT(op_fu->area_m, "Area information not specified for unit " + component_name);
      if(lut_area)
      {
         *lut_area = area_estimation::get_lut_component(op_fu->area_m);
      }
      if(dsp_count)
      {
         *dsp_count = op_fu->area_m->resource_or_default(area_info::DSP);
      }
      area = area_estimation::get_lut_equivalent_area(HLS_D, op_fu->area_m);
      return true;
   }

   static std::string buildUiComponentName(const std::string& op_name, unsigned fu_prec, bool needs_suffix)
   {
      std::string name = "ui_" + op_name + std::string("_FU_") + STR(fu_prec) + "_" + STR(fu_prec) + "_" + STR(fu_prec);
      if(needs_suffix)
      {
         name += "_0";
      }
      return name;
   }

   static std::string buildComponentName(const std::string& op_name, unsigned fu_prec, bool needs_suffix)
   {
      std::string name = op_name + std::string("_FU_") + STR(fu_prec) + "_" + STR(fu_prec) + "_" + STR(fu_prec);
      if(needs_suffix)
      {
         name += "_0";
      }
      return name;
   }

   static ConstDivOpCosts estimateConstDivCosts(const IR_lowering& lowering, const application_managerRef& AppM,
                                                unsigned data_bitsize, bool rem_flag)
   {
      ConstDivOpCosts costs;
      costs.add_delay = 1.0;
      costs.add_area = 1.0;
      costs.mult_delay = 3.0;
      costs.mult_area = 1.0;
      costs.mult_lut_area = 1.0;
      costs.mult_dsp_count = 0.0;
      costs.shift_delay = 1.0;
      costs.shift_area = 1.0;
      costs.div_delay = costs.mult_delay * 8.0;
      costs.div_area = costs.mult_area * 8.0;
      costs.div_available = false;

      const HLS_deviceRef HLS_D = getHlsDeviceFromApp(AppM);
      if(!HLS_D)
      {
         return costs;
      }

      double add_delay = costs.add_delay;
      double sub_delay = costs.add_delay;
      double mult_delay = costs.mult_delay;
      double shift_delay = costs.shift_delay;
      double add_area = costs.add_area;
      double mult_area = costs.mult_area;
      double mult_lut_area = costs.mult_lut_area;
      double mult_dsp_count = costs.mult_dsp_count;
      double shift_area = costs.shift_area;
      lowering.getAddSubMultCosts(HLS_D, data_bitsize, add_delay, sub_delay, mult_delay, shift_delay, add_area,
                                  mult_area, mult_lut_area, mult_dsp_count, shift_area, HLS_D != nullptr);
      costs.add_delay = add_delay;
      costs.add_area = add_area;
      costs.mult_delay = mult_delay;
      costs.mult_area = mult_area;
      costs.mult_lut_area = mult_lut_area;
      costs.mult_dsp_count = mult_dsp_count;
      costs.shift_delay = shift_delay;
      costs.shift_area = shift_area;

      double delay = 0.0;
      double area = 0.0;
      const auto fu_prec = std::max(8ull, ceil_pow2(static_cast<unsigned long long>(data_bitsize)));

      const std::string op_name = rem_flag ? "irem_node" : "idiv_node";
      const std::string ui_component = buildUiComponentName(op_name, static_cast<unsigned>(fu_prec), true);
      const std::string component = buildComponentName(op_name, static_cast<unsigned>(fu_prec), true);
      if(getOpCost(HLS_D, ui_component, op_name, delay, area) || getOpCost(HLS_D, component, op_name, delay, area))
      {
         costs.div_delay = delay;
         costs.div_area = area;
         costs.div_available = true;
      }

      return costs;
   }

   static bool getLutBaseCost(const IR_lowering& lowering, const application_managerRef& AppM, unsigned max_lut_size,
                              double& lut_delay, double& lut_area)
   {
      const HLS_deviceRef HLS_D = getHlsDeviceFromApp(AppM);
      if(!HLS_D)
      {
         return false;
      }
      return lowering.getLutCost(HLS_D, max_lut_size, max_lut_size, lut_delay, lut_area);
   }

   static MappedLutStats mapAigToLutStats(const mockturtle::aig_network& aig, unsigned max_lut_size)
   {
      MappedLutStats stats{0U, 0U, false};
      if(max_lut_size == 0)
      {
         return stats;
      }
      try
      {
         mockturtle::mapping_view<mockturtle::aig_network, true> mapped_klut{aig};
         mockturtle::lut_mapping_params mp;
         mp.cut_enumeration_ps.cut_size = static_cast<uint32_t>(max_lut_size);
         mp.cut_enumeration_ps.cut_limit = 16;
#ifndef NDEBUG
         mp.verbose = false;
         mp.cut_enumeration_ps.very_verbose = false;
#endif
         mockturtle::lut_mapping<decltype(mapped_klut), true>(mapped_klut, mp);
         auto collapsed = *mockturtle::collapse_mapped_network<mockturtle::klut_network>(mapped_klut);
         collapsed = mockturtle::cleanup_luts(collapsed);
         mockturtle::depth_view depth_view{collapsed};
         stats.lut_count = static_cast<unsigned>(collapsed.num_gates());
         stats.depth = static_cast<unsigned>(depth_view.depth());
         stats.valid = true;
      }
      catch(...)
      {
         stats.valid = false;
      }
      return stats;
   }

   static MappedLutStats mapKlutToLutStats(const mockturtle::klut_network& klut, unsigned max_lut_size)
   {
      MappedLutStats stats{0U, 0U, false};
      if(max_lut_size == 0)
      {
         return stats;
      }
      try
      {
         mockturtle::shannon_resynthesis<mockturtle::aig_network> fallback;
         mockturtle::dsd_resynthesis<mockturtle::aig_network, decltype(fallback)> aig_resyn(fallback);
         auto aig = mockturtle::node_resynthesis<mockturtle::aig_network>(klut, aig_resyn);
         aig = mockturtle::cleanup_dangling(aig);
         return mapAigToLutStats(aig, max_lut_size);
      }
      catch(...)
      {
         return stats;
      }
   }

   static std::vector<KlutSignal> emitAigInKlut(mockturtle::klut_network& ntk, const mockturtle::aig_network& aig,
                                                const std::vector<KlutSignal>& inputs)
   {
      std::map<mockturtle::aig_network::node, KlutSignal> node_map;
      unsigned pi_index = 0;
      aig.foreach_pi([&](const auto& node) {
         node_map[node] = pi_index < inputs.size() ? inputs[pi_index] : ntk.get_constant(false);
         ++pi_index;
      });

      auto resolve_signal = [&](mockturtle::aig_network::signal s) -> KlutSignal {
         if(s.index == 0)
         {
            return s.complement ? ntk.get_constant(true) : ntk.get_constant(false);
         }
         const auto it = node_map.find(s.index);
         THROW_ASSERT(it != node_map.end(), "AIG node not found while emitting klut network");
         return s.complement ? createKlutNot(ntk, it->second) : it->second;
      };

      mockturtle::topo_view topo{aig};
      topo.foreach_node([&](const auto& node) {
         if(aig.is_constant(node) || aig.is_pi(node))
         {
            return true;
         }
         mockturtle::aig_network::signal fanins[2]{};
         aig.foreach_fanin(node, [&](const mockturtle::aig_network::signal& s, unsigned index) {
            if(index < 2)
            {
               fanins[index] = s;
            }
         });
         const auto lhs = resolve_signal(fanins[0]);
         const auto rhs = resolve_signal(fanins[1]);
         node_map[node] = createKlutAnd(ntk, lhs, rhs);
         return true;
      });

      std::vector<KlutSignal> outputs;
      outputs.reserve(aig.num_pos());
      aig.foreach_po([&](const auto& s) { outputs.push_back(resolve_signal(s)); });
      return outputs;
   }

   static std::vector<KlutSignal> orKlutWords(mockturtle::klut_network& ntk, const std::vector<KlutSignal>& lhs,
                                              const std::vector<KlutSignal>& rhs)
   {
      THROW_ASSERT(lhs.size() == rhs.size(), "Mismatched widths in orKlutWords");
      std::vector<KlutSignal> out(lhs.size(), ntk.get_constant(false));
      for(size_t bit = 0; bit < lhs.size(); ++bit)
      {
         out[bit] = createKlutOr(ntk, lhs[bit], rhs[bit]);
      }
      return out;
   }

   static mockturtle::klut_network buildLinArchFullKlutNetwork(unsigned long long d, unsigned data_bitsize, unsigned k,
                                                               unsigned t, unsigned digits)
   {
      mockturtle::klut_network ntk;
      if(data_bitsize == 0 || k == 0 || t == 0 || digits == 0)
      {
         return ntk;
      }

      std::vector<KlutSignal> x_bits;
      x_bits.reserve(data_bitsize);
      for(unsigned bit = 0; bit < data_bitsize; ++bit)
      {
         x_bits.push_back(ntk.create_pi());
      }

      auto q_value = makeZeroWord(ntk, data_bitsize);
      auto r_value = makeZeroWord(ntk, data_bitsize);
      const auto& step_net = getLinArchStepNetwork(d, k, t);

      for(unsigned di = 0; di < digits; ++di)
      {
         const unsigned digit_index = digits - di - 1;
         const unsigned shift = digit_index * k;

         std::vector<KlutSignal> step_inputs;
         step_inputs.reserve(t + k);
         for(unsigned i = 0; i < t; ++i)
         {
            step_inputs.push_back(i < r_value.size() ? r_value[i] : ntk.get_constant(false));
         }
         for(unsigned i = 0; i < k; ++i)
         {
            const unsigned src = shift + i;
            step_inputs.push_back(src < data_bitsize ? x_bits[src] : ntk.get_constant(false));
         }

         const auto step_outputs = emitAigInKlut(ntk, step_net.aig, step_inputs);
         THROW_ASSERT(step_outputs.size() == k + t, "linarch full-model: unexpected number of LUT outputs");

         auto q_digit = makeZeroWord(ntk, data_bitsize);
         for(unsigned i = 0; i < k && i < data_bitsize; ++i)
         {
            q_digit[i] = step_outputs[i];
         }
         const auto q_shifted = shiftKlutWord(ntk, q_value, k, data_bitsize);
         q_value = orKlutWords(ntk, q_shifted, q_digit);

         auto r_next = makeZeroWord(ntk, data_bitsize);
         for(unsigned i = 0; i < t && i < data_bitsize; ++i)
         {
            r_next[i] = step_outputs[k + i];
         }
         r_value = r_next;
      }

      for(const auto bit : q_value)
      {
         ntk.create_po(bit);
      }
      for(unsigned i = 0; i < t && i < data_bitsize; ++i)
      {
         ntk.create_po(r_value[i]);
      }
      return ntk;
   }

   struct BtcdNodeKlut
   {
      std::vector<KlutSignal> q;
      std::vector<KlutSignal> r;
      unsigned digits;
   };

   static mockturtle::klut_network buildBtcdFullKlutNetwork(unsigned long long d, unsigned data_bitsize, unsigned k,
                                                            unsigned r, unsigned digits)
   {
      mockturtle::klut_network ntk;
      if(data_bitsize == 0 || k == 0 || r == 0 || digits == 0)
      {
         return ntk;
      }

      std::vector<KlutSignal> x_bits;
      x_bits.reserve(data_bitsize);
      for(unsigned bit = 0; bit < data_bitsize; ++bit)
      {
         x_bits.push_back(ntk.create_pi());
      }

      const auto zero_word = makeZeroWord(ntk, data_bitsize);
      const unsigned long long leaf_count = std::max(1ULL, ceil_pow2(static_cast<unsigned long long>(digits)));
      std::vector<BtcdNodeKlut> nodes;
      nodes.reserve(static_cast<size_t>(leaf_count));

      for(unsigned long long idx = 0; idx < leaf_count; ++idx)
      {
         if(idx >= digits)
         {
            nodes.push_back(BtcdNodeKlut{zero_word, zero_word, 1U});
            continue;
         }

         const unsigned shift = static_cast<unsigned>(idx) * k;
         std::vector<KlutSignal> digit_inputs;
         digit_inputs.reserve(k);
         for(unsigned i = 0; i < k; ++i)
         {
            const unsigned src = shift + i;
            digit_inputs.push_back(src < data_bitsize ? x_bits[src] : ntk.get_constant(false));
         }

         const auto i_outputs = emitAigInKlut(ntk, getBtcdILutNetwork(d, k, r).aig, digit_inputs);
         THROW_ASSERT(i_outputs.size() == k + r, "btcd full-model: unexpected iLUT output size");

         auto q_leaf = makeZeroWord(ntk, data_bitsize);
         auto r_leaf = makeZeroWord(ntk, data_bitsize);
         for(unsigned i = 0; i < k && i < data_bitsize; ++i)
         {
            q_leaf[i] = i_outputs[i];
         }
         for(unsigned i = 0; i < r && i < data_bitsize; ++i)
         {
            r_leaf[i] = i_outputs[k + i];
         }
         nodes.push_back(BtcdNodeKlut{std::move(q_leaf), std::move(r_leaf), 1U});
      }

      while(nodes.size() > 1)
      {
         std::vector<BtcdNodeKlut> next;
         next.reserve((nodes.size() + 1) / 2);
         for(size_t i = 0; i < nodes.size(); i += 2)
         {
            if(i + 1 >= nodes.size())
            {
               next.push_back(nodes[i]);
               continue;
            }

            const auto& lo = nodes[i];
            const auto& hi = nodes[i + 1];
            const unsigned shift_bits = lo.digits * k;

            std::vector<KlutSignal> r_inputs;
            r_inputs.reserve(2U * r);
            for(unsigned bit = 0; bit < r; ++bit)
            {
               r_inputs.push_back(bit < lo.r.size() ? lo.r[bit] : ntk.get_constant(false));
            }
            for(unsigned bit = 0; bit < r; ++bit)
            {
               r_inputs.push_back(bit < hi.r.size() ? hi.r[bit] : ntk.get_constant(false));
            }

            const auto r_outputs = emitAigInKlut(ntk, getBtcdRLutNetwork(d, r, shift_bits).aig, r_inputs);
            THROW_ASSERT(r_outputs.size() == shift_bits + r, "btcd full-model: unexpected rLUT output size");

            auto q_rem = makeZeroWord(ntk, data_bitsize);
            const unsigned qrem_bits = std::min(shift_bits, data_bitsize);
            for(unsigned bit = 0; bit < qrem_bits; ++bit)
            {
               q_rem[bit] = r_outputs[bit];
            }

            auto r_out = makeZeroWord(ntk, data_bitsize);
            for(unsigned bit = 0; bit < r && bit < data_bitsize; ++bit)
            {
               r_out[bit] = r_outputs[shift_bits + bit];
            }

            const auto hi_shifted = shiftKlutWord(ntk, hi.q, shift_bits, data_bitsize);
            const auto q_sum = addKlutWords(ntk, hi_shifted, lo.q);
            const auto q_out = addKlutWords(ntk, q_sum, q_rem);
            next.push_back(BtcdNodeKlut{q_out, r_out, lo.digits + hi.digits});
         }
         nodes.swap(next);
      }

      if(nodes.empty())
      {
         return ntk;
      }
      for(const auto bit : nodes.front().q)
      {
         ntk.create_po(bit);
      }
      for(unsigned i = 0; i < r && i < data_bitsize; ++i)
      {
         ntk.create_po(nodes.front().r[i]);
      }
      return ntk;
   }

   static MappedLutStats getLinArchStepLutStats(unsigned long long d, unsigned k, unsigned t, unsigned max_lut_size)
   {
      using key_t = std::tuple<unsigned long long, unsigned, unsigned, unsigned>;
      static std::map<key_t, MappedLutStats> cache;
      const auto key = std::make_tuple(d, k, t, max_lut_size);
      const auto it = cache.find(key);
      if(it != cache.end())
      {
         return it->second;
      }

      const auto& net = getLinArchStepNetwork(d, k, t);
      const auto stats = mapAigToLutStats(net.aig, max_lut_size);
      cache.emplace(key, stats);
      return stats;
   }

   static MappedLutStats getBtcdILutStats(unsigned long long d, unsigned k, unsigned r, unsigned max_lut_size)
   {
      using key_t = std::tuple<unsigned long long, unsigned, unsigned, unsigned>;
      static std::map<key_t, MappedLutStats> cache;
      const auto key = std::make_tuple(d, k, r, max_lut_size);
      const auto it = cache.find(key);
      if(it != cache.end())
      {
         return it->second;
      }

      const auto& net = getBtcdILutNetwork(d, k, r);
      const auto stats = mapAigToLutStats(net.aig, max_lut_size);
      cache.emplace(key, stats);
      return stats;
   }

   static MappedLutStats getBtcdRLutStats(unsigned long long d, unsigned r, unsigned shift_bits, unsigned max_lut_size)
   {
      using key_t = std::tuple<unsigned long long, unsigned, unsigned, unsigned>;
      static std::map<key_t, MappedLutStats> cache;
      const auto key = std::make_tuple(d, r, shift_bits, max_lut_size);
      const auto it = cache.find(key);
      if(it != cache.end())
      {
         return it->second;
      }

      const auto& net = getBtcdRLutNetwork(d, r, shift_bits);
      const auto stats = mapAigToLutStats(net.aig, max_lut_size);
      cache.emplace(key, stats);
      return stats;
   }

   static MappedLutStats getLinArchFullLutStats(unsigned long long d, unsigned data_bitsize, unsigned k, unsigned t,
                                                unsigned digits, unsigned max_lut_size)
   {
      using key_t = std::tuple<unsigned long long, unsigned, unsigned, unsigned, unsigned, unsigned>;
      static std::map<key_t, MappedLutStats> cache;
      const auto key = std::make_tuple(d, data_bitsize, k, t, digits, max_lut_size);
      const auto it = cache.find(key);
      if(it != cache.end())
      {
         return it->second;
      }

      const auto full_klut = buildLinArchFullKlutNetwork(d, data_bitsize, k, t, digits);
      auto stats = mapKlutToLutStats(full_klut, max_lut_size);
      cache.emplace(key, stats);
      return stats;
   }

   static MappedLutStats getBtcdFullLutStats(unsigned long long d, unsigned data_bitsize, unsigned k, unsigned r,
                                             unsigned digits, unsigned max_lut_size)
   {
      using key_t = std::tuple<unsigned long long, unsigned, unsigned, unsigned, unsigned, unsigned>;
      static std::map<key_t, MappedLutStats> cache;
      const auto key = std::make_tuple(d, data_bitsize, k, r, digits, max_lut_size);
      const auto it = cache.find(key);
      if(it != cache.end())
      {
         return it->second;
      }

      const auto full_klut = buildBtcdFullKlutNetwork(d, data_bitsize, k, r, digits);
      auto stats = mapKlutToLutStats(full_klut, max_lut_size);
      cache.emplace(key, stats);
      return stats;
   }

   static LutBitCost estimateLutBitCost(const IR_lowering& lowering, const application_managerRef& AppM,
                                        unsigned in_bits, unsigned max_lut_size, const std::string& lut_cost_model,
                                        const std::string& ASSERT_PARAMETER(tag))
   {
      LutBitCost cost{0.0, 0.0, false};
      THROW_ASSERT(in_bits > 0, tag + ": invalid input bits");
      THROW_ASSERT(max_lut_size > 0, tag + ": missing max_lut_size parameter");
      THROW_ASSERT(in_bits <= max_lut_size, tag + ": input bits exceed max_lut_size");

      double lut_delay = 0.0;
      double lut_area = 0.0;
      if(is_mockturtle_lut_cost_model(lut_cost_model))
      {
         if(!getLutBaseCost(lowering, AppM, max_lut_size, lut_delay, lut_area))
         {
            return cost;
         }
      }
      else
      {
         const HLS_deviceRef HLS_D = getHlsDeviceFromApp(AppM);
         if(!HLS_D)
         {
            return cost;
         }
         if(!lowering.getLutCost(HLS_D, in_bits, max_lut_size, lut_delay, lut_area))
         {
            return cost;
         }
      }

      cost.area = lut_area;
      cost.delay = lut_delay;
      cost.valid = true;
      return cost;
   }

   static LinArchCosts estimateLinArchCosts(const IR_lowering& lowering, const application_managerRef& AppM,
                                            unsigned long long d, unsigned data_bitsize, unsigned k, unsigned t,
                                            unsigned digits, unsigned max_lut_size, const std::string& lut_cost_model)
   {
      const unsigned in_bits = t + k;
      LinArchCosts costs{0.0, 0.0, digits, in_bits, false};
      THROW_ASSERT(in_bits > 0, "linarch: invalid input bits");
      THROW_ASSERT(digits > 0, "linarch: invalid digit count");

      if(is_mockturtle_full_lut_cost_model(lut_cost_model))
      {
         double lut_delay = 0.0;
         double lut_area = 0.0;
         if(!getLutBaseCost(lowering, AppM, max_lut_size, lut_delay, lut_area))
         {
            return costs;
         }
         const auto full_stats = getLinArchFullLutStats(d, data_bitsize, k, t, digits, max_lut_size);
         if(!full_stats.valid)
         {
            return costs;
         }
         const auto full_depth = full_stats.lut_count > 0 ? std::max(1U, full_stats.depth) : 0U;
         costs.area = static_cast<double>(full_stats.lut_count) * lut_area;
         costs.delay = static_cast<double>(full_depth) * lut_delay;
         costs.valid = true;
         return costs;
      }

      if(lut_cost_model == "mockturtle")
      {
         double lut_delay = 0.0;
         double lut_area = 0.0;
         if(!getLutBaseCost(lowering, AppM, max_lut_size, lut_delay, lut_area))
         {
            return costs;
         }
         const auto step_stats = getLinArchStepLutStats(d, k, t, max_lut_size);
         if(!step_stats.valid)
         {
            return costs;
         }
         costs.area = static_cast<double>(step_stats.lut_count) * lut_area * static_cast<double>(digits);
         const auto step_depth = step_stats.lut_count > 0 ? std::max(1U, step_stats.depth) : 0U;
         costs.delay = static_cast<double>(step_depth) * lut_delay * static_cast<double>(digits);
         costs.valid = true;
         return costs;
      }

      const auto bit_cost = estimateLutBitCost(lowering, AppM, in_bits, max_lut_size, lut_cost_model, "linarch");
      if(!bit_cost.valid)
      {
         return costs;
      }
      costs.area = bit_cost.area * static_cast<double>(in_bits) * static_cast<double>(digits);
      costs.delay = bit_cost.delay * static_cast<double>(digits);
      costs.valid = true;
      return costs;
   }

   static BtcdCosts estimateBtcdCosts(const IR_lowering& lowering, const application_managerRef& AppM,
                                      unsigned long long d, unsigned data_bitsize, unsigned k, unsigned r,
                                      unsigned max_lut_size, double add_delay, double add_area,
                                      const std::string& lut_cost_model)
   {
      BtcdCosts costs{0.0, 0.0, 0U, k, r, false};
      THROW_ASSERT(d > 0, "btcd: invalid divisor");
      THROW_ASSERT(k > 0, "btcd: invalid radix");
      THROW_ASSERT(r > 0, "btcd: invalid remainder bits");
      THROW_ASSERT(max_lut_size > 0, "btcd: missing max_lut_size parameter");

      const unsigned digits = (data_bitsize + k - 1) / k;
      if(digits == 0)
      {
         return costs;
      }
      costs.digits = digits;

      const unsigned long long leaf_count = std::max(1ULL, ceil_pow2(static_cast<unsigned long long>(digits)));
      const unsigned levels = leaf_count > 1 ? static_cast<unsigned>(floor_log2(leaf_count)) : 0U;
      double area = 0.0;
      double delay = 0.0;

      if(is_mockturtle_full_lut_cost_model(lut_cost_model))
      {
         double lut_delay = 0.0;
         double lut_area = 0.0;
         if(!getLutBaseCost(lowering, AppM, max_lut_size, lut_delay, lut_area))
         {
            return costs;
         }
         const auto full_stats = getBtcdFullLutStats(d, data_bitsize, k, r, digits, max_lut_size);
         if(!full_stats.valid)
         {
            return costs;
         }
         const auto full_depth = full_stats.lut_count > 0 ? std::max(1U, full_stats.depth) : 0U;
         costs.area = static_cast<double>(full_stats.lut_count) * lut_area;
         costs.delay = static_cast<double>(full_depth) * lut_delay;
         costs.valid = true;
         return costs;
      }

      if(lut_cost_model == "mockturtle")
      {
         double lut_delay = 0.0;
         double lut_area = 0.0;
         if(!getLutBaseCost(lowering, AppM, max_lut_size, lut_delay, lut_area))
         {
            return costs;
         }

         const auto i_stats = getBtcdILutStats(d, k, r, max_lut_size);
         if(!i_stats.valid)
         {
            return costs;
         }
         const auto i_depth = i_stats.lut_count > 0 ? std::max(1U, i_stats.depth) : 0U;
         area += static_cast<double>(leaf_count) * static_cast<double>(i_stats.lut_count) * lut_area;
         delay = static_cast<double>(i_depth) * lut_delay;

         unsigned digits_lo = 1;
         for(unsigned level = 0; level < levels; ++level)
         {
            const unsigned nodes = static_cast<unsigned>(leaf_count >> (level + 1));
            const unsigned shift_bits = digits_lo * k;
            const auto r_stats = getBtcdRLutStats(d, r, shift_bits, max_lut_size);
            if(!r_stats.valid)
            {
               return costs;
            }
            const auto r_depth = r_stats.lut_count > 0 ? std::max(1U, r_stats.depth) : 0U;
            area += static_cast<double>(nodes) * static_cast<double>(r_stats.lut_count) * lut_area;
            area += static_cast<double>(nodes) * 2.0 * add_area;
            delay += static_cast<double>(r_depth) * lut_delay + 2.0 * add_delay;
            digits_lo <<= 1;
         }
      }
      else
      {
         const auto i_cost = estimateLutBitCost(lowering, AppM, k, max_lut_size, lut_cost_model, "btcd_iLUT");
         const auto r_cost = estimateLutBitCost(lowering, AppM, 2U * r, max_lut_size, lut_cost_model, "btcd_rLUT");
         if(!i_cost.valid || !r_cost.valid)
         {
            return costs;
         }

         area += static_cast<double>(leaf_count) * i_cost.area * static_cast<double>(k + r);

         unsigned digits_lo = 1;
         for(unsigned level = 0; level < levels; ++level)
         {
            const unsigned nodes = static_cast<unsigned>(leaf_count >> (level + 1));
            const unsigned shift_bits = digits_lo * k;
            const double lut_outputs = static_cast<double>(r + shift_bits);
            area += static_cast<double>(nodes) * r_cost.area * lut_outputs;
            area += static_cast<double>(nodes) * 2.0 * add_area;
            digits_lo <<= 1;
         }

         delay = i_cost.delay;
         if(levels > 0)
         {
            delay += static_cast<double>(levels) * (r_cost.delay + 2.0 * add_delay);
         }
      }

      costs.area = area;
      costs.delay = delay;
      costs.valid = true;
      return costs;
   }

#if HAVE_ASSERTS
   static void debugCheckMagic64()
   {
      static bool done = false;
      if(done)
      {
         return;
      }
      done = true;

      {
         const SDivMagic64 mag = computeSDivMagic64(3, 32);
         THROW_ASSERT(mag.magic == 0x55555556ULL, "sdiv magic mismatch for d=3");
         THROW_ASSERT(mag.shift == 0U, "sdiv shift mismatch for d=3");
         THROW_ASSERT(mag.numeratorFactor == 0, "sdiv numeratorFactor mismatch for d=3");
         THROW_ASSERT(mag.shiftMask == -1, "sdiv shiftMask mismatch for d=3");
      }
      {
         const SDivMagic64 mag = computeSDivMagic64(-3, 32);
         THROW_ASSERT(mag.magic == 0x55555555ULL, "sdiv magic mismatch for d=-3");
         THROW_ASSERT(mag.shift == 1U, "sdiv shift mismatch for d=-3");
         THROW_ASSERT(mag.numeratorFactor == -1, "sdiv numeratorFactor mismatch for d=-3");
      }
      {
         const SDivMagic64 mag = computeSDivMagic64(7, 32);
         THROW_ASSERT(mag.magic == 0x92492493ULL, "sdiv magic mismatch for d=7");
         THROW_ASSERT(mag.shift == 2U, "sdiv shift mismatch for d=7");
         THROW_ASSERT(mag.numeratorFactor == 1, "sdiv numeratorFactor mismatch for d=7");
      }
      {
         const UDivMagic64 mag = computeUDivMagic64(3ULL, 32, 0, true);
         THROW_ASSERT(mag.magic == 0xaaaaaaabULL, "udiv magic mismatch for d=3");
         THROW_ASSERT(!mag.isAdd && mag.preShift == 0 && mag.postShift == 1, "udiv params mismatch for d=3");
      }
      {
         const UDivMagic64 mag = computeUDivMagic64(7ULL, 32, 0, true);
         THROW_ASSERT(mag.magic == 0x24924925ULL, "udiv magic mismatch for d=7");
         THROW_ASSERT(mag.isAdd && mag.preShift == 0 && mag.postShift == 2, "udiv params mismatch for d=7");
      }
      {
         const UDivMagic64 mag = computeUDivMagic64(14ULL, 32, 0, true);
         THROW_ASSERT(mag.magic == 0x92492493ULL, "udiv magic mismatch for d=14");
         THROW_ASSERT(!mag.isAdd && mag.preShift == 1 && mag.postShift == 2, "udiv params mismatch for d=14");
      }
      {
         const UDivMagic64 mag = computeUDivMagic64(3ULL, 32, 1, true);
         THROW_ASSERT(mag.magic == 0x55555556ULL, "udiv magic mismatch for d=3 lz=1");
         THROW_ASSERT(!mag.isAdd && mag.preShift == 0 && mag.postShift == 0, "udiv params mismatch for d=3 lz=1");
      }
   }
#endif
} // namespace

void IR_lowering::getAddSubMultCosts(const HLS_deviceRef& HLS_D, unsigned data_bitsize, double& add_delay,
                                     double& sub_delay, double& mult_delay, double& shift_delay, double& add_area,
                                     double& mult_area, double& mult_lut_area, double& mult_dsp_count,
                                     double& shift_area, bool require_fus) const
{
   add_delay = 1.0;
   sub_delay = 1.0;
   mult_delay = 3.0;
   shift_delay = 1.0;
   add_area = 1.0;
   mult_area = 1.0;
   mult_lut_area = 1.0;
   mult_dsp_count = 0.0;
   shift_area = 1.0;

   if(!HLS_D)
   {
      return;
   }
   const auto TechManager = HLS_D->get_technology_manager();
   if(!TechManager)
   {
      THROW_ASSERT(!require_fus, "missing technology manager");
      return;
   }

   const auto fu_prec = std::max(8ull, ceil_pow2(static_cast<unsigned long long>(data_bitsize)));
   const std::string add_component =
       ADDER_STD + std::string("_") + STR(fu_prec) + "_" + STR(fu_prec) + "_" + STR(fu_prec);
   const std::string mult_component =
       MULTIPLIER_STD + std::string("_") + STR(fu_prec) + "_" + STR(fu_prec) + "_" + STR(fu_prec) + "_0";

   double delay = 0.0;
   double area = 0.0;
   if(getOpCost(HLS_D, add_component, "add_node", delay, area))
   {
      add_delay = delay;
      add_area = area;
   }
   else if(require_fus)
   {
      THROW_ASSERT(false, "missing add_node from " + add_component);
   }

   sub_delay = add_delay;
   if(const auto add_f_unit = TechManager->get_fu(add_component, LIBRARY_STD_FU))
   {
      if(const auto* add_fu = GetPointer<functional_unit>(add_f_unit))
      {
         if(technology_nodeRef sub_op_node = add_fu->get_operation("sub_node"))
         {
            if(const auto* sub_op = GetPointer<operation>(sub_op_node))
            {
               sub_delay = sub_op->time_m->get_execution_time();
            }
         }
      }
   }
   else if(require_fus)
   {
      THROW_ASSERT(false, "missing component: " + add_component);
   }

   if(getOpCost(HLS_D, mult_component, "mul_node", delay, area, &mult_lut_area, &mult_dsp_count))
   {
      mult_delay = delay;
      mult_area = area;
   }
   else if(require_fus)
   {
      THROW_ASSERT(false, "missing mul_node from " + mult_component);
   }

   shift_delay = add_delay;
   shift_area = add_area;
   const auto op_prec = static_cast<unsigned>(fu_prec);
   auto try_shift_cost = [&](const std::string& op_name, double& op_delay, double& op_area) {
      const std::string ui_component_ns = buildUiComponentName(op_name, op_prec, false);
      const std::string ui_component_s = buildUiComponentName(op_name, op_prec, true);
      const std::string component_ns = buildComponentName(op_name, op_prec, false);
      const std::string component_s = buildComponentName(op_name, op_prec, true);
      return getOpCost(HLS_D, ui_component_ns, op_name, op_delay, op_area) ||
             getOpCost(HLS_D, ui_component_s, op_name, op_delay, op_area) ||
             getOpCost(HLS_D, component_ns, op_name, op_delay, op_area) ||
             getOpCost(HLS_D, component_s, op_name, op_delay, op_area);
   };

   double lshift_delay = 0.0;
   double lshift_area = 0.0;
   double rshift_delay = 0.0;
   double rshift_area = 0.0;
   const bool has_lshift = try_shift_cost("shl_node", lshift_delay, lshift_area);
   const bool has_rshift = try_shift_cost("shr_node", rshift_delay, rshift_area);
   if(has_lshift || has_rshift)
   {
      shift_delay =
          has_lshift && has_rshift ? std::max(lshift_delay, rshift_delay) : (has_lshift ? lshift_delay : rshift_delay);
      shift_area =
          has_lshift && has_rshift ? std::max(lshift_area, rshift_area) : (has_lshift ? lshift_area : rshift_area);
   }
}

bool IR_lowering::getLutCost(const HLS_deviceRef& HLS_D, unsigned in_bits, unsigned max_lut_size, double& lut_delay,
                             double& lut_area) const
{
   lut_delay = 0.0;
   lut_area = 0.0;
   if(!HLS_D || in_bits == 0 || max_lut_size == 0 || in_bits > max_lut_size)
   {
      return false;
   }
   double base_delay = 0.0;
   double base_area = 0.0;
   if(!getOpCost(HLS_D, LUT_NODE_STD, "lut_node", base_delay, base_area))
   {
      return false;
   }
   const double scale = static_cast<double>(in_bits) / static_cast<double>(max_lut_size);
   lut_delay = base_delay * scale;
   lut_area = base_area * scale;
   return true;
}

bool IR_lowering::handleTrivialDivByConstant(assign_stmt* ga, const ir_nodeRef& op0, const ir_nodeRef& typeExpr,
                                             long long extOp1, bool unsignedp, bool remFlag,
                                             const std::string& locInfoDefault, bool& restartAnalysis)
{
   /// very special case op1 == 1
   if(extOp1 == 1)
   {
      ir_nodeRef newOp1;
      if(remFlag)
      {
         newOp1 = TM->CreateUniqueIntegerCst(0, typeExpr);
      }
      else
      {
         newOp1 = op0;
      }
      ga->op1 = newOp1;
      restartAnalysis = true;
      return true;
   }

   if(!unsignedp && extOp1 == -1)
   {
      ir_nodeRef newOp1;
      if(remFlag)
      {
         newOp1 = TM->CreateUniqueIntegerCst(0, typeExpr);
      }
      else
      {
         newOp1 = ir_man->create_unary_operation(typeExpr, op0, locInfoDefault, neg_node_K);
      }
      ga->op1 = newOp1;
      restartAnalysis = true;
      return true;
   }

   return false;
}

void IR_lowering::assignRemainderFromQuotient(const std::pair<unsigned int, blocRef>& block,
                                              std::list<ir_nodeRef>::const_iterator& itLos, assign_stmt* ga,
                                              const ir_nodeRef& op0, const ir_nodeRef& op1, const ir_nodeRef& typeExpr,
                                              const ir_nodeRef& quotientExpr, const std::string& locInfoDefault,
                                              const std::string& stepName, bool restrictWidth, unsigned int dataBitsize)
{
   const auto quotientGa =
       ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), quotientExpr, function_id, locInfoDefault);
   block.second->PushBefore(quotientGa, *itLos, AppM);
   const auto quotientGaVar = GetPointer<assign_stmt>(quotientGa)->op0;
   const auto mulExpr = ir_man->create_binary_operation(typeExpr, quotientGaVar, op1, locInfoDefault, mul_node_K);
   const auto mulGa =
       ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), mulExpr, function_id, locInfoDefault);
   block.second->PushBefore(mulGa, *itLos, AppM);
   const auto mulGaVar = GetPointerS<assign_stmt>(mulGa)->op0;
   /// restrict if needed the input bit-widths
   ir_nodeRef subExpr;
   if(restrictWidth && AppM->ApplyNewTransformation() &&
      static_cast<unsigned long long>(dataBitsize) > ir_helper::Size(ga->op0))
   {
      const auto masklow = (integer_cst_t(1) << ir_helper::Size(ga->op0)) - 1;
      const auto constMasklow = TM->CreateUniqueIntegerCst(masklow, typeExpr);
      const auto tempOp0Expr = ir_man->create_binary_operation(typeExpr, op0, constMasklow, locInfoDefault, and_node_K);
      const auto tempOp0Ga =
          ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), tempOp0Expr, function_id, locInfoDefault);
      block.second->PushBefore(tempOp0Ga, *itLos, AppM);
      const auto tempOp0GaVar = GetPointer<assign_stmt>(tempOp0Ga)->op0;
      const auto tempOp1Expr =
          ir_man->create_binary_operation(typeExpr, mulGaVar, constMasklow, locInfoDefault, and_node_K);
      const auto tempOp1Ga =
          ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), tempOp1Expr, function_id, locInfoDefault);
      block.second->PushBefore(tempOp1Ga, *itLos, AppM);
      const auto tempOp1GaVar = GetPointerS<assign_stmt>(tempOp1Ga)->op0;
      const auto tempSubExpr =
          ir_man->create_binary_operation(typeExpr, tempOp0GaVar, tempOp1GaVar, locInfoDefault, sub_node_K);
      const auto tempSubExprGa =
          ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), tempSubExpr, function_id, locInfoDefault);
      AppM->RegisterTransformation(stepName, tempSubExprGa);
      block.second->PushBefore(tempSubExprGa, *itLos, AppM);
      const auto tempSubExprGaVar = GetPointerS<assign_stmt>(tempSubExprGa)->op0;
      subExpr = ir_man->create_binary_operation(typeExpr, tempSubExprGaVar, constMasklow, locInfoDefault, and_node_K);
   }
   else
   {
      subExpr = ir_man->create_binary_operation(typeExpr, op0, mulGaVar, locInfoDefault, sub_node_K);
   }
   ga->op1 = subExpr;
}

void IR_lowering::lowerUnsignedTruncDivModByConstant(const std::pair<unsigned int, blocRef>& block,
                                                     std::list<ir_nodeRef>::const_iterator& itLos, assign_stmt* ga,
                                                     const ir_nodeRef& op0, const ir_nodeRef& op1,
                                                     const ir_nodeRef& typeExpr, long long extOp1, bool remFlag,
                                                     const std::string& locInfoDefault, const std::string& stepName,
                                                     bool& restartAnalysis)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---unsigned");
   auto d = static_cast<unsigned long long int>(extOp1);
   if(POWER2_OR_0(d))
   {
      const auto preShift = static_cast<int>(floor_log2(d));
      ir_nodeRef newOp1;
      if(remFlag)
      {
         const auto mask = TM->CreateUniqueIntegerCst((static_cast<long long int>(1) << preShift) - 1, typeExpr);
         newOp1 = ir_man->create_binary_operation(typeExpr, op0, mask, locInfoDefault, and_node_K);
      }
      else
      {
         const auto shift = TM->CreateUniqueIntegerCst(preShift, typeExpr);
         newOp1 = createShiftOperationWithCostGuard(ir_man, typeExpr, op0, shift, locInfoDefault, shr_node_K,
                                                    "lowerUnsignedTruncDivModByConstant");
      }
      ga->op1 = newOp1;
      restartAnalysis = true;
      return;
   }

   const auto dataBitsize = static_cast<int>(ir_helper::Size(ir_helper::CGetType(typeExpr)));
   if(dataBitsize > 64)
   {
      return;
   }

   const unsigned linarch_d_max = 255;
   const unsigned linarch_k_max = 6;
   const unsigned linarch_k_min = 2;
   const unsigned linarch_n_max = 64;
   const unsigned btcd_k_max = 6;
   const unsigned btcd_k_min = 2;
   const unsigned btcd_n_max = 64;
   const HLS_deviceRef hls_d = getHlsDeviceFromApp(AppM);
   const unsigned max_lut_size = resolveRequiredMaxLutSize(AppM, hls_d);
   const bool auto_lut_cost_model = is_auto_lut_cost_model(constmultdiv_lut_cost_model);
   const bool use_mockturtle_lut_estimator =
       auto_lut_cost_model || is_mockturtle_lut_cost_model(constmultdiv_lut_cost_model);
   const unsigned linarch_input_budget =
       use_mockturtle_lut_estimator ? std::max(max_lut_size, CONSTDIV_MOCKTURTLE_MAX_TT_INPUTS) : max_lut_size;
   const unsigned btcd_input_budget =
       use_mockturtle_lut_estimator ? std::max(max_lut_size, CONSTDIV_MOCKTURTLE_MAX_TT_INPUTS) : max_lut_size;
   const unsigned btcd_k_input_limit = use_mockturtle_lut_estimator ? btcd_k_max : max_lut_size;
   auto select_current_lut_cost_model = [&](unsigned in_bits, unsigned lut_limit) {
      return select_lut_cost_model(constmultdiv_lut_cost_model, in_bits, lut_limit);
   };

   const bool can_use_magic = dataBitsize <= 64 && d != 0 && d != 1;
   bool use_magic = false;
   bool use_linarch = false;
   bool keep_div = false;
   unsigned leading_zeros = 0;
   UDivMagic64 magic{0ULL, 0U, 0U, false};

   unsigned linarch_k = 0;
   unsigned linarch_t = 0;
   unsigned linarch_digits = 0;
   unsigned linarch_in_bits = 0;
   LinArchCosts linarch_costs{0.0, 0.0, 0U, 0U, false};
   bool linarch_candidate = false;
   if((constdiv_lowering_mode == "auto" || constdiv_lowering_mode == "linarch") && d > 1 && d <= linarch_d_max &&
      dataBitsize <= static_cast<int>(linarch_n_max))
   {
      linarch_t = static_cast<unsigned>(ceil_log2(d));
      const unsigned in_budget = linarch_input_budget > linarch_t ? linarch_input_budget - linarch_t : 0U;
      linarch_k = in_budget >= linarch_k_min ? std::min(linarch_k_max, in_budget) : 0U;
      if(linarch_k > static_cast<unsigned>(dataBitsize))
      {
         linarch_k = static_cast<unsigned>(dataBitsize);
      }
      linarch_in_bits = linarch_t + linarch_k;
      if(linarch_k >= linarch_k_min && linarch_in_bits <= linarch_input_budget)
      {
         const unsigned data_bits = static_cast<unsigned>(dataBitsize);
         linarch_digits = (data_bits + linarch_k - 1) / linarch_k;
         const auto lut_cost_model = select_current_lut_cost_model(linarch_in_bits, max_lut_size);
         linarch_costs = estimateLinArchCosts(*this, AppM, d, data_bits, linarch_k, linarch_t, linarch_digits,
                                              max_lut_size, lut_cost_model);
         if(auto_lut_cost_model && lut_cost_model == "analytic" && !linarch_costs.valid)
         {
            linarch_costs = estimateLinArchCosts(*this, AppM, d, data_bits, linarch_k, linarch_t, linarch_digits,
                                                 max_lut_size, "mockturtle");
         }
         linarch_candidate = linarch_costs.valid;
      }
   }

   ConstDivOpCosts div_costs{1.0, 1.0, 3.0, 1.0, 1.0, 0.0, 1.0, 1.0, 24.0, 8.0, false};
   bool div_costs_ready = false;
   if(constdiv_lowering_mode == "auto" || constdiv_lowering_mode == "btcd")
   {
      div_costs = estimateConstDivCosts(*this, AppM, static_cast<unsigned>(dataBitsize), remFlag);
      div_costs_ready = true;
   }
   auto estimate_btcd_costs = [&](unsigned long long divisor, unsigned data_bits, unsigned k, unsigned r) -> BtcdCosts {
      const auto lut_cost_model = select_current_lut_cost_model(2U * r, max_lut_size);
      auto costs = estimateBtcdCosts(*this, AppM, divisor, data_bits, k, r, max_lut_size,
                                     div_costs_ready ? div_costs.add_delay : 1.0,
                                     div_costs_ready ? div_costs.add_area : 1.0, lut_cost_model);
      if(auto_lut_cost_model && lut_cost_model == "analytic" && !costs.valid)
      {
         costs = estimateBtcdCosts(*this, AppM, divisor, data_bits, k, r, max_lut_size,
                                   div_costs_ready ? div_costs.add_delay : 1.0,
                                   div_costs_ready ? div_costs.add_area : 1.0, "mockturtle");
      }
      return costs;
   };

   bool use_btcd = false;
   unsigned btcd_r = 0;
   unsigned btcd_k = 0;
   unsigned btcd_digits = 0;
   BtcdCosts btcd_costs{0.0, 0.0, 0U, 0U, 0U, false};
   bool btcd_candidate = false;
   if((constdiv_lowering_mode == "auto" || constdiv_lowering_mode == "btcd") && d > 1 &&
      dataBitsize <= static_cast<int>(btcd_n_max))
   {
      btcd_r = static_cast<unsigned>(ceil_log2(d));
      if(btcd_input_budget > 0 && (2U * btcd_r) <= btcd_input_budget)
      {
         const unsigned data_bits = static_cast<unsigned>(dataBitsize);
         const unsigned k_limit = std::min({btcd_k_max, btcd_k_input_limit, data_bits});
         double best_score = std::numeric_limits<double>::infinity();
         for(unsigned k = btcd_k_min; k <= k_limit; ++k)
         {
            const auto costs = estimate_btcd_costs(d, data_bits, k, btcd_r);
            if(!costs.valid)
            {
               continue;
            }
            const double cand_score = getConstMultDivScore(costs.delay, costs.area);
            if(cand_score < best_score)
            {
               best_score = cand_score;
               btcd_costs = costs;
               btcd_k = k;
               btcd_digits = costs.digits;
               btcd_candidate = true;
            }
         }
      }
   }

   if(constdiv_composite_enable && d > 1 && !POWER2_OR_0(d) &&
      (constdiv_lowering_mode == "auto" || constdiv_lowering_mode == "linarch" || constdiv_lowering_mode == "btcd") &&
      constdiv_composite_max_pairs > 0U)
   {
      if(!div_costs_ready)
      {
         div_costs = estimateConstDivCosts(*this, AppM, static_cast<unsigned>(dataBitsize), remFlag);
         div_costs_ready = true;
      }

      ConstDivOpCosts div_costs_div = div_costs;
      ConstDivOpCosts div_costs_mod = div_costs;
      if(constdiv_lowering_mode == "auto")
      {
         if(remFlag)
         {
            div_costs_div = estimateConstDivCosts(*this, AppM, static_cast<unsigned>(dataBitsize), false);
         }
         else
         {
            div_costs_mod = estimateConstDivCosts(*this, AppM, static_cast<unsigned>(dataBitsize), true);
         }
      }

      const auto mul_area = getWeightedMultArea(hls_d, div_costs, constdiv_dsp_scale_k);
      const auto overhead_delay = div_costs.mult_delay + div_costs.add_delay;
      const auto overhead_area = mul_area + div_costs.add_area;
      const auto overhead_score = getConstMultDivScore(overhead_delay, overhead_area);
      unsigned leading_zeros_split = 0;
      if(constdiv_lowering_mode == "auto")
      {
         leading_zeros_split = estimateKnownLeadingZeros(op0, static_cast<unsigned>(dataBitsize));
         if(leading_zeros_split >= static_cast<unsigned>(dataBitsize))
         {
            leading_zeros_split = static_cast<unsigned>(dataBitsize - 1);
         }
      }

      struct TableEstimate
      {
         bool linarch_candidate;
         bool btcd_candidate;
         double best_table_score;
         bool auto_prefers_table;
      };

      auto estimate_table = [&](unsigned long long divisor, const ConstDivOpCosts* div_costs_choice,
                                bool allow_auto_choice) -> TableEstimate {
         TableEstimate estimate{false, false, std::numeric_limits<double>::infinity(), false};
         if(divisor <= 1)
         {
            return estimate;
         }

         LinArchCosts local_linarch_costs{0.0, 0.0, 0U, 0U, false};
         bool local_linarch_candidate = false;
         if(divisor <= linarch_d_max && dataBitsize <= static_cast<int>(linarch_n_max))
         {
            const unsigned t = static_cast<unsigned>(ceil_log2(divisor));
            const unsigned in_budget = linarch_input_budget > t ? linarch_input_budget - t : 0U;
            unsigned k = in_budget >= linarch_k_min ? std::min(linarch_k_max, in_budget) : 0U;
            if(k > static_cast<unsigned>(dataBitsize))
            {
               k = static_cast<unsigned>(dataBitsize);
            }
            const unsigned in_bits = t + k;
            if(k >= linarch_k_min && in_bits <= linarch_input_budget)
            {
               const unsigned data_bits = static_cast<unsigned>(dataBitsize);
               const unsigned digits = (data_bits + k - 1) / k;
               const auto lut_cost_model = select_current_lut_cost_model(in_bits, max_lut_size);
               local_linarch_costs =
                   estimateLinArchCosts(*this, AppM, divisor, data_bits, k, t, digits, max_lut_size, lut_cost_model);
               if(auto_lut_cost_model && lut_cost_model == "analytic" && !local_linarch_costs.valid)
               {
                  local_linarch_costs =
                      estimateLinArchCosts(*this, AppM, divisor, data_bits, k, t, digits, max_lut_size, "mockturtle");
               }
               local_linarch_candidate = local_linarch_costs.valid;
            }
         }

         BtcdCosts local_btcd_costs{0.0, 0.0, 0U, 0U, 0U, false};
         bool local_btcd_candidate = false;
         if(dataBitsize <= static_cast<int>(btcd_n_max))
         {
            const unsigned r = static_cast<unsigned>(ceil_log2(divisor));
            if(btcd_input_budget > 0 && (2U * r) <= btcd_input_budget)
            {
               const unsigned data_bits = static_cast<unsigned>(dataBitsize);
               const unsigned k_limit = std::min({btcd_k_max, btcd_k_input_limit, data_bits});
               double best_score = std::numeric_limits<double>::infinity();
               for(unsigned k = btcd_k_min; k <= k_limit; ++k)
               {
                  const auto costs = estimate_btcd_costs(divisor, data_bits, k, r);
                  if(!costs.valid)
                  {
                     continue;
                  }
                  const double cand_score = getConstMultDivScore(costs.delay, costs.area);
                  if(cand_score < best_score)
                  {
                     best_score = cand_score;
                     local_btcd_costs = costs;
                     local_btcd_candidate = true;
                  }
               }
            }
         }

         const double linarch_score = local_linarch_candidate ?
                                          getConstMultDivScore(local_linarch_costs.delay, local_linarch_costs.area) :
                                          std::numeric_limits<double>::infinity();
         const double btcd_score = local_btcd_candidate ?
                                       getConstMultDivScore(local_btcd_costs.delay, local_btcd_costs.area) :
                                       std::numeric_limits<double>::infinity();
         estimate.linarch_candidate = local_linarch_candidate;
         estimate.btcd_candidate = local_btcd_candidate;
         estimate.best_table_score = std::min(linarch_score, btcd_score);

         if(constdiv_lowering_mode == "auto" && allow_auto_choice && div_costs_choice)
         {
            const bool can_use_local_magic = dataBitsize <= 64;
            UDivMagic64 local_magic{0ULL, 0U, 0U, false};
            if(can_use_local_magic)
            {
               local_magic = computeUDivMagic64(divisor, static_cast<unsigned>(dataBitsize), leading_zeros_split, true);
            }
            const unsigned llvm_adds = local_magic.isAdd ? 2U : 0U;
            const double llvm_latency =
                can_use_local_magic ? div_costs_choice->mult_delay + llvm_adds * div_costs_choice->add_delay : 0.0;
            const double llvm_mul_area =
                can_use_local_magic ? getWeightedMultArea(hls_d, *div_costs_choice, constdiv_dsp_scale_k) : 0.0;
            const double llvm_area = can_use_local_magic ? llvm_mul_area + llvm_adds * div_costs_choice->add_area : 0.0;
            const double llvm_score = can_use_local_magic ? getConstMultDivScore(llvm_latency, llvm_area) :
                                                            std::numeric_limits<double>::infinity();
            const double keep_score = getConstMultDivScore(div_costs_choice->div_delay, div_costs_choice->div_area);
            double best_score_auto = keep_score;
            enum class Choice
            {
               Keep,
               Magic,
               Linarch,
               Btcd
            };
            Choice choice = Choice::Keep;
            if(llvm_score < best_score_auto)
            {
               best_score_auto = llvm_score;
               choice = Choice::Magic;
            }
            if(linarch_score < best_score_auto)
            {
               best_score_auto = linarch_score;
               choice = Choice::Linarch;
            }
            if(btcd_score < best_score_auto)
            {
               best_score_auto = btcd_score;
               choice = Choice::Btcd;
            }
            estimate.auto_prefers_table = (choice == Choice::Linarch || choice == Choice::Btcd);
         }

         return estimate;
      };

      const ConstDivOpCosts* div_choice_costs = constdiv_lowering_mode == "auto" ? &div_costs_div : nullptr;
      const auto atomic_estimate = estimate_table(d, div_choice_costs, true);
      if(std::isfinite(atomic_estimate.best_table_score))
      {
         struct FactorEstimate
         {
            TableEstimate div_est;
            TableEstimate mod_est;
            bool allowed;
         };

         auto evaluate_factor = [&](unsigned long long divisor) -> FactorEstimate {
            FactorEstimate factor;
            factor.div_est = estimate_table(divisor, div_choice_costs, true);
            factor.mod_est = factor.div_est;
            if(remFlag && constdiv_lowering_mode == "auto")
            {
               factor.mod_est = estimate_table(divisor, &div_costs_mod, true);
            }

            bool allowed = false;
            if(constdiv_lowering_mode == "auto")
            {
               allowed = factor.div_est.auto_prefers_table;
               if(remFlag)
               {
                  allowed = allowed && factor.mod_est.auto_prefers_table;
               }
            }
            else if(constdiv_lowering_mode == "linarch")
            {
               allowed = factor.div_est.linarch_candidate;
            }
            else if(constdiv_lowering_mode == "btcd")
            {
               allowed = factor.div_est.btcd_candidate;
            }
            factor.allowed = allowed;
            return factor;
         };

         auto total_score_for = [&](const FactorEstimate& first, const FactorEstimate& second) -> double {
            if(remFlag)
            {
               return first.div_est.best_table_score + first.mod_est.best_table_score +
                      second.div_est.best_table_score + second.mod_est.best_table_score + overhead_score;
            }
            return first.div_est.best_table_score + second.div_est.best_table_score;
         };

         const auto table_allowed = [&](const FactorEstimate& factor) -> bool {
            return factor.allowed && std::isfinite(factor.div_est.best_table_score);
         };

         double best_split_score = std::numeric_limits<double>::infinity();
         unsigned long long best_first = 0;
         unsigned long long best_second = 0;
         unsigned pairs_seen = 0;
         for(unsigned long long da = 2; da * da <= d; ++da)
         {
            if(d % da != 0)
            {
               continue;
            }
            ++pairs_seen;
            if(pairs_seen > constdiv_composite_max_pairs)
            {
               break;
            }
            const unsigned long long db = d / da;
            if(POWER2_OR_0(da) || POWER2_OR_0(db))
            {
               continue;
            }
            const auto factor_a = evaluate_factor(da);
            const auto factor_b = evaluate_factor(db);
            if(!table_allowed(factor_a) || !table_allowed(factor_b))
            {
               continue;
            }
            const double total_ab = total_score_for(factor_a, factor_b);
            if(total_ab < best_split_score)
            {
               best_split_score = total_ab;
               best_first = da;
               best_second = db;
            }
            const double total_ba = total_score_for(factor_b, factor_a);
            if(total_ba < best_split_score)
            {
               best_split_score = total_ba;
               best_first = db;
               best_second = da;
            }
         }

         if(best_first > 1 && best_second > 1)
         {
            INDENT_OUT_MEX(
                OUTPUT_LEVEL_MINIMUM, output_level,
                "---constdiv composite(u): D=" + STR(d) + " best_atomic=" + STR(atomic_estimate.best_table_score) +
                    " best_split=" + STR(best_split_score) + " Da=" + STR(best_first) + " Db=" + STR(best_second));
         }

         if(best_first > 1 && best_second > 1 &&
            best_split_score + constdiv_composite_margin < atomic_estimate.best_table_score)
         {
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                           "---constdiv composite(u): split D=" + STR(d) + " as " + STR(best_first) + "*" +
                               STR(best_second));

            auto create_binary_assign = [&](const ir_nodeRef& lhs, const ir_nodeRef& rhs, enum kind code) {
               const auto expr = ir_man->create_binary_operation(typeExpr, lhs, rhs, locInfoDefault, code);
               const auto ga_expr =
                   ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), expr, function_id, locInfoDefault);
               block.second->PushBefore(ga_expr, *itLos, AppM);
               return GetPointer<assign_stmt>(ga_expr)->op0;
            };

            const auto const_first = TM->CreateUniqueIntegerCst(static_cast<long long>(best_first), typeExpr);
            const auto const_second = TM->CreateUniqueIntegerCst(static_cast<long long>(best_second), typeExpr);
            const auto q_a = create_binary_assign(op0, const_first, idiv_node_K);
            AppM->RegisterTransformation(stepName, *itLos);
            if(remFlag)
            {
               const auto r_a = create_binary_assign(op0, const_first, irem_node_K);
               const auto r_b = create_binary_assign(q_a, const_second, irem_node_K);
               const auto r_mul = create_binary_assign(r_b, const_first, mul_node_K);
               const auto r_out = create_binary_assign(r_mul, r_a, add_node_K);
               ga->op1 = r_out;
            }
            else
            {
               const auto q_out = create_binary_assign(q_a, const_second, idiv_node_K);
               ga->op1 = q_out;
            }
            restartAnalysis = true;
            return;
         }
      }
   }

   if(can_use_magic)
   {
      leading_zeros = estimateKnownLeadingZeros(op0, static_cast<unsigned>(dataBitsize));
      if(leading_zeros >= static_cast<unsigned>(dataBitsize))
      {
         leading_zeros = static_cast<unsigned>(dataBitsize - 1);
      }
      magic = computeUDivMagic64(d, static_cast<unsigned>(dataBitsize), leading_zeros, true);
   }
   if(constdiv_lowering_mode == "llvm_magic")
   {
      use_magic = can_use_magic;
      keep_div = !use_magic;
   }
   else if(constdiv_lowering_mode == "linarch")
   {
      use_linarch = linarch_candidate;
      keep_div = !use_linarch;
   }
   else if(constdiv_lowering_mode == "btcd")
   {
      use_btcd = btcd_candidate;
      keep_div = !use_btcd;
   }
   else if(constdiv_lowering_mode == "auto")
   {
      const auto decision_metric = getEffectiveConstMultDivDecisionMetric();
      const auto mul_area = getWeightedMultArea(hls_d, div_costs, constdiv_dsp_scale_k);
      const unsigned llvm_adds = magic.isAdd ? 2U : 0U;
      const double llvm_latency = can_use_magic ? div_costs.mult_delay + llvm_adds * div_costs.add_delay : 0.0;
      const double llvm_area = can_use_magic ? mul_area + llvm_adds * div_costs.add_area : 0.0;
      const double keep_latency = div_costs.div_delay;
      const double keep_area = div_costs.div_area;
      const double llvm_score =
          can_use_magic ? getConstMultDivScore(llvm_latency, llvm_area) : std::numeric_limits<double>::infinity();
      const double keep_score = getConstMultDivScore(keep_latency, keep_area);
      const double linarch_score = linarch_candidate ? getConstMultDivScore(linarch_costs.delay, linarch_costs.area) :
                                                       std::numeric_limits<double>::infinity();
      const double btcd_score = btcd_candidate ? getConstMultDivScore(btcd_costs.delay, btcd_costs.area) :
                                                 std::numeric_limits<double>::infinity();
      const double keep_proxy = std::max(1.0, keep_area / std::max(1e-9, div_costs.add_area));
      const double llvm_proxy =
          can_use_magic ? static_cast<double>(1U + llvm_adds) : std::numeric_limits<double>::infinity();
      const double linarch_proxy = linarch_candidate ?
                                       std::max(1.0, linarch_costs.area / std::max(1e-9, div_costs.add_area)) :
                                       std::numeric_limits<double>::infinity();
      const double btcd_proxy = btcd_candidate ? std::max(1.0, btcd_costs.area / std::max(1e-9, div_costs.add_area)) :
                                                 std::numeric_limits<double>::infinity();

      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "---constdiv auto(u): llvm=" + STR(llvm_score) + " keep=" + STR(keep_score) +
                         " linarch=" + STR(linarch_score) + " btcd=" + STR(btcd_score));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---constdiv auto(u) proxy: llvm=" + STR(llvm_proxy) + " keep=" + STR(keep_proxy) +
                         " linarch=" + STR(linarch_proxy) + " btcd=" + STR(btcd_proxy) + " metric=" + decision_metric);

      double best_score = keep_score;
      double best_proxy = keep_proxy;
      enum class Choice
      {
         Keep,
         Magic,
         Linarch,
         Btcd
      };
      Choice choice = Choice::Keep;
      if(better_multdiv_candidate(decision_metric, llvm_score, llvm_proxy, best_score, best_proxy))
      {
         best_score = llvm_score;
         best_proxy = llvm_proxy;
         choice = Choice::Magic;
      }
      if(better_multdiv_candidate(decision_metric, linarch_score, linarch_proxy, best_score, best_proxy))
      {
         best_score = linarch_score;
         best_proxy = linarch_proxy;
         choice = Choice::Linarch;
      }
      if(better_multdiv_candidate(decision_metric, btcd_score, btcd_proxy, best_score, best_proxy))
      {
         best_score = btcd_score;
         best_proxy = btcd_proxy;
         choice = Choice::Btcd;
      }

      if(choice == Choice::Keep)
      {
         keep_div = true;
      }
      else if(choice == Choice::Linarch)
      {
         use_linarch = linarch_candidate;
      }
      else if(choice == Choice::Btcd)
      {
         use_btcd = btcd_candidate;
      }
      else
      {
         use_magic = can_use_magic;
      }
   }
   else
   {
      keep_div = true;
   }

   if(keep_div)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "---constdiv(u): keep division D=" + STR(d) + " bits=" + STR(dataBitsize));
      return;
   }

   if(use_linarch)
   {
      const auto& net = getLinArchStepNetwork(d, linarch_k, linarch_t);
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "---constdiv(u): linarch D=" + STR(d) + " bits=" + STR(dataBitsize) + " t=" + STR(linarch_t) +
                         " k=" + STR(linarch_k) + " in_bits=" + STR(linarch_in_bits) +
                         " digits=" + STR(linarch_digits) + " max_lut=" + STR(max_lut_size));

      const auto const_zero = TM->CreateUniqueIntegerCst(0, typeExpr);
      const auto const_one = TM->CreateUniqueIntegerCst(1, typeExpr);
      const auto digit_mask = TM->CreateUniqueIntegerCst(static_cast<long long>((1ULL << linarch_k) - 1ULL), typeExpr);

      auto create_binary_assign = [&](const ir_nodeRef& lhs, const ir_nodeRef& rhs, enum kind code) {
         const auto expr = ir_man->create_binary_operation(typeExpr, lhs, rhs, locInfoDefault, code);
         const auto ga_expr =
             ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), expr, function_id, locInfoDefault);
         block.second->PushBefore(ga_expr, *itLos, AppM);
         return GetPointer<assign_stmt>(ga_expr)->op0;
      };

      auto create_shift_assign = [&](const ir_nodeRef& value, unsigned shift, enum kind code) {
         const auto shift_node = TM->CreateUniqueIntegerCst(static_cast<long long>(shift), typeExpr);
         const auto shift_expr = createShiftOperationWithCostGuard(ir_man, typeExpr, value, shift_node, locInfoDefault,
                                                                   code, "lowerUnsignedTruncDivModByConstant");
         const auto ga_expr =
             ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), shift_expr, function_id, locInfoDefault);
         block.second->PushBefore(ga_expr, *itLos, AppM);
         return GetPointer<assign_stmt>(ga_expr)->op0;
      };

      auto extract_bit = [&](const ir_nodeRef& value, unsigned shift) -> ir_nodeRef {
         if(shift >= static_cast<unsigned>(dataBitsize))
         {
            return const_zero;
         }
         const auto shifted = shift == 0 ? value : create_shift_assign(value, shift, shr_node_K);
         return create_binary_assign(shifted, const_one, and_node_K);
      };

      auto pack_bits = [&](const std::vector<ir_nodeRef>& bits, unsigned offset, unsigned count) -> ir_nodeRef {
         ir_nodeRef packed = const_zero;
         for(unsigned i = 0; i < count; ++i)
         {
            const auto bit = bits[offset + i];
            const auto shifted = i == 0 ? bit : create_shift_assign(bit, i, shl_node_K);
            if(i == 0)
            {
               packed = shifted;
            }
            else
            {
               packed = create_binary_assign(packed, shifted, or_node_K);
            }
         }
         return packed;
      };

      auto emit_aig = [&](const mockturtle::aig_network& aig,
                          const std::vector<ir_nodeRef>& inputs) -> std::vector<ir_nodeRef> {
         std::map<mockturtle::aig_network::node, ir_nodeRef> node_map;
         unsigned pi_index = 0;
         aig.foreach_pi([&](const auto& node) {
            if(pi_index < inputs.size())
            {
               node_map[node] = inputs[pi_index];
            }
            ++pi_index;
         });

         auto resolve_signal = [&](mockturtle::aig_network::signal s) -> ir_nodeRef {
            if(s.index == 0)
            {
               return s.complement ? const_one : const_zero;
            }
            const auto it = node_map.find(s.index);
            if(it == node_map.end())
            {
               return s.complement ? const_one : const_zero;
            }
            if(s.complement)
            {
               return create_binary_assign(it->second, const_one, xor_node_K);
            }
            return it->second;
         };

         mockturtle::topo_view topo{aig};
         topo.foreach_node([&](const auto& node) {
            if(aig.is_constant(node) || aig.is_pi(node))
            {
               return true;
            }
            mockturtle::aig_network::signal fanins[2];
            aig.foreach_fanin(node, [&](const mockturtle::aig_network::signal& s, unsigned index) {
               if(index < 2)
               {
                  fanins[index] = s;
               }
            });
            const auto lhs = resolve_signal(fanins[0]);
            const auto rhs = resolve_signal(fanins[1]);
            node_map[node] = create_binary_assign(lhs, rhs, and_node_K);
            return true;
         });

         std::vector<ir_nodeRef> outputs;
         aig.foreach_po([&](const auto& s) { outputs.push_back(resolve_signal(s)); });
         return outputs;
      };

      ir_nodeRef r_value = const_zero;
      ir_nodeRef q_value = const_zero;
      for(unsigned di = 0; di < linarch_digits; ++di)
      {
         const unsigned digit_index = linarch_digits - di - 1;
         const unsigned shift = digit_index * linarch_k;
         ir_nodeRef digit_value = const_zero;
         if(shift < static_cast<unsigned>(dataBitsize))
         {
            const auto shifted = shift == 0 ? op0 : create_shift_assign(op0, shift, shr_node_K);
            digit_value = linarch_k >= static_cast<unsigned>(dataBitsize) ?
                              shifted :
                              create_binary_assign(shifted, digit_mask, and_node_K);
         }

         std::vector<ir_nodeRef> step_inputs;
         step_inputs.reserve(linarch_t + linarch_k);
         for(unsigned i = 0; i < linarch_t; ++i)
         {
            step_inputs.push_back(extract_bit(r_value, i));
         }
         for(unsigned i = 0; i < linarch_k; ++i)
         {
            step_inputs.push_back(extract_bit(digit_value, i));
         }

         const auto step_outputs = emit_aig(net.aig, step_inputs);
         if(step_outputs.size() == net.out_bits)
         {
            const auto q_digit = pack_bits(step_outputs, 0, linarch_k);
            const auto r_next = pack_bits(step_outputs, linarch_k, linarch_t);
            ir_nodeRef q_shifted = const_zero;
            if(linarch_k < static_cast<unsigned>(dataBitsize))
            {
               q_shifted = create_shift_assign(q_value, linarch_k, shl_node_K);
            }
            q_value = create_binary_assign(q_shifted, q_digit, or_node_K);
            r_value = r_next;
         }
      }

      AppM->RegisterTransformation(stepName, *itLos);
      ga->op1 = remFlag ? r_value : q_value;
      restartAnalysis = true;
      return;
   }

   if(use_btcd)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "---constdiv(u): btcd D=" + STR(d) + " bits=" + STR(dataBitsize) + " r=" + STR(btcd_r) +
                         " k=" + STR(btcd_k) + " digits=" + STR(btcd_digits) + " max_lut=" + STR(max_lut_size));

      const auto const_zero = TM->CreateUniqueIntegerCst(0, typeExpr);
      const auto const_one = TM->CreateUniqueIntegerCst(1, typeExpr);
      const auto digit_mask = TM->CreateUniqueIntegerCst(static_cast<long long>((1ULL << btcd_k) - 1ULL), typeExpr);

      auto create_binary_assign = [&](const ir_nodeRef& lhs, const ir_nodeRef& rhs, enum kind code) {
         const auto expr = ir_man->create_binary_operation(typeExpr, lhs, rhs, locInfoDefault, code);
         const auto ga_expr =
             ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), expr, function_id, locInfoDefault);
         block.second->PushBefore(ga_expr, *itLos, AppM);
         return GetPointer<assign_stmt>(ga_expr)->op0;
      };

      auto create_shift_assign = [&](const ir_nodeRef& value, unsigned shift, enum kind code) {
         const auto shift_node = TM->CreateUniqueIntegerCst(static_cast<long long>(shift), typeExpr);
         const auto shift_expr = createShiftOperationWithCostGuard(ir_man, typeExpr, value, shift_node, locInfoDefault,
                                                                   code, "lowerUnsignedTruncDivModByConstant");
         const auto ga_expr =
             ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), shift_expr, function_id, locInfoDefault);
         block.second->PushBefore(ga_expr, *itLos, AppM);
         return GetPointer<assign_stmt>(ga_expr)->op0;
      };

      auto extract_bit = [&](const ir_nodeRef& value, unsigned shift) -> ir_nodeRef {
         if(shift >= static_cast<unsigned>(dataBitsize))
         {
            return const_zero;
         }
         const auto shifted = shift == 0 ? value : create_shift_assign(value, shift, shr_node_K);
         return create_binary_assign(shifted, const_one, and_node_K);
      };

      auto pack_bits = [&](const std::vector<ir_nodeRef>& bits, unsigned offset, unsigned count) -> ir_nodeRef {
         ir_nodeRef packed = const_zero;
         for(unsigned i = 0; i < count; ++i)
         {
            const auto bit = bits[offset + i];
            const auto shifted = i == 0 ? bit : create_shift_assign(bit, i, shl_node_K);
            if(i == 0)
            {
               packed = shifted;
            }
            else
            {
               packed = create_binary_assign(packed, shifted, or_node_K);
            }
         }
         return packed;
      };

      auto emit_aig = [&](const mockturtle::aig_network& aig,
                          const std::vector<ir_nodeRef>& inputs) -> std::vector<ir_nodeRef> {
         std::map<mockturtle::aig_network::node, ir_nodeRef> node_map;
         unsigned pi_index = 0;
         aig.foreach_pi([&](const auto& node) {
            if(pi_index < inputs.size())
            {
               node_map[node] = inputs[pi_index];
            }
            ++pi_index;
         });

         auto resolve_signal = [&](mockturtle::aig_network::signal s) -> ir_nodeRef {
            if(s.index == 0)
            {
               return s.complement ? const_one : const_zero;
            }
            const auto it = node_map.find(s.index);
            if(it == node_map.end())
            {
               return s.complement ? const_one : const_zero;
            }
            if(s.complement)
            {
               return create_binary_assign(it->second, const_one, xor_node_K);
            }
            return it->second;
         };

         mockturtle::topo_view topo{aig};
         topo.foreach_node([&](const auto& node) {
            if(aig.is_constant(node) || aig.is_pi(node))
            {
               return true;
            }
            mockturtle::aig_network::signal fanins[2];
            aig.foreach_fanin(node, [&](const mockturtle::aig_network::signal& s, unsigned index) {
               if(index < 2)
               {
                  fanins[index] = s;
               }
            });
            const auto lhs = resolve_signal(fanins[0]);
            const auto rhs = resolve_signal(fanins[1]);
            node_map[node] = create_binary_assign(lhs, rhs, and_node_K);
            return true;
         });

         std::vector<ir_nodeRef> outputs;
         aig.foreach_po([&](const auto& s) { outputs.push_back(resolve_signal(s)); });
         return outputs;
      };

      struct BtcdNode
      {
         ir_nodeRef q;
         ir_nodeRef r;
         unsigned digits;
      };

      const unsigned long long leaf_count = std::max(1ULL, ceil_pow2(static_cast<unsigned long long>(btcd_digits)));
      std::vector<BtcdNode> nodes;
      nodes.reserve(static_cast<size_t>(leaf_count));

      for(unsigned long long idx = 0; idx < leaf_count; ++idx)
      {
         if(idx >= btcd_digits)
         {
            nodes.emplace_back(BtcdNode{const_zero, const_zero, 1U});
            continue;
         }
         const unsigned shift = static_cast<unsigned>(idx) * btcd_k;
         ir_nodeRef digit_value = const_zero;
         if(shift < static_cast<unsigned>(dataBitsize))
         {
            const auto shifted = shift == 0 ? op0 : create_shift_assign(op0, shift, shr_node_K);
            digit_value = btcd_k >= static_cast<unsigned>(dataBitsize) ?
                              shifted :
                              create_binary_assign(shifted, digit_mask, and_node_K);
         }

         const auto& i_net = getBtcdILutNetwork(d, btcd_k, btcd_r);
         std::vector<ir_nodeRef> inputs;
         inputs.reserve(btcd_k);
         for(unsigned i = 0; i < btcd_k; ++i)
         {
            inputs.push_back(extract_bit(digit_value, i));
         }
         const auto outputs = emit_aig(i_net.aig, inputs);
         const auto q_leaf = pack_bits(outputs, 0, btcd_k);
         const auto r_leaf = pack_bits(outputs, btcd_k, btcd_r);
         nodes.emplace_back(BtcdNode{q_leaf, r_leaf, 1U});
      }

      while(nodes.size() > 1)
      {
         std::vector<BtcdNode> next;
         next.reserve((nodes.size() + 1) / 2);
         for(size_t i = 0; i < nodes.size(); i += 2)
         {
            if(i + 1 >= nodes.size())
            {
               next.push_back(nodes[i]);
               continue;
            }
            const auto& lo = nodes[i];
            const auto& hi = nodes[i + 1];
            const unsigned shift_bits = lo.digits * btcd_k;
            const auto& r_net = getBtcdRLutNetwork(d, btcd_r, shift_bits);
            std::vector<ir_nodeRef> inputs;
            inputs.reserve(2U * btcd_r);
            for(unsigned bit = 0; bit < btcd_r; ++bit)
            {
               inputs.push_back(extract_bit(lo.r, bit));
            }
            for(unsigned bit = 0; bit < btcd_r; ++bit)
            {
               inputs.push_back(extract_bit(hi.r, bit));
            }
            const auto outputs = emit_aig(r_net.aig, inputs);
            const auto q_rem = pack_bits(outputs, 0, shift_bits);
            const auto r_out = pack_bits(outputs, shift_bits, btcd_r);

            ir_nodeRef hi_shifted = const_zero;
            if(shift_bits == 0)
            {
               hi_shifted = hi.q;
            }
            else if(shift_bits < static_cast<unsigned>(dataBitsize))
            {
               hi_shifted = create_shift_assign(hi.q, shift_bits, shl_node_K);
            }

            const auto q_sum = create_binary_assign(hi_shifted, lo.q, add_node_K);
            const auto q_out = create_binary_assign(q_sum, q_rem, add_node_K);
            next.emplace_back(BtcdNode{q_out, r_out, lo.digits + hi.digits});
         }
         nodes.swap(next);
      }

      AppM->RegisterTransformation(stepName, *itLos);
      ga->op1 = remFlag ? nodes.front().r : nodes.front().q;
      restartAnalysis = true;
      return;
   }

   if(use_magic)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "---constdiv(u): llvm_magic D=" + STR(d) + " bits=" + STR(dataBitsize) + " magic=" +
                         STR(magic.magic) + " preShift=" + STR(magic.preShift) + " postShift=" + STR(magic.postShift) +
                         " isAdd=" + std::string(magic.isAdd ? "1" : "0") + " lz=" + STR(leading_zeros));
      // LLVM-style magic-number sequence for unsigned div/mod:
      // Q = op0; if PreShift: Q >>= PreShift; Q = MULHU(Q, Magic);
      // if IsAdd: NPQ = (op0 - Q) >> 1; Q = NPQ + Q; if PostShift: Q >>= PostShift.
      ir_nodeRef quotientExpr = op0;
      if(magic.preShift != 0)
      {
         const auto preShiftNode = TM->CreateUniqueIntegerCst(static_cast<long long>(magic.preShift), typeExpr);
         const auto preShiftExpr =
             createShiftOperationWithCostGuard(ir_man, typeExpr, quotientExpr, preShiftNode, locInfoDefault, shr_node_K,
                                               "lowerUnsignedTruncDivModByConstant");
         const auto preShiftGa =
             ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), preShiftExpr, function_id, locInfoDefault);
         block.second->PushBefore(preShiftGa, *itLos, AppM);
         quotientExpr = GetPointer<assign_stmt>(preShiftGa)->op0;
      }

      quotientExpr =
          expand_mult_highpart(quotientExpr, magic.magic, typeExpr, dataBitsize, itLos, block.second, locInfoDefault);

      if(magic.isAdd)
      {
         const auto npqExpr = ir_man->create_binary_operation(typeExpr, op0, quotientExpr, locInfoDefault, sub_node_K);
         const auto npqGa =
             ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), npqExpr, function_id, locInfoDefault);
         block.second->PushBefore(npqGa, *itLos, AppM);
         const auto npqVar = GetPointer<assign_stmt>(npqGa)->op0;

         const auto shiftNode = TM->CreateUniqueIntegerCst(1, typeExpr);
         const auto npqShiftExpr = createShiftOperationWithCostGuard(
             ir_man, typeExpr, npqVar, shiftNode, locInfoDefault, shr_node_K, "lowerUnsignedTruncDivModByConstant");
         const auto npqShiftGa =
             ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), npqShiftExpr, function_id, locInfoDefault);
         block.second->PushBefore(npqShiftGa, *itLos, AppM);
         const auto npqShiftVar = GetPointer<assign_stmt>(npqShiftGa)->op0;

         const auto qAddExpr =
             ir_man->create_binary_operation(typeExpr, npqShiftVar, quotientExpr, locInfoDefault, add_node_K);
         const auto qAddGa =
             ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), qAddExpr, function_id, locInfoDefault);
         block.second->PushBefore(qAddGa, *itLos, AppM);
         quotientExpr = GetPointer<assign_stmt>(qAddGa)->op0;
      }

      if(magic.postShift != 0)
      {
         const auto postShiftNode = TM->CreateUniqueIntegerCst(static_cast<long long>(magic.postShift), typeExpr);
         const auto postShiftExpr =
             createShiftOperationWithCostGuard(ir_man, typeExpr, quotientExpr, postShiftNode, locInfoDefault,
                                               shr_node_K, "lowerUnsignedTruncDivModByConstant");
         const auto postShiftGa =
             ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), postShiftExpr, function_id, locInfoDefault);
         block.second->PushBefore(postShiftGa, *itLos, AppM);
         quotientExpr = GetPointer<assign_stmt>(postShiftGa)->op0;
      }

      AppM->RegisterTransformation(stepName, *itLos);
      if(remFlag)
      {
         assignRemainderFromQuotient(block, itLos, ga, op0, op1, typeExpr, quotientExpr, locInfoDefault, stepName, true,
                                     static_cast<unsigned int>(dataBitsize));
      }
      else
      {
         ga->op1 = quotientExpr;
      }
      restartAnalysis = true;
      return;
   }
}

void IR_lowering::lowerSignedTruncDivModByConstant(const std::pair<unsigned int, blocRef>& block,
                                                   std::list<ir_nodeRef>::const_iterator& itLos, assign_stmt* ga,
                                                   const ir_nodeRef& op0, const ir_nodeRef& op1,
                                                   const ir_nodeRef& typeExpr, long long extOp1, bool remFlag,
                                                   const std::string& locInfoDefault, const std::string& stepName,
                                                   bool& restartAnalysis)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---signed");
   ir_nodeRef newOp1;
   const auto d = extOp1;
   const auto absD = d >= 0 ? static_cast<unsigned long long int>(d) :
                              static_cast<unsigned long long int>(0ULL - static_cast<unsigned long long int>(d));

   const auto size = ir_helper::Size(typeExpr);
   if(absD == (1ull << (size - 1)))
   {
      if(AppM->ApplyNewTransformation())
      {
         const auto bt = ir_man->GetBooleanType();
         const auto quotientExpr = ir_man->create_binary_operation(bt, op0, op1, locInfoDefault, eq_node_K);
         const auto quotientGa =
             ir_man->CreateAssignStmt(bt, TM->CreateUniqueIntegerCst(0, bt), TM->CreateUniqueIntegerCst(1, bt),
                                      quotientExpr, function_id, locInfoDefault);
         block.second->PushBefore(quotientGa, *itLos, AppM);
         const auto quotientGaVar = GetPointerS<assign_stmt>(quotientGa)->op0;
         const auto quotientNopExpr =
             ir_man->create_unary_operation(typeExpr, quotientGaVar, locInfoDefault, nop_node_K);
         if(remFlag)
         {
            const auto quotientNop = ir_man->CreateAssignStmt(typeExpr, TM->CreateUniqueIntegerCst(0, typeExpr),
                                                              TM->CreateUniqueIntegerCst(1, typeExpr), quotientNopExpr,
                                                              function_id, locInfoDefault);
            block.second->PushBefore(quotientNop, *itLos, AppM);
            const auto quotientNopVar = GetPointerS<assign_stmt>(quotientNop)->op0;
            const auto mulExpr =
                ir_man->create_binary_operation(typeExpr, quotientNopVar, op1, locInfoDefault, mul_node_K);
            const auto mulGa =
                ir_man->CreateAssignStmt(typeExpr, nullptr, nullptr, mulExpr, function_id, locInfoDefault);
            block.second->PushBefore(mulGa, *itLos, AppM);
            const auto mulGaVar = GetPointerS<assign_stmt>(mulGa)->op0;
            const auto subExpr = ir_man->create_binary_operation(typeExpr, op0, mulGaVar, locInfoDefault, sub_node_K);
            ga->op1 = subExpr;
         }
         else
         {
            ga->op1 = quotientNopExpr;
         }
         AppM->RegisterTransformation(stepName, *itLos);
         restartAnalysis = true;
      }
      return;
   }

   if(POWER2_OR_0(absD))
   {
      if(AppM->ApplyNewTransformation())
      {
         if(remFlag)
         {
            newOp1 = expand_smod_pow2(op0, absD, *itLos, block.second, typeExpr, locInfoDefault);
            ga->op1 = newOp1;
         }
         else
         {
            newOp1 = expand_sdiv_pow2(op0, absD, *itLos, block.second, typeExpr, locInfoDefault);
            /// We have computed OP0 / abs(OP1).  If OP1 is negative, negate the quotient.
            if(d < 0)
            {
               ir_nodeRef sdivPow2Ga =
                   ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), newOp1, function_id, locInfoDefault);
               block.second->PushBefore(sdivPow2Ga, *itLos, AppM);
               ir_nodeRef sdivPow2GaVar = GetPointer<assign_stmt>(sdivPow2Ga)->op0;
               newOp1 = ir_man->create_unary_operation(typeExpr, sdivPow2GaVar, locInfoDefault, neg_node_K);
            }
            ga->op1 = newOp1;
         }
         AppM->RegisterTransformation(stepName, *itLos);
         restartAnalysis = true;
      }
      return;
   }

   const auto dataBitsize = static_cast<int>(ir_helper::Size(typeExpr));
   if(dataBitsize > 64)
   {
      return;
   }
   const unsigned linarch_d_max = 255;
   const unsigned linarch_k_max = 6;
   const unsigned linarch_k_min = 2;
   const unsigned linarch_n_max = 64;
   const unsigned btcd_k_max = 6;
   const unsigned btcd_k_min = 2;
   const unsigned btcd_n_max = 64;
   const HLS_deviceRef hls_d = getHlsDeviceFromApp(AppM);
   const unsigned max_lut_size = resolveRequiredMaxLutSize(AppM, hls_d);
   const bool auto_lut_cost_model = is_auto_lut_cost_model(constmultdiv_lut_cost_model);
   const bool use_mockturtle_lut_estimator =
       auto_lut_cost_model || is_mockturtle_lut_cost_model(constmultdiv_lut_cost_model);
   const unsigned linarch_input_budget =
       use_mockturtle_lut_estimator ? std::max(max_lut_size, CONSTDIV_MOCKTURTLE_MAX_TT_INPUTS) : max_lut_size;
   const unsigned btcd_input_budget =
       use_mockturtle_lut_estimator ? std::max(max_lut_size, CONSTDIV_MOCKTURTLE_MAX_TT_INPUTS) : max_lut_size;
   const unsigned btcd_k_input_limit = use_mockturtle_lut_estimator ? btcd_k_max : max_lut_size;
   auto select_current_lut_cost_model = [&](unsigned in_bits, unsigned lut_limit) {
      return select_lut_cost_model(constmultdiv_lut_cost_model, in_bits, lut_limit);
   };

   const bool can_use_magic = dataBitsize <= 64 && d != 0 && d != 1 && d != -1;
   bool use_magic = false;
   bool keep_div = false;
   bool use_wrapper = false;
   SDivMagic64 magic{0ULL, 0U, 0, 0};
   if(can_use_magic)
   {
      magic = computeSDivMagic64(d, static_cast<unsigned>(dataBitsize));
   }

   unsigned linarch_k = 0;
   unsigned linarch_t = 0;
   unsigned linarch_digits = 0;
   unsigned linarch_in_bits = 0;
   LinArchCosts linarch_costs{0.0, 0.0, 0U, 0U, false};
   bool linarch_candidate = false;
   if((constdiv_lowering_mode == "auto" || constdiv_lowering_mode == "linarch") && absD > 1 && absD <= linarch_d_max &&
      dataBitsize <= static_cast<int>(linarch_n_max))
   {
      linarch_t = static_cast<unsigned>(ceil_log2(absD));
      const unsigned in_budget = linarch_input_budget > linarch_t ? linarch_input_budget - linarch_t : 0U;
      linarch_k = in_budget >= linarch_k_min ? std::min(linarch_k_max, in_budget) : 0U;
      if(linarch_k > static_cast<unsigned>(dataBitsize))
      {
         linarch_k = static_cast<unsigned>(dataBitsize);
      }
      linarch_in_bits = linarch_t + linarch_k;
      if(linarch_k >= linarch_k_min && linarch_in_bits <= linarch_input_budget)
      {
         const unsigned data_bits = static_cast<unsigned>(dataBitsize);
         linarch_digits = (data_bits + linarch_k - 1) / linarch_k;
         const auto lut_cost_model = select_current_lut_cost_model(linarch_in_bits, max_lut_size);
         linarch_costs = estimateLinArchCosts(*this, AppM, absD, data_bits, linarch_k, linarch_t, linarch_digits,
                                              max_lut_size, lut_cost_model);
         if(auto_lut_cost_model && lut_cost_model == "analytic" && !linarch_costs.valid)
         {
            linarch_costs = estimateLinArchCosts(*this, AppM, absD, data_bits, linarch_k, linarch_t, linarch_digits,
                                                 max_lut_size, "mockturtle");
         }
         linarch_candidate = linarch_costs.valid;
      }
   }

   ConstDivOpCosts div_costs = estimateConstDivCosts(*this, AppM, static_cast<unsigned>(dataBitsize), remFlag);
   auto estimate_btcd_costs = [&](unsigned long long divisor, unsigned data_bits, unsigned k, unsigned r) -> BtcdCosts {
      const auto lut_cost_model = select_current_lut_cost_model(2U * r, max_lut_size);
      auto costs = estimateBtcdCosts(*this, AppM, divisor, data_bits, k, r, max_lut_size, div_costs.add_delay,
                                     div_costs.add_area, lut_cost_model);
      if(auto_lut_cost_model && lut_cost_model == "analytic" && !costs.valid)
      {
         costs = estimateBtcdCosts(*this, AppM, divisor, data_bits, k, r, max_lut_size, div_costs.add_delay,
                                   div_costs.add_area, "mockturtle");
      }
      return costs;
   };

   bool btcd_candidate = false;
   unsigned btcd_r = 0;
   BtcdCosts btcd_costs{0.0, 0.0, 0U, 0U, 0U, false};
   if((constdiv_lowering_mode == "auto" || constdiv_lowering_mode == "btcd") && absD > 1 &&
      dataBitsize <= static_cast<int>(btcd_n_max))
   {
      btcd_r = static_cast<unsigned>(ceil_log2(absD));
      if(btcd_input_budget > 0 && (2U * btcd_r) <= btcd_input_budget)
      {
         const unsigned data_bits = static_cast<unsigned>(dataBitsize);
         const unsigned k_limit = std::min({btcd_k_max, btcd_k_input_limit, data_bits});
         double best_score = std::numeric_limits<double>::infinity();
         for(unsigned k = btcd_k_min; k <= k_limit; ++k)
         {
            const auto costs = estimate_btcd_costs(absD, data_bits, k, btcd_r);
            if(!costs.valid)
            {
               continue;
            }
            const double cand_score = getConstMultDivScore(costs.delay, costs.area);
            if(cand_score < best_score)
            {
               best_score = cand_score;
               btcd_costs = costs;
               btcd_candidate = true;
            }
         }
      }
   }

   if(constdiv_lowering_mode == "llvm_magic")
   {
      use_magic = can_use_magic;
      keep_div = !use_magic;
   }
   else if(constdiv_lowering_mode == "linarch")
   {
      use_wrapper = linarch_candidate;
      keep_div = !use_wrapper;
   }
   else if(constdiv_lowering_mode == "btcd")
   {
      use_wrapper = btcd_candidate;
      keep_div = !use_wrapper;
   }
   else if(constdiv_lowering_mode == "auto")
   {
      const auto decision_metric = getEffectiveConstMultDivDecisionMetric();
      const auto mul_area = getWeightedMultArea(hls_d, div_costs, constdiv_dsp_scale_k);
      const unsigned llvm_adds = (magic.numeratorFactor != 0 ? 1U : 0U) + 2U;
      const double llvm_latency = can_use_magic ? div_costs.mult_delay + llvm_adds * div_costs.add_delay : 0.0;
      const double llvm_area = can_use_magic ? mul_area + llvm_adds * div_costs.add_area : 0.0;
      const double llvm_score =
          can_use_magic ? getConstMultDivScore(llvm_latency, llvm_area) : std::numeric_limits<double>::infinity();
      const double keep_score = getConstMultDivScore(div_costs.div_delay, div_costs.div_area);
      const double keep_proxy = std::max(1.0, div_costs.div_area / std::max(1e-9, div_costs.add_area));
      const double llvm_proxy =
          can_use_magic ? static_cast<double>(1U + llvm_adds) : std::numeric_limits<double>::infinity();

      ConstDivOpCosts div_costs_div = div_costs;
      ConstDivOpCosts div_costs_mod = div_costs;
      if(remFlag)
      {
         div_costs_div = estimateConstDivCosts(*this, AppM, static_cast<unsigned>(dataBitsize), false);
      }
      else
      {
         div_costs_mod = estimateConstDivCosts(*this, AppM, static_cast<unsigned>(dataBitsize), true);
      }

      double unsigned_magic_score = std::numeric_limits<double>::infinity();
      double unsigned_table_score = std::numeric_limits<double>::infinity();
      double unsigned_magic_proxy = std::numeric_limits<double>::infinity();
      double unsigned_table_proxy = std::numeric_limits<double>::infinity();

      if(absD > 1)
      {
         const bool can_use_unsigned_magic = dataBitsize <= 64;
         if(can_use_unsigned_magic)
         {
            const unsigned leading_zeros = 0;
            const auto u_magic = computeUDivMagic64(absD, static_cast<unsigned>(dataBitsize), leading_zeros, true);
            const unsigned u_llvm_adds = u_magic.isAdd ? 2U : 0U;
            const double u_llvm_latency = div_costs.mult_delay + u_llvm_adds * div_costs.add_delay;
            const double u_llvm_area = mul_area + u_llvm_adds * div_costs.add_area;
            unsigned_magic_score = getConstMultDivScore(u_llvm_latency, u_llvm_area);
            unsigned_magic_proxy = static_cast<double>(1U + u_llvm_adds);
         }

         if(constdiv_composite_enable && !POWER2_OR_0(absD) && constdiv_composite_max_pairs > 0U)
         {
            const auto overhead_delay = div_costs.mult_delay + div_costs.add_delay;
            const auto overhead_area = mul_area + div_costs.add_area;
            const auto overhead_score = getConstMultDivScore(overhead_delay, overhead_area);
            const unsigned leading_zeros_split = 0;

            struct TableEstimate
            {
               bool linarch_candidate;
               bool btcd_candidate;
               double best_table_score;
               bool auto_prefers_table;
            };

            auto estimate_table = [&](unsigned long long divisor, const ConstDivOpCosts* div_costs_choice,
                                      bool allow_auto_choice) -> TableEstimate {
               TableEstimate estimate{false, false, std::numeric_limits<double>::infinity(), false};
               if(divisor <= 1)
               {
                  return estimate;
               }

               LinArchCosts local_linarch_costs{0.0, 0.0, 0U, 0U, false};
               bool local_linarch_candidate = false;
               if(divisor <= linarch_d_max && dataBitsize <= static_cast<int>(linarch_n_max))
               {
                  const unsigned t = static_cast<unsigned>(ceil_log2(divisor));
                  const unsigned in_budget = linarch_input_budget > t ? linarch_input_budget - t : 0U;
                  unsigned k = in_budget >= linarch_k_min ? std::min(linarch_k_max, in_budget) : 0U;
                  if(k > static_cast<unsigned>(dataBitsize))
                  {
                     k = static_cast<unsigned>(dataBitsize);
                  }
                  const unsigned in_bits = t + k;
                  if(k >= linarch_k_min && in_bits <= linarch_input_budget)
                  {
                     const unsigned data_bits = static_cast<unsigned>(dataBitsize);
                     const unsigned digits = (data_bits + k - 1) / k;
                     const auto lut_cost_model = select_current_lut_cost_model(in_bits, max_lut_size);
                     local_linarch_costs = estimateLinArchCosts(*this, AppM, divisor, data_bits, k, t, digits,
                                                                max_lut_size, lut_cost_model);
                     if(auto_lut_cost_model && lut_cost_model == "analytic" && !local_linarch_costs.valid)
                     {
                        local_linarch_costs = estimateLinArchCosts(*this, AppM, divisor, data_bits, k, t, digits,
                                                                   max_lut_size, "mockturtle");
                     }
                     local_linarch_candidate = local_linarch_costs.valid;
                  }
               }

               BtcdCosts local_btcd_costs{0.0, 0.0, 0U, 0U, 0U, false};
               bool local_btcd_candidate = false;
               if(dataBitsize <= static_cast<int>(btcd_n_max))
               {
                  const unsigned r = static_cast<unsigned>(ceil_log2(divisor));
                  if(btcd_input_budget > 0 && (2U * r) <= btcd_input_budget)
                  {
                     const unsigned data_bits = static_cast<unsigned>(dataBitsize);
                     const unsigned k_limit = std::min({btcd_k_max, btcd_k_input_limit, data_bits});
                     double best_score = std::numeric_limits<double>::infinity();
                     for(unsigned k = btcd_k_min; k <= k_limit; ++k)
                     {
                        const auto costs = estimate_btcd_costs(divisor, data_bits, k, r);
                        if(!costs.valid)
                        {
                           continue;
                        }
                        const double cand_score = getConstMultDivScore(costs.delay, costs.area);
                        if(cand_score < best_score)
                        {
                           best_score = cand_score;
                           local_btcd_costs = costs;
                           local_btcd_candidate = true;
                        }
                     }
                  }
               }

               const double linarch_score =
                   local_linarch_candidate ? getConstMultDivScore(local_linarch_costs.delay, local_linarch_costs.area) :
                                             std::numeric_limits<double>::infinity();
               const double btcd_score = local_btcd_candidate ?
                                             getConstMultDivScore(local_btcd_costs.delay, local_btcd_costs.area) :
                                             std::numeric_limits<double>::infinity();
               estimate.linarch_candidate = local_linarch_candidate;
               estimate.btcd_candidate = local_btcd_candidate;
               estimate.best_table_score = std::min(linarch_score, btcd_score);

               if(allow_auto_choice && div_costs_choice)
               {
                  const bool can_use_local_magic = dataBitsize <= 64;
                  UDivMagic64 local_magic{0ULL, 0U, 0U, false};
                  if(can_use_local_magic)
                  {
                     local_magic =
                         computeUDivMagic64(divisor, static_cast<unsigned>(dataBitsize), leading_zeros_split, true);
                  }
                  const unsigned local_llvm_adds = local_magic.isAdd ? 2U : 0U;
                  const double local_llvm_latency =
                      can_use_local_magic ?
                          div_costs_choice->mult_delay + local_llvm_adds * div_costs_choice->add_delay :
                          0.0;
                  const double local_llvm_mul_area =
                      can_use_local_magic ? getWeightedMultArea(hls_d, *div_costs_choice, constdiv_dsp_scale_k) : 0.0;
                  const double local_llvm_area =
                      can_use_local_magic ? local_llvm_mul_area + local_llvm_adds * div_costs_choice->add_area : 0.0;
                  const double local_llvm_score = can_use_local_magic ?
                                                      getConstMultDivScore(local_llvm_latency, local_llvm_area) :
                                                      std::numeric_limits<double>::infinity();
                  const double local_keep_score =
                      getConstMultDivScore(div_costs_choice->div_delay, div_costs_choice->div_area);
                  double best_score_auto = local_keep_score;
                  enum class Choice
                  {
                     Keep,
                     Magic,
                     Linarch,
                     Btcd
                  };
                  Choice choice = Choice::Keep;
                  if(local_llvm_score < best_score_auto)
                  {
                     best_score_auto = local_llvm_score;
                     choice = Choice::Magic;
                  }
                  if(linarch_score < best_score_auto)
                  {
                     best_score_auto = linarch_score;
                     choice = Choice::Linarch;
                  }
                  if(btcd_score < best_score_auto)
                  {
                     best_score_auto = btcd_score;
                     choice = Choice::Btcd;
                  }
                  estimate.auto_prefers_table = (choice == Choice::Linarch || choice == Choice::Btcd);
               }

               return estimate;
            };

            const auto atomic_estimate = estimate_table(absD, &div_costs_div, true);
            if(std::isfinite(atomic_estimate.best_table_score))
            {
               struct FactorEstimate
               {
                  TableEstimate div_est;
                  TableEstimate mod_est;
                  bool allowed;
               };

               auto evaluate_factor = [&](unsigned long long divisor) -> FactorEstimate {
                  FactorEstimate factor;
                  factor.div_est = estimate_table(divisor, &div_costs_div, true);
                  factor.mod_est = factor.div_est;
                  if(remFlag)
                  {
                     factor.mod_est = estimate_table(divisor, &div_costs_mod, true);
                  }

                  bool allowed = factor.div_est.auto_prefers_table;
                  if(remFlag)
                  {
                     allowed = allowed && factor.mod_est.auto_prefers_table;
                  }
                  factor.allowed = allowed;
                  return factor;
               };

               auto total_score_for = [&](const FactorEstimate& first, const FactorEstimate& second) -> double {
                  if(remFlag)
                  {
                     return first.div_est.best_table_score + first.mod_est.best_table_score +
                            second.div_est.best_table_score + second.mod_est.best_table_score + overhead_score;
                  }
                  return first.div_est.best_table_score + second.div_est.best_table_score;
               };

               const auto table_allowed = [&](const FactorEstimate& factor) -> bool {
                  return factor.allowed && std::isfinite(factor.div_est.best_table_score);
               };

               double best_split_score = std::numeric_limits<double>::infinity();
               unsigned long long best_first = 0;
               unsigned long long best_second = 0;
               unsigned pairs_seen = 0;
               for(unsigned long long da = 2; da * da <= absD; ++da)
               {
                  if(absD % da != 0)
                  {
                     continue;
                  }
                  ++pairs_seen;
                  if(pairs_seen > constdiv_composite_max_pairs)
                  {
                     break;
                  }
                  const unsigned long long db = absD / da;
                  if(POWER2_OR_0(da) || POWER2_OR_0(db))
                  {
                     continue;
                  }
                  const auto factor_a = evaluate_factor(da);
                  const auto factor_b = evaluate_factor(db);
                  if(!table_allowed(factor_a) || !table_allowed(factor_b))
                  {
                     continue;
                  }
                  const double total_ab = total_score_for(factor_a, factor_b);
                  if(total_ab < best_split_score)
                  {
                     best_split_score = total_ab;
                     best_first = da;
                     best_second = db;
                  }
                  const double total_ba = total_score_for(factor_b, factor_a);
                  if(total_ba < best_split_score)
                  {
                     best_split_score = total_ba;
                     best_first = db;
                     best_second = da;
                  }
               }

               if(best_first > 1 && best_second > 1)
               {
                  INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                                 "---constdiv composite(s): D=" + STR(absD) + " best_atomic=" +
                                     STR(atomic_estimate.best_table_score) + " best_split=" + STR(best_split_score));
               }

               unsigned_table_score = atomic_estimate.best_table_score;
               const double linarch_proxy = linarch_candidate ?
                                                std::max(1.0, linarch_costs.area / std::max(1e-9, div_costs.add_area)) :
                                                std::numeric_limits<double>::infinity();
               const double btcd_proxy = btcd_candidate ?
                                             std::max(1.0, btcd_costs.area / std::max(1e-9, div_costs.add_area)) :
                                             std::numeric_limits<double>::infinity();
               unsigned_table_proxy = std::min(linarch_proxy, btcd_proxy);
               if(best_first > 1 && best_second > 1 &&
                  best_split_score + constdiv_composite_margin < atomic_estimate.best_table_score)
               {
                  unsigned_table_score = best_split_score;
               }
            }
         }
         else
         {
            const auto atomic_estimate = [&]() {
               const double linarch_score = linarch_candidate ?
                                                getConstMultDivScore(linarch_costs.delay, linarch_costs.area) :
                                                std::numeric_limits<double>::infinity();
               const double btcd_score = btcd_candidate ? getConstMultDivScore(btcd_costs.delay, btcd_costs.area) :
                                                          std::numeric_limits<double>::infinity();
               return std::min(linarch_score, btcd_score);
            }();
            unsigned_table_score = atomic_estimate;
            const double linarch_proxy = linarch_candidate ?
                                             std::max(1.0, linarch_costs.area / std::max(1e-9, div_costs.add_area)) :
                                             std::numeric_limits<double>::infinity();
            const double btcd_proxy = btcd_candidate ?
                                          std::max(1.0, btcd_costs.area / std::max(1e-9, div_costs.add_area)) :
                                          std::numeric_limits<double>::infinity();
            unsigned_table_proxy = std::min(linarch_proxy, btcd_proxy);
         }
      }

      const double unsigned_best_score = std::min(keep_score, std::min(unsigned_magic_score, unsigned_table_score));
      const double unsigned_best_proxy = std::min(keep_proxy, std::min(unsigned_magic_proxy, unsigned_table_proxy));

      const unsigned negate_count = 2U + (remFlag ? 1U : 0U);
      const double wrapper_delay = div_costs.add_delay * static_cast<double>(negate_count + 1U);
      const double wrapper_area = div_costs.add_area * static_cast<double>(negate_count + 1U);
      const double wrapper_overhead_score = getConstMultDivScore(wrapper_delay, wrapper_area);
      const double wrapper_score = unsigned_best_score + wrapper_overhead_score;
      const double wrapper_proxy = unsigned_best_proxy + static_cast<double>(negate_count + 1U);

      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "---constdiv auto(s): llvm=" + STR(llvm_score) + " keep=" + STR(keep_score) +
                         " wrapper=" + STR(wrapper_score));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---constdiv auto(s) proxy: llvm=" + STR(llvm_proxy) + " keep=" + STR(keep_proxy) +
                         " wrapper=" + STR(wrapper_proxy) + " metric=" + decision_metric);

      double best_score = keep_score;
      double best_proxy = keep_proxy;
      enum class Choice
      {
         Keep,
         Magic,
         Wrapper
      };
      Choice choice = Choice::Keep;
      if(better_multdiv_candidate(decision_metric, llvm_score, llvm_proxy, best_score, best_proxy))
      {
         best_score = llvm_score;
         best_proxy = llvm_proxy;
         choice = Choice::Magic;
      }
      if(better_multdiv_candidate(decision_metric, wrapper_score, wrapper_proxy, best_score, best_proxy))
      {
         best_score = wrapper_score;
         best_proxy = wrapper_proxy;
         choice = Choice::Wrapper;
      }

      if(choice == Choice::Keep)
      {
         keep_div = true;
      }
      else if(choice == Choice::Magic)
      {
         use_magic = can_use_magic;
      }
      else
      {
         use_wrapper = true;
      }
   }
   else
   {
      keep_div = true;
   }

   if(keep_div)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "---constdiv(s): keep division D=" + STR(d) + " bits=" + STR(dataBitsize));
      return;
   }

   if(use_magic)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "---constdiv(s): llvm_magic D=" + STR(d) + " bits=" + STR(dataBitsize) +
                         " magic=" + STR(magic.magic) + " shift=" + STR(magic.shift) +
                         " numFactor=" + STR(magic.numeratorFactor) + " shiftMask=" + STR(magic.shiftMask));
      // LLVM-style magic-number sequence for signed div/mod with trunc-toward-zero correction.
      const auto magic_sext =
          static_cast<unsigned long long>(constdiv_magic::signExtend(magic.magic, static_cast<unsigned>(dataBitsize)));
      ir_nodeRef quotientExpr =
          expand_mult_highpart(op0, magic_sext, typeExpr, dataBitsize, itLos, block.second, locInfoDefault);
      THROW_ASSERT(quotientExpr, "unexpected null quotient");

      if(magic.numeratorFactor != 0)
      {
         const auto op_kind = magic.numeratorFactor > 0 ? add_node_K : sub_node_K;
         const auto addExpr = ir_man->create_binary_operation(typeExpr, quotientExpr, op0, locInfoDefault, op_kind);
         const auto addGa =
             ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), addExpr, function_id, locInfoDefault);
         block.second->PushBefore(addGa, *itLos, AppM);
         quotientExpr = GetPointer<assign_stmt>(addGa)->op0;
      }

      if(magic.shift != 0)
      {
         const auto shiftNode = TM->CreateUniqueIntegerCst(static_cast<long long>(magic.shift), typeExpr);
         const auto shiftExpr = createShiftOperationWithCostGuard(
             ir_man, typeExpr, quotientExpr, shiftNode, locInfoDefault, shr_node_K, "lowerSignedTruncDivModByConstant");
         const auto shiftGa =
             ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), shiftExpr, function_id, locInfoDefault);
         block.second->PushBefore(shiftGa, *itLos, AppM);
         quotientExpr = GetPointer<assign_stmt>(shiftGa)->op0;
      }

      const auto unsigned_type = ir_man->CreateUnsigned(typeExpr);
      const auto qUnsignedExpr =
          ir_man->create_unary_operation(unsigned_type, quotientExpr, locInfoDefault, nop_node_K);
      const auto qUnsignedGa = ir_man->CreateAssignStmt(unsigned_type, ir_nodeRef(), ir_nodeRef(), qUnsignedExpr,
                                                        function_id, locInfoDefault);
      block.second->PushBefore(qUnsignedGa, *itLos, AppM);
      const auto qUnsignedVar = GetPointer<assign_stmt>(qUnsignedGa)->op0;

      const auto signShiftNode = TM->CreateUniqueIntegerCst(static_cast<long long>(dataBitsize - 1), unsigned_type);
      const auto tExpr =
          createShiftOperationWithCostGuard(ir_man, unsigned_type, qUnsignedVar, signShiftNode, locInfoDefault,
                                            shr_node_K, "lowerSignedTruncDivModByConstant");
      const auto tGa =
          ir_man->CreateAssignStmt(unsigned_type, ir_nodeRef(), ir_nodeRef(), tExpr, function_id, locInfoDefault);
      block.second->PushBefore(tGa, *itLos, AppM);
      const auto tVar = GetPointer<assign_stmt>(tGa)->op0;

      const auto shiftMaskNode = TM->CreateUniqueIntegerCst(magic.shiftMask, unsigned_type);
      const auto tMaskExpr =
          ir_man->create_binary_operation(unsigned_type, tVar, shiftMaskNode, locInfoDefault, and_node_K);
      const auto tMaskGa =
          ir_man->CreateAssignStmt(unsigned_type, ir_nodeRef(), ir_nodeRef(), tMaskExpr, function_id, locInfoDefault);
      block.second->PushBefore(tMaskGa, *itLos, AppM);
      const auto tMaskVar = GetPointer<assign_stmt>(tMaskGa)->op0;

      const auto tSignedExpr = ir_man->create_unary_operation(typeExpr, tMaskVar, locInfoDefault, nop_node_K);
      const auto tSignedGa =
          ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), tSignedExpr, function_id, locInfoDefault);
      block.second->PushBefore(tSignedGa, *itLos, AppM);
      const auto tSignedVar = GetPointer<assign_stmt>(tSignedGa)->op0;

      const auto qFixExpr =
          ir_man->create_binary_operation(typeExpr, quotientExpr, tSignedVar, locInfoDefault, add_node_K);
      const auto qFixGa =
          ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), qFixExpr, function_id, locInfoDefault);
      block.second->PushBefore(qFixGa, *itLos, AppM);
      quotientExpr = GetPointer<assign_stmt>(qFixGa)->op0;

      if(remFlag)
      {
         assignRemainderFromQuotient(block, itLos, ga, op0, op1, typeExpr, quotientExpr, locInfoDefault, stepName,
                                     false, 0);
      }
      else
      {
         ga->op1 = quotientExpr;
      }
      AppM->RegisterTransformation(stepName, *itLos);
      restartAnalysis = true;
      return;
   }

   if(use_wrapper)
   {
      const auto bt = ir_man->GetBooleanType();
      const auto const_zero = TM->CreateUniqueIntegerCst(0, typeExpr);
      const auto const_one_bt = TM->CreateUniqueIntegerCst(1, bt);
      const auto sign_expr = ir_man->create_binary_operation(bt, op0, const_zero, locInfoDefault, lt_node_K);
      const auto sign_ga =
          ir_man->CreateAssignStmt(bt, TM->CreateUniqueIntegerCst(0, bt), TM->CreateUniqueIntegerCst(1, bt), sign_expr,
                                   function_id, locInfoDefault);
      block.second->PushBefore(sign_ga, *itLos, AppM);
      const auto sign_var = GetPointer<assign_stmt>(sign_ga)->op0;

      const auto neg_expr = ir_man->create_unary_operation(typeExpr, op0, locInfoDefault, neg_node_K);
      const auto abs_node =
          ir_man->create_ternary_operation(typeExpr, sign_var, neg_expr, op0, locInfoDefault, select_node_K);
      const auto abs_ga =
          ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), abs_node, function_id, locInfoDefault);
      block.second->PushBefore(abs_ga, *itLos, AppM);
      const auto abs_var = GetPointer<assign_stmt>(abs_ga)->op0;

      const auto unsigned_type = ir_man->CreateUnsigned(typeExpr);
      const auto abs_unsigned_expr = ir_man->create_unary_operation(unsigned_type, abs_var, locInfoDefault, nop_node_K);
      const auto abs_unsigned_ga = ir_man->CreateAssignStmt(unsigned_type, ir_nodeRef(), ir_nodeRef(),
                                                            abs_unsigned_expr, function_id, locInfoDefault);
      block.second->PushBefore(abs_unsigned_ga, *itLos, AppM);
      const auto abs_unsigned_var = GetPointer<assign_stmt>(abs_unsigned_ga)->op0;

      auto create_unsigned_assign = [&](const ir_nodeRef& lhs, const ir_nodeRef& rhs, enum kind code) {
         const auto expr = ir_man->create_binary_operation(unsigned_type, lhs, rhs, locInfoDefault, code);
         const auto ga_expr =
             ir_man->CreateAssignStmt(unsigned_type, ir_nodeRef(), ir_nodeRef(), expr, function_id, locInfoDefault);
         block.second->PushBefore(ga_expr, *itLos, AppM);
         return GetPointer<assign_stmt>(ga_expr)->op0;
      };

      const auto const_absd = TM->CreateUniqueIntegerCst(static_cast<long long>(absD), unsigned_type);
      if(remFlag)
      {
         const auto r_abs_u = create_unsigned_assign(abs_unsigned_var, const_absd, irem_node_K);
         const auto r_abs_node = ir_man->create_unary_operation(typeExpr, r_abs_u, locInfoDefault, nop_node_K);
         const auto r_abs_ga =
             ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), r_abs_node, function_id, locInfoDefault);
         block.second->PushBefore(r_abs_ga, *itLos, AppM);
         const auto r_abs_var = GetPointer<assign_stmt>(r_abs_ga)->op0;
         const auto r_neg_expr = ir_man->create_unary_operation(typeExpr, r_abs_var, locInfoDefault, neg_node_K);
         const auto r_fix_expr =
             ir_man->create_ternary_operation(typeExpr, sign_var, r_neg_expr, r_abs_var, locInfoDefault, select_node_K);
         const auto r_fix_ga =
             ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), r_fix_expr, function_id, locInfoDefault);
         block.second->PushBefore(r_fix_ga, *itLos, AppM);
         ga->op1 = GetPointer<assign_stmt>(r_fix_ga)->op0;
      }
      else
      {
         const auto q_abs_u = create_unsigned_assign(abs_unsigned_var, const_absd, idiv_node_K);
         const auto q_abs_node = ir_man->create_unary_operation(typeExpr, q_abs_u, locInfoDefault, nop_node_K);
         const auto q_abs_ga =
             ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), q_abs_node, function_id, locInfoDefault);
         block.second->PushBefore(q_abs_ga, *itLos, AppM);
         const auto q_abs_var = GetPointer<assign_stmt>(q_abs_ga)->op0;
         ir_nodeRef sign_q_var = sign_var;
         if(d < 0)
         {
            const auto sign_q_expr =
                ir_man->create_binary_operation(bt, sign_var, const_one_bt, locInfoDefault, xor_node_K);
            const auto sign_q_ga =
                ir_man->CreateAssignStmt(bt, TM->CreateUniqueIntegerCst(0, bt), TM->CreateUniqueIntegerCst(1, bt),
                                         sign_q_expr, function_id, locInfoDefault);
            block.second->PushBefore(sign_q_ga, *itLos, AppM);
            sign_q_var = GetPointer<assign_stmt>(sign_q_ga)->op0;
         }
         const auto q_neg_expr = ir_man->create_unary_operation(typeExpr, q_abs_var, locInfoDefault, neg_node_K);
         const auto q_fix_expr = ir_man->create_ternary_operation(typeExpr, sign_q_var, q_neg_expr, q_abs_var,
                                                                  locInfoDefault, select_node_K);
         const auto q_fix_ga =
             ir_man->CreateAssignStmt(typeExpr, ir_nodeRef(), ir_nodeRef(), q_fix_expr, function_id, locInfoDefault);
         block.second->PushBefore(q_fix_ga, *itLos, AppM);
         ga->op1 = GetPointer<assign_stmt>(q_fix_ga)->op0;
      }

      AppM->RegisterTransformation(stepName, *itLos);
      restartAnalysis = true;
      return;
   }
}

void IR_lowering::initConstDivMulLoweringParams()
{
   if(constdivmul_params_initialized)
   {
      return;
   }

   constdiv_lowering_mode = "auto";
   constdiv_dsp_scale_k = 1.0;
   constmultdiv_score = "weighted";
   constmultdiv_decision_metric = "auto";
   constmultdiv_lut_cost_model = "auto";
   constmultdiv_w_latency = 1.0;
   constmultdiv_w_area = 1.0;
   constdiv_composite_enable = true;
   constdiv_composite_margin = 0.0;
   constdiv_composite_max_pairs = 32;
   constmul_enable = true;
   constmul_balance_tree = true;
   constmul_balance_tree_min_terms = 4;
   constmul_max_terms = 16;
   constmul_max_depth = 8;
   constmul_try_factor_forms = true;
   constmul_enable_small_factor_chains = true;
   constmul_dsp_scale_k = 1.0;
   constmul_kcm_enable = true;
   constmul_kcm_sum_strategy = "tree";
   constmul_kcm_merge_table_add = false;
   constmul_kcm_cost_model = "heuristic";
   const HLS_deviceRef hls_d = getHlsDeviceFromApp(AppM);
   constmul_kcm_alpha = resolveRequiredMaxLutSize(AppM, hls_d);

   AppM->TryGetParameterFromParameterOrDevice<std::string>("constdiv_lowering_mode", hls_d, constdiv_lowering_mode);
   AppM->TryGetParameterFromParameterOrDevice<double>("constdiv_dsp_scale_k", hls_d, constdiv_dsp_scale_k);
   AppM->TryGetParameterFromParameterOrDevice<std::string>("constmultdiv_score", hls_d, constmultdiv_score);
   AppM->TryGetParameterFromParameterOrDevice<std::string>("constmultdiv_decision_metric", hls_d,
                                                           constmultdiv_decision_metric);
   AppM->TryGetParameterFromParameterOrDevice<std::string>("constmultdiv_lut_cost_model", hls_d,
                                                           constmultdiv_lut_cost_model);
   AppM->TryGetParameterFromParameterOrDevice<double>("constmultdiv_w_latency", hls_d, constmultdiv_w_latency);
   AppM->TryGetParameterFromParameterOrDevice<double>("constmultdiv_w_area", hls_d, constmultdiv_w_area);
   constdiv_composite_enable = AppM->GetParameterFromParameterOrDeviceOrDefault<bool>("constdiv_composite_enable",
                                                                                      hls_d, constdiv_composite_enable);
   AppM->TryGetParameterFromParameterOrDevice<double>("constdiv_composite_margin", hls_d, constdiv_composite_margin);
   AppM->TryGetParameterFromParameterOrDevice<unsigned>("constdiv_composite_max_pairs", hls_d,
                                                        constdiv_composite_max_pairs);
   constmul_enable = AppM->GetParameterFromParameterOrDeviceOrDefault<bool>("constmul_enable", hls_d, constmul_enable);
   constmul_balance_tree =
       AppM->GetParameterFromParameterOrDeviceOrDefault<bool>("constmul_balance_tree", hls_d, constmul_balance_tree);
   AppM->TryGetParameterFromParameterOrDevice<unsigned>("constmul_balance_tree_min_terms", hls_d,
                                                        constmul_balance_tree_min_terms);
   AppM->TryGetParameterFromParameterOrDevice<unsigned>("constmul_max_terms", hls_d, constmul_max_terms);
   AppM->TryGetParameterFromParameterOrDevice<unsigned>("constmul_max_depth", hls_d, constmul_max_depth);
   constmul_try_factor_forms = AppM->GetParameterFromParameterOrDeviceOrDefault<bool>("constmul_try_factor_forms",
                                                                                      hls_d, constmul_try_factor_forms);
   constmul_enable_small_factor_chains = AppM->GetParameterFromParameterOrDeviceOrDefault<bool>(
       "constmul_enable_small_factor_chains", hls_d, constmul_enable_small_factor_chains);
   AppM->TryGetParameterFromParameterOrDevice<double>("constmul_dsp_scale_k", hls_d, constmul_dsp_scale_k);
   constmul_kcm_enable =
       AppM->GetParameterFromParameterOrDeviceOrDefault<bool>("constmul_kcm_enable", hls_d, constmul_kcm_enable);
   AppM->TryGetParameterFromParameterOrDevice<unsigned>("constmul_kcm_alpha", hls_d, constmul_kcm_alpha);
   AppM->TryGetParameterFromParameterOrDevice<std::string>("constmul_kcm_sum_strategy", hls_d,
                                                           constmul_kcm_sum_strategy);
   constmul_kcm_merge_table_add = AppM->GetParameterFromParameterOrDeviceOrDefault<bool>(
       "constmul_kcm_merge_table_add", hls_d, constmul_kcm_merge_table_add);
   AppM->TryGetParameterFromParameterOrDevice<std::string>("constmul_kcm_cost_model", hls_d, constmul_kcm_cost_model);
   if(constmultdiv_decision_metric != "auto" && constmultdiv_decision_metric != "score" &&
      constmultdiv_decision_metric != "proxy" && constmultdiv_decision_metric != "hybrid")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---constmultdiv_decision_metric='" + constmultdiv_decision_metric + "' unsupported, using auto");
      constmultdiv_decision_metric = "auto";
   }
   if(constmultdiv_lut_cost_model != "auto" && constmultdiv_lut_cost_model != "analytic" &&
      constmultdiv_lut_cost_model != "mockturtle" && constmultdiv_lut_cost_model != "mockturtle_full")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---constmultdiv_lut_cost_model='" + constmultdiv_lut_cost_model + "' unsupported, using auto");
      constmultdiv_lut_cost_model = "auto";
   }
   constdivmul_params_initialized = true;
}

void IR_lowering::decomposeDivisionByConstant(const std::pair<unsigned int, blocRef>& block,
                                              std::list<ir_nodeRef>::const_iterator& it_los, assign_stmt* ga,
                                              const ir_nodeRef& op1, enum kind code1, bool& restart_analysis,
                                              const std::string& loc_info_default, const std::string& step_name)
{
   // Div/mod-by-constant entrypoint; the selected strategy is a front-end option:
   // llvm_magic relies on LLVM-style magic-number lowering, and auto prefers llvm_magic
   // when applicable and otherwise keeps the division/modulo after trivial fast paths.
   initConstDivMulLoweringParams();
#if HAVE_ASSERTS
   debugCheckMagic64();
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---constdiv mode=" + constdiv_lowering_mode);
   THROW_ASSERT(ir_helper::IsConstant(op1), "");
   const auto op0 = GetPointerS<const binary_node>(ga->op1)->op0;
   const auto type_expr = GetPointerS<const binary_node>(ga->op1)->type;

   const auto unsignedp = ir_helper::IsUnsignedIntegerType(type_expr);
   THROW_ASSERT(std::numeric_limits<long long>::min() <= ir_helper::GetConstValue(op1) &&
                    ir_helper::GetConstValue(op1) <= std::numeric_limits<long long>::max(),
                "");
   const auto ext_op1 = static_cast<long long>(ir_helper::GetConstValue(op1));
   const auto rem_flag = code1 == irem_node_K;

   if(!AppM->ApplyNewTransformation())
   {
      return;
   }

   if(handleTrivialDivByConstant(ga, op0, type_expr, ext_op1, unsignedp, rem_flag, loc_info_default, restart_analysis))
   {
      return;
   }

   if(ext_op1 == 0)
   {
      return;
   }

   switch(code1)
   {
      case idiv_node_K:
      case irem_node_K:
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Trunc_div or trunc_mod");
         if(unsignedp)
         {
            lowerUnsignedTruncDivModByConstant(block, it_los, ga, op0, op1, type_expr, ext_op1, rem_flag,
                                               loc_info_default, step_name, restart_analysis);
         }
         else
         {
            lowerSignedTruncDivModByConstant(block, it_los, ga, op0, op1, type_expr, ext_op1, rem_flag,
                                             loc_info_default, step_name, restart_analysis);
         }
         break;
      }
      case and_node_K:
      case or_node_K:
      case xor_node_K:
      case eq_node_K:
      case ge_node_K:
      case gt_node_K:
      case le_node_K:
      case shl_node_K:
      case lt_node_K:
      case max_node_K:
      case min_node_K:
      case sub_node_K:
      case mul_node_K:
      case ne_node_K:
      case add_node_K:
      case gep_node_K:
      case fdiv_node_K:
      case frem_node_K:
      case shr_node_K:
      case lut_node_K:
      case widen_mul_node_K:
      case call_node_K:
      case constructor_node_K:
      case identifier_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case extract_bit_node_K:
      case add_sat_node_K:
      case sub_sat_node_K:
      case extractvalue_node_K:
      case extractelement_node_K:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TERNARY_NODES:
      case CASE_TYPE_NODES:
      case CASE_UNARY_NODES:
      default:
      {
         THROW_ERROR("not yet supported code: " + ga->op1->get_kind_text());
         break;
      }
   }
}
