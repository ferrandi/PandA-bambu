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
 * @file raw_writer.hpp
 * @brief tree node writer. This class exploiting the visitor design pattern write a tree node according to the raw format.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef RAW_WRITER_HPP
#define RAW_WRITER_HPP

/// Autoheader include
#include "config_HAVE_MAPPING_BUILT.hpp"
#include "config_HAVE_RTL_BUILT.hpp"

#if HAVE_RTL_BUILT
#include "rtl_common.hpp"
#endif

/// STD include
#include <ostream>

/// Tree include
#include "tree_node.hpp"

/// Utility include
#include "refcount.hpp"
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(ComponentType);
REF_FORWARD_DECL(raw_writer);
//@}

struct raw_writer : public tree_node_visitor
{
   /// default constructor
   explicit raw_writer(
#if HAVE_MAPPING_BUILT
       const ComponentTypeRef& _driving_component,
#endif
       std::ostream& _os);

   /**
    * Write the field when t is not null
    * @param str is the string key associated to t
    * @param t is the tree_nodeRef
    */
   void write_when_not_null(const std::string& str, const tree_nodeRef& t) const;

   /**
    * Write the field when t is not null
    * @param str is the string key associated to t
    * @param t is the blocRef
    */
   void write_when_not_null_bloc(const std::string& str, const blocRef& t);

   /**
    * Write a point to solution when is not null
    * @param type is the type of point to solution (i.e., use or clb)
    * @param solution is the solution to be printed
    */
   void write_when_not_null_point_to(const std::string& type, const PointToSolutionRef& solution) const;

   /// tree_node visitors
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO_DECL, BOOST_PP_EMPTY, OBJ_SPECIALIZED_SEQ)
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO, BOOST_PP_EMPTY, OBJ_NOT_SPECIALIZED_SEQ)

 private:
#if HAVE_MAPPING_BUILT
   /// The driving component
   const ComponentTypeRef& driving_component;
#endif

   /// output stream
   std::ostream& os;
};

#endif
