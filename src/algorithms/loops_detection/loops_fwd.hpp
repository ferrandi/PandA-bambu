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
 *   Copyright (C) 2025-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
