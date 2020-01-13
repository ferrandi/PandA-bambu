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
 * @file var_pp_functor.hpp
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 */

#ifndef _VAR_PP_FUNCTOR_HPP
#define _VAR_PP_FUNCTOR_HPP

#include "custom_set.hpp" // for CustomSet
#include "refcount.hpp"
#include <string> // for string
#include <utility>

/**
 * @name Forward declarations.
 */
//@{
REF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(BehavioralHelper);
REF_FORWARD_DECL(var_pp_functor);
//@}

/**
 * Base class functor used by prettyPrintVertex to print variables
 */
struct var_pp_functor
{
   /// Destructor
   virtual ~var_pp_functor()
   {
   }

   /**
    * This functor returns a string representing the variable (usually the name of the variable). This can be used both in variable declaration and in variable use.
    * The string returned depends on the type of manipulation performed by the backend layer.
    * @param var is the nodeid of the variable that should be analyzed.
    */
   virtual std::string operator()(unsigned int var) const = 0;
};
typedef refcount<var_pp_functor> var_pp_functorRef;
typedef refcount<const var_pp_functor> var_pp_functorConstRef;

/**
 * Standard functor that returns the name of a variable
 */
struct std_var_pp_functor : public var_pp_functor
{
 private:
   /// behavioral helper
   const BehavioralHelperConstRef BH;

 public:
   /// Constructor
   explicit std_var_pp_functor(const BehavioralHelperConstRef _BH) : BH(_BH)
   {
   }

   /// Destructor
   ~std_var_pp_functor() override = default;

   /**
    * return the name of the variable.
    * @param var is the nodeid of the variable.
    */
   std::string operator()(unsigned int var) const override;
};

/**
 * Pointer version functor that returns the name of a variable with a star in front.
 */
struct pointer_var_pp_functor : public var_pp_functor
{
   /**
    * Constructor
    * @param BH is the behavioral helper.
    * @param vars is the reference to the set of variables for which a star should be returned along with their name.
    * @param add_restrict controls the addition to the parameters declarations of the __restrict__ keyword.
    */
   pointer_var_pp_functor(const BehavioralHelperConstRef _BH, const CustomSet<unsigned int> vars, bool _add_restrict = false);

   /**
    * Destructor
    */
   ~pointer_var_pp_functor() override = default;

   /**
    * return the name of the variable with a star as a prefix.
    * @param var is the nodeid of the variable.
    */
   std::string operator()(unsigned int var) const override;

 private:
   /// reference to the set of variable that has to have a star in front when returned by operator()
   const CustomSet<unsigned int> pointer_based_variables;

   /// behavioral helper
   const BehavioralHelperConstRef BH;

   /// standard functor used for print array variable
   const std_var_pp_functor std_functor;

   /// it controls the addition to the parameters declarations of the __restrict__ keyword.
   bool add_restrict;
};

/**
 * Address version functor that returns the name of a variable with "&" in front.
 */
struct address_var_pp_functor : public var_pp_functor
{
 public:
   /**
    * Constructor
    * @param BH is the behavioral helper.
    * @param vars is the reference to the set of variables for which a star should be returned along with their name.
    */
   address_var_pp_functor(const BehavioralHelperConstRef _BH, const CustomSet<unsigned int> vars, const CustomSet<unsigned int> pointer_vars);

   /// Destructor
   ~address_var_pp_functor() override = default;

   /**
    * return the name of the variable with a star as a prefix.
    * @param var is the nodeid of the variable.
    */
   std::string operator()(unsigned int var) const override;

 private:
   /// reference to the set of variable that has to have a star in front when returned by operator()
   const CustomSet<unsigned int> addr_based_variables;

   /// reference to the set of variable that has to have a star in front when returned by operator()
   const CustomSet<unsigned int> pointer_based_variables;

   /// behavioral helper
   const BehavioralHelperConstRef BH;
};

struct isolated_var_pp_functor : public var_pp_functor
{
 private:
   /// behavioral helper
   const BehavioralHelperConstRef BH;
   unsigned int repl_var;
   std::string var_string;

 public:
   /// Constructor
   isolated_var_pp_functor(const BehavioralHelperConstRef _BH, unsigned int _repl_var, std::string _var_string) : BH(_BH), repl_var(_repl_var), var_string(std::move(_var_string))
   {
   }

   /// Destructor
   ~isolated_var_pp_functor() override = default;

   /**
    * return the name of the variable.
    * @param var is the nodeid of the variable.
    */
   std::string operator()(unsigned int var) const override;
};

#endif
