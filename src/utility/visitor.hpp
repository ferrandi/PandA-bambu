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
 * @file visitor.hpp
 * @brief Visitor/walker design pattern.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef VISITOR_HPP
#define VISITOR_HPP

#include "refcount.hpp"
#include <algorithm>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <deque>

/// return the id given a super class or a class member
#define GETID(field) field##_ID
/// macro used to visit the super class
#define VISIT_SC(mask, superclass, method)    \
   if(((1 << GETID(superclass)) & mask) == 0) \
   this->superclass::method
/// macro used to visit a non null member
#define VISIT_MEMBER_NAMED(ref_obj_name, mask, ref_obj, method) \
   if(((1 << GETID(ref_obj_name)) & mask) == 0 && ref_obj)      \
   ref_obj->method
#define VISIT_MEMBER(mask, ref_obj, method) VISIT_MEMBER_NAMED(ref_obj, mask, ref_obj, method)
/// macro used to traverse non empty sequences
#define SEQ_VISIT_MEMBER(mask, seq, seqbasetype, method, visitor_type, visitor_obj) \
   if(((1 << GETID(seq)) & mask) == 0 && !seq.empty())                              \
   std::for_each(seq.begin(), seq.end(), for_each_functor<seqbasetype, visitor_type>(&seqbasetype::method, visitor_obj))
/// constant used to avoid member visit
#define NO_VISIT ~0U
/// constant used to allow member visit
#define ALL_VISIT 0U
/// macro used to selectively avoid member visit
#define SET_NO_VISIT_INDEX(mask, index) (mask |= (1U << GETID(index)))
/// macro used to selectively allow member visit
#define SET_VISIT_INDEX(mask, index) (mask &= ~(1U << GETID(index)))

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
   /// destructor
   virtual ~object_visitor()
   {
   }
   /**
    * virtual template object functor
    * @param obj is the object visited
    * @param mask control the visit of the object members.
    * the next macro will define a set of virtual function starting from VISITED_OBJ_SEQ1 VISITED_OBJ_SEQ2 following the schema:
    *
    * virtual void operator()(const VISITED_OBJ * obj, unsigned int & mask) {}
    *
    * where
    * VISITED_OBJ is taken from the sequence VISITED_OBJ_SEQ1 VISITED_OBJ_SEQ2
    */
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO, BOOST_PP_EMPTY, VISITED_OBJ_SEQ1);
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO, BOOST_PP_EMPTY, VISITED_OBJ_SEQ2);
#undef VISITED_OBJ_SEQ1
#undef VISITED_OBJ_SEQ2
};

/// functor used to traverse sequences with std::for_each
template <class T, class visitor_obj>
class for_each_functor : public std::unary_function<const refcount<T>&, void>
{
   /// visitor type definition
   typedef visitor_obj* const visitor_type;
   typedef void (T::*const visitor_function_type)(visitor_type) const;

 public:
   /// constructor
   for_each_functor(const visitor_function_type _visitor_function, visitor_type _vo) : visitor_function(_visitor_function), vo(_vo)
   {
   }
   /// functor
   void operator()(const refcount<T>& x) const
   {
      ((x.get())->*visitor_function)(vo);
   }

 private:
   /// visitor member
   const visitor_function_type visitor_function;
   /// visitor object
   visitor_type vo;
};
#endif
