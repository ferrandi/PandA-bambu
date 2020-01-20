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
 * @file tree_reindex.cpp
 * @brief Class implementation of the tree_reindex support class.
 *
 * This class is used during the tree walking to store the NODE_ID value.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @version $Revision$
 * @date $Date$
 * @warning This file is still in a work in progress state
 * @warning Last modified by $Author$
 *
 */
#include "tree_reindex.hpp"

tree_reindex::tree_reindex(const unsigned int i, const tree_nodeRef& tn) : tree_node(i), actual_tree_node(tn)
{
}

tree_reindex::~tree_reindex() = default;

void tree_reindex::print(std::ostream& os) const
{
   if(html)
   {
      os << "<a href=\"" << index << "\">@" << index << "</a>";
   }
   else
   {
      os << "@" << index;
   }
}

void tree_reindex::visit(tree_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_MEMBER(mask, actual_tree_node, visit(v));
}

bool tree_reindex::html = false;

bool lt_tree_reindex::operator()(const tree_nodeRef& x, const tree_nodeRef& y) const
{
   return GET_INDEX_NODE(x) < GET_INDEX_NODE(y);
}
