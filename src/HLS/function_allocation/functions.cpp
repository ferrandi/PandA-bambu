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
 *              Copyright (C) 2004-2026 Politecnico di Milano
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
 * @file memory.cpp
 * @brief Data structure describing function allocation in high-level synthesis
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "functions.hpp"
#include "behavioral_helper.hpp"
#include "constant_strings.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "library_manager.hpp"
#include "string_manipulation.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"

#include <boost/algorithm/string/predicate.hpp>

functions::functions() = default;

void functions::map_shared_function(unsigned int funID_scope, const std::string& fun)
{
   THROW_ASSERT(!is_a_proxied_function(fun),
                "function already mapped in a different scope: " + fun + "->" + STR(proxied_functions.at(fun)));
   shared_functions[funID_scope].insert(fun);
   proxied_functions[fun] = funID_scope;
}

const CustomOrderedSet<std::string>& functions::get_shared_functions(unsigned int funID_scope) const
{
   THROW_ASSERT(has_shared_functions(funID_scope), "");
   return shared_functions.at(funID_scope);
}

bool functions::has_shared_functions(unsigned int funID_scope) const
{
   return shared_functions.count(funID_scope);
}

bool functions::is_a_shared_function(unsigned int funID_scope, const std::string& fun) const
{
   return has_shared_functions(funID_scope) && get_shared_functions(funID_scope).count(fun);
}

void functions::add_shared_function_proxy(unsigned int funID_scope, const std::string& fun)
{
   shared_function_proxy[funID_scope].insert(fun);
}

const CustomOrderedSet<std::string>& functions::get_proxied_shared_functions(unsigned int funID_scope) const
{
   THROW_ASSERT(has_proxied_shared_functions(funID_scope), "No proxy functions for " + STR(funID_scope));
   return shared_function_proxy.at(funID_scope);
}

bool functions::has_proxied_shared_functions(unsigned int funID_scope) const
{
   return shared_function_proxy.count(funID_scope);
}

bool functions::is_a_proxied_shared_function(unsigned int funID_scope, const std::string& fun) const
{
   return has_proxied_shared_functions(funID_scope) && get_proxied_shared_functions(funID_scope).count(fun);
}

bool functions::is_a_proxied_function(const std::string& fun) const
{
   return proxied_functions.count(fun);
}

unsigned int functions::get_proxy_mapping(const std::string& fun) const
{
   THROW_ASSERT(is_a_proxied_function(fun), "this is not a proxy function");
   return proxied_functions.at(fun);
}

std::string functions::GetFUName(const std::string& fname, const HLS_managerRef HLSMgr)
{
   const auto HLS_D = HLSMgr->get_HLS_device();
   const auto TechM = HLS_D->get_technology_manager();
   const auto fu_node = TechM->GetFunctionFU(fname);
   return fu_node ? fu_node->get_name() : fname;
}

std::string functions::GetFUName(unsigned int funID, const HLS_managerRef HLSMgr)
{
   const auto original_function_name = HLSMgr->CGetFunctionBehavior(funID)->CGetBehavioralHelper()->GetFunctionName();
   return GetFUName(original_function_name, HLSMgr);
}
