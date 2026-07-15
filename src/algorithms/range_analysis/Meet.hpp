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
