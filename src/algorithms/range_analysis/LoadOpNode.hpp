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
 * @file LoadOpNode.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _RANGE_ANALYSIS_LOAD_OP_NODE_HPP_
#define _RANGE_ANALYSIS_LOAD_OP_NODE_HPP_
#include "OpNode.hpp"
#include "refcount.hpp"

#include <functional>
#include <vector>

class NodeContainer;
CONSTREF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(application_manager);

class LoadOpNode : public OpNode
{
 private:
   /// reference to the memory access operand
   std::vector<VarNode*> sources;

   RangeRef eval() const override;

 public:
   LoadOpNode(VarNode* sink, const tree_nodeConstRef& inst);
   LoadOpNode(const LoadOpNode&) = delete;
   LoadOpNode(LoadOpNode&&) = delete;
   LoadOpNode& operator=(const LoadOpNode&) = delete;
   LoadOpNode& operator=(LoadOpNode&&) = delete;

   OpNodeType getValueId() const override;

   std::vector<VarNode*> getSources() const override;

   void replaceSource(VarNode* _old, VarNode* _new) override;

   void print(std::ostream& OS) const override;
   void printDot(std::ostream& OS) const override;

   inline void addSource(VarNode* newsrc)
   {
      sources.push_back(newsrc);
   }

   inline const VarNode* getSource(size_t index) const
   {
      return sources[index];
   }

   inline size_t getNumSources() const
   {
      return sources.size();
   }

   static inline bool classof(LoadOpNode const*)
   {
      return true;
   }
   static inline bool classof(OpNode const* BO)
   {
      return BO->getValueId() == OpNodeType::OpNodeType_Load;
   }

   static std::function<OpNode*(NodeContainer*)> opCtorGenerator(const tree_nodeConstRef& stmt,
                                                                 const application_managerRef& AppM);
};

#endif // _RANGE_ANALYSIS_LOAD_OP_NODE_HPP_