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
 * @file cg_graph.cpp
 * @brief Node, edge and graph description of the graph associated with a structural description.
 * This file also provides some classes used by graphviz writer.
 *
 * @author Matteo Barbati <matteo.mbarbati@gmail.com>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "cg_graph.hpp"

#include "exceptions.hpp"
#include "structural_objects.hpp"

void CGEdgeInfo::print(std::ostream& os) const
{
   if(from_port->get_kind() == constant_o_K)
   {
      auto val = GetPointer<constant_o>(from_port)->get_value();
      val.erase(std::remove(val.begin(), val.end(), '\"'), val.end());
      os << val;
   }
   else if(from_port->get_owner()->get_kind() == port_vector_o_K)
   {
      os << from_port->get_owner()->get_id() << "[" << from_port->get_id() << "]";
   }
   else
   {
      os << from_port->get_id();
   }
   os << "-";
   if(to_port->get_owner()->get_kind() == port_vector_o_K)
   {
      os << to_port->get_owner()->get_id() << "[" << to_port->get_id() << "]";
   }
   else
   {
      os << to_port->get_id();
   }
}

void CGEdgeWriter::operator()(std::ostream& out, const CGGraph::edge_descriptor& e) const
{
   const auto& edge_info = printing_graph.CGetEdgeInfo(e);
   if(edge_info.is_critical)
   {
      out << "[color=red, ";
   }
   else if(DATA_SELECTOR & printing_graph.GetSelector(e))
   {
      out << "[color=blue, ";
   }
   else if(CLOCK_SELECTOR & printing_graph.GetSelector(e))
   {
      out << "[color=yellow, ";
   }
   else
   {
      THROW_ERROR(std::string("InconsistentDataStructure"));
   }
   out << "label=\"";
   edge_info.print(out);
   out << "\"]";
}

void CGVertexWriter::operator()(std::ostream& out, CGGraph::vertex_descriptor v) const
{
   const auto& node_info = printing_graph.CGetNodeInfo(v);
   if(node_info.node_type == TYPE_ENTRY || node_info.node_type == TYPE_EXIT)
   {
      out << "[color=" << (node_info.is_critical ? "red" : "blue") << ",shape=Msquare";
   }
   else
   {
      out << "[shape=box";
      if(node_info.is_critical)
      {
         out << ",color=red";
      }
   }
   out << ", label=\"" << node_info.vertex_name << " \\n" << node_info.node_operation << "\"]";
}
