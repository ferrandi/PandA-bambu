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
 * @file Nuutila.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef _RANGE_ANALYSIS_NUUTILA_HPP_
#define _RANGE_ANALYSIS_NUUTILA_HPP_
#include "NodeContainer.hpp"
#include "VarNode.hpp"
#include "custom_set.hpp"

#include <deque>
#include <map>
#include <set>
#include <stack>

class Nuutila
{
 public:
   using VarNodes = NodeContainer::VarNodes;
   using UseMap = NodeContainer::UseMap;
   using key_type = VarNodes::key_type;
   using key_compare = VarNodes::key_compare;
   using mapped_type = VarNodes::mapped_type;

   /**
    * @brief Finds the strongly connected components in the constraint graph formed by varNodes and useMap
    * Finds the strongly connected components in the constraint graph formed by varNodes and useMap. The class receives
    * the map of futures to insert the control dependence edges in the constraint graph. These edges are removed after
    * the class is done computing the SCCs.
    */
   Nuutila(const VarNodes& varNodes, UseMap& useMap, const UseMap& symbMap);

   const CustomSet<mapped_type>& getComponent(const key_type n) const;

   inline auto begin()
   {
      return worklist.rbegin();
   }

   inline auto cbegin() const
   {
      return worklist.crbegin();
   }

   inline auto end()
   {
      return worklist.rend();
   }

   inline auto cend() const
   {
      return worklist.crend();
   }

 private:
#ifndef NDEBUG
   int debug_level;
#endif

   const VarNodes& variables;
   int index;
   std::map<key_type, int, key_compare> dfs;
   std::map<key_type, key_type, key_compare> root;
   std::set<key_type, key_compare> inComponent;
   std::map<key_type, CustomSet<mapped_type>, key_compare> components;
   std::deque<key_type> worklist;

   /**
    * @brief Adds the edges that ensure that we solve a future before fixing its interval.
    *
    * @param useMap
    * @param symbMap
    * @param vars
    */
   void addControlDependenceEdges(UseMap& useMap, const UseMap& symbMap, const VarNodes& vars);

   /**
    * @brief Removes the control dependence edges from the constraint graph.
    *
    * @param useMap
    */
   void delControlDependenceEdges(UseMap& useMap);

   /**
    * @brief Finds SCCs using Nuutila's algorithm.
    * This algorithm is divided in two parts. The first calls the recursive visit procedure on every node in the
    * constraint graph. The second phase revisits these nodes, grouping them in components.
    *
    * @param V
    * @param stack
    * @param useMap
    */
   void visit(const key_type& V, std::stack<key_type>& stack, const UseMap& useMap);
};

#endif // _RANGE_ANALYSIS_NUUTILA_HPP_