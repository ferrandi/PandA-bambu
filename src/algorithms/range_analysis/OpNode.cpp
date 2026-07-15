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
 * @file OpNode.cpp
 * @brief This class represents a generic operation in range analysis
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "OpNode.hpp"

#include "SymbValueRange.hpp"
#include "dbgPrintHelper.hpp"
#include "ir_helper.hpp"
#include "ir_node.hpp"

#ifndef NDEBUG
int OpNode::debug_level = DEBUG_LEVEL_NONE;
#endif

OpNode::OpNode(VarNode* _sink, const ir_nodeConstRef& _inst) : sink(_sink), inst(_inst)
{
   THROW_ASSERT(sink, "");
   // TODO: here should use ir_helper::NodeRange instead of ir_helper::TypeRange, but this causes errors
   intersect = ValueRangeRef(new ValueRange(ir_helper::TypeRange(_sink->getValue())));
}

void OpNode::solveFuture()
{
   if(const auto SI = RefcountCast<const SymbRange>(getIntersect()))
   {
      setIntersect(SI->solveFuture(getSink()));
   }
}
