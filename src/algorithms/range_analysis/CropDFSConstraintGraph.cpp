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
 * @file CropDFSConstraintGraph.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "CropDFSConstraintGraph.hpp"

#include "Meet.hpp"

#include <set>

namespace
{
   void crop(const NodeContainer::UseMap& compUseMap, OpNode* op)
   {
      ConstraintGraph::OpNodes activeOps;
      std::set<VarNode::key_type, VarNode::key_compare> visitedOps;

      // init the activeOps only with the op received
      activeOps.insert(op);

      while(!activeOps.empty())
      {
         const auto V = *activeOps.begin();
         activeOps.erase(V);
         const auto sinkId = V->getSink()->getId();

         // if the sink has been visited go to the next activeOps
         if(visitedOps.count(sinkId))
         {
            continue;
         }

         Meet::crop(V);
         visitedOps.insert(sinkId);

         // The use list.of sink
         const auto& L = compUseMap.at(sinkId);
         for(auto user : L)
         {
            activeOps.insert(user);
         }
      }
   }

} // namespace

CropDFSConstraintGraph::CropDFSConstraintGraph(application_managerRef _AppM, int _debug_level, int _graph_debug)
    : ConstraintGraph(_AppM, _debug_level, _graph_debug)
{
}

void CropDFSConstraintGraph::preUpdate(const UseMap& compUseMap,
                                       std::set<VarNode::key_type, VarNode::key_compare>& entryPoints)
{
   update(compUseMap, entryPoints, [](OpNode* b, const std::vector<APInt>&) { return Meet::growth(b); });
}

void CropDFSConstraintGraph::posUpdate(const UseMap& compUseMap,
                                       std::set<VarNode::key_type, VarNode::key_compare>& /*activeVars*/,
                                       const CustomSet<VarNode*>& component)
{
   for(const auto& varNode : component)
   {
      varNode->storeAbstractState();
   }

   for(const auto& op : getOpNodes())
   {
      if(component.count(op->getSink()))
      {
         crop(compUseMap, op);
      }
   }
}
