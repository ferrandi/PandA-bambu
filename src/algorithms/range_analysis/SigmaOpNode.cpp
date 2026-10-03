/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2019-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file SigmaOpNode.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "SigmaOpNode.hpp"

#include "NodeContainer.hpp"
#include "SymbValueRange.hpp"
#include "dbgPrintHelper.hpp"
#include "ir_helper.hpp"
#include "ir_node.hpp"

SigmaOpNode::SigmaOpNode(const ValueRangeRef& _intersect, VarNode* _sink, VarNode* _source)
    : UnaryOpNode(_sink, _source, nullptr, phi_stmt_R), SymbolicSource(nullptr), unresolved(false)
{
   setIntersect(_intersect);
   if(auto symb = RefcountCast<SymbRange>(_intersect))
   {
      SymbolicSource = symb->getBound();
   }
}

OpNode::OpNodeType SigmaOpNode::getValueId() const
{
   return OpNodeType::OpNodeType_Sigma;
}

std::vector<VarNode*> SigmaOpNode::getSources() const
{
   auto s = UnaryOpNode::getSources();
   if(SymbolicSource)
   {
      s.push_back(SymbolicSource);
   }
   return s;
}

/// Computes the interval of the sink based on the interval of the sources,
/// the operation and the interval associated to the operation.
Range SigmaOpNode::eval() const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, ToString());

   Range result(getSource()->getRange());
   Range aux(Unknown, result.getBitWidth());
   if(!getIntersect()->tryGetRange(aux))
   {
      aux = Range(Unknown, result.getBitWidth());
   }
   if(!aux.isUnknown())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---aux = " + aux.ToString() + " from " + getIntersect()->ToString());
      auto _intersect = result.intersectWith(aux);
      if(_intersect.isEmpty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---res = " + aux.ToString());
         result = aux;
      }
      else
      {
         // Sigma operations are used to narrow live range split after conditional statements,
         // thus it is useful to intersect their range only if it actually produces tighter interval
         if(_intersect.getSpan() < result.getSpan())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---res = " + _intersect.ToString());
            result = _intersect;
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---result not changed because not improved");
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---" + result.ToString() + " = SIGMA< " + getSource()->getRange().ToString() + " >");
   return result;
}

void SigmaOpNode::print(std::ostream& OS) const
{
   OS << getSink()->getValue() << " = SIGMA< " << getSource()->getValue() << " >";
}

std::string SigmaOpNode::getName() const
{
   return "sigma_expr";
}
