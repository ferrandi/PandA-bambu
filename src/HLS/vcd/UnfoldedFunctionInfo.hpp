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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 *
 */
#ifndef UNFOLDED_FUNCTION_INFO_HPP
#define UNFOLDED_FUNCTION_INFO_HPP
#include "node_info.hpp"

// include from behavior/
#include "function_behavior.hpp"

// include from utility/
#include "refcount.hpp"

class UnfoldedFunctionInfo : public NodeInfo
{
 public:
   const unsigned int f_id;

   const FunctionBehaviorConstRef behavior;

   explicit UnfoldedFunctionInfo(unsigned int _f_id = 0) : f_id(_f_id)
   {
   }
   explicit UnfoldedFunctionInfo(unsigned int _f_id, const FunctionBehaviorConstRef b) : f_id(_f_id), behavior(b)
   {
   }

   ~UnfoldedFunctionInfo() override = default;
};

typedef refcount<UnfoldedFunctionInfo> UnfoldedFunctionInfoRef;
typedef refcount<const UnfoldedFunctionInfo> UnfoldedFunctionInfoConstRef;
#endif
