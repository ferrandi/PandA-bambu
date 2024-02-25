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
 * @file NodeContainer.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "NodeContainer.hpp"

#include "BinaryOpNode.hpp"
#include "LoadOpNode.hpp"
#include "OpNode.hpp"
#include "PhiOpNode.hpp"
#include "SigmaOpNode.hpp"
#include "TernaryOpNode.hpp"
#include "UnaryOpNode.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "tree_basic_block.hpp"

#ifndef NDEBUG
int NodeContainer::debug_level = DEBUG_LEVEL_NONE;
#endif

const std::vector<
    std::function<std::function<OpNode*(NodeContainer*)>(const tree_nodeConstRef&, const application_managerRef&)>>
    NodeContainer::_opCtorGenerators = {LoadOpNode::opCtorGenerator,   UnaryOpNode::opCtorGenerator,
                                        BinaryOpNode::opCtorGenerator, PhiOpNode::opCtorGenerator,
                                        SigmaOpNode::opCtorGenerator,  TernaryOpNode::opCtorGenerator};

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

VarNode* NodeContainer::addVarNode(const tree_nodeConstRef& V, unsigned int function_id)
{
   return addVarNode(V, function_id, BB_ENTRY);
}

VarNode* NodeContainer::addVarNode(const tree_nodeConstRef& V, unsigned int function_id, unsigned int use_bbi)
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

void NodeContainer::addConditionalValueRange(const ConditionalValueRange&& cvr)
{
   auto cvrIt = _cvrMap.find(cvr.getVar());
   if(cvrIt != _cvrMap.end())
   {
      for(const auto& BBIvr : cvr.getVR())
      {
         cvrIt->second.addVR(BBIvr.first, BBIvr.second);
      }
   }
   else
   {
      _cvrMap.insert(std::make_pair(cvr.getVar(), cvr));
   }
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

OpNode* NodeContainer::addOperation(const tree_nodeConstRef& stmt, const application_managerRef& AppM)
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
