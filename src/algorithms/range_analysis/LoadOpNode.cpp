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
 * @file LoadOpNode.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "LoadOpNode.hpp"

#include "NodeContainer.hpp"
#include "application_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "memory.hpp"

#ifndef NDEBUG
bool _ra_enable_load = true;
#endif

LoadOpNode::LoadOpNode(VarNode* _sink, const ir_nodeConstRef& _inst) : OpNode(_sink, _inst)
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

void LoadOpNode::replaceSource(const VarNode* _old, VarNode* _new)
{
   for(auto& src : sources)
   {
      if(_old->getId() == src->getId())
      {
         src = _new;
      }
   }
}

Range LoadOpNode::eval() const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, ToString());

#ifndef NDEBUG
   if(getNumSources() == 0 || !_ra_enable_load)
#else
   if(getNumSources() == 0)
#endif
   {
      Range aux(Unknown, getSink()->getBitWidth());
      if(!getIntersect()->tryGetRange(aux))
      {
         aux = Range(Unknown, getSink()->getBitWidth());
      }
      THROW_ASSERT(getSink()->getBitWidth() == aux.getBitWidth(),
                   "Sink (" + getSink()->getValue()->ToString() + ") has bitwidth " +
                       std::to_string(+getSink()->getBitWidth()) + " while intersect has bitwidth " +
                       std::to_string(+aux.getBitWidth()));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "= " + aux.ToString());
      return aux;
   }

   // Iterate over the sources of the load
   auto result = ir_helper::TypeRange(getSink()->getValue(), Empty);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   for(const VarNode* varNode : sources)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "  ->" + varNode->getRange().ToString() + " " + varNode->ToString());
      result = result.unionWith(varNode->getRange());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--  = " + result.ToString());

   Range aux(Unknown, result.getBitWidth());
   if(getIntersect()->tryGetRange(aux) && !aux.isFullSet())
   {
      auto _intersect = result.intersectWith(aux);
      if(!_intersect.isEmpty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---aux = " + aux.ToString() + " from " + getIntersect()->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---res = " + _intersect.ToString());
         result = _intersect;
      }
   }
   return result;
}

static Range constructor_range(const ir_managerConstRef& TM, const ir_nodeConstRef& tn, const Range& init,
                               unsigned long long element_size)
{
   THROW_ASSERT(tn->get_kind() == constructor_node_K, "tn is not constructor_node node");
   const auto* c = GetPointerS<const constructor_node>(tn);
   std::vector<unsigned long long> array_dims;
   unsigned long long elements_bitsize = 0;
   ir_helper::get_array_dim_and_bitsize(TM, c->type->index, array_dims, elements_bitsize);
   if(elements_bitsize != element_size)
   {
      return init;
   }
   unsigned int initialized_elements = 0;
   auto ctor_range = init;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "-->");
   for(const auto& [idx, elm] : c->list_of_idx_valu)
   {
      THROW_ASSERT(elm, "unexpected condition");
      if(idx->get_kind() == field_val_node_K && GetPointerS<field_val_node>(idx)->bitfield)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "<--");
         return init;
      }

      const auto elm_range = [&]() {
         if(elm->get_kind() == constructor_node_K &&
            ir_helper::IsArrayEquivType(GetPointerS<const constructor_node>(elm)->type))
         {
            THROW_ASSERT(array_dims.size() > 1 || c->type->get_kind() == struct_ty_node_K ||
                             GetPointerS<const constructor_node>(elm)->type->get_kind() == struct_ty_node_K,
                         "invalid nested constructors:" + tn->ToString() + " " + std::to_string(array_dims.size()));
            return constructor_range(TM, elm, ctor_range, element_size);
         }
         return ir_helper::NodeRange(elm);
      }();
      if(elm_range.getBitWidth() != static_cast<Range::bw_t>(elements_bitsize))
      {
         // NOTE: this may happen due to double dereferencing
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                        "---Initializer value not compliant " + elm->ToString() + " " + elm_range.ToString());
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                        "---Initializer value is " + elm->ToString() + " " + elm_range.ToString());
         ctor_range = ctor_range.unionWith(elm_range);
      }
      initialized_elements++;
   }
   if(initialized_elements < array_dims.front())
   {
      ctor_range = ctor_range.unionWith(Range(Regular, static_cast<Range::bw_t>(elements_bitsize), 0, 0));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "<--");
   return ctor_range;
}

std::function<OpNode*(NodeContainer*)> LoadOpNode::opCtorGenerator(const ir_nodeConstRef& stmt,
                                                                   const application_managerRef& AppM)
{
   if(stmt->get_kind() != assign_stmt_K)
   {
      return nullptr;
   }
   const auto ga = GetPointerS<const assign_stmt>(stmt);
   const auto FB = AppM->CGetFunctionBehavior(ga->parent->index);
   if(!ir_helper::IsLoad(stmt, FB->get_function_mem()))
   {
      return nullptr;
   }
   return [stmt, ga, FB, AppM](NodeContainer* NC) {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                     "Analysing load operation " + ga->ToString());
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "-->");
      const auto bw = static_cast<Range::bw_t>(ir_helper::TypeSize(ga->op0));
      const auto function_id = ga->parent->index;
      const auto sink = NC->addVarNode(ga->op0, function_id);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                     "Sink variable is " + ga->op0->get_kind_text() + " (size = " + std::to_string(+bw) + ")");

      auto intersection = ir_helper::TypeRange(sink->getValue(), Empty);
      if(ga->op1->get_kind() == mem_access_node_K || ga->op1->get_kind() == variable_val_node_K)
      {
         const auto TM = AppM->get_ir_manager();
         const auto base_var = ir_helper::GetBaseVariable(ga->op1);
         const auto* hm = GetPointer<HLS_manager>(AppM);
         if(hm && hm->Rmem && base_var && FB->is_variable_mem(base_var->index) && hm->Rmem->is_sds_var(base_var->index))
         {
            const auto is_written =
                AppM->get_written_objects().find(base_var->index) != AppM->get_written_objects().end();
            const auto is_private =
                hm->Rmem->get_enable_hls_bit_value() && hm->Rmem->is_private_memory(base_var->index);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                           "---Base variable is " + base_var->ToString());
            if(!is_written || is_private)
            {
               if(base_var->get_kind() == variable_val_node_K)
               {
                  const auto* vd = GetPointerS<const variable_val_node>(base_var);
                  if(vd->init)
                  {
                     const auto init_range = [&]() {
                        if(vd->init->get_kind() == constructor_node_K)
                        {
                           return constructor_range(TM, vd->init, intersection,
                                                    hm->Rmem->get_sds_var_size(base_var->index));
                        }
                        if(ir_helper::IsConstant(vd->init))
                        {
                           return ir_helper::NodeRange(vd->init);
                        }
                        return Range(Empty, bw);
                     }();
                     if(init_range.isEmpty() || init_range.getBitWidth() != bw)
                     {
                        // NOTE: this may happen due to double dereferencing
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                                       "---Initializer value not compliant " + vd->init->ToString() + " " +
                                           init_range.ToString());
                     }
                     else
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                                       "---Initializer value is " + vd->init->ToString() + " " + init_range.ToString());
                        intersection = init_range;
                     }
                  }
                  else if(is_written && is_private)
                  {
                     intersection = Range(Regular, bw, 0, 0);
                  }
               }
               if(is_written && is_private)
               {
                  for(const auto& cur_var : hm->Rmem->get_source_values(base_var->index))
                  {
                     const auto cur_node = TM->GetIRNode(cur_var);
                     const auto init_range = ir_helper::NodeRange(cur_node);
                     if(init_range.getBitWidth() != bw)
                     {
                        // NOTE: this may be due to double dereferencing
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                                       "---Initializer value not compliant " + cur_node->ToString() + " " +
                                           init_range.ToString());
                     }
                     else
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                                       "---Initializer value is " + cur_node->ToString() + " " + init_range.ToString());
                        intersection = intersection.unionWith(init_range);
                     }
                  }
               }
            }
         }
      }
      if(intersection.isEmpty())
      {
         intersection = ir_helper::NodeRange(stmt);
      }
      THROW_ASSERT(intersection.getBitWidth() <= bw,
                   "Pointed variables range should have bitwidth contained in sink bitwidth: " +
                       intersection.ToString() + " > " + std::to_string(static_cast<unsigned>(bw)));
      if(intersection.getBitWidth() < bw)
      {
         intersection = intersection.zextOrTrunc(bw);
      }
      auto BI =
#ifndef NDEBUG
          !_ra_enable_load ? ValueRangeRef(new ValueRange(ir_helper::NodeRange(stmt))) :
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

std::string LoadOpNode::getName() const
{
   return "load_node";
}
