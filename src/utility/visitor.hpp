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
 * @file visitor.hpp
 * @brief Visitor/walker design pattern.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef VISITOR_HPP
#define VISITOR_HPP
#include "refcount.hpp"

#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#include <algorithm>
#include <deque>

/// return the id given a super class or a class member
#define GETID(field) field##_ID
/// macro used to visit the super class
#define VISIT_SC(mask, superclass, method)      \
   if(((1 << GETID(superclass)) & (mask)) == 0) \
   this->superclass::method
/// macro used to visit a non null member
#define VISIT_MEMBER_NAMED(ref_obj_name, mask, ref_obj, method) \
   if(((1 << GETID(ref_obj_name)) & (mask)) == 0 && (ref_obj))  \
   (ref_obj)->method
#define VISIT_MEMBER(mask, ref_obj, method) VISIT_MEMBER_NAMED(ref_obj, mask, ref_obj, method)
/// macro used to traverse non empty sequences
#define SEQ_VISIT_MEMBER(mask, seq, method)                \
   if(((1 << GETID(seq)) & (mask)) == 0 && !(seq).empty()) \
   {                                                       \
      for(const auto& k : seq)                             \
         (k.get())->method;                                \
   }
/// constant used to avoid member visit
#define NO_VISIT ~0U
/// constant used to allow member visit
#define ALL_VISIT 0U
/// macro used to selectively avoid member visit
#define SET_NO_VISIT_INDEX(mask, index) ((mask) |= (1U << GETID(index)))
/// macro used to selectively allow member visit
#define SET_VISIT_INDEX(mask, index) ((mask) &= ~(1U << GETID(index)))

#ifndef VISITED_OBJ_SEQ1
#error "VISITED_OBJ_SEQ1 has to be defined in the following form #define VISITED_OBJ_SEQ1 (w)(x)(y)(z)"
#endif
#ifndef VISITED_OBJ_SEQ2
#error "VISITED_OBJ_SEQ2 has to be defined in the following form #define VISITED_OBJ_SEQ2 (w)(x)(y)(z)"
#endif

#define OPERATOR_MACRO_HEADER(r, data, elem) virtual void operator()(const elem*, unsigned int&)
#define OPERATOR_MACRO_DECL(r, data, elem) OPERATOR_MACRO_HEADER(r, data, elem);
#define OPERATOR_MACRO(r, data, elem)   \
   OPERATOR_MACRO_HEADER(r, data, elem) \
   {                                    \
   }
#define OPERATOR_MACRO_HEADERO(r, data, elem) void operator()(const elem*, unsigned int&) override
#define OPERATOR_MACRO_DECLO(r, data, elem) OPERATOR_MACRO_HEADERO(r, data, elem);
#define OPERATOR_MACROO(r, data, elem)   \
   OPERATOR_MACRO_HEADERO(r, data, elem) \
   {                                     \
   }

/// forward declaration macro
#define FORWARD_DECLARATION_MACRO(r, data, elem) class elem;
BOOST_PP_SEQ_FOR_EACH(FORWARD_DECLARATION_MACRO, BOOST_PP_EMPTY, VISITED_OBJ_SEQ1)
BOOST_PP_SEQ_FOR_EACH(FORWARD_DECLARATION_MACRO, BOOST_PP_EMPTY, VISITED_OBJ_SEQ2)
#undef FORWARD_DECLARATION_MACRO

/**
 * object visitor template. Specializing this class is possible to traverse a complex set of objects.
 */
class object_visitor
{
 public:
   virtual ~object_visitor()
   {
   }
   /*
    * Virtual template object functor.
    * The next macro defines a set of virtual functions starting from VISITED_OBJ_SEQ1 and VISITED_OBJ_SEQ2 following
    * this schema:
    *
    * virtual void operator()(const VISITED_OBJ* obj, unsigned int& mask) {}
    *
    * where VISITED_OBJ is taken from the VISITED_OBJ_SEQ1/VISITED_OBJ_SEQ2 sequences.
    */
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO, BOOST_PP_EMPTY, VISITED_OBJ_SEQ1);
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO, BOOST_PP_EMPTY, VISITED_OBJ_SEQ2);
#undef VISITED_OBJ_SEQ1
#undef VISITED_OBJ_SEQ2
};
#endif
