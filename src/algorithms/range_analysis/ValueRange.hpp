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
 *              Copyright (C) 2019-2024 Politecnico di Milano
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
 * @file ValueRange.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _RANGE_ANALYSIS_VALUE_RANGE_HPP_
#define _RANGE_ANALYSIS_VALUE_RANGE_HPP_
#include "Range.hpp"

#include <string>

enum ValueRangeType
{
   ValueRangeId,
   SymbRangeId
};

class ValueRange
{
 private:
   RangeConstRef range;

 public:
   explicit ValueRange(const RangeConstRef& range);
   virtual ~ValueRange();
   ValueRange(const ValueRange&) = delete;
   ValueRange(ValueRange&&) = delete;
   ValueRange& operator=(const ValueRange&) = delete;
   ValueRange& operator=(ValueRange&&) = delete;

   inline RangeConstRef getRange() const
   {
      return range;
   }

   inline void setRange(const RangeConstRef& newRange)
   {
      range.reset(newRange->clone());
   }

   virtual void print(std::ostream& OS) const;
   std::string ToString() const;

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