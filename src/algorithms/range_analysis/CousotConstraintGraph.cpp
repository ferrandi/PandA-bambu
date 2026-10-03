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
 * @file CousotConstraintGraph.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "CousotConstraintGraph.hpp"

#include "Meet.hpp"

CousotConstraintGraph::CousotConstraintGraph(application_managerRef _AppM, int _debug_level, int _graph_debug)
    : ConstraintGraph(_AppM, _debug_level, _graph_debug)
{
}

void CousotConstraintGraph::preUpdate(const UseMap& compUseMap,
                                      std::set<VarNode::key_type, VarNode::key_compare>& entryPoints)
{
   update(compUseMap, entryPoints, Meet::widen);
}

void CousotConstraintGraph::posUpdate(const UseMap& compUseMap,
                                      std::set<VarNode::key_type, VarNode::key_compare>& entryPoints,
                                      const CustomSet<VarNode*>& /*component*/)
{
   update(compUseMap, entryPoints, Meet::narrow);
}