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
 * @file raw_writer.hpp
 * @brief IR node writer. This class exploiting the visitor design pattern writes an IR node according to the raw
 * format.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef RAW_WRITER_HPP
#define RAW_WRITER_HPP

#include "ir_node.hpp"
#include "refcount.hpp"

#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <ostream>

REF_FORWARD_DECL(ComponentType);
REF_FORWARD_DECL(raw_writer);

struct raw_writer : public ir_node_visitor
{
   /// default constructor
   explicit raw_writer(std::ostream& _os);

   /**
    * Write the field when t is not null
    * @param str is the string key associated to t
    * @param t is the ir_nodeRef
    */
   void write_when_not_null(const std::string& str, const ir_nodeRef& t) const;

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
   void write_when_not_null_point_to(const std::string& type, const PointToSolution& solution) const;

   /// ir_node visitors
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO_DECL, BOOST_PP_EMPTY, OBJ_SPECIALIZED_SEQ)
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO, BOOST_PP_EMPTY, OBJ_NOT_SPECIALIZED_SEQ)

 private:
   /// output stream
   std::ostream& os;
};

#endif
