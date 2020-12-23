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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 */

// header for this class
#include "CallSitesCollectorVisitor.hpp"

#include <utility>

#include "Discrepancy.hpp"
#include "call_graph_manager.hpp"
#include "hls_manager.hpp"
#include "string_manipulation.hpp" // for STR

CallSitesCollectorVisitor::CallSitesCollectorVisitor(const HLS_managerRef& _HLSMgr) : HLSMgr(_HLSMgr), CGMan(_HLSMgr->CGetCallGraphManager())
{
}

CallSitesCollectorVisitor::~CallSitesCollectorVisitor() = default;

void CallSitesCollectorVisitor::start_vertex(const vertex&, const CallGraph&)
{
   THROW_ASSERT(HLSMgr->RDiscr, "Discrepancy data structure not initialized");
}

void CallSitesCollectorVisitor::discover_vertex(const vertex& v, const CallGraph&)
{
   const unsigned int this_fun_id = CGMan->get_function(v);
   HLSMgr->RDiscr->call_sites_info->fu_id_to_call_ids[this_fun_id];
}

void CallSitesCollectorVisitor::back_edge(const EdgeDescriptor&, const CallGraph&)
{
   THROW_ERROR("Recursive functions not supported");
}

void CallSitesCollectorVisitor::examine_edge(const EdgeDescriptor& e, const CallGraph& g)
{
   const unsigned int called_id = CGMan->get_function(boost::target(e, g));
   const unsigned int caller_id = CGMan->get_function(boost::source(e, g));
   for(const unsigned int callid : g.CGetFunctionEdgeInfo(e)->direct_call_points)
   {
      HLSMgr->RDiscr->call_sites_info->fu_id_to_call_ids[caller_id].insert(callid);
      THROW_ASSERT(HLSMgr->RDiscr->call_sites_info->call_id_to_called_id[callid].empty() or callid == 0, "direct call " + STR(callid) + " calls more than one function");
      HLSMgr->RDiscr->call_sites_info->call_id_to_called_id[callid].insert(called_id);
   }
   for(const unsigned int callid : g.CGetFunctionEdgeInfo(e)->indirect_call_points)
   {
      HLSMgr->RDiscr->call_sites_info->fu_id_to_call_ids[caller_id].insert(callid);
      HLSMgr->RDiscr->call_sites_info->call_id_to_called_id[callid].insert(called_id);
      HLSMgr->RDiscr->call_sites_info->indirect_calls.insert(callid);
   }
   //   for(const unsigned int callid : g.CGetFunctionEdgeInfo(e)->function_addresses)
   //   {
   //      HLSMgr->RDiscr->call_sites_info->taken_addresses.insert(callid);
   //   }
}
