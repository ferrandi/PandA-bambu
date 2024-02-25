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
 * @file ConditionalValueRange.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "ConditionalValueRange.hpp"

#include "ValueRange.hpp"

ConditionalValueRange::ConditionalValueRange(const tree_nodeConstRef& _V,
                                             const std::map<unsigned int, ValueRangeRef>& _bbVR)
    : V(_V), bbVR(_bbVR)
{
}

ConditionalValueRange::ConditionalValueRange(const tree_nodeConstRef& _V, unsigned int TrueBBI, unsigned int FalseBBI,
                                             const ValueRangeRef& TrueVR, const ValueRangeRef& FalseVR)
    : V(_V), bbVR({{FalseBBI, FalseVR}, {TrueBBI, TrueVR}})
{
}

void ConditionalValueRange::addVR(unsigned int bbi, const ValueRangeRef& cvr)
{
   if(!bbVR.count(bbi))
   {
      bbVR.insert(std::make_pair(bbi, cvr));
   }
   // TODO: maybe find some way to combine two ValueRange instances (difficult because of symbolic ranges)
}
