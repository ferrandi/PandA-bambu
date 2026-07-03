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
 * @file Meet.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef _RANGE_ANALYSIS_MEET_HPP_
#define _RANGE_ANALYSIS_MEET_HPP_
#include "OpNode.hpp"

#include <vector>

class Meet
{
 public:
   static bool fixed(OpNode* op);

   /**
    * @brief This is the meet operator of the growth analysis.
    * The growth analysis will change the bounds of each variable, if necessary. Initially, each variable is bound to
    * either the undefined interval, e.g. [., .], or to a constant interval, e.g., [3, 15]. After this analysis runs,
    * there will be no undefined interval. Each variable will be either bound to a constant interval, or to [-, c], or
    * to [c, +], or to [-, +].
    *
    * @param op
    * @param constantvector
    * @return true
    * @return false
    */
   static bool widen(OpNode* op, const std::vector<APInt>& constantvector);

   static bool growth(OpNode* op);

   /**
    * @brief This is the meet operator of the cropping analysis.
    * Whereas the growth analysis expands the bounds of each variable, regardless of intersections in the constraint
    * graph, the cropping analysis shrinks these bounds back to ranges that respect the intersections.
    *
    * @param op
    * @param constantvector
    * @return true
    * @return false
    */
   static bool narrow(OpNode* op, const std::vector<APInt>& constantvector);

   static bool crop(OpNode* op);

#ifndef NDEBUG
   static int debug_level;
#endif

 private:
   /**
    * @brief Get the first constant from vector greater than val
    *
    * @param constantvector
    * @param val
    * @return const APInt&
    */
   static const APInt& getFirstGreaterFromVector(const std::vector<APInt>& constantvector, const APInt& val);

   /**
    * @brief Get the first constant from vector less than val
    *
    * @param constantvector
    * @param val
    * @return const APInt&
    */
   static const APInt& getFirstLessFromVector(const std::vector<APInt>& constantvector, const APInt& val);
};

#endif // _RANGE_ANALYSIS_MEET_HPP_
