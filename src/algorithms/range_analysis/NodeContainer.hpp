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
 * @file NodeContainer.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _RANGE_ANALYSIS_NODE_CONTAINER_HPP_
#define _RANGE_ANALYSIS_NODE_CONTAINER_HPP_
#include "ConditionalValueRange.hpp"
#include "VarNode.hpp"
#include "custom_set.hpp"
#include "refcount.hpp"
#include "tree_node.hpp"

#include <functional>
#include <map>
#include <vector>

class OpNode;
CONSTREF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(application_manager);

class NodeContainer
{
 public:
   // The VarNodes type.
   using VarNodes = std::map<VarNode::key_type, VarNode*, VarNode::key_compare>;
   // The Operations type.
   using OpNodes = CustomSet<OpNode*>;
   // A map from varnodes to the operation in which this variable is defined
   using DefMap = std::map<VarNode::key_type, OpNode*, VarNode::key_compare>;
   // A map from variables to the operations where these variables are used.
   using UseMap = std::map<VarNode::key_type, OpNodes, VarNode::key_compare>;

   using ConditionalValueRanges = std::map<tree_nodeConstRef, ConditionalValueRange, TreeNodeConstSorter>;

   virtual ~NodeContainer();

   inline const VarNodes& getVarNodes() const
   {
      return _varNodes;
   }

   inline const OpNodes& getOpNodes() const
   {
      return _opNodes;
   }

   inline const DefMap& getDefs() const
   {
      return _defMap;
   }

   inline const UseMap& getUses() const
   {
      return _useMap;
   }

   inline const ConditionalValueRanges& getCVR() const
   {
      return _cvrMap;
   }

   VarNode* addVarNode(const tree_nodeConstRef& V, unsigned int function_id);

   void addConditionalValueRange(const ConditionalValueRange&& cvr);

   OpNode* pushOperation(OpNode* op);

   OpNode* addOperation(const tree_nodeConstRef& stmt, const application_managerRef& AppM);

#ifndef NDEBUG
   static int debug_level;
#endif

 protected:
   VarNode* addVarNode(const tree_nodeConstRef& V, unsigned int function_id, unsigned int use_bbi);

   UseMap& getUses()
   {
      return _useMap;
   }

 private:
   static const std::vector<
       std::function<std::function<OpNode*(NodeContainer*)>(const tree_nodeConstRef&, const application_managerRef&)>>
       _opCtorGenerators;

   VarNodes _varNodes;

   OpNodes _opNodes;

   DefMap _defMap;

   UseMap _useMap;

   ConditionalValueRanges _cvrMap;
};

#endif // _RANGE_ANALYSIS_NODE_CONTAINER_HPP_