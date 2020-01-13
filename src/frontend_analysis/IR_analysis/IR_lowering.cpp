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
 * @file IR_lowering.cpp
 * @brief Decompose some complex gimple statements into a set of simpler operations.
 * Matteo M. Fusi and Matteo Locatelli modifies the division by an integer constant
 * integrating the work done in this paper:
 * Florent de Dinechin, “Multiplication by Rational Constants” -
 * IEEE Transactions on Circuit Systems-II: Express Briefs, Vol. 59, NO 2, february 2012
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "IR_lowering.hpp"

#include "config_HAVE_ASSERTS.hpp"     // for HAVE_ASSERTS
#include "config_HAVE_BAMBU_BUILT.hpp" // for HAVE_BAMBU_BUILT

#include "Parameter.hpp"                    // for Parameter
#include "application_manager.hpp"          // for application_manager, app...
#include "custom_map.hpp"                   // for unordered_map, operator!=
#include "dbgPrintHelper.hpp"               // for DEBUG_LEVEL_VERY_PEDANTIC
#include "design_flow_graph.hpp"            // for DesignFlowGraph, DesignF...
#include "design_flow_manager.hpp"          // for DesignFlowManager, Desig...
#include "design_flow_step_factory.hpp"     // for DesignFlowManagerConstRef
#include "exceptions.hpp"                   // for THROW_ASSERT, THROW_UNRE...
#include "graph.hpp"                        // for vertex
#include "hash_helper.hpp"                  // for hash
#include "hls_manager.hpp"                  // for HLS_manager
#include "hls_target.hpp"                   // for HLS_target, HLS_targetRef
#include "math_function.hpp"                // for floor_log2, exact_log2
#include "string_manipulation.hpp"          // for STR, GET_CLASS
#include "technology_flow_step.hpp"         // for TechnologyFlowStep_Type
#include "technology_flow_step_factory.hpp" // for TechnologyFlowStepFactory
#include "technology_manager.hpp"           // for LIBRARY_STD_FU, technolo...
#include "technology_node.hpp"              // for functional_unit, operation
#include "time_model.hpp"                   // for time_model
#include "tree_basic_block.hpp"             // for bloc
#include "tree_common.hpp"                  // for plus_expr_K, lshift_expr_K
#include "tree_helper.hpp"                  // for tree_helper
#include "tree_manager.hpp"                 // for tree_manager
#include "tree_manipulation.hpp"            // for tree_manipulation, Param...
#include "tree_node.hpp"                    // for tree_nodeRef, gimple_assign
#include "tree_reindex.hpp"
#include <cmath>   // for ceil
#include <cstddef> // for size_t
#include <limits>
#include <vector> // for vector

IR_lowering::IR_lowering(const ParameterConstRef Param, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, IR_LOWERING, _design_flow_manager, Param)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> IR_lowering::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BLOCK_FIX, SAME_FUNCTION));
         relationships.insert(std::make_pair(SWITCH_FIX, SAME_FUNCTION));
         relationships.insert(std::make_pair(FIX_STRUCTS_PASSED_BY_VALUE, SAME_FUNCTION));
         relationships.insert(std::make_pair(REMOVE_CLOBBER_GA, SAME_FUNCTION));
         relationships.insert(std::make_pair(HWCALL_INJECTION, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(REBUILD_INITIALIZATION, SAME_FUNCTION));
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return relationships;
}

IR_lowering::~IR_lowering() = default;

void IR_lowering::Initialize()
{
   TM = AppM->get_tree_manager();
   tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters));
}

void IR_lowering::ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type)
{
   switch(relationship_type)
   {
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case DEPENDENCE_RELATIONSHIP:
      {
         const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
         const auto* technology_flow_step_factory = GetPointer<const TechnologyFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Technology"));
         const std::string technology_flow_signature = TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         const vertex technology_flow_step = design_flow_manager.lock()->GetDesignFlowStep(technology_flow_signature);
         const DesignFlowStepRef technology_design_flow_step =
             technology_flow_step ? design_flow_graph->CGetDesignFlowStepInfo(technology_flow_step)->design_flow_step : technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         relationship.insert(technology_design_flow_step);
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   FunctionFrontendFlowStep::ComputeRelationships(relationship, relationship_type);
}

/// the code for lowering of div, mult and rem comes from GCC sources (expmed.c)

enum alg_code
{
   alg_unknown,
   alg_zero,
   alg_m,
   alg_shift,
   alg_add_t_m2,
   alg_sub_t_m2,
   alg_add_factor,
   alg_sub_factor,
   alg_add_t2_m,
   alg_sub_t2_m,
   alg_impossible
};

/** This structure holds the "cost" of a multiply sequence.  The
   "cost" field holds the total cost of every operator in the
   synthetic multiplication sequence, hence cost(a op b) is defined
   as cost(op) + cost(a) + cost(b), where cost(leaf) is zero.
   The "latency" field holds the minimum possible latency of the
   synthetic multiply, on a hypothetical infinitely parallel CPU.
   This is the critical path, or the maximum height, of the expression
   tree which is the sum of costs on the most expensive path from
   any leaf to the root.  Hence latency(a op b) is defined as zero for
   leaves and cost(op) + max(latency(a), latency(b)) otherwise.  */
struct mult_cost
{
   short cost;    /** Total cost of the multiplication sequence.  */
   short latency; /** The latency of the multiplication sequence.  */
};

/** This structure records a sequence of operations.
   `ops' is the number of operations recorded.
   `cost' is their total cost.
   The operations are stored in `op' and the corresponding
   logarithms of the integer coefficients in `log'.

   These are the operations:
   alg_zero		total := 0;
   alg_m		total := multiplicand;
   alg_shift		total := total * coeff
   alg_add_t_m2		total := total + multiplicand * coeff;
   alg_sub_t_m2		total := total - multiplicand * coeff;
   alg_add_factor	total := total * coeff + total;
   alg_sub_factor	total := total * coeff - total;
   alg_add_t2_m		total := total * coeff + multiplicand;
   alg_sub_t2_m		total := total * coeff - multiplicand;

   The first operand must be either alg_zero or alg_m.  */
struct algorithm
{
   struct mult_cost cost;
   short ops;
   /* The size of the OP and LOG fields are not directly related to the
      word size, but the worst-case algorithms will be if we have few
      consecutive ones or zeros, i.e., a multiplicand like 10101010101...
      In that case we will generate shift-by-2, add, shift-by-2, add,...,
      in total wordsize operations.  */
   enum alg_code op[64];
   char log[64];
};

/** This macro is used to compare a pointer to a mult_cost against an
   single integer "cost" value.  This is equivalent to the macro
   CHEAPER_MULT_COST(X,Z) where Z = {Y,Y}.  */
#define MULT_COST_LESS(X, Y) ((X).cost < (Y) || ((X).cost == (Y) && (X).latency < (Y)))

/** This macro is used to compare two mult_costs against
   each other.  The macro returns true if X is cheaper than Y.
   Currently, the cheaper of two mult_costs is the one with the
   lower "cost".  If "cost"s are tied, the lower latency is cheaper.  */
#define CHEAPER_MULT_COST(X, Y) ((X).cost < (Y).cost || ((X).cost == (Y).cost && (X).latency < (Y).latency))

static CustomUnorderedMap<std::pair<unsigned int, unsigned long long>, std::pair<enum alg_code, struct mult_cost>> alg_hash;

/** Compute and return the best algorithm for multiplying by T.
   The algorithm must cost less than cost_limit
   If retval.cost >= COST_LIMIT, no algorithm was found and all
   other field of the returned struct are undefined.
   MODE is the machine mode of the multiplication.  */
static void synth_mult(struct algorithm& alg_out, unsigned long long t, const struct mult_cost& cost_limit, unsigned data_bitsize, tree_managerRef& TM)
{
   int m;
   struct algorithm alg_in, best_alg;
   struct mult_cost best_cost;
   struct mult_cost new_limit;
   short op_cost, op_latency;
   unsigned long long int orig_t = t;
   unsigned long long int q;
   int maxm = static_cast<int>(std::min(32u, data_bitsize));
   bool cache_hit = false;
   enum alg_code cache_alg = alg_zero;
   best_alg.cost.cost = std::numeric_limits<short>::max();
   best_alg.cost.latency = std::numeric_limits<short>::max();
   best_alg.ops = 0;

   /** Indicate that no algorithm is yet found.  If no algorithm
      is found, this value will be returned and indicate failure.  */
   alg_out.cost.cost = static_cast<short>(cost_limit.cost + 1);
   alg_out.cost.latency = static_cast<short>(cost_limit.latency + 1);

   if(cost_limit.cost < 0 || (cost_limit.cost == 0 && cost_limit.latency <= 0))
      return;

   /** t == 1 can be done in zero cost.  */
   if(t == 1)
   {
      alg_out.ops = 1;
      alg_out.cost.cost = 0;
      alg_out.cost.latency = 0;
      alg_out.op[0] = alg_m;
      return;
   }

   /** t == 0 sometimes has a cost.  If it does and it exceeds our limit,
      fail now.  */
   if(t == 0)
   {
      alg_out.ops = 1;
      alg_out.cost.cost = 0 /*zero_cost[speed]*/;
      alg_out.cost.latency = 0 /*zero_cost[speed]*/;
      alg_out.op[0] = alg_zero;
      return;
   }
   best_cost = cost_limit;
   if(alg_hash.find(std::make_pair(data_bitsize, t)) != alg_hash.end())
   {
      std::pair<enum alg_code, struct mult_cost> res = alg_hash.find(std::make_pair(data_bitsize, t))->second;
      cache_alg = res.first;
      if(cache_alg == alg_impossible)
      {
         /** The cache tells us that it's impossible to synthesize
               multiplication by T within alg_hash[hash_index].cost.  */
         if(!CHEAPER_MULT_COST(res.second, cost_limit))
            /** COST_LIMIT is at least as restrictive as the one
                 recorded in the hash table, in which case we have no
                 hope of synthesizing a multiplication.  Just
                 return.  */
            return;

         /** If we get here, COST_LIMIT is less restrictive than the
               one recorded in the hash table, so we may be able to
               synthesize a multiplication.  Proceed as if we didn't
               have the cache entry.  */
      }
      else
      {
         if(CHEAPER_MULT_COST(cost_limit, res.second))
            /** The cached algorithm shows that this multiplication
                 requires more cost than COST_LIMIT.  Just return.  This
                 way, we don't clobber this cache entry with
                 alg_impossible but retain useful information.  */
            return;
         cache_hit = true;

         switch(cache_alg)
         {
            case alg_shift:
               goto do_alg_shift;

            case alg_add_t_m2:
            case alg_sub_t_m2:
               goto do_alg_addsub_t_m2;

            case alg_add_factor:
            case alg_sub_factor:
               goto do_alg_addsub_factor;

            case alg_add_t2_m:
               goto do_alg_add_t2_m;

            case alg_sub_t2_m:
               goto do_alg_sub_t2_m;
            case alg_impossible:
            case alg_m:
            case alg_unknown:
            case alg_zero:
            default:
               THROW_ERROR("condition not expected");
         }
      }
   }

   /** If we have a group of zero bits at the low-order part of T, try
      multiplying by the remaining bits and then doing a shift.  */

   if((t & 1) == 0)
   {
   do_alg_shift:
      m = floor_log2(t & -t); /* m = number of low zero bits */
      if(m < maxm)
      {
         q = t >> m;
         /* The function expand_shift will choose between a shift and
                  a sequence of additions, so the observed cost is given as
                  MIN (m * add_cost[speed][mode], shift_cost[speed][mode][m]).  */
         op_cost = static_cast<short>(m * 1 /*add_cost[speed][mode]*/);
         if(0 /*shift_cost[speed][mode][m]*/ < op_cost)
            op_cost = 0 /*shift_cost[speed][mode][m]*/;
         new_limit.cost = static_cast<short>(best_cost.cost - op_cost);
         new_limit.latency = static_cast<short>(best_cost.latency - op_cost);
         synth_mult(alg_in, q, new_limit, data_bitsize, TM);

         alg_in.cost.cost = static_cast<short>(alg_in.cost.cost + op_cost);
         alg_in.cost.latency = static_cast<short>(alg_in.cost.latency + op_cost);
         if(CHEAPER_MULT_COST(alg_in.cost, best_cost))
         {
            best_cost = alg_in.cost;
            std::swap(alg_in, best_alg);
            best_alg.log[best_alg.ops] = static_cast<char>(m);
            best_alg.op[best_alg.ops] = alg_shift;
         }

         /** See if treating ORIG_T as a signed number yields a better
          sequence.  Try this sequence only for a negative ORIG_T
          as it would be useless for a non-negative ORIG_T.  */
         if(static_cast<long long int>(orig_t) < 0)
         {
            /** Shift ORIG_T as follows because a right shift of a
          negative-valued signed type is implementation
          defined.  */
            q = ~(~orig_t >> m);
            /** The function expand_shift will choose between a shift
          and a sequence of additions, so the observed cost is
          given as MIN (m * add_cost[speed][mode], shift_cost[speed][mode][m]).  */
            op_cost = static_cast<short>(m * 1) /*add_cost[speed][mode]*/;
            if(0 /*shift_cost[speed][mode][m]*/ < op_cost)
               op_cost = 0 /*shift_cost[speed][mode][m]*/;
            new_limit.cost = static_cast<short>(best_cost.cost - op_cost);
            new_limit.latency = static_cast<short>(best_cost.latency - op_cost);
            synth_mult(alg_in, q, new_limit, data_bitsize, TM);

            alg_in.cost.cost = static_cast<short>(alg_in.cost.cost + op_cost);
            alg_in.cost.latency = static_cast<short>(alg_in.cost.latency + op_cost);
            if(CHEAPER_MULT_COST(alg_in.cost, best_cost))
            {
               best_cost = alg_in.cost;
               std::swap(alg_in, best_alg);
               best_alg.log[best_alg.ops] = static_cast<char>(m);
               best_alg.op[best_alg.ops] = alg_shift;
            }
         }
      }
      if(cache_hit)
         goto done;
   }

   /** If we have an odd number, add or subtract one.  */
   if((t & 1) != 0)
   {
      unsigned long long int w;

   do_alg_addsub_t_m2:
      for(w = 1; (w & t) != 0; w <<= 1)
         ;
      /** If T was -1, then W will be zero after the loop.  This is another
      case where T ends with ...111.  Handling this with (T + 1) and
      subtract 1 produces slightly better code and results in algorithm
      selection much faster than treating it like the ...0111 case
      below.  */
      if(w == 0 || (w > 2
                    /** Reject the case where t is 3.
                        Thus we prefer addition in that case.  */
                    && t != 3))
      {
         /** T ends with ...111.  Multiply by (T + 1) and subtract 1.  */

         op_cost = 1 /*add_cost[speed][mode]*/;
         new_limit.cost = static_cast<short>(best_cost.cost - op_cost);
         new_limit.latency = static_cast<short>(best_cost.latency - op_cost);
         synth_mult(alg_in, t + 1, new_limit, data_bitsize, TM);

         alg_in.cost.cost = static_cast<short>(alg_in.cost.cost + op_cost);
         alg_in.cost.latency = static_cast<short>(alg_in.cost.latency + op_cost);
         if(CHEAPER_MULT_COST(alg_in.cost, best_cost))
         {
            best_cost = alg_in.cost;
            std::swap(alg_in, best_alg);
            best_alg.log[best_alg.ops] = 0;
            best_alg.op[best_alg.ops] = alg_sub_t_m2;
         }
      }
      else
      {
         /** T ends with ...01 or ...011.  Multiply by (T - 1) and add 1.  */

         op_cost = 1 /*add_cost[speed][mode]*/;
         new_limit.cost = static_cast<short>(best_cost.cost - op_cost);
         new_limit.latency = static_cast<short>(best_cost.latency - op_cost);
         synth_mult(alg_in, t - 1, new_limit, data_bitsize, TM);

         alg_in.cost.cost = static_cast<short>(alg_in.cost.cost + op_cost);
         alg_in.cost.latency = static_cast<short>(alg_in.cost.latency + op_cost);
         if(CHEAPER_MULT_COST(alg_in.cost, best_cost))
         {
            best_cost = alg_in.cost;
            std::swap(alg_in, best_alg);
            best_alg.log[best_alg.ops] = 0;
            best_alg.op[best_alg.ops] = alg_add_t_m2;
         }
      }

      /** We may be able to calculate a * -7, a * -15, a * -31, etc
      quickly with a - a * n for some appropriate constant n.  */
      m = exact_log2(-orig_t + 1);
      if(m >= 0 && m < maxm)
      {
         op_cost = 1 /*shiftsub1_cost[speed][mode][m]*/;
         new_limit.cost = static_cast<short>(best_cost.cost - op_cost);
         new_limit.latency = static_cast<short>(best_cost.latency - op_cost);
         synth_mult(alg_in, static_cast<unsigned long long int>(-orig_t + 1) >> m, new_limit, data_bitsize, TM);

         alg_in.cost.cost = static_cast<short>(alg_in.cost.cost + op_cost);
         alg_in.cost.latency = static_cast<short>(alg_in.cost.latency + op_cost);
         if(CHEAPER_MULT_COST(alg_in.cost, best_cost))
         {
            best_cost = alg_in.cost;
            std::swap(alg_in, best_alg);
            best_alg.log[best_alg.ops] = static_cast<char>(m);
            best_alg.op[best_alg.ops] = alg_sub_t_m2;
         }
      }

      if(cache_hit)
         goto done;
   }

   /** Look for factors of t of the form
      t = q(2**m +- 1), 2 <= m <= floor(log2(t - 1)).
      If we find such a factor, we can multiply by t using an algorithm that
      multiplies by q, shift the result by m and add/subtract it to itself.

      We search for large factors first and loop down, even if large factors
      are less probable than small; if we find a large factor we will find a
      good sequence quickly, and therefore be able to prune (by decreasing
      COST_LIMIT) the search.  */

do_alg_addsub_factor:
   for(m = floor_log2(t - 1); m >= 2; m--)
   {
      unsigned long long int d;

      d = (1ULL << m) + 1;
      if(t % d == 0 && t > d && m < maxm && (!cache_hit || cache_alg == alg_add_factor))
      {
         /** If the target has a cheap shift-and-add instruction use
          that in preference to a shift insn followed by an add insn.
          Assume that the shift-and-add is "atomic" with a latency
          equal to its cost, otherwise assume that on superscalar
          hardware the shift may be executed concurrently with the
          earlier steps in the algorithm.  */
         op_cost = 1 /*add_cost[speed][mode]*/ + 0 /*shift_cost[speed][mode][m]*/;
         // if (1/*shiftadd_cost[speed][mode][m]*/ < op_cost)
         //{
         //   op_cost = 1/*shiftadd_cost[speed][mode][m]*/;
         //   op_latency = op_cost;
         //}
         // else
         op_latency = 1 /*add_cost[speed][mode]*/;

         new_limit.cost = static_cast<short>(best_cost.cost - op_cost);
         new_limit.latency = static_cast<short>(best_cost.latency - op_latency);
         synth_mult(alg_in, t / d, new_limit, data_bitsize, TM);

         alg_in.cost.cost = static_cast<short>(alg_in.cost.cost + op_cost);
         alg_in.cost.latency = static_cast<short>(alg_in.cost.latency + op_latency);
         if(alg_in.cost.latency < op_cost)
            alg_in.cost.latency = op_cost;
         if(CHEAPER_MULT_COST(alg_in.cost, best_cost))
         {
            best_cost = alg_in.cost;
            std::swap(alg_in, best_alg);
            best_alg.log[best_alg.ops] = static_cast<char>(m);
            best_alg.op[best_alg.ops] = alg_add_factor;
         }
         /** Other factors will have been taken care of in the recursion.  */
         break;
      }

      d = (1ULL << m) - 1;
      if(t % d == 0 && t > d && m < maxm && (!cache_hit || cache_alg == alg_sub_factor))
      {
         /** If the target has a cheap shift-and-subtract insn use
          that in preference to a shift insn followed by a sub insn.
          Assume that the shift-and-sub is "atomic" with a latency
          equal to it's cost, otherwise assume that on superscalar
          hardware the shift may be executed concurrently with the
          earlier steps in the algorithm.  */
         op_cost = 1 /*add_cost[speed][mode]*/ + 0 /*shift_cost[speed][mode][m]*/;
         // if (1/*shiftsub0_cost[speed][mode][m]*/ < op_cost)
         //{
         //   op_cost = 1/*shiftsub0_cost[speed][mode][m]*/;
         //   op_latency = op_cost;
         //}
         // else
         op_latency = 1 /*add_cost[speed][mode]*/;

         new_limit.cost = static_cast<short>(best_cost.cost - op_cost);
         new_limit.latency = static_cast<short>(best_cost.latency - op_latency);
         synth_mult(alg_in, t / d, new_limit, data_bitsize, TM);

         alg_in.cost.cost = static_cast<short>(alg_in.cost.cost + op_cost);
         alg_in.cost.latency = static_cast<short>(alg_in.cost.latency + op_latency);
         if(alg_in.cost.latency < op_cost)
            alg_in.cost.latency = op_cost;
         if(CHEAPER_MULT_COST(alg_in.cost, best_cost))
         {
            best_cost = alg_in.cost;
            std::swap(alg_in, best_alg);
            best_alg.log[best_alg.ops] = static_cast<char>(m);
            best_alg.op[best_alg.ops] = alg_sub_factor;
         }
         break;
      }
   }
   if(cache_hit)
      goto done;

   /** Try shift-and-add (load effective address) instructions,
      i.e. do a*3, a*5, a*9.  */
   if((t & 1) != 0)
   {
   do_alg_add_t2_m:
      q = t - 1;
      q = q & -q;
      m = exact_log2(q);
      if(m >= 0 && m < maxm)
      {
         op_cost = 1 /*shiftadd_cost[speed][mode][m]*/;
         new_limit.cost = static_cast<short>(best_cost.cost - op_cost);
         new_limit.latency = static_cast<short>(best_cost.latency - op_cost);
         synth_mult(alg_in, (t - 1) >> m, new_limit, data_bitsize, TM);

         alg_in.cost.cost = static_cast<short>(alg_in.cost.cost + op_cost);
         alg_in.cost.latency = static_cast<short>(alg_in.cost.latency + op_cost);
         if(CHEAPER_MULT_COST(alg_in.cost, best_cost))
         {
            best_cost = alg_in.cost;
            std::swap(alg_in, best_alg);
            best_alg.log[best_alg.ops] = static_cast<char>(m);
            best_alg.op[best_alg.ops] = alg_add_t2_m;
         }
      }
      if(cache_hit)
         goto done;

   do_alg_sub_t2_m:
      q = t + 1;
      q = q & -q;
      m = exact_log2(q);
      if(m >= 0 && m < maxm)
      {
         op_cost = 1 /*shiftsub0_cost[speed][mode][m]*/;
         new_limit.cost = static_cast<short>(best_cost.cost - op_cost);
         new_limit.latency = static_cast<short>(best_cost.latency - op_cost);
         synth_mult(alg_in, (t + 1) >> m, new_limit, data_bitsize, TM);

         alg_in.cost.cost = static_cast<short>(alg_in.cost.cost + op_cost);
         alg_in.cost.latency = static_cast<short>(alg_in.cost.latency + op_cost);
         if(CHEAPER_MULT_COST(alg_in.cost, best_cost))
         {
            best_cost = alg_in.cost;
            std::swap(alg_in, best_alg);
            best_alg.log[best_alg.ops] = static_cast<char>(m);
            best_alg.op[best_alg.ops] = alg_sub_t2_m;
         }
      }
      if(cache_hit)
         goto done;
   }

done:
   /** If best_cost has not decreased, we have not found any algorithm.  */
   if(!CHEAPER_MULT_COST(best_cost, cost_limit))
   {
      /** We failed to find an algorithm.  Record alg_impossible for
      this case (that is, <T, MODE, COST_LIMIT>) so that next time
      we are asked to find an algorithm for T within the same or
      lower COST_LIMIT, we can immediately return to the
      caller.  */
      alg_hash[std::make_pair(data_bitsize, t)] = std::make_pair(alg_impossible, cost_limit);
      return;
   }

   /** Cache the result.  */
   if(!cache_hit)
   {
      mult_cost last_limit = {best_cost.cost, best_cost.latency};
      alg_hash[std::make_pair(data_bitsize, t)] = std::make_pair(best_alg.op[best_alg.ops], last_limit);
   }

   /** If we are getting a too long sequence for `struct algorithm'
      to record, make this search fail.  */
   if(best_alg.ops == 64)
      return;

   /** Copy the algorithm from temporary space to the space at alg_out.
      We avoid using structure assignment because the majority of
      best_alg is normally undefined, and this is a critical function.  */
   alg_out = best_alg;
   alg_out.ops = static_cast<short>(best_alg.ops + 1);
}

/** Find the cheapest way of multiplying a value of mode MODE by VAL.
   Try three variations:
       - a shift/add sequence based on VAL itself
       - a shift/add sequence based on -VAL, followed by a negation
       - a shift/add sequence based on VAL - 1, followed by an addition.

   Return true if the cheapest of these cost less than MULT_COST,
   describing the algorithm in *ALG and final fixup in *VARIANT.  */
static bool choose_mult_variant(unsigned int data_bitsize, long long int val, struct algorithm& alg, enum mult_variant& variant, short int Mult_cost, tree_managerRef& TM)
{
   struct algorithm alg2;
   struct mult_cost limit;
   short int op_cost;

   /** Fail quickly for impossible bounds.  */
   if(Mult_cost < 0)
      return false;

   /** Ensure that Mult_cost provides a reasonable upper bound.
      Any constant multiplication can be performed with less
      than 2 * bits additions.  */
   op_cost = static_cast<short>(2 * data_bitsize * 1 /*add_cost[speed][mode]*/);
   if(Mult_cost > op_cost)
      Mult_cost = op_cost;

   variant = basic_variant;
   limit.cost = Mult_cost;
   limit.latency = Mult_cost;
   synth_mult(alg, static_cast<unsigned long long int>(val), limit, data_bitsize, TM);

   /* This works only if the inverted value actually fits in an
      `unsigned int' */
   if(8 * sizeof(int) >= data_bitsize)
   {
      op_cost = 1 /*neg_cost[speed][mode]*/;
      if(MULT_COST_LESS(alg.cost, Mult_cost))
      {
         limit.cost = static_cast<short>(alg.cost.cost - op_cost);
         limit.latency = static_cast<short>(alg.cost.latency - op_cost);
      }
      else
      {
         limit.cost = static_cast<short>(Mult_cost - op_cost);
         limit.latency = static_cast<short>(Mult_cost - op_cost);
      }

      synth_mult(alg2, static_cast<unsigned long long int>(-val), limit, data_bitsize, TM);
      alg2.cost.cost = static_cast<short>(alg2.cost.cost + op_cost);
      alg2.cost.latency = static_cast<short>(alg2.cost.latency + op_cost);
      if(CHEAPER_MULT_COST(alg2.cost, alg.cost))
         alg = alg2, variant = negate_variant;
   }

   /* This proves very useful for division-by-constant.  */
   op_cost = 1 /*add_cost[speed][mode]*/;
   if(MULT_COST_LESS(alg.cost, Mult_cost))
   {
      limit.cost = static_cast<short>(alg.cost.cost - op_cost);
      limit.latency = static_cast<short>(alg.cost.latency - op_cost);
   }
   else
   {
      limit.cost = static_cast<short>(Mult_cost - op_cost);
      limit.latency = static_cast<short>(Mult_cost - op_cost);
   }

   synth_mult(alg2, static_cast<unsigned long long int>(val - 1), limit, data_bitsize, TM);
   alg2.cost.cost = static_cast<short>(alg2.cost.cost + op_cost);
   alg2.cost.latency = static_cast<short>(alg2.cost.latency + op_cost);
   if(CHEAPER_MULT_COST(alg2.cost, alg.cost))
      alg = alg2, variant = add_variant;

   return MULT_COST_LESS(alg.cost, Mult_cost);
}

tree_nodeRef IR_lowering::expand_mult_const(tree_nodeRef op0, unsigned long long int ASSERT_PARAMETER(val), const struct algorithm& alg, enum mult_variant& variant, const tree_nodeRef stmt, const blocRef block, tree_nodeRef& type,
                                            const std::string& srcp_default)
{
   long long int val_so_far = 0;
   tree_nodeRef accum, tem;
   int opno;
   unsigned data_bitsize = tree_helper::size(TM, tree_helper::get_type_index(TM, GET_INDEX_NODE(type)));
   unsigned long long int data_mask = data_bitsize >= 64 ? ~0ULL : (1ULL << data_bitsize) - 1;
   tree_nodeRef tem_ga;
   tree_nodeRef COST0 = TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(type));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Expanding " + op0->ToString());

   /* ACCUM starts out either as OP0 or as a zero, depending on
      the first operation.  */

   if(alg.op[0] == alg_zero)
   {
      accum = COST0;
      val_so_far = 0;
   }
   else if(alg.op[0] == alg_m)
   {
      accum = op0;
      val_so_far = 1;
   }
   else
      THROW_ERROR("condition not expected");

   for(opno = 1; opno < alg.ops; opno++)
   {
      int log = alg.log[opno];
      tree_nodeRef log_node;

      if(log == 0)
         log_node = COST0;
      else
      {
         log_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(log), GET_INDEX_NODE(type));
      }

      switch(alg.op[opno])
      {
         case alg_shift:
            tem = tree_man->create_binary_operation(type, accum, log_node, srcp_default, lshift_expr_K);
            tem_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), tem, block->number, srcp_default);
            block->PushBefore(tem_ga, stmt);
            accum = GetPointer<gimple_assign>(GET_NODE(tem_ga))->op0;
            val_so_far <<= log;
            break;

         case alg_add_t_m2:
            if(log_node != COST0)
            {
               tem = tree_man->create_binary_operation(type, op0, log_node, srcp_default, lshift_expr_K);
               tem_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), tem, block->number, srcp_default);
               block->PushBefore(tem_ga, stmt);
               tem = GetPointer<gimple_assign>(GET_NODE(tem_ga))->op0;
            }
            else
               tem = op0;
            if(accum != COST0)
            {
               accum = tree_man->create_binary_operation(type, accum, tem, srcp_default, plus_expr_K);
               tem_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), accum, block->number, srcp_default);
               block->PushBefore(tem_ga, stmt);
               accum = GetPointer<gimple_assign>(GET_NODE(tem_ga))->op0;
            }
            else
               accum = tem;
            val_so_far += 1LL << log;
            break;

         case alg_sub_t_m2:
            if(log_node != COST0)
            {
               tem = tree_man->create_binary_operation(type, op0, log_node, srcp_default, lshift_expr_K);
               tem_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), tem, block->number, srcp_default);
               block->PushBefore(tem_ga, stmt);
               tem = GetPointer<gimple_assign>(GET_NODE(tem_ga))->op0;
            }
            else
               tem = op0;
            if(accum != COST0)
               accum = tree_man->create_binary_operation(type, accum, tem, srcp_default, minus_expr_K);
            else
               accum = tree_man->create_unary_operation(type, tem, srcp_default, negate_expr_K);
            tem_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), accum, block->number, srcp_default);
            block->PushBefore(tem_ga, stmt);
            accum = GetPointer<gimple_assign>(GET_NODE(tem_ga))->op0;
            val_so_far -= 1LL << log;
            break;

         case alg_add_t2_m:
            if(log_node != COST0 && accum != COST0)
            {
               accum = tree_man->create_binary_operation(type, accum, log_node, srcp_default, lshift_expr_K);
               tem_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), accum, block->number, srcp_default);
               block->PushBefore(tem_ga, stmt);
               accum = GetPointer<gimple_assign>(GET_NODE(tem_ga))->op0;
            }
            if(accum != COST0)
            {
               accum = tree_man->create_binary_operation(type, accum, op0, srcp_default, plus_expr_K);
               tem_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), accum, block->number, srcp_default);
               block->PushBefore(tem_ga, stmt);
               accum = GetPointer<gimple_assign>(GET_NODE(tem_ga))->op0;
            }
            else
               accum = op0;
            val_so_far = (val_so_far << log) + 1;
            break;

         case alg_sub_t2_m:
            if(log_node != COST0 && accum != COST0)
            {
               accum = tree_man->create_binary_operation(type, accum, log_node, srcp_default, lshift_expr_K);
               tem_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), accum, block->number, srcp_default);
               block->PushBefore(tem_ga, stmt);
               accum = GetPointer<gimple_assign>(GET_NODE(tem_ga))->op0;
            }
            if(accum != COST0)
               accum = tree_man->create_binary_operation(type, accum, op0, srcp_default, minus_expr_K);
            else
               accum = tree_man->create_unary_operation(type, op0, srcp_default, negate_expr_K);
            tem_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), accum, block->number, srcp_default);
            block->PushBefore(tem_ga, stmt);
            accum = GetPointer<gimple_assign>(GET_NODE(tem_ga))->op0;
            val_so_far = (val_so_far << log) - 1;
            break;

         case alg_add_factor:
            if(log_node != COST0 && accum != COST0)
            {
               tem = tree_man->create_binary_operation(type, accum, log_node, srcp_default, lshift_expr_K);
               tem_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), tem, block->number, srcp_default);
               block->PushBefore(tem_ga, stmt);
               tem = GetPointer<gimple_assign>(GET_NODE(tem_ga))->op0;
            }
            else
               tem = accum;
            if(accum != COST0)
            {
               accum = tree_man->create_binary_operation(type, accum, tem, srcp_default, plus_expr_K);
               tem_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), accum, block->number, srcp_default);
               block->PushBefore(tem_ga, stmt);
               accum = GetPointer<gimple_assign>(GET_NODE(tem_ga))->op0;
            }
            else
               accum = tem;
            val_so_far += val_so_far << log;
            break;

         case alg_sub_factor:
            if(log_node != COST0 && accum != COST0)
            {
               tem = tree_man->create_binary_operation(type, accum, log_node, srcp_default, lshift_expr_K);
               tem_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), tem, block->number, srcp_default);
               block->PushBefore(tem_ga, stmt);
               tem = GetPointer<gimple_assign>(GET_NODE(tem_ga))->op0;
            }
            else
               tem = accum;
            if(accum != COST0)
            {
               accum = tree_man->create_binary_operation(type, tem, accum, srcp_default, minus_expr_K);
               tem_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), accum, block->number, srcp_default);
               block->PushBefore(tem_ga, stmt);
               accum = GetPointer<gimple_assign>(GET_NODE(tem_ga))->op0;
            }
            else
               accum = tem;
            val_so_far = (val_so_far << log) - val_so_far;
            break;
         case alg_impossible:
         case alg_m:
         case alg_unknown:
         case alg_zero:
         default:
            THROW_UNREACHABLE("");
      }
   }

   if(variant == negate_variant)
   {
      val_so_far = -val_so_far;
      tem = tree_man->create_unary_operation(type, accum, srcp_default, negate_expr_K);
      tem_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), tem, block->number, srcp_default);
      block->PushBefore(tem_ga, stmt);
      accum = GetPointer<gimple_assign>(GET_NODE(tem_ga))->op0;
   }
   else if(variant == add_variant)
   {
      val_so_far = val_so_far + 1;
      tem = tree_man->create_binary_operation(type, accum, op0, srcp_default, plus_expr_K);
      tem_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), tem, block->number, srcp_default);
      block->PushBefore(tem_ga, stmt);
      accum = GetPointer<gimple_assign>(GET_NODE(tem_ga))->op0;
   }

   /** Compare only the bits of val and val_so_far that are significant
     in the result mode, to avoid sign-/zero-extension confusion.  */
#if HAVE_ASSERTS
   val = val & data_mask;
#endif
   val_so_far = val_so_far & static_cast<long long int>(data_mask);
   THROW_ASSERT(val == static_cast<unsigned long long int>(val_so_far), "unexpected difference");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Expanded " + op0->ToString());

   return accum;
}

tree_nodeRef IR_lowering::expand_smod_pow2(tree_nodeRef op0, unsigned long long int d, const tree_nodeRef stmt, const blocRef block, tree_nodeRef& type, const std::string& srcp_default)
{
   unsigned long long int masklow;
   int logd;

   logd = floor_log2(d);
   tree_nodeRef const0 = TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(type));

   tree_nodeRef bt = tree_man->create_boolean_type();
   tree_nodeRef cond_op0 = tree_man->create_binary_operation(bt, op0, const0, srcp_default, lt_expr_K);
   tree_nodeRef signmask_ga = tree_man->CreateGimpleAssign(type, TM->CreateUniqueIntegerCst(0, bt->index), TM->CreateUniqueIntegerCst(1, bt->index), cond_op0, block->number, srcp_default);
#ifndef NDEBUG
   AppM->RegisterTransformation(GetName(), signmask_ga);
#endif
   block->PushBefore(signmask_ga, stmt);
   tree_nodeRef signmask_var = GetPointer<gimple_assign>(GET_NODE(signmask_ga))->op0;

   if(logd < 0)
      THROW_ERROR("unexpected condition");
   masklow = (1ULL << logd) - 1;
   tree_nodeRef Constmasklow = TM->CreateUniqueIntegerCst(static_cast<long long int>(masklow), GET_INDEX_NODE(type));

   tree_nodeRef temp = tree_man->create_binary_operation(type, op0, signmask_var, srcp_default, bit_xor_expr_K);
   tree_nodeRef temp_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), temp, block->number, srcp_default);
#ifndef NDEBUG
   AppM->RegisterTransformation(GetName(), temp_ga);
#endif
   block->PushBefore(temp_ga, stmt);
   tree_nodeRef temp_var = GetPointer<gimple_assign>(GET_NODE(temp_ga))->op0;

   temp = tree_man->create_binary_operation(type, temp_var, signmask_var, srcp_default, minus_expr_K);
   temp_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), temp, block->number, srcp_default);
#ifndef NDEBUG
   AppM->RegisterTransformation(GetName(), temp_ga);
#endif
   block->PushBefore(temp_ga, stmt);
   temp_var = GetPointer<gimple_assign>(GET_NODE(temp_ga))->op0;

   temp = tree_man->create_binary_operation(type, temp_var, Constmasklow, srcp_default, bit_and_expr_K);
   temp_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), temp, block->number, srcp_default);
#ifndef NDEBUG
   AppM->RegisterTransformation(GetName(), temp_ga);
#endif
   block->PushBefore(temp_ga, stmt);
   temp_var = GetPointer<gimple_assign>(GET_NODE(temp_ga))->op0;

   temp = tree_man->create_binary_operation(type, temp_var, signmask_var, srcp_default, bit_xor_expr_K);
   temp_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), temp, block->number, srcp_default);
#ifndef NDEBUG
   AppM->RegisterTransformation(GetName(), temp_ga);
#endif
   block->PushBefore(temp_ga, stmt);
   temp_var = GetPointer<gimple_assign>(GET_NODE(temp_ga))->op0;

   return tree_man->create_binary_operation(type, temp_var, signmask_var, srcp_default, minus_expr_K);
}

tree_nodeRef IR_lowering::expand_sdiv_pow2(tree_nodeRef op0, unsigned long long int d, const tree_nodeRef stmt, const blocRef block, tree_nodeRef& type, const std::string& srcp_default)
{
   int logd;

   logd = floor_log2(d);
   tree_nodeRef const0 = TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(type));

   tree_nodeRef bt = tree_man->create_boolean_type();
   tree_nodeRef cond_op0 = tree_man->create_binary_operation(bt, op0, const0, srcp_default, lt_expr_K);
   tree_nodeRef cond_op0_ga = tree_man->CreateGimpleAssign(bt, TM->CreateUniqueIntegerCst(0, bt->index), TM->CreateUniqueIntegerCst(1, bt->index), cond_op0, block->number, srcp_default);
   block->PushBefore(cond_op0_ga, stmt);
   tree_nodeRef cond_op0_ga_var = GetPointer<gimple_assign>(GET_NODE(cond_op0_ga))->op0;
   tree_nodeRef t_ga;
   tree_nodeRef t2_ga;

   if(d == 2)
   {
      tree_nodeRef const1 = TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(type));
      tree_nodeRef cond_op = tree_man->create_ternary_operation(type, cond_op0_ga_var, const1, const0, srcp_default, cond_expr_K);
      t_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), cond_op, block->number, srcp_default);
      block->PushBefore(t_ga, stmt);
      tree_nodeRef cond_ga_var = GetPointer<gimple_assign>(GET_NODE(t_ga))->op0;

      tree_nodeRef sum_expr = tree_man->create_binary_operation(type, op0, cond_ga_var, srcp_default, plus_expr_K);

      t2_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), sum_expr, block->number, srcp_default);
   }
   else
   {
      tree_nodeRef d_m1 = TM->CreateUniqueIntegerCst(static_cast<long long int>(d - 1), GET_INDEX_NODE(type));

      tree_nodeRef t_expr = tree_man->create_binary_operation(type, op0, d_m1, srcp_default, plus_expr_K);
      t_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), t_expr, block->number, srcp_default);
      block->PushBefore(t_ga, stmt);
      tree_nodeRef t_ga_var = GetPointer<gimple_assign>(GET_NODE(t_ga))->op0;

      tree_nodeRef cond_op = tree_man->create_ternary_operation(type, cond_op0_ga_var, t_ga_var, op0, srcp_default, cond_expr_K);

      t2_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), cond_op, block->number, srcp_default);
   }

   block->PushBefore(t2_ga, stmt);

   tree_nodeRef t2_ga_var = GetPointer<gimple_assign>(GET_NODE(t2_ga))->op0;

   tree_nodeRef logdConst = TM->CreateUniqueIntegerCst(logd, GET_INDEX_NODE(type));
   return tree_man->create_binary_operation(type, t2_ga_var, logdConst, srcp_default, rshift_expr_K);
}

tree_nodeRef IR_lowering::expand_MC(tree_nodeRef op0, integer_cst* ic_node, tree_nodeRef old_target, const tree_nodeRef stmt, const blocRef block, tree_nodeRef& type_expr, const std::string& srcp_default)
{
   long long int ext_op1 = tree_helper::get_integer_cst_value(ic_node);
   short int mult_plus_ratio = 3;
   unsigned int data_bitsize = tree_helper::Size(GET_NODE(op0));
   unsigned int typeSize = tree_helper::Size(type_expr);
   if(typeSize < 64)
   {
      ext_op1 <<= 64 - typeSize;
      ext_op1 >>= 64 - typeSize;
   }
#if HAVE_BAMBU_BUILT
   if(GetPointer<HLS_manager>(AppM))
   {
      const HLS_targetRef HLS_T = GetPointer<HLS_manager>(AppM)->get_HLS_target();
      const technology_managerRef TechManager = HLS_T->get_technology_manager();
      unsigned int fu_prec = resize_to_1_8_16_32_64_128_256_512(data_bitsize);
      if(fu_prec == 1)
      {
         fu_prec = 8;
      }
      technology_nodeRef mult_f_unit = TechManager->get_fu(MULTIPLIER_STD + std::string("_") + STR(fu_prec) + "_" + STR(fu_prec) + "_" + STR(fu_prec) + "_0", LIBRARY_STD_FU);
      auto* mult_fu = GetPointer<functional_unit>(mult_f_unit);
      technology_nodeRef mult_op_node = mult_fu->get_operation("mult_expr");
      auto* mult_op = GetPointer<operation>(mult_op_node);
      double mult_delay = mult_op->time_m->get_execution_time();
      technology_nodeRef add_f_unit = TechManager->get_fu(ADDER_STD + std::string("_") + STR(fu_prec) + "_" + STR(fu_prec) + "_" + STR(fu_prec), LIBRARY_STD_FU);
      auto* add_fu = GetPointer<functional_unit>(add_f_unit);
      technology_nodeRef add_op_node = add_fu->get_operation("plus_expr");
      auto* add_op = GetPointer<operation>(add_op_node);
      double add_delay = add_op->time_m->get_execution_time();
      mult_plus_ratio = static_cast<short int>(ceil(mult_delay / add_delay));
   }
#endif

   /// very special case op1 == 0
   if(ext_op1 == 0)
   {
      return TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(type_expr));
   }
   /// very special case op1 == 1
   else if(ext_op1 == 1)
   {
      return op0;
   }
   /// very special case op1 == -1
   else if(ext_op1 == -1)
   {
      return tree_man->create_unary_operation(type_expr, op0, srcp_default, negate_expr_K);
   }
   else
   {
      if(ext_op1 < 0)
      {
         struct algorithm alg;
         enum mult_variant variant;
         short int max_cost = mult_plus_ratio; // static_cast<short int>(tree_helper::size(TM, tree_helper::get_type_index(TM,GET_INDEX_NODE(type_expr)))/mult_cost_divisor);
         if(choose_mult_variant(data_bitsize, -ext_op1, alg, variant, max_cost, TM))
         {
            tree_nodeRef temp_expr = expand_mult_const(op0, static_cast<unsigned long long int>(-ext_op1), alg, variant, stmt, block, type_expr, srcp_default);
            tree_nodeRef temp_expr_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(), temp_expr, block->number, srcp_default);
            block->PushBefore(temp_expr_ga, stmt);
            tree_nodeRef temp_expr_var = GetPointer<gimple_assign>(GET_NODE(temp_expr_ga))->op0;
            return tree_man->create_unary_operation(type_expr, temp_expr_var, srcp_default, negate_expr_K);
         }
         else
         {
            /// keep the old assign
            return old_target;
         }
      }
      else
      {
         auto coeff = static_cast<unsigned long long int>(ext_op1);
         if(EXACT_POWER_OF_2_OR_ZERO_P(coeff))
         {
            int l_shift = floor_log2(coeff);
            tree_nodeRef l_shift_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(l_shift), GET_INDEX_NODE(type_expr));
            return tree_man->create_binary_operation(type_expr, op0, l_shift_node, srcp_default, lshift_expr_K);
         }
         else
         {
            struct algorithm alg;
            enum mult_variant variant;
            short int max_cost = mult_plus_ratio; // static_cast<short int>(tree_helper::size(TM, tree_helper::get_type_index(TM,GET_INDEX_NODE(type_expr)))/mult_cost_divisor);

            if(choose_mult_variant(data_bitsize, static_cast<long long int>(coeff), alg, variant, max_cost, TM))
               return expand_mult_const(op0, coeff, alg, variant, stmt, block, type_expr, srcp_default);
            else
               /// keep the old assign
               return old_target;
         }
      }
   }
}

bool IR_lowering::expand_target_mem_ref(target_mem_ref461* tmr, const tree_nodeRef stmt, const blocRef block, const std::string& srcp_default, bool temp_addr)
{
   tree_nodeRef accum;
   tree_nodeRef type_sum;
   unsigned int params = 0;
   bool changed = false;

   if(tmr->idx)
      ++params;
   if(tmr->step)
      ++params;
   if(tmr->offset)
      ++params;
   if(tmr->idx2)
      ++params;

   if(params < 1)
      return changed; /// nothing to optimize

   if(tmr->idx)
   {
      if(tmr->step)
      {
         auto* ic_step_node = GetPointer<integer_cst>(GET_NODE(tmr->step));
         type_sum = ic_step_node->type;
         accum = expand_MC(tmr->idx, ic_step_node, tree_nodeRef(), stmt, block, type_sum, srcp_default);
         if(accum)
         {
            tree_nodeRef t_ga = tree_man->CreateGimpleAssign(type_sum, tree_nodeRef(), tree_nodeRef(), accum, block->number, srcp_default);
            block->PushBefore(t_ga, stmt);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + GET_NODE(t_ga)->ToString());
            accum = GetPointer<gimple_assign>(GET_NODE(t_ga))->op0;
         }
         else
         {
            tree_nodeRef t_expr = tree_man->create_binary_operation(type_sum, tmr->idx, tmr->step, srcp_default, mult_expr_K);
            tree_nodeRef t_ga = tree_man->CreateGimpleAssign(type_sum, tree_nodeRef(), tree_nodeRef(), t_expr, block->number, srcp_default);
            block->PushBefore(t_ga, stmt);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + GET_NODE(t_ga)->ToString());
            accum = GetPointer<gimple_assign>(GET_NODE(t_ga))->op0;
         }
         tmr->step = tree_nodeRef();
      }
      else
         accum = tmr->idx;
      tmr->idx = tree_nodeRef();
      changed = true;
   }
   if(tmr->offset)
   {
      auto* ic_node = GetPointer<integer_cst>(GET_NODE(tmr->offset));
      long long int ic_value = tree_helper::get_integer_cst_value(ic_node);
      if(ic_value != 0)
      {
         if(!type_sum)
            type_sum = tree_man->create_size_type();

         tree_nodeRef ne = tree_man->create_unary_operation(type_sum, tmr->offset, srcp_default, nop_expr_K);
         tree_nodeRef casted_offset_ga = tree_man->CreateGimpleAssign(type_sum, tree_nodeRef(), tree_nodeRef(), ne, block->number, srcp_default);
         block->PushBefore(casted_offset_ga, stmt);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + GET_NODE(casted_offset_ga)->ToString());
         tree_nodeRef casted_offset_var = GetPointer<gimple_assign>(GET_NODE(casted_offset_ga))->op0;

         if(accum)
         {
            tree_nodeRef t_expr = tree_man->create_binary_operation(type_sum, casted_offset_var, accum, srcp_default, plus_expr_K);
            tree_nodeRef t_ga;
            t_ga = tree_man->CreateGimpleAssign(type_sum, tree_nodeRef(), tree_nodeRef(), t_expr, block->number, srcp_default);
            block->PushBefore(t_ga, stmt);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + GET_NODE(t_ga)->ToString());
            accum = GetPointer<gimple_assign>(GET_NODE(t_ga))->op0;
         }
         else
            accum = casted_offset_var;
      }
      tmr->offset = tree_nodeRef();
      changed = true;
   }

   if(tmr->idx2)
   {
      if(!type_sum)
         type_sum = tree_man->create_size_type();
      unsigned int type_index = tree_helper::get_type_index(TM, GET_INDEX_NODE(tmr->idx2));
      if(type_index != GET_INDEX_NODE(type_sum))
      {
         tree_nodeRef ne = tree_man->create_unary_operation(type_sum, tmr->idx2, srcp_default, nop_expr_K);
         tree_nodeRef casted_idx2_ga = tree_man->CreateGimpleAssign(type_sum, tree_nodeRef(), tree_nodeRef(), ne, block->number, srcp_default);
         block->PushBefore(casted_idx2_ga, stmt);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + GET_NODE(casted_idx2_ga)->ToString());
         tree_nodeRef casted_idx2_var = GetPointer<gimple_assign>(GET_NODE(casted_idx2_ga))->op0;
         if(accum)
         {
            tree_nodeRef t_expr = tree_man->create_binary_operation(type_sum, accum, casted_idx2_var, srcp_default, plus_expr_K);
            tree_nodeRef t_ga = tree_man->CreateGimpleAssign(type_sum, tree_nodeRef(), tree_nodeRef(), t_expr, block->number, srcp_default);
            block->PushBefore(t_ga, stmt);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + GET_NODE(t_ga)->ToString());
            accum = GetPointer<gimple_assign>(GET_NODE(t_ga))->op0;
         }
         else
            accum = casted_idx2_var;
      }
      else if(accum)
      {
         tree_nodeRef t_expr = tree_man->create_binary_operation(type_sum, accum, tmr->idx2, srcp_default, plus_expr_K);
         tree_nodeRef t_ga = tree_man->CreateGimpleAssign(type_sum, tree_nodeRef(), tree_nodeRef(), t_expr, block->number, srcp_default);
         block->PushBefore(t_ga, stmt);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + GET_NODE(t_ga)->ToString());
         accum = GetPointer<gimple_assign>(GET_NODE(t_ga))->op0;
      }
      else
         accum = tmr->idx2;
      changed = true;
      tmr->idx2 = tree_nodeRef();
   }

   if(GET_NODE(tmr->base)->get_kind() == addr_expr_K)
   {
      auto* ae = GetPointer<addr_expr>(GET_NODE(tmr->base));
      tree_nodeRef ae_expr = tree_man->create_unary_operation(ae->type, ae->op, srcp_default, addr_expr_K); /// It is required to de-share some IR nodes
      tree_nodeRef ae_ga = tree_man->CreateGimpleAssign(ae->type, tree_nodeRef(), tree_nodeRef(), ae_expr, block->number, srcp_default);
      tree_nodeRef ae_vd = GetPointer<gimple_assign>(GET_NODE(ae_ga))->op0;
      GetPointer<gimple_assign>(GET_NODE(ae_ga))->temporary_address = temp_addr;
      block->PushBefore(ae_ga, stmt);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + GET_NODE(ae_ga)->ToString());
      tmr->base = ae_vd;
      changed = true;
   }
   if(accum)
   {
      tree_nodeRef type = tmr->type;
      tree_nodeRef pt = tree_man->create_pointer_type(type, 8);

      tree_nodeRef ppe_expr = tree_man->create_binary_operation(pt, tmr->base, accum, srcp_default, pointer_plus_expr_K);
      tree_nodeRef ppe_ga = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), ppe_expr, block->number, srcp_default);
      tree_nodeRef ppe_vd = GetPointer<gimple_assign>(GET_NODE(ppe_ga))->op0;
      tmr->base = ppe_vd;
      GetPointer<gimple_assign>(GET_NODE(ppe_ga))->temporary_address = temp_addr;
      block->PushBefore(ppe_ga, stmt);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + GET_NODE(ppe_ga)->ToString());
   }

   return changed;
}

tree_nodeRef IR_lowering::expand_mult_highpart(tree_nodeRef op0, unsigned long long int ml, tree_nodeRef type_expr, int data_bitsize, const std::list<tree_nodeRef>::const_iterator it_los, const blocRef block, const std::string& srcp_default)
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
   tree_nodeRef mask_node = TM->CreateUniqueIntegerCst(static_cast<long long int>((1LL << half_data_bitsize) - 1), GET_INDEX_NODE(type_expr));
   tree_nodeRef half_data_bitsize_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(half_data_bitsize), GET_INDEX_NODE(type_expr));

   tree_nodeRef u0_expr = tree_man->create_binary_operation(type_expr, op0, mask_node, srcp_default, bit_and_expr_K);
   tree_nodeRef u0_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(), u0_expr, block->number, srcp_default);
   block->PushBefore(u0_ga, *it_los);
   tree_nodeRef u0_ga_var = GetPointer<gimple_assign>(GET_NODE(u0_ga))->op0;

   tree_nodeRef u1_expr = tree_man->create_binary_operation(type_expr, op0, half_data_bitsize_node, srcp_default, rshift_expr_K);
   tree_nodeRef u1_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(), u1_expr, block->number, srcp_default);
   block->PushBefore(u1_ga, *it_los);
   tree_nodeRef u1_ga_var = GetPointer<gimple_assign>(GET_NODE(u1_ga))->op0;

   long long int v0 = static_cast<long long int>(ml) & ((1LL << half_data_bitsize) - 1);
   long long int v1;
   bool unsignedp = tree_helper::is_unsigned(TM, GET_INDEX_NODE(type_expr));
   v1 = static_cast<long long int>(ml >> half_data_bitsize);

   tree_nodeRef u0v0_ga_var;
   tree_nodeRef v0_node;
   tree_nodeRef v1_node;
   if(v0 != 0)
   {
      if(v0 == 1)
         u0v0_ga_var = u0_ga_var;
      else
      {
         v0_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(v0), GET_INDEX_NODE(type_expr));
         tree_nodeRef u0v0_expr = tree_man->create_binary_operation(type_expr, u0_ga_var, v0_node, srcp_default, mult_expr_K);
         tree_nodeRef u0v0_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(), u0v0_expr, block->number, srcp_default);
         block->PushBefore(u0v0_ga, *it_los);
         u0v0_ga_var = GetPointer<gimple_assign>(GET_NODE(u0v0_ga))->op0;
      }
   }
   tree_nodeRef u0v0h_ga_var;
   if(u0v0_ga_var)
   {
      tree_nodeRef u0v0h_expr = tree_man->create_binary_operation(type_expr, u0v0_ga_var, half_data_bitsize_node, srcp_default, rshift_expr_K);
      tree_nodeRef u0v0h_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(), u0v0h_expr, block->number, srcp_default);
      block->PushBefore(u0v0h_ga, *it_los);
      u0v0h_ga_var = GetPointer<gimple_assign>(GET_NODE(u0v0h_ga))->op0;
   }
   tree_nodeRef u0v0hU_ga_var;
   if(u0v0h_ga_var && !unsignedp)
   {
      tree_nodeRef u0v0hU_expr = tree_man->create_binary_operation(type_expr, u0v0h_ga_var, mask_node, srcp_default, bit_and_expr_K);
      tree_nodeRef u0v0hU_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(), u0v0hU_expr, block->number, srcp_default);
      block->PushBefore(u0v0hU_ga, *it_los);
      u0v0hU_ga_var = GetPointer<gimple_assign>(GET_NODE(u0v0hU_ga))->op0;
   }
   else
      u0v0hU_ga_var = u0v0h_ga_var;
   tree_nodeRef u1v0_ga_var;
   if(v0 != 0)
   {
      if(v0 == 1)
         u1v0_ga_var = u1_ga_var;
      else
      {
         tree_nodeRef u1v0_expr = tree_man->create_binary_operation(type_expr, u1_ga_var, v0_node, srcp_default, mult_expr_K);
         tree_nodeRef u1v0_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(), u1v0_expr, block->number, srcp_default);
         block->PushBefore(u1v0_ga, *it_los);
         u1v0_ga_var = GetPointer<gimple_assign>(GET_NODE(u1v0_ga))->op0;
      }
   }
   tree_nodeRef u0v0hu1v0_ga_var;
   if(u0v0hU_ga_var)
   {
      THROW_ASSERT(u1v0_ga_var, "unexpected condition");
      tree_nodeRef u0v0hu1v0_expr = tree_man->create_binary_operation(type_expr, u0v0hU_ga_var, u1v0_ga_var, srcp_default, plus_expr_K);
      tree_nodeRef u0v0hu1v0_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(), u0v0hu1v0_expr, block->number, srcp_default);
      block->PushBefore(u0v0hu1v0_ga, *it_los);
      u0v0hu1v0_ga_var = GetPointer<gimple_assign>(GET_NODE(u0v0hu1v0_ga))->op0;
   }
   tree_nodeRef w1_ga_var;
   if(u0v0hu1v0_ga_var)
   {
      tree_nodeRef w1_expr = tree_man->create_binary_operation(type_expr, u0v0hu1v0_ga_var, mask_node, srcp_default, bit_and_expr_K);
      tree_nodeRef w1_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(), w1_expr, block->number, srcp_default);
      block->PushBefore(w1_ga, *it_los);
      w1_ga_var = GetPointer<gimple_assign>(GET_NODE(w1_ga))->op0;
   }

   tree_nodeRef w2_ga_var;
   if(u0v0hu1v0_ga_var)
   {
      tree_nodeRef w2_expr = tree_man->create_binary_operation(type_expr, u0v0hu1v0_ga_var, half_data_bitsize_node, srcp_default, rshift_expr_K);
      tree_nodeRef w2_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(), w2_expr, block->number, srcp_default);
      block->PushBefore(w2_ga, *it_los);
      w2_ga_var = GetPointer<gimple_assign>(GET_NODE(w2_ga))->op0;
   }

   tree_nodeRef u0v1_ga_var;
   if(v1 != 0)
   {
      if(v1 == 1)
         u0v1_ga_var = u0_ga_var;
      else
      {
         v1_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(v1), GET_INDEX_NODE(type_expr));
         tree_nodeRef u0v1_expr = tree_man->create_binary_operation(type_expr, u0_ga_var, v1_node, srcp_default, mult_expr_K);
         tree_nodeRef u0v1_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(), u0v1_expr, block->number, srcp_default);
         block->PushBefore(u0v1_ga, *it_los);
         u0v1_ga_var = GetPointer<gimple_assign>(GET_NODE(u0v1_ga))->op0;
      }
   }
   tree_nodeRef w1u0v1_ga_var;
   if(w1_ga_var)
   {
      if(u0v1_ga_var)
      {
         tree_nodeRef w1u0v1_expr = tree_man->create_binary_operation(type_expr, w1_ga_var, u0v1_ga_var, srcp_default, plus_expr_K);
         tree_nodeRef w1u0v1_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(), w1u0v1_expr, block->number, srcp_default);
         block->PushBefore(w1u0v1_ga, *it_los);
         w1u0v1_ga_var = GetPointer<gimple_assign>(GET_NODE(w1u0v1_ga))->op0;
      }
      else
         w1u0v1_ga_var = w1_ga_var;
   }
   else if(u0v1_ga_var)
      w1u0v1_ga_var = u0v1_ga_var;

   tree_nodeRef w1u0v1h_ga_var;
   if(w1u0v1_ga_var)
   {
      tree_nodeRef w1u0v1h_expr = tree_man->create_binary_operation(type_expr, w1u0v1_ga_var, half_data_bitsize_node, srcp_default, rshift_expr_K);
      tree_nodeRef w1u0v1h_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(), w1u0v1h_expr, block->number, srcp_default);
      block->PushBefore(w1u0v1h_ga, *it_los);
      w1u0v1h_ga_var = GetPointer<gimple_assign>(GET_NODE(w1u0v1h_ga))->op0;
   }
   tree_nodeRef u1v1_ga_var;
   if(v1 != 0)
   {
      if(v1 == 1)
         u1v1_ga_var = u1_ga_var;
      else
      {
         tree_nodeRef u1v1_expr = tree_man->create_binary_operation(type_expr, u1_ga_var, v1_node, srcp_default, mult_expr_K);
         tree_nodeRef u1v1_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(), u1v1_expr, block->number, srcp_default);
         block->PushBefore(u1v1_ga, *it_los);
         u1v1_ga_var = GetPointer<gimple_assign>(GET_NODE(u1v1_ga))->op0;
      }
   }
   tree_nodeRef w1u0v1hw2_ga_var;
   if(w1u0v1h_ga_var)
   {
      if(w2_ga_var)
      {
         tree_nodeRef w1u0v1hw2_expr = tree_man->create_binary_operation(type_expr, w1u0v1h_ga_var, w2_ga_var, srcp_default, plus_expr_K);
         tree_nodeRef w1u0v1hw2_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(), w1u0v1hw2_expr, block->number, srcp_default);
         block->PushBefore(w1u0v1hw2_ga, *it_los);
         w1u0v1hw2_ga_var = GetPointer<gimple_assign>(GET_NODE(w1u0v1hw2_ga))->op0;
      }
      else
         w1u0v1hw2_ga_var = w1u0v1h_ga_var;
   }
   else if(w2_ga_var)
      w1u0v1hw2_ga_var = w2_ga_var;

   tree_nodeRef res_ga_var;
   if(w1u0v1hw2_ga_var)
   {
      if(u1v1_ga_var)
      {
         tree_nodeRef res_expr = tree_man->create_binary_operation(type_expr, w1u0v1hw2_ga_var, u1v1_ga_var, srcp_default, plus_expr_K);
         tree_nodeRef res_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(), res_expr, block->number, srcp_default);
         block->PushBefore(res_ga, *it_los);
         res_ga_var = GetPointer<gimple_assign>(GET_NODE(res_ga))->op0;
      }
      else
         res_ga_var = w1u0v1hw2_ga_var;
   }
   else if(u1v1_ga_var)
      res_ga_var = u1v1_ga_var;
   else
   {
      res_ga_var = TM->CreateUniqueIntegerCst(static_cast<long long int>(0), GET_INDEX_NODE(type_expr));
   }
   return res_ga_var;
}

tree_nodeRef IR_lowering::array_ref_lowering(array_ref* AR, const std::string& srcp_default, std::pair<unsigned int, blocRef> block, std::list<tree_nodeRef>::const_iterator it_los, bool temp_addr)
{
   tree_nodeRef type = AR->type;

   tree_nodeRef pt = tree_man->create_pointer_type(type, GetPointer<type_node>(GET_NODE(type))->algn);
   tree_nodeRef ae = tree_man->create_unary_operation(pt, AR->op0, srcp_default, addr_expr_K);
   tree_nodeRef ae_ga = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), ae, block.first, srcp_default);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + GET_NODE(ae_ga)->ToString());
   GetPointer<gimple_assign>(GET_NODE(ae_ga))->temporary_address = temp_addr;
   tree_nodeRef ae_vd = GetPointer<gimple_assign>(GET_NODE(ae_ga))->op0;
   block.second->PushBefore(ae_ga, *it_los);

   tree_nodeRef offset_type = tree_man->create_size_type();
   unsigned int ar_op1_type_index;
   tree_helper::get_type_node(GET_NODE(AR->op1), ar_op1_type_index);
   tree_nodeRef offset_node;
   if(ar_op1_type_index != GET_INDEX_NODE(offset_type))
   {
      tree_nodeRef ne = tree_man->create_unary_operation(offset_type, AR->op1, srcp_default, nop_expr_K);
      tree_nodeRef nop_ga = tree_man->CreateGimpleAssign(offset_type, tree_nodeRef(), tree_nodeRef(), ne, block.first, srcp_default);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + GET_NODE(nop_ga)->ToString());
      offset_node = GetPointer<gimple_assign>(GET_NODE(nop_ga))->op0;
      block.second->PushBefore(nop_ga, *it_los);
   }
   else
   {
      offset_node = AR->op1;
   }
   unsigned ar_op0_type_index;
   tree_nodeRef ar_op0_type_node = tree_helper::get_type_node(GET_NODE(AR->op0), ar_op0_type_index);
   THROW_ASSERT(ar_op0_type_node->get_kind() == array_type_K, "array_type expected: @" + STR(ar_op0_type_index));
   unsigned int data_bitsize = tree_helper::get_array_data_bitsize(TM, ar_op0_type_index);
   unsigned int n_byte = compute_n_bytes(data_bitsize);
   std::vector<unsigned int> dims;
   tree_helper::get_array_dimensions(TM, ar_op0_type_index, dims);
   for(size_t ind = 1; ind < dims.size(); ++ind)
      n_byte *= dims.at(ind);
   tree_nodeRef coef_node = TM->CreateUniqueIntegerCst(n_byte, GET_INDEX_NODE(offset_type));
   tree_nodeRef m = tree_man->create_binary_operation(offset_type, offset_node, coef_node, srcp_default, mult_expr_K);
   tree_nodeRef m_ga = tree_man->CreateGimpleAssign(offset_type, tree_nodeRef(), tree_nodeRef(), m, block.first, srcp_default);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + GET_NODE(m_ga)->ToString());
   tree_nodeRef m_vd = GetPointer<gimple_assign>(GET_NODE(m_ga))->op0;
   block.second->PushBefore(m_ga, *it_los);

   tree_nodeRef pp = tree_man->create_binary_operation(pt, ae_vd, m_vd, srcp_default, pointer_plus_expr_K);
   tree_nodeRef pp_ga = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), pp, block.first, srcp_default);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + GET_NODE(pp_ga)->ToString());
   GetPointer<gimple_assign>(GET_NODE(pp_ga))->temporary_address = temp_addr;
   tree_nodeRef pp_vd = GetPointer<gimple_assign>(GET_NODE(pp_ga))->op0;
   block.second->PushBefore(pp_ga, *it_los);

   tree_nodeRef offset = TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(pt));
   return tree_man->create_binary_operation(type, pp_vd, offset, srcp_default, mem_ref_K);
}

bool IR_lowering::reached_max_transformation_limit(tree_nodeRef
#ifndef NDEBUG
                                                       stmt
#endif
)
{
#ifndef NDEBUG
   if(stmt)
   {
      const auto ga = GetPointer<const gimple_assign>(GET_CONST_NODE(stmt));
      if(ga)
      {
         const auto op0 = GET_CONST_NODE(ga->op0);
         const auto op1 = GET_CONST_NODE(ga->op1);
         if(op1->get_kind() == cond_expr_K)
         {
            return false;
         }
         if(not ga->init_assignment and op0->get_kind() == ssa_name_K and op1->get_kind() == var_decl_K)
         {
            return false;
         }
      }
   }
   if(not AppM->ApplyNewTransformation())
      return true;
#endif
   return false;
}
