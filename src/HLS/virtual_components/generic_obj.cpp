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
 *              Copyright (C) 2015-2026 Politecnico di Milano
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
 * @file generic_obj.cpp
 * @brief Base class for all resources into datapath
 *
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "generic_obj.hpp"

#include "exceptions.hpp"

#if !HAVE_UNORDERED
GenericObjSorter::GenericObjSorter() = default;

bool GenericObjSorter::operator()(const generic_objRef& x, const generic_objRef& y) const
{
   if(x == y)
   {
      return false;
   }
   THROW_ASSERT(x->get_string() != y->get_string() or x->get_string().find("CONSTANT") != std::string::npos,
                x->get_string());
   return x->get_string() < y->get_string();
}

GenericObjUnsignedIntSorter::GenericObjUnsignedIntSorter() = default;

bool GenericObjUnsignedIntSorter::operator()(const std::pair<generic_objRef, int>& x,
                                             const std::pair<generic_objRef, int>& y) const
{
   if(x.first == y.first)
   {
      return x.second < y.second;
   }
   THROW_ASSERT(x.first->get_string() != y.first->get_string(), x.first->get_string());
   return x.first->get_string() < y.first->get_string();
}

#endif

bool generic_obj::operator<(const generic_obj& other) const
{
   return get_string() < other.get_string();
}
