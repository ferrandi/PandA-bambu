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

#include "Dominance.hpp"
#include "OrderedInstructions.hpp"
#include "VarNode.hpp"

#include <filesystem>
#include <map>
#include <set>
#include <sstream>
#include <vector>

#define BITVALUE_UPDATE // Read/write bitvalue information during the analysis
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
   if(SSA == nullptr || interval->isUnknown() || varNode->makeId(V, BB_ENTRY) != varNode->getId())
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

static void compute_dominator_tree(const BBGraphRef& DT, const std::map<unsigned int, blocRef>& list_of_bloc,
                                   const BBGraphsCollectionRef& bbgc, const ParameterConstRef& parameters)
{
   /// store the IR BB graph ala boost::graph
   auto& inverse_vertex_map = DT->GetBBGraphInfo()->bb_index_map;
   inverse_vertex_map.clear();
   bbgc->clear();
   /// add vertices
   for(const auto& block : list_of_bloc)
   {
      inverse_vertex_map.try_emplace(block.first, bbgc->AddVertex(BBNodeInfoRef(new BBNodeInfo(block.second))));
   }

   /// add edges
   for(const auto& curr_bb_pair : list_of_bloc)
   {
      unsigned int curr_bb = curr_bb_pair.first;
      for(const auto& lop : list_of_bloc.at(curr_bb)->list_of_pred)
      {
         THROW_ASSERT(static_cast<bool>(inverse_vertex_map.count(lop)),
                      "BB" + STR(lop) + " (successor of BB" + STR(curr_bb) + ") does not exist");
         bbgc->AddEdge(inverse_vertex_map.at(lop), inverse_vertex_map.at(curr_bb), CFG_SELECTOR);
      }

      for(const auto& los : list_of_bloc.at(curr_bb)->list_of_succ)
      {
         if(los == bloc::EXIT_BLOCK_ID)
         {
            bbgc->AddEdge(inverse_vertex_map.at(curr_bb), inverse_vertex_map.at(los), CFG_SELECTOR);
         }
      }

      if(list_of_bloc.at(curr_bb)->list_of_succ.empty())
      {
         bbgc->AddEdge(inverse_vertex_map.at(curr_bb), inverse_vertex_map.at(bloc::EXIT_BLOCK_ID), CFG_SELECTOR);
      }
   }

   /// add a connection between entry and exit thus avoiding problems with non terminating code
   bbgc->AddEdge(inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID), inverse_vertex_map.at(bloc::EXIT_BLOCK_ID), CFG_SELECTOR);

   dominance<BBGraph> bb_dominators(BBGraph(bbgc, CFG_SELECTOR), inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID),
                                    inverse_vertex_map.at(bloc::EXIT_BLOCK_ID), parameters);
   bb_dominators.calculate_dominance_info(dominance<BBGraph>::CDI_DOMINATORS);
   for(const auto& [child, dom] : bb_dominators.get_dominator_map())
   {
      if(child != inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID))
      {
         bbgc->AddEdge(dom, child, D_SELECTOR);
      }
   }
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

   OpNodeType getValueId() const override
   {
      return OpNodeType::OpNodeType_ControlDep;
   }

   std::vector<VarNode*> getSources() const override
   {
      return {source};
   }

   void replaceSource(VarNode* _old, VarNode* _new)
   {
      if(_old->getId() == source->getId())
      {
         source = _new;
      }
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
      return BO->getValueId() == OpNodeType::OpNodeType_ControlDep;
   }
};

ControlDepOpNode::ControlDepOpNode(VarNode* _sink, VarNode* _source) : OpNode(_sink, nullptr), source(_source)
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

REF_FORWARD_DECL(VarUse);

class VarUse
{
 public:
   VarUse(VarNode* var, OpNode* op) : _var(var), _inst(op->getInstruction()), _op(op)
   {
   }

   VarUse(VarNode* var, tree_nodeConstRef inst) : _var(var), _inst(inst), _op(nullptr)
   {
   }

   unsigned long long getId() const
   {
      return static_cast<unsigned long long>(GET_INDEX_CONST_NODE(_var->getValue())) << 32 |
             GET_INDEX_CONST_NODE(_inst);
   }

   VarNode* getOperand() const
   {
      return _var;
   }

   tree_nodeConstRef getInstruction() const
   {
      return _inst;
   }

   OpNode* getUser() const
   {
      return _op;
   }

   void updateUse(VarNode* var)
   {
      _op->replaceSource(_var, var);
      _var = var;
   }

 private:
   VarNode* _var;
   tree_nodeConstRef _inst;
   OpNode* _op;
};

class PredicateBase
{
 public:
   kind Type;
   // The original operand before we renamed it.
   // This can be use by passes, when destroying predicateinfo, to know
   // whether they can just drop the intrinsic, or have to merge metadata.
   tree_nodeConstRef OriginalOp;
   PredicateBase(const PredicateBase&) = delete;
   PredicateBase& operator=(const PredicateBase&) = delete;
   PredicateBase() = delete;
   virtual ~PredicateBase() = default;

 protected:
   PredicateBase(kind PT, tree_nodeConstRef Op) : Type(PT), OriginalOp(Op)
   {
   }
};

// Mixin class for edge predicates.  The FROM block is the block where the
// predicate originates, and the TO block is the block where the predicate is
// valid.
class PredicateWithEdge : public PredicateBase
{
 public:
   unsigned int From;
   unsigned int To;

   ValueRangeRef intersect;

   explicit PredicateWithEdge(kind PType, tree_nodeConstRef Op, unsigned int _From, unsigned int _To,
                              ValueRangeRef _intersect)
       : PredicateBase(PType, Op), From(_From), To(_To), intersect(_intersect)
   {
      THROW_ASSERT(PType == gimple_cond_K || PType == gimple_multi_way_if_K,
                   "Only branch or multi-way if types allowd");
   }

   static bool classof(const PredicateBase* PB)
   {
      return PB->Type == gimple_cond_K || PB->Type == gimple_multi_way_if_K;
   }
};

// Given a predicate info that is a type of branching terminator, get the
// branching block.
static unsigned int getBranchBlock(const PredicateBase* PB)
{
   THROW_ASSERT(PredicateWithEdge::classof(PB),
                "Only branches and switches should have PHIOnly defs that require branch blocks.");
   return reinterpret_cast<const PredicateWithEdge*>(PB)->From;
}

class ValueInfoMap
{
 public:
   // Used to store information about each value we might rename.
   struct ValueInfo
   {
      // Information about each possible copy. During processing, this is each
      // inserted info. After processing, we move the uninserted ones to the
      // uninserted vector.
      std::vector<PredicateBase*> Infos;
      std::vector<PredicateBase*> UninsertedInfos;
   };

   ValueInfo& operator[](const VarNode::key_type& key)
   {
      return _m[key];
   }

   const ValueInfo& at(const VarNode::key_type& key) const
   {
      return _m.at(key);
   }

 private:
   std::map<VarNode::key_type, ValueInfo, VarNode::key_compare> _m;
};

struct RenameInfos
{
   struct DFSInfo
   {
      unsigned int DFSIn;
      unsigned int DFSOut;
   };
   using DFSInfoMap = CustomMap<decltype(bloc::number), DFSInfo>;

   DFSInfoMap DFSInfos;

   ValueInfoMap ValueInfos;

   /* The set of edges along which we can only handle phi uses, due to critical edges. */
   CustomSet<std::pair<unsigned int, unsigned int>> EdgeUsesOnly;

   /* Collect operands to rename from all conditional branch terminators, as well as multi-way if. */
   CustomSet<VarUseRef> OpsToRename;
};

struct IRVisitor : public boost::default_dfs_visitor
{
 public:
   using BBMap = decltype(statement_list::list_of_bloc);

   IRVisitor(RenameInfos& infos, NodeContainer* nc, unsigned int function_id, const application_managerRef& _AppM,
             int _debug_level)
       : _step(0),
         _infos(infos),
         _nc(nc),
         bb_map(GetPointer<const statement_list>(
                    GET_CONST_NODE(
                        GetPointer<const function_decl>(_AppM->get_tree_manager()->CGetTreeNode(function_id))->body))
                    ->list_of_bloc),
         _function_id(function_id),
         FB(_AppM->CGetFunctionBehavior(function_id)),
         AppM(_AppM),
         debug_level(_debug_level)
   {
   }

   void discover_vertex(vertex u, const BBGraph& g);

   void finish_vertex(vertex u, const BBGraph& g);

 private:
   unsigned int _step;
   RenameInfos& _infos;
   NodeContainer* const _nc;
   const BBMap& bb_map;
   const unsigned int _function_id;
   const FunctionBehaviorConstRef FB;
   const application_managerRef AppM;
   int debug_level;

   void addInfoFor(VarUseRef Op, PredicateBase* PB);

   void processBranch(tree_nodeConstRef tn);

   void processMultiWayIf(tree_nodeConstRef tn);
};

void IRVisitor::addInfoFor(VarUseRef Op, PredicateBase* PB)
{
   _infos.OpsToRename.insert(Op);
   auto& OperandInfo = _infos.ValueInfos[Op->getOperand()->getId()];
   OperandInfo.Infos.push_back(PB);
}

void IRVisitor::discover_vertex(vertex u, const BBGraph& g)
{
   const auto& BB = g.CGetBBNodeInfo(u)->block;
   _infos.DFSInfos[BB->number].DFSIn = _step++;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing BB" + STR(BB->number));

   const auto& phi_list = BB->CGetPhiList();
   for(const auto& stmt : phi_list)
   {
      if(range_analysis::isValidInstruction(stmt, FB))
      {
         _nc->addOperation(stmt, AppM);
      }
   }

   const auto& stmt_list = BB->CGetStmtList();
   if(stmt_list.size())
   {
      for(const auto& stmt : stmt_list)
      {
         if(range_analysis::isValidInstruction(stmt, FB))
         {
            _nc->addOperation(stmt, AppM);
         }
      }
      const auto& terminator = stmt_list.back();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Block terminates with " + GET_NODE(terminator)->get_kind_text() + " " + STR(terminator));
      if(GET_CONST_NODE(terminator)->get_kind() == gimple_cond_K)
      {
         processBranch(terminator);
      }
      else if(GET_CONST_NODE(terminator)->get_kind() == gimple_multi_way_if_K)
      {
         processMultiWayIf(terminator);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
}

void IRVisitor::finish_vertex(vertex u, const BBGraph& g)
{
   const auto& BB = g.CGetBBNodeInfo(u)->block;
   _infos.DFSInfos[BB->number].DFSOut = _step++;
}

void IRVisitor::processBranch(tree_nodeConstRef tn)
{
   const auto gc = GetPointer<const gimple_cond>(GET_CONST_NODE(tn));
   THROW_ASSERT(gc, "Branch instruction should be gimple_cond");
   const auto sourceBB = bb_map.at(gc->bb_index);
   THROW_ASSERT(bb_map.count(sourceBB->true_edge), "True BB should be a valid BB (BB" + STR(sourceBB->true_edge) +
                                                       " from BB" + STR(sourceBB->number) + ")");
   THROW_ASSERT(bb_map.count(sourceBB->false_edge), "False BB should be a valid BB (BB" + STR(sourceBB->true_edge) +
                                                        " from BB" + STR(sourceBB->number) + ")");
   const auto TrueBB = bb_map.at(sourceBB->true_edge);
   const auto FalseBB = bb_map.at(sourceBB->false_edge);

   if(tree_helper::IsConstant(gc->op0))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Branch variable is a cst_node, skipping...");
      return;
   }
   THROW_ASSERT(GET_CONST_NODE(gc->op0)->get_kind() == ssa_name_K, "Non SSA variable found in branch (" +
                                                                       GET_CONST_NODE(gc->op0)->get_kind_text() + " " +
                                                                       GET_CONST_NODE(gc->op0)->ToString() + ")");
   const auto Cond = range_analysis::branchOpRecurse(gc->op0);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "Branch condition is " + GET_CONST_NODE(Cond)->get_kind_text() + " " + STR(Cond));

   const auto InsertPredicate = [&](VarNode* Op, blocRef targetBB, const ValueRangeRef& intersect) {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Conditional intersect " + intersect->ToString() + " added for variable " + STR(Op->getValue()) +
                         " in BB" + STR(targetBB->number));

      PredicateBase* PB =
          new PredicateWithEdge(gimple_cond_K, Op->getValue(), sourceBB->number, targetBB->number, intersect);
      // TODO: not sure if gimple_cond statement is the correct user to be set in the following VarUse, since it is not
      // actually using Op
      addInfoFor(VarUseRef(new VarUse(Op, tn)), PB);
      if(targetBB->list_of_pred.size() > 1)
      {
         _infos.EdgeUsesOnly.insert({sourceBB->number, targetBB->number});
      }
   };

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   if(const auto be = GetPointer<const binary_expr>(GET_CONST_NODE(Cond)))
   {
      if(!range_analysis::isCompare(be))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not a compare condition, skipping...");
         return;
      }
      if(!range_analysis::isValidType(be->op0) || !range_analysis::isValidType(be->op1))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Non-integer operands, skipping...");
         return;
      }

      // We have a Variable-Constant comparison.
      const auto Op0 = GET_CONST_NODE(be->op0);
      const auto Op1 = GET_CONST_NODE(be->op1);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Op0 is " + Op0->get_kind_text() + " and Op1 is " + Op1->get_kind_text());

#if !defined(NDEBUG) || HAVE_ASSERTS
      const auto bw0 = tree_helper::TypeSize(be->op0);
#endif
#if HAVE_ASSERTS
      const auto bw1 = tree_helper::TypeSize(be->op1);
      THROW_ASSERT(bw0 == bw1, "Operands of same operation have different bitwidth (Op0 = " + STR(bw0) +
                                   ", Op1 = " + STR(bw1) + ").");
#endif

      // If both operands are constants, nothing to do here
      if(tree_helper::IsConstant(Op0) && tree_helper::IsConstant(Op1))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         return;
      }

      const auto var0 = _nc->addVarNode(be->op0, _function_id);
      const auto var1 = _nc->addVarNode(be->op1, _function_id);

      const auto [variable, constant] = [&]() -> std::tuple<VarNode*, VarNode*> {
         if(tree_helper::IsConstant(Op0))
         {
            return {var1, var0};
         }
         else if(tree_helper::IsConstant(Op1))
         {
            return {var0, var1};
         }
         return {nullptr, nullptr};
      }();

      // Then there are two cases: variable being compared to a constant,
      // or variable being compared to another variable
      if(constant != nullptr)
      {
         const kind pred = range_analysis::isSignedType(variable->getValue()) ?
                               be->get_kind() :
                               range_analysis::op_unsigned(be->get_kind());
         const kind swappred = range_analysis::op_swap(pred);
         const auto CR = tree_helper::Range(constant->getValue());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Variable bitwidth is " + STR(tree_helper::TypeSize(variable->getValue())) +
                            " and constant value is " + constant->getValue()->ToString());

         auto TValues = variable == var0 ? range_analysis::makeSatisfyingCmpRegion(pred, CR) :
                                           range_analysis::makeSatisfyingCmpRegion(swappred, CR);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Condition is true on " + TValues->ToString());
         auto FValues = TValues->isFullSet() ? tree_helper::TypeRange(variable->getValue(), Empty) : TValues->getAnti();

         // TODO: not clear why the following should be true (clang often converts gt/lt into eq/ne, thus the following
         // would invalidate most conditional statements).
         // When dealing with eq/ne conditions it is safer to propagate only the constant branch value if(be->get_kind()
         // == eq_expr_K)
         // {
         //    FValues = tree_helper::TypeRange(variable->getValue(), Regular);
         // }
         // else if(be->get_kind() == ne_expr_K)
         // {
         //    TValues = tree_helper::TypeRange(variable->getValue(), Regular);
         // }

         // Create the interval using the intersection in the branch.
         InsertPredicate(variable, TrueBB, ValueRangeRef(new ValueRange(TValues)));
         InsertPredicate(variable, FalseBB, ValueRangeRef(new ValueRange(FValues)));

         // Do the same for the operand of variable (if variable is a cast instruction)
         if(const auto* Var = GetPointer<const ssa_name>(GET_CONST_NODE(variable->getValue())))
         {
            const auto* VDef = GetPointer<const gimple_assign>(GET_CONST_NODE(Var->CGetDefStmt()));
            if(VDef && (GET_CONST_NODE(VDef->op1)->get_kind() == nop_expr_K ||
                        GET_CONST_NODE(VDef->op1)->get_kind() == convert_expr_K))
            {
               const auto* cast_inst = GetPointer<const unary_expr>(GET_CONST_NODE(VDef->op1));
#ifndef NDEBUG
               if(variable == var0)
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
               const auto cast_var = _nc->addVarNode(cast_inst->op, _function_id);
               InsertPredicate(cast_var, TrueBB, ValueRangeRef(new ValueRange(TValues)));
               InsertPredicate(cast_var, FalseBB, ValueRangeRef(new ValueRange(FValues)));
            }
         }
      }
      else
      {
         const kind pred =
             range_analysis::isSignedType(be->op0) ? be->get_kind() : range_analysis::op_unsigned(be->get_kind());
         const kind invPred = range_analysis::op_inv(pred);
         const kind swappred = range_analysis::op_swap(pred);
         const kind invSwappred = range_analysis::op_inv(swappred);

         const auto CR = tree_helper::TypeRange(be->op0, Unknown);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variables bitwidth is " + STR(bw0));

         // Symbolic intervals for op0
         InsertPredicate(var0, TrueBB, ValueRangeRef(new SymbRange(CR, var1, pred)));
         InsertPredicate(var0, FalseBB, ValueRangeRef(new SymbRange(CR, var1, invPred)));

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

               const auto cast_var = _nc->addVarNode(cast_inst->op, _function_id);
               InsertPredicate(cast_var, TrueBB, ValueRangeRef(new SymbRange(CR, var1, pred)));
               InsertPredicate(cast_var, FalseBB, ValueRangeRef(new SymbRange(CR, var1, invPred)));
            }
         }

         // Symbolic intervals for op1
         InsertPredicate(var1, TrueBB, ValueRangeRef(new SymbRange(CR, var0, swappred)));
         InsertPredicate(var1, FalseBB, ValueRangeRef(new SymbRange(CR, var0, invSwappred)));

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

               const auto cast_var = _nc->addVarNode(cast_inst->op, _function_id);
               InsertPredicate(cast_var, TrueBB, ValueRangeRef(new SymbRange(CR, var0, swappred)));
               InsertPredicate(cast_var, FalseBB, ValueRangeRef(new SymbRange(CR, var0, invSwappred)));
            }
         }
      }
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Unhandled condition type, skipping... (" + GET_CONST_NODE(Cond)->get_kind_text() + " " +
                         STR(Cond) + ")");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
}

void IRVisitor::processMultiWayIf(tree_nodeConstRef tn)
{
   const auto* gmw = GetPointer<const gimple_multi_way_if>(GET_CONST_NODE(tn));
   THROW_ASSERT(gmw, "Multi way if instruction should be gimple_multi_way_if");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "Multi-way if with " + STR(gmw->list_of_cond.size()) + " conditions");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");

   const auto sourceBBI = gmw->bb_index;
   const auto InsertPredicate = [&](VarNode* Op, blocRef targetBB, const ValueRangeRef& intersect) {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Conditional intersect " + intersect->ToString() + " added for variable " + STR(Op->getValue()) +
                         " in BB" + STR(targetBB->number));

      PredicateBase* PB = new PredicateWithEdge(gimple_cond_K, Op->getValue(), sourceBBI, targetBB->number, intersect);
      // TODO: not sure if gimple_cond statement is the correct user to be set in the following VarUse, since it is not
      // actually using Op
      addInfoFor(VarUseRef(new VarUse(Op, tn)), PB);
      if(targetBB->list_of_pred.size() > 1)
      {
         _infos.EdgeUsesOnly.insert({sourceBBI, targetBB->number});
      }
   };

   for(const auto& [cond, targetBBI] : gmw->list_of_cond)
   {
      if(!cond)
      {
         // Default branch is handled at the end
         continue;
      }
      // if(targetBBI == sourceBBI)
      // {
      //    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
      //                   "Branch loopback detected: variable renaming not safe, skipping...");
      //    continue;
      // }
      if(tree_helper::IsConstant(cond))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch variable is a cst_node, skipping...");
         continue;
      }

      THROW_ASSERT(GET_CONST_NODE(cond)->get_kind() == ssa_name_K, "Case conditional variable should be an ssa_name (" +
                                                                       GET_CONST_NODE(cond)->get_kind_text() + " " +
                                                                       GET_CONST_NODE(cond)->ToString() + ")");
      const auto Cond = range_analysis::branchOpRecurse(cond);

      if(const auto be = GetPointer<const binary_expr>(GET_CONST_NODE(Cond)))
      {
         if(!range_analysis::isCompare(be))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Not a compare condition, skipping...");
            continue;
         }

         if(!range_analysis::isValidType(be->op0) || !range_analysis::isValidType(be->op1))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Non-integer operands, skipping...");
            continue;
         }

         // We have a Variable-Constant comparison.
         const auto Op0 = GET_CONST_NODE(be->op0);
         const auto Op1 = GET_CONST_NODE(be->op1);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Op0 is " + Op0->get_kind_text() + " and Op1 is " + Op1->get_kind_text());

#if !defined(NDEBUG) || HAVE_ASSERTS
         const auto bw0 = tree_helper::TypeSize(be->op0);
#endif
#if HAVE_ASSERTS
         const auto bw1 = tree_helper::TypeSize(be->op1);
         THROW_ASSERT(bw0 == bw1, "Operands of same operation have different bitwidth (Op0 = " + STR(bw0) +
                                      ", Op1 = " + STR(bw1) + ").");
#endif

         // If both operands are constants, nothing to do here
         if(tree_helper::IsConstant(Op0) && tree_helper::IsConstant(Op1))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Both operands are constants, dead code elimination necessary!");
            // TODO: abort and call dead code elimination to evaluate constant condition
            //    return true;
            continue;
         }

         const auto& targetBB = bb_map.at(targetBBI);

         const auto var0 = _nc->addVarNode(be->op0, _function_id);
         const auto var1 = _nc->addVarNode(be->op1, _function_id);

         const auto [variable, constant] = [&]() -> std::tuple<VarNode*, VarNode*> {
            if(tree_helper::IsConstant(Op0))
            {
               return {var1, var0};
            }
            else if(tree_helper::IsConstant(Op1))
            {
               return {var0, var1};
            }
            return {nullptr, nullptr};
         }();

         if(constant != nullptr)
         {
            const kind pred = range_analysis::isSignedType(variable->getValue()) ?
                                  be->get_kind() :
                                  range_analysis::op_unsigned(be->get_kind());
            const kind swappred = range_analysis::op_swap(pred);
            const auto CR = tree_helper::Range(constant->getValue());
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Variable bitwidth is " + STR(tree_helper::TypeSize(variable->getValue())) +
                               " and constant value is " + constant->getValue()->ToString());

            const auto TValues = variable == var0 ? range_analysis::makeSatisfyingCmpRegion(pred, CR) :
                                                    range_analysis::makeSatisfyingCmpRegion(swappred, CR);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Condition is true on " + TValues->ToString());

            InsertPredicate(variable, targetBB, ValueRangeRef(new ValueRange(TValues)));

            // Do the same for the operand of variable (if variable is a cast instruction)
            if(const auto* Var = GetPointer<const ssa_name>(GET_CONST_NODE(variable->getValue())))
            {
               const auto* VDef = GetPointer<const gimple_assign>(GET_CONST_NODE(Var->CGetDefStmt()));
               if(VDef && (GET_CONST_NODE(VDef->op1)->get_kind() == nop_expr_K ||
                           GET_CONST_NODE(VDef->op1)->get_kind() == convert_expr_K))
               {
                  const auto* cast_inst = GetPointer<const unary_expr>(GET_CONST_NODE(VDef->op1));
#ifndef NDEBUG
                  if(variable == var0)
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
                  const auto cast_var = _nc->addVarNode(cast_inst->op, _function_id);
                  InsertPredicate(cast_var, targetBB, ValueRangeRef(new ValueRange(TValues)));
               }
            }
         }
         else
         {
            const kind pred =
                range_analysis::isSignedType(be->op0) ? be->get_kind() : range_analysis::op_unsigned(be->get_kind());
            const kind swappred = range_analysis::op_swap(pred);

            const auto CR = tree_helper::TypeRange(be->op0, Unknown);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variables bitwidth is " + STR(bw0));

            InsertPredicate(var0, targetBB, ValueRangeRef(new SymbRange(CR, var1, pred)));

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

                  const auto cast_var = _nc->addVarNode(cast_inst->op, _function_id);
                  InsertPredicate(cast_var, targetBB, ValueRangeRef(new SymbRange(CR, var1, pred)));
               }
            }

            InsertPredicate(var1, targetBB, ValueRangeRef(new SymbRange(CR, var0, swappred)));

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

                  const auto cast_var = _nc->addVarNode(cast_inst->op, _function_id);
                  InsertPredicate(cast_var, targetBB, ValueRangeRef(new SymbRange(CR, var0, swappred)));
               }
            }
         }
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "Multi-way-if condition different from binary_expr not handled, skipping... (" +
                            GET_CONST_NODE(Cond)->get_kind_text() + " " + STR(Cond) + ")");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
}

// Perform a strict weak ordering on instructions and arguments.
static bool valueComesBefore(OrderedInstructions& OI, tree_nodeConstRef A, tree_nodeConstRef B)
{
   THROW_ASSERT(A, "A is nullptr");
   THROW_ASSERT(GetPointer<const gimple_node>(GET_CONST_NODE(A)),
                "A is not a gimple_node: " + GET_CONST_NODE(A)->get_kind_text() + " " + GET_CONST_NODE(A)->ToString());
   THROW_ASSERT(B, "B is nullptr");
   THROW_ASSERT(GetPointer<const gimple_node>(GET_CONST_NODE(B)),
                "B is not a gimple_node: " + GET_CONST_NODE(B)->get_kind_text() + " " + GET_CONST_NODE(B)->ToString());
   return OI.dominates(GetPointer<const gimple_node>(GET_CONST_NODE(A)),
                       GetPointer<const gimple_node>(GET_CONST_NODE(B)));
}

// Given a predicate info that is a type of branching terminator, get the
// edge this predicate info represents
static const std::pair<unsigned int, unsigned int> getBlockEdge(const PredicateBase* PB)
{
   THROW_ASSERT(PredicateWithEdge::classof(PB), "Not a predicate info type we know how to get an edge from.");
   const auto* PEdge = static_cast<const PredicateWithEdge*>(PB);
   return std::make_pair(PEdge->From, PEdge->To);
}

enum LocalNum
{
   // Operations that must appear first in the block.
   LN_First,
   // Operations that are somewhere in the middle of the block, and are sorted on
   // demand.
   LN_Middle,
   // Operations that must appear last in a block, like successor phi node uses.
   LN_Last
};

// Associate global and local DFS info with defs and uses, so we can sort them
// into a global domination ordering.
struct ValueDFS
{
   unsigned int DFSIn = 0;
   unsigned int DFSOut = 0;
   unsigned int LocalNum = LN_Middle;
   // Only one of Def or Use will be set.
   OpNode* Def = nullptr;
   VarUseRef U = nullptr;
   // Neither PInfo nor EdgeOnly participate in the ordering
   PredicateBase* PInfo = nullptr;
   bool EdgeOnly = false;

   std::string ToString() const
   {
      return "Predicate info: " + (PInfo ? PInfo->OriginalOp->ToString() : "null") +
             " Def: " + (Def ? Def->ToString() : "null") + " Use: " + (U ? U->getUser()->ToString() : "null") +
             " DFS: (" + STR(DFSIn) + ", " + STR(DFSOut) + ", " +
             (LocalNum == LN_First ? "first" : (LocalNum == LN_Middle ? "middle" : "last")) +
             ") EdgeOnly: " + (EdgeOnly ? "true" : "false");
   }
};

// This compares ValueDFS structures, creating OrderedBasicBlocks where
// necessary to compare uses/defs in the same block.  Doing so allows us to walk
// the minimum number of instructions necessary to compute our def/use ordering.
struct ValueDFS_Compare
{
   OrderedInstructions& OI;
   explicit ValueDFS_Compare(OrderedInstructions& _OI) : OI(_OI)
   {
   }

   // For a phi use, or a non-materialized def, return the edge it represents.
   const std::pair<unsigned int, unsigned int> getBlockEdge_local(const ValueDFS& VD) const
   {
      if(!VD.Def && VD.U)
      {
         const auto PHI = GetPointer<const gimple_phi>(GET_CONST_NODE(VD.U->getInstruction()));
         auto phiDefEdge = std::find_if(
             PHI->CGetDefEdgesList().begin(), PHI->CGetDefEdgesList().end(), [&](const gimple_phi::DefEdge& de) {
                return GET_INDEX_CONST_NODE(de.first) == GET_INDEX_CONST_NODE(VD.U->getOperand()->getValue());
             });
         THROW_ASSERT(phiDefEdge != PHI->CGetDefEdgesList().end(), "Unable to find variable in phi definitions");
         return std::make_pair(phiDefEdge->second, PHI->bb_index);
      }
      // This is really a non-materialized def.
      return getBlockEdge(VD.PInfo);
   }

   // Get the definition of an instruction that occurs in the middle of a block.
   tree_nodeConstRef getMiddleDef(const ValueDFS& VD) const
   {
      if(VD.Def)
      {
         return VD.Def->getInstruction();
      }
      return nullptr;
   }

   // Return either the Def, if it's not null, or the user of the Use, if the def
   // is null.
   tree_nodeConstRef getDefOrUser(const tree_nodeConstRef Def, const VarUseRef U) const
   {
      return Def ? Def : U->getInstruction();
   }

   // This performs the necessary local basic block ordering checks to tell
   // whether A comes before B, where both are in the same basic block.
   bool localComesBefore(const ValueDFS& A, const ValueDFS& B) const
   {
      auto ADef = getMiddleDef(A);
      auto BDef = getMiddleDef(B);
      auto AInst = getDefOrUser(ADef, A.U);
      auto BInst = getDefOrUser(BDef, B.U);
      return valueComesBefore(OI, AInst, BInst);
   }

   bool operator()(const ValueDFS& A, const ValueDFS& B) const
   {
      if(&A == &B)
      {
         return false;
      }
      // The only case we can't directly compare them is when they in the same
      // block, and both have localnum == middle.  In that case, we have to use
      // comesbefore to see what the real ordering is, because they are in the
      // same basic block.

      const auto SameBlock = std::tie(A.DFSIn, A.DFSOut) == std::tie(B.DFSIn, B.DFSOut);

      // We want to put the def that will get used for a given set of phi uses,
      // before those phi uses.
      // So we sort by edge, then by def.
      // Note that only phi nodes uses and defs can come last.
      if(SameBlock && A.LocalNum == LN_Last && B.LocalNum == LN_Last)
      {
         const auto ABlockEdge = getBlockEdge_local(A);
         const auto BBlockEdge = getBlockEdge_local(B);
         // Now sort by block edge and then defs before uses.
         return std::tie(ABlockEdge, A.Def, A.U) < std::tie(BBlockEdge, B.Def, B.U);
      }

      if(!SameBlock || A.LocalNum != LN_Middle || B.LocalNum != LN_Middle)
      {
         return std::tie(A.DFSIn, A.DFSOut, A.LocalNum, A.Def, A.U) <
                std::tie(B.DFSIn, B.DFSOut, B.LocalNum, B.Def, B.U);
      }
      return localComesBefore(A, B);
   }
};
using ValueDFSStack = std::vector<ValueDFS>;

static bool stackIsInScope(const ValueDFSStack& Stack, const ValueDFS& VDUse, const OrderedInstructions& OI)
{
   if(Stack.empty())
   {
      return false;
   }
   // If it's a phi only use, make sure it's for this phi node edge, and that the
   // use is in a phi node.  If it's anything else, and the top of the stack is
   // EdgeOnly, we need to pop the stack.  We deliberately sort phi uses next to
   // the defs they must go with so that we can know it's time to pop the stack
   // when we hit the end of the phi uses for a given def.
   if(Stack.back().EdgeOnly)
   {
      if(!VDUse.U)
      {
         return false;
      }
      const auto PHI = GetPointer<const gimple_phi>(GET_CONST_NODE(VDUse.U->getInstruction()));
      if(!PHI)
      {
         return false;
      }
      // Check edge
      auto EdgePredIt = std::find_if(
          PHI->CGetDefEdgesList().begin(), PHI->CGetDefEdgesList().end(), [&](const gimple_phi::DefEdge& de) {
             return GET_INDEX_CONST_NODE(de.first) == GET_INDEX_CONST_NODE(VDUse.U->getOperand()->getValue());
          });
      if(EdgePredIt->second != getBranchBlock(Stack.back().PInfo))
      {
         return false;
      }

      const auto bbedge = getBlockEdge(Stack.back().PInfo);
      if(PHI->bb_index == bbedge.second && EdgePredIt->second == bbedge.first)
      {
         return true;
      }
      return OI.dominates(bbedge.second, EdgePredIt->second);
   }

   return (VDUse.DFSIn >= Stack.back().DFSIn && VDUse.DFSOut <= Stack.back().DFSOut);
}

static void popStackUntilDFSScope(ValueDFSStack& Stack, const ValueDFS& VD, const OrderedInstructions& OI)
{
   while(!Stack.empty() && !stackIsInScope(Stack, VD, OI))
   {
      Stack.pop_back();
   }
}

// Convert the uses of Op into a vector of uses, associating global and local
// DFS info with each one.
static void convertUsesToDFSOrdered(VarNode* Op, const OpNodes& uses, std::vector<ValueDFS>& DFSOrderedSet,
                                    BBGraphRef DT, const RenameInfos::DFSInfoMap& DFSInfos,
                                    int
#ifndef NDEBUG
                                        debug_level
#endif
)
{
   const auto& BBmap = DT->CGetBBGraphInfo()->bb_index_map;
   const auto dfs_gen = [&](OpNode* user, unsigned int stmt_bbi, LocalNum ln) {
      ValueDFS VD;
      THROW_ASSERT(BBmap.find(stmt_bbi) != BBmap.end(), "BB" + STR(stmt_bbi) + " not found in DT");
      if(DT->IsReachable(BBmap.at(bloc::ENTRY_BLOCK_ID), BBmap.at(stmt_bbi)))
      {
         const auto& DomNode_DFSInfo = DFSInfos.at(stmt_bbi);
         VD.DFSIn = DomNode_DFSInfo.DFSIn;
         VD.DFSOut = DomNode_DFSInfo.DFSOut;
         VD.LocalNum = ln;
         VD.U = VarUseRef(new VarUse(Op, user));
         DFSOrderedSet.push_back(VD);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Pushed on renaming stack");
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---BB" + STR(stmt_bbi) + " is unreachable from DT root");
      }
   };

   const auto op = GetPointer<const ssa_name>(GET_CONST_NODE(Op->getValue()));
   THROW_ASSERT(op, "Op is not an ssa_name (" + GET_CONST_NODE(Op->getValue())->get_kind_text() + ")");
   const auto defBBI = GetPointer<const gimple_node>(GET_CONST_NODE(op->CGetDefStmt()))->bb_index;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   for(const auto userOp : uses)
   {
      const auto& user = userOp->getInstruction();
      if(!user)
      {
         // This is a materialized Sigma operation without a relative IR statement
         // TODO: this use should be also added to DFSOrderedSet with the DFSInfo relative to the basic block where it
         // was previously materialized
         THROW_ASSERT(GetOp<SigmaOpNode>(userOp), "");
         // ValueDFS VD;
         // const auto& DomNode_DFSInfo = DFSInfos.at(stmt_bbi);
         // VD.DFSIn = DomNode_DFSInfo.DFSIn;
         // VD.DFSOut = DomNode_DFSInfo.DFSOut;
         // VD.LocalNum = LN_Last;
         // VD.U = VarUseRef(new VarUse(Op, userOp));
         // DFSOrderedSet.push_back(VD);
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Checking " + user->ToString());
      const auto gn = GetPointer<const gimple_node>(GET_CONST_NODE(user));
      THROW_ASSERT(gn, "Use statement should be a gimple_node");
      if(gn->get_kind() == gimple_phi_K)
      {
         const auto gp = GetPointerS<const gimple_phi>(GET_CONST_NODE(user));
         if(gp->CGetDefEdgesList().size() == 1)
         {
            // Sigma uses not intresting (already e-SSA)
            continue;
         }
         for(const auto& [def, source_bbi] : gp->CGetDefEdgesList())
         {
            if(GET_INDEX_CONST_NODE(def) == GET_INDEX_CONST_NODE(Op->getValue()))
            {
               dfs_gen(userOp, source_bbi, LN_Last);
            }
         }
      }
      else
      {
         if(gn->bb_index == defBBI)
         {
            // Uses within the same basic block not interesting (they are casts or the actual branch eveluating the
            // condition)
            continue;
         }
         dfs_gen(userOp, gn->bb_index, LN_Middle);
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
}

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

   // Given the renaming stack, make all the operands currently on the stack real
   // by inserting them into the IR.  Return the last operation's value.
   OpNode* materializeStack(ValueDFSStack& RenameStack, unsigned int function_id, VarNode* OrigOp)
   {
      // Find the first thing we have to materialize
      auto RevIter = RenameStack.rbegin();
      for(; RevIter != RenameStack.rend(); ++RevIter)
      {
         if(RevIter->Def)
         {
            break;
         }
      }

      auto Start = RevIter - RenameStack.rbegin();
      // The maximum number of things we should be trying to materialize at once
      // right now is 4, depending on if we had an assume, a branch, and both used
      // and of conditions.
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(auto RenameIter = RenameStack.end() - Start; RenameIter != RenameStack.end(); ++RenameIter)
      {
         auto Op = OrigOp;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Checking variable " + Op->ToString());
         if(RenameIter != RenameStack.begin())
         {
            THROW_ASSERT((RenameIter - 1)->Def, "A valid definition shold be on the stack at this point");
            const auto sigmaOp = GetOp<SigmaOpNode>((RenameIter - 1)->Def);
            THROW_ASSERT(sigmaOp, "Previous definition on stack should be a SigmaOpNode (" +
                                      (RenameIter - 1)->Def->ToString() + ")");
            Op = sigmaOp->getSink();
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Moving check to " + Op->ToString());
         }
         ValueDFS& Result = *RenameIter;
         const auto* ValInfo = Result.PInfo;
         // For edge predicates, we can just place the operand in the block before
         // the terminator.  For assume, we have to place it right before the assume
         // to ensure we dominate all of our uses.  Always insert right before the
         // relevant instruction (terminator, assume), so that we insert in proper
         // order in the case of multiple predicateinfo in the same block.
         if(PredicateWithEdge::classof(ValInfo))
         {
            const auto pwe = static_cast<const PredicateWithEdge*>(ValInfo);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Inserting sigma in BB" + STR(pwe->To) + " with intersect " + pwe->intersect->ToString());

            const auto sink = addVarNode(Op->getValue(), function_id, pwe->To);
            THROW_ASSERT(sink->getId() != Op->getId(), "unexpected condition");
            Result.Def = pushOperation(new SigmaOpNode(pwe->intersect, sink, Op, nullptr, gimple_phi_K));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Materialized " + Result.Def->ToString());
         }
         else
         {
            THROW_UNREACHABLE("Invalid PredicateInfo type");
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      return RenameStack.back().Def;
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

   void buildGraph(unsigned int function_id)
   {
      const auto TM = AppM->get_tree_manager();
      const auto FB = AppM->CGetFunctionBehavior(function_id);
      const auto fd = GetPointer<const function_decl>(TM->CGetTreeNode(function_id));
      const auto sl = GetPointer<const statement_list>(GET_CONST_NODE(fd->body));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Analysing function " + tree_helper::GetMangledFunctionName(fd) + " with " +
                         STR(sl->list_of_bloc.size()) + " blocks");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");

      BBGraphsCollectionRef bbgc(
          new BBGraphsCollection(BBGraphInfoRef(new BBGraphInfo(AppM, function_id)), AppM->get_parameter()));
      BBGraphRef dt(new BBGraph(bbgc, D_SELECTOR));

      compute_dominator_tree(dt, sl->list_of_bloc, bbgc, AppM->get_parameter());

      RenameInfos infos;

      {
         const auto entryVertex = dt->GetBBGraphInfo()->bb_index_map.at(bloc::ENTRY_BLOCK_ID);
         IRVisitor bv(infos, this, function_id, AppM,
#ifndef NDEBUG
                      debug_level
#else
                      0
#endif
         );
         std::vector<boost::default_color_type> color_vec(boost::num_vertices(*dt), boost::white_color);
         boost::depth_first_visit(*dt, entryVertex, bv,
                                  boost::make_iterator_property_map(
                                      color_vec.begin(), boost::get(boost::vertex_index, *dt), boost::white_color));
      }

      THROW_ASSERT(static_cast<size_t>(infos.DFSInfos.size()) == boost::num_vertices(*dt),
                   "Discovered " + STR(infos.DFSInfos.size()) + "/" + STR(boost::num_vertices(*dt)) + " vertices.");

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Analysis detected " + STR(infos.OpsToRename.size()) + " operations to rename");

      if(infos.OpsToRename.size())
      {
         auto& DFSInfos = infos.DFSInfos;
         CustomMap<std::pair<unsigned int, unsigned int>, blocRef> interBranchBBs;

         // Sort OpsToRename since we are going to iterate it.
         std::vector<VarUseRef> OpsToRename(infos.OpsToRename.begin(), infos.OpsToRename.end());
         for(const auto& vuse : OpsToRename)
         {
            THROW_ASSERT(vuse->getInstruction(),
                         "Missing instruction for use of " + STR(vuse->getOperand()->getValue()));
         }
         OrderedInstructions OI(dt);
         auto Comparator = [&](const VarUseRef A, const VarUseRef B) {
            return valueComesBefore(OI, A->getInstruction(), B->getInstruction());
         };
         std::sort(OpsToRename.begin(), OpsToRename.end(), Comparator);
         ValueDFS_Compare Compare(OI);

         for(auto& Op : OpsToRename)
         {
            std::vector<ValueDFS> OrderedUses;
            const auto& ValueInfo = infos.ValueInfos.at(Op->getOperand()->getId());
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Analysing " + Op->getOperand()->ToString() + " with " + STR(ValueInfo.Infos.size()) +
                               " possible copies");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
            // Insert the possible copies into the def/use list.
            // They will become real copies if we find a real use for them, and never
            // created otherwise.
            for(auto& PossibleCopy : ValueInfo.Infos)
            {
               ValueDFS VD{};
               if(PredicateWithEdge::classof(PossibleCopy))
               {
                  // If we can only do phi uses, we treat it like it's in the branch
                  // block, and handle it specially. We know that it goes last, and only
                  // dominate phi uses.
                  const auto BlockEdge = getBlockEdge(PossibleCopy);
                  if(infos.EdgeUsesOnly.count(BlockEdge))
                  {
                     // If we can only do phi uses, we treat it like it's in the branch
                     // block, and handle it specially. We know that it goes last, and only
                     // dominate phi uses.
                     VD.LocalNum = LN_Last;
                     const auto& DomNode = BlockEdge.first;
                     if(DomNode)
                     {
                        THROW_ASSERT(DFSInfos.contains(DomNode), "Invalid DT node");
                        const auto& DomNode_DFSInfo = DFSInfos.at(DomNode);
                        VD.DFSIn = DomNode_DFSInfo.DFSIn;
                        VD.DFSOut = DomNode_DFSInfo.DFSOut;
                        VD.PInfo = PossibleCopy;
                        VD.EdgeOnly = true;
                        OrderedUses.push_back(VD);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Possible copy: " + VD.ToString());
                     }
                  }
                  else
                  {
                     // Otherwise, we are in the split block (even though we perform
                     // insertion in the branch block).
                     // Insert a possible copy at the split block and before the branch.
                     VD.LocalNum = LN_First;
                     const auto& DomNode = BlockEdge.second;
                     if(DomNode)
                     {
                        THROW_ASSERT(DFSInfos.contains(DomNode), "Invalid DT node");
                        const auto& DomNode_DFSInfo = DFSInfos.at(DomNode);
                        VD.DFSIn = DomNode_DFSInfo.DFSIn;
                        VD.DFSOut = DomNode_DFSInfo.DFSOut;
                        VD.PInfo = PossibleCopy;
                        OrderedUses.push_back(VD);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Possible copy: " + VD.ToString());
                     }
                  }
               }
            }

            convertUsesToDFSOrdered(Op->getOperand(), getUses().at(Op->getOperand()->getId()), OrderedUses, dt,
                                    DFSInfos, debug_level);
            // Here we require a stable sort because we do not bother to try to
            // assign an order to the operands the uses represent. Thus, two
            // uses in the same instruction do not have a strict sort order
            // currently and will be considered equal. We could get rid of the
            // stable sort by creating one if we wanted.
            std::stable_sort(OrderedUses.begin(), OrderedUses.end(), Compare);
            std::vector<ValueDFS> RenameStack;
            // For each use, sorted into dfs order, push values and replaces uses with
            // top of stack, which will represent the reaching def.
            for(auto& VD : OrderedUses)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing " + VD.ToString());
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");

               // We currently do not materialize copy over copy, but we should decide if
               // we want to.
               bool PossibleCopy = VD.PInfo != nullptr;
#ifndef NDEBUG
               if(RenameStack.empty())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "RenameStack empty");
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "RenameStack top DFS numbers are (" + STR(RenameStack.back().DFSIn) + "," +
                                     STR(RenameStack.back().DFSOut) + ")");
               }
#endif
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "Current DFS numbers are (" + STR(VD.DFSIn) + "," + STR(VD.DFSOut) + ")");
               bool ShouldPush = (VD.Def || PossibleCopy);
               bool OutOfScope = !stackIsInScope(RenameStack, VD, OI);
               if(OutOfScope || ShouldPush)
               {
                  // Sync to our current scope.
                  popStackUntilDFSScope(RenameStack, VD, OI);
                  if(ShouldPush)
                  {
                     RenameStack.push_back(VD);
                  }
               }
               // If we get to this point, and the stack is empty we must have a use
               // with no renaming needed, just skip it.
               if(RenameStack.empty())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Current use needs no renaming");
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
                  continue;
               }
               // Skip values, only want to rename the uses
               if(VD.Def || PossibleCopy)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
                  continue;
               }

               ValueDFS& Result = RenameStack.back();
               THROW_ASSERT(VD.U, "A use should be in scope for current renaming operation");
#if HAVE_ASSERTS
               if(const auto gp = GetPointer<const gimple_phi>(GET_CONST_NODE(VD.U->getInstruction())))
               {
                  THROW_ASSERT(gp->CGetDefEdgesList().size() > 1, "Sigma operation should not be renamed (BB" +
                                                                      STR(gp->bb_index) + " " + gp->ToString() + ")");
               }
#endif

               // If the possible copy dominates something, materialize our stack up to
               // this point. This ensures every comparison that affects our operation
               // ends up with predicateinfo.
               if(!Result.Def)
               {
                  Result.Def = materializeStack(RenameStack, function_id, Op->getOperand());
               }

               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Found replacement " + Result.Def->ToString() + " for " +
                                  VD.U->getOperand()->ToString() + " in " + VD.U->getUser()->ToString());
               getUses().at(VD.U->getOperand()->getId()).erase(VD.U->getUser());
               VD.U->updateUse(Result.Def->getSink());
               getUses().at(Result.Def->getSink()->getId()).insert(VD.U->getUser());
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         }
      }

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }

   void findIntervals(
#ifndef NDEBUG
       const ParameterConstRef parameters, const std::string& step_name
#endif
   )
   {
      // Initializes the nodes and the use map structure.
      const auto& defs = getDefs();
      for(auto& [id, var] : getVarNodes())
      {
         var->init(!defs.count(id));
      }

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
            auto var = *component.begin();
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
      std::set<VarNode::key_type, VarNode::key_compare> visitedOps;

      // init the activeOps only with the op received
      activeOps.insert(op);

      while(!activeOps.empty())
      {
         const auto V = *activeOps.begin();
         activeOps.erase(V);
         const auto sinkId = V->getSink()->getId();

         // if the sink has been visited go to the next activeOps
         if(visitedOps.count(sinkId))
         {
            continue;
         }

         Meet::crop(V);
         visitedOps.insert(sinkId);

         // The use list.of sink
         const auto& L = compUseMap.at(sinkId);
         for(auto user : L)
         {
            activeOps.insert(user);
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
         const auto sink = CG->addVarNode(ssa_node, function_id);

         // Check for pragma mask directives user defined range
         const auto parm = GetPointerS<const parm_decl>(GET_CONST_NODE(pnode));
         auto phiOp = new PhiOpNode(sink, nullptr);
         if(parm->range)
         {
            sink->setRange(parm->range);
            phiOp->setIntersect(parm->range);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Range hints found in parameter declaration: " + parm->range->ToString());
         }
         else
         {
            sink->setRange(sink->getMaxRange());
         }
         parameters.push_back(ssa_node);
         matchers.push_back(phiOp);
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
                  returnVars.push_back(CG->addVarNode(gr->op, function_id));
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
            from = CG->addVarNode(args->at(i), caller_id);

            // Connect nodes
            matchers[i]->addSource(from);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");

         // Match return values when return type is stored from caller
         if(hasReturn && GET_CONST_NODE(call_stmt)->get_kind() != gimple_call_K)
         {
            // Add caller instruction to the CG (it receives the return value)
            to = CG->addVarNode(ret_var, caller_id);
            to->setRange(to->getMaxRange());

            auto* phiOp = new PhiOpNode(to, nullptr);
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
   const auto rb_funcs = AppM->CGetCallGraphManager()->GetReachedBodyFunctions();
   for(const auto& f : rb_funcs)
   {
      CG->buildGraph(f);
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameters and return value propagation...");
   for(const auto f : rb_funcs)
   {
      ParmAndRetValPropagation(f, AppM, CG, debug_level);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameters and return value propagation completed");

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
#endif
   {
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
   }

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

   const auto rbf = AppM->CGetCallGraphManager()->GetReachedBodyFunctions();
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
