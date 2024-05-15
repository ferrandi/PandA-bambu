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
 *              Copyright (C) 2019-2024 Politecnico di Milano
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
 * @file lut_transformation.cpp
 * @brief identify and optimize lut expressions.
 * @author Marco Speziali
 * @author Davide Toschi
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "lut_transformation.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
#define USE_SAT 0
#pragma region needed by mockturtle / algorithms / satlut_mapping.hpp
#define LIN64
#define ABC_NAMESPACE pabc
#define ABC_NO_USE_READLINE
#pragma endregion
#define FMT_HEADER_ONLY 1
#define PHMAP_BIDIRECTIONAL 0
#define MCDBGQ_NOLOCKFREE_FREELIST 0
#define MCDBGQ_TRACKMEM 0
#define MCDBGQ_NOLOCKFREE_IMPLICITPRODBLOCKINDEX 0
#define MCDBGQ_NOLOCKFREE_IMPLICITPRODHASH 0
#define MCDBGQ_USEDEBUGFREELIST 0
#define DISABLE_NAUTY
#define MIG_SYNTHESIS 0

#include <kitty/print.hpp>
#include <mockturtle/algorithms/aig_resub.hpp>
#include <mockturtle/algorithms/balancing.hpp>
#include <mockturtle/algorithms/balancing/sop_balancing.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/collapse_mapped.hpp>
#include <mockturtle/algorithms/cut_rewriting.hpp>
#include <mockturtle/algorithms/functional_reduction.hpp>
#include <mockturtle/algorithms/lut_mapping.hpp>
#include <mockturtle/algorithms/node_resynthesis.hpp>
#include <mockturtle/algorithms/node_resynthesis/bidecomposition.hpp>
#include <mockturtle/algorithms/node_resynthesis/dsd.hpp>
#include <mockturtle/algorithms/node_resynthesis/shannon.hpp>
#include <mockturtle/algorithms/node_resynthesis/xag_npn.hpp>
#include <mockturtle/algorithms/refactoring.hpp>
#include <mockturtle/generators/arithmetic.hpp>
#include <mockturtle/io/write_bench.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/views/mapping_view.hpp>
#include <type_traits>
#if MIG_SYNTHESIS
#include <mockturtle/algorithms/mig_algebraic_rewriting.hpp>
#include <mockturtle/algorithms/node_resynthesis/mig_npn.hpp>
#endif
#if USE_SAT
#include <mockturtle/algorithms/satlut_mapping.hpp>
#endif

#include "Parameter.hpp"
#include "allocation_constants.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "function_behavior.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "math_function.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "technology_flow_step.hpp"
#include "technology_flow_step_factory.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "time_info.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include <fstream>

#pragma region Macros declaration

bool lut_transformation::CHECK_BIN_EXPR_BOOL_SIZE(binary_expr* be) const
{
   auto b0 = tree_helper::CGetType(be->op0);
   if(tree_helper::IsRealType(b0) || tree_helper::IsComplexType(b0) || tree_helper::IsVectorType(b0) ||
      tree_helper::IsStructType(b0))
   {
      return false;
   }
   auto b1 = tree_helper::CGetType(be->op1);
   if(tree_helper::IsRealType(b1) || tree_helper::IsComplexType(b1) || tree_helper::IsVectorType(b1) ||
      tree_helper::IsStructType(b1))
   {
      return false;
   }
   if(tree_helper::IsSignedIntegerType(be->op0) || tree_helper::IsSignedIntegerType(be->op1))
   {
      return false;
   }
   return tree_helper::Size(be->op0) == 1 && tree_helper::Size(be->op1) == 1;
}
bool lut_transformation::CHECK_BIN_EXPR_INT_SIZE(binary_expr* be, unsigned int max) const
{
   auto b0 = tree_helper::CGetType(be->op0);
   if(tree_helper::IsRealType(b0) || tree_helper::IsComplexType(b0) || tree_helper::IsVectorType(b0) ||
      tree_helper::IsStructType(b0))
   {
      return false;
   }
   auto b1 = tree_helper::CGetType(be->op1);
   if(tree_helper::IsRealType(b1) || tree_helper::IsComplexType(b1) || tree_helper::IsVectorType(b1) ||
      tree_helper::IsStructType(b1))
   {
      return false;
   }
   bool is_simple_case = [&]() -> bool {
      if(be->op1->get_kind() != integer_cst_K)
      {
         return false;
      }
      const auto cst_val = tree_helper::GetConstValue(be->op1);
      auto k = be->get_kind();
      return (k == lt_expr_K || k == gt_expr_K) && cst_val == 0;
   }();
   return (tree_helper::Size(be->op0) <= max && tree_helper::Size(be->op1) <= max) ||
          (is_simple_case && !parameters->isOption(OPT_context_switch));
}
bool lut_transformation::CHECK_COND_EXPR_SIZE(cond_expr* ce) const
{
   auto c0 = tree_helper::CGetType(ce->op1);
   if(tree_helper::IsRealType(c0) || tree_helper::IsComplexType(c0) || tree_helper::IsVectorType(c0) ||
      tree_helper::IsStructType(c0))
   {
      return false;
   }
   auto c1 = tree_helper::CGetType(ce->op2);
   if(tree_helper::IsRealType(c1) || tree_helper::IsComplexType(c1) || tree_helper::IsVectorType(c1) ||
      tree_helper::IsStructType(c1))
   {
      return false;
   }
   if(tree_helper::IsSignedIntegerType(ce->op1) || tree_helper::IsSignedIntegerType(ce->op2))
   {
      return false;
   }
   return tree_helper::Size(ce->op1) == 1 && tree_helper::Size(ce->op2) == 1;
}
bool lut_transformation::CHECK_NOT_EXPR_SIZE(unary_expr* ne) const
{
   auto c0 = tree_helper::CGetType(ne->op);
   if(tree_helper::IsRealType(c0) || tree_helper::IsComplexType(c0) || tree_helper::IsVectorType(c0) ||
      tree_helper::IsStructType(c0))
   {
      return false;
   }
   if(tree_helper::IsSignedIntegerType(ne->op))
   {
      return false;
   }
   return tree_helper::Size(ne->op) == 1;
}

#define VECT_CONTAINS(v, x) (std::find((v).begin(), (v).end(), x) != (v).end())

bool lut_transformation::cannotBeLUT(tree_nodeRef op) const
{
   auto op_node = op;
   auto code = op_node->get_kind();

   return !(GetPointer<lut_expr>(op_node) ||
            (GetPointer<truth_not_expr>(op_node) && CHECK_NOT_EXPR_SIZE(GetPointer<truth_not_expr>(op_node))) ||
            (GetPointer<bit_not_expr>(op_node) && CHECK_NOT_EXPR_SIZE(GetPointer<bit_not_expr>(op_node))) ||
            (GetPointer<cond_expr>(op_node) && CHECK_COND_EXPR_SIZE(GetPointer<cond_expr>(op_node))) ||
            (VECT_CONTAINS(lutBooleanExpressibleOperations, code) && GetPointer<binary_expr>(op_node) &&
             CHECK_BIN_EXPR_BOOL_SIZE(GetPointer<binary_expr>(op_node))) ||
            (VECT_CONTAINS(lutIntegerExpressibleOperations, code) && GetPointer<binary_expr>(op_node) &&
             CHECK_BIN_EXPR_INT_SIZE(GetPointer<binary_expr>(op_node),
                                     parameters->GetParameter<unsigned int>("MAX_LUT_INT_SIZE"))));
}

#pragma endregion

#pragma region Types declaration

/**
 * `klut_network_ext` class provides operations derived from the one already existing in `mockturtle::klut_network`.
 */
class klut_network_ext : public mockturtle::klut_network
{
 private:
   void fix_inputs_size(std::vector<signal>* a, std::vector<signal>* b, bool signedValues)
   {
      if(a->size() > b->size())
      {
         auto msbPos = b->size() - 1;
         auto niter = a->size() - b->size();
         for(size_t i = 0; i < niter; ++i)
         {
            if(signedValues)
            {
               b->push_back(b->at(msbPos));
            }
            else
            {
               b->push_back(this->get_constant(false));
            }
         }
      }
      else if(a->size() < b->size())
      {
         auto msbPos = a->size() - 1;
         auto niter = b->size() - a->size();
         for(size_t i = 0; i < niter; ++i)
         {
            if(signedValues)
            {
               a->push_back(a->at(msbPos));
            }
            else
            {
               a->push_back(this->get_constant(false));
            }
         }
      }
   }

   kitty::dynamic_truth_table create_lt_tt(unsigned int bits)
   {
      kitty::dynamic_truth_table tt(bits * 2);
      kitty::clear(tt);

      for(unsigned int i = 0; i < bits; ++i)
      {
         for(unsigned int j = i + 1; j < bits; ++j)
         {
            kitty::set_bit(tt, j + i * bits);
         }
      }

      return tt;
   }

 public:
#pragma region single - bit operations
   /**
    * Creates a 'greater' or equal operation.
    *
    * @param a a `mockturtle::klut_network::signal` representing the first operator of the `ge` operation
    * @param b a `mockturtle::klut_network::signal` representing the second operator of the `ge` operation
    *
    * @return a `mockturtle::klut_network::signal` representing the operation `ge` between `a` and `b`
    */
   signal create_ge(signal const a, signal const b)
   {
      return this->create_not(this->create_lt(a, b));
   }

   /**
    * Creates a 'greater' operation.
    *
    * @param a a `mockturtle::klut_network::signal` representing the first operator of the `gt` operation
    * @param b a `mockturtle::klut_network::signal` representing the second operator of the `gt` operation
    *
    * @return a `mockturtle::klut_network::signal` representing the operation `gt` between `a` and `b`
    */
   signal create_gt(signal const a, signal const b)
   {
      return this->create_not(this->create_le(a, b));
   }

   /**
    * Creates a 'equal' operation.
    *
    * @param a a `mockturtle::klut_network::signal` representing the first operator of the `gt` operation
    * @param b a `mockturtle::klut_network::signal` representing the second operator of the `gt` operation
    *
    * @return a `mockturtle::klut_network::signal` representing the operation `eq` between `a` and `b`
    */
   signal create_eq(signal const a, signal const b)
   {
      return this->create_not(this->create_xor(a, b));
   }

   /**
    * Creates a 'not equal' operation.
    *
    * @param a a `mockturtle::klut_network::signal` representing the first operator of the `ne` operation
    * @param b a `mockturtle::klut_network::signal` representing the second operator of the `ne` operation
    *
    * @return a `mockturtle::klut_network::signal` representing the operation `ne` between `a` and `b`
    */
   signal create_ne(signal const a, signal const b)
   {
      return this->create_xor(a, b);
   }
#pragma endregion

#pragma region utilities

   std::vector<signal> create_pi_v(size_t size)
   {
      std::vector<signal> pis(size);

      for(size_t i = 0; i < size; ++i)
      {
         pis[i] = create_pi();
      }

      return pis;
   }

   void create_po_v(std::vector<signal> pos)
   {
      for(size_t i = 0; i < pos.size(); ++i)
      {
         create_po(pos[i]);
      }
   }

   std::vector<signal> get_constant_v(std::vector<bool> bits)
   {
      std::vector<signal> outputs;
      for(auto b : bits)
      {
         outputs.push_back(b == false ? this->get_constant(false) : this->create_not(this->get_constant(false)));
      }

      return outputs;
   }

   /**
    * Creates a 'lut' operation from an `std::vector` of `mockturtle::klut_network::signal` with the associated
    * constant.
    *
    * @param s an `std::vector` of `mockturtle::klut_network::signal` containing the inputs of the lut
    * @param f the constant associated to the lut
    *
    * @return a `mockturtle::klut_network::signal` representing a lut between `s` with constant `f`
    */
   signal create_lut(std::vector<signal> s, long long f)
   {
      if(f == -1LL)
      {
         return this->create_not(this->get_constant(false));
      }
      kitty::dynamic_truth_table tt(static_cast<unsigned>(s.size()));
      std::stringstream resHex;
      resHex << std::hex << static_cast<unsigned long long>(f);
      std::string res0 = resHex.str();
      if(tt.num_vars() > 1)
      {
         while((res0.size() << 2) < tt.num_bits())
         {
            res0 = "0" + res0;
         }
      }
      while((res0.size() << 2) > tt.num_bits() && tt.num_vars() > 1)
      {
         res0 = res0.substr(1);
      }

      kitty::create_from_hex_string(tt, res0);
      return create_node(s, tt);
   }

#pragma endregion

#pragma region multi - bit operations

   std::vector<signal> create_buf_v(std::vector<signal> const& a)
   {
      return a;
   }

   std::vector<signal> create_not_v(std::vector<signal> const& a)
   {
      std::vector<signal> outputs;
      for(auto s : a)
      {
         outputs.push_back(this->create_not(s));
      }
      return outputs;
   }

   std::vector<signal> create_and_v(std::vector<signal> const& a, std::vector<signal> const& b, bool signedValues)
   {
      std::vector<signal> a_c(a), b_c(b);
      this->fix_inputs_size(&a_c, &b_c, signedValues);

      std::vector<signal> outputs;
      outputs.reserve(a_c.size());
      std::transform(a_c.begin(), a_c.end(), b_c.begin(), std::back_inserter(outputs),
                     [&](auto const& s1, auto const& s2) { return this->create_and(s1, s2); });

      return outputs;
   }

   std::vector<signal> create_or_v(std::vector<signal> const& a, std::vector<signal> const& b, bool signedValues)
   {
      std::vector<signal> a_c(a), b_c(b);
      this->fix_inputs_size(&a_c, &b_c, signedValues);

      std::vector<signal> outputs;
      outputs.reserve(a_c.size());
      std::transform(a_c.begin(), a_c.end(), b_c.begin(), std::back_inserter(outputs),
                     [&](auto const& s1, auto const& s2) { return this->create_or(s1, s2); });

      return outputs;
   }

   std::vector<signal> create_xor_v(std::vector<signal> const& a, std::vector<signal> const& b, bool signedValues)
   {
      std::vector<signal> a_c(a), b_c(b);
      this->fix_inputs_size(&a_c, &b_c, signedValues);

      std::vector<signal> outputs;
      outputs.reserve(a_c.size());
      std::transform(a_c.begin(), a_c.end(), b_c.begin(), std::back_inserter(outputs),
                     [&](auto const& s1, auto const& s2) { return this->create_xor(s1, s2); });

      return outputs;
   }

   std::vector<signal> create_lt_v(std::vector<signal> const& a, std::vector<signal> const& b, bool signedValues)
   {
      std::vector<signal> a_c(a), b_c(b);
      this->fix_inputs_size(&a_c, &b_c, signedValues);

      if(!signedValues)
      {
         a_c.push_back(this->get_constant(false));
         b_c.push_back(this->get_constant(false));
      }
      a_c.push_back(a_c.at(a_c.size() - 1));
      b_c.push_back(b_c.at(b_c.size() - 1));

      signal cbit = this->create_not(this->get_constant(false));
      signal neg1, result = this->get_constant(false);
      for(auto column = 0u; column < a_c.size(); ++column)
      {
         neg1 = this->create_not(b_c.at(column));
         std::tie(result, cbit) = mockturtle::full_adder(*this, a_c.at(column), neg1, cbit);
      }
      return {result};
   }

   std::vector<signal> create_ge_v(std::vector<signal> const& a, std::vector<signal> const& b, bool signedValues)
   {
      auto lt_res = this->create_lt_v(a, b, signedValues);
      return {this->create_not(lt_res.at(0))};
   }

   std::vector<signal> create_gt_v(std::vector<signal> const& a, std::vector<signal> const& b, bool signedValues)
   {
      return this->create_lt_v(b, a, signedValues);
   }

   std::vector<signal> create_le_v(std::vector<signal> const& a, std::vector<signal> const& b, bool signedValues)
   {
      return this->create_ge_v(b, a, signedValues);
   }

   std::vector<signal> create_eq_v(std::vector<signal> const& a, std::vector<signal> const& b, bool signedValues)
   {
      std::vector<signal> a_c(a), b_c(b);
      this->fix_inputs_size(&a_c, &b_c, signedValues);

      std::vector<signal> outputs;
      outputs.reserve(a_c.size());
      std::transform(a_c.begin(), a_c.end(), b_c.begin(), std::back_inserter(outputs),
                     [&](auto const& s1, auto const& s2) { return this->create_eq(s1, s2); });

      return {this->create_nary_and(outputs)};
   }

   std::vector<signal> create_ne_v(std::vector<signal> const& a, std::vector<signal> const& b, bool signedValues)
   {
      return this->create_not_v(this->create_eq_v(a, b, signedValues));
   }

#pragma endregion
};

/**
 * Helper structure that better represents a `mockturtle::klut_network`'s node.
 */
struct klut_network_node
{
   /// the index of the node
   uint64_t index;

   /// the lut constant
   long long lut_constant;

   /// a `std::vector` containing the indexes of all inputs of the current node
   std::vector<uint64_t> fan_in;

   /// whether the current node is a primary output
   bool is_po;

   /// in case the current node is a primary output, holds the index of the primary output
   uint64_t po_index;

   /// true in case the node is a constant value
   bool is_constant;
   explicit klut_network_node(uint64_t _index, long long _lut_constant, const std::vector<uint64_t>& _fan_in,
                              bool _is_po, uint64_t _po_index, bool _is_constant)
       : index(_index),
         lut_constant(_lut_constant),
         fan_in(_fan_in),
         is_po(_is_po),
         po_index(_po_index),
         is_constant(_is_constant)
   {
   }
};

/**
 * Pointer that points to a function of `klut_network_ext`, that represents a binary operation between two
 * `mockturtle::klut_network::signal`s and returns a `mockturtle::klut_network::signal`.
 */
using klut_network_fn = mockturtle::klut_network::signal (klut_network_ext::*)(const mockturtle::klut_network::signal,
                                                                               const mockturtle::klut_network::signal);

/**
 * Pointer that points to a function of `klut_network_ext`, that represents a binary operation between two
 * `std::vector<mockturtle::klut_network::signal>`s and returns a `std::vector<mockturtle::klut_network::signal>`.
 */
using klut_network_fn_v = std::vector<mockturtle::klut_network::signal> (klut_network_ext::*)(
    const std::vector<mockturtle::klut_network::signal>&, const std::vector<mockturtle::klut_network::signal>&, bool);

#pragma endregion

/**
 * Checks whether the provided node is a primary input of lut network.
 *
 * @param in a `tree_nodeRef`
 */
bool lut_transformation::CheckIfPI(tree_nodeRef in, unsigned int BB_index)
{
   auto ssa = GetPointer<ssa_name>(in);
   if(!ssa)
   {
      THROW_ERROR("expected as in a ssa variable");
   }
   tree_nodeRef def_stmt = ssa->CGetDefStmt();
   if(def_stmt->get_kind() != gimple_assign_K)
   {
      return true;
   }
   auto* gaDef = GetPointer<gimple_assign>(def_stmt);
   if(gaDef->bb_index != BB_index)
   {
      return true;
   }
   return cannotBeLUT(gaDef->op1);
}

/**
 * Checks if the provided basic block can be further processed. There are two cases in which this condition is true:
 *  - the basic block contains an instruction convertible to a `lut_expr_K`
 *  - the basic block contains a `lut_expr_K` that has constant inputs
 *
 * @param block the block to check
 * @return whether the provided basic block can be further processed
 */
bool lut_transformation::CheckIfProcessable(std::pair<unsigned int, blocRef> block)
{
   for(const auto& statement : block.second->CGetStmtList())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing CheckIfProcessable" + statement->ToString());
      // only gimple assignments are considered
      if(statement->get_kind() != gimple_assign_K)
      {
         continue;
      }

      auto* gimpleAssign = GetPointer<gimple_assign>(statement);
      enum kind code = gimpleAssign->op1->get_kind();

      if(code == lut_expr_K)
      { // check if it has constant inputs
         auto* lut = GetPointer<lut_expr>(gimpleAssign->op1);

         // cycle for each inputs (op0 is the constant)
         for(auto node : {lut->op1, lut->op2, lut->op3, lut->op4, lut->op5, lut->op6, lut->op7, lut->op8})
         {
            // if the node is null then there are no more inputs
            if(!node)
            {
               break;
            }

            // if the node can be converted into an `integer_cst` then the lut as constant inputs
            if(GetPointer<integer_cst>(node))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---CheckIfProcessable: lut with a constant input returns true");
               return true;
            }
         }
      }
      else if(not cannotBeLUT(gimpleAssign->op1))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---CheckIfProcessable: " + gimpleAssign->op1->get_kind_text() + " can be a LUT");
         return true;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---CheckIfProcessable: false");

   return false;
}

/**
 * Checks if the provided `gimple_assign` is a primary output of lut network.
 *
 * @param gimpleAssign the `gimple_assign` to check
 * @return whether the provided `gimple_assign` is a primary output
 */
bool lut_transformation::CheckIfPO(gimple_assign* gimpleAssign)
{
   /// the index of the basic block holding the provided `gimpleAssign`
   const unsigned int currentBBIndex = gimpleAssign->bb_index;
   // the variables that uses the result of the provided `gimpleAssign`
   const auto op0 = gimpleAssign->op0;
   auto ssa0 = GetPointer<ssa_name>(op0);
   THROW_ASSERT(ssa0, "unexpected condition");
   const auto usedIn = ssa0->CGetUseStmts();

   for(auto node : usedIn)
   {
      auto* childGimpleNode = GetPointer<gimple_node>(node.first);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing use " + childGimpleNode->ToString());
      THROW_ASSERT(childGimpleNode, "unexpected condition");

      // the current operation is a primary output if it is used in
      // operation not belonging to the current basic block or if the operation
      // in which it is used does cannot be a LUT
      if(childGimpleNode->bb_index != currentBBIndex)
      {
         return true;
      }
      else
      {
         auto* childGimpleAssign = GetPointer<gimple_assign>(node.first);
         if(!childGimpleAssign)
         {
            return true;
         }
         if(cannotBeLUT(childGimpleAssign->op1))
         {
            return true;
         }
      }
   }
   return false;
}

static klut_network_fn GetBooleanNodeCreationFunction(enum kind code)
{
   switch(code)
   {
      case bit_and_expr_K:
      case truth_and_expr_K:
         return &klut_network_ext::create_and;
      case bit_ior_expr_K:
      case truth_or_expr_K:
      case truth_orif_expr_K:
         return &klut_network_ext::create_or;
      case bit_xor_expr_K:
      case truth_xor_expr_K:
         return &klut_network_ext::create_xor;
      case eq_expr_K:
         return &klut_network_ext::create_eq;
      case ge_expr_K:
         return &klut_network_ext::create_ge;
      case gt_expr_K:
         return &klut_network_ext::create_gt;
      case le_expr_K:
         return &klut_network_ext::create_le;
      case lt_expr_K:
         return &klut_network_ext::create_lt;
      case ne_expr_K:
         return &klut_network_ext::create_ne;
      default:
         return nullptr;
   }
}

static std::vector<bool> IntegerToBitArray(integer_cst_t n, size_t size)
{
   const auto oldbits = [&]() {
      std::vector<bool> bits;
      for(size_t i = 0; i < size; ++i)
      {
         bits.push_back((static_cast<unsigned long long int>(n) & (1ULL << i)) ? true : false);
      }
      return bits;
   }();
   std::vector<bool> bits;
   for(size_t i = 0; i < size; ++i)
   {
      bits.push_back((n & (integer_cst_t(1) << i)) ? true : false);
   }
   THROW_ASSERT(bits == oldbits, "Bits for " + STR(n) + " was " + container_to_string(oldbits, "") + " now is " +
                                     container_to_string(bits, ""));
   return bits;
}

tree_nodeRef lut_transformation::CreateBitSelectionNodeOrCast(const tree_nodeRef source, int index,
                                                              std::vector<tree_nodeRef>& prev_stmts_to_add)
{
   const auto indexType = tree_man->GetUnsignedLongLongType();
   tree_nodeRef bit_pos_constant = TM->CreateUniqueIntegerCst(index, indexType);
   const std::string srcp_default("built-in:0:0");
   tree_nodeRef eb_op = tree_man->create_extract_bit_expr(source, bit_pos_constant, srcp_default);
   auto boolType = tree_man->GetBooleanType();
   tree_nodeRef eb_ga =
       tree_man->CreateGimpleAssign(boolType, TM->CreateUniqueIntegerCst(0, boolType),
                                    TM->CreateUniqueIntegerCst(1, boolType), eb_op, function_id, srcp_default);
   prev_stmts_to_add.push_back(eb_ga);
   return GetPointer<const gimple_assign>(eb_ga)->op0;
}

static klut_network_fn_v GetIntegerNodeCreationFunction(enum kind code)
{
   switch(code)
   {
      case eq_expr_K:
         return &klut_network_ext::create_eq_v;
      case ne_expr_K:
         return &klut_network_ext::create_ne_v;
      case le_expr_K:
         return &klut_network_ext::create_le_v;
      case lt_expr_K:
         return &klut_network_ext::create_lt_v;
      case ge_expr_K:
         return &klut_network_ext::create_ge_v;
      case gt_expr_K:
         return &klut_network_ext::create_gt_v;
      default:
         return nullptr;
   }
}

#ifndef NDEBUG
static std::string ConvertBitsToString(const std::vector<bool>& bits, const std::string& true_string = "vdd",
                                       const std::string& false_string = "gnd", const std::string& sep = ", ")
{
   std::string s;

   for(size_t i = 0; i < bits.size(); ++i)
   {
      if(i != 0)
      {
         s += sep;
      }
      s += bits[i] ? true_string : false_string;
   }

   return s;
}
#endif

template <typename T>
static T ConvertHexToNumber(const std::string& hex0)
{
   uint64_t x;
   std::stringstream ss;
   ss << std::hex << hex0;
   ss >> x;

   return T(x);
}

static void ParseKLutNetwork(const mockturtle::klut_network& klut, std::vector<klut_network_node>& luts)
{
   std::map<mockturtle::klut_network::node, CustomOrderedSet<unsigned>> po_set;

   mockturtle::topo_view ntk_topo{klut};

   ntk_topo.foreach_po([&](auto const& s, auto i) { po_set[s].insert(i); });

   ntk_topo.foreach_node([&](const auto& node) {
      if(ntk_topo.is_pi(node) || ntk_topo.is_constant(node))
      {
         return; // continue
      }
      auto func = ntk_topo.node_function(node);

      std::vector<uint64_t> fanIns;
      ntk_topo.foreach_fanin(node, [&](auto const& fanin_node, auto) { fanIns.push_back(fanin_node); });
      auto LUT_func = ConvertHexToNumber<long long>(kitty::to_hex(func));
      auto is_zero = LUT_func == 0;
      if(!is_zero)
      {
         if(po_set.find(node) != po_set.end())
         {
            for(auto po_i : po_set.find(node)->second)
            {
               klut_network_node lut_node(node, LUT_func, fanIns, true, po_i, false);
               luts.push_back(lut_node);
            }
         }
         else
         {
            klut_network_node lut_node(node, LUT_func, fanIns, false, 0, false);
            luts.push_back(lut_node);
         }
      }
   });

   ntk_topo.foreach_po([&](auto const& s, auto i) {
      if(ntk_topo.is_constant(ntk_topo.get_node(s)))
      {
         std::vector<uint64_t> fanIns;
         klut_network_node lut_node(
             s, static_cast<long long>(ntk_topo.constant_value(ntk_topo.get_node(s)) ^ ntk_topo.is_complemented(s)),
             fanIns, true, i, true);
         luts.push_back(lut_node);
      }
      else
      {
         auto func = ntk_topo.node_function(s);
         auto LUT_func = ConvertHexToNumber<uint64_t>(kitty::to_hex(func));
         auto is_zero = LUT_func == 0;
         if(is_zero)
         {
            std::vector<uint64_t> fanIns;
            klut_network_node lut_node(
                s, static_cast<long long>(ntk_topo.constant_value(ntk_topo.get_node(s)) ^ ntk_topo.is_complemented(s)),
                fanIns, true, i, true);
            luts.push_back(lut_node);
         }
         else if(ntk_topo.is_pi(ntk_topo.get_node(s)))
         {
            std::vector<uint64_t> fanIns;
            fanIns.push_back(s);
            klut_network_node lut_node(s, static_cast<long long>(ntk_topo.is_complemented(s) ? 1 : 2), fanIns, true, i,
                                       false);
            luts.push_back(lut_node);
         }
      }
   });
}

template <class kne>
static mockturtle::klut_network SimplifyLutNetwork(const kne& klut_e, size_t max_lut_size)
{
/// scripts taken from https://github.com/lnis-uofu/LSOracle
#if MIG_SYNTHESIS
   mockturtle::shannon_resynthesis<mockturtle::mig_network> fallback;
   mockturtle::dsd_resynthesis<mockturtle::mig_network, decltype(fallback)> mig_resyn(fallback);
   auto mig0 = mockturtle::node_resynthesis<mockturtle::mig_network>(klut_e, mig_resyn);
   auto resyn2 = [&](mockturtle::mig_network& mig) -> mockturtle::mig_network {
      mockturtle::depth_view mig_depth{mig};

      mockturtle::mig_algebraic_depth_rewriting_params pm;
      // pm.strategy = mockturtle::mig_algebraic_depth_rewriting_params::selective;

      // std::cout << "1st round depth optimization " << std::endl;

      mockturtle::mig_algebraic_depth_rewriting(mig_depth, pm);

      mig = mockturtle::cleanup_dangling(mig);

      // std::cout << "1st round area recovering " << std::endl;

      // AREA RECOVERING
      mockturtle::mig_npn_resynthesis resyn;
      mockturtle::cut_rewriting_params ps;

      ps.cut_enumeration_ps.cut_size = 4;

      mockturtle::cut_rewriting(mig, resyn, ps);
      mig = mockturtle::cleanup_dangling(mig);

      // std::cout << "2nd round area recovering " << std::endl;

      // AREA RECOVERING
      mockturtle::cut_rewriting(mig, resyn, ps);
      mig = mockturtle::cleanup_dangling(mig);

      // std::cout << "2nd round depth optimization" << std::endl;

      // DEPTH REWRITING
      mockturtle::depth_view mig_depth1{mig};

      mockturtle::mig_algebraic_depth_rewriting(mig_depth1, pm);
      mig = mockturtle::cleanup_dangling(mig);

      // std::cout << "3rd round area recovering" << std::endl;

      // AREA RECOVERING
      mockturtle::cut_rewriting(mig, resyn, ps);
      mig = mockturtle::cleanup_dangling(mig);

      // std::cout << "4th round area recovering" << std::endl;

      // AREA RECOVERING
      mockturtle::cut_rewriting(mig, resyn, ps);
      mig = mockturtle::cleanup_dangling(mig);

      // std::cout << "3rd round depth optimization" << std::endl;

      // DEPTH REWRITING
      mockturtle::depth_view mig_depth2{mig};

      mockturtle::mig_algebraic_depth_rewriting(mig_depth2, pm);
      mig = mockturtle::cleanup_dangling(mig);

      // std::cout << "5th round area recovering" << std::endl;

      // AREA RECOVERING
      mockturtle::cut_rewriting(mig, resyn, ps);
      mig = mockturtle::cleanup_dangling(mig);

      // std::cout << "6th round area recovering" << std::endl;

      // AREA RECOVERING
      mockturtle::cut_rewriting(mig, resyn, ps);
      mig = mockturtle::cleanup_dangling(mig);

      // std::cout << "Final depth optimization" << std::endl;

      // DEPTH REWRITING
      mockturtle::depth_view mig_depth3{mig};

      // std::cout << "Network Optimized" << std::endl;

      mockturtle::mig_algebraic_depth_rewriting(mig_depth3, pm);
      mig = mockturtle::cleanup_dangling(mig);

      // std::cout << "Majority nodes " << mig.num_gates() << " MIG depth " << mig_depth3.depth() << std::endl;

      return mig;
   };
   resyn2(mig0);
   auto cleanedUp = mockturtle::cleanup_dangling(mig0);

   mockturtle::mapping_view<mockturtle::mig_network, true> mapped_klut{cleanedUp};
   std::cerr << "std\n";
   mockturtle::write_bench(mapped_klut, std::cout);
   std::cerr << "===============\n";

   mockturtle::lut_mapping_params mp;
   mp.cut_enumeration_ps.cut_size = static_cast<uint32_t>(max_lut_size);
   mp.cut_enumeration_ps.cut_limit = 16;

#ifndef NDEBUG
   mp.verbose = false;
   mp.cut_enumeration_ps.very_verbose = false;
#endif

   mockturtle::lut_mapping<decltype(mapped_klut), true>(mapped_klut, mp);
   std::cerr << "lut\n";
   std::cerr << "===============\n";
   mockturtle::write_bench(mapped_klut, std::cout);

   auto collapsed = *mockturtle::collapse_mapped_network<mockturtle::klut_network>(mapped_klut);
   collapsed = mockturtle::cleanup_luts(collapsed);
   std::cerr << "res\n";
   mockturtle::write_bench(collapsed, std::cout);
   std::cerr << "===============\n";

#else
   mockturtle::shannon_resynthesis<mockturtle::aig_network> fallback;
   mockturtle::dsd_resynthesis<mockturtle::aig_network, decltype(fallback)> aig_resyn(fallback);
   auto aig = mockturtle::node_resynthesis<mockturtle::aig_network>(klut_e, aig_resyn);

   auto cleanedUp = cleanup_dangling(aig);
   mockturtle::mapping_view<mockturtle::aig_network, true> mapped_klut{cleanedUp};
   //   std::cerr << "std\n";
   //   mockturtle::write_bench(mapped_klut, std::cout);
   //   std::cerr << "===============\n";

   mockturtle::lut_mapping_params mp;
   mp.cut_enumeration_ps.cut_size = static_cast<uint32_t>(max_lut_size);
   mp.cut_enumeration_ps.cut_limit = 16;

#ifndef NDEBUG
   mp.verbose = false;
   mp.cut_enumeration_ps.very_verbose = false;
#endif

   mockturtle::lut_mapping<decltype(mapped_klut), true>(mapped_klut, mp);
   //   std::cerr << "lut\n";
   //   std::cerr << "===============\n";
   //   mockturtle::write_bench(mapped_klut, std::cout);

#if USE_SAT
   mockturtle::satlut_mapping_params satlut_mp;
   satlut_mp.cut_enumeration_ps.cut_size = max_lut_size;
   satlut_mp.cut_enumeration_ps.cut_limit = 16;
   satlut_mp.conflict_limit = 100;
   satlut_mp.progress = true;
   mockturtle::satlut_mapping_stats st;

   mockturtle::satlut_mapping<decltype(mapped_klut), true>(mapped_klut, 32, satlut_mp, &st);
   std::cerr << "sat\n";
   std::cerr << "===============\n";
   mockturtle::write_bench(mapped_klut, std::cout);
#endif
   auto collapsed = *mockturtle::collapse_mapped_network<mockturtle::klut_network>(mapped_klut);
   collapsed = mockturtle::cleanup_luts(collapsed);
//   std::cerr << "res\n";
//   mockturtle::write_bench(collapsed, std::cout);
//   std::cerr << "===============\n";
#endif
   return collapsed;
}

bool lut_transformation::ProcessBasicBlock(std::pair<unsigned int, blocRef> block)
{
   klut_network_ext klut_e;
   auto BB_index = block.first;

   std::map<unsigned int, mockturtle::klut_network::signal> nodeRefToSignal;
   std::map<unsigned int, std::vector<mockturtle::klut_network::signal>> nodeRefToSignalBus;

   std::vector<tree_nodeRef> pis;
   std::vector<int> pis_offset;
   std::vector<tree_nodeRef> pos;
   std::vector<unsigned> pos_offset;

   auto DefaultUnsignedLongLongInt = this->tree_man->GetUnsignedLongLongType();

   /**
    * Creates a const expression with 0 (gnd) as value, used for constant LUT inputs (index 0 in mockturtle)
    */
   pis.push_back(TM->CreateUniqueIntegerCst(0, DefaultUnsignedLongLongInt));
   pis_offset.push_back(0);

   /**
    * Creates a const expression with 1 (vdd) as value, used for constant LUT inputs (index 1 in mockturtle)
    */
   pis.push_back(TM->CreateUniqueIntegerCst(1, DefaultUnsignedLongLongInt));
   pis_offset.push_back(0);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing BB" + STR(BB_index));
   const auto& statements = block.second->CGetStmtList();

   /// whether the BB has been modified
   bool modified = false;

   for(const auto& statement : statements)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + statement->ToString());

      if(!AppM->ApplyNewTransformation())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Reached max cfg transformations");
         continue;
      }

      if(statement->get_kind() != gimple_assign_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not a gimple assign");
         continue;
      }

      auto* gimpleAssign = GetPointer<gimple_assign>(statement);
      enum kind code1 = gimpleAssign->op1->get_kind();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing code " + gimpleAssign->op1->get_kind_text());

      if(code1 == lut_expr_K)
      {
         auto* le = GetPointer<lut_expr>(gimpleAssign->op1);

         std::vector<mockturtle::klut_network::signal> ops;
         for(auto op : {le->op1, le->op2, le->op3, le->op4, le->op5, le->op6, le->op7, le->op8})
         {
            if(!op)
            {
               break;
            }

            // if the first operand has already been processed then the previous signal is used
            if(nodeRefToSignal.find(op->index) != nodeRefToSignal.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---used PI " + op->ToString());
               ops.push_back(nodeRefToSignal[op->index]);
            }
            else
            { // otherwise the operand is a primary input
               mockturtle::klut_network::signal kop = 0;

               if(op->get_kind() == integer_cst_K)
               {
                  const auto cst_val = tree_helper::GetConstValue(op);
                  kop = cst_val == 0 ? klut_e.get_constant(false) : klut_e.create_not(klut_e.get_constant(false));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, cst_val == 0 ? "---used gnd" : "---used vdd");
                  modified = true;
               }
               else if(CheckIfPI(op, BB_index))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---used PI " + op->ToString());
                  kop = klut_e.create_pi();
                  pis.push_back(op);
                  pis_offset.push_back(0);
               }
               else
               {
                  THROW_ERROR("unexpected condition: " + op->ToString());
               }

               nodeRefToSignal[op->index] = kop;
               ops.push_back(kop);
            }
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---translating in klut " + STR(tree_helper::GetConstValue(le->op0)));
         const auto cst_val = tree_helper::GetConstValue(le->op0);
         THROW_ASSERT(tree_helper::GetConstValue(le->op0, false) <= std::numeric_limits<unsigned long long>::max(),
                      "Cast will change signedness of current value: " + STR(cst_val));
         auto res = klut_e.create_lut(ops, static_cast<long long>(cst_val));
         nodeRefToSignal[gimpleAssign->op0->index] = res;

         if(this->CheckIfPO(gimpleAssign))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---is PO");
            klut_e.create_po(res);
            pos.push_back(statement);
            pos_offset.push_back(0);
         }

         // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---====");
         // mockturtle::write_bench(klut_e, std::cout);
         // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---====");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--LUT found");
         continue;
      }

      if(code1 == truth_not_expr_K || code1 == bit_not_expr_K)
      {
         auto* ne = GetPointer<unary_expr>(gimpleAssign->op1);
         auto is_size_one = CHECK_NOT_EXPR_SIZE(ne);
         if(!is_size_one)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not a Boolean not expr");
            continue;
         }
         std::vector<mockturtle::klut_network::signal> ops;
         for(auto op : {ne->op})
         {
            // if the first operand has already been processed then the previous signal is used
            if(nodeRefToSignal.find(op->index) != nodeRefToSignal.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---used PI " + op->ToString());
               ops.push_back(nodeRefToSignal[op->index]);
            }
            else
            { // otherwise the operand is a primary input
               mockturtle::klut_network::signal kop = 0;

               if(op->get_kind() == integer_cst_K)
               {
                  const auto cst_val = tree_helper::GetConstValue(op);
                  kop = cst_val == 0 ? klut_e.get_constant(false) : klut_e.create_not(klut_e.get_constant(false));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, cst_val == 0 ? "---used gnd" : "---used vdd");
               }
               else if(CheckIfPI(op, BB_index))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---used PI " + op->ToString());
                  kop = klut_e.create_pi();
                  pis.push_back(op);
                  pis_offset.push_back(0);
               }
               else
               {
                  THROW_ERROR("unexpected condition");
               }

               nodeRefToSignal[op->index] = kop;
               ops.push_back(kop);
            }
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---translating in klut");
         auto res = klut_e.create_not(ops.at(0));
         nodeRefToSignal[gimpleAssign->op0->index] = res;

         if(this->CheckIfPO(gimpleAssign))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---is PO");
            klut_e.create_po(res);
            pos.push_back(statement);
            pos_offset.push_back(0);
         }

         // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---====");
         // mockturtle::write_bench(klut_e, std::cout);
         // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---====");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--truth_not_expr/bit_not_expr found");

         modified = true;
         continue;
      }

      if(code1 == cond_expr_K)
      {
         auto* ce = GetPointer<cond_expr>(gimpleAssign->op1);
         auto is_size_one = CHECK_COND_EXPR_SIZE(ce);
         if(!is_size_one)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not a Boolean cond_expr");
            continue;
         }
         std::vector<mockturtle::klut_network::signal> ops;
         for(auto op : {ce->op0, ce->op1, ce->op2})
         {
            // if the first operand has already been processed then the previous signal is used
            if(nodeRefToSignal.find(op->index) != nodeRefToSignal.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---used PI " + op->ToString());
               ops.push_back(nodeRefToSignal[op->index]);
            }
            else
            { // otherwise the operand is a primary input
               mockturtle::klut_network::signal kop = 0;

               if(op->get_kind() == integer_cst_K)
               {
                  const auto cst_val = tree_helper::GetConstValue(op);
                  kop = cst_val == 0 ? klut_e.get_constant(false) : klut_e.create_not(klut_e.get_constant(false));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, cst_val == 0 ? "---used gnd" : "---used vdd");
               }
               else if(CheckIfPI(op, BB_index))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---used PI " + op->ToString());
                  kop = klut_e.create_pi();
                  pis.push_back(op);
                  pis_offset.push_back(0);
               }
               else
               {
                  THROW_ERROR("unexpected condition: " + op->ToString());
               }

               nodeRefToSignal[op->index] = kop;
               ops.push_back(kop);
            }
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---translating in klut");
         auto res = klut_e.create_ite(ops.at(0), ops.at(1), ops.at(2));
         nodeRefToSignal[gimpleAssign->op0->index] = res;

         if(this->CheckIfPO(gimpleAssign))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---is PO");
            klut_e.create_po(res);
            pos.push_back(statement);
            pos_offset.push_back(0);
         }

         // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---====");
         // mockturtle::write_bench(klut_e, std::cout);
         // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---====");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--cond_expr found");

         modified = true;
         continue;
      }

      auto* binaryExpression = GetPointer<binary_expr>(gimpleAssign->op1);
      if(!binaryExpression)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not a binary expression");
         continue;
      }

      if(VECT_CONTAINS(lutBooleanExpressibleOperations, code1) && CHECK_BIN_EXPR_BOOL_SIZE(binaryExpression))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Boolean operands");

         klut_network_fn nodeCreateFn = GetBooleanNodeCreationFunction(code1);

         if(nodeCreateFn == nullptr)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not supported expression");
            continue;
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---translating in klut");

         mockturtle::klut_network::signal res;
         mockturtle::klut_network::signal op1 = 0;
         mockturtle::klut_network::signal op2 = 0;

         // if the first operand has already been processed then the previous signal is used
         if(nodeRefToSignal.find(binaryExpression->op0->index) != nodeRefToSignal.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---used PI " + binaryExpression->op0->ToString());
            op1 = nodeRefToSignal[binaryExpression->op0->index];
         }
         else
         { // otherwise the operand is a primary input
            if(binaryExpression->op0->get_kind() == integer_cst_K)
            {
               const auto cst_val = tree_helper::GetConstValue(binaryExpression->op0);
               op1 = cst_val == 0 ? klut_e.get_constant(false) : klut_e.create_not(klut_e.get_constant(false));
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, (cst_val == 0) ? "---used gnd" : "---used vdd");
            }
            else if(CheckIfPI(binaryExpression->op0, BB_index))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---used PI " + binaryExpression->op0->ToString());
               op1 = klut_e.create_pi();
               pis.push_back(binaryExpression->op0);
               pis_offset.push_back(0);
               nodeRefToSignal[binaryExpression->op0->index] = op1;
            }
            else
            {
               THROW_ERROR("unexpected condition");
            }
         }

         // if the second operand has already been processed then the previous signal is used
         if(nodeRefToSignal.find(binaryExpression->op1->index) != nodeRefToSignal.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---used PI " + binaryExpression->op1->ToString());
            op2 = nodeRefToSignal[binaryExpression->op1->index];
         }
         else
         { // otherwise the operand is a primary input
            if(binaryExpression->op1->get_kind() == integer_cst_K)
            {
               const auto cst_val = tree_helper::GetConstValue(binaryExpression->op1);
               op2 = cst_val == 0 ? klut_e.get_constant(false) : klut_e.create_not(klut_e.get_constant(false));
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, (cst_val == 0) ? "---used gnd" : "---used vdd");
            }
            else if(CheckIfPI(binaryExpression->op1, BB_index))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---used PI " + binaryExpression->op1->ToString());
               op2 = klut_e.create_pi();
               pis.push_back(binaryExpression->op1);
               pis_offset.push_back(0);
               nodeRefToSignal[binaryExpression->op1->index] = op2;
            }
            else
            {
               THROW_ERROR("unexpected condition");
            }
         }

         res = (klut_e.*nodeCreateFn)(op1, op2);
         nodeRefToSignal[gimpleAssign->op0->index] = res;

         if(this->CheckIfPO(gimpleAssign))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---is PO");
            klut_e.create_po(res);
            pos.push_back(statement);
            pos_offset.push_back(0);
         }

         // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---====");
         // mockturtle::write_bench(klut_e, std::cout);
         // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---====");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed statement ");
         modified = true;
         continue;
      }
      if(VECT_CONTAINS(lutIntegerExpressibleOperations, code1) &&
         CHECK_BIN_EXPR_INT_SIZE(binaryExpression, parameters->GetParameter<unsigned int>("MAX_LUT_INT_SIZE")))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Integer operands");

         klut_network_fn_v nodeCreateFn = GetIntegerNodeCreationFunction(code1);

         if(nodeCreateFn == nullptr)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not supported expression");
            continue;
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---translating in klut");

         std::vector<mockturtle::klut_network::signal> res;
         std::vector<mockturtle::klut_network::signal> op1 = {};
         std::vector<mockturtle::klut_network::signal> op2 = {};

         // if the first operand has already been processed then the previous signal is used
         if(nodeRefToSignalBus.find(binaryExpression->op0->index) != nodeRefToSignalBus.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---used PI " + binaryExpression->op0->ToString());
            op1 = nodeRefToSignalBus[binaryExpression->op0->index];
         }
         else
         { // otherwise the operand is a primary input
            if(binaryExpression->op0->get_kind() == integer_cst_K)
            {
               const auto cst_val = tree_helper::GetConstValue(binaryExpression->op0);
               auto bits = IntegerToBitArray(cst_val, tree_helper::Size(binaryExpression->op0));

               op1 = klut_e.get_constant_v(bits);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---used {" + ConvertBitsToString(bits) + "}");
            }
            else if(CheckIfPI(binaryExpression->op0, BB_index))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---used PIs " + binaryExpression->op0->ToString());
               op1 = klut_e.create_pi_v(tree_helper::Size(binaryExpression->op0));

               int index = 0;
               std::for_each(op1.begin(), op1.end(), [&binaryExpression, &pis, &pis_offset, &index](auto /*op*/) {
                  pis.push_back(binaryExpression->op0);
                  pis_offset.push_back(index);
                  ++index;
               });

               nodeRefToSignalBus[binaryExpression->op0->index] = op1;
            }
            else
            {
               THROW_ERROR("unexpected condition");
            }
         }

         // if the second operand has already been processed then the previous signal is used
         if(nodeRefToSignalBus.find(binaryExpression->op1->index) != nodeRefToSignalBus.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---used PI " + binaryExpression->op1->ToString());
            op2 = nodeRefToSignalBus[binaryExpression->op1->index];
         }
         else
         { // otherwise the operand is a primary input
            if(binaryExpression->op1->get_kind() == integer_cst_K)
            {
               const auto cst_val = tree_helper::GetConstValue(binaryExpression->op1);
               auto bits = IntegerToBitArray(cst_val, tree_helper::Size(binaryExpression->op1));

               op2 = klut_e.get_constant_v(bits);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---used {" + ConvertBitsToString(bits) + "}");
            }
            else if(CheckIfPI(binaryExpression->op1, BB_index))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---used PIs " + binaryExpression->op1->ToString());
               op2 = klut_e.create_pi_v(tree_helper::Size(binaryExpression->op1));

               int index = 0;
               std::for_each(op2.begin(), op2.end(), [&binaryExpression, &pis, &pis_offset, &index](auto /*op*/) {
                  pis.push_back(binaryExpression->op1);
                  pis_offset.push_back(index);
                  ++index;
               });

               nodeRefToSignalBus[binaryExpression->op1->index] = op2;
            }
            else
            {
               THROW_ERROR("unexpected condition");
            }
         }
         bool isSigned = tree_helper::is_int(TM, binaryExpression->op0->index);
         res = (klut_e.*nodeCreateFn)(op1, op2, isSigned);
         nodeRefToSignalBus[gimpleAssign->op0->index] = res;
         if(res.size() == 1)
         {
            nodeRefToSignal[gimpleAssign->op0->index] = res.at(0);
         }
         if(this->CheckIfPO(gimpleAssign))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---is PO");
            klut_e.create_po_v(res);

            unsigned int index = 0;
            std::for_each(res.begin(), res.end(), [&statement, &pos, &pos_offset, &index](auto /*op*/) {
               pos.push_back(statement);
               pos_offset.push_back(index);
               ++index;
            });
         }

         // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---====");
         // mockturtle::write_bench(klut_e, std::cout);
         // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---====");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed statement ");
         modified = true;
         continue;
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Integer operands' size is too large");
         continue;
      }
   }

   if(modified)
   {
      // mockturtle::write_bench(klut_e, std::cout);
      // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---====");
      mockturtle::klut_network klut = SimplifyLutNetwork(klut_e, this->max_lut_size);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---PI size " + STR(pis.size()));
#ifndef NDEBUG
      if(DEBUG_LEVEL_VERY_PEDANTIC <= debug_level)
         mockturtle::write_bench(klut, std::cout);
#endif

      std::vector<klut_network_node> luts;
      ParseKLutNetwork(klut, luts);

      std::map<mockturtle::klut_network::node, tree_nodeRef> internal_nets;
      std::vector<tree_nodeRef> prev_stmts_to_add;
      for(auto lut : luts)
      {
         if(!AppM->ApplyNewTransformation())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Reached max cfg transformations");
            continue;
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---index: " + STR(lut.index));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- po_index: " + STR(lut.po_index));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- func: " + STR(lut.lut_constant));
#ifndef NDEBUG
         for(auto in : lut.fan_in)
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---   in " + STR(in));
#endif
         if(lut.is_po)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Is PO");
            auto po_stmpt = pos.at(lut.po_index);
            /// add previous statements defining non-primary outputs just before the current statement
            for(auto stmt : prev_stmts_to_add)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding statement " + stmt->ToString());
               block.second->PushBefore(stmt, po_stmpt, AppM);
            }
            prev_stmts_to_add.clear();
         }
         tree_nodeRef lut_constant_node = TM->CreateUniqueIntegerCst(lut.lut_constant, DefaultUnsignedLongLongInt);
         tree_nodeRef op1, op2, op3, op4, op5, op6, op7, op8;
         auto p_index = 1u;
         for(auto in : lut.fan_in)
         {
            tree_nodeRef operand;
            if(klut.is_pi(in))
            {
               operand = pis.at(in);
               auto operand_offset = pis_offset.at(in);

               if(tree_helper::Size(operand) == 1 && !tree_helper::IsBooleanType(operand))
               {
                  THROW_ASSERT(operand_offset == 0, "unexpected condition");
                  operand = CreateBitSelectionNodeOrCast(operand, 0, prev_stmts_to_add);
               }
               else if(tree_helper::Size(operand) > 1)
               {
                  operand = CreateBitSelectionNodeOrCast(operand, operand_offset, prev_stmts_to_add);
               }
            }
            else if(internal_nets.find(in) != internal_nets.end())
            {
               operand = internal_nets.find(in)->second;
            }
            else
            {
               THROW_ERROR("unexpected condition" + STR(p_index));
            }

            if(p_index == 1)
            {
               op1 = operand;
            }
            else if(p_index == 2)
            {
               op2 = operand;
            }
            else if(p_index == 3)
            {
               op3 = operand;
            }
            else if(p_index == 4)
            {
               op4 = operand;
            }
            else if(p_index == 5)
            {
               op5 = operand;
            }
            else if(p_index == 6)
            {
               op6 = operand;
            }
            else if(p_index == 7)
            {
               op7 = operand;
            }
            else if(p_index == 8)
            {
               op8 = operand;
            }
            else
            {
               THROW_ERROR("unexpected number of inputs");
            }
            ++p_index;
         }

         if(lut.is_po)
         {
            auto po_stmpt = pos.at(lut.po_index);
            // auto po_offset = pos_offset.at(lut.po_index);
            /// add selection bit stmts
            for(auto stmt : prev_stmts_to_add)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding statement " + stmt->ToString());
               block.second->PushBefore(stmt, po_stmpt, AppM);
            }
            prev_stmts_to_add.clear();
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Before statement " + po_stmpt->ToString());
            auto* gimpleAssign = GetPointer<gimple_assign>(po_stmpt);
            THROW_ASSERT(gimpleAssign, "unexpected condition");
            const std::string srcp_default = gimpleAssign->include_name + ":" + STR(gimpleAssign->line_number) + ":" +
                                             STR(gimpleAssign->column_number);
            auto ga_op0 = gimpleAssign->op0;
            auto* ssa_ga_op0 = GetPointer<ssa_name>(ga_op0);
            THROW_ASSERT(ssa_ga_op0, "unexpected condition");
            if(!lut.is_constant)
            {
               internal_nets[lut.index] = gimpleAssign->op0;
            }

            if(lut.is_constant)
            {
               const auto new_op1 = TM->CreateUniqueIntegerCst(lut.lut_constant, ssa_ga_op0->type);
               TM->ReplaceTreeNode(po_stmpt, gimpleAssign->op1, new_op1);
            }
            else if(lut.fan_in.size() == 1 && lut.lut_constant == 2)
            {
               const auto op1_type_node = tree_helper::CGetType(op1);
               if(ssa_ga_op0->type->index == op1_type_node->index)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Replacing " + STR(gimpleAssign->op1) + " with " + STR(op1));
                  TM->ReplaceTreeNode(po_stmpt, gimpleAssign->op1, op1);
               }
               else
               {
                  const auto new_op1 =
                      tree_man->create_unary_operation(ssa_ga_op0->type, op1, srcp_default, nop_expr_K);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Replacing " + STR(gimpleAssign->op1) + " with " + STR(new_op1));
                  TM->ReplaceTreeNode(po_stmpt, gimpleAssign->op1, new_op1);
               }
            }
            else
            {
               auto boolType = tree_man->GetBooleanType();
               /// check if operands are of bool type
               auto check_lut_compatibility = [&](tree_nodeRef& lut_operand) {
                  if(lut_operand && !tree_helper::IsBooleanType(lut_operand))
                  {
                     tree_nodeRef ga_nop =
                         tree_man->CreateNopExpr(lut_operand, boolType, tree_nodeRef(), tree_nodeRef(), function_id);
                     block.second->PushBefore(ga_nop, po_stmpt, AppM);
                     lut_operand = GetPointer<gimple_assign>(ga_nop)->op0;
                  }
               };
               check_lut_compatibility(op1);
               check_lut_compatibility(op2);
               check_lut_compatibility(op3);
               check_lut_compatibility(op4);
               check_lut_compatibility(op5);
               check_lut_compatibility(op6);
               check_lut_compatibility(op7);
               check_lut_compatibility(op8);
               if(tree_helper::IsBooleanType(gimpleAssign->op0))
               {
                  tree_nodeRef new_op1 = tree_man->create_lut_expr(ssa_ga_op0->type, lut_constant_node, op1, op2, op3,
                                                                   op4, op5, op6, op7, op8, srcp_default);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Replacing " + STR(gimpleAssign->op1) + " with " + STR(op1));
                  TM->ReplaceTreeNode(po_stmpt, gimpleAssign->op1, new_op1);
               }
               else
               {
                  tree_nodeRef lut_node = tree_man->create_lut_expr(boolType, lut_constant_node, op1, op2, op3, op4,
                                                                    op5, op6, op7, op8, srcp_default);
                  auto lut_ga = tree_man->CreateGimpleAssign(boolType, TM->CreateUniqueIntegerCst(0, boolType),
                                                             TM->CreateUniqueIntegerCst(1, boolType), lut_node,
                                                             function_id, srcp_default);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding statement " + lut_ga->ToString());
                  block.second->PushBefore(lut_ga, po_stmpt, AppM);
                  auto ssa_vd = GetPointer<gimple_assign>(lut_ga)->op0;
                  tree_nodeRef new_op1 =
                      tree_man->create_unary_operation(ssa_ga_op0->type, ssa_vd, srcp_default, nop_expr_K);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Replacing " + STR(gimpleAssign->op1) + " with " + STR(op1));
                  TM->ReplaceTreeNode(po_stmpt, gimpleAssign->op1, new_op1);
               }
            }
            AppM->RegisterTransformation(GetName(), po_stmpt);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Modified statement " + po_stmpt->ToString());
         }
         else
         {
            auto boolType = tree_man->GetBooleanType();
            /// check if operands are of bool type
            auto check_lut_compatibility = [&](tree_nodeRef& lut_operand) {
               if(lut_operand && !tree_helper::IsBooleanType(lut_operand))
               {
                  tree_nodeRef ga_nop =
                      tree_man->CreateNopExpr(lut_operand, boolType, tree_nodeRef(), tree_nodeRef(), function_id);
                  prev_stmts_to_add.push_back(ga_nop);
                  lut_operand = GetPointer<gimple_assign>(ga_nop)->op0;
               }
            };
            check_lut_compatibility(op1);
            check_lut_compatibility(op2);
            check_lut_compatibility(op3);
            check_lut_compatibility(op4);
            check_lut_compatibility(op5);
            check_lut_compatibility(op6);
            check_lut_compatibility(op7);
            check_lut_compatibility(op8);
            tree_nodeRef new_op1 = tree_man->create_lut_expr(boolType, lut_constant_node, op1, op2, op3, op4, op5, op6,
                                                             op7, op8, BUILTIN_SRCP);
            auto lut_ga = tree_man->CreateGimpleAssign(boolType, TM->CreateUniqueIntegerCst(0, boolType),
                                                       TM->CreateUniqueIntegerCst(1, boolType), new_op1, function_id,
                                                       BUILTIN_SRCP);
            auto ssa_vd = GetPointer<gimple_assign>(lut_ga)->op0;
            prev_stmts_to_add.push_back(lut_ga);
            internal_nets[lut.index] = ssa_vd;
         }
      }
      THROW_ASSERT(prev_stmts_to_add.empty(), "unexpected condition");
      /// dependencies could be broken so we may need to reorder the lut based statements
#if HAVE_ASSERTS
      auto nStmts = statements.size();
#endif
      //      for(auto stmt: statements)
      //         std::cerr<< "Before STMT " << stmt->ToString()<<"\n";
      block.second->ReorderLUTs();
      //      for(auto stmt: statements)
      //         std::cerr<< "STMT " << stmt->ToString()<<"\n";
      THROW_ASSERT(nStmts == statements.size(), "unexpected result");
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed BB" + STR(block.first));

   return modified;
}

#pragma region Life cycle

lut_transformation::lut_transformation(const ParameterConstRef Param, const application_managerRef _AppM,
                                       unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, LUT_TRANSFORMATION, _design_flow_manager, Param),
      max_lut_size(NUM_CST_allocation_default_max_lut_size)
{
   debug_level = Param->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

lut_transformation::~lut_transformation() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
lut_transformation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         if(!parameters->getOption<int>(OPT_gcc_openmp_simd))
         {
            relationships.insert(std::make_pair(BITVALUE_RANGE, SAME_FUNCTION));
         }
         relationships.insert(std::make_pair(CSE_STEP, SAME_FUNCTION));
         relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION_IPA, WHOLE_APPLICATION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         if(GetStatus() == DesignFlowStep_Status::SUCCESS)
         {
            if(!parameters->getOption<int>(OPT_gcc_openmp_simd))
            {
               relationships.insert(std::make_pair(BIT_VALUE, SAME_FUNCTION));
            }
            relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION, SAME_FUNCTION));
         }
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return relationships;
}

void lut_transformation::ComputeRelationships(DesignFlowStepSet& relationship,
                                              const DesignFlowStep::RelationshipType relationship_type)
{
   switch(relationship_type)
   {
      case(PRECEDENCE_RELATIONSHIP):
         break;
      case DEPENDENCE_RELATIONSHIP:
      {
         const auto design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
         const auto technology_flow_step_factory = GetPointerS<const TechnologyFlowStepFactory>(
             design_flow_manager.lock()->CGetDesignFlowStepFactory(DesignFlowStep::TECHNOLOGY));
         const auto technology_flow_signature =
             TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         const auto technology_flow_step = design_flow_manager.lock()->GetDesignFlowStep(technology_flow_signature);
         const auto technology_design_flow_step =
             technology_flow_step != DesignFlowGraph::null_vertex() ?
                 design_flow_graph->CGetNodeInfo(technology_flow_step)->design_flow_step :
                 technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         relationship.insert(technology_design_flow_step);
         break;
      }
      case INVALIDATION_RELATIONSHIP:
         break;
      default:
         THROW_UNREACHABLE("");
   }

   FunctionFrontendFlowStep::ComputeRelationships(relationship, relationship_type);
}

void lut_transformation::Initialize()
{
   TM = AppM->get_tree_manager();
   tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters, AppM));
   THROW_ASSERT(GetPointer<const HLS_manager>(AppM)->get_HLS_device(), "unexpected condition");
   const auto hls_d = GetPointerS<const HLS_manager>(AppM)->get_HLS_device();
   THROW_ASSERT(hls_d->has_parameter("max_lut_size"), "unexpected condition");
   max_lut_size = hls_d->get_parameter<size_t>("max_lut_size");
}

DesignFlowStep_Status lut_transformation::InternalExec()
{
   if(max_lut_size == 0 ||
      (parameters->IsParameter("lut-transformation") && !parameters->GetParameter<unsigned int>("lut-transformation")))
   {
      return DesignFlowStep_Status::UNCHANGED;
   }
   const auto fd = GetPointer<const function_decl>(TM->GetTreeNode(function_id));
   THROW_ASSERT(fd && fd->body, "Node is not a function or it has not a body");
   const auto sl = GetPointer<const statement_list>(fd->body);
   THROW_ASSERT(sl, "Body is not a statement list");

   bool modified = false;
   for(std::pair<unsigned int, blocRef> block : sl->list_of_bloc)
   {
      if(this->CheckIfProcessable(block))
      {
         modified |= this->ProcessBasicBlock(block);
      }
   }

   if(modified)
   {
      function_behavior->UpdateBBVersion();
      return DesignFlowStep_Status::SUCCESS;
   }
   return DesignFlowStep_Status::UNCHANGED;
}

#pragma endregion

#pragma GCC diagnostic pop
