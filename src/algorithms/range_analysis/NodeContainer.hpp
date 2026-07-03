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
 * @file NodeContainer.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef _RANGE_ANALYSIS_NODE_CONTAINER_HPP_
#define _RANGE_ANALYSIS_NODE_CONTAINER_HPP_
#include "VarNode.hpp"
#include "custom_set.hpp"
#include "ir_node.hpp"
#include "refcount.hpp"

#include <functional>
#include <map>
#include <vector>

class OpNode;
CONSTREF_FORWARD_DECL(ir_node);
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

   VarNode* addVarNode(const ir_nodeConstRef& V, unsigned int function_id);

   OpNode* pushOperation(OpNode* op);

   OpNode* addOperation(const ir_nodeConstRef& stmt, const application_managerRef& AppM);

#ifndef NDEBUG
   static int debug_level;
#endif

 protected:
   VarNode* addVarNode(const ir_nodeConstRef& V, unsigned int function_id, unsigned int use_bbi);

   UseMap& getUses()
   {
      return _useMap;
   }

 private:
   static const std::vector<
       std::function<std::function<OpNode*(NodeContainer*)>(const ir_nodeConstRef&, const application_managerRef&)>>
       _opCtorGenerators;

   VarNodes _varNodes;

   OpNodes _opNodes;

   DefMap _defMap;

   UseMap _useMap;
};

#endif // _RANGE_ANALYSIS_NODE_CONTAINER_HPP_