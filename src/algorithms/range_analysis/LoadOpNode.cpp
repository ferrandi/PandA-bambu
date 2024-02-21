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
 * @file LoadOpNode.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "LoadOpNode.hpp"

#include "NodeContainer.hpp"
#include "application_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "memory.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#ifndef NDEBUG
bool _ra_enable_load = true;
#endif

LoadOpNode::LoadOpNode(VarNode* _sink, const tree_nodeConstRef& _inst) : OpNode(_sink, _inst)
{
}

OpNode::OpNodeType LoadOpNode::getValueId() const
{
   return OpNodeType::OpNodeType_Load;
}

std::vector<VarNode*> LoadOpNode::getSources() const
{
   return sources;
}

void LoadOpNode::replaceSource(VarNode* _old, VarNode* _new)
{
   for(auto& src : sources)
   {
      if(_old->getId() == src->getId())
      {
         src = _new;
      }
   }
}

RangeRef LoadOpNode::eval() const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, ToString());

#ifndef NDEBUG
   if(getNumSources() == 0 || !_ra_enable_load)
#else
   if(getNumSources() == 0)
#endif
   {
      THROW_ASSERT(getSink()->getBitWidth() == getIntersect()->getRange()->getBitWidth(),
                   "Sink (" + GET_CONST_NODE(getSink()->getValue())->ToString() + ") has bitwidth " +
                       std::to_string(+getSink()->getBitWidth()) + " while intersect has bitwidth " +
                       std::to_string(+getIntersect()->getRange()->getBitWidth()));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "= " + getIntersect()->getRange()->ToString());
      return RangeRef(getIntersect()->getRange()->clone());
   }

   // Iterate over the sources of the load
   auto result = tree_helper::TypeRange(getSink()->getValue(), Empty);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   for(const VarNode* varNode : sources)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "  ->" + varNode->getRange()->ToString() + " " + varNode->ToString());
      result = result->unionWith(varNode->getRange());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--  = " + result->ToString());

   bool test = getIntersect()->getRange()->isFullSet();
   if(!test)
   {
      const auto aux = getIntersect()->getRange();
      auto _intersect = result->intersectWith(aux);
      if(!_intersect->isEmpty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---aux = " + aux->ToString() + " from " + getIntersect()->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---result = " + _intersect->ToString());
         result = _intersect;
      }
   }
   return result;
}

static RangeRef constructor_range(const tree_managerConstRef TM, const tree_nodeConstRef tn, const RangeConstRef init)
{
   THROW_ASSERT(tn->get_kind() == constructor_K, "tn is not constructor node");
   const auto* c = GetPointer<const constructor>(tn);
   std::vector<unsigned long long> array_dims;
   unsigned long long elements_bitsize;
   tree_helper::get_array_dim_and_bitsize(TM, GET_INDEX_CONST_NODE(c->type), array_dims, elements_bitsize);
   unsigned int initialized_elements = 0;
   auto ctor_range = RangeRef(init->clone());
   for(const auto& i : c->list_of_idx_valu)
   {
      const auto el = GET_CONST_NODE(i.second);
      THROW_ASSERT(el, "unexpected condition");

      if(el->get_kind() == constructor_K && tree_helper::IsArrayEquivType(GetPointerS<const constructor>(el)->type))
      {
         THROW_ASSERT(array_dims.size() > 1 || GET_CONST_NODE(c->type)->get_kind() == record_type_K,
                      "invalid nested constructors:" + tn->ToString() + " " + std::to_string(array_dims.size()));
         ctor_range = ctor_range->unionWith(constructor_range(TM, el, ctor_range));
      }
      else
      {
         const auto init_range = tree_helper::Range(el);
         if(init_range->getBitWidth() > static_cast<Range::bw_t>(elements_bitsize))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                           "---Initializer value not compliant " + el->ToString());
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                           "---Initializer value is " + el->ToString());
            ctor_range = ctor_range->unionWith(init_range);
         }
      }
      initialized_elements++;
   }
   if(initialized_elements < array_dims.front())
   {
      ctor_range =
          ctor_range->unionWith(RangeRef(new Range(Regular, static_cast<Range::bw_t>(elements_bitsize), 0, 0)));
   }
   return ctor_range;
}

std::function<OpNode*(NodeContainer*)> LoadOpNode::opCtorGenerator(const tree_nodeConstRef& stmt,
                                                                   const application_managerRef& AppM)
{
   const auto ga = GetPointer<const gimple_assign>(GET_CONST_NODE(stmt));
   if(ga == nullptr)
   {
      return nullptr;
   }
   const auto FB = AppM->CGetFunctionBehavior(GET_INDEX_CONST_NODE(ga->scpe));
   if(!tree_helper::IsLoad(stmt, FB->get_function_mem()))
   {
      return nullptr;
   }
   return [stmt, ga, FB, AppM](NodeContainer* NC) {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                     "Analysing load operation " + ga->ToString());
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "-->");
      const auto bw = static_cast<Range::bw_t>(tree_helper::TypeSize(ga->op0));
      const auto function_id = GET_INDEX_CONST_NODE(ga->scpe);
      const auto sink = NC->addVarNode(ga->op0, function_id);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                     "Sink variable is " + GET_CONST_NODE(ga->op0)->get_kind_text() +
                         " (size = " + std::to_string(+bw) + ")");

      auto intersection = tree_helper::TypeRange(sink->getValue(), Empty);
      if(GET_NODE(ga->op1)->get_kind() == array_ref_K || GET_NODE(ga->op1)->get_kind() == mem_ref_K ||
         GET_NODE(ga->op1)->get_kind() == target_mem_ref_K || GET_NODE(ga->op1)->get_kind() == target_mem_ref461_K ||
         GET_NODE(ga->op1)->get_kind() == var_decl_K)
      {
         const auto TM = AppM->get_tree_manager();
         const auto base_var = tree_helper::GetBaseVariable(ga->op1);
         const auto base_var_id = GET_INDEX_CONST_NODE(base_var);
         const auto* hm = GetPointer<HLS_manager>(AppM);
         if(base_var && AppM->get_written_objects().find(base_var_id) == AppM->get_written_objects().end() && hm &&
            hm->Rmem && FB->is_variable_mem(base_var_id) && hm->Rmem->is_sds_var(base_var_id))
         {
            const auto* vd = GetPointer<const var_decl>(GET_CONST_NODE(base_var));
            if(vd && vd->init)
            {
               if(GET_NODE(vd->init)->get_kind() == constructor_K)
               {
                  intersection = constructor_range(TM, GET_CONST_NODE(vd->init), intersection);
               }
               else if(GetPointer<const cst_node>(GET_CONST_NODE(vd->init)))
               {
                  auto init_range = tree_helper::Range(vd->init);
                  if(init_range->getBitWidth() != bw)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                                    "---Initializer value not compliant " + GET_NODE(vd->init)->ToString());
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                                    "---Initializer value is " + GET_NODE(vd->init)->ToString());
                     intersection = init_range;
                  }
               }
            }
         }
         if(base_var && AppM->get_written_objects().find(base_var_id) != AppM->get_written_objects().end() && hm &&
            hm->Rmem && hm->Rmem->get_enable_hls_bit_value() && FB->is_variable_mem(base_var_id) &&
            hm->Rmem->is_private_memory(base_var_id) && hm->Rmem->is_sds_var(base_var_id))
         {
            const auto* vd = GetPointer<const var_decl>(GET_CONST_NODE(base_var));
            if(vd && vd->init)
            {
               if(GET_NODE(vd->init)->get_kind() == constructor_K)
               {
                  intersection = constructor_range(TM, GET_CONST_NODE(vd->init), intersection);
               }
               else if(GetPointer<const cst_node>(GET_CONST_NODE(vd->init)))
               {
                  auto init_range = tree_helper::Range(vd->init);
                  if(init_range->getBitWidth() != bw)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                                    "---Initializer value not compliant " + GET_NODE(vd->init)->ToString());
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                                    "---Initializer value is " + GET_NODE(vd->init)->ToString());
                     intersection = init_range;
                  }
               }
            }
            else
            {
               intersection = RangeRef(new Range(Regular, bw, 0, 0));
            }
            for(const auto& cur_var : hm->Rmem->get_source_values(base_var_id))
            {
               const auto cur_node = TM->CGetTreeReindex(cur_var);
               THROW_ASSERT(cur_node, "");
               auto init_range = tree_helper::Range(cur_node);
               if(init_range->getBitWidth() != bw)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                                 "---Initializer value not compliant " + cur_node->ToString());
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                                 "---Initializer value is " + cur_node->ToString());
                  intersection = intersection->unionWith(init_range);
               }
            }
         }
      }
      if(intersection->isEmpty())
      {
         intersection = tree_helper::Range(stmt);
      }
      THROW_ASSERT(intersection->getBitWidth() <= bw,
                   "Pointed variables range should have bitwidth contained in sink bitwidth");
      THROW_ASSERT(!intersection->isEmpty(), "Variable range should not be empty");
      if(intersection->getBitWidth() < bw)
      {
         intersection = intersection->zextOrTrunc(bw);
      }
      auto BI =
#ifndef NDEBUG
          !_ra_enable_load ? ValueRangeRef(new ValueRange(tree_helper::Range(stmt))) :
#endif
                             ValueRangeRef(new ValueRange(intersection));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                     "<--Added LoadOp with range " + BI->ToString());
      auto loadOp = new LoadOpNode(sink, stmt);
      loadOp->setIntersect(BI);
      return loadOp;
   };
}

void LoadOpNode::print(std::ostream& OS) const
{
   OS << getSink()->getValue() << " = LOAD()";
}

void LoadOpNode::printDot(std::ostream& OS) const
{
   OS << " \"" << this << "\" [label=\"LoadOp\"]\n";

   for(auto src : sources)
   {
      const auto& V = src->getValue();
      if(tree_helper::IsConstant(V))
      {
         OS << " " << tree_helper::GetConstValue(V) << " -> \"" << this << "\"\n";
      }
      else
      {
         OS << " \"" << V << "\" -> \"" << this << "\"\n";
      }
   }

   const auto& VS = getSink()->getValue();
   OS << " \"" << this << "\" -> \"" << VS << "\"\n";
}
