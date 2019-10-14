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
#include "basic_block.hpp"
#include "function_behavior.hpp"

#include "Dominance.hpp"

/// design_flows include
#include "design_flow_manager.hpp"

/// frontend_analysis
#include "application_frontend_flow_step.hpp"

/// tree include
#include "behavioral_helper.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_reindex.hpp"

/// wrapper/treegcc include
#include "gcc_wrapper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

namespace eSSAInfo
{
   class PredicateBase
   {
    public:
      kind Type;
      // The original operand before we renamed it.
      // This can be use by passes, when destroying predicateinfo, to know
      // whether they can just drop the intrinsic, or have to merge metadata.
      tree_nodeRef OriginalOp;
      PredicateBase(const PredicateBase&) = delete;
      PredicateBase& operator=(const PredicateBase&) = delete;
      PredicateBase() = delete;
      virtual ~PredicateBase() = default;

    protected:
      PredicateBase(kind PT, tree_nodeRef Op) : Type(PT), OriginalOp(Op)
      {
      }
   };

   class PredicateWithCondition : public PredicateBase
   {
    public:
      tree_nodeRef Condition;
      static bool classof(const PredicateBase* PB)
      {
         return PB->Type == gimple_cond_K || PB->Type == gimple_multi_way_if_K;
      }

    protected:
      PredicateWithCondition(kind PT, tree_nodeRef Op, tree_nodeRef Cond) : PredicateBase(PT, Op), Condition(Cond)
      {
      }
   };

   // Mixin class for edge predicates.  The FROM block is the block where the
   // predicate originates, and the TO block is the block where the predicate is
   // valid.
   class PredicateWithEdge : public PredicateWithCondition
   {
    public:
      blocRef From;
      blocRef To;
      PredicateWithEdge() = delete;
      static bool classof(const PredicateBase* PB)
      {
         return PB->Type == gimple_cond_K || PB->Type == gimple_multi_way_if_K;
      }

    protected:
      PredicateWithEdge(kind PType, tree_nodeRef Op, blocRef _From, blocRef _To, tree_nodeRef Cond) : PredicateWithCondition(PType, Op, Cond), From(_From), To(_To)
      {
      }
   };

   // Provides predicate information for branches.
   class PredicateBranch : public PredicateWithEdge
   {
    public:
      // If true, SplitBB is the true successor, otherwise it's the false successor.
      bool TrueEdge;
      PredicateBranch(tree_nodeRef Op, blocRef BranchBB, blocRef SplitBB, tree_nodeRef Cond, bool TakenEdge) : PredicateWithEdge(gimple_cond_K, Op, BranchBB, SplitBB, Cond), TrueEdge(TakenEdge)
      {
      }
      PredicateBranch() = delete;
      static bool classof(const PredicateBase* PB)
      {
         return PB->Type == gimple_cond_K;
      }
   };

   // Given a predicate info that is a type of branching terminator, get the
   // branching block.
   const blocRef getBranchBlock(const PredicateBase* PB)
   {
      THROW_ASSERT(PB->Type == gimple_cond_K, 
         "Only branches and switches should have PHIOnly defs that require branch blocks.");
      return reinterpret_cast<const PredicateBranch*>(PB)->From;
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

   const ValueInfo& getValueInfo(tree_nodeRef Operand, std::map<tree_nodeRef, unsigned int>& ValueInfoNums, std::vector<ValueInfo>& ValueInfos)
   {
      auto OIN = ValueInfoNums.find(Operand);
      THROW_ASSERT(OIN != ValueInfoNums.end(), "Operand was not really in the Value Info Numbers");
      auto OINI = OIN->second;
      THROW_ASSERT(OINI < ValueInfos.size(), "Value Info Number greater than size of Value Info Table");
      return ValueInfos[OINI];
   }

   ValueInfo& getOrCreateValueInfo(tree_nodeRef Operand, std::map<tree_nodeRef, unsigned int>& ValueInfoNums, std::vector<ValueInfo>& ValueInfos)
   {
      auto OIN = ValueInfoNums.find(Operand);
      if(OIN == ValueInfoNums.end())
      {
         // This will grow it
         ValueInfos.resize(ValueInfos.size() + 1);
         // This will use the new size and give us a 0 based number of the info
         auto InsertResult = ValueInfoNums.insert({Operand, ValueInfos.size() - 1});
         THROW_ASSERT(InsertResult.second, "Value info number already existed?");
         return ValueInfos[InsertResult.first->second];
      }
      return ValueInfos[OIN->second];
   }

   void addInfoFor(tree_nodeRef Op, PredicateBase* PB,
      std::set<tree_nodeRef>& OpsToRename, 
      std::map<tree_nodeRef, unsigned int>& ValueInfoNums, std::vector<eSSAInfo::ValueInfo>& ValueInfos)
   {
      OpsToRename.insert(Op);
      auto& OperandInfo = getOrCreateValueInfo(Op, ValueInfoNums, ValueInfos);
      OperandInfo.Infos.push_back(PB);
   }

   bool isCompare(struct binary_expr* condition)
   {
      auto c_type = condition->get_kind();
      return c_type == eq_expr_K || c_type == ne_expr_K || c_type == ltgt_expr_K || c_type == uneq_expr_K
         || c_type == gt_expr_K || c_type == lt_expr_K || c_type == ge_expr_K || c_type == le_expr_K 
         || c_type == unlt_expr_K || c_type == ungt_expr_K || c_type == unle_expr_K || c_type == unge_expr_K;
   }

   void processBranch(struct gimple_cond* BI, blocRef BranchBB, 
      std::set<tree_nodeRef>& OpsToRename, 
      std::map<tree_nodeRef, unsigned int>& ValueInfoNums, std::vector<eSSAInfo::ValueInfo>& ValueInfos, 
      std::set<std::pair<blocRef, blocRef>>& EdgeUsesOnly, 
      const BBGraphConstRef DT, const application_managerRef /*AppM*/, 
      int debug_level)
   {
      const auto bb_index_map = DT->CGetBBGraphInfo()->bb_index_map;
      const auto TrueBB = DT->CGetBBNodeInfo(bb_index_map.at(BranchBB->true_edge))->block;
      const auto FalseBB = DT->CGetBBNodeInfo(bb_index_map.at(BranchBB->false_edge))->block;
      const std::vector<blocRef> SuccsToProcess = {TrueBB, FalseBB};

      auto* branch_var = GetPointer<ssa_name>(GET_NODE(BI->op0));
      THROW_ASSERT(branch_var, "Non SSA variable found in branch");
      auto* var_def = GetPointer<gimple_assign>(GET_NODE(branch_var->CGetDefStmt()));
      const auto condition = GET_NODE(var_def->op1);

      auto InsertHelper = [&](tree_nodeRef Op, tree_nodeRef Cond)
      {
         for(const auto& Succ : SuccsToProcess)
         {
            if(Succ == BranchBB)
            {
               continue;
            }

            bool TakenEdge = (Succ == TrueBB);

            PredicateBase* PB = new PredicateBranch(Op, BranchBB, Succ, Cond, TakenEdge);
            addInfoFor(Op, PB, OpsToRename, ValueInfoNums, ValueInfos);
            if(Succ->list_of_pred.size() == 1)
            {
               EdgeUsesOnly.insert({BranchBB, Succ});
            }
         }
      };

      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
         " <-- eSSA: branch statement in block " << BranchBB->number << " has condition of type " << condition->get_kind_text());

      // TODO: check if condition is a compare instruction or an and/or instruction
      if(auto* bin = GetPointer<binary_expr>(condition))
      {
         if(isCompare(bin))
         {
            const auto lhs = GET_NODE(bin->op0);
            const auto rhs = GET_NODE(bin->op1);

            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
               " <--eSSA process branch: if(" << bin->get_kind_text()<< ")");

            if(lhs != rhs)
            {
               InsertHelper(condition, condition);

               // TODO: add lhs and rhs using InsertHelper if they aren't constants and have more than one use
            }
         }
         else
         {
            InsertHelper(condition, condition);
         }
      }
      else
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " <--eSSA process branch: unhandled condition type");
      }
   }

   void renameUses(std::set<tree_nodeRef>& OpSet, 
      std::map<tree_nodeRef, unsigned int>& /*ValueInfoNums*/, std::vector<eSSAInfo::ValueInfo>& /*ValueInfos*/,
      std::set<std::pair<blocRef, blocRef>>& /*EdgeUsesOnly*/,
      int debug_level)
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " <--eSSA rename uses: time to rename");

      // This maps from copy operands to Predicate Info. Note that it does not own
      // the Predicate Info, they belong to the ValueInfo structs in the ValueInfos
      // vector.
      std::map<tree_nodeRef, const PredicateBase*> PredicateMap;
      // Sort OpsToRename since we are going to iterate it.
      std::vector<tree_nodeRef> OpsToRename(OpSet.begin(), OpSet.end());

      // TODO: sort OpsToRename using dominator tree

      /*
        commented since there is an unused variable
      for(auto& Op : OpsToRename)
      {
         // TODO: do stuff about Op
      }
*/
   }
}

eSSA::eSSA(const ParameterConstRef params, const application_managerRef AM, unsigned int f_id, const DesignFlowManagerConstRef dfm)
    : FunctionFrontendFlowStep(AM, f_id, ESSA, dfm, params)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

eSSA::~eSSA() = default;

const std::unordered_set<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> 
eSSA::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   std::unordered_set<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
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

DesignFlowStep_Status eSSA::InternalExec()
{
   auto TM = AppM->get_tree_manager();
   tree_nodeRef temp = TM->get_tree_node_const(function_id);
   auto* fd = GetPointer<function_decl>(temp);
   auto sl = GetPointer<statement_list>(GET_NODE(fd->body));
   /// store the GCC BB graph ala boost::graph
   BBGraphsCollectionRef GCC_bb_graphs_collection(new BBGraphsCollection(BBGraphInfoRef(new BBGraphInfo(AppM, function_id)), parameters));
   BBGraphRef GCC_bb_graph(new BBGraph(GCC_bb_graphs_collection, CFG_SELECTOR));
   std::unordered_map<unsigned int, vertex> inverse_vertex_map;
   /// add vertices
   for(auto block : sl->list_of_bloc)
   {
      inverse_vertex_map[block.first] = GCC_bb_graphs_collection->AddVertex(BBNodeInfoRef(new BBNodeInfo(block.second)));
   }
   /// add edges
   for(auto curr_bb_pair : sl->list_of_bloc)
   {
      unsigned int curr_bb = curr_bb_pair.first;
      std::vector<unsigned int>::const_iterator lop_it_end = sl->list_of_bloc[curr_bb]->list_of_pred.end();
      for(std::vector<unsigned int>::const_iterator lop_it = sl->list_of_bloc[curr_bb]->list_of_pred.begin(); lop_it != lop_it_end; ++lop_it)
      {
         THROW_ASSERT(inverse_vertex_map.find(*lop_it) != inverse_vertex_map.end(), "BB" + STR(*lop_it) + " (successor of BB" + STR(curr_bb) + ") does not exist");
         GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[*lop_it], inverse_vertex_map[curr_bb], CFG_SELECTOR);
      }
      std::vector<unsigned int>::const_iterator los_it_end = sl->list_of_bloc[curr_bb]->list_of_succ.end();
      for(std::vector<unsigned int>::const_iterator los_it = sl->list_of_bloc[curr_bb]->list_of_succ.begin(); los_it != los_it_end; ++los_it)
      {
         if(*los_it == bloc::EXIT_BLOCK_ID)
            GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[curr_bb], inverse_vertex_map[*los_it], CFG_SELECTOR);
      }
      if(sl->list_of_bloc[curr_bb]->list_of_succ.empty())
         GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[curr_bb], inverse_vertex_map[bloc::EXIT_BLOCK_ID], CFG_SELECTOR);
   }
   /// add a connection between entry and exit thus avoiding problems with non terminating code
   GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[bloc::ENTRY_BLOCK_ID], inverse_vertex_map[bloc::EXIT_BLOCK_ID], CFG_SELECTOR);

   refcount<dominance<BBGraph>> bb_dominators;
   bb_dominators = refcount<dominance<BBGraph>>(new dominance<BBGraph>(*GCC_bb_graph, inverse_vertex_map[bloc::ENTRY_BLOCK_ID], inverse_vertex_map[bloc::EXIT_BLOCK_ID], parameters));
   bb_dominators->calculate_dominance_info(dominance<BBGraph>::CDI_DOMINATORS);
   const std::unordered_map<vertex, vertex>& bb_dominator_map = bb_dominators->get_dominator_map();

   BBGraphRef DT(new BBGraph(GCC_bb_graphs_collection, D_SELECTOR));
   for(auto it : bb_dominator_map)
   {
      if(it.first != inverse_vertex_map[bloc::ENTRY_BLOCK_ID])
      {
         GCC_bb_graphs_collection->AddEdge(it.second, it.first, D_SELECTOR);
      }
   }

   std::deque<vertex> DTNs;
   boost::topological_sort(*DT, std::front_inserter(DTNs));

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
         " <-- eSSA: Dominator tree has " << DTNs.size() << " BB");

   // This stores info about each operand or comparison result we make copies
   // of.  The real ValueInfos start at index 1, index 0 is unused so that we can
   // more easily detect invalid indexing.
   std::vector<eSSAInfo::ValueInfo> ValueInfos;
   ValueInfos.resize(1);
   // This gives the index into the ValueInfos array for a given Value.  Because
   // 0 is not a valid Value Info index, you can use DenseMap::lookup and tell
   // whether it returned a valid result.
   std::map<tree_nodeRef, unsigned int> ValueInfoNums;
   // The set of edges along which we can only handle phi uses, due to critical edges.
   std::set<std::pair<blocRef, blocRef>> EdgeUsesOnly;

   // Collect operands to rename from all conditional branch terminators, as well
   // as assume statements.
   std::set<tree_nodeRef> OpsToRename;

   for(auto DTN : DTNs)
   {
      const auto BranchBB = DT->CGetBBNodeInfo(DTN)->block;
      const auto& stmt_list = BranchBB->CGetStmtList();

      // Skip entry/exit empty BB
      if(stmt_list.empty())
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
         " <-- eSSA: BB " << BranchBB->number << " empty");
         continue;
      }

      const auto terminator = GET_NODE(stmt_list.back());

      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
         " <-- eSSA: BB" << BranchBB->number << " has " << terminator->get_kind_text() << " as terminator");
      
      if(auto* BI = GetPointer<gimple_cond>(terminator))
      {
         // Can't insert conditional information if they all go to the same place.
         if(BranchBB->true_edge == BranchBB->false_edge)
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
               " <-- eSSA: branch true edge == false edge");
            continue;
         }

         eSSAInfo::processBranch(BI, BranchBB, OpsToRename, ValueInfoNums, ValueInfos, EdgeUsesOnly, DT, AppM, debug_level);
      }
      // TODO: add multi way if handler
   }

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
         " <-- eSSA: " << OpsToRename.size() << " operations to rename found");

   eSSAInfo::renameUses(OpsToRename, ValueInfoNums, ValueInfos, EdgeUsesOnly, debug_level);

   return DesignFlowStep_Status::UNCHANGED;
}

void eSSA::Initialize()
{
}
