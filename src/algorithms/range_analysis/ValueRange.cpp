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
 * @file ValueRange.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "ValueRange.hpp"

#include <sstream>

ValueRange::ValueRange(const Range& _range) : range(_range)
{
}

ValueRange::~ValueRange()
{
}

ValueRangeType ValueRange::getValueId() const
{
   return ValueRangeId;
}

bool ValueRange::tryGetRange(Range& out) const
{
   out = getRange();
   return true;
}

void ValueRange::print(std::ostream& OS) const
{
   getRange().print(OS);
}

std::string ValueRange::ToString() const
{
   std::stringstream ss;
   print(ss);
   return ss.str();
}
