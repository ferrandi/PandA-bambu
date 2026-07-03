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
 * @file ConstraintGraph.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef _RANGE_ANALYSIS_CONSTRAINT_GRAPH_HPP_
#define _RANGE_ANALYSIS_CONSTRAINT_GRAPH_HPP_
#include "NodeContainer.hpp"
#include "OpNode.hpp"
#include "OrderedInstructions.hpp"
#include "VarNode.hpp"
#include "custom_set.hpp"
#include "refcount.hpp"

#include <filesystem>
#include <functional>
#include <set>
#include <vector>

CONSTREF_FORWARD_DECL(application_manager);
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(VarUse);
struct PredicateWithEdge;

enum LocalNum
{
   // Operations that must appear first in the block.
   LN_First,
   // Operations that are somewhere in the middle of the block, and are sorted on  demand.
   LN_Middle,
   // Operations that must appear last in a block, like successor phi node uses.
   LN_Last
};

/**
 * @brief Associate global and local DFS info with defs and uses, so we can sort them into a global domination
 * ordering
 */
struct ValueDFS
{
   DFSInfo DIndex{};
   unsigned int LocalNum{LN_Middle};
   // Only one of Def or Use will be set.
   OpNode* Def{nullptr};
   VarUseRef U{nullptr};
   // Neither PInfo nor EdgeOnly participate in the ordering
   PredicateWithEdge* PInfo{nullptr};
   bool EdgeOnly{false};

   ValueDFS(DFSInfo _DIndex, unsigned int _LocalNum, PredicateWithEdge* _PInfo, bool _EdgeOnly = false);

   ValueDFS(DFSInfo _DIndex, unsigned int _LocalNum, VarNode* var, OpNode* op, bool _EdgeOnly = false);

   std::string ToString() const;
};
using ValueDFSStack = std::vector<ValueDFS>;

class ConstraintGraph : public NodeContainer
{
 private:
#ifndef NDEBUG
   int debug_level;
   int graph_debug;
#endif

   const application_managerRef AppM;

   // Vector containing the constants from a SCC
   // It is cleared at the beginning of every SCC resolution
   std::vector<APInt> constantvector;

   // Given the renaming stack, make all the operands currently on the stack real by inserting them into the IR.  Return
   // the last operation's value.
   OpNode* materializeStack(ValueDFSStack& RenameStack, unsigned int function_id, VarNode* OrigOp);

   void generateEntryPoints(const CustomSet<VarNode*>& component,
                            std::set<VarNode::key_type, VarNode::key_compare>& entryPoints) const;

   /*
    * This method evaluates once each operation that uses a variable in
    * component, so that the next SCCs after component will have entry
    * points to kick start the range analysis algorithm.
    */
   void propagateToNextSCC(const CustomSet<VarNode*>& component) const;

   void solveFutures(const CustomSet<VarNode*>& component, const UseMap& symbMap) const;

   void solveFuturesSC(VarNode* varNode, const UseMap& symbMap) const;

 protected:
   /**
    * @brief Perform the widening and narrowing operations
    *
    * @param compUseMap
    * @param actv
    * @param meet
    */
   void update(const UseMap& compUseMap, std::set<VarNode::key_type, VarNode::key_compare>& actv,
               const std::function<bool(OpNode*, const std::vector<APInt>&)>& meet) const;

   static void update(size_t nIterations, const UseMap& compUseMap,
                      std::set<VarNode::key_type, VarNode::key_compare>& actv);

   virtual void preUpdate(const UseMap& compUseMap, std::set<VarNode::key_type, VarNode::key_compare>& entryPoints) = 0;
   virtual void posUpdate(const UseMap& compUseMap, std::set<VarNode::key_type, VarNode::key_compare>& activeVars,
                          const CustomSet<VarNode*>& component) = 0;

 public:
   ConstraintGraph(application_managerRef _AppM, int _debug_level, int _graph_debug);

   /// Iterates through all instructions in the function and builds the graph.
   void buildGraph(unsigned int function_id, bool computeESSA);

#ifndef NDEBUG
   void findIntervals(const ParameterConstRef& parameters, const std::string& step_name);
#else
   void findIntervals();
#endif

#ifndef NDEBUG
   std::filesystem::path printDot(const std::filesystem::path& file_name, const ParameterConstRef& parameters,
                                  const NodeContainer::VarNodes& vars, const NodeContainer::OpNodes& ops) const;

   inline std::filesystem::path printDot(const std::filesystem::path& file_name,
                                         const ParameterConstRef& parameters) const
   {
      return printDot(file_name, parameters, getVarNodes(), getOpNodes());
   }
#endif
};

#endif // _RANGE_ANALYSIS_CONSTRAINT_GRAPH_HPP_
