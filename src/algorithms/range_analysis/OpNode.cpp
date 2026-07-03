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
