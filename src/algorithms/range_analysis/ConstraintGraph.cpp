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
 *              Copyright (C) 2019-2026 Politecnico di Milano
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
 * @file ConstraintGraph.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "ConstraintGraph.hpp"

#include "BinaryOpNode.hpp"
#include "IR/function_ordered_instructions.hpp"
#include "Meet.hpp"
#include "Nuutila.hpp"
#include "OpNode.hpp"
#include "Parameter.hpp"
#include "PhiOpNode.hpp"
#include "SigmaOpNode.hpp"
#include "SymbValueRange.hpp"
#include "UnaryOpNode.hpp"
#include "ValueRange.hpp"
#include "ValueSetRange.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "range_analysis_helper.hpp"
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <sstream>

// #define CAST_TRAVERSE
#define RA_JUMPSET

class VarUse
{
 public:
   VarUse(VarNode* var, OpNode* op) : _var(var), _inst(op->getInstruction()), _op(op)
   {
   }

   VarUse(VarNode* var, ir_nodeConstRef inst) : _var(var), _inst(std::move(inst)), _op(nullptr)
   {
   }

   unsigned long long getId() const
   {
      return (static_cast<unsigned long long>(_var->getValue()->index) << 32U) | _inst->index;
   }

   VarNode* getOperand() const
   {
      return _var;
   }

   ir_nodeConstRef getInstruction() const
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
   ir_nodeConstRef _inst;
   OpNode* _op;
};

// Mixin class for edge predicates.  The FROM block is the block where the
// predicate originates, and the TO block is the block where the predicate is
// valid.
struct PredicateWithEdge
{
   // The original operand before we renamed it.
   // This can be use by passes, when destroying predicateinfo, to know
   // whether they can just drop the intrinsic, or have to merge metadata.
   ir_nodeConstRef OriginalOp;

   unsigned int From;
   unsigned int To;

   ValueRangeRef intersect;

   explicit PredicateWithEdge(ir_nodeConstRef Op, unsigned int _From, unsigned int _To, ValueRangeRef _intersect)
       : OriginalOp(std::move(Op)), From(_From), To(_To), intersect(std::move(_intersect))
   {
      THROW_ASSERT(_From && _To, "");
   }
};

ValueDFS::ValueDFS(DFSInfo _DIndex, unsigned int _LocalNum, PredicateWithEdge* _PInfo, bool _EdgeOnly)
    : DIndex(_DIndex), LocalNum(_LocalNum), PInfo(_PInfo), EdgeOnly(_EdgeOnly)
{
}

ValueDFS::ValueDFS(DFSInfo _DIndex, unsigned int _LocalNum, VarNode* var, OpNode* op, bool _EdgeOnly)
    : DIndex(_DIndex), LocalNum(_LocalNum), U(new VarUse(var, op)), EdgeOnly(_EdgeOnly)
{
}

std::string ValueDFS::ToString() const
{
   std::stringstream ss;
   if(PInfo)
   {
      ss << "Predicate info: " << PInfo->OriginalOp->ToString() << " Edge: " << PInfo->From << " -> " << PInfo->To;
   }
   else
   {
      ss << "Def: " << (Def ? Def->ToString() : "null") << " Use: " << (U ? U->getUser()->ToString() : "null");
   }
   ss << " EdgeOnly: " << (EdgeOnly ? "true" : "false") << " DFS: (" << DIndex.DFSIn << ", " << DIndex.DFSOut << ", "
      << (LocalNum == LN_First ? "first" : (LocalNum == LN_Middle ? "middle" : "last")) << ")";
   return ss.str();
}

namespace
{
   const size_t _fixed_iterations_count = 16L;
   const unsigned int _default_disjoint_interval_max = 8;

   class ValueInfoMap
   {
    public:
      // Used to store information about each value we might rename.
      struct ValueInfo
      {
         // Information about each possible copy. During processing, this is each
         // inserted info. After processing, we move the uninserted ones to the
         // uninserted vector.
         std::vector<PredicateWithEdge*> Infos;
         std::vector<PredicateWithEdge*> UninsertedInfos;
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
      CustomMap<unsigned int, DFSInfo> DFSInfos;

      ValueInfoMap ValueInfos;

      /* The set of edges along which we can only handle phi uses, due to critical edges. */
      CustomSet<std::pair<unsigned int, unsigned int>> EdgeUsesOnly;

      /* Collect operands to rename from all conditional branch terminators, as well as multi-way if. */
      std::vector<VarUseRef> OpsToRename;
   };

   struct IRVisitor : public boost::default_dfs_visitor
   {
    public:
      using BBMap = decltype(statement_list_node::list_of_bloc);

      IRVisitor(RenameInfos& infos, NodeContainer* nc, unsigned int function_id, application_managerRef _AppM,
                bool computeESSA
#ifndef NDEBUG
                ,
                int _debug_level
#endif
      );

      void discover_vertex(BBGraph::vertex_descriptor u, const BBGraph& g);

      void finish_vertex(BBGraph::vertex_descriptor u, const BBGraph& g);

    private:
      unsigned int _step;
      RenameInfos& _infos;
      NodeContainer* const _nc;
      const BBMap& bb_map;
      const unsigned int _function_id;
      const FunctionBehaviorConstRef FB;
      const application_managerRef AppM;
      const bool _computeESSA;
      unsigned int max_disjoint_intervals;
#ifndef NDEBUG
      int debug_level;
#endif

      void addInfoFor(VarUseRef Op, PredicateWithEdge* PB);

      void insertEdgePredicate(unsigned int sourceBBI, const blocRef& targetBB, VarNode* Op,
                               const ValueRangeRef& intersect, const ir_nodeConstRef& userStmt);
      void insertEdgePredicateMerged(unsigned int sourceBBI, const blocRef& targetBB, VarNode* Op,
                                     const ValueRangeRef& intersect, const ir_nodeConstRef& userStmt);

      void processMultiWayIf(const ir_nodeConstRef& tn);
   };

   IRVisitor::IRVisitor(RenameInfos& infos, NodeContainer* nc, unsigned int function_id, application_managerRef _AppM,
                        bool computeESSA
#ifndef NDEBUG
                        ,
                        int _debug_level
#endif
                        )
       : _step(0),
         _infos(infos),
         _nc(nc),
         bb_map(GetPointerS<const statement_list_node>(
                    GetPointerS<const function_val_node>(_AppM->get_ir_manager()->GetIRNode(function_id))->body)
                    ->list_of_bloc),
         _function_id(function_id),
         FB(_AppM->CGetFunctionBehavior(function_id)),
         AppM(std::move(_AppM)),
         _computeESSA(computeESSA),
         max_disjoint_intervals(_default_disjoint_interval_max)
#ifndef NDEBUG
         ,
         debug_level(_debug_level)
#endif
   {
      const auto params = AppM->get_parameter();
      if(params && params->IsParameter("ra-disjoint-interval-max"))
      {
         max_disjoint_intervals = params->GetParameter<unsigned int>("ra-disjoint-interval-max");
      }
      if(max_disjoint_intervals == 0)
      {
         max_disjoint_intervals = 1;
      }
   }

   void IRVisitor::discover_vertex(BBGraph::vertex_descriptor u, const BBGraph& g)
   {
      const auto& BB = g.CGetNodeInfo(u).block;
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
      for(const auto& stmt : stmt_list)
      {
         if(range_analysis::isValidInstruction(stmt, FB))
         {
            _nc->addOperation(stmt, AppM);
         }
      }

      if(_computeESSA && !stmt_list.empty())
      {
         const auto& terminator = stmt_list.back();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Block terminates with " + terminator->get_kind_text() + " " + STR(terminator));
         if(terminator->get_kind() == multi_way_if_stmt_K)
         {
            processMultiWayIf(terminator);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
   }

   void IRVisitor::finish_vertex(BBGraph::vertex_descriptor u, const BBGraph& g)
   {
      const auto& BB = g.CGetNodeInfo(u).block;
      _infos.DFSInfos[BB->number].DFSOut = _step++;
   }

   void IRVisitor::addInfoFor(VarUseRef Op, PredicateWithEdge* PB)
   {
      auto& OperandInfo = _infos.ValueInfos[Op->getOperand()->getId()];
      OperandInfo.Infos.push_back(PB);
      _infos.OpsToRename.push_back(std::move(Op));
   }

   void IRVisitor::insertEdgePredicate(unsigned int sourceBBI, const blocRef& targetBB, VarNode* Op,
                                       const ValueRangeRef& intersect, const ir_nodeConstRef& userStmt)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Conditional intersect " + intersect->ToString() + " added for variable " + STR(Op->getValue()) +
                         " in BB" + STR(targetBB->number));

      auto* PB = new PredicateWithEdge(Op->getValue(), sourceBBI, targetBB->number, intersect);
      // TODO: not sure if multi_way_if_stmt statement is the correct user to be set in the following VarUse, since
      // it is not actually using Op
      addInfoFor(VarUseRef(new VarUse(Op, userStmt)), PB);
      if(targetBB->list_of_pred.size() > 1)
      {
         _infos.EdgeUsesOnly.insert({sourceBBI, targetBB->number});
      }
   }

   void IRVisitor::insertEdgePredicateMerged(unsigned int sourceBBI, const blocRef& targetBB, VarNode* Op,
                                             const ValueRangeRef& intersect, const ir_nodeConstRef& userStmt)
   {
      auto& OperandInfo = _infos.ValueInfos[Op->getId()];
      for(auto* existing : OperandInfo.Infos)
      {
         if(existing->From != sourceBBI || existing->To != targetBB->number)
         {
            continue;
         }
         if(SymbRange::classof(existing->intersect.get()) || SymbRange::classof(intersect.get()))
         {
            break;
         }

         auto toSet = [&](const ValueRangeRef& vr) -> range_analysis::ValueSet {
            if(auto vs = RefcountCast<const ValueSetRange>(vr))
            {
               return vs->getSet();
            }
            Range aux(Unknown, Op->getBitWidth());
            // If we cannot express the constraint as a single Range, fall back to the full domain.
            if(!vr->tryGetRange(aux))
            {
               return range_analysis::domainForVar(Op);
            }
            return range_analysis::rangeToValueSet(aux, Op);
         };

         const auto merged = range_analysis::intersectValueSets(toSet(existing->intersect), toSet(intersect));
         existing->intersect = range_analysis::toValueRange(Op, merged, max_disjoint_intervals, true);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Merged conditional intersect " + existing->intersect->ToString() + " for variable " +
                            STR(Op->getValue()) + " in BB" + STR(targetBB->number));
         return;
      }

      insertEdgePredicate(sourceBBI, targetBB, Op, intersect, userStmt);
   }

   void IRVisitor::processMultiWayIf(const ir_nodeConstRef& tn)
   {
      THROW_ASSERT(tn->get_kind() == multi_way_if_stmt_K, "Multi way if instruction should be multi_way_if_stmt");
      const auto* gmw = GetPointerS<const multi_way_if_stmt>(tn);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Multi way if with " + STR(gmw->list_of_cond.size()) + " conditions");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");

      const auto sourceBBI = gmw->bb_index;

      struct SimpleCond
      {
         enum class Op
         {
            EQ,
            NE,
            LT,
            LE,
            GT,
            GE
         };

         ir_nodeConstRef var;
         Op op;
         APInt cst;
         bool isSigned;
         Range::bw_t bw;
      };

      auto parseSimpleCond = [&](const ir_nodeConstRef& cond) -> std::optional<SimpleCond> {
         if(!cond || cond->get_kind() != ssa_node_K)
         {
            return std::nullopt;
         }
         const auto compare_stmt = range_analysis::castTraverse(cond);
         const auto Cond = compare_stmt->get_kind() == assign_stmt_K ?
                               GetPointerS<const assign_stmt>(compare_stmt)->op1 :
                               compare_stmt;

         const auto* be = GetPointer<const binary_node>(Cond);
         if(!be || !range_analysis::isCompare(be))
         {
            return std::nullopt;
         }

         const auto pred = range_analysis::op_convert(be->get_kind());
         SimpleCond::Op op;
         if(pred == eq_node_R || pred == unsigned_eq_node_R)
         {
            op = SimpleCond::Op::EQ;
         }
         else if(pred == ne_node_R)
         {
            op = SimpleCond::Op::NE;
         }
         else if(pred == lt_node_R || pred == unsigned_lt_node_R)
         {
            op = SimpleCond::Op::LT;
         }
         else if(pred == le_node_R || pred == unsigned_le_node_R)
         {
            op = SimpleCond::Op::LE;
         }
         else if(pred == gt_node_R || pred == unsigned_gt_node_R)
         {
            op = SimpleCond::Op::GT;
         }
         else if(pred == ge_node_R || pred == unsigned_ge_node_R)
         {
            op = SimpleCond::Op::GE;
         }
         else
         {
            return std::nullopt;
         }

         const auto& Op0 = be->op0;
         const auto& Op1 = be->op1;
         const bool c0 = ir_helper::IsConstant(Op0);
         const bool c1 = ir_helper::IsConstant(Op1);
         if(c0 == c1)
         {
            return std::nullopt;
         }

         const auto varOp = c0 ? Op1 : Op0;
         const auto cstOp = c0 ? Op0 : Op1;
         if(varOp->get_kind() != ssa_node_K || cstOp->get_kind() != constant_int_val_node_K)
         {
            return std::nullopt;
         }

         auto canonVar = range_analysis::castTraverseSSA(varOp);
         if(!canonVar || canonVar->get_kind() != ssa_node_K)
         {
            return std::nullopt;
         }

         // Preserve the comparison-domain signedness: traversing through nop_node
         // can expose the underlying unsigned SSA and over-constrain false branches
         // (e.g. (signed)x > 0 false -> x == 0 instead of x <= 0).
         const auto var_bw = static_cast<Range::bw_t>(ir_helper::TypeSize(varOp));
         const auto canon_bw = static_cast<Range::bw_t>(ir_helper::TypeSize(canonVar));
         const bool var_signed = range_analysis::isSignedType(varOp);
         const bool canon_signed = range_analysis::isSignedType(canonVar);
         if(var_bw != canon_bw || var_signed != canon_signed)
         {
            canonVar = varOp;
         }

         const auto bw = static_cast<Range::bw_t>(ir_helper::TypeSize(canonVar));
         const bool isUnsignedPred = pred == unsigned_eq_node_R || pred == unsigned_lt_node_R ||
                                     pred == unsigned_le_node_R || pred == unsigned_gt_node_R ||
                                     pred == unsigned_ge_node_R;
         const bool isSigned = isUnsignedPred ? false : range_analysis::isSignedType(canonVar);

         auto cstVal = ir_helper::GetConstValue(cstOp);
         const auto cst = cstVal.extOrTrunc(bw, isSigned);

         return SimpleCond{canonVar, op, cst, isSigned, bw};
      };

      auto buildClosed = [](const APInt& lb, const APInt& ub) -> range_analysis::ValueSet {
         if(ub < lb)
         {
            return range_analysis::ValueSet{};
         }
         range_analysis::ValueSet set;
         set += range_analysis::ValueInterval::closed(lb, ub);
         return set;
      };

      bool hasDefault = false;
      unsigned int defaultBBI = 0;
      ir_nodeConstRef switchVar = nullptr;
      Range::bw_t switchBw = 0;
      struct CaseEntry
      {
         SimpleCond cond;
         unsigned int targetBBI;
      };
      std::vector<CaseEntry> cases;
      cases.reserve(gmw->list_of_cond.size());

      const auto nConds = gmw->list_of_cond.size();
      size_t idx = 0;
      for(const auto& entry : gmw->list_of_cond)
      {
         const auto& cond = entry.first;
         const auto targetBBI = entry.second;
         if(!cond)
         {
            if(hasDefault)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "Multiple default branches in multi-way-if, skipping predicates");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               return;
            }
            if(idx + 1 != nConds)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "Default branch is not last in multi-way-if, skipping predicates");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               return;
            }
            hasDefault = true;
            defaultBBI = targetBBI;
            ++idx;
            continue;
         }
         if(hasDefault)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Conditional entry found after default, skipping predicates");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
            return;
         }

         auto parsed = parseSimpleCond(cond);
         if(!parsed)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Unable to parse condition, skipping all predicates for this multi-way-if");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
            return;
         }
         if(!switchVar)
         {
            switchVar = parsed->var;
            switchBw = parsed->bw;
         }
         else if(parsed->var->index != switchVar->index || parsed->bw != switchBw)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Mixed condition variables in multi-way-if, skipping predicates");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
            return;
         }
         cases.push_back(CaseEntry{*parsed, targetBBI});
         ++idx;
      }

      if(cases.empty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "No parsable cases, skipping predicates");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         return;
      }

      auto* varNode = _nc->addVarNode(switchVar, _function_id);
      auto domain = range_analysis::domainForVar(varNode);
      if(domain.empty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Domain is empty, skipping predicates");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         return;
      }
      auto subtractSet = [&](const range_analysis::ValueSet& base,
                             const range_analysis::ValueSet& cut) -> range_analysis::ValueSet {
         range_analysis::ValueSet result = base;
         result -= cut;
         return result;
      };
      auto clampIntervals = [&](range_analysis::ValueSet& set) {
         if(max_disjoint_intervals > 0 && range_analysis::intervalCount(set) > max_disjoint_intervals)
         {
            set = domain;
         }
      };

      auto condToSet = [&](const SimpleCond& c) -> range_analysis::ValueSet {
         const auto minV = c.isSigned ? APInt::getSignedMinValue(c.bw) : APInt::getMinValue(c.bw);
         const auto maxV = c.isSigned ? APInt::getSignedMaxValue(c.bw) : APInt::getMaxValue(c.bw);
         range_analysis::ValueSet set;
         switch(c.op)
         {
            case SimpleCond::Op::EQ:
               set = range_analysis::singletonSet(c.cst);
               break;
            case SimpleCond::Op::NE:
               set = subtractSet(domain, range_analysis::singletonSet(c.cst));
               break;
            case SimpleCond::Op::LT:
               if(c.cst <= minV)
               {
                  set = range_analysis::ValueSet{};
               }
               else
               {
                  set = buildClosed(minV, c.cst - Range::MinDelta);
               }
               break;
            case SimpleCond::Op::LE:
               if(c.cst < minV)
               {
                  set = range_analysis::ValueSet{};
               }
               else
               {
                  set = buildClosed(minV, c.cst);
               }
               break;
            case SimpleCond::Op::GT:
               if(c.cst >= maxV)
               {
                  set = range_analysis::ValueSet{};
               }
               else
               {
                  set = buildClosed(c.cst + Range::MinDelta, maxV);
               }
               break;
            case SimpleCond::Op::GE:
               if(c.cst > maxV)
               {
                  set = range_analysis::ValueSet{};
               }
               else
               {
                  set = buildClosed(c.cst, maxV);
               }
               break;
            default:
               set = domain;
               break;
         }
         set = range_analysis::intersectValueSets(set, domain);
         clampIntervals(set);
         return set;
      };

      range_analysis::ValueSet remaining = domain;
      std::map<unsigned int, range_analysis::ValueSet> edgeSets;
      auto addOrConstraint = [&](unsigned int target, const range_analysis::ValueSet& set) {
         if(set.empty())
         {
            return;
         }
         auto it = edgeSets.find(target);
         if(it == edgeSets.end())
         {
            edgeSets.emplace(target, set);
            return;
         }
         it->second = it->second + set;
         clampIntervals(it->second);
      };

      for(const auto& c : cases)
      {
         const auto condSet = condToSet(c.cond);
         const auto pathSet = range_analysis::intersectValueSets(remaining, condSet);
         addOrConstraint(c.targetBBI, pathSet);
         remaining = subtractSet(remaining, condSet);
         clampIntervals(remaining);
      }

      if(hasDefault)
      {
         addOrConstraint(defaultBBI, remaining);
      }

      for(const auto& entry : edgeSets)
      {
         const auto& targetBB = bb_map.at(entry.first);
         auto vr = range_analysis::toValueRange(varNode, entry.second, max_disjoint_intervals, true);
         insertEdgePredicateMerged(sourceBBI, targetBB, varNode, vr, tn);
      }

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }

   // Given a predicate info that is a type of branching terminator, get the
   // edge this predicate info represents
   std::pair<unsigned int, unsigned int> getBlockEdge(const PredicateWithEdge* PEdge)
   {
      return std::make_pair(PEdge->From, PEdge->To);
   }

   inline ir_nodeConstRef getDefOrUser(const ValueDFS& VD)
   {
      if(VD.Def)
      {
         return VD.Def->getInstruction();
      }
      if(VD.U)
      {
         return VD.U->getInstruction();
      }
      return nullptr;
   }

   // This compares ValueDFS structures, creating OrderedBasicBlocks where
   // necessary to compare uses/defs in the same block. Doing so allows us to walk
   // the minimum number of instructions necessary to compute our def/use ordering.
   struct ValueDFS_Compare
   {
      const OrderedInstructions& OI;
      explicit ValueDFS_Compare(const OrderedInstructions& _OI) : OI(_OI)
      {
      }

      // For a phi use, or a non-materialized def, return the edge it represents.
      std::pair<unsigned int, unsigned int> getBlockEdge_local(const ValueDFS& VD) const
      {
         if(!VD.Def && VD.U)
         {
            THROW_ASSERT(VD.U->getInstruction()->get_kind() == phi_stmt_K, "");
            const auto* gp = GetPointerS<const phi_stmt>(VD.U->getInstruction());
            auto phiDefEdge = std::find_if(
                gp->CGetDefEdgesList().begin(), gp->CGetDefEdgesList().end(),
                [&](const phi_stmt::DefEdge& de) { return de.first->index == VD.U->getOperand()->getValue()->index; });
            THROW_ASSERT(phiDefEdge != gp->CGetDefEdgesList().end(), "Unable to find variable in phi definitions");
            return std::make_pair(phiDefEdge->second, gp->bb_index);
         }
         // This is really a non-materialized def.
         return getBlockEdge(VD.PInfo);
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

         const auto SameBlock = A.DIndex == B.DIndex;

         // We want to put the def that will get used for a given set of phi uses,
         // before those phi uses.
         // So we sort by edge, then by def.
         // Note that only phi nodes uses and defs can come last.
         if(SameBlock && A.LocalNum == LN_Last && B.LocalNum == LN_Last)
         {
            const auto [srcA, tgtA] = getBlockEdge_local(A);
            const auto [srcB, tgtB] = getBlockEdge_local(B);
            // Now sort by block edge and then defs before uses.
            if(tgtA != tgtB)
            {
               return tgtA < tgtB;
            }
            auto defUseA = getDefOrUser(A);
            auto defUseB = getDefOrUser(B);
            THROW_ASSERT(defUseA || defUseB,
                         "Two sigma operations in the same basic block must have different predicate edges.");
            if(defUseA && defUseB)
            {
               return OI.dominates(defUseA, defUseB);
            }
            return !defUseA;
         }

         if(!SameBlock)
         {
            return A.DIndex < B.DIndex;
         }
         if(A.LocalNum != B.LocalNum)
         {
            return A.LocalNum < B.LocalNum;
         }
         auto defUseA = getDefOrUser(A);
         auto defUseB = getDefOrUser(B);
         THROW_ASSERT(defUseA && defUseB, "Non-materialized sigma operations are not expected here.");
         return OI.dominates(defUseA, defUseB);
      }
   };

   bool stackIsInScope(const ValueDFSStack& Stack, const ValueDFS& VDUse, const OrderedInstructions& OI)
   {
      if(Stack.empty())
      {
         return false;
      }
      // If it's a phi only use, make sure it's for this phi node edge, and that the use is in a phi node. If it's
      // anything else, and the top of the stack is EdgeOnly, we need to pop the stack. We deliberately sort phi uses
      // next to the defs they must go with so that we can know it's time to pop the stack when we hit the end of the
      // phi uses for a given def.
      if(Stack.back().EdgeOnly)
      {
         if(!VDUse.U || VDUse.U->getInstruction()->get_kind() != phi_stmt_K)
         {
            return false;
         }
         const auto* gp = GetPointerS<const phi_stmt>(VDUse.U->getInstruction());
         const auto& def_edges = gp->CGetDefEdgesList();
         auto is_use = [&](const phi_stmt::DefEdge& de) {
            return de.first->index == VDUse.U->getOperand()->getValue()->index;
         };
         // Check edge
         const auto de_it = std::find_if(def_edges.begin(), def_edges.end(), is_use);
         THROW_ASSERT(de_it != def_edges.end(), "Unable to find assumed use");
         if(de_it->second != Stack.back().PInfo->From)
         {
            return false;
         }

         const auto [edge_from, edge_to] = getBlockEdge(Stack.back().PInfo);
         if(gp->bb_index == edge_to && de_it->second == edge_from)
         {
            return true;
         }
         return OI.dominates(edge_from, de_it->second);
      }

      return Stack.back().DIndex <= VDUse.DIndex;
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
   void convertUsesToDFSOrdered(VarNode* Op, const NodeContainer::OpNodes& uses, std::vector<ValueDFS>& OrderedUses,
                                const CustomMap<unsigned int, DFSInfo>& DFSInfos
#ifndef NDEBUG
                                ,
                                int debug_level
#endif
   )
   {
      THROW_ASSERT(Op->getValue()->get_kind() == ssa_node_K,
                   "Op is not an ssa_node (" + Op->getValue()->get_kind_text() + ")");
      const auto* op = GetPointerS<const ssa_node>(Op->getValue());
      THROW_ASSERT(GetPointer<const node_stmt>(op->GetDefStmt()), "");
      const auto defBBI = GetPointerS<const node_stmt>(op->GetDefStmt())->bb_index;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(auto* userOp : uses)
      {
         const auto& user = userOp->getInstruction();
         if(!user)
         {
            // This is a materialized Sigma operation without a relative IR statement
            // TODO: this use should be also added to OrderedUses with the DFSInfo relative to the basic block where it
            // was previously materialized
            THROW_ASSERT(GetOp<SigmaOpNode>(userOp), "");
            // dfs_gen(userOp, stmt_bbi, LN_Last);
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Checking " + user->ToString());
         if(user->get_kind() == phi_stmt_K && GetPointerS<const phi_stmt>(user)->CGetDefEdgesList().size() > 1)
         {
            const auto* gp = GetPointerS<const phi_stmt>(user);
            for(const auto& [def, source_bbi] : gp->CGetDefEdgesList())
            {
               if(def->index == Op->getValue()->index)
               {
                  OrderedUses.emplace_back(DFSInfos.at(source_bbi), LN_Last, Op, userOp, true);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Pushed " + OrderedUses.back().ToString());
               }
            }
         }
         else
         {
            THROW_ASSERT(GetPointer<const node_stmt>(user), "Use statement should be a node_stmt");
            const auto* gn = GetPointerS<const node_stmt>(user);
            if(gn->bb_index == defBBI)
            {
               // Uses within the same basic block not interesting (they are casts or the actual branch eveluating the
               // condition)
               continue;
            }
            OrderedUses.emplace_back(DFSInfos.at(gn->bb_index), LN_Middle, Op, userOp);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Pushed " + OrderedUses.back().ToString());
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }

   /*
    *	This method builds a map that binds each variable label to the operations where this variable is used.
    */
   NodeContainer::UseMap buildUseMap(const CustomSet<VarNode*>& component, const NodeContainer::UseMap& uses)
   {
      NodeContainer::UseMap compUseMap;
      for(const auto* var : component)
      {
         // Get the component's use list for V (it does not exist until we try to get it)
         auto& list = compUseMap[var->getId()];
         // Get the use list of the variable in component
         const auto p = uses.at(var->getId());
         // For each operation in the list, verify if its sink is in the component
         for(auto* use : p)
         {
            auto* sink = use->getSink();
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
    * Create a vector containing all constants related to the component
    * They include:
    *   - Constants inside component
    *   - Constants that are source of an edge to an entry point
    *   - Constants from intersections generated by sigmas
    */
   std::vector<APInt> buildConstantVector(const CustomSet<VarNode*>& component, const NodeContainer::DefMap& defs,
                                          const NodeContainer::UseMap& compusemap)
   {
      // Remove all elements from the vector
      std::vector<APInt> constantvector;

      const auto insertConstantIntoVector = [&](const APInt& constantval, Range::bw_t bw) {
         constantvector.push_back(constantval.extOrTrunc(bw, true));
      };

      // Get constants inside component (TODO: may not be necessary, since
      // components with more than 1 node may
      // never have a constant inside them)
      for(const auto* varNode : component)
      {
         const auto& V = varNode->getValue();
         if(V->get_kind() == constant_int_val_node_K)
         {
            insertConstantIntoVector(ir_helper::GetConstValue(V), varNode->getBitWidth());
         }
      }

      // Get constants that are sources of operations whose sink belong to the component
      for(const auto* varNode : component)
      {
         auto dfit = defs.find(varNode->getId());
         if(dfit == defs.end())
         {
            continue;
         }

         auto pushConstFor = [&](const APInt& cst, Range::bw_t bw, kind_R pred) {
            if(range_analysis::isCompare(pred))
            {
               if(pred == eq_node_R || pred == ne_node_R)
               {
                  insertConstantIntoVector(cst, bw);
                  insertConstantIntoVector(cst - 1, bw);
                  insertConstantIntoVector(cst + 1, bw);
               }
               else if(pred == unsigned_eq_node_R)
               {
                  const auto ucst = cst.extOrTrunc(bw, false);
                  insertConstantIntoVector(ucst, bw);
                  insertConstantIntoVector(ucst - 1, bw);
                  insertConstantIntoVector(ucst + 1, bw);
               }
               else if(pred == gt_node_R || pred == le_node_R)
               {
                  insertConstantIntoVector(cst, bw);
                  insertConstantIntoVector(cst + 1, bw);
               }
               else if(pred == ge_node_R || pred == lt_node_R)
               {
                  insertConstantIntoVector(cst, bw);
                  insertConstantIntoVector(cst - 1, bw);
               }
               else if(pred == unsigned_gt_node_R || pred == unsigned_le_node_R)
               {
                  const auto ucst = cst.extOrTrunc(bw, false);
                  insertConstantIntoVector(ucst, bw);
                  insertConstantIntoVector(ucst + 1, bw);
               }
               else if(pred == unsigned_ge_node_R || pred == unsigned_lt_node_R)
               {
                  const auto ucst = cst.extOrTrunc(bw, false);
                  insertConstantIntoVector(ucst, bw);
                  insertConstantIntoVector(ucst - 1, bw);
               }
               else
               {
                  THROW_UNREACHABLE("unexpected condition");
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

            if(sourceval1->get_kind() == constant_int_val_node_K)
            {
               const auto bw = source1->getBitWidth();
               const auto cst_val = ir_helper::GetConstValue(sourceval1);
               pushConstFor(cst_val, bw, pred); // TODO: maybe should swap predicate for lhs constant?
            }
            if(sourceval2->get_kind() == constant_int_val_node_K)
            {
               const auto bw = source2->getBitWidth();
               const auto cst_val = ir_helper::GetConstValue(sourceval2);
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
               if(sourceval->get_kind() == constant_int_val_node_K)
               {
                  insertConstantIntoVector(ir_helper::GetConstValue(sourceval), source->getBitWidth());
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
            Range rintersect(Unknown, op->getSink()->getBitWidth());
            if(!op->getIntersect()->tryGetRange(rintersect))
            {
               continue;
            }
            const auto bw = rintersect.getBitWidth();
            if(rintersect.isAnti())
            {
               const auto anti = rintersect.getAnti();
               const auto& lb = anti.getLower();
               const auto& ub = anti.getUpper();
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
               const auto& lb = rintersect.getLower();
               const auto& ub = rintersect.getUpper();
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
      std::sort(constantvector.begin(), constantvector.end(), std::less{});
      constantvector.erase(std::unique(constantvector.begin(), constantvector.end()), constantvector.end());

      return constantvector;
   }

   /*
    * This method builds a map of variables to the lists of operations where
    * these variables are used as futures. Its C++ type should be something like
    * map<VarNode, List<Operation>>.
    */
   NodeContainer::UseMap buildSymbolicIntersectMap(const NodeContainer::OpNodes& ops)
   {
      // Creates the symbolic intervals map
      NodeContainer::UseMap symbMap;

      // Iterate over the operations set
      for(auto* op : ops)
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
               NodeContainer::OpNodes l;
               l.insert(op);
               symbMap.insert(std::make_pair(V, std::move(l)));
            }
         }
      }
      return symbMap;
   }

   void generateActivesVars(const CustomSet<VarNode*>& component,
                            std::set<VarNode::key_type, VarNode::key_compare>& activeVars)
   {
      for(const auto* varNode : component)
      {
         if(ir_helper::IsConstant(varNode->getValue()))
         {
            continue;
         }
         activeVars.insert(varNode->getId());
      }
   }

} // namespace

ConstraintGraph::ConstraintGraph(application_managerRef _AppM
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
      AppM(std::move(_AppM))
{
#ifndef NDEBUG
   NodeContainer::debug_level = debug_level;
   Meet::debug_level = graph_debug;
#endif
}

/**
 * @brief Perform the widening and narrowing operations
 *
 * @param compUseMap
 * @param actv
 * @param meet
 */
void ConstraintGraph::update(const UseMap& compUseMap, std::set<VarNode::key_type, VarNode::key_compare>& actv,
                             const std::function<bool(OpNode*, const std::vector<APInt>&)>& meet) const
{
#ifndef NDEBUG
   const auto& vars = getVarNodes();
#endif
   while(!actv.empty())
   {
      const auto V = *actv.begin();
      actv.erase(V);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "Forwarding " + vars.at(V)->ToString() + ":");

      // The use list.
      const auto& L = compUseMap.at(V);

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "-->");
      for(auto* op : L)
      {
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

void ConstraintGraph::update(size_t nIterations, const UseMap& compUseMap,
                             std::set<VarNode::key_type, VarNode::key_compare>& actv)
{
   std::deque<VarNode::key_type> queue(actv.begin(), actv.end());
   actv.clear();
   while(!queue.empty())
   {
      const auto V = queue.front();
      queue.pop_front();
      // The use list.
      const auto& L = compUseMap.at(V);
      for(auto* op : L)
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

// Given the renaming stack, make all the operands currently on the stack real by inserting them into the IR.  Return
// the last operation's value.
OpNode* ConstraintGraph::materializeStack(ValueDFSStack& RenameStack, unsigned int function_id, VarNode* OrigOp)
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
   // The maximum number of things we should be trying to materialize at once right now is 4, depending on if we had
   // an assume, a branch, and both used and of conditions.
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   for(auto RenameIter = RenameStack.end() - Start; RenameIter != RenameStack.end(); ++RenameIter)
   {
      auto* Op = OrigOp;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Checking variable " + Op->ToString());
      if(RenameIter != RenameStack.begin())
      {
         THROW_ASSERT((RenameIter - 1)->Def, "A valid definition shold be on the stack at this point");
         const auto* sigmaOp = GetOp<SigmaOpNode>((RenameIter - 1)->Def);
         THROW_ASSERT(sigmaOp, "Previous definition on stack should be a SigmaOpNode (" +
                                   (RenameIter - 1)->Def->ToString() + ")");
         Op = sigmaOp->getSink();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Moving check to " + Op->ToString());
      }
      ValueDFS& Result = *RenameIter;
      const auto* ValInfo = Result.PInfo;
      // For edge predicates, we can just place the operand in the block before
      // the terminator. For assume, we have to place it right before the assume
      // to ensure we dominate all of our uses. Always insert right before the
      // relevant instruction (terminator, assume), so that we insert in proper
      // order in the case of multiple predicateinfo in the same block.
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Inserting sigma in BB" + STR(ValInfo->To) + " with intersect " + ValInfo->intersect->ToString());

      auto* sink = addVarNode(Op->getValue(), function_id, ValInfo->To);
      THROW_ASSERT(sink->getId() != Op->getId(), "unexpected condition");
      Result.Def = pushOperation(new SigmaOpNode(ValInfo->intersect, sink, Op));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Materialized " + Result.Def->ToString());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   return RenameStack.back().Def;
}

void ConstraintGraph::generateEntryPoints(const CustomSet<VarNode*>& component,
                                          std::set<VarNode::key_type, VarNode::key_compare>& entryPoints) const
{
   const auto& defs = getDefs();
   // Iterate over the varnodes in the component
   for(const auto* varNode : component)
   {
      const auto& V = varNode->getValue();
      if(V->get_kind() == ssa_node_K)
      {
         const auto* ssa = GetPointerS<const ssa_node>(V);
         if(ssa->GetDefStmt()->get_kind() == phi_stmt_K)
         {
            const auto* phi_def = GetPointerS<const phi_stmt>(ssa->GetDefStmt());
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
      if(!varNode->getRange().isUnknown())
      {
         entryPoints.insert(varNode->getId());
      }
   }
}

/*
 * This method evaluates once each operation that uses a variable in
 * component, so that the next SCCs after component will have entry
 * points to kick start the range analysis algorithm.
 */
void ConstraintGraph::propagateToNextSCC(const CustomSet<VarNode*>& component) const
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
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "FORWARD:: " + op->ToString() + ":");
         auto sigmaop = GetOp<SigmaOpNode>(op);
         const auto newInterval = op->eval();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        std::string(!op->getSink()->getRange().isSameRange(newInterval) ? "      new " : "          ") +
                            op->getSink()->getRange().ToString() + " -> " + newInterval.ToString());
         op->getSink()->setRange(newInterval);
         if(sigmaop)
         {
            Range aux(Unknown, sigmaop->getSink()->getBitWidth());
            if(!sigmaop->getIntersect()->tryGetRange(aux) || aux.isUnknown())
            {
               sigmaop->markUnresolved();
            }
         }
      }
   }
}

void ConstraintGraph::solveFutures(const CustomSet<VarNode*>& component, const UseMap& symbMap) const
{
   // Iterate again over the varnodes in the component
   for(auto* varNode : component)
   {
      solveFuturesSC(varNode, symbMap);
   }
}

void ConstraintGraph::solveFuturesSC(VarNode* varNode, const UseMap& symbMap) const
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

/// Iterates through all instructions in the function and builds the graph.
void ConstraintGraph::buildGraph(unsigned int function_id, bool computeESSA)
{
#ifndef NDEBUG
   const auto TM = AppM->get_ir_manager();
   const auto fnode = TM->GetIRNode(function_id);
   const auto* fd = GetPointerS<const function_val_node>(fnode);
   const auto* sl = GetPointerS<const statement_list_node>(fd->body);
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "Analysing function " + ir_helper::GetFunctionName(fnode) + " with " + STR(sl->list_of_bloc.size()) +
                      " blocks");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");

   FunctionOrderedInstructions function_ordered_instructions(AppM, function_id);
   const auto& dt = function_ordered_instructions.getDT();
   const auto& OI = function_ordered_instructions.getOrderedInstructions();

   RenameInfos infos;
   {
      const auto entryVertex = dt.CGetGraphInfo().bb_index_map.at(bloc::ENTRY_BLOCK_ID);
      IRVisitor bv(infos, this, function_id, AppM, computeESSA
#ifndef NDEBUG
                   ,
                   debug_level
#endif
      );
      std::vector<boost::default_color_type> color_vec(dt.num_vertices(), boost::white_color);
      boost::depth_first_visit(dt, entryVertex, bv,
                               boost::make_iterator_property_map(color_vec.begin(), boost::get(boost::vertex_index, dt),
                                                                 boost::white_color));
   }

   if(computeESSA)
   {
      auto& DFSInfos = infos.DFSInfos;
      auto& OpsToRename = infos.OpsToRename;
      THROW_ASSERT(static_cast<size_t>(DFSInfos.size()) == dt.num_vertices(),
                   "Discovered " + STR(DFSInfos.size()) + "/" + STR(dt.num_vertices()) + " vertices.");

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Analysis detected " + STR(OpsToRename.size()) + " operations to rename");

      if(!OpsToRename.empty())
      {
         CustomMap<std::pair<unsigned int, unsigned int>, blocRef> interBranchBBs;

         // Sort OpsToRename since we are going to iterate it.
#if HAVE_ASSERTS
         for(const auto& vuse : OpsToRename)
         {
            THROW_ASSERT(vuse->getInstruction(),
                         "Missing instruction for use of " + STR(vuse->getOperand()->getValue()));
         }
#endif
         std::sort(OpsToRename.begin(), OpsToRename.end(), [&](const VarUseRef& A, const VarUseRef& B) {
            return OI.dominates(A->getInstruction(), B->getInstruction());
         });
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
            // They will become real copies if we find a real use for them, and never created otherwise.
            for(auto* PossibleCopy : ValueInfo.Infos)
            {
               // If we can only do phi uses, we treat it like it's in the branch block, and handle it specially.
               // We know that it goes last, and only dominate phi uses.
               const auto& BlockEdge = getBlockEdge(PossibleCopy);
               if(infos.EdgeUsesOnly.count(BlockEdge))
               {
                  // If we can only do phi uses, we treat it like it's in the branch block, and handle it
                  // specially. We know that it goes last, and only dominate phi uses.
                  THROW_ASSERT(DFSInfos.contains(BlockEdge.first), "Invalid DT node");
                  OrderedUses.emplace_back(DFSInfos.at(BlockEdge.first), LN_Last, PossibleCopy, true);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Possible copy BB" + STR(BlockEdge.first) +
                                     " last: " + OrderedUses.back().ToString());
               }
               else
               {
                  THROW_ASSERT(DFSInfos.contains(BlockEdge.second), "Invalid DT node");
                  OrderedUses.emplace_back(DFSInfos.at(BlockEdge.second), LN_First, PossibleCopy);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Possible copy BB" + STR(BlockEdge.second) +
                                     " first: " + OrderedUses.back().ToString());
               }
            }

            convertUsesToDFSOrdered(Op->getOperand(), getUses().at(Op->getOperand()->getId()), OrderedUses, DFSInfos
#ifndef NDEBUG
                                    ,
                                    debug_level
#endif
            );
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

               // We currently do not materialize copy over copy, but we should decide if we want to.
               bool PossibleCopy = VD.PInfo != nullptr;
#ifndef NDEBUG
               if(RenameStack.empty())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "RenameStack empty");
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "RenameStack top DFS numbers are (" + STR(RenameStack.back().DIndex.DFSIn) + "," +
                                     STR(RenameStack.back().DIndex.DFSOut) + ")");
               }
#endif
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "Current DFS numbers are (" + STR(VD.DIndex.DFSIn) + "," + STR(VD.DIndex.DFSOut) + ")");
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

               // If the possible copy dominates something, materialize our stack up to this point. This ensures
               // every comparison that affects our operation ends up with predicateinfo.
               if(!Result.Def)
               {
                  Result.Def = materializeStack(RenameStack, function_id, Op->getOperand());
               }

               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Found replacement " + Result.Def->ToString() + " for " +
                                  VD.U->getOperand()->ToString() + " in " + VD.U->getUser()->ToString());
               auto& varUses = getUses();
               if(auto* phiOp = GetOp<PhiOpNode>(VD.U->getUser()))
               {
                  const auto* gp = GetPointerS<const phi_stmt>(phiOp->getInstruction());
                  size_t opIdx = 0;
                  bool allReplaced = true;
                  for(const auto& [op, edge_bbi] : gp->CGetDefEdgesList())
                  {
                     if(op->index == VD.U->getOperand()->getValue()->index)
                     {
                        if(VD.DIndex <= OI.info(edge_bbi))
                        {
                           THROW_ASSERT(phiOp->getSource(opIdx)->getValue()->index ==
                                            Result.Def->getSink()->getValue()->index,
                                        "");
                           phiOp->replaceSource(opIdx, Result.Def->getSink());
                        }
                        else
                        {
                           allReplaced = false;
                        }
                     }
                     ++opIdx;
                  }
                  if(allReplaced)
                  {
                     varUses.at(VD.U->getOperand()->getId()).erase(VD.U->getUser());
                  }
               }
               else
               {
                  Range aux(Unknown, Result.Def->getSink()->getBitWidth());
                  if(!Result.Def->getIntersect()->tryGetRange(aux))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Skip set-based replacement in non-phi use (unmappable predicate)");
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
                     continue;
                  }
                  if(aux.isAnti())
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Skip anti-range replacement in non-phi use (being conservative to avoid "
                                    "erratic range narrowing)");
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
                     continue;
                  }
                  varUses.at(VD.U->getOperand()->getId()).erase(VD.U->getUser());
                  VD.U->updateUse(Result.Def->getSink());
               }
               varUses.at(Result.Def->getSink()->getId()).insert(VD.U->getUser());
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
}

void ConstraintGraph::findIntervals(
#ifndef NDEBUG
    const ParameterConstRef& parameters, const std::string& step_name
#endif
)
{
   // Initializes the nodes and the use map structure.
   const auto& defs = getDefs();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "Initialization:");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "-->");
   for(auto& [id, var] : getVarNodes())
   {
      var->init(!defs.count(id));
      if(!var->getRange().isUnknown() && !ir_helper::IsConstant(var->getValue()))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, var->ToString());
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "<--");

   const auto symbMap = buildSymbolicIntersectMap(getOpNodes());
   Nuutila sccList(getVarNodes(), getUses(), symbMap);

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
         auto* var = *component.begin();
         solveFuturesSC(var, symbMap);
         const auto varDef = defs.find(var->getId());
         if(varDef != defs.end())
         {
            const auto* op = varDef->second;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "FIXED:: " + op->ToString() + ":");
            const auto newInterval = op->eval();
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           std::string(!var->getRange().isSameRange(newInterval) ? "    new " : "        ") +
                               var->getRange().ToString() + " -> " + newInterval.ToString());
            var->setRange(newInterval);
         }
         if(var->getRange().isUnknown())
         {
            var->setRange(var->getMaxRange());
         }
      }
      else
      {
         const auto compUseMap = buildUseMap(component, getUses());

#ifndef NDEBUG
         const auto printComponentDot = [&](const std::string& graph_id) {
            VarNodes vars;
            OpNodes ops;
            for(const auto& [var_id, uses] : compUseMap)
            {
               for(auto* use : uses)
               {
                  for(auto* op : use->getSources())
                  {
                     vars.emplace(op->getId(), op);
                  }
                  vars.emplace(use->getSink()->getId(), use->getSink());
                  ops.emplace(use);
               }
            }
            return printDot(step_name + "." + graph_id + "." + STR(getVarNodes().at(n)->getValue()->index) + ".dot",
                            parameters, vars, ops)
                .string();
         };
#endif

#ifdef RA_JUMPSET
         // Create vector of constants inside component
         // Comment this line below to deactivate jump-set
         constantvector = buildConstantVector(component, defs, compUseMap);
#ifndef NDEBUG
         if(DEBUG_LEVEL_VERY_PEDANTIC <= graph_debug && !constantvector.empty())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug,
                           "Constant lattice: {-inf, " + container_to_string(constantvector, ", ") + ", +inf}");
         }
#endif
#endif

         // Get the entry points of the SCC
         std::set<VarNode::key_type, VarNode::key_compare> entryPoints;
#ifndef NDEBUG
         const auto printEntryFor = [&](const std::string& mType) {
            const auto& vars = getVarNodes();
            if(DEBUG_LEVEL_VERY_PEDANTIC <= graph_debug)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, mType + " step entry points:");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "-->");
               for(const auto& el : entryPoints)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, vars.at(el)->ToString());
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
                        "Printed constraint graph to " + printComponentDot("fixed"));

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
                        "Printed constraint graph to " + printComponentDot("futures"));

         for(const auto varNode : component)
         {
            if(varNode->getRange().isUnknown())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "initialize unknown: " + varNode->ToString());
               //    THROW_UNREACHABLE("unexpected condition");
               varNode->setRange(varNode->getMaxRange());
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug,
                        "Printed constraint graph to " + printComponentDot("int"));

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
                  "Printed final constraint graph to " + printDot(step_name + ".constraints.dot", parameters).string());
}

#ifndef NDEBUG
std::filesystem::path ConstraintGraph::printDot(const std::filesystem::path& file_name,
                                                const ParameterConstRef& parameters,
                                                const NodeContainer::VarNodes& vars,
                                                const NodeContainer::OpNodes& ops) const
{
   const auto output_directory = parameters->getOption<std::filesystem::path>(OPT_dot_directory) / "RangeAnalysis";
   if(!std::filesystem::exists(output_directory))
   {
      std::filesystem::create_directories(output_directory);
   }
   const auto full_name = output_directory / file_name;
   std::ofstream os(full_name);

   // Print the header of the .dot file.
   os << "digraph G {\n";

   size_t idx = 0, op_start;
   std::map<VarNode::key_type, size_t, VarNode::key_compare> var_idx;
   for(const auto& [var_id, var] : vars)
   {
      var_idx.emplace(var->getId(), idx);
      os << idx++ << "[shape=ellipse, label=\"" << var << "\"]\n";
   }
   op_start = idx;
   for(const auto* op : ops)
   {
      os << idx++ << "[shape=box, label=\"" << op->getName() << " ";
      op->getIntersect()->print(os);
      os << "\"]\n";
   }
   for(const auto* op : ops)
   {
      for(auto* operand : op->getSources())
      {
         auto v_it = var_idx.find(operand->getId());
         if(v_it != var_idx.end())
         {
            os << v_it->second << "->" << op_start << "\n";
         }
      }
      auto v_it = var_idx.find(op->getSink()->getId());
      if(v_it != var_idx.end())
      {
         os << op_start++ << "->" << v_it->second << "\n";
      }
   }
   THROW_ASSERT(idx == op_start, "");
   os << "}\n";

   return full_name;
}
#endif
