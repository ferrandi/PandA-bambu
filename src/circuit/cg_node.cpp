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
 *              Copyright (C) 2004-2020 Politecnico di Milano
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
 * @file cg_node.cpp
 * @brief Node, edge and graph description of the graph associated with a structural description.
 * This file also provides some classes used by graphviz writer.
 *
 * @author Matteo Barbati <matteo.mbarbati@gmail.com>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "cg_node.hpp"
#include "exceptions.hpp"         // for THROW_ERROR
#include "structural_objects.hpp" // for structural_object, structural_obje...

void cg_edge_info::print(std::ostream& os) const
{
   if(from_port->get_kind() == constant_o_K)
   {
      os << GetPointer<constant_o>(from_port)->get_value();
   }
   else if(from_port->get_owner()->get_kind() == port_vector_o_K)
      os << from_port->get_owner()->get_id() << "[" << from_port->get_id() << "]";
   else
      os << from_port->get_id();
   os << "-";
   if(to_port->get_owner()->get_kind() == port_vector_o_K)
      os << to_port->get_owner()->get_id() << "[" << to_port->get_id() << "]";
   else
      os << to_port->get_id();
}

cg_edge_writer::cg_edge_writer(const graph* _g) : g(_g)
{
}

void cg_edge_writer::operator()(std::ostream& out, const EdgeDescriptor& e) const
{
   const auto* edge_info = Cget_edge_info<cg_edge_info>(e, *g);
   bool is_critical = false;
   if(edge_info)
      is_critical = edge_info->is_critical;
   if(is_critical)
      out << "[color=red, ";
   else if(DATA_SELECTOR & g->GetSelector(e))
      out << "[color=blue, ";
   else if(CLOCK_SELECTOR & g->GetSelector(e))
      out << "[color=yellow, ";
   else if(CHANNEL_SELECTOR & g->GetSelector(e))
      out << "[color=green, ";
   else
      THROW_ERROR(std::string("InconsistentDataStructure"));
   if(edge_info)
   {
      out << "label=\"";
      edge_info->print(out);
      out << "\"";
   }
   out << "]";
}

cg_label_writer::cg_label_writer(const graph* _g) : g(_g)
{
}

void cg_label_writer::operator()(std::ostream& out, const vertex& v) const
{
   bool is_critical = GET_CRITICAL(g, v);
   if(GET_TYPE(g, v) == TYPE_ENTRY || GET_TYPE(g, v) == TYPE_EXIT)
   {
      out << "[color=" << (is_critical ? "red" : "blue") << ",shape=Msquare";
   }
   else
   {
      out << "[shape=box";
      if(is_critical)
         out << ",color=red";
   }
   out << ", label=\"" << GET_NAME(g, v) << " \\n" << GET_OPERATION(g, v) << "\"]";
}
