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
 *              Copyright (C) 2004-2019 Politecnico di Milano
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
 * @file eSSA.cpp
 * @brief
 *
 * @author Michele Fiorito <michele2.fiorito@mail.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "eSSA.hpp"

///. include
#include "Parameter.hpp"

/// behavior includes
#include "application_manager.hpp"
#include "function_behavior.hpp"

#include "Dominance.hpp"

/// design_flows include
#include "design_flow_manager.hpp"

/// frontend_analysis
#include "application_frontend_flow_step.hpp"

/// tree include
#include "behavioral_helper.hpp"
#include "ext_tree_node.hpp"
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_reindex.hpp"

/// wrapper/treegcc include
#include "gcc_wrapper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

#include "Range.hpp"

#include "absl/memory/memory.h"

class OrderedBasicBlock
{
   /// Map a instruction to its position in a BasicBlock.
   CustomMap<const struct gimple_node*, unsigned> NumberedInsts;

   /// Keep track of last instruction inserted into \p NumberedInsts.
   /// It speeds up queries for uncached instructions by providing a start point
   /// for new queries in OrderedBasicBlock::comesBefore.
   std::list<tree_nodeRef>::const_iterator LastInstFound;

   /// The position/number to tag the next instruction to be found.
   unsigned NextInstPos;

   /// The source BasicBlock instruction list
   const std::list<tree_nodeRef>& BBInst;

   /// The source BasicBlock to map.
   const blocRef BB;

   /// Given no cached results, find if \p A comes before \p B in \p BB.
   /// Cache and number out instruction while walking \p BB.
   bool instComesBefore(const struct gimple_node* A, const struct gimple_node* B)
   {
      const struct gimple_node* Inst = nullptr;
      THROW_ASSERT(!(LastInstFound == BBInst.end() && NextInstPos != 0), "Instruction supposed to be in NumberedInsts");

      // Start the search with the instruction found in the last lookup round.
      auto II = BBInst.begin();
      auto IE = BBInst.end();
      if(LastInstFound != IE)
      {
         II = std::next(LastInstFound);
      }

      // Number all instructions up to the point where we find 'A' or 'B'.
      for(; II != IE; ++II)
      {
         Inst = GetPointer<const gimple_node>(GET_CONST_NODE(*II));
         NumberedInsts[Inst] = NextInstPos++;
         if(Inst == A || Inst == B)
            break;
      }

      THROW_ASSERT(II != IE, "Instruction not found?");
      THROW_ASSERT((Inst == A || Inst == B), "Should find A or B");
      LastInstFound = II;
      return Inst != B;
   }

 public:
   OrderedBasicBlock(const blocRef BasicB) : LastInstFound(BasicB->CGetStmtList().end()), NextInstPos(0), BBInst(BasicB->CGetStmtList()), BB(BasicB)
   {
      unsigned int phiPos = 0U;
      for(const auto& gp : BasicB->CGetPhiList())
      {
         NumberedInsts.insert({GetPointer<const gimple_node>(GET_CONST_NODE(gp)), phiPos++});
      }
   }

   /// Find out whether \p A dominates \p B, meaning whether \p A
   /// comes before \p B in \p BB. This is a simplification that considers
   /// cached instruction positions and ignores other basic blocks, being
   /// only relevant to compare relative instructions positions inside \p BB.
   /// Returns false for A == B.
   bool dominates(const struct gimple_node* A, const struct gimple_node* B)
   {
      THROW_ASSERT(A->bb_index == B->bb_index, "Instructions must be in the same basic block!");
      THROW_ASSERT(A->bb_index == BB->number, "Instructions must be in the tracked block!");

      // Phi statements always comes before non-phi statements
      if(A->get_kind() == gimple_phi_K && B->get_kind() != gimple_phi_K)
      {
         return true;
      }
      else if(A->get_kind() != gimple_phi_K && B->get_kind() == gimple_phi_K)
      {
         return false;
      }

      // First we lookup the instructions. If they don't exist, lookup will give us
      // back ::end(). If they both exist, we compare the numbers. Otherwise, if NA
      // exists and NB doesn't, it means NA must come before NB because we would
      // have numbered NB as well if it didn't. The same is true for NB. If it
      // exists, but NA does not, NA must come after it. If neither exist, we need
      // to number the block and cache the results instComesBefore.
      const auto NAI = NumberedInsts.find(A);
      const auto NBI = NumberedInsts.find(B);
      if(NAI != NumberedInsts.end() && NBI != NumberedInsts.end())
      {
         return NAI->second < NBI->second;
      }
      if(NAI != NumberedInsts.end())
      {
         return B->get_kind() != gimple_phi_K; // Not found phi nodes have been just added from this step in front of all other phis
      }
      if(NBI != NumberedInsts.end())
      {
         return false;
      }
      THROW_ASSERT(A->get_kind() != gimple_phi_K, "Non dato, not given, nicht gegeben, pas donné, no dado, non detur");

      return instComesBefore(A, B);
   }
};

class OrderedInstructions
{
   /// Used to check dominance for instructions in same basic block.
   mutable CustomMap<unsigned int, std::unique_ptr<OrderedBasicBlock>> OBBMap;

   /// The dominator tree of the parent function.
   const BBGraphConstRef DT;

 public:
   /// Constructor.
   OrderedInstructions(BBGraphConstRef _DT) : DT(_DT)
   {
   }

   bool dominates(const unsigned int BBIA, const unsigned int BBIB) const
   {
      const auto& BBmap = DT->CGetBBGraphInfo()->bb_index_map;

      const auto BBAi_v = BBmap.find(BBIA);
      const auto BBBi_v = BBmap.find(BBIB);

      if(BBIA == BBIB)
      {
         return true;
      }

      if(BBAi_v->second == BBBi_v->second)
      {
         // Intermediate BB shadowing incoming BB
         const auto& b_pred = DT->CGetBBNodeInfo(BBBi_v->second)->block->list_of_pred;
         return std::find(b_pred.begin(), b_pred.end(), BBIA) != b_pred.end();
      }
      THROW_ASSERT(BBAi_v != BBmap.end(), "Unknown BB index (" + STR(BBIA) + ")");
      THROW_ASSERT(BBBi_v != BBmap.end(), "Unknown BB index (" + STR(BBIB) + ")");

      // An unreachable block is dominated by anything.
      if(!DT->IsReachable(BBmap.at(bloc::ENTRY_BLOCK_ID), BBBi_v->second))
      {
         return true;
      }

      // And dominates nothing.
      if(!DT->IsReachable(BBmap.at(bloc::ENTRY_BLOCK_ID), BBAi_v->second))
      {
         return false;
      }

      /// When block B is reachable from block A in the DT, A dominates B
      /// This because the DT used is a tree composed by immediate dominators only
      if(DT->IsReachable(BBAi_v->second, BBBi_v->second))
      {
         return true;
      }

      return false;
   }

   /// Return true if first instruction dominates the second.
   bool dominates(const struct gimple_node* InstA, const struct gimple_node* InstB) const
   {
      THROW_ASSERT(InstA, "Instruction A cannot be null");
      THROW_ASSERT(InstB, "Instruction B cannot be null");

      const auto BBIA = InstA->bb_index;
      const auto BBIB = InstB->bb_index;

      // Use ordered basic block to do dominance check in case the 2 instructions
      // are in the same basic block.
      if(BBIA == BBIB)
      {
         auto OBB = OBBMap.find(BBIA);
         if(OBB == OBBMap.end())
         {
            const auto BBmap = DT->CGetBBGraphInfo()->bb_index_map;
            const auto BBvertex = BBmap.find(BBIA);
            THROW_ASSERT(BBvertex != BBmap.end(), "Unknown BB index (" + STR(BBIA) + ")");

            const auto BB = DT->CGetBBNodeInfo(BBvertex->second)->block;
            THROW_ASSERT(BB->number == BBIA, "Intermediate BB not allowed here"); // Intermediate BB shadows its incoming BB, thus its index is different from associated vertex
            OBB = OBBMap.insert({BBIA, absl::make_unique<OrderedBasicBlock>(BB)}).first;
         }
         return OBB->second->dominates(InstA, InstB);
      }

      return dominates(BBIA, BBIB);
   }

   /// Invalidate the OrderedBasicBlock cache when its basic block changes.
   /// i.e. If an instruction is deleted or added to the basic block, the user
   /// should call this function to invalidate the OrderedBasicBlock cache for
   /// this basic block.
   void invalidateBlock(unsigned int BBI)
   {
      OBBMap.erase(BBI);
   }

   const BBGraphConstRef& getDT() const
   {
      return DT;
   }
};

class Operand
{
   tree_nodeRef operand;

   const tree_nodeRef user_stmt;

 public:
   Operand(const tree_nodeRef _operand, tree_nodeRef stmt) : operand(_operand), user_stmt(stmt)
   {
      THROW_ASSERT(GetPointer<gimple_node>(GET_NODE(stmt)), "stmt should be a gimple_node");
   }

   const tree_nodeRef getOperand() const
   {
      return operand;
   }

   const tree_nodeRef getUser() const
   {
      return user_stmt;
   }

   bool set(tree_nodeRef new_ssa, tree_managerRef TM)
   {
      const auto* ssaOperand = GetPointer<const ssa_name>(GET_CONST_NODE(operand));
      THROW_ASSERT(GET_NODE(new_ssa)->get_kind() == ssa_name_K, "New variable should be an ssa_name");
      THROW_ASSERT(ssaOperand, "Old variable should be an ssa_name");
      THROW_ASSERT(TM, "Null reference to tree manager");

      if(auto* gp = GetPointer<gimple_phi>(GET_NODE(user_stmt)))
      {
         const auto& deList = gp->CGetDefEdgesList();
         std::vector<gimple_phi::DefEdge> validDE;
         for(const auto& de : deList)
         {
            if(GET_INDEX_CONST_NODE(de.first) == ssaOperand->index)
            {
               validDE.push_back(de);
            }
         }
         THROW_ASSERT(static_cast<bool>(validDE.size()), "Required variable to be replaced not found");
         if(validDE.size() > 1)
         {
            const auto newSSADefBBI = GetPointer<const gimple_node>(GET_CONST_NODE(GetPointer<const ssa_name>(GET_CONST_NODE(new_ssa))->CGetDefStmt()))->bb_index;
            // Replace only correct DefEdge in the list
            for(const auto& de : validDE)
            {
               if(de.second == newSSADefBBI)
               {
                  gp->ReplaceDefEdge(TM, de, {new_ssa, newSSADefBBI});
                  bool emptyUses = ssaOperand->CGetUseStmts().empty();
                  operand = new_ssa;
                  return emptyUses;
               }
            }
            // If it is not possible to detect the correct DefEdge skip replacement and keep conservative
            return false;
         }
      }

      TM->ReplaceTreeNode(user_stmt, operand, new_ssa);

      // BEWARE: check on ssaOperand is done before operand is reassigned while doing it after could lead to nullptr exception
      bool emptyUses = ssaOperand->CGetUseStmts().empty();
      operand = new_ssa;
      return emptyUses;
   }
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
   PredicateWithEdge() = delete;
   static bool classof(const PredicateBase* PB)
   {
      return PB->Type == gimple_cond_K || PB->Type == gimple_multi_way_if_K;
   }

   PredicateWithEdge(kind PType, tree_nodeConstRef Op, unsigned int _From, unsigned int _To) : PredicateBase(PType, Op), From(_From), To(_To)
   {
      THROW_ASSERT(PType == gimple_cond_K || PType == gimple_multi_way_if_K, "Only branch or multi-way if types allowd");
   }
};

// Given a predicate info that is a type of branching terminator, get the
// branching block.
unsigned int getBranchBlock(const PredicateBase* PB)
{
   THROW_ASSERT(PredicateWithEdge::classof(PB), "Only branches and switches should have PHIOnly defs that require branch blocks.");
   return reinterpret_cast<const PredicateWithEdge*>(PB)->From;
}

// Used to store information about each value we might rename.
struct ValueInfo
{
   // Information about each possible copy. During processing, this is each
   // inserted info. After processing, we move the uninserted ones to the
   // uninserted vector.
   std::vector<PredicateBase*> Infos;
   std::vector<PredicateBase*> UninsertedInfos;
};

const ValueInfo& getValueInfo(tree_nodeConstRef Operand, eSSA::ValueInfoLookup& ValueInfoNums, std::vector<ValueInfo>& ValueInfos)
{
   auto OIN = ValueInfoNums.find(Operand);
   THROW_ASSERT(OIN != ValueInfoNums.end(), "Operand was not really in the Value Info Numbers");
   auto OINI = OIN->second;
   THROW_ASSERT(OINI < ValueInfos.size(), "Value Info Number greater than size of Value Info Table");
   return ValueInfos.at(OINI);
}

ValueInfo& getOrCreateValueInfo(tree_nodeConstRef Operand, eSSA::ValueInfoLookup& ValueInfoNums, std::vector<ValueInfo>& ValueInfos)
{
   auto OIN = ValueInfoNums.find(Operand);
   if(OIN == ValueInfoNums.end())
   {
      // This will grow it
      ValueInfos.resize(ValueInfos.size() + 1);
      // This will use the new size and give us a 0 based number of the info
      auto InsertResult = ValueInfoNums.insert({Operand, ValueInfos.size() - 1});
      THROW_ASSERT(InsertResult.second, "Value info number already existed?");
      return ValueInfos.at(InsertResult.first->second);
   }
   return ValueInfos.at(OIN->second);
}

void addInfoFor(OperandRef Op, PredicateBase* PB, CustomSet<OperandRef>& OpsToRename, eSSA::ValueInfoLookup& ValueInfoNums, std::vector<ValueInfo>& ValueInfos)
{
   OpsToRename.insert(Op);
   auto& OperandInfo = getOrCreateValueInfo(Op->getOperand(), ValueInfoNums, ValueInfos);
   OperandInfo.Infos.push_back(PB);
}

bool isCompare(const struct binary_expr* condition)
{
   auto c_type = condition->get_kind();
   return c_type == eq_expr_K || c_type == ne_expr_K || c_type == ltgt_expr_K || c_type == uneq_expr_K || c_type == gt_expr_K || c_type == lt_expr_K || c_type == ge_expr_K || c_type == le_expr_K || c_type == unlt_expr_K || c_type == ungt_expr_K ||
          c_type == unle_expr_K || c_type == unge_expr_K;
}

tree_nodeRef branchOpRecurse(tree_nodeRef op, tree_nodeRef stmt = nullptr)
{
   const auto Op = GET_CONST_NODE(op);
   if(const auto* nop = GetPointer<const nop_expr>(Op))
   {
      return branchOpRecurse(nop->op, stmt);
   }
   else if(const auto* ce = GetPointer<const convert_expr>(op))
   {
      return branchOpRecurse(ce->op, stmt);
   }
   else if(const auto* ssa = GetPointer<const ssa_name>(Op))
   {
      const auto DefStmt = ssa->CGetDefStmt();
      if(const auto* ga = GetPointer<const gimple_assign>(GET_CONST_NODE(DefStmt)))
      {
         return branchOpRecurse(ga->op1, DefStmt);
      }
      else if(const auto* gp = GetPointer<const gimple_phi>(GET_CONST_NODE(DefStmt)))
      {
         const auto& defEdges = gp->CGetDefEdgesList();
         THROW_ASSERT(not defEdges.empty(), "Branch variable definition from nowhere");
         return defEdges.size() > 1 ? nullptr : branchOpRecurse(defEdges.front().first, DefStmt);
      }
      else if(GetPointer<const gimple_nop>(GET_CONST_NODE(DefStmt)) != nullptr)
      {
         // Branch variable is a function parameter
         return nullptr;
      }
      THROW_UNREACHABLE("Branch var definition statement not handled (" + GET_CONST_NODE(DefStmt)->get_kind_text() + " " + GET_CONST_NODE(DefStmt)->ToString() + ")");
   }
   else if(GetPointer<const cst_node>(Op))
   {
      return op;
   }
   return stmt;
}

void processBranch(tree_nodeConstRef bi, CustomSet<OperandRef>& OpsToRename, eSSA::ValueInfoLookup& ValueInfoNums, std::vector<ValueInfo>& ValueInfos, CustomSet<std::pair<unsigned int, unsigned int>>& EdgeUsesOnly,
                   const std::map<unsigned int, blocRef> BBs,
                   int
#ifndef NDEBUG
                       debug_level
#endif
)
{
   const auto* BI = GetPointer<const gimple_cond>(GET_CONST_NODE(bi));
   THROW_ASSERT(BI, "Branch instruction should be gimple_cond");
   THROW_ASSERT(BBs.count(BI->bb_index), "Branch BB should be a valid BB");
   const auto BranchBB = BBs.at(BI->bb_index);
   THROW_ASSERT(BBs.count(BranchBB->true_edge), "True BB should be a valid BB");
   THROW_ASSERT(BBs.count(BranchBB->false_edge), "False BB should be a valid BB");
   const auto TrueBB = BBs.at(BranchBB->true_edge);
   const auto FalseBB = BBs.at(BranchBB->false_edge);
   const std::vector<blocRef> SuccsToProcess = {TrueBB, FalseBB};

   if(GetPointer<const cst_node>(GET_CONST_NODE(BI->op0)) != nullptr)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Branch variable is a cst_node, skipping...");
      return;
   }
   THROW_ASSERT(GET_CONST_NODE(BI->op0)->get_kind() == ssa_name_K, "Non SSA variable found in branch (" + GET_CONST_NODE(BI->op0)->ToString() + ")");
   const auto cond_ssa = BI->op0;
   const auto cond_stmt = branchOpRecurse(BI->op0);
   if(cond_stmt == nullptr)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Could not retrieve condition from branch variable, skipping... (" + GET_CONST_NODE(BI->op0)->ToString() + ")");
      return;
   }
   if(GetPointer<const cst_node>(GET_CONST_NODE(cond_stmt)))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Constant condition from branch variable, skipping... (" + GET_CONST_NODE(cond_stmt)->ToString() + ")");
      return;
   }
   const auto* CondStmt = GetPointer<const gimple_assign>(GET_CONST_NODE(cond_stmt));
   THROW_ASSERT(CondStmt, "Condition variable should be defined by gimple_assign (" + GET_CONST_NODE(cond_stmt)->ToString() + ")");
   const auto CondOp = GET_CONST_NODE(CondStmt->op1);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch condition is " + CondOp->get_kind_text() + " " + CondOp->ToString());

   // Can't insert conditional information if they all go to the same place.
   if(TrueBB->number == FalseBB->number)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---True and false edge target same block, skipping...");
      return;
   }

   auto InsertHelper = [&](tree_nodeRef Op) {
      for(const auto& Succ : SuccsToProcess)
      {
         if(Succ->number == BranchBB->number)
         {
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, GET_NODE(Op)->ToString() + " eligible for renaming in BB" + STR(Succ->number));

         PredicateBase* PB = new PredicateWithEdge(gimple_cond_K, Op, BranchBB->number, Succ->number);
         addInfoFor(OperandRef(new Operand(Op, cond_stmt)), PB, OpsToRename, ValueInfoNums, ValueInfos);
         if(Succ->list_of_pred.size() > 1)
         {
            EdgeUsesOnly.insert({BranchBB->number, Succ->number});
         }
      }
   };

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   if(const auto* bin = GetPointer<const binary_expr>(CondOp))
   {
      if(isCompare(bin))
      {
         const auto lhs = GET_CONST_NODE(bin->op0);
         const auto rhs = GET_CONST_NODE(bin->op1);
         if(lhs != rhs)
         {
            InsertHelper(cond_ssa);

            if(!GetPointer<const cst_node>(lhs) && GetPointer<const ssa_name>(lhs)->CGetUseStmts().size() > 1)
            {
               InsertHelper(bin->op0);
            }

            if(!GetPointer<const cst_node>(rhs) && GetPointer<const ssa_name>(rhs)->CGetUseStmts().size() > 1)
            {
               InsertHelper(bin->op1);
            }
         }
      }
      else if(bin->get_kind() == bit_and_expr_K || bin->get_kind() == bit_ior_expr_K)
      {
         InsertHelper(cond_ssa);
      }
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Unhandled condition type, skipping... (" + CondOp->get_kind_text() + " " + CondOp->ToString() + ")");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
}

// Process a block terminating switch, and place relevant operations to be
// renamed into OpsToRename.
void processMultiWayIf(tree_nodeConstRef mwii, CustomSet<OperandRef>& OpsToRename, eSSA::ValueInfoLookup& ValueInfoNums, std::vector<ValueInfo>& ValueInfos, CustomSet<std::pair<unsigned int, unsigned int>>& EdgeUsesOnly,
                       const std::map<unsigned int, blocRef> BBs,
                       int
#ifndef NDEBUG
                           debug_level
#endif
)
{
   const auto* MWII = GetPointer<const gimple_multi_way_if>(GET_CONST_NODE(mwii));
   THROW_ASSERT(MWII, "Multi way if instruction should be gimple_multi_way_if");
   const auto BranchBBI = MWII->bb_index;
   const auto BranchBB = BBs.at(BranchBBI);
   auto InsertHelper = [&](tree_nodeRef ssa_var, tree_nodeRef use_stmt, unsigned int TargetBBI) {
      THROW_ASSERT(static_cast<bool>(BBs.count(TargetBBI)), "Target BB should be in BB list");
      const auto& TargetBB = BBs.at(TargetBBI);

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + GET_CONST_NODE(ssa_var)->ToString() + " eligible for renaming in BB" + STR(TargetBBI));
      auto* PS = new PredicateWithEdge(gimple_multi_way_if_K, ssa_var, BranchBBI, TargetBBI);
      addInfoFor(OperandRef(new Operand(ssa_var, use_stmt)), PS, OpsToRename, ValueInfoNums, ValueInfos);
      if(TargetBB->list_of_pred.size() > 1)
      {
         EdgeUsesOnly.insert(std::make_pair(BranchBBI, TargetBBI));
      }
   };

   for(const auto& var_pair : MWII->list_of_cond)
   {
      const auto case_var = var_pair.first;
      const auto BBI = var_pair.second;
      if(case_var == nullptr)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Condition: else branch");
         continue;
      }
      if(BBI == BranchBBI)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch loopback detected: variable renaming not safe, skipping...");
         continue;
      }
      if(GetPointer<const cst_node>(GET_CONST_NODE(case_var)) != nullptr)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch variable is a cst_node, skipping...");
         continue;
      }

      const auto* case_ssa = GetPointer<const ssa_name>(GET_CONST_NODE(case_var));
      THROW_ASSERT(case_ssa, "Case conditional variable should be an ssa_name (" + GET_CONST_NODE(case_var)->ToString() + ")");
      const auto case_stmt = case_ssa->CGetDefStmt();
      if(GET_CONST_NODE(case_stmt)->get_kind() == gimple_phi_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch variable already defined inside a phi node, skipping...");
         continue;
      }
      const auto* Case_stmt = GetPointer<const gimple_assign>(GET_CONST_NODE(case_stmt));
      THROW_ASSERT(Case_stmt, "Case statement should be a gimple_assign (" + GET_CONST_NODE(case_stmt)->get_kind_text() + " " + GET_CONST_NODE(case_stmt)->ToString() + ")");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Condition: " + GET_CONST_NODE(Case_stmt->op1)->get_kind_text() + " " + GET_CONST_NODE(Case_stmt->op1)->ToString());
      if(const auto* case_cmp = GetPointer<const binary_expr>(GET_CONST_NODE(Case_stmt->op1)))
      {
         if(isCompare(case_cmp))
         {
            const auto lhs = GET_CONST_NODE(case_cmp->op0);
            const auto rhs = GET_CONST_NODE(case_cmp->op1);
            if(lhs != rhs)
            {
               InsertHelper(case_var, case_stmt, BBI);

               if(!GetPointer<const cst_node>(lhs) && GetPointer<const ssa_name>(lhs)->CGetUseStmts().size() > 1)
               {
                  InsertHelper(case_cmp->op0, case_stmt, BBI);
               }

               if(!GetPointer<const cst_node>(rhs) && GetPointer<const ssa_name>(rhs)->CGetUseStmts().size() > 1)
               {
                  InsertHelper(case_cmp->op1, case_stmt, BBI);
               }
            }
         }
         else if(case_cmp->get_kind() == bit_and_expr_K || case_cmp->get_kind() == bit_ior_expr_K)
         {
            InsertHelper(case_var, case_stmt, BBI);
         }
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Unhandled condition type, skipping...");
      }
   }
}

// Perform a strict weak ordering on instructions and arguments.
bool valueComesBefore(OrderedInstructions& OI, tree_nodeConstRef A, tree_nodeConstRef B)
{
   return OI.dominates(GetPointer<const gimple_node>(GET_CONST_NODE(A)), GetPointer<const gimple_node>(GET_CONST_NODE(B)));
}

// Given a predicate info that is a type of branching terminator, get the
// edge this predicate info represents
const std::pair<unsigned int, unsigned int> getBlockEdge(const PredicateBase* PB)
{
   THROW_ASSERT(PredicateWithEdge::classof(PB), "Not a predicate info type we know how to get an edge from.");
   const auto* PEdge = static_cast<const PredicateWithEdge*>(PB);
   return std::make_pair(PEdge->From, PEdge->To);
}

struct DFSInfo
{
   unsigned int DFSIn = 0;
   unsigned int DFSOut = 0;
};

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
   tree_nodeRef Def = nullptr;
   OperandRef U = nullptr;
   // Neither PInfo nor EdgeOnly participate in the ordering
   PredicateBase* PInfo = nullptr;
   bool EdgeOnly = false;

   std::string ToString()
   {
      return "Predicate info: " + (PInfo ? PInfo->OriginalOp->ToString() : "null") + " Def: " + (Def ? Def->ToString() : "null") + " Use: " + (U ? U->getUser()->ToString() : "null") + " EdgeOnly: " + (EdgeOnly ? "true" : "false");
   }
};

// This compares ValueDFS structures, creating OrderedBasicBlocks where
// necessary to compare uses/defs in the same block.  Doing so allows us to walk
// the minimum number of instructions necessary to compute our def/use ordering.
struct ValueDFS_Compare
{
   OrderedInstructions& OI;
   ValueDFS_Compare(OrderedInstructions& _OI) : OI(_OI)
   {
   }

   // For a phi use, or a non-materialized def, return the edge it represents.
   const std::pair<unsigned int, unsigned int> getBlockEdge_local(const ValueDFS& VD) const
   {
      if(!VD.Def && VD.U)
      {
         const auto* PHI = GetPointer<const gimple_phi>(GET_CONST_NODE(VD.U->getUser()));
         auto phiDefEdge = std::find_if(PHI->CGetDefEdgesList().begin(), PHI->CGetDefEdgesList().end(), [&](const gimple_phi::DefEdge& de) { return GET_INDEX_CONST_NODE(de.first) == GET_INDEX_CONST_NODE(VD.U->getOperand()); });
         THROW_ASSERT(phiDefEdge != PHI->CGetDefEdgesList().end(), "Unable to find variable in phi definitions");
         return std::make_pair(phiDefEdge->second, PHI->bb_index);
      }
      // This is really a non-materialized def.
      return getBlockEdge(VD.PInfo);
   }

   // For two phi related values, return the ordering.
   bool comparePHIRelated(const ValueDFS& A, const ValueDFS& B) const
   {
      auto& ABlockEdge = getBlockEdge_local(A);
      auto& BBlockEdge = getBlockEdge_local(B);
      // Now sort by block edge and then defs before uses.
      return std::tie(ABlockEdge, A.Def, A.U) < std::tie(BBlockEdge, B.Def, B.U);
   }

   // Get the definition of an instruction that occurs in the middle of a block.
   tree_nodeRef getMiddleDef(const ValueDFS& VD) const
   {
      if(VD.Def)
      {
         return VD.Def;
      }
      return nullptr;
   }

   // Return either the Def, if it's not null, or the user of the Use, if the def
   // is null.
   tree_nodeRef getDefOrUser(const tree_nodeRef Def, const OperandRef U) const
   {
      if(Def)
      {
         return Def;
      }
      return U->getUser();
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

      bool SameBlock = std::tie(A.DFSIn, A.DFSOut) == std::tie(B.DFSIn, B.DFSOut);

      // We want to put the def that will get used for a given set of phi uses,
      // before those phi uses.
      // So we sort by edge, then by def.
      // Note that only phi nodes uses and defs can come last.
      if(SameBlock && A.LocalNum == LN_Last && B.LocalNum == LN_Last)
      {
         return comparePHIRelated(A, B);
      }

      if(!SameBlock || A.LocalNum != LN_Middle || B.LocalNum != LN_Middle)
      {
         return std::tie(A.DFSIn, A.DFSOut, A.LocalNum, A.Def, A.U) < std::tie(B.DFSIn, B.DFSOut, B.LocalNum, B.Def, B.U);
      }
      return localComesBefore(A, B);
   }
};
using ValueDFSStack = std::vector<ValueDFS>;

bool stackIsInScope(const ValueDFSStack& Stack, const ValueDFS& VDUse, const OrderedInstructions& OI)
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
      const auto* PHI = GetPointer<const gimple_phi>(GET_CONST_NODE(VDUse.U->getUser()));
      if(!PHI)
      {
         return false;
      }
      // Check edge
      auto EdgePredIt = std::find_if(PHI->CGetDefEdgesList().begin(), PHI->CGetDefEdgesList().end(), [&](const gimple_phi::DefEdge& de) { return GET_INDEX_CONST_NODE(de.first) == GET_INDEX_CONST_NODE(VDUse.U->getOperand()); });
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

void popStackUntilDFSScope(ValueDFSStack& Stack, const ValueDFS& VD, const OrderedInstructions& OI)
{
   while(!Stack.empty() && !stackIsInScope(Stack, VD, OI))
   {
      Stack.pop_back();
   }
}

// Convert the uses of Op into a vector of uses, associating global and local
// DFS info with each one.
void convertUsesToDFSOrdered(tree_nodeRef Op, std::vector<ValueDFS>& DFSOrderedSet, BBGraphRef DT, const CustomMap<unsigned int, DFSInfo>& DFSInfos,
                             int
#ifndef NDEBUG
                                 debug_level
#endif
)
{
   const auto* op = GetPointer<const ssa_name>(GET_CONST_NODE(Op));
   THROW_ASSERT(op, "Op is not an ssa_name (" + GET_CONST_NODE(Op)->get_kind_text() + ")");
   const auto defBBI = GetPointer<const gimple_node>(GET_CONST_NODE(op->CGetDefStmt()))->bb_index;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   for(auto& Usize : op->CGetUseStmts())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Checking " + Usize.first->ToString());
      const auto* gp = GetPointer<const gimple_phi>(GET_CONST_NODE(Usize.first));
      if(gp)
      {
         if(gp->CGetDefEdgesList().size() == 1)
         {
            // Sigma uses not intresting (already e-SSA)
            continue;
         }
      }

      auto& U = Usize.first;
      const auto* I = GetPointer<const gimple_node>(GET_CONST_NODE(U));
      THROW_ASSERT(I, "Use statement should be a gimple_node");
      if(I->bb_index == defBBI)
      {
         // Uses within the same basic block not intresting (they are casts or the actual branch eveluating the condition)
         continue;
      }
      ValueDFS VD;

      unsigned int IBlock;
      if(gp)
      {
         const auto EdgePredIt = std::find_if(gp->CGetDefEdgesList().begin(), gp->CGetDefEdgesList().end(), [&](const gimple_phi::DefEdge& de) { return GET_INDEX_CONST_NODE(de.first) == GET_INDEX_CONST_NODE(Op); });
         THROW_ASSERT(EdgePredIt != gp->CGetDefEdgesList().end(), "");
         IBlock = EdgePredIt->second;
         VD.LocalNum = LN_Last;
      }
      else
      {
         IBlock = I->bb_index;
         VD.LocalNum = LN_Middle;
      }

      const auto& BBmap = DT->CGetBBGraphInfo()->bb_index_map;
      auto DomNode_vertex = BBmap.find(IBlock);
      THROW_ASSERT(DomNode_vertex != BBmap.end(), "BB" + STR(IBlock) + " not found in DT");
      // It's possible our use is in an unreachable block. Skip it if so.
      if(!DT->IsReachable(BBmap.at(bloc::ENTRY_BLOCK_ID), DomNode_vertex->second))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---BB" + STR(IBlock) + " is unreachable from DT root");
         continue;
      }
      const auto& DomNode_DFSInfo = DFSInfos.at(IBlock);
      VD.DFSIn = DomNode_DFSInfo.DFSIn;
      VD.DFSOut = DomNode_DFSInfo.DFSOut;
      VD.U = OperandRef(new Operand(Op, U));
      DFSOrderedSet.push_back(VD);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Pushed on renaming stack");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
}

// Given the renaming stack, make all the operands currently on the stack real
// by inserting them into the IR.  Return the last operation's value.
tree_nodeRef materializeStack(ValueDFSStack& RenameStack, unsigned int function_id, statement_list* sl, tree_nodeRef OrigOp, CustomMap<tree_nodeConstRef, const PredicateBase*>& PredicateMap, const BBGraphRef DT, tree_managerRef TM,
                              tree_manipulationRef tree_man, CustomMap<std::pair<unsigned int, unsigned int>, blocRef>& interBranchBBs, CustomMap<unsigned int, DFSInfo>& DFSInfos
#ifndef NDEBUG
                              ,
                              int debug_level
#endif
)
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
   auto& BBmap = DT->GetBBGraphInfo()->bb_index_map;
   // The maximum number of things we should be trying to materialize at once
   // right now is 4, depending on if we had an assume, a branch, and both used
   // and of conditions.
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   for(auto RenameIter = RenameStack.end() - Start; RenameIter != RenameStack.end(); ++RenameIter)
   {
      auto Op = OrigOp;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Checking variable " + GET_CONST_NODE(Op)->ToString());
      if(RenameIter != RenameStack.begin())
      {
         THROW_ASSERT((RenameIter - 1)->Def, "A valid definition shold be on the stack at this point");
         const auto* gp = GetPointer<const gimple_phi>(GET_CONST_NODE((RenameIter - 1)->Def));
         THROW_ASSERT(gp, "Previous definition on stack should be a gimple_phi (" + GET_CONST_NODE((RenameIter - 1)->Def)->ToString() + ")");
         Op = gp->res;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Moving check to " + GET_CONST_NODE(Op)->ToString());
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
         const auto* pwe = static_cast<const PredicateWithEdge*>(ValInfo);
         THROW_ASSERT(BBmap.contains(pwe->To), "Basic block should be in dominator tree");
         const auto& ToBB_vertex = BBmap.at(pwe->To);
         const auto& ToBB = DT->CGetBBNodeInfo(ToBB_vertex)->block;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Inserting into BB" + STR(ToBB->number));

         tree_nodeRef PIC, new_ssa_var;
         std::vector<std::pair<tree_nodeRef, unsigned int>> list_of_def_edge = {{Op, pwe->From}};
         if(ToBB->list_of_pred.size() > 1) // New intermediate BB has to be created
         {
            blocRef interBB;
            // Check if intermediate BB has already been created by previous renaming operation
            auto from_number = std::make_pair(pwe->From, ToBB->number);
            auto interBBIt = interBranchBBs.find(from_number);
            if(interBBIt != interBranchBBs.end())
            {
               interBB = interBBIt->second;
            }
            else // When intermediate branch BB isn't present, create a new one
            {
               THROW_ASSERT(static_cast<bool>(BBmap.count(pwe->From)), "UOT??? ");
               const auto& FromBB = DT->CGetBBNodeInfo(BBmap.at(pwe->From))->block;

               // Create new intermediate BB
               const auto interBBI = (sl->list_of_bloc.rbegin())->first + 1;
               interBB = blocRef(new bloc(interBBI));
               sl->list_of_bloc[interBBI] = interBB;
               interBB->loop_id = FromBB->loop_id;
               interBB->SetSSAUsesComputed();
               interBB->schedule = FromBB->schedule;

               // Fix BB connections
               interBB->list_of_pred.push_back(FromBB->number);
               FromBB->list_of_succ.erase(std::find(FromBB->list_of_succ.begin(), FromBB->list_of_succ.end(), ToBB->number));
               FromBB->list_of_succ.push_back(interBBI);
               interBB->list_of_succ.push_back(ToBB->number);
               ToBB->list_of_pred.erase(std::find(ToBB->list_of_pred.begin(), ToBB->list_of_pred.end(), FromBB->number));
               ToBB->list_of_pred.push_back(interBBI);

               // Add new BB to intermediate branch block lookup list
               interBranchBBs.insert({{FromBB->number, ToBB->number}, interBB});

               // Shadow incoming BB in dominator tree mapping
               BBmap.insert({interBBI, BBmap.at(FromBB->number)});
               DFSInfos.insert({interBBI, DFSInfos.at(FromBB->number)});

               // Fix branch routes
               if(FromBB->false_edge == ToBB->number)
               {
                  FromBB->false_edge = interBBI;
               }
               if(FromBB->true_edge == ToBB->number)
               {
                  FromBB->true_edge = interBBI;
               }

               // Fix multi_way_if routes
               if(auto* mwi = GetPointer<gimple_multi_way_if>(GET_NODE(FromBB->CGetStmtList().back())))
               {
                  for(auto& cond : mwi->list_of_cond)
                  {
                     if(cond.second == ToBB->number)
                     {
                        cond.second = interBBI;
                     }
                  }
               }

               // Fix destination BB phis
               for(auto phi : ToBB->CGetPhiList())
               {
                  auto* gp = GetPointer<gimple_phi>(GET_NODE(phi));
                  const auto defFrom = std::find_if(gp->CGetDefEdgesList().begin(), gp->CGetDefEdgesList().end(), [&](const gimple_phi::DefEdge& de) { return de.second == FromBB->number; });
                  if(defFrom != gp->CGetDefEdgesList().end())
                  {
                     gp->ReplaceDefEdge(TM, *defFrom, {defFrom->first, interBBI});
                  }
               }
            }

            // Insert required sigma operation into the intermediate basic block
            PIC = tree_man->create_phi_node(new_ssa_var, list_of_def_edge, TM->GetTreeReindex(function_id), interBB->number);
            GetPointer<gimple_phi>(GET_NODE(PIC))->SetSSAUsesComputed();
            interBB->AddPhi(PIC);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Insertion moved to intermediate BB" + STR(interBB->number));
         }
         else
         {
            // Insert required sigma operation into the destination basic block
            PIC = tree_man->create_phi_node(new_ssa_var, list_of_def_edge, TM->GetTreeReindex(function_id), pwe->To);
            GetPointer<gimple_phi>(GET_NODE(PIC))->SetSSAUsesComputed();
            ToBB->AddPhi(PIC);
         }

         // Clone renamed ssa properties
         const auto* op = GetPointer<const ssa_name>(GET_CONST_NODE(Op));
         auto* newSSA = GetPointer<ssa_name>(GET_NODE(new_ssa_var));
         newSSA->bit_values = op->bit_values;
         newSSA->range = RangeRef(op->range ? op->range->clone() : nullptr);
         newSSA->min = op->min;
         newSSA->max = op->max;

         PredicateMap.insert({PIC, ValInfo});
         Result.Def = PIC;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Materialized " + GET_CONST_NODE(PIC)->ToString());
      }
      else
      {
         THROW_UNREACHABLE("Invalid PredicateInfo type");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   return RenameStack.back().Def;
}

// Instead of the standard SSA renaming algorithm, which is O(Number of
// instructions), and walks the entire dominator tree, we walk only the defs +
// uses.  The standard SSA renaming algorithm does not really rely on the
// dominator tree except to order the stack push/pops of the renaming stacks, so
// that defs end up getting pushed before hitting the correct uses.  This does
// not require the dominator tree, only the *order* of the dominator tree. The
// complete and correct ordering of the defs and uses, in dominator tree is
// contained in the DFS numbering of the dominator tree. So we sort the defs and
// uses into the DFS ordering, and then just use the renaming stack as per
// normal, pushing when we hit a def (which is a predicateinfo instruction),
// popping when we are out of the dfs scope for that def, and replacing any uses
// with top of stack if it exists.  In order to handle liveness without
// propagating liveness info, we don't actually insert the predicateinfo
// instruction def until we see a use that it would dominate.  Once we see such
// a use, we materialize the predicateinfo instruction in the right place and
// use it.
//
bool eSSA::renameUses(CustomSet<OperandRef>& OpSet, eSSA::ValueInfoLookup& ValueInfoNums, std::vector<ValueInfo>& ValueInfos, CustomMap<unsigned int, DFSInfo>& DFSInfos, CustomSet<std::pair<unsigned int, unsigned int>>& EdgeUsesOnly, statement_list* sl)
{
   const auto TM = AppM->get_tree_manager();
   const auto tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters));
   CustomMap<std::pair<unsigned int, unsigned int>, blocRef> interBranchBBs;
   bool modified = false;
   // This maps from copy operands to Predicate Info. Note that it does not own
   // the Predicate Info, they belong to the ValueInfo structs in the ValueInfos
   // vector.
   CustomMap<tree_nodeConstRef, const PredicateBase*> PredicateMap;
   // Sort OpsToRename since we are going to iterate it.
   std::vector<OperandRef> OpsToRename(OpSet.begin(), OpSet.end());
   OrderedInstructions OI(DT);
   auto Comparator = [&](const OperandRef A, const OperandRef B) { return valueComesBefore(OI, A->getUser(), B->getUser()); };
   std::sort(OpsToRename.begin(), OpsToRename.end(), Comparator);
   ValueDFS_Compare Compare(OI);

   for(auto& Op : OpsToRename)
   {
      std::vector<ValueDFS> OrderedUses;
      const auto& ValueInfo = getValueInfo(Op->getOperand(), ValueInfoNums, ValueInfos);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing " + Op->getOperand()->ToString() + " with " + STR(ValueInfo.Infos.size()) + " possible copies");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      // Insert the possible copies into the def/use list.
      // They will become real copies if we find a real use for them, and never
      // created otherwise.
      for(auto& PossibleCopy : ValueInfo.Infos)
      {
         ValueDFS VD;
         if(PredicateWithEdge::classof(PossibleCopy))
         {
            // If we can only do phi uses, we treat it like it's in the branch
            // block, and handle it specially. We know that it goes last, and only
            // dominate phi uses.
            const auto BlockEdge = getBlockEdge(PossibleCopy);
            if(EdgeUsesOnly.count(BlockEdge))
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
               }
            }
         }
      }

      convertUsesToDFSOrdered(Op->getOperand(), OrderedUses, DT, DFSInfos, debug_level);
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
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "RenameStack top DFS numbers are (" + STR(RenameStack.back().DFSIn) + "," + STR(RenameStack.back().DFSOut) + ")");
         }
#endif
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Current DFS numbers are (" + STR(VD.DFSIn) + "," + STR(VD.DFSOut) + ")");
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
         THROW_ASSERT(VD.U, "A use sohuld be in scope for current renaming operation");
#if HAVE_ASSERTS
         if(const auto* gp = GetPointer<const gimple_phi>(GET_CONST_NODE(VD.U->getUser())))
         {
            THROW_ASSERT(gp->CGetDefEdgesList().size() > 1, "Sigma operation should not be renamed (BB" + STR(gp->bb_index) + " " + gp->ToString() + ")");
         }
#endif

         // If the possible copy dominates something, materialize our stack up to
         // this point. This ensures every comparison that affects our operation
         // ends up with predicateinfo.
         if(!Result.Def)
         {
#ifndef NDEBUG
            if(not AppM->ApplyNewTransformation())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               return modified;
            }
#endif
            Result.Def = materializeStack(RenameStack, function_id, sl, Op->getOperand(), PredicateMap, DT, TM, tree_man, interBranchBBs, DFSInfos
#ifndef NDEBUG
                                          ,
                                          debug_level
#endif
            );
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found replacement " + GET_CONST_NODE(Result.Def)->ToString() + " for " + GET_CONST_NODE(VD.U->getOperand())->ToString() + " in " + GET_CONST_NODE(VD.U->getUser())->ToString());
         const auto* phi = GetPointer<const gimple_phi>(GET_CONST_NODE(Result.Def));
#if 0
         if(!valueComesBefore(OI, Result.Def, VD.U->getUser()))
         {
            const auto defVertex = DT->CGetBBGraphInfo()->bb_index_map.at(GetPointer<const gimple_node>(GET_CONST_NODE(Result.Def))->bb_index);
            const auto defBB = DT->CGetBBNodeInfo(defVertex)->block;
            const auto defBB_succ = defBB->list_of_succ;
            if(std::find(defBB_succ.begin(), defBB_succ.end(), defBB->number) == defBB_succ.end())
            {
               THROW_UNREACHABLE("PredicateInfo def should have dominated this use at least in the CFG");
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---New definition not dominating use in DT, but in CFG only");
         }
#endif
#ifndef NDEBUG
         AppM->RegisterTransformation(GetName(), VD.U->getUser());
#endif
         VD.U->set(phi->res, TM);
         modified = true;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
   return modified;
}

eSSA::eSSA(const ParameterConstRef params, const application_managerRef AM, unsigned int f_id, const DesignFlowManagerConstRef dfm) : FunctionFrontendFlowStep(AM, f_id, ESSA, dfm, params)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

eSSA::~eSSA() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> eSSA::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case DEPENDENCE_RELATIONSHIP:
      {
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(EXTRACT_GIMPLE_COND_OP, SAME_FUNCTION));
         relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return relationships;
}

bool eSSA::HasToBeExecuted() const
{
   const FunctionBehaviorConstRef FB = AppM->CGetFunctionBehavior(function_id);
   return FB->GetBBVersion() != bb_ver || FB->GetBitValueVersion() != bv_ver;
}

DesignFlowStep_Status eSSA::InternalExec()
{
   auto TM = AppM->get_tree_manager();
   const auto* fd = GetPointer<const function_decl>(TM->get_tree_node_const(function_id));
   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Extended SSA step");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Dominator tree computation...");

   /// store the IR BB graph ala boost::graph
   BBGraphsCollectionRef GCC_bb_graphs_collection(new BBGraphsCollection(BBGraphInfoRef(new BBGraphInfo(AppM, function_id)), parameters));
   BBGraph GCC_bb_graph(GCC_bb_graphs_collection, CFG_SELECTOR);
   CustomUnorderedMap<unsigned int, vertex> inverse_vertex_map;
   /// add vertices
   for(auto block : sl->list_of_bloc)
   {
      inverse_vertex_map.try_emplace(block.first, GCC_bb_graphs_collection->AddVertex(BBNodeInfoRef(new BBNodeInfo(block.second))));
   }

   /// add edges
   for(auto curr_bb_pair : sl->list_of_bloc)
   {
      unsigned int curr_bb = curr_bb_pair.first;
      for(const auto& lop : sl->list_of_bloc.at(curr_bb)->list_of_pred)
      {
         THROW_ASSERT(static_cast<bool>(inverse_vertex_map.count(lop)), "BB" + STR(lop) + " (successor of BB" + STR(curr_bb) + ") does not exist");
         GCC_bb_graphs_collection->AddEdge(inverse_vertex_map.at(lop), inverse_vertex_map.at(curr_bb), CFG_SELECTOR);
      }

      for(const auto& los : sl->list_of_bloc.at(curr_bb)->list_of_succ)
      {
         if(los == bloc::EXIT_BLOCK_ID)
         {
            GCC_bb_graphs_collection->AddEdge(inverse_vertex_map.at(curr_bb), inverse_vertex_map.at(los), CFG_SELECTOR);
         }
      }

      if(sl->list_of_bloc.at(curr_bb)->list_of_succ.empty())
      {
         GCC_bb_graphs_collection->AddEdge(inverse_vertex_map.at(curr_bb), inverse_vertex_map.at(bloc::EXIT_BLOCK_ID), CFG_SELECTOR);
      }
   }

   /// add a connection between entry and exit thus avoiding problems with non terminating code
   GCC_bb_graphs_collection->AddEdge(inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID), inverse_vertex_map.at(bloc::EXIT_BLOCK_ID), CFG_SELECTOR);

   dominance<BBGraph> bb_dominators(GCC_bb_graph, inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID), inverse_vertex_map.at(bloc::EXIT_BLOCK_ID), parameters);
   bb_dominators.calculate_dominance_info(dominance<BBGraph>::CDI_DOMINATORS);

   DT.reset(new BBGraph(GCC_bb_graphs_collection, D_SELECTOR));
   for(auto it : bb_dominators.get_dominator_map())
   {
      if(it.first != inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID))
      {
         GCC_bb_graphs_collection->AddEdge(it.second, it.first, D_SELECTOR);
      }
   }
   DT->GetBBGraphInfo()->bb_index_map = std::move(inverse_vertex_map);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Dominator tree computation completed (" + STR(DT->num_bblocks()) + " BB generated)");

   // This stores info about each operand or comparison result we make copies
   // of.  The real ValueInfos start at index 1, index 0 is unused so that we can
   // more easily detect invalid indexing.
   std::vector<ValueInfo> ValueInfos;
   ValueInfos.resize(1);
   // This gives the index into the ValueInfos array for a given Value.  Because
   // 0 is not a valid Value Info index, you can use DenseMap::lookup and tell
   // whether it returned a valid result.
   ValueInfoLookup ValueInfoNums;
   // The set of edges along which we can only handle phi uses, due to critical edges.
   CustomSet<std::pair<unsigned int, unsigned int>> EdgeUsesOnly;
   // Collect operands to rename from all conditional branch terminators, as well
   // as multi-way if.
   CustomSet<OperandRef> OpsToRename;

   auto BBvisit = [&](blocRef BB) {
      const auto& stmt_list = BB->CGetStmtList();

      // Skip empty BB
      if(stmt_list.empty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Empty block");
         return;
      }

      const auto terminator = stmt_list.back();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Block terminates with " + GET_NODE(terminator)->get_kind_text());
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      if(GET_CONST_NODE(terminator)->get_kind() == gimple_cond_K)
      {
         processBranch(terminator, OpsToRename, ValueInfoNums, ValueInfos, EdgeUsesOnly, sl->list_of_bloc, debug_level);
      }
      else if(GET_CONST_NODE(terminator)->get_kind() == gimple_multi_way_if_K)
      {
         processMultiWayIf(terminator, OpsToRename, ValueInfoNums, ValueInfos, EdgeUsesOnly, sl->list_of_bloc, debug_level);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   };

   // This map stores DFS numeration for each DT node
   CustomMap<unsigned int, DFSInfo> DFSInfos;

   std::stack<std::pair<vertex, boost::iterator_range<BBGraph::adjacency_iterator>>> workStack;
   const auto entryVertex = DT->GetBBGraphInfo()->bb_index_map.at(bloc::ENTRY_BLOCK_ID);
   workStack.push({entryVertex, boost::make_iterator_range(boost::adjacent_vertices(entryVertex, *DT))});

   unsigned int DFSNum = 0;
   const auto& BBentry = DT->CGetBBNodeInfo(workStack.top().first)->block;
   DFSInfos[BBentry->number].DFSIn = DFSNum++;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing BB" + STR(BBentry->number));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   BBvisit(BBentry);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");

   while(!workStack.empty())
   {
      const auto& BB = DT->CGetBBNodeInfo(workStack.top().first)->block;
      auto ChildRange = workStack.top().second;

      // If we visited all of the children of this node, "recurse" back up the
      // stack setting the DFOutNum.
      if(ChildRange.empty())
      {
         DFSInfos[BB->number].DFSOut = DFSNum++;
         workStack.pop();
      }
      else
      {
         // Otherwise, recursively visit this child.
         const auto Child = DT->CGetBBNodeInfo(ChildRange.front())->block;
         workStack.top().second.pop_front();

         workStack.push({ChildRange.front(), boost::make_iterator_range(boost::adjacent_vertices(ChildRange.front(), *DT))});
         DFSInfos[Child->number].DFSIn = DFSNum++;

         // Perform BB analysis for eSSA purpose
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing BB" + STR(Child->number));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
         BBvisit(Child);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
   }
#ifndef NDEBUG
   if(static_cast<size_t>(DFSInfos.size()) < (DT->num_bblocks() + 2))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Dominator tree has some unreachable blocks");
   }
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysis detected " + STR(OpsToRename.size()) + " operations to rename");
   if(OpsToRename.empty())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      return DesignFlowStep_Status::UNCHANGED;
   }

   bool modified = renameUses(OpsToRename, ValueInfoNums, ValueInfos, DFSInfos, EdgeUsesOnly, sl);
   DT.reset();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   const auto FB = AppM->GetFunctionBehavior(function_id);
   bb_ver = FB->GetBBVersion();
   bv_ver = FB->GetBitValueVersion();
   if(modified)
   {
      bb_ver = FB->UpdateBBVersion();
      return DesignFlowStep_Status::SUCCESS;
   }
   return DesignFlowStep_Status::UNCHANGED;
}

void eSSA::Initialize()
{
}
