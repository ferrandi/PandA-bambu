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
#ifndef CALL_SITES_COLLECTOR_VISITOR_HPP
#define CALL_SITES_COLLECTOR_VISITOR_HPP

#include "call_graph.hpp"

#include <boost/graph/visitors.hpp>

#include "custom_map.hpp"
#include "custom_set.hpp"

CONSTREF_FORWARD_DECL(CallGraphManager);
REF_FORWARD_DECL(HLS_manager);

class CallSitesCollectorVisitor : public boost::default_dfs_visitor
{
 private:
   /// A refcount to the HLSMgr
   const HLS_managerRef HLSMgr;

   /// A refcount to the call graph manager
   const CallGraphManagerConstRef CGMan;

 public:
   /**
    * Constructor
    */
   CallSitesCollectorVisitor(const HLS_managerRef& _HLSMgr);

   /**
    * Destructor
    */
   ~CallSitesCollectorVisitor();

   void start_vertex(const vertex&, const CallGraph&);

   void back_edge(const EdgeDescriptor&, const CallGraph&);

   void examine_edge(const EdgeDescriptor&, const CallGraph&);

   void discover_vertex(const vertex& v, const CallGraph&);
};
#endif
