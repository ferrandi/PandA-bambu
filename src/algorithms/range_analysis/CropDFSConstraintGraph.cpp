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
