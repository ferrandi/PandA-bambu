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
#include "refcount.hpp"   // for REF_FORWARD_DECL

REF_FORWARD_DECL(tree_node);

namespace RangeAnalysis
{
   using APInt = boost::multiprecision::int128_t;

   enum RangeType
   {
      Unknown,
      Regular,
      Empty,
      Anti
   };

   class Range_base
   {
      /// The lower bound of the range.
      APInt l;
      /// The upper bound of the range.
      APInt u;
      /// the range bit-width
      unsigned bw;
      /// the range type
      RangeType type;

      void normalizeRange(const APInt& lb, const APInt& ub, RangeType rType);

    public:
      Range_base(RangeType type, unsigned bw);
      Range_base(RangeType rType, unsigned bw, const APInt& lb, const APInt& ub);
      ~Range_base() = default;
      Range_base(const Range_base& other) = default;
      Range_base(Range_base&&) = default;
      static Range_base getAnti(const Range_base& o);
      Range_base& operator=(const Range_base& other) = default;
      Range_base& operator=(Range_base&&) = default;
      unsigned int getBitWidth() const;
      const APInt getLower() const;
      const APInt getUpper() const;
      const APInt getSignedMax() const;
      const APInt getSignedMin() const;
      const APInt getUnsignedMax() const;
      const APInt getUnsignedMin() const;

      bool isUnknown() const
      {
         return type == Unknown;
      }
      void setUnknown()
      {
         type = Unknown;
      }
      bool isRegular() const
      {
         return type == Regular;
      }
      bool isAnti() const
      {
         return type == Anti;
      }
      bool isEmpty() const
      {
         return type == Empty;
      }
      bool isSameType(const Range_base& a, const Range_base& b) const
      {
         return a.type == b.type;
      }
      bool isSameRange(const Range_base& a, const Range_base& b) const
      {
         return (a.l == b.l) && (a.u == b.u);
      }
      bool isSingleElement()
      {
         return type == Regular && l == u;
      }

      static unsigned neededBits(const APInt& a, const APInt& b, bool sign);
   };
   class Range : public Range_base
   {
    public:
      Range(RangeType rType, unsigned bw) : Range_base(rType, bw)
      {
      }
      Range(RangeType rType, unsigned bw, const APInt& lb, const APInt& ub) : Range_base(rType, bw, lb, ub)
      {
      }
      Range(const Range_base& rb) : Range_base(rb)
      {
      }
      ~Range() = default;
      Range(const Range& other) = default;
      Range(Range&&) = default;
      Range& operator=(const Range& other) = default;
      Range& operator=(Range&&) = default;

      bool isFullSet() const;
      bool isMaxRange() const;
      bool isConstant() const;
      void print(std::ostream& OS) const;
      std::string ToString() const;
      Range add(const Range& other) const;
      Range sub(const Range& other) const;
      Range mul(const Range& other) const;
      Range udiv(const Range& other) const;
      Range sdiv(const Range& other) const;
      Range urem(const Range& other) const;
      Range srem(const Range& other) const;
      Range shl(const Range& other) const;
      Range lshr(const Range& other) const;
      Range ashr(const Range& other) const;
      Range And(const Range& other) const;
      Range Or(const Range& other) const;
      Range Xor(const Range& other) const;
      Range Eq(const Range& other, unsigned bw) const;
      Range Ne(const Range& other, unsigned bw) const;
      Range Ugt(const Range& other, unsigned bw) const;
      Range Uge(const Range& other, unsigned bw) const;
      Range Ult(const Range& other, unsigned bw) const;
      Range Ule(const Range& other, unsigned bw) const;
      Range Sgt(const Range& other, unsigned bw) const;
      Range Sge(const Range& other, unsigned bw) const;
      Range Slt(const Range& other, unsigned bw) const;
      Range Sle(const Range& other, unsigned bw) const;
      Range truncate(unsigned bitwidth) const;
      Range sextOrTrunc(unsigned bitwidth) const;
      Range zextOrTrunc(unsigned from_bitwidth) const;
      bool operator==(const Range& other) const;
      bool operator!=(const Range& other) const;
      Range intersectWith(const Range& other) const;
      Range unionWith(const Range& other) const;
      Range BestRange(const Range& UR, const Range& SR, unsigned bw) const;

      static Range makeSatisfyingCmpRegion(kind pred, const Range& Other);
   };

   static std::ostream& operator<<(std::ostream& OS, const Range& R);

   class Range_Analysis : public ApplicationFrontendFlowStep
   {
   private: 
      std::map<tree_nodeRef, Range> ranges;

   protected:

      const std::unordered_set<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   public:
      /**
       * Constructor.
       * @param _Param is the set of the parameters
       * @param _AppM is the application manager
       * @param function_id is the identifier of the function
       * @param design_flow_manager is the design flow manager
       */
      Range_Analysis(const application_managerRef AppM, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

      /**
       *  Destructor
       */
      ~Range_Analysis() override;

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

      Range getRange(const tree_nodeRef ssa_name) const;

      /** Gets the maximum bit width of the operands in the instructions of the
       * function. This function is necessary because the class APInt only
       * supports binary operations on operands that have the same number of
       * bits; thus, all the APInts that we allocate to process the function will
       * have the maximum bit size. The complexity of this function is linear on
       * the number of operands used in the function.
       */
      unsigned getMaxBitWidth(unsigned int F);
      unsigned getMaxBitWidth();
      static void updateConstantIntegers(unsigned maxBitWidth);
      void finalizeRangeAnalysis(void* CG);

   };

}

#endif // !RANGE_ANALYSIS_HPP