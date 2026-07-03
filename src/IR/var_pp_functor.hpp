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
 * @file var_pp_functor.hpp
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */

#ifndef _VAR_PP_FUNCTOR_HPP
#define _VAR_PP_FUNCTOR_HPP

#include "custom_set.hpp"
#include "refcount.hpp"

#include <string>
#include <utility>

REF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(BehavioralHelper);

/**
 * Base class functor used to print variables
 */
struct var_pp_functor
{
   virtual ~var_pp_functor() = default;

   /**
    * This functor returns a string representing the variable (usually the name of the variable). This can be used both
    * in variable declaration and in variable use. The string returned depends on the type of manipulation performed by
    * the backend layer.
    * @param var is the nodeid of the variable that should be analyzed.
    */
   virtual std::string operator()(unsigned int var) const = 0;
};

/**
 * Standard functor that returns the name of a variable
 */
struct std_var_pp_functor : public var_pp_functor
{
 private:
   /// behavioral helper
   const BehavioralHelperConstRef BH;

 public:
   explicit std_var_pp_functor(const BehavioralHelperConstRef _BH) : BH(_BH)
   {
   }

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
    * @param _BH is the behavioral helper.
    * @param vars is the reference to the set of variables for which a star should be returned along with their name.
    * @param _add_restrict controls the addition to the parameters declarations of the __restrict__ keyword.
    */
   pointer_var_pp_functor(const BehavioralHelperConstRef _BH, const CustomSet<unsigned int> vars,
                          bool _add_restrict = false);

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
   /**
    * Constructor
    * @param _BH is the behavioral helper.
    * @param vars is the reference to the set of variables for which an ampersand should be returned along with their
    * name.
    * @param pointer_vars collects the subset of variables that are also pointer-based.
    */
   address_var_pp_functor(const BehavioralHelperConstRef _BH, const CustomSet<unsigned int> vars,
                          const CustomSet<unsigned int> pointer_vars);

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
   isolated_var_pp_functor(const BehavioralHelperConstRef _BH, unsigned int _repl_var, const std::string& _var_string)
       : BH(_BH), repl_var(_repl_var), var_string(_var_string)
   {
   }

   /**
    * return the name of the variable.
    * @param var is the nodeid of the variable.
    */
   std::string operator()(unsigned int var) const override;
};

#endif
