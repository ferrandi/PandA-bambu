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
 * @file tree_reindex.hpp
 * @brief Class specification of the tree_reindex support class.
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
#ifndef TREE_REINDEX_HPP
#define TREE_REINDEX_HPP

#include <tree_node.hpp>

/**
This class is used to perform the re-index of all tree nodes.
Each reference to a tree_node is implemented by this type of tree_node
*/
class tree_reindex : public tree_node
{
 private:
   friend class tree_manager;

   /**
    * Private constructor with index initialization
    * It can be accesses only by tree_manager
    * @param ind is the value of the index member.
    * @param tn is the actual reference to the tree_node.
    * NOTE: this has to be a reference since tree_nodeRef at the moment of the construction of this can not ready
    */
   tree_reindex(const unsigned int ind, const tree_nodeRef& tn);

 public:
   /**
    * Represent the actual reference to the tree_node.
    * NOTE: this has to be a reference since tree_nodeRef at the moment of the construction of this can not ready
    */
   const tree_nodeRef& actual_tree_node;

   static bool html;

   ~tree_reindex() override;

   /**
    * function that prints the class tree_reindex.
    */
   void print(std::ostream& os) const;
   /**
    * Redefinition of get_tree_node_kind_text.
    */
   GET_KIND_TEXT(tree_reindex)

   /**
    * Redefinition of get_tree_node_kind.
    */
   GET_KIND(tree_reindex)

   /**
    * Switch the print function to the html format
    */
   static void enable_html()
   {
      html = true;
   }
   static void enable_raw()
   {
      html = false;
   }
   /**
    * virtual function used to traverse the tree_node data structure.
    * @param v is a reference to the tree_node visitor class
    */
   void visit(tree_node_visitor* const v) const override;
   /// visitor enum
   enum
   {
      GETID(tree_node) = 0,
      GETID(actual_tree_node)
   };
};

/// functor used to correctly compare two tree_reindex
struct lt_tree_reindex
{
   /// operator() used to compare two tree_reindex
   bool operator()(const tree_nodeRef& x, const tree_nodeRef& y) const;
};

#endif
