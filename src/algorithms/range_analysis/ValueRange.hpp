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
 * @file ValueRange.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef _RANGE_ANALYSIS_VALUE_RANGE_HPP_
#define _RANGE_ANALYSIS_VALUE_RANGE_HPP_
#include "Range.hpp"

#include <string>

enum ValueRangeType
{
   ValueRangeId,
   SymbRangeId,
   ValueSetRangeId
};

class ValueRange
{
 private:
   Range range;

 public:
   explicit ValueRange(const Range& range);
   virtual ~ValueRange();
   ValueRange(const ValueRange&) = delete;
   ValueRange(ValueRange&&) = delete;
   ValueRange& operator=(const ValueRange&) = delete;
   ValueRange& operator=(ValueRange&&) = delete;

   inline const Range& getRange() const
   {
      return range;
   }

   inline void setRange(const Range& newRange)
   {
      range = newRange;
   }

   virtual void print(std::ostream& OS) const;
   std::string ToString() const;

   virtual bool tryGetRange(Range& out) const;

   virtual ValueRangeType getValueId() const;

   static inline bool classof(ValueRange const*)
   {
      return true;
   }
};

inline std::ostream& operator<<(std::ostream& OS, const ValueRange* BI)
{
   BI->print(OS);
   return OS;
}

template <class T>
inline T* GetVR(const ValueRange* t)
{
   return T::classof(t) ? static_cast<T*>(t) : nullptr;
}

#endif // _RANGE_ANALYSIS_VALUE_RANGE_HPP_
