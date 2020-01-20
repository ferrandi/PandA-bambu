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
 * @file prettyPrintVertex.hpp
 * @brief Helper class supporting the printing of vertexes of a graph.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *

*/
#ifndef PRETTYPRINTVERTEX_HPP
#define PRETTYPRINTVERTEX_HPP

/// Graph include
#include "graph.hpp"

#include "custom_set.hpp"
#include <ostream>
#include <string> // for string

/// Utility include
#include "refcount.hpp"

/**
 * @name forward declarations
 */
//@{
class simple_indent;
CONSTREF_FORWARD_DECL(BehavioralHelper);
CONSTREF_FORWARD_DECL(OpGraph);
REF_FORWARD_DECL(prettyPrintVertex);
CONSTREF_FORWARD_DECL(var_pp_functor);
REF_FORWARD_DECL(pointer_var_pp_functor);
//@}

/**
 * Class used to print a vertex of a graph.
 * Let us consider the following example:
 *
  \verbatim
    typedef int MYINT;
    void simple_add(MYINT a, int b, MYINT * res)
    {
      *res = a + b + b + a;
    }
  \endverbatim
 * The obtained data flow graph for this simple piece of C code is:
  \dot
  digraph G {
  0[color=blue,shape=Msquare,label="ENTRY"];
  1[label="simple_add_1_31\nInternal = a + b;\nInternal_0 = Internal;\n"];
  2[label="simple_add_1_33\nInternal_1 = Internal_0 + b;\nInternal_2 = Internal_1;\n"];
  3[label="simple_add_1_35\nInternal_3 = Internal_2 + a;\nInternal_4 = Internal_3;\n"];
  4[label="simple_add_1_21\n*((int*)(((unsigned int)res))) = Internal_4;\n"];
  5[color=blue,shape=Msquare,label="EXIT"];
  0->1 [color=blue,label="a b "];
  1->2 [color=blue,label="30 "];
  2->3 [color=blue,label="32 "];
  3->4 [color=blue,label="34 "];
  0->2 [color=blue,label="b "];
  0->3 [color=blue,label="a "];
  0->4 [color=blue,label="res "];
  }
 \enddot
 The expression is splitted in four vertexes each one performing an addition or a result storage.
 In particular, the first vertex, simple_add_1_31, compute the first addition a+b but the statements stored in the vertex
 are actually two: the first one compute the addition (stored in the PandA artificial variable with nodeid 31) and the second one store the result in the var
 written (GCC artificial variable with nodeid 30) associated with the vertex. The considered vertex has
 \verbatim
   VAR_READ(simple_add_1_31) = 4(a) 5(b)
   VAR_WRITTEN(simple_add_1_31) = 30(Internal_0)
 \endverbatim
 but to correctly dump a C description before write the two statements with the print_statement
 member function we need to declare the two variables used by the vertex.
 To obtain these two variables the macro VAR_READ and VAR_WRITTEN are not enough since the variable 31(Internal) is missing.
 To obtain all the referenced variables in the vertex simple_add_1_31 we have to use the get_referenced_vars member function.
 Given the list of variables referenced by the vertex of the graph we need to print the possible type declarations for
 the variables based on user defined types.
 In the example the parameter a and res are of type MYINT, therefore, before print the declarations of the variables we have to print
 the declaration of the user type MYINT. To do that we have first to detect the set of user type declarations and then we can print the declaration with
 print_type_declaration. To check if a type is a user defined type we can use behavior_helper::is_built_in_type while to get the type from a variable we can use behavior_helper::get_type.
 After the type definition we can print the declarations of the variables with print_var_declaration.
*/
struct prettyPrintVertex
{
   /**
    * This function prints the forward type declaration of a type.
    * @param os is the output stream.
    * @param type is the nodeid of the type.
    * @param PP is the pretty print functor object used to indent the generated code.
    * @param BH is the behavioral helper.
    */
   static void print_forward_declaration(std::ostream& os, unsigned int type, simple_indent& PP, const BehavioralHelperConstRef BH);

   static void get_internal_vars(const vertex& v, const OpGraphConstRef g, CustomUnorderedSet<unsigned int>& list_of_variables, const BehavioralHelperConstRef BH);

 private:
   /**
    * return a string for the passed statement. The referenced variables are printed with the indenter and variable functor.
    * @param vppf is the functor used to dump the variable var.
    * @param PP is the pretty print functor object used to indent the generated code.
    * @param BH is the behavioral helper.
    */
   static std::string statement2string(unsigned int stm, var_pp_functor& vppf, simple_indent& PP, const BehavioralHelperConstRef BH);
};

#endif
