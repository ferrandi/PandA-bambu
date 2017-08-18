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
 *              Copyright (c) 2004-2017 Politecnico di Milano
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

// includes from behavior/
#include "call_graph_manager.hpp"

CallSitesCollectorVisitor::CallSitesCollectorVisitor(CallGraphManagerConstRef cgman,
      std::unordered_map<unsigned int, std::unordered_set<unsigned int> > & _fu_id_to_call_ids,
      std::unordered_map<unsigned int, std::unordered_set<unsigned int> > & _call_id_to_called_id,
      std::unordered_set<unsigned int> & _indirect_calls) :
   CGMan(cgman),
   fu_id_to_call_ids(_fu_id_to_call_ids),
   call_id_to_called_id(_call_id_to_called_id),
   indirect_calls(_indirect_calls)
{}

CallSitesCollectorVisitor::~CallSitesCollectorVisitor()
{}

void CallSitesCollectorVisitor::discover_vertex(const vertex & v, const CallGraph &)
{
   const unsigned int this_fun_id = CGMan->get_function(v);
   fu_id_to_call_ids[this_fun_id];
}

void CallSitesCollectorVisitor::back_edge(const EdgeDescriptor &, const CallGraph &)
{
   THROW_ERROR("Recursive functions not yet supported");
}

void CallSitesCollectorVisitor::examine_edge(const EdgeDescriptor &e, const CallGraph &g)
{
   const unsigned int called_id = CGMan->get_function(boost::target(e, g));
   const unsigned int caller_id = CGMan->get_function(boost::source(e, g));
   const std::set<unsigned int> & direct_calls =
      g.CGetFunctionEdgeInfo(e)->direct_call_points;
   const std::set<unsigned int> & indir_calls =
      g.CGetFunctionEdgeInfo(e)->indirect_call_points;
   for (const unsigned int callid : direct_calls)
   {
      fu_id_to_call_ids[caller_id].insert(callid);
      THROW_ASSERT(call_id_to_called_id[callid].empty() or callid == 0,
            "direct call " + STR(callid) + " calls more than one function");
      call_id_to_called_id[callid].insert(called_id);
   }
   for (const unsigned int callid : indir_calls)
   {
      fu_id_to_call_ids[caller_id].insert(callid);
      call_id_to_called_id[callid].insert(called_id);
      indirect_calls.insert(callid);
   }
}
