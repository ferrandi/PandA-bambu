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
 *              Copyright (C) 2004-2019 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/**
 * @file Range_Analysis.hpp
 * @brief 
 *
 * @author Michele Fiorito <michele2.fiorito@mail.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef RANGE_ANALYSIS_HPP
#define RANGE_ANALYSIS_HPP

#include <boost/multiprecision/cpp_int.hpp>
#include <iostream>

#include "application_frontend_flow_step.hpp"
#include "tree_common.hpp"
#include "tree_node.hpp"

REF_FORWARD_DECL(Range);
CONSTREF_FORWARD_DECL(Range);
REF_FORWARD_DECL(ConstraintGraph);

struct tree_reindexCompare 
{
  bool operator()(const tree_nodeConstRef &lhs, const tree_nodeConstRef &rhs) const;
};

enum RangeType
{
   Empty,
   Unknown,
   Regular,
   Anti,
   Real
};

class Range
{
 public:
   using APInt = boost::multiprecision::int128_t;
   using bw_t = unsigned;

 private:
   /// The lower bound of the range.
   APInt l;
   /// The upper bound of the range.
   APInt u;
   /// the range bit-width
   bw_t bw;
   /// the range type
   RangeType type;

   void normalizeRange(const APInt& lb, const APInt& ub, RangeType rType);

 public:
   Range(RangeType type, bw_t bw);
   Range(RangeType rType, bw_t bw, const APInt& lb, const APInt& ub);
   virtual ~Range() = default;
   Range(const Range& other) = default;
   Range(Range&&) = default;
   Range& operator=(const Range& other) = default;
   Range& operator=(Range&&) = default;
   bw_t getBitWidth() const;
   const APInt &getLower() const;
   const APInt &getUpper() const;
   APInt getSignedMax() const;
   APInt getSignedMin() const;
   APInt getUnsignedMax() const;
   APInt getUnsignedMin() const;
   RangeRef getAnti() const;

   bool isUnknown() const;
   void setUnknown();
   bool isRegular() const;
   bool isAnti() const;
   bool isEmpty() const;
   bool isReal() const;
   bool isSameType(RangeConstRef other) const;
   virtual bool isSameRange(RangeConstRef other) const;
   bool isSingleElement();
   virtual bool isFullSet() const;
   bool isMaxRange() const;
   virtual bool isConstant() const;
   virtual Range* clone() const;
   virtual void print(std::ostream& OS) const;
   std::string ToString() const;

   RangeRef add(RangeConstRef other) const;
   RangeRef sub(RangeConstRef other) const;
   RangeRef mul(RangeConstRef other) const;
   RangeRef udiv(RangeConstRef other) const;
   RangeRef sdiv(RangeConstRef other) const;
   RangeRef urem(RangeConstRef other) const;
   RangeRef srem(RangeConstRef other) const;
   RangeRef shl(RangeConstRef other) const;
   RangeRef shr(RangeConstRef other, bool sign) const;
   RangeRef And(RangeConstRef other) const;
   RangeRef Or(RangeConstRef other) const;
   RangeRef Xor(RangeConstRef other) const;
   RangeRef Eq(RangeConstRef other, bw_t bw) const;
   RangeRef Ne(RangeConstRef other, bw_t bw) const;
   RangeRef Ugt(RangeConstRef other, bw_t bw) const;
   RangeRef Uge(RangeConstRef other, bw_t bw) const;
   RangeRef Ult(RangeConstRef other, bw_t bw) const;
   RangeRef Ule(RangeConstRef other, bw_t bw) const;
   RangeRef Sgt(RangeConstRef other, bw_t bw) const;
   RangeRef Sge(RangeConstRef other, bw_t bw) const;
   RangeRef Slt(RangeConstRef other, bw_t bw) const;
   RangeRef Sle(RangeConstRef other, bw_t bw) const;
   RangeRef abs() const;
   RangeRef truncate(bw_t bitwidth) const;
   RangeRef sextOrTrunc(bw_t bitwidth) const;
   RangeRef zextOrTrunc(bw_t bitwidth) const;
   bool operator==(const Range& other) const;
   bool operator!=(const Range& other) const;
   virtual RangeRef intersectWith(RangeConstRef other) const;
   virtual RangeRef unionWith(RangeConstRef other) const;
   RangeRef BestRange(RangeConstRef UR, RangeConstRef SR, bw_t bw) const;

   static RangeRef makeSatisfyingCmpRegion(kind pred, RangeConstRef Other);
   static bw_t neededBits(const APInt& a, const APInt& b, bool sign);
};

std::ostream& operator<<(std::ostream& OS, const Range& R);

class RealRange : public Range
{
 private:
  RangeRef sign;
  RangeRef exponent;
  RangeRef fractional;

 public:
  RealRange(const Range& s, const Range& e, const Range& f);
  RealRange(RangeConstRef s, RangeConstRef e, RangeConstRef f);
  RealRange(RangeConstRef vc);
  ~RealRange() = default;
  RealRange(const RealRange& other) = default;
  RealRange(RealRange&&) = default;
  RealRange& operator=(const RealRange& other) = default;
  RealRange& operator=(RealRange&&) = default;
  RangeRef getRange() const;

  RangeRef getSign() const;
  RangeRef getExponent() const;
  RangeRef getFractional() const;
  void setSign(RangeConstRef s);
  void setExponent(RangeConstRef e);
  void setFractional(RangeConstRef f);
  bool isSameRange(RangeConstRef other) const override;
  bool isFullSet() const override;
  bool isConstant() const override;
  Range* clone() const override;
  void print(std::ostream& OS) const override;

  RangeRef intersectWith(RangeConstRef other) const override;
  RangeRef unionWith(RangeConstRef other) const override;
};

class RangeAnalysis : public ApplicationFrontendFlowStep
{
   ConstraintGraphRef CG;
   /// True if dead code elimination step must be restarted
   bool dead_code_restart;

   /** Gets the maximum bit width of the operands in the instructions of the
    * function. This function is necessary because the class APInt only
    * supports binary operations on operands that have the same number of
    * bits; thus, all the APInts that we allocate to process the function will
    * have the maximum bit size. The complexity of this function is linear on
    * the number of operands used in the function.
    */
   Range::bw_t getFunction_BW(unsigned int F);
   Range::bw_t getApplication_BW();

   bool finalize();

 protected:

   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param _Param is the set of the parameters
    * @param _AppM is the application manager
    * @param function_id is the identifier of the function
    * @param design_flow_manager is the design flow manager
    */
   RangeAnalysis(const application_managerRef AppM, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    *  Destructor
    */
   ~RangeAnalysis() override;

   /**
    * perform the range analysis
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;

};

#endif // !RANGE_ANALYSIS_HPP