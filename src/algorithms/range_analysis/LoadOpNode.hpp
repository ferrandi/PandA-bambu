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
 * @file LoadOpNode.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef _RANGE_ANALYSIS_LOAD_OP_NODE_HPP_
#define _RANGE_ANALYSIS_LOAD_OP_NODE_HPP_
#include "OpNode.hpp"
#include "refcount.hpp"

#include <functional>

class NodeContainer;
CONSTREF_FORWARD_DECL(ir_node);
REF_FORWARD_DECL(application_manager);

class LoadOpNode : public OpNode
{
 private:
   /// reference to the memory access operand
   std::vector<VarNode*> sources;

   Range eval() const override;

 public:
   LoadOpNode(VarNode* sink, const ir_nodeConstRef& inst);
   LoadOpNode(const LoadOpNode&) = delete;
   LoadOpNode(LoadOpNode&&) = delete;
   LoadOpNode& operator=(const LoadOpNode&) = delete;
   LoadOpNode& operator=(LoadOpNode&&) = delete;

   OpNodeType getValueId() const override;

   std::vector<VarNode*> getSources() const override;

   void replaceSource(const VarNode* _old, VarNode* _new) override;

   std::string getName() const override;
   void print(std::ostream& OS) const override;

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

   static std::function<OpNode*(NodeContainer*)> opCtorGenerator(const ir_nodeConstRef& stmt,
                                                                 const application_managerRef& AppM);
};

#endif // _RANGE_ANALYSIS_LOAD_OP_NODE_HPP_
