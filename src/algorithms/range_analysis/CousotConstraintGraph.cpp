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