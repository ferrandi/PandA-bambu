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
 * @file NodeContainer.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "NodeContainer.hpp"

#include "BinaryOpNode.hpp"
#include "LoadOpNode.hpp"
#include "OpNode.hpp"
#include "PhiOpNode.hpp"
#include "TernaryOpNode.hpp"
#include "UnaryOpNode.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "ir_basic_block.hpp"

#ifndef NDEBUG
int NodeContainer::debug_level = DEBUG_LEVEL_NONE;
#endif

const std::vector<
    std::function<std::function<OpNode*(NodeContainer*)>(const ir_nodeConstRef&, const application_managerRef&)>>
    NodeContainer::_opCtorGenerators = {LoadOpNode::opCtorGenerator, UnaryOpNode::opCtorGenerator,
                                        BinaryOpNode::opCtorGenerator, PhiOpNode::opCtorGenerator,
                                        TernaryOpNode::opCtorGenerator};

NodeContainer::~NodeContainer()
{
   for(const auto& [key, node] : _varNodes)
   {
      delete node;
   }
   for(const auto& op : _opNodes)
   {
      delete op;
   }
}

VarNode* NodeContainer::addVarNode(const ir_nodeConstRef& V, unsigned int function_id)
{
   return addVarNode(V, function_id, BB_ENTRY);
}

VarNode* NodeContainer::addVarNode(const ir_nodeConstRef& V, unsigned int function_id, unsigned int use_bbi)
{
   THROW_ASSERT(V, "Can't insert nullptr as variable");
   auto vit = _varNodes.find(VarNode::makeId(V, use_bbi));
   if(vit != _varNodes.end())
   {
      return vit->second;
   }

   const auto node = new VarNode(V, function_id, use_bbi);
   _varNodes.insert(std::make_pair(node->getId(), node));
   _useMap.insert(std::make_pair(node->getId(), OpNodes()));
   return node;
}

OpNode* NodeContainer::pushOperation(OpNode* op)
{
   if(op)
   {
      _opNodes.insert(op);
      _defMap.insert({op->getSink()->getId(), op});
      for(const auto node : op->getSources())
      {
         _useMap[node->getId()].insert(op);
      }
   }
   return op;
}

OpNode* NodeContainer::addOperation(const ir_nodeConstRef& stmt, const application_managerRef& AppM)
{
   for(const auto& generateCtorFor : _opCtorGenerators)
   {
      if(auto generateOpFor = generateCtorFor(stmt, AppM))
      {
         return pushOperation(generateOpFor(this));
      }
   }
   return nullptr;
}
