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
 * @file Range_Analysis.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "Range_Analysis.hpp"

#include "BinaryOpNode.hpp"
#include "Bit_Value_opt.hpp"
#include "NodeContainer.hpp"
#include "OpNode.hpp"
#include "Parameter.hpp"
#include "PhiOpNode.hpp"
#include "SigmaOpNode.hpp"
#include "SymbValueRange.hpp"
#include "UnaryOpNode.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "bit_lattice.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "custom_map.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "ext_tree_node.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "op_graph.hpp"
#include "range_analysis_helper.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#include <filesystem>
#include <map>
#include <set>
#include <sstream>
#include <vector>

#define BITVALUE_UPDATE // Read/write bitvalue information during the analysis
#define INTEGER_PTR     // Pointers are considered as integers
// #define EARLY_DEAD_CODE_RESTART // Abort analysis when dead code is detected instead of waiting step's end
#define RA_JUMPSET

#define RA_EXEC_NORMAL 0
#define RA_EXEC_READONLY 1
#define RA_EXEC_SKIP 2

#ifndef NDEBUG
extern bool _ra_enable_abs;
extern bool _ra_enable_negate;
extern bool _ra_enable_sext;
extern bool _ra_enable_zext;

extern bool _ra_enable_add;
extern bool _ra_enable_sub;
extern bool _ra_enable_mul;
extern bool _ra_enable_sdiv;
extern bool _ra_enable_udiv;
extern bool _ra_enable_srem;
extern bool _ra_enable_urem;
extern bool _ra_enable_shl;
extern bool _ra_enable_shr;
extern bool _ra_enable_and;
extern bool _ra_enable_or;
extern bool _ra_enable_xor;
extern bool _ra_enable_min;
extern bool _ra_enable_max;

extern bool _ra_enable_ternary;

extern bool _ra_enable_load;

#define OPERATION_OPTION(opts, X)                                                                          \
   if((opts).erase("no_" #X))                                                                              \
   {                                                                                                       \
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: " #X " operation disabled"); \
      _ra_enable_##X = false;                                                                              \
   }
#else
#define OPERATION_OPTION(opts, X) void(0)
#endif

REF_FORWARD_DECL(ValueRange);
CONSTREF_FORWARD_DECL(ValueRange);

using bw_t = Range::bw_t;
using VarNodes = NodeContainer::VarNodes;
using OpNodes = NodeContainer::OpNodes;
using DefMap = NodeContainer::DefMap;
using UseMap = NodeContainer::UseMap;

static const size_t _fixed_iterations_count = 16L;

// ========================================================================== //
// Static global functions and definitions
// ========================================================================== //

// Used to print pseudo-edges in the Constraint Graph dot
static std::string pestring;
static std::stringstream pseudoEdgesString(pestring);

static int updateIR(const VarNode* varNode, const tree_managerRef& TM,
                    int
#ifndef NDEBUG
                        debug_level
#endif
                    ,
                    application_managerRef AppM)
{
   const auto V = varNode->getValue();
   const auto ssa_node = TM->GetTreeReindex(GET_INDEX_CONST_NODE(V));
   const auto interval = varNode->getRange();
   auto* SSA = GetPointer<ssa_name>(GET_NODE(ssa_node));
   if(SSA == nullptr || interval->isUnknown())
   {
      return ut_None;
   }

#ifdef BITVALUE_UPDATE
   auto updateBitValue = [&](ssa_name* ssa, const std::deque<bit_lattice>& bv) -> int {
      const auto curr_bv = string_to_bitstring(ssa->bit_values);
      if(isBetter(bitstring_to_string(bv), ssa->bit_values))
      {
         ssa->bit_values = bitstring_to_string(bv);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "BitValue updated for " + ssa->ToString() + " " + GET_CONST_NODE(ssa->type)->get_kind_text() +
                            ": " + SSA->bit_values + " <= " + bitstring_to_string(curr_bv));
         return ut_BitValue;
      }
      return ut_None;
   };
#endif

   const bool isSigned = range_analysis::isSignedType(SSA->type);
   if(SSA->range != nullptr)
   {
      if(SSA->range->isSameRange(interval))
      {
         return ut_None;
      }
      if(!AppM->ApplyNewTransformation())
      {
         return ut_None;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Modified range " + SSA->range->ToString() + " to " + interval->ToString() + " for " +
                         SSA->ToString() + " " + GET_CONST_NODE(SSA->type)->get_kind_text());
   }
   else
   {
      auto newBW = interval->getBitWidth();
      if(interval->isFullSet())
      {
         return ut_None;
      }
      if(interval->isConstant())
      {
         newBW = 0U;
      }
      else
      {
         if(interval->isRegular())
         {
            newBW = isSigned ? Range::neededBits(interval->getSignedMin(), interval->getSignedMax(), true) :
                               Range::neededBits(interval->getUnsignedMin(), interval->getUnsignedMax(), false);
            const auto currentBW = tree_helper::TypeSize(V);
            if(newBW >= currentBW)
            {
               return ut_None;
            }
         }
         else if(interval->isAnti())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Anti range " + interval->ToString() + " not stored for " + SSA->ToString() + " " +
                               GET_CONST_NODE(SSA->type)->get_kind_text());
            return ut_None;
         }
         else if(interval->isEmpty())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Empty range not stored for " + SSA->ToString() + " " +
                               GET_CONST_NODE(SSA->type)->get_kind_text());
            return ut_None;
         }
      }
      if(!AppM->ApplyNewTransformation())
      {
         return ut_None;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Added range " + interval->ToString() + "<" + STR(newBW) + "> for " + SSA->ToString() + " " +
                         GET_CONST_NODE(SSA->type)->get_kind_text());
   }

   int updateState = ut_None;
   auto bit_values = string_to_bitstring(SSA->bit_values);
   if(interval->isAnti() || interval->isEmpty())
   {
      updateState = ut_Range;
   }
   else if(interval->isFullSet())
   {
      updateState = ut_Range;
#ifdef BITVALUE_UPDATE
      bit_values = interval->getBitValues(isSigned);
#endif
   }
   else
   {
      updateState = ut_Range;

#ifdef BITVALUE_UPDATE
      if(!bit_values.empty())
      {
         auto range_bv = interval->getBitValues(isSigned);
         const auto sup_bv = sup(bit_values, range_bv, interval->getBitWidth(), isSigned, interval->getBitWidth() == 1);
         if(bit_values != sup_bv)
         {
            bit_values = sup_bv;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Range bit_values: " + bitstring_to_string(range_bv));
         }
      }
      else
      {
         bit_values = interval->getBitValues(isSigned);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Range bit_values: " + bitstring_to_string(bit_values));
      }
#endif
   }

   if(!AppM->ApplyNewTransformation())
   {
      return ut_None;
   }
   auto resUpdate = updateBitValue(SSA, bit_values);
   if(resUpdate == ut_BitValue)
   {
      updateState |= ut_BitValue;
      Bit_Value_opt::constrainSSA(SSA, TM);
      AppM->RegisterTransformation("RangeAnalysis", V);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---BitValue updated for " + SSA->ToString() + " " + GET_CONST_NODE(SSA->type)->get_kind_text() +
                         ": " + bitstring_to_string(bit_values) + " <= " + SSA->bit_values);
   }

   if(const auto* gp = GetPointer<const gimple_phi>(GET_NODE(SSA->CGetDefStmt())))
   {
      if(gp->CGetDefEdgesList().size() == 1)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Sigma defined variable not considered for invalidation loop...");
         return ut_None;
      }
   }
   return updateState;
}

static unsigned int evaluateBranch(const tree_nodeRef br_op, const blocRef branchBB
#ifndef NDEBUG
                                   ,
                                   int debug_level
#endif
)
{
   // Evaluate condition variable if possible
   if(tree_helper::IsConstant(br_op))
   {
      const auto branchValue = tree_helper::GetConstValue(br_op);
      if(branchValue)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Branch variable value is " + STR(branchValue) + ", false edge BB" + STR(branchBB->false_edge) +
                            " to be removed");
         return branchBB->false_edge;
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Branch variable value is " + STR(branchValue) + ", true edge BB" + STR(branchBB->true_edge) +
                            " to be removed");
         return branchBB->true_edge;
      }
   }
   else if(const auto* bin_op = GetPointer<const binary_expr>(GET_CONST_NODE(br_op)))
   {
      const auto* l = GetPointer<const integer_cst>(GET_CONST_NODE(bin_op->op0));
      const auto* r = GetPointer<const integer_cst>(GET_CONST_NODE(bin_op->op1));
      if(l != nullptr && r != nullptr)
      {
         const auto lc = tree_helper::get_integer_cst_value(l);
         const auto rc = tree_helper::get_integer_cst_value(r);
         RangeRef lhs(new Range(Regular, Range::max_digits, lc, lc));
         RangeRef rhs(new Range(Regular, Range::max_digits, rc, rc));
         const auto branchValue =
             BinaryOpNode::evaluate(bin_op->get_kind(), 1, lhs, rhs, range_analysis::isSignedType(bin_op->op0));
         THROW_ASSERT(branchValue->isConstant(), "Constant binary operation should resolve to either true or false");
         if(branchValue->getUnsignedMax())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Branch condition " + STR(lc) + " " + bin_op->get_kind_text() + " " + STR(rc) + " == " +
                               STR(branchValue) + ", false edge BB" + STR(branchBB->false_edge) + " to be removed");
            return branchBB->false_edge;
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Branch condition " + STR(lc) + " " + bin_op->get_kind_text() + " " + STR(rc) + " == " +
                               STR(branchValue) + ", false edge BB" + STR(branchBB->false_edge) + " to be removed");
            return branchBB->true_edge;
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Branch condition has non-integer cst_node operands, skipping...");
      return bloc::EXIT_BLOCK_ID;
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch variable is a non-integer cst_node, skipping...");
   return bloc::EXIT_BLOCK_ID;
}

// ========================================================================== //
// ControlDep
// ========================================================================== //
/// Specific type of OpNode used in Nuutila's strongly connected
/// components algorithm.
class ControlDepOpNode : public OpNode
{
 private:
   VarNode* source;
   RangeRef eval() const override;

 public:
   ControlDepOpNode(VarNode* sink, VarNode* source);
   ControlDepOpNode(const ControlDepOpNode&) = delete;
   ControlDepOpNode(ControlDepOpNode&&) = delete;
   ControlDepOpNode& operator=(const ControlDepOpNode&) = delete;
   ControlDepOpNode& operator=(ControlDepOpNode&&) = delete;

   OperationId getValueId() const override
   {
      return OperationId::ControlDepId;
   }

   std::vector<VarNode*> getSources() const override
   {
      return {source};
   }

   inline VarNode* getSource() const
   {
      return source;
   }

   void print(std::ostream& OS) const override;
   void printDot(std::ostream& OS) const override;

   static bool classof(ControlDepOpNode const*)
   {
      return true;
   }

   static bool classof(OpNode const* BO)
   {
      return BO->getValueId() == OperationId::ControlDepId;
   }
};

ControlDepOpNode::ControlDepOpNode(VarNode* _sink, VarNode* _source)
    : OpNode(ValueRangeRef(new ValueRange(_sink->getMaxRange())), _sink, nullptr), source(_source)
{
}

RangeRef ControlDepOpNode::eval() const
{
   return RangeRef(new Range(Regular, Range::max_digits));
}

void ControlDepOpNode::print(std::ostream& /*OS*/) const
{
}

void ControlDepOpNode::printDot(std::ostream& /*OS*/) const
{
}

// ========================================================================== //
// Nuutila
// ========================================================================== //

/* A map from variables to the operations where these variables are used as bounds */
using SymbMap = UseMap;

class Nuutila
{
 public:
   using key_type = VarNodes::key_type;
   using key_compare = VarNodes::key_compare;
   using mapped_type = VarNodes::mapped_type;

   /**
    * @brief Finds the strongly connected components in the constraint graph formed by Variables and UseMap
    * Finds the strongly connected components in the constraint graph formed by Variables and UseMap. The class receives
    * the map of futures to insert the control dependence edges in the constraint graph. These edges are removed after
    * the class is done computing the SCCs.
    */
   Nuutila(const VarNodes& varNodes, UseMap& useMap, const SymbMap& symbMap
#ifndef NDEBUG
           ,
           int _debug_level
#endif
   );
   Nuutila(const Nuutila&) = delete;
   Nuutila(Nuutila&&) = delete;
   Nuutila& operator=(const Nuutila&) = delete;
   Nuutila& operator=(Nuutila&&) = delete;

   const CustomSet<mapped_type>& getComponent(const key_type n) const;

   inline auto begin()
   {
      return worklist.rbegin();
   }

   inline auto cbegin() const
   {
      return worklist.crbegin();
   }

   inline auto end()
   {
      return worklist.rend();
   }

   inline auto cend() const
   {
      return worklist.crend();
   }

 private:
#ifndef NDEBUG
   int debug_level;
#endif

   const VarNodes& variables;
   int index;
   std::map<key_type, int, key_compare> dfs;
   std::map<key_type, key_type, key_compare> root;
   std::set<key_type, key_compare> inComponent;
   std::map<key_type, CustomSet<mapped_type>, key_compare> components;
   std::deque<key_type> worklist;

   /**
    * @brief Adds the edges that ensure that we solve a future before fixing its interval.
    *
    * @param useMap
    * @param symbMap
    * @param vars
    */
   void addControlDependenceEdges(UseMap& useMap, const SymbMap& symbMap, const VarNodes& vars);

   /**
    * @brief Removes the control dependence edges from the constraint graph.
    *
    * @param useMap
    */
   void delControlDependenceEdges(UseMap& useMap);

   /**
    * @brief Finds SCCs using Nuutila's algorithm.
    * This algorithm is divided in two parts. The first calls the recursive visit procedure on every node in the
    * constraint graph. The second phase revisits these nodes, grouping them in components.
    *
    * @param V
    * @param stack
    * @param useMap
    */
   void visit(const key_type& V, std::stack<key_type>& stack, const UseMap& useMap);
};

Nuutila::Nuutila(const VarNodes& varNodes, UseMap& useMap, const SymbMap& symbMap
#ifndef NDEBUG
                 ,
                 int _debug_level)
    : debug_level(_debug_level),
#else
                 )
    :
#endif
      variables(varNodes)
{
   // Copy structures
   index = 0;

   // Initialize DFS control variable for each Value in the graph
   for(const auto& [key, node] : varNodes)
   {
      dfs[key] = -1;
   }

   addControlDependenceEdges(useMap, symbMap, varNodes);
   // Iterate again over all varnodes of the constraint graph
   for(const auto& [key, node] : varNodes)
   {
      // If the Value has not been visited yet, call visit for him
      if(dfs[key] < 0)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Start visit from " + STR(node));
         std::stack<key_type> pilha;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
         visit(key, pilha, useMap);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
   }
   delControlDependenceEdges(useMap);
}

const CustomSet<Nuutila::mapped_type>& Nuutila::getComponent(const key_type n) const
{
   THROW_ASSERT(components.count(n), "Required component not found: " + STR(n));
   return components.at(n);
}

void Nuutila::addControlDependenceEdges(UseMap& useMap, const SymbMap& symbMap, const VarNodes& vars)
{
   for(const auto& [key, users] : symbMap)
   {
      for(const auto& op : users)
      {
         THROW_ASSERT(vars.count(key), "Variable should be stored in VarNodes map");
         const auto source = vars.at(key);
         const auto cdedge = new ControlDepOpNode(op->getSink(), source);
         useMap[key].insert(cdedge);
      }
   }
}

void Nuutila::delControlDependenceEdges(UseMap& useMap)
{
   for(auto& [key, users] : useMap)
   {
      for(auto it = users.begin(); it != users.end();)
      {
         if(auto cd = GetOp<ControlDepOpNode>(*it))
         {
#ifndef NDEBUG
            // Add pseudo edge to the string
            const auto& V = cd->getSource()->getValue();
            if(tree_helper::IsConstant(V))
            {
               pseudoEdgesString << " " << tree_helper::GetConstValue(V) << " -> ";
            }
            else
            {
               pseudoEdgesString << " \"" << V << "\" -> ";
            }
            const auto& VS = cd->getSink()->getValue();
            pseudoEdgesString << " \"" << VS << "\" [style=dashed]\n";
#endif
            // Remove pseudo edge from the map
            delete cd;
            it = users.erase(it);
         }
         else
         {
            ++it;
         }
      }
   }
}

void Nuutila::visit(const key_type& V, std::stack<key_type>& stack, const UseMap& useMap)
{
   dfs[V] = index;
   ++index;
   root[V] = V;

   // Visit every node defined in an instruction that uses V
   for(const auto& op : useMap.at(V))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, op->ToString());
      const auto& sink = op->getSink()->getId();
      if(dfs[sink] < 0)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->" + GET_CONST_NODE(op->getSink()->getValue())->ToString());
         visit(sink, stack, useMap);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      if(!inComponent.count(sink) && (dfs[root[V]] >= dfs[root[sink]]))
      {
         root[V] = root[sink];
      }
   }

   // The second phase of the algorithm assigns components to stacked nodes
   if(key_compare()(root[V], V) == key_compare()(V, root[V]))
   {
      // Neither the worklist nor the map of components is part of Nuutila's
      // original algorithm. We are using these data structures to get a
      // topological ordering of the SCCs without having to go over the root
      // list once more.
      worklist.push_back(V);
      components[V].insert(variables.at(V));
      inComponent.insert(V);
      while(!stack.empty() && (dfs[stack.top()] > dfs[V]))
      {
         auto node = stack.top();
         stack.pop();
         inComponent.insert(node);
         components[V].insert(variables.at(node));
      }
   }
   else
   {
      stack.push(V);
   }
}

// ========================================================================== //
// Meet
// ========================================================================== //
class Meet
{
 public:
   static bool fixed(OpNode* op);

   /**
    * @brief This is the meet operator of the growth analysis.
    * The growth analysis will change the bounds of each variable, if necessary. Initially, each variable is bound to
    * either the undefined interval, e.g. [., .], or to a constant interval, e.g., [3, 15]. After this analysis runs,
    * there will be no undefined interval. Each variable will be either bound to a constant interval, or to [-, c], or
    * to [c, +], or to [-, +].
    *
    * @param op
    * @param constantvector
    * @return true
    * @return false
    */
   static bool widen(OpNode* op, const std::vector<APInt>& constantvector);

   static bool growth(OpNode* op);

   /**
    * @brief This is the meet operator of the cropping analysis.
    * Whereas the growth analysis expands the bounds of each variable, regardless of intersections in the constraint
    * graph, the cropping analysis shrinks these bounds back to ranges that respect the intersections.
    *
    * @param op
    * @param constantvector
    * @return true
    * @return false
    */
   static bool narrow(OpNode* op, const std::vector<APInt>& constantvector);

   static bool crop(OpNode* op);

#ifndef NDEBUG
   static int debug_level;
#endif

 private:
   /**
    * @brief Get the first constant from vector greater than val
    *
    * @param constantvector
    * @param val
    * @return const APInt&
    */
   static const APInt& getFirstGreaterFromVector(const std::vector<APInt>& constantvector, const APInt& val);

   /**
    * @brief Get the first constant from vector less than val
    *
    * @param constantvector
    * @param val
    * @return const APInt&
    */
   static const APInt& getFirstLessFromVector(const std::vector<APInt>& constantvector, const APInt& val);
};

#ifndef NDEBUG
int Meet::debug_level = DEBUG_LEVEL_NONE;
#endif

const APInt& Meet::getFirstGreaterFromVector(const std::vector<APInt>& constantvector, const APInt& val)
{
   for(const auto& vapint : constantvector)
   {
      if(vapint >= val)
      {
         return vapint;
      }
   }
   return Range::Max;
}

const APInt& Meet::getFirstLessFromVector(const std::vector<APInt>& constantvector, const APInt& val)
{
   for(auto vit = constantvector.rbegin(), vend = constantvector.rend(); vit != vend; ++vit)
   {
      const auto& vapint = *vit;
      if(vapint <= val)
      {
         return vapint;
      }
   }
   return Range::Min;
}

bool Meet::fixed(OpNode* op)
{
   const auto oldInterval = op->getSink()->getRange();
   const auto newInterval = op->eval();

   op->getSink()->setRange(newInterval);
   if(op->getInstruction())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "FIXED::@" + STR(GET_INDEX_CONST_NODE(op->getInstruction())) + ": " + oldInterval->ToString() +
                         " -> " + newInterval->ToString());
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "FIXED::artificial phi : " + oldInterval->ToString() + " -> " + newInterval->ToString());
   }
   return !oldInterval->isSameRange(newInterval);
}

bool Meet::widen(OpNode* op, const std::vector<APInt>& constantvector)
{
   const auto oldRange = op->getSink()->getRange();
   const auto newRange = op->eval();

   auto intervalWiden = [&](RangeConstRef oldInterval, RangeConstRef newInterval) {
      const auto bw = oldInterval->getBitWidth();
      if(oldInterval->isUnknown() || oldInterval->isEmpty() || oldInterval->isAnti() || newInterval->isEmpty() ||
         newInterval->isAnti())
      {
         if(oldInterval->isAnti() && newInterval->isAnti() && !newInterval->isSameRange(oldInterval))
         {
            const auto oldAnti = oldInterval->getAnti();
            const auto newAnti = newInterval->getAnti();
            const auto& oldLower = oldAnti->getLower();
            const auto& oldUpper = oldAnti->getUpper();
            const auto& newLower = newAnti->getLower();
            const auto& newUpper = newAnti->getUpper();
            const auto& nlconstant = getFirstGreaterFromVector(constantvector, newLower);
            const auto& nuconstant = getFirstLessFromVector(constantvector, newUpper);

            if(newLower > oldLower || newUpper < oldUpper)
            {
               const auto& l = newLower > oldLower ? nlconstant : oldLower;
               const auto& u = newUpper < oldUpper ? nuconstant : oldUpper;
               if(l > u)
               {
                  return RangeRef(new Range(Regular, bw));
               }
               return RangeRef(new Range(Anti, bw, l, u));
            }
         }
         else
         {
            // Sometimes sigma operation could cause confusion after maximum widening has been reached and generate
            // loops
            if(!oldInterval->isUnknown() && oldInterval->isFullSet() && newInterval->isAnti())
            {
               return RangeRef(oldInterval->clone());
            }
            if(oldInterval->isRegular() && newInterval->isAnti())
            {
               return oldInterval->unionWith(newInterval);
            }
            return RangeRef(newInterval->clone());
         }
      }
      else
      {
         const auto& oldLower = oldInterval->getLower();
         const auto& oldUpper = oldInterval->getUpper();
         const auto& newLower = newInterval->getLower();
         const auto& newUpper = newInterval->getUpper();

         // Jump-set
         const auto& nlconstant = getFirstLessFromVector(constantvector, newLower);
         const auto& nuconstant = getFirstGreaterFromVector(constantvector, newUpper);

         if(newLower < oldLower || newUpper > oldUpper)
         {
            return RangeRef(new Range(Regular, bw, newLower < oldLower ? nlconstant : oldLower,
                                      newUpper > oldUpper ? nuconstant : oldUpper));
         }
      }
      //    THROW_UNREACHABLE("Meet::widen unreachable state");
      return RangeRef(oldInterval->clone());
   };

   const auto widen = intervalWiden(oldRange, newRange);
   //    THROW_ASSERT(oldRange->getSpan() <= widen->getSpan(), "Widening should produce bigger range: " +
   //    oldRange->ToString() + " > " + widen->ToString());
   op->getSink()->setRange(widen);

   const auto sinkRange = op->getSink()->getRange();

   if(op->getInstruction())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "WIDEN::@" + STR(GET_INDEX_CONST_NODE(op->getInstruction())) + ": " + oldRange->ToString() +
                         " -> " + newRange->ToString() + " -> " + sinkRange->ToString());
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "WIDEN::@artificial phi : " + oldRange->ToString() + " -> " + newRange->ToString() + " -> " +
                         sinkRange->ToString());
   }
   return !oldRange->isSameRange(sinkRange);
}

bool Meet::growth(OpNode* op)
{
   const auto oldRange = op->getSink()->getRange();
   const auto newRange = op->eval();

   auto intervalGrowth = [](RangeConstRef oldInterval, RangeConstRef newInterval) {
      if(oldInterval->isUnknown() || oldInterval->isEmpty() || oldInterval->isAnti() || newInterval->isEmpty() ||
         newInterval->isAnti())
      {
         return RangeRef(newInterval->clone());
      }
      else
      {
         auto bw = oldInterval->getBitWidth();
         const auto& oldLower = oldInterval->getLower();
         const auto& oldUpper = oldInterval->getUpper();
         const auto& newLower = newInterval->getLower();
         const auto& newUpper = newInterval->getUpper();

         if(newLower < oldLower || newUpper > oldUpper)
         {
            return RangeRef(new Range(Regular, bw, newLower < oldLower ? Range::Min : oldLower,
                                      newUpper > oldUpper ? Range::Max : oldUpper));
         }
      }
      //    THROW_UNREACHABLE("Meet::growth unreachable state");
      return RangeRef(oldInterval->clone());
   };

   op->getSink()->setRange(intervalGrowth(oldRange, newRange));

   const auto sinkRange = op->getSink()->getRange();
   if(op->getInstruction())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "GROWTH::@" + STR(GET_INDEX_CONST_NODE(op->getInstruction())) + ": " + oldRange->ToString() +
                         " -> " + sinkRange->ToString());
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "GROWTH::artificial phi : " + oldRange->ToString() + " -> " + sinkRange->ToString());
   }
   return !oldRange->isSameRange(sinkRange);
}

bool Meet::narrow(OpNode* op, const std::vector<APInt>& constantvector)
{
   const auto oldRange = op->getSink()->getRange();
   const auto newRange = op->eval();

   auto intervalNarrow = [&](RangeConstRef oldInterval, RangeConstRef newInterval) {
      if(newInterval->isConstant())
      {
         return RangeRef(newInterval->clone());
      }
      const auto bw = oldInterval->getBitWidth();
      if(oldInterval->isAnti() || newInterval->isAnti() || oldInterval->isEmpty() || newInterval->isEmpty())
      {
         if(oldInterval->isAnti() && newInterval->isAnti() && !newInterval->isSameRange(oldInterval))
         {
            const auto oldAnti = oldInterval->getAnti();
            const auto newAnti = newInterval->getAnti();
            const auto& oldLower = oldAnti->getLower();
            const auto& oldUpper = oldAnti->getUpper();
            const auto& newLower = newAnti->getLower();
            const auto& newUpper = newAnti->getUpper();
            const auto& nlconstant = getFirstGreaterFromVector(constantvector, newLower);
            const auto& nuconstant = getFirstLessFromVector(constantvector, newUpper);

            if(oldLower < nlconstant && oldUpper > nuconstant)
            {
               if(nlconstant <= nuconstant)
               {
                  return RangeRef(new Range(Anti, bw, nlconstant, nuconstant));
               }
               return RangeRef(new Range(Regular, bw));
            }
            if(oldLower < nlconstant)
            {
               return RangeRef(new Range(Anti, bw, nlconstant, oldUpper));
            }
            if(oldUpper > nuconstant)
            {
               return RangeRef(new Range(Anti, bw, oldLower, nuconstant));
            }
         }
         else if(newInterval->isUnknown() || !newInterval->isFullSet())
         {
            return RangeRef(newInterval->clone());
         }
      }
      else
      {
         const auto& oLower = oldInterval->isFullSet() ? Range::Min : oldInterval->getLower();
         const auto& oUpper = oldInterval->isFullSet() ? Range::Max : oldInterval->getUpper();
         const auto& nLower = newInterval->isFullSet() ? Range::Min : newInterval->getLower();
         const auto& nUpper = newInterval->isFullSet() ? Range::Max : newInterval->getUpper();
         auto sinkInterval = RangeRef(oldInterval->clone());
         if((oLower == Range::Min) && (nLower != Range::Min))
         {
            sinkInterval = RangeRef(new Range(Regular, bw, nLower, oUpper));
         }
         else if(nLower < oLower)
         {
            sinkInterval = RangeRef(new Range(Regular, bw, nLower, oUpper));
         }
         if(!sinkInterval->isAnti())
         {
            if((oUpper == Range::Max) && (nUpper != Range::Max))
            {
               sinkInterval = RangeRef(new Range(Regular, bw, sinkInterval->getLower(), nUpper));
            }
            else if(oUpper < nUpper)
            {
               sinkInterval = RangeRef(new Range(Regular, bw, sinkInterval->getLower(), nUpper));
            }
         }
         return sinkInterval;
      }
      return RangeRef(oldInterval->clone());
   };

   const auto narrow = intervalNarrow(oldRange, newRange);
   //    THROW_ASSERT(oldRange->getSpan() >= narrow->getSpan(), "Narrowing should produce smaller range: " +
   //    oldRange->ToString() + " < " + narrow->ToString());
   op->getSink()->setRange(narrow);

   const auto sinkRange = op->getSink()->getRange();
   if(op->getInstruction())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "NARROW::@" + STR(GET_INDEX_CONST_NODE(op->getInstruction())) + ": " + oldRange->ToString() +
                         " -> " + sinkRange->ToString());
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "NARROW::artificial phi : " + oldRange->ToString() + " -> " + sinkRange->ToString());
   }
   return !oldRange->isSameRange(sinkRange);
}

bool Meet::crop(OpNode* op)
{
   const auto oldRange = op->getSink()->getRange();
   const auto newRange = op->eval();
   const char _abstractState = op->getSink()->getAbstractState();

   auto intervalCrop = [](RangeConstRef oldInterval, RangeConstRef newInterval, char abstractState) {
      if(oldInterval->isAnti() || newInterval->isAnti() || oldInterval->isEmpty() || newInterval->isEmpty())
      {
         return RangeRef(newInterval->clone());
      }
      else
      {
         const auto bw = oldInterval->getBitWidth();
         if((abstractState == '-' || abstractState == '?') && (newInterval->getLower() > oldInterval->getLower()))
         {
            return RangeRef(new Range(Regular, bw, newInterval->getLower(), oldInterval->getUpper()));
         }

         if((abstractState == '+' || abstractState == '?') && (newInterval->getUpper() < oldInterval->getUpper()))
         {
            return RangeRef(new Range(Regular, bw, oldInterval->getLower(), newInterval->getUpper()));
         }
         return RangeRef(oldInterval->clone());
      }
   };

   op->getSink()->setRange(intervalCrop(oldRange, newRange, _abstractState));

   const auto sinkRange = op->getSink()->getRange();
   if(op->getInstruction())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "CROP::@" + STR(GET_INDEX_CONST_NODE(op->getInstruction())) + ": " + oldRange->ToString() +
                         " -> " + sinkRange->ToString());
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "CROP::artificial phi : " + oldRange->ToString() + " -> " + sinkRange->ToString());
   }
   return !oldRange->isSameRange(sinkRange);
}

// ========================================================================== //
// ConstraintGraph
// ========================================================================== //

class ConstraintGraph : public NodeContainer
{
 protected:
   /**
    * @brief Perform the widening and narrowing operations
    *
    * @param compUseMap
    * @param actv
    * @param meet
    */
   void update(const UseMap& compUseMap, std::set<VarNode::key_type, VarNode::key_compare>& actv,
               std::function<bool(OpNode*, const std::vector<APInt>&)> meet)
   {
      while(!actv.empty())
      {
         const auto V = *actv.begin();
         actv.erase(V);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug,
                        "-> update: " + GET_CONST_NODE(getVarNodes().at(V)->getValue())->ToString());

         // The use list.
         const auto& L = compUseMap.at(V);

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "-->");
         for(auto* op : L)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "> " + op->getSink()->ToString());
            if(meet(op, constantvector))
            {
               // I want to use it as a set, but I also want
               // keep an order of insertions and removals.
               const auto& val = op->getSink()->getId();
               actv.insert(val);
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "<--");
      }
   }

   void update(size_t nIterations, const UseMap& compUseMap, std::set<VarNode::key_type, VarNode::key_compare>& actv)
   {
      std::deque<VarNode::key_type> queue(actv.begin(), actv.end());
      actv.clear();
      while(!queue.empty())
      {
         const auto V = queue.front();
         queue.pop_front();
         // The use list.
         const auto& L = compUseMap.at(V);
         for(auto op : L)
         {
            if(nIterations == 0)
            {
               return;
            }
            --nIterations;
            if(Meet::fixed(op))
            {
               const auto& next = op->getSink()->getId();
               if(std::find(queue.begin(), queue.end(), next) == queue.end())
               {
                  queue.push_back(next);
               }
            }
         }
      }
   }

   virtual void preUpdate(const UseMap& compUseMap, std::set<VarNode::key_type, VarNode::key_compare>& entryPoints) = 0;
   virtual void posUpdate(const UseMap& compUseMap, std::set<VarNode::key_type, VarNode::key_compare>& activeVars,
                          const CustomSet<VarNode*>& component) = 0;

 private:
#ifndef NDEBUG
   int debug_level;
   int graph_debug;
#endif

   const application_managerRef AppM;

   // Vector containing the constants from a SCC
   // It is cleared at the beginning of every SCC resolution
   std::vector<APInt> constantvector;

   /**
    * @brief Analyze branch instruction and build conditional value range
    *
    * @param br Branch instruction
    * @param branchBB Branch basic block
    * @param function_id Function id
    * @return unsigned int Return dead basic block to be removed when necessary and possible (bloc::ENTRY_BLOCK_ID
    * indicates no dead block found, bloc::EXIT_BLOCK_ID indicates constant condition was found but could not be
    * evaluated)
    */
   unsigned int buildCVR(const gimple_cond* br, const blocRef branchBB, unsigned int function_id)
   {
      if(GetPointer<const cst_node>(GET_CONST_NODE(br->op0)) != nullptr)
      {
         return evaluateBranch(br->op0, branchBB
#ifndef NDEBUG
                               ,
                               debug_level
#endif
         );
      }
      THROW_ASSERT(GET_CONST_NODE(br->op0)->get_kind() == ssa_name_K,
                   "Non SSA variable found in branch (" + GET_CONST_NODE(br->op0)->get_kind_text() + " " +
                       GET_CONST_NODE(br->op0)->ToString() + ")");
      const auto Cond = range_analysis::branchOpRecurse(br->op0);

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Branch condition is " + Cond->get_kind_text() + " " + Cond->ToString());
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      if(const auto* bin_op = GetPointer<const binary_expr>(Cond))
      {
         if(!range_analysis::isCompare(bin_op))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not a compare condition, skipping...");
            return bloc::ENTRY_BLOCK_ID;
         }

         if(!range_analysis::isValidType(bin_op->op0) || !range_analysis::isValidType(bin_op->op1))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Non-integer operands, skipping...");
            return bloc::ENTRY_BLOCK_ID;
         }

         // Create VarNodes for comparison operands explicitly
         // TODO: use_bbi should be that of the BB where the branch condition is evaluated (which might be different
         // from the one where the gimple_cond statement is located)
         const auto varOp0 = addVarNode(bin_op->op0, function_id, br->bb_index);
         const auto varOp1 = addVarNode(bin_op->op1, function_id, br->bb_index);

         // Gets the successors of the current basic block.
         const auto TrueBBI = branchBB->true_edge;
         const auto FalseBBI = branchBB->false_edge;

         // We have a Variable-Constant comparison.
         const auto Op0 = GET_CONST_NODE(bin_op->op0);
         const auto Op1 = GET_CONST_NODE(bin_op->op1);
         tree_nodeConstRef constant = nullptr;
         tree_nodeConstRef variable = nullptr;

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Op0 is " + Op0->get_kind_text() + " and Op1 is " + Op1->get_kind_text());

         // If both operands are constants, nothing to do here
         if(GetPointer<const cst_node>(Op0) != nullptr && GetPointer<const cst_node>(Op1) != nullptr)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
            return evaluateBranch(br->op0, branchBB
#ifndef NDEBUG
                                  ,
                                  debug_level
#endif
            );
         }

         // Then there are two cases: variable being compared to a constant,
         // or variable being compared to another variable

         // Op0 is constant, Op1 is variable
         if(GetPointer<const cst_node>(Op0) != nullptr)
         {
            constant = Op0;
            variable = bin_op->op1;
            // Op0 is variable, Op1 is constant
         }
         else if(GetPointer<const cst_node>(Op1) != nullptr)
         {
            constant = Op1;
            variable = bin_op->op0;
         }
         // Both are variables
         // which means constant == 0 and variable == 0

         if(constant != nullptr)
         {
            const kind pred = range_analysis::isSignedType(variable) ? bin_op->get_kind() :
                                                                       range_analysis::op_unsigned(bin_op->get_kind());
            const kind swappred = range_analysis::op_swap(pred);
            RangeRef CR = tree_helper::Range(constant);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Variable bitwidth is " + STR(tree_helper::TypeSize(variable)) + " and constant value is " +
                               constant->ToString());

            auto TValues = (GET_INDEX_CONST_NODE(variable) == GET_INDEX_CONST_NODE(bin_op->op0)) ?
                               range_analysis::makeSatisfyingCmpRegion(pred, CR) :
                               range_analysis::makeSatisfyingCmpRegion(swappred, CR);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Condition is true on " + TValues->ToString());
            auto FValues = TValues->isFullSet() ? tree_helper::TypeRange(variable, Empty) : TValues->getAnti();
            // When dealing with eq/ne conditions it is safer to propagate only the constant branch value
            if(bin_op->get_kind() == eq_expr_K)
            {
               FValues = tree_helper::TypeRange(variable, Regular);
            }
            else if(bin_op->get_kind() == ne_expr_K)
            {
               TValues = tree_helper::TypeRange(variable, Regular);
            }

            // Create the interval using the intersection in the branch.
            const auto BT = ValueRangeRef(new ValueRange(TValues));
            const auto BF = ValueRangeRef(new ValueRange(FValues));

            addConditionalValueRange(ConditionalValueRange(variable, TrueBBI, FalseBBI, BT, BF));

            // Do the same for the operand of variable (if variable is a cast
            // instruction)
            if(const auto* Var = GetPointer<const ssa_name>(GET_CONST_NODE(variable)))
            {
               const auto* VDef = GetPointer<const gimple_assign>(GET_CONST_NODE(Var->CGetDefStmt()));
               if(VDef && (GET_CONST_NODE(VDef->op1)->get_kind() == nop_expr_K ||
                           GET_CONST_NODE(VDef->op1)->get_kind() == convert_expr_K))
               {
                  const auto* cast_inst = GetPointer<const unary_expr>(GET_CONST_NODE(VDef->op1));
#ifndef NDEBUG
                  if(GET_INDEX_CONST_NODE(variable) == GET_INDEX_CONST_NODE(bin_op->op0))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "Op0 comes from a cast expression " + cast_inst->ToString());
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "Op1 comes from a cast expression" + cast_inst->ToString());
                  }
#endif

                  const auto _BT = ValueRangeRef(new ValueRange(TValues));
                  const auto _BF = ValueRangeRef(new ValueRange(FValues));

                  addConditionalValueRange(ConditionalValueRange(cast_inst->op, TrueBBI, FalseBBI, _BT, _BF));
               }
            }
         }
         else
         {
            const kind pred = range_analysis::isSignedType(bin_op->op0) ?
                                  bin_op->get_kind() :
                                  range_analysis::op_unsigned(bin_op->get_kind());
            const kind invPred = range_analysis::op_inv(pred);
            const kind swappred = range_analysis::op_swap(pred);
            const kind invSwappred = range_analysis::op_inv(swappred);

#if !defined(NDEBUG) || HAVE_ASSERTS
            const auto bw0 = tree_helper::TypeSize(bin_op->op0);
#endif
#if HAVE_ASSERTS
            const auto bw1 = tree_helper::TypeSize(bin_op->op1);
            THROW_ASSERT(bw0 == bw1, "Operands of same operation have different bitwidth (Op0 = " + STR(bw0) +
                                         ", Op1 = " + STR(bw1) + ").");
#endif

            const auto CR = tree_helper::TypeRange(bin_op->op0, Unknown);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variables bitwidth is " + STR(bw0));

            // Symbolic intervals for op0
            const auto STOp0 = ValueRangeRef(new SymbRange(CR, varOp1, pred));
            const auto SFOp0 = ValueRangeRef(new SymbRange(CR, varOp1, invPred));

            addConditionalValueRange(ConditionalValueRange(bin_op->op0, TrueBBI, FalseBBI, STOp0, SFOp0));

            // Symbolic intervals for operand of op0 (if op0 is a cast instruction)
            if(const auto* Var = GetPointer<const ssa_name>(Op0))
            {
               const auto* VDef = GetPointer<const gimple_assign>(GET_CONST_NODE(Var->CGetDefStmt()));
               if(VDef && (GET_CONST_NODE(VDef->op1)->get_kind() == nop_expr_K ||
                           GET_CONST_NODE(VDef->op1)->get_kind() == convert_expr_K))
               {
                  const auto* cast_inst = GetPointer<const unary_expr>(GET_CONST_NODE(VDef->op1));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "Op0 comes from a cast expression " + cast_inst->ToString());

                  const auto STOp0_0 = ValueRangeRef(new SymbRange(CR, varOp1, pred));
                  const auto SFOp0_0 = ValueRangeRef(new SymbRange(CR, varOp1, invPred));

                  addConditionalValueRange(ConditionalValueRange(cast_inst->op, TrueBBI, FalseBBI, STOp0_0, SFOp0_0));
               }
            }

            // Symbolic intervals for op1
            const auto STOp1 = ValueRangeRef(new SymbRange(CR, varOp0, swappred));
            const auto SFOp1 = ValueRangeRef(new SymbRange(CR, varOp0, invSwappred));
            addConditionalValueRange(ConditionalValueRange(bin_op->op1, TrueBBI, FalseBBI, STOp1, SFOp1));

            // Symbolic intervals for operand of op1 (if op1 is a cast instruction)
            if(const auto* Var = GetPointer<const ssa_name>(Op1))
            {
               const auto* VDef = GetPointer<const gimple_assign>(GET_CONST_NODE(Var->CGetDefStmt()));
               if(VDef && (GET_CONST_NODE(VDef->op1)->get_kind() == nop_expr_K ||
                           GET_CONST_NODE(VDef->op1)->get_kind() == convert_expr_K))
               {
                  const auto* cast_inst = GetPointer<const unary_expr>(GET_CONST_NODE(VDef->op1));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "Op1 comes from a cast expression" + cast_inst->ToString());

                  const auto STOp1_1 = ValueRangeRef(new SymbRange(CR, varOp0, swappred));
                  const auto SFOp1_1 = ValueRangeRef(new SymbRange(CR, varOp0, invSwappred));

                  addConditionalValueRange(ConditionalValueRange(cast_inst->op, TrueBBI, FalseBBI, STOp1_1, SFOp1_1));
               }
            }
         }
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Not a compare condition, skipping...");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      return bloc::ENTRY_BLOCK_ID;
   }

   bool buildCVR(const gimple_multi_way_if* mwi, const blocRef /*mwifBB*/, unsigned int function_id)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Multi-way if with " + STR(mwi->list_of_cond.size()) + " conditions");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");

      // Find else branch BBI if any
      unsigned int DefaultBBI = 0;
      for(const auto& [cond, targetBBI] : mwi->list_of_cond)
      {
         if(!cond)
         {
            DefaultBBI = targetBBI;
            break;
         }
      }

      // Analyze each if branch condition
      CustomMap<tree_nodeConstRef, std::map<unsigned int, ValueRangeRef>> switchSSAMap;
      for(const auto& [cond, targetBBI] : mwi->list_of_cond)
      {
         if(!cond)
         {
            // Default branch is handled at the end
            continue;
         }

         if(GetPointer<const cst_node>(GET_CONST_NODE(cond)) != nullptr)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Branch variable is a cst_node, dead code elimination necessary!");
            // TODO: abort and call dead code elimination to evaluate constant condition
            //    return true;
            continue;
         }
         THROW_ASSERT(GET_CONST_NODE(cond)->get_kind() == ssa_name_K,
                      "Case conditional variable should be an ssa_name (" + GET_CONST_NODE(cond)->get_kind_text() +
                          " " + GET_CONST_NODE(cond)->ToString() + ")");
         const auto case_compare = range_analysis::branchOpRecurse(cond);
         if(const auto* cmp_op = GetPointer<const binary_expr>(case_compare))
         {
            if(!range_analysis::isCompare(cmp_op))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Not a compare condition, skipping...");
               continue;
            }

            if(!range_analysis::isValidType(cmp_op->op0) || !range_analysis::isValidType(cmp_op->op1))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Non-integer operands, skipping...");
               continue;
            }

            // Create VarNodes for comparison operands explicitly
            // TODO: use_bbi should be that of the BB where the branch condition is evaluated (which might be different
            // from the one where the gimple_multi_way_if statement is located)
            const auto varOp0 = addVarNode(cmp_op->op0, function_id, mwi->bb_index);
            const auto varOp1 = addVarNode(cmp_op->op1, function_id, mwi->bb_index);

            // We have a Variable-Constant comparison.
            const auto Op0 = GET_CONST_NODE(cmp_op->op0);
            const auto Op1 = GET_CONST_NODE(cmp_op->op1);
            const struct integer_cst* constant = nullptr;
            tree_nodeConstRef variable = nullptr;

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Op0 is " + Op0->get_kind_text() + " and Op1 is " + Op1->get_kind_text());

            // If both operands are constants, nothing to do here
            if(GetPointer<const cst_node>(Op0) != nullptr && GetPointer<const cst_node>(Op1) != nullptr)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "Both operands are constants, dead code elimination necessary!");
               // TODO: abort and call dead code elimination to evaluate constant condition
               //    return true;
               continue;
            }

            // Then there are two cases: variable being compared to a constant,
            // or variable being compared to another variable

            // Op0 is constant, Op1 is variable
            if((constant = GetPointer<const integer_cst>(Op0)) != nullptr)
            {
               variable = cmp_op->op1;
            }
            else if((constant = GetPointer<const integer_cst>(Op1)) != nullptr)
            {
               // Op0 is variable, Op1 is constant
               variable = cmp_op->op0;
            }
            // Both are variables
            // which means constant == 0 and variable == 0

            if(constant != nullptr)
            {
               const kind pred = range_analysis::isSignedType(variable) ?
                                     cmp_op->get_kind() :
                                     range_analysis::op_unsigned(cmp_op->get_kind());
               const kind swappred = range_analysis::op_swap(pred);
               const auto bw = static_cast<bw_t>(tree_helper::TypeSize(variable));
               RangeConstRef CR(new Range(Regular, bw, constant->value, constant->value));
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "Variable bitwidth is " + STR(bw) + " and constant value is " + STR(constant->value));

               const auto tmpT = (GET_INDEX_CONST_NODE(variable) == GET_INDEX_CONST_NODE(cmp_op->op0)) ?
                                     range_analysis::makeSatisfyingCmpRegion(pred, CR) :
                                     range_analysis::makeSatisfyingCmpRegion(swappred, CR);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Condition is true on " + tmpT->ToString());

               RangeRef TValues = tmpT->isFullSet() ? RangeRef(new Range(Regular, bw)) : tmpT;

               // Create the interval using the intersection in the branch.
               auto BT = ValueRangeRef(new ValueRange(TValues));
               switchSSAMap[variable].insert(std::make_pair(targetBBI, BT));

               // Do the same for the operand of variable (if variable is a cast
               // instruction)
               if(const auto* Var = GetPointer<const ssa_name>(GET_CONST_NODE(variable)))
               {
                  const auto* VDef = GetPointer<const gimple_assign>(GET_CONST_NODE(Var->CGetDefStmt()));
                  if(VDef && (GET_CONST_NODE(VDef->op1)->get_kind() == nop_expr_K ||
                              GET_CONST_NODE(VDef->op1)->get_kind() == convert_expr_K))
                  {
                     const auto* cast_inst = GetPointer<const unary_expr>(GET_CONST_NODE(VDef->op1));
#ifndef NDEBUG
                     if(GET_INDEX_CONST_NODE(variable) == GET_INDEX_CONST_NODE(cmp_op->op0))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "Op0 comes from a cast expression " + cast_inst->ToString());
                     }
                     else
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "Op1 comes from a cast expression" + cast_inst->ToString());
                     }
#endif

                     auto _BT = ValueRangeRef(new ValueRange(TValues));
                     switchSSAMap[cast_inst->op].insert(std::make_pair(targetBBI, _BT));
                  }
               }
            }
            else
            {
               const kind pred = range_analysis::isSignedType(cmp_op->op0) ?
                                     cmp_op->get_kind() :
                                     range_analysis::op_unsigned(cmp_op->get_kind());
               const kind swappred = range_analysis::op_swap(pred);

#if !defined(NDEBUG) || HAVE_ASSERTS
               const auto bw0 = tree_helper::TypeSize(cmp_op->op0);
#endif
#if HAVE_ASSERTS
               const auto bw1 = tree_helper::TypeSize(cmp_op->op1);
               THROW_ASSERT(bw0 == bw1, "Operands of same operation have different bitwidth (Op0 = " + STR(bw0) +
                                            ", Op1 = " + STR(bw1) + ").");
#endif

               const auto CR = tree_helper::TypeRange(cmp_op->op0, Unknown);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variables bitwidth is " + STR(bw0));

               // Symbolic intervals for op0
               const auto STOp0 = ValueRangeRef(new SymbRange(CR, varOp1, pred));
               switchSSAMap[cmp_op->op0].insert(std::make_pair(targetBBI, STOp0));

               // Symbolic intervals for operand of op0 (if op0 is a cast instruction)
               if(const auto* Var = GetPointer<const ssa_name>(Op0))
               {
                  const auto* VDef = GetPointer<const gimple_assign>(GET_CONST_NODE(Var->CGetDefStmt()));
                  if(VDef && (GET_CONST_NODE(VDef->op1)->get_kind() == nop_expr_K ||
                              GET_CONST_NODE(VDef->op1)->get_kind() == convert_expr_K))
                  {
                     const auto* cast_inst = GetPointer<const unary_expr>(GET_CONST_NODE(VDef->op1));
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "Op0 comes from a cast expression" + cast_inst->ToString());

                     const auto STOp0_0 = ValueRangeRef(new SymbRange(CR, varOp1, pred));
                     switchSSAMap[cast_inst->op].insert(std::make_pair(targetBBI, STOp0_0));
                  }
               }

               // Symbolic intervals for op1
               const auto STOp1 = ValueRangeRef(new SymbRange(CR, varOp0, swappred));
               switchSSAMap[cmp_op->op1].insert(std::make_pair(targetBBI, STOp1));

               // Symbolic intervals for operand of op1 (if op1 is a cast instruction)
               if(const auto* Var = GetPointer<const ssa_name>(Op1))
               {
                  const auto* VDef = GetPointer<const gimple_assign>(GET_CONST_NODE(Var->CGetDefStmt()));
                  if(VDef && (GET_CONST_NODE(VDef->op1)->get_kind() == nop_expr_K ||
                              GET_CONST_NODE(VDef->op1)->get_kind() == convert_expr_K))
                  {
                     const auto* cast_inst = GetPointer<const unary_expr>(GET_CONST_NODE(VDef->op1));
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "Op1 comes from a cast expression" + cast_inst->ToString());

                     const auto STOp1_1 = ValueRangeRef(new SymbRange(CR, varOp0, swappred));
                     switchSSAMap[cast_inst->op].insert(std::make_pair(targetBBI, STOp1_1));
                  }
               }
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "Multi-way condition different from binary_expr not handled, skipping... (" +
                               case_compare->get_kind_text() + " " + case_compare->ToString() + ")");
         }
      }

      // Handle else branch, if there is any
      // TODO: maybe it should be better to leave fullset as interval for default edge
      //       because usign getAnti implies internal values to be excluded while they
      //       could still be valid values
      if(DefaultBBI)
      {
         for(auto& [var, VSM] : switchSSAMap)
         {
            auto elseRange = tree_helper::TypeRange(var, Empty);
            for(const auto& [targetBBI, interval] : VSM)
            {
               elseRange = elseRange->unionWith(interval->getRange());
            }
            elseRange = elseRange->getAnti();
            VSM.insert(std::make_pair(DefaultBBI, ValueRangeRef(new ValueRange(elseRange))));
         }
      }

      for(const auto& [var, VSM] : switchSSAMap)
      {
         addConditionalValueRange(ConditionalValueRange(var, VSM));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      return false;
   }

   /*
    *	This method builds a map that binds each variable label to the
    * operations
    * where this variable is used.
    */
   UseMap buildUseMap(const CustomSet<VarNode*>& component)
   {
      UseMap compUseMap;
      for(const auto var : component)
      {
         // Get the component's use list for V (it does not exist until we try to get it)
         auto& list = compUseMap[var->getId()];
         // Get the use list of the variable in component
         const auto p = getUses().at(var->getId());
         // For each operation in the list, verify if its sink is in the component
         for(const auto use : p)
         {
            const auto sink = use->getSink();
            // If it is, add op to the component's use map
            if(component.count(sink))
            {
               list.insert(use);
            }
         }
      }
      return compUseMap;
   }

   /*
    * Used to insert constant in the right position
    */
   void insertConstantIntoVector(const APInt& constantval, bw_t bw)
   {
      constantvector.push_back(constantval.extOrTrunc(bw, true));
   }

   /*
    * Create a vector containing all constants related to the component
    * They include:
    *   - Constants inside component
    *   - Constants that are source of an edge to an entry point
    *   - Constants from intersections generated by sigmas
    */
   void buildConstantVector(const CustomSet<VarNode*>& component, const UseMap& compusemap)
   {
      // Remove all elements from the vector
      constantvector.clear();

      // Get constants inside component (TODO: may not be necessary, since
      // components with more than 1 node may
      // never have a constant inside them)
      for(const auto* varNode : component)
      {
         const auto& V = varNode->getValue();
         if(const auto* ic = GetPointer<const integer_cst>(GET_CONST_NODE(V)))
         {
            insertConstantIntoVector(tree_helper::get_integer_cst_value(ic), varNode->getBitWidth());
         }
      }

      // Get constants that are sources of operations whose sink belong to the
      // component
      for(const auto* varNode : component)
      {
         auto dfit = getDefs().find(varNode->getId());
         if(dfit == getDefs().end())
         {
            continue;
         }

         auto pushConstFor = [this](const APInt& cst, bw_t bw, kind pred) {
            if(range_analysis::isCompare(pred))
            {
               if(pred == eq_expr_K || pred == ne_expr_K)
               {
                  insertConstantIntoVector(cst, bw);
                  insertConstantIntoVector(cst - 1, bw);
                  insertConstantIntoVector(cst + 1, bw);
               }
               else if(pred == uneq_expr_K)
               {
                  const auto ucst = cst.extOrTrunc(bw, false);
                  insertConstantIntoVector(ucst, bw);
                  insertConstantIntoVector(ucst - 1, bw);
                  insertConstantIntoVector(ucst + 1, bw);
               }
               else if(pred == gt_expr_K || pred == le_expr_K)
               {
                  insertConstantIntoVector(cst, bw);
                  insertConstantIntoVector(cst + 1, bw);
               }
               else if(pred == ge_expr_K || pred == lt_expr_K)
               {
                  insertConstantIntoVector(cst, bw);
                  insertConstantIntoVector(cst - 1, bw);
               }
               else if(pred == ungt_expr_K || pred == unle_expr_K)
               {
                  const auto ucst = cst.extOrTrunc(bw, false);
                  insertConstantIntoVector(ucst, bw);
                  insertConstantIntoVector(ucst + 1, bw);
               }
               else if(pred == unge_expr_K || pred == unlt_expr_K)
               {
                  const auto ucst = cst.extOrTrunc(bw, false);
                  insertConstantIntoVector(ucst, bw);
                  insertConstantIntoVector(ucst - 1, bw);
               }
               else
               {
                  THROW_UNREACHABLE("unexpected condition (" + tree_node::GetString(pred) + ")");
               }
            }
            else
            {
               insertConstantIntoVector(cst, bw);
            }
         };

         // Handle BinaryOp case
         if(const auto* bop = GetOp<BinaryOpNode>(dfit->second))
         {
            const auto* source1 = bop->getSource1();
            const auto& sourceval1 = source1->getValue();
            const auto* source2 = bop->getSource2();
            const auto& sourceval2 = source2->getValue();

            const auto pred = bop->getOpcode();

            if(const auto* const1 = GetPointer<const integer_cst>(GET_CONST_NODE(sourceval1)))
            {
               const auto bw = source1->getBitWidth();
               const auto cst_val = tree_helper::get_integer_cst_value(const1);
               pushConstFor(cst_val, bw, pred); // TODO: maybe should swap predicate for lhs constant?
            }
            if(const auto* const2 = GetPointer<const integer_cst>(GET_CONST_NODE(sourceval2)))
            {
               const auto bw = source2->getBitWidth();
               const auto cst_val = tree_helper::get_integer_cst_value(const2);
               pushConstFor(cst_val, bw, pred);
            }
         }
         // Handle PhiOp case
         else if(const auto* pop = GetOp<PhiOpNode>(dfit->second))
         {
            for(size_t i = 0, e = pop->getNumSources(); i < e; ++i)
            {
               const auto* source = pop->getSource(i);
               const auto& sourceval = source->getValue();
               if(const auto* ic = GetPointer<const integer_cst>(GET_CONST_NODE(sourceval)))
               {
                  insertConstantIntoVector(tree_helper::get_integer_cst_value(ic), source->getBitWidth());
               }
            }
         }
      }

      // Get constants used in intersections
      for(const auto& varOps : compusemap)
      {
         for(const auto* op : varOps.second)
         {
            const auto* sigma = GetOp<SigmaOpNode>(op);
            // Symbolic intervals are discarded, as they don't have fixed values yet
            if(sigma == nullptr || SymbRange::classof(sigma->getIntersect().get()))
            {
               continue;
            }
            const auto rintersect = op->getIntersect()->getRange();
            const auto bw = rintersect->getBitWidth();
            if(rintersect->isAnti())
            {
               const auto anti = rintersect->getAnti();
               const auto& lb = anti->getLower();
               const auto& ub = anti->getUpper();
               if((lb != Range::Min) && (lb != Range::Max))
               {
                  insertConstantIntoVector(lb - 1, bw);
                  insertConstantIntoVector(lb, bw);
               }
               if((ub != Range::Min) && (ub != Range::Max))
               {
                  insertConstantIntoVector(ub, bw);
                  insertConstantIntoVector(ub + 1, bw);
               }
            }
            else
            {
               const auto& lb = rintersect->getLower();
               const auto& ub = rintersect->getUpper();
               if((lb != Range::Min) && (lb != Range::Max))
               {
                  insertConstantIntoVector(lb - 1, bw);
                  insertConstantIntoVector(lb, bw);
               }
               if((ub != Range::Min) && (ub != Range::Max))
               {
                  insertConstantIntoVector(ub, bw);
                  insertConstantIntoVector(ub + 1, bw);
               }
            }
         }
      }

      // Sort vector in ascending order and remove duplicates
      std::sort(constantvector.begin(), constantvector.end(), [](const APInt& i1, const APInt& i2) { return i1 < i2; });

      // std::unique doesn't remove duplicate elements, only
      // move them to the end
      // This is why erase is necessary. To remove these duplicates
      // that will be now at the end.
      auto last = std::unique(constantvector.begin(), constantvector.end());
      constantvector.erase(last, constantvector.end());
   }

   /*
    * This method builds a map of variables to the lists of operations where
    * these variables are used as futures. Its C++ type should be something like
    * map<VarNode, List<Operation>>.
    */
   SymbMap buildSymbolicIntersectMap()
   {
      // Creates the symbolic intervals map
      SymbMap symbMap;

      // Iterate over the operations set
      for(const auto op : getOpNodes())
      {
         // If the operation is unary and its interval is symbolic
         const auto uop = GetOp<UnaryOpNode>(op);
         if(uop && SymbRange::classof(uop->getIntersect().get()))
         {
            const auto symbi = std::static_pointer_cast<const SymbRange>(uop->getIntersect());
            const auto V = symbi->getBound()->getId();
            auto p = symbMap.find(V);
            if(p != symbMap.end())
            {
               p->second.insert(op);
            }
            else
            {
               OpNodes l;
               l.insert(op);
               symbMap.insert(std::make_pair(V, std::move(l)));
            }
         }
      }
      return symbMap;
   }

   /*
    * This method evaluates once each operation that uses a variable in
    * component, so that the next SCCs after component will have entry
    * points to kick start the range analysis algorithm.
    */
   void propagateToNextSCC(const CustomSet<VarNode*>& component)
   {
      const auto& uses = getUses();
      for(const auto& var : component)
      {
         const auto& p = uses.at(var->getId());
         for(auto* op : p)
         {
            /// VarNodes belonging to the current SCC must not be evaluated otherwise we break the fixed point
            /// previously computed
            if(component.contains(op->getSink()))
            {
               continue;
            }
            auto* sigmaop = GetOp<SigmaOpNode>(op);
            op->getSink()->setRange(op->eval());
            if((sigmaop != nullptr) && sigmaop->getIntersect()->getRange()->isUnknown())
            {
               sigmaop->markUnresolved();
            }
         }
      }
   }

   void generateEntryPoints(const CustomSet<VarNode*>& component,
                            std::set<VarNode::key_type, VarNode::key_compare>& entryPoints)
   {
      const auto& defs = getDefs();
      // Iterate over the varnodes in the component
      for(const auto varNode : component)
      {
         const auto& V = varNode->getValue();
         if(const auto* ssa = GetPointer<const ssa_name>(GET_CONST_NODE(V)))
         {
            if(const auto* phi_def = GetPointer<const gimple_phi>(GET_CONST_NODE(ssa->CGetDefStmt())))
            {
               if(phi_def->CGetDefEdgesList().size() == 1)
               {
                  auto dit = defs.find(varNode->getId());
                  if(dit != defs.end())
                  {
                     auto* bop = dit->second;
                     auto* defop = GetOp<SigmaOpNode>(bop);

                     if((defop != nullptr) && defop->isUnresolved())
                     {
                        defop->getSink()->setRange(bop->eval());
                        defop->markResolved();
                     }
                  }
               }
            }
         }
         if(!varNode->getRange()->isUnknown())
         {
            entryPoints.insert(varNode->getId());
         }
      }
   }

   void solveFutures(const CustomSet<VarNode*>& component, const SymbMap& symbMap)
   {
      // Iterate again over the varnodes in the component
      for(auto* varNode : component)
      {
         solveFuturesSC(varNode, symbMap);
      }
   }

   void solveFuturesSC(VarNode* varNode, const SymbMap& symbMap)
   {
      const auto& V = varNode->getId();
      auto sit = symbMap.find(V);
      if(sit != symbMap.end())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "Fix intersects: " + varNode->ToString());
         for(auto* op : sit->second)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "Op intersects: " + op->ToString());
            op->solveFuture();
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "Sink: " + op->ToString());
         }
      }
   }

   void generateActivesVars(const CustomSet<VarNode*>& component,
                            std::set<VarNode::key_type, VarNode::key_compare>& activeVars)
   {
      for(const auto varNode : component)
      {
         if(tree_helper::IsConstant(varNode->getValue()))
         {
            continue;
         }
         activeVars.insert(varNode->getId());
      }
   }

 public:
   ConstraintGraph(application_managerRef _AppM
#ifndef NDEBUG
                   ,
                   int _debug_level, int _graph_debug
#else
                   ,
                   int, int
#endif
                   )
       :
#ifndef NDEBUG
         debug_level(_debug_level),
         graph_debug(_graph_debug),
#endif
         AppM(_AppM)
   {
#ifndef NDEBUG
      NodeContainer::debug_level = debug_level;
#endif
   }

   /// Iterates through all instructions in the function and builds the graph.
   bool buildGraph(unsigned int function_id)
   {
      const auto TM = AppM->get_tree_manager();
      const auto FB = AppM->CGetFunctionBehavior(function_id);
      const auto FD = GetPointer<const function_decl>(TM->CGetTreeNode(function_id));
      const auto SL = GetPointer<const statement_list>(GET_CONST_NODE(FD->body));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Analysing function " + tree_helper::GetMangledFunctionName(FD) + " with " +
                         STR(SL->list_of_bloc.size()) + " blocks");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch variables analysis...");
      for(const auto& [idx, BB] : SL->list_of_bloc)
      {
         const auto& stmt_list = BB->CGetStmtList();
         if(stmt_list.empty())
         {
            continue;
         }

         const auto terminator = GET_CONST_NODE(stmt_list.back());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "BB" + STR(idx) + " has terminator type " + terminator->get_kind_text() + " " +
                            terminator->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
         if(const auto* br = GetPointer<const gimple_cond>(terminator))
         {
#ifdef EARLY_DEAD_CODE_RESTART
            if(buildCVR(br, BB, function_id))
            {
               // Dead code elimination necessary
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               return true;
            }
#else
            buildCVR(br, BB, function_id);
#endif
         }
         else if(const auto* mwi = GetPointer<const gimple_multi_way_if>(terminator))
         {
#ifdef EARLY_DEAD_CODE_RESTART
            if(buildCVR(mwi, BB, function_id))
            {
               // Dead code elimination necessary
               return true;
            }
#else
            buildCVR(mwi, BB, function_id);
#endif
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch variables analysis completed");

      for(const auto& [idx, BB] : SL->list_of_bloc)
      {
         const auto& phi_list = BB->CGetPhiList();
         if(phi_list.size())
         {
            for(const auto& stmt : phi_list)
            {
               if(range_analysis::isValidInstruction(stmt, FB))
               {
                  addOperation(stmt, AppM);
               }
            }
         }

         const auto& stmt_list = BB->CGetStmtList();
         if(stmt_list.size())
         {
            for(const auto& stmt : stmt_list)
            {
               if(range_analysis::isValidInstruction(stmt, FB))
               {
                  addOperation(stmt, AppM);
               }
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Graph built for function " + tree_helper::GetMangledFunctionName(FD));
      return false;
   }

   void buildVarNodes()
   {
      // Initializes the nodes and the use map structure.
      for(auto& pair : getVarNodes())
      {
         pair.second->init(!getDefs().count(pair.first));
      }
   }

   void findIntervals(
#ifndef NDEBUG
       const ParameterConstRef parameters, const std::string& step_name
#endif
   )
   {
      const auto symbMap = buildSymbolicIntersectMap();
// List of SCCs
#ifndef NDEBUG
      Nuutila sccList(getVarNodes(), getUses(), symbMap, graph_debug);
#else
      Nuutila sccList(getVarNodes(), getUses(), symbMap);
#endif

      for(const auto& n : sccList)
      {
         const auto& component = sccList.getComponent(n);

#ifndef NDEBUG
         if(DEBUG_LEVEL_VERY_PEDANTIC <= graph_debug)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "Components:");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "-->");
            for(const auto* var : component)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, var->ToString());
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "-------------");
         }
#endif
         if(component.size() == 1)
         {
            VarNode* var = *component.begin();
            solveFuturesSC(var, symbMap);
            auto varDef = getDefs().find(var->getId());
            if(varDef != getDefs().end())
            {
               auto* op = varDef->second;
               var->setRange(op->eval());
            }
            if(var->getRange()->isUnknown())
            {
               var->setRange(var->getMaxRange());
            }
         }
         else
         {
            const auto compUseMap = buildUseMap(component);

#ifdef RA_JUMPSET
            // Create vector of constants inside component
            // Comment this line below to deactivate jump-set
            buildConstantVector(component, compUseMap);
#ifndef NDEBUG
            if(DEBUG_LEVEL_VERY_PEDANTIC <= graph_debug)
            {
               std::stringstream ss;
               for(const auto& cnst : constantvector)
               {
                  ss << cnst << ", ";
               }
               if(!constantvector.empty())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug,
                                 "Constant lattice: {-inf, " + ss.str() + "+inf}");
               }
            }
#endif
#endif

            // Get the entry points of the SCC
            std::set<VarNode::key_type, VarNode::key_compare> entryPoints;
#ifndef NDEBUG
            auto printEntryFor = [&](const std::string& mType) {
               const auto& vars = getVarNodes();
               if(DEBUG_LEVEL_VERY_PEDANTIC <= graph_debug)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, mType + " step entry points:");
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "-->");
                  for(const auto& el : entryPoints)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug,
                                    GET_CONST_NODE(vars.at(el)->getValue())->ToString());
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "<--");
               }
            };
#endif

            generateEntryPoints(component, entryPoints);
#ifndef NDEBUG
            printEntryFor("Fixed");
#endif
            // iterate a fixed number of time before widening
            update(static_cast<size_t>(component.size()) * _fixed_iterations_count, compUseMap, entryPoints);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug,
                           "Printed constraint graph to " +
                               printToFile("after_" + step_name + ".fixed." +
                                               STR(GET_INDEX_CONST_NODE(getVarNodes().at(n)->getValue())) + ".dot",
                                           parameters));

            generateEntryPoints(component, entryPoints);
#ifndef NDEBUG
            printEntryFor("Widen");
#endif
            // First iterate till fix point
            preUpdate(compUseMap, entryPoints);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "fixIntersects");
            solveFutures(component, symbMap);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, " --");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug,
                           "Printed constraint graph to " +
                               printToFile("after_" + step_name + ".futures." +
                                               STR(GET_INDEX_CONST_NODE(getVarNodes().at(n)->getValue())) + ".dot",
                                           parameters));

            for(const auto varNode : component)
            {
               if(varNode->getRange()->isUnknown())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "initialize unknown: " + varNode->ToString());
                  //    THROW_UNREACHABLE("unexpected condition");
                  varNode->setRange(varNode->getMaxRange());
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug,
                           "Printed constraint graph to " +
                               printToFile("after_" + step_name + ".int." +
                                               STR(GET_INDEX_CONST_NODE(getVarNodes().at(n)->getValue())) + ".dot",
                                           parameters));

            // Second iterate till fix point
            std::set<VarNode::key_type, VarNode::key_compare> activeVars;
            generateActivesVars(component, activeVars);
#ifndef NDEBUG
            printEntryFor("Narrow");
#endif
            posUpdate(compUseMap, activeVars, component);
         }
         propagateToNextSCC(component);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug,
                     "Printed final constraint graph to " + printToFile(step_name + ".constraints.dot", parameters));
   }

#ifndef NDEBUG
   std::string printToFile(const std::string& file_name, const ParameterConstRef parameters) const
   {
      std::string output_directory = parameters->getOption<std::string>(OPT_dot_directory) + "RangeAnalysis/";
      if(!std::filesystem::exists(output_directory))
      {
         std::filesystem::create_directories(output_directory);
      }
      const std::string full_name = output_directory + file_name;
      std::ofstream file(full_name);
      printDot(file);
      return full_name;
   }

   /// Prints the content of the graph in dot format. For more information
   /// about the dot format, see: http://www.graphviz.org/pdf/dotguide.pdf
   void printDot(std::ostream& OS) const
   {
      // Print the header of the .dot file.
      OS << "digraph dotgraph {\n"
         << "label=\"Constraint Graph for \'all\' functions\";\n"
         << "node [shape=record,fontname=\"Times-Roman\",fontsize=14];\n";

      // Print the body of the .dot file.
      for(const auto& [key, node] : getVarNodes())
      {
         const auto& V = node->getValue();
         if(tree_helper::IsConstant(V))
         {
            OS << " " << tree_helper::GetConstValue(V);
         }
         else
         {
            OS << "\"" << V << "\"";
         }
         OS << " [label=\"" << node << "\"]\n";
      }

      for(auto* op : getOpNodes())
      {
         op->printDot(OS);
         OS << '\n';
      }
      OS << pseudoEdgesString.str();
      // Print the footer of the .dot file.
      OS << "}\n";
   }
#endif
};

// ========================================================================== //
// Cousot
// ========================================================================== //
class Cousot : public ConstraintGraph
{
 private:
   void preUpdate(const UseMap& compUseMap, std::set<VarNode::key_type, VarNode::key_compare>& entryPoints) override
   {
      update(compUseMap, entryPoints, Meet::widen);
   }

   void posUpdate(const UseMap& compUseMap, std::set<VarNode::key_type, VarNode::key_compare>& entryPoints,
                  const CustomSet<VarNode*>& /*component*/) override
   {
      update(compUseMap, entryPoints, Meet::narrow);
   }

 public:
   Cousot(application_managerRef _AppM, int _debug_level, int _graph_debug)
       : ConstraintGraph(_AppM, _debug_level, _graph_debug)
   {
   }
};

// ========================================================================== //
// CropDFS
// ========================================================================== //
class CropDFS : public ConstraintGraph
{
 private:
   void preUpdate(const UseMap& compUseMap, std::set<VarNode::key_type, VarNode::key_compare>& entryPoints) override
   {
      update(compUseMap, entryPoints, [](OpNode* b, const std::vector<APInt>&) { return Meet::growth(b); });
   }

   void posUpdate(const UseMap& compUseMap, std::set<VarNode::key_type, VarNode::key_compare>& /*activeVars*/,
                  const CustomSet<VarNode*>& component) override
   {
      storeAbstractStates(component);
      for(const auto& op : getOpNodes())
      {
         if(component.count(op->getSink()))
         {
            crop(compUseMap, op);
         }
      }
   }

   void storeAbstractStates(const CustomSet<VarNode*>& component)
   {
      for(const auto& varNode : component)
      {
         varNode->storeAbstractState();
      }
   }

   void crop(const UseMap& compUseMap, OpNode* op)
   {
      OpNodes activeOps;
      CustomSet<const VarNode*> visitedOps;

      // init the activeOps only with the op received
      activeOps.insert(op);

      while(!activeOps.empty())
      {
         auto* V = *activeOps.begin();
         activeOps.erase(V);
         const VarNode* sink = V->getSink();

         // if the sink has been visited go to the next activeOps
         if(visitedOps.count(sink))
         {
            continue;
         }

         Meet::crop(V);
         visitedOps.insert(sink);

         // The use list.of sink
         const auto& L = compUseMap.at(sink->getId());
         for(auto* opr : L)
         {
            activeOps.insert(opr);
         }
      }
   }

 public:
   CropDFS(application_managerRef _AppM, int _debug_level, int _graph_debug)
       : ConstraintGraph(_AppM, _debug_level, _graph_debug)
   {
   }
};

static void ParmAndRetValPropagation(unsigned int function_id, const application_managerRef AppM,
                                     const ConstraintGraphRef CG,
                                     int
#ifndef NDEBUG
                                         debug_level
#endif
)
{
   const auto CGM = AppM->CGetCallGraphManager();
   const auto call_graph = CGM->CGetCallGraph();
   const auto f_v = CGM->GetVertex(function_id);
   const auto TM = AppM->get_tree_manager();
   const auto fnode = TM->CGetTreeNode(function_id);
   const auto FD = GetPointer<const function_decl>(fnode);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "Parameters and return value propagation on function " + tree_helper::GetMangledFunctionName(FD));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");

   // Fetch the function arguments (formal parameters) into the data structure and generate PhiOp nodes for parameters
   // call values
   std::vector<tree_nodeConstRef> parameters;
   std::vector<PhiOpNode*> matchers;
#ifndef NDEBUG
   auto pindex = 0;
#endif
   for(const auto& pnode : FD->list_of_args)
   {
      const auto ssa_id = AppM->getSSAFromParm(function_id, GET_INDEX_CONST_NODE(pnode));
      const auto ssa_node = TM->CGetTreeReindex(ssa_id);
      if(ssa_node && range_analysis::isValidType(ssa_node))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Parameter " + std::to_string(pindex) + " defined as " + GET_CONST_NODE(ssa_node)->ToString());
         // TODO: use_bbi should be the BBI where the variable is first used inside the function
         const auto sink = CG->addVarNode(ssa_node, function_id, BB_ENTRY);

         // Check for pragma mask directives user defined range
         const auto parm = GetPointerS<const parm_decl>(GET_CONST_NODE(pnode));
         if(parm->range)
         {
            sink->setRange(parm->range);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Range hints found in parameter declaration: " + parm->range->ToString());
         }
         else
         {
            sink->setRange(sink->getMaxRange());
         }
         parameters.push_back(ssa_node);
         matchers.push_back(new PhiOpNode(ValueRangeRef(new ValueRange(sink->getRange())), sink, nullptr));
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Parameter " + std::to_string(pindex) + " unused or with invalid type");
         parameters.push_back(nullptr);
         matchers.push_back(nullptr);
      }
#ifndef NDEBUG
      ++pindex;
#endif
   }

   if(!boost::in_degree(f_v, *call_graph) || AppM->CGetCallGraphManager()->GetRootFunctions().count(function_id))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "No call statements for this function, skipping...");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      return;
   }

   // Check if the function returns a supported value type. If not, no return
   // value matching is done
   const auto ret_type = tree_helper::GetFunctionReturnType(fnode);
   auto hasReturn = ret_type && range_analysis::isValidType(ret_type);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "Function has " +
                      (hasReturn ? ("return type " + GET_CONST_NODE(ret_type)->get_kind_text()) : "no return type"));

   // Creates the data structure which receives the return values of the
   // function, if there is any
   std::vector<VarNode*> returnVars;
   if(hasReturn)
   {
      const auto SL = GetPointer<const statement_list>(GET_CONST_NODE(FD->body));
      for(const auto& [idx, BB] : SL->list_of_bloc)
      {
         const auto& stmt_list = BB->CGetStmtList();
         if(stmt_list.size())
         {
            if(const auto gr = GetPointer<const gimple_return>(GET_CONST_NODE(stmt_list.back())))
            {
               if(gr->op) // Compiler defined return statements may be without argument
               {
                  returnVars.push_back(CG->addVarNode(gr->op, function_id, gr->bb_index));
               }
            }
         }
      }
   }
   if(returnVars.empty() && hasReturn)
   {
#ifndef NDEBUG
      if(range_analysis::isValidType(ret_type))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Function should return, but no return statement was found");
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Function return type not supported");
      }
#endif
      hasReturn = false;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  std::string("Function ") + (hasReturn ? "has explicit" : "has no") + " return statement" +
                      (returnVars.size() > 1 ? "s" : ""));

   BOOST_FOREACH(EdgeDescriptor ie, boost::in_edges(f_v, *call_graph))
   {
      const auto einfo = call_graph->CGetFunctionEdgeInfo(ie);
      for(const auto call_id : einfo->direct_call_points)
      {
         const auto call_stmt = TM->CGetTreeReindex(call_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Analysing call " + GET_CONST_NODE(call_stmt)->ToString());
         const auto gn = GetPointer<const gimple_node>(GET_CONST_NODE(call_stmt));
         const auto caller_id = GET_INDEX_CONST_NODE(gn->scpe);
         const auto call_bbi = gn->bb_index;
         const std::vector<tree_nodeRef>* args = nullptr;
         tree_nodeConstRef ret_var = nullptr;
         if(const auto ga = GetPointer<const gimple_assign>(GET_CONST_NODE(call_stmt)))
         {
            const auto ce = GetPointer<const call_expr>(GET_CONST_NODE(ga->op1));
            args = &ce->args;
            ret_var = ga->op0;
         }
         else if(const auto* gc = GetPointer<const gimple_call>(GET_CONST_NODE(call_stmt)))
         {
            args = &gc->args;
         }
         else
         {
            THROW_UNREACHABLE("Call statement should be a gimple_assign or a gimple_call");
         }
         THROW_ASSERT(args->size() == parameters.size(), "Function parameters and call arguments size mismatch");

         // Do the inter-procedural construction of CG
         VarNode* to = nullptr;
         VarNode* from = nullptr;

         // Match formal and real parameters
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
         for(size_t i = 0; i < parameters.size(); ++i)
         {
            if(parameters[i] == nullptr)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "Parameter " + STR(i) + " was constant, matching not necessary");
               continue;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           GET_CONST_NODE(args->at(i))->ToString() + " bound to argument " +
                               GET_CONST_NODE(parameters[i])->ToString());
            // Add real parameter to the CG
            from = CG->addVarNode(args->at(i), caller_id, call_bbi);

            // Connect nodes
            matchers[i]->addSource(from);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");

         // Match return values when return type is stored from caller
         if(hasReturn && GET_CONST_NODE(call_stmt)->get_kind() != gimple_call_K)
         {
            // Add caller instruction to the CG (it receives the return value)
            to = CG->addVarNode(ret_var, caller_id, call_bbi);
            to->setRange(to->getMaxRange());

            auto* phiOp = new PhiOpNode(ValueRangeRef(new ValueRange(to->getRange())), to, nullptr);
            for(VarNode* var : returnVars)
            {
               phiOp->addSource(var);
            }
            CG->pushOperation(phiOp);

#ifndef NDEBUG
            if(DEBUG_LEVEL_VERY_PEDANTIC <= debug_level)
            {
               std::string phiString = "Return variable " + STR(phiOp->getSink()->getValue()) + " = PHI<";
               for(size_t i = 0; i < phiOp->getNumSources(); ++i)
               {
                  phiString += STR(phiOp->getSource(i)->getValue()) + ", ";
               }
               phiString[phiString.size() - 2] = '>';
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, phiString);
            }
#endif
         }
      }
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   for(auto m : matchers)
   {
      if(m)
      {
         CG->pushOperation(m);
#ifndef NDEBUG
         if(DEBUG_LEVEL_VERY_PEDANTIC <= debug_level)
         {
            std::string phiString = STR(m->getSink()->getValue()) + " = PHI<";
            for(size_t i = 0; i < m->getNumSources(); ++i)
            {
               phiString += STR(m->getSource(i)->getValue()) + ", ";
            }
            phiString[phiString.size() - 2] = '>';
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, phiString);
         }
#endif
      }
   }
}

// ========================================================================== //
// RangeAnalysis
// ========================================================================== //
RangeAnalysis::RangeAnalysis(const application_managerRef AM, const DesignFlowManagerConstRef dfm,
                             const ParameterConstRef par)
    : ApplicationFrontendFlowStep(AM, RANGE_ANALYSIS, dfm, par)
#ifndef NDEBUG
      ,
      graph_debug(DEBUG_LEVEL_NONE),
      iteration(0),
      stop_iteration(std::numeric_limits<decltype(stop_iteration)>::max()),
      stop_transformation(std::numeric_limits<decltype(stop_transformation)>::max())
#endif
      ,
      solverType(st_Cousot),
      requireESSA(true),
      execution_mode(RA_EXEC_NORMAL)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
   const auto opts = SplitString(parameters->getOption<std::string>(OPT_range_analysis_mode), ",");
   CustomSet<std::string> ra_mode;
   for(const auto& opt : opts)
   {
      if(opt.size())
      {
         ra_mode.insert(opt);
      }
   }
   if(ra_mode.erase("crop"))
   {
      solverType = st_Crop;
   }
   if(ra_mode.erase("noESSA"))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: no Extended SSA required");
      requireESSA = false;
   }
#ifndef NDEBUG
   if(ra_mode.erase("ro"))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: read-only mode enabled");
      execution_mode = RA_EXEC_READONLY;
   }
#endif
   if(ra_mode.erase("skip"))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: skip mode enabled");
      execution_mode = RA_EXEC_SKIP;
   }
#ifndef NDEBUG
   if(ra_mode.erase("debug_op"))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: range operations debug");
      OpNode::debug_level = debug_level;
   }
   if(ra_mode.erase("debug_graph"))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: graph debug");
      graph_debug = debug_level;
      Meet::debug_level = debug_level;
   }

   OPERATION_OPTION(ra_mode, abs);
   OPERATION_OPTION(ra_mode, negate);
   OPERATION_OPTION(ra_mode, sext);
   OPERATION_OPTION(ra_mode, zext);
   OPERATION_OPTION(ra_mode, add);
   OPERATION_OPTION(ra_mode, sub);
   OPERATION_OPTION(ra_mode, mul);
   OPERATION_OPTION(ra_mode, sdiv);
   OPERATION_OPTION(ra_mode, udiv);
   OPERATION_OPTION(ra_mode, srem);
   OPERATION_OPTION(ra_mode, urem);
   OPERATION_OPTION(ra_mode, shl);
   OPERATION_OPTION(ra_mode, shr);
   OPERATION_OPTION(ra_mode, and);
   OPERATION_OPTION(ra_mode, or);
   OPERATION_OPTION(ra_mode, xor);
   OPERATION_OPTION(ra_mode, min);
   OPERATION_OPTION(ra_mode, max);
   OPERATION_OPTION(ra_mode, ternary);
   OPERATION_OPTION(ra_mode, load);
   if(ra_mode.size() && ra_mode.begin()->size())
   {
      THROW_ASSERT(ra_mode.size() <= 2, "Too many range analysis options left to parse");
      auto it = ra_mode.begin();
      if(ra_mode.size() == 2)
      {
         auto tr = ++ra_mode.begin();
         if(it->front() == 't')
         {
            it = ++ra_mode.begin();
            tr = ra_mode.begin();
         }
         THROW_ASSERT(tr->front() == 't', "Invalid range analysis option: " + *tr);
         stop_transformation = std::strtoull(tr->data() + sizeof(char), nullptr, 10);
         if(stop_transformation == 0)
         {
            THROW_ERROR("Invalid range analysis option: " + *tr);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Range analysis: only " + STR(stop_transformation) + " transformation" +
                            (stop_transformation > 1 ? "s" : "") + " will run on last iteration");
      }
      if(it->front() == 'i')
      {
         stop_iteration = std::strtoull(it->data() + sizeof(char), nullptr, 10);
      }
      else
      {
         stop_iteration = std::strtoull(it->data(), nullptr, 10);
      }
      if(stop_iteration == 0)
      {
         THROW_ERROR("Invalid range analysis option: " + *it);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Range analysis: only " + STR(stop_iteration) + " iteration" + (stop_iteration > 1 ? "s" : "") +
                         " will run");
   }
#else
   THROW_ASSERT(ra_mode.empty(), "Invalid range analysis mode falgs. (" + *ra_mode.begin() + ")");
#endif
}

RangeAnalysis::~RangeAnalysis() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
RangeAnalysis::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   if(execution_mode == RA_EXEC_SKIP)
   {
      return relationships;
   }
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         if(!parameters->getOption<int>(OPT_gcc_openmp_simd))
         {
            relationships.insert(std::make_pair(BIT_VALUE_OPT, ALL_FUNCTIONS));
         }
         relationships.insert(std::make_pair(BLOCK_FIX, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(CALL_GRAPH_BUILTIN_CALL, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(COMPUTE_IMPLICIT_CALLS, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(DETERMINE_MEMORY_ACCESSES, ALL_FUNCTIONS));
         if(requireESSA)
         {
            relationships.insert(std::make_pair(ESSA, ALL_FUNCTIONS));
         }
         relationships.insert(std::make_pair(EXTRACT_GIMPLE_COND_OP, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(FUNCTION_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(IR_LOWERING, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(PARM2SSA, ALL_FUNCTIONS));
         if(parameters->isOption(OPT_soft_float) && parameters->getOption<bool>(OPT_soft_float))
         {
            relationships.insert(std::make_pair(SOFT_FLOAT_CG_EXT, ALL_FUNCTIONS));
         }
         relationships.insert(std::make_pair(USE_COUNTING, ALL_FUNCTIONS));
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      {
         relationships.insert(std::make_pair(UN_COMPARISON_LOWERING, ALL_FUNCTIONS));
         break;
      }
      case INVALIDATION_RELATIONSHIP:
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

void RangeAnalysis::ComputeRelationships(DesignFlowStepSet& relationships,
                                         const DesignFlowStep::RelationshipType relationship_type)
{
   if(relationship_type == INVALIDATION_RELATIONSHIP)
   {
      if(!parameters->getOption<int>(OPT_gcc_openmp_simd))
      {
         const auto dfm = design_flow_manager.lock();
         const auto design_flow_graph = dfm->CGetDesignFlowGraph();
         for(const auto f_id : fun_id_to_restart)
         {
            const auto bv_signature = FunctionFrontendFlowStep::ComputeSignature(BIT_VALUE, f_id);
            const auto frontend_bv = dfm->GetDesignFlowStep(bv_signature);
            THROW_ASSERT(frontend_bv != NULL_VERTEX, "step " + bv_signature + " is not present");
            const auto bv = design_flow_graph->CGetDesignFlowStepInfo(frontend_bv)->design_flow_step;
            relationships.insert(bv);
         }
      }
      fun_id_to_restart.clear();
   }
   ApplicationFrontendFlowStep::ComputeRelationships(relationships, relationship_type);
}

bool RangeAnalysis::HasToBeExecuted() const
{
#ifndef NDEBUG
   if(iteration >= stop_iteration || execution_mode == RA_EXEC_SKIP)
#else
   if(execution_mode == RA_EXEC_SKIP)
#endif
   {
      return false;
   }
   std::map<unsigned int, unsigned int> cur_bitvalue_ver;
   std::map<unsigned int, unsigned int> cur_bb_ver;
   const auto CGMan = AppM->CGetCallGraphManager();
   for(const auto i : CGMan->GetReachedBodyFunctions())
   {
      const auto FB = AppM->CGetFunctionBehavior(i);
      cur_bitvalue_ver[i] = FB->GetBitValueVersion();
      cur_bb_ver[i] = FB->GetBBVersion();
   }
   return cur_bb_ver != last_bb_ver || cur_bitvalue_ver != last_bitvalue_ver;
}

void RangeAnalysis::Initialize()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range Analysis step");
   fun_id_to_restart.clear();
}

DesignFlowStep_Status RangeAnalysis::Exec()
{
#ifndef NDEBUG
   if(iteration >= stop_iteration || execution_mode == RA_EXEC_SKIP)
#else
   if(execution_mode == RA_EXEC_SKIP)
#endif
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Range analysis no execution mode enabled");
      return DesignFlowStep_Status::SKIPPED;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");

   // Initialize constraint graph
   ConstraintGraphRef CG;
   switch(solverType)
   {
      case st_Cousot:
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Using jump-set abstract operators");
         CG.reset(new Cousot(AppM,
#ifndef NDEBUG
                             debug_level, graph_debug));
#else
                             DEBUG_LEVEL_NONE, DEBUG_LEVEL_NONE));
#endif
         break;
      case st_Crop:
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Using standard abstract operators");
         CG.reset(new CropDFS(AppM,
#ifndef NDEBUG
                              debug_level, graph_debug));
#else
                              DEBUG_LEVEL_NONE, DEBUG_LEVEL_NONE));
#endif
         break;
      default:
         THROW_UNREACHABLE("Unknown solver type " + STR(solverType));
         break;
   }

      // Analyse only reached functions
#if defined(EARLY_DEAD_CODE_RESTART) || !defined(NDEBUG)
   const auto TM = AppM->get_tree_manager();
#endif
   CustomOrderedSet<unsigned int> rb_funcs = AppM->CGetCallGraphManager()->GetReachedBodyFunctions();

#ifdef EARLY_DEAD_CODE_RESTART
   for(const auto f : rb_funcs)
   {
      bool dead_code_necessary = CG->buildGraph(f);
      if(dead_code_necessary)
      {
         fun_id_to_restart.insert(f);
      }
   }
   if(fun_id_to_restart.size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Following functions have unpropagated constants:");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(const auto f_id : fun_id_to_restart)
      {
         const auto FB = AppM->GetFunctionBehavior(f_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, FB->CGetBehavioralHelper()->GetMangledFunctionName());
         FB->UpdateBBVersion();
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Unpropagated constants detected, aborting...");
      return DesignFlowStep_Status::ABORTED;
   }
#else
   for(const auto& f : rb_funcs)
   {
      CG->buildGraph(f);
   }
#endif

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameters and return value propagation...");
   for(const auto f : rb_funcs)
   {
      ParmAndRetValPropagation(f, AppM, CG, debug_level);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameters and return value propagation completed");
   CG->buildVarNodes();

#ifndef NDEBUG
   CG->findIntervals(parameters, GetName() + "(" + STR(iteration) + ")");
   ++iteration;
#else
   CG->findIntervals();
#endif

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
#ifndef NDEBUG
   const auto modified = finalize(CG);
   if(stop_iteration != std::numeric_limits<decltype(stop_iteration)>::max())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Iteration " + STR(iteration) + "/" + STR(stop_iteration) + "completed (" +
                         STR(stop_iteration - iteration) + " to go)");
   }
   if(modified)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variable ranges updated");
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variable ranges reached fixed point");
   }
   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
#else
   return finalize(CG) ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
#endif
}

bool RangeAnalysis::finalize(ConstraintGraphRef CG)
{
   THROW_ASSERT(CG, "");
   const auto& vars = std::static_pointer_cast<const ConstraintGraph>(CG)->getVarNodes();
   CustomSet<unsigned int> modifiedFunctionsBit;

#ifndef NDEBUG
   if(execution_mode >= RA_EXEC_READONLY)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Bounds for " + STR(vars.size()) + " variables");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(const auto& [key, node] : vars)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Range " + node->getRange()->ToString() + " for " +
                            GET_CONST_NODE(node->getValue())->ToString());
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "IR update not applied in read-only mode");
   }
   else
   {
#endif
      const auto TM = AppM->get_tree_manager();

#ifndef NDEBUG
      unsigned long long updated = 0;
#endif
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Bounds for " + STR(vars.size()) + " variables");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(const auto& [key, node] : vars)
      {
#ifndef NDEBUG
         if(iteration == stop_iteration && updated >= stop_transformation)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Max required transformations performed. IR update aborted.");
            break;
         }
#endif
         if(const auto ut = updateIR(node, TM, debug_level, AppM))
         {
            if(ut & ut_BitValue)
            {
               const auto funID = node->getFunctionId();
               modifiedFunctionsBit.insert(funID);
#ifndef NDEBUG
               ++updated;
#endif
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Bounds updated for " + STR(updated) + "/" + STR(vars.size()) + " variables");
#ifndef NDEBUG
   }
#endif

   const auto rbf = AppM->CGetCallGraphManager()->GetReachedBodyFunctions();
   const auto cgm = AppM->CGetCallGraphManager();
   const auto cg = cgm->CGetCallGraph();
#ifndef NDEBUG
   const auto TM = AppM->get_tree_manager();
#endif

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "Modified BitValues " + STR(modifiedFunctionsBit.size()) + " functions:");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   for(const auto fUT : modifiedFunctionsBit)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     AppM->CGetFunctionBehavior(fUT)->CGetBehavioralHelper()->GetMangledFunctionName());
      fun_id_to_restart.insert(fUT);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");

   for(const auto f : rbf)
   {
      const auto FB = AppM->GetFunctionBehavior(f);
      auto isInBit = fun_id_to_restart.count(f);
      if(isInBit)
      {
         last_bb_ver[f] = FB->GetBBVersion();
         last_bitvalue_ver[f] = FB->UpdateBitValueVersion();
      }
      else
      {
         last_bb_ver[f] = FB->GetBBVersion();
         last_bitvalue_ver[f] = FB->GetBitValueVersion();
      }
   }
   return !fun_id_to_restart.empty();
}
