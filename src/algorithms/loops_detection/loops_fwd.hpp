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
 *                Copyright (C) 2025-2026 Politecnico di Milano
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
 * @file loops_fwd.hpp
 * @brief Forward declarations for loops detection types.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */

#ifndef LOOPS_FWD_HPP
#define LOOPS_FWD_HPP

#include "refcount.hpp"

template <typename Graph>
struct DefaultLoopTraits;

template <typename GraphTraits>
class LoopTemplate;

template <typename Graph, typename GraphTraits, typename LoopT>
class LoopsTemplate;

class BBGraph;
struct BBGraphTraits;

template <typename Graph, typename GraphTraits = DefaultLoopTraits<Graph>>
using LoopsT = LoopsTemplate<Graph, GraphTraits, LoopTemplate<GraphTraits>>;

using Loops = LoopsTemplate<BBGraph, BBGraphTraits, LoopTemplate<BBGraphTraits>>;
using LoopsRef = refcount<Loops>;
using LoopsConstRef = refcount<const Loops>;

#endif // LOOPS_FWD_HPP
