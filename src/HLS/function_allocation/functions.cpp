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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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
 * @file memory.cpp
 * @brief Data structure describing function allocation in high-level synthesis
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "functions.hpp"
#include "behavioral_helper.hpp"
#include "constant_strings.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"
#include "library_manager.hpp"
#include "string_manipulation.hpp" //STR
#include "technology_manager.hpp"
#include "technology_node.hpp"

#include <boost/algorithm/string/predicate.hpp>

functions::functions() = default;

functions::~functions() = default;

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
   const auto HLS_T = HLSMgr->get_HLS_target();
   const auto TechM = HLS_T->get_technology_manager();
   const auto fu_node = TechM->GetFunctionFU(fname);
   return fu_node ? fu_node->get_name() : fname;
}

std::string functions::GetFUName(unsigned int funID, const HLS_managerRef HLSMgr)
{
   const auto original_function_name = HLSMgr->CGetFunctionBehavior(funID)->CGetBehavioralHelper()->get_function_name();
   return GetFUName(original_function_name, HLSMgr);
}
