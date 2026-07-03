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
