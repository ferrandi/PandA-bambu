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
 *              Copyright (C) 2004-2020 Politecnico di Milano
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
 * @file cdfg_edge_info.cpp
 * @brief Data structures used to represent an edge in operation and basic block graphs.
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "cdfg_edge_info.hpp"
#include "behavioral_helper.hpp"
#include <utility> // for pair

bool CdfgEdgeInfo::CdgEdgeT() const
{
   if(labels.find(CDG_SELECTOR) == labels.end())
      return false;
   return labels.find(CDG_SELECTOR)->second.find(T_COND) != labels.find(CDG_SELECTOR)->second.end();
}

bool CdfgEdgeInfo::CdgEdgeF() const
{
   if(labels.find(CDG_SELECTOR) == labels.end())
      return false;
   return labels.find(CDG_SELECTOR)->second.find(F_COND) != labels.find(CDG_SELECTOR)->second.end();
}

bool CdfgEdgeInfo::CfgEdgeT() const
{
   if(labels.find(CFG_SELECTOR) == labels.end())
      return false;
   return labels.find(CFG_SELECTOR)->second.find(T_COND) != labels.find(CFG_SELECTOR)->second.end();
}

bool CdfgEdgeInfo::CfgEdgeF() const
{
   if(labels.find(CFG_SELECTOR) == labels.end())
      return false;
   return labels.find(CFG_SELECTOR)->second.find(F_COND) != labels.find(CFG_SELECTOR)->second.end();
}

bool CdfgEdgeInfo::Switch() const
{
   return not CdgEdgeT() and not CdgEdgeF() and not CfgEdgeT() and not CfgEdgeF() and (labels.find(CDG_SELECTOR) != labels.end() or labels.find(CFG_SELECTOR) != labels.end());
}

const std::string CdfgEdgeInfo::PrintLabels(const int selector, const BehavioralHelperConstRef BH) const
{
   if(labels.find(selector) == labels.end())
      return "";
   std::string ret;
   const CustomOrderedSet<unsigned int>& labels_to_be_printed = labels.find(selector)->second;
   CustomOrderedSet<unsigned int>::const_iterator label, label_end = labels_to_be_printed.end();
   for(label = labels_to_be_printed.begin(); label != label_end; ++label)
   {
      if(label != labels_to_be_printed.begin())
         ret += ", ";
      if(*label == T_COND)
      {
         ret = "T";
      }
      else if(*label == F_COND)
      {
         ret = "F";
      }
      else if(*label == default_COND)
      {
         ret = "default";
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
      return labels.find(selector)->second;
   return null_set;
}
