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
 * @file Nuutila.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "Nuutila.hpp"

#include "OpNode.hpp"
#include "exceptions.hpp"

namespace
{
   // ========================================================================== //
   // ControlDep
   // ========================================================================== //
   /// Specific type of OpNode used in Nuutila's strongly connected
   /// components algorithm.
   class ControlDepOpNode : public OpNode
   {
    private:
      VarNode* source;

    public:
      ControlDepOpNode(VarNode* sink, VarNode* source);

      OpNodeType getValueId() const override
      {
         return OpNodeType::OpNodeType_ControlDep;
      }

      Range eval() const override;

      std::vector<VarNode*> getSources() const override
      {
         return {source};
      }

      void replaceSource(const VarNode* _old, VarNode* _new) override
      {
         if(_old->getId() == source->getId())
         {
            source = _new;
         }
      }

      inline VarNode* getSource() const
      {
         return source;
      }

      std::string getName() const override;
      void print(std::ostream& OS) const override;

      static bool classof(ControlDepOpNode const*)
      {
         return true;
      }

      static bool classof(OpNode const* BO)
      {
         return BO->getValueId() == OpNodeType::OpNodeType_ControlDep;
      }
   };

   ControlDepOpNode::ControlDepOpNode(VarNode* _sink, VarNode* _source) : OpNode(_sink, nullptr), source(_source)
   {
      setIntersect(_sink->getMaxRange());
   }

   Range ControlDepOpNode::eval() const
   {
      return Range(Regular, Range::max_digits);
   }

   std::string ControlDepOpNode::getName() const
   {
      return "ControlDepOp";
   }

   void ControlDepOpNode::print(std::ostream& /*OS*/) const
   {
   }
} // namespace

Nuutila::Nuutila(const VarNodes& varNodes, UseMap& useMap, const UseMap& symbMap) : variables(varNodes), index(0)
{
   // Initialize DFS control variable for each Value in the graph
   for(const auto& [key, node] : varNodes)
   {
      dfs[key] = -1;
   }

   addControlDependenceEdges(useMap, symbMap, varNodes);
   // Iterate again over all varnodes of the constraint graph
   for(const auto& [key, node] : varNodes)
   {
      // If the Value has not been visited yet, call visit for him
      if(dfs[key] < 0)
      {
         std::stack<key_type> pilha;
         visit(key, pilha, useMap);
      }
   }
   delControlDependenceEdges(useMap);
}

const CustomSet<Nuutila::mapped_type>& Nuutila::getComponent(const key_type n) const
{
   THROW_ASSERT(components.count(n), "Required component not found: " + std::to_string(n));
   return components.at(n);
}

void Nuutila::addControlDependenceEdges(UseMap& useMap, const UseMap& symbMap, const VarNodes& vars)
{
   for(const auto& [key, users] : symbMap)
   {
      for(const auto* op : users)
      {
         THROW_ASSERT(vars.count(key), "Variable should be stored in VarNodes map");
         auto* source = vars.at(key);
         auto* cdedge = new ControlDepOpNode(op->getSink(), source);
         useMap[key].insert(cdedge);
      }
   }
}

void Nuutila::delControlDependenceEdges(UseMap& useMap)
{
   for(auto& [key, users] : useMap)
   {
      for(auto it = users.begin(); it != users.end();)
      {
         if(auto* cd = GetOp<ControlDepOpNode>(*it))
         {
            // Remove pseudo edge from the map
            delete cd;
            it = users.erase(it);
         }
         else
         {
            ++it;
         }
      }
   }
}

void Nuutila::visit(const key_type& V, std::stack<key_type>& stack, const UseMap& useMap)
{
   dfs[V] = index;
   ++index;
   root[V] = V;

   // Visit every node defined in an instruction that uses V
   for(const auto* op : useMap.at(V))
   {
      const auto& sink = op->getSink()->getId();
      if(dfs[sink] < 0)
      {
         visit(sink, stack, useMap);
      }
      if(!inComponent.count(sink) && (dfs[root[V]] >= dfs[root[sink]]))
      {
         root[V] = root[sink];
      }
   }

   // The second phase of the algorithm assigns components to stacked nodes
   if(key_compare()(root[V], V) == key_compare()(V, root[V]))
   {
      // Neither the worklist nor the map of components is part of Nuutila's
      // original algorithm. We are using these data structures to get a
      // topological ordering of the SCCs without having to go over the root
      // list once more.
      worklist.push_back(V);
      components[V].insert(variables.at(V));
      inComponent.insert(V);
      while(!stack.empty() && (dfs[stack.top()] > dfs[V]))
      {
         auto node = stack.top();
         stack.pop();
         inComponent.insert(node);
         components[V].insert(variables.at(node));
      }
   }
   else
   {
      stack.push(V);
   }
}
