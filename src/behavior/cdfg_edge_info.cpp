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
 *              Copyright (C) 2004-2026 Politecnico di Milano
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
 * @file cdfg_edge_info.cpp
 * @brief Data structures used to represent an edge in operation and basic block graphs.
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "cdfg_edge_info.hpp"
#include "behavioral_helper.hpp"

#include <utility>

bool CdfgEdgeInfo::IfElseIf() const
{
   return (labels.find(CDG_SELECTOR) != labels.end() or labels.find(CFG_SELECTOR) != labels.end());
}

std::string CdfgEdgeInfo::PrintLabels(const int selector, const BehavioralHelperConstRef& BH) const
{
   if(labels.find(selector) == labels.end())
   {
      return "";
   }
   std::string ret;
   const CustomOrderedSet<unsigned int>& labels_to_be_printed = labels.find(selector)->second;
   CustomOrderedSet<unsigned int>::const_iterator label, label_end = labels_to_be_printed.end();
   for(label = labels_to_be_printed.begin(); label != label_end; ++label)
   {
      if(label != labels_to_be_printed.begin())
      {
         ret += ", ";
      }
      else if(*label == default_COND)
      {
         ret = "else";
      }
      else
      {
         ret += BH->PrintVariable(*label);
      }
   }
   return ret;
}

void CdfgEdgeInfo::add_nodeID(unsigned int _nodeID, const int type)
{
   labels[type].insert(_nodeID);
}

const CustomOrderedSet<unsigned int>& CdfgEdgeInfo::get_nodeID(const int selector) const
{
   static CustomOrderedSet<unsigned int> null_set;
   if(labels.find(selector) != labels.end())
   {
      return labels.find(selector)->second;
   }
   return null_set;
}
