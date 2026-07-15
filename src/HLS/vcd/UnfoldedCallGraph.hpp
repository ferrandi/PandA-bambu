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
 *   Copyright (C) 2015-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef UNFOLDED_CALL_GRAPH_HPP
#define UNFOLDED_CALL_GRAPH_HPP

#include "edge_info.hpp"
#include "graph.hpp"
#include "node_info.hpp"
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(FunctionBehavior);

struct UnfoldedFunctionNodeInfo : public NodeInfo
{
   unsigned int f_id;

   FunctionBehaviorConstRef behavior;

   UnfoldedFunctionNodeInfo(unsigned int _f_id = 0, const FunctionBehaviorConstRef& b = nullptr)
       : f_id(_f_id), behavior(b)
   {
   }
};

struct UnfoldedCallEdgeInfo : public EdgeInfo
{
   unsigned int call_id;

   bool is_direct;

   UnfoldedCallEdgeInfo(unsigned int _call_id = 0, bool _is_direct = true) : call_id(_call_id), is_direct(_is_direct)
   {
   }
};

using UnfoldedCallGraph = RawGraph<UnfoldedFunctionNodeInfo, UnfoldedCallEdgeInfo>;

#endif
