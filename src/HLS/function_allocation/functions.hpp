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
 * @file functions.hpp
 * @brief Datastructure to describe functions allocation in high-level synthesis
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef FUNCTIONS_HPP
#define FUNCTIONS_HPP

#include "refcount.hpp"

REF_FORWARD_DECL(HLS_manager);

#include "custom_map.hpp"
#include "custom_set.hpp"

class functions
{
   /// set of function proxies called by a function
   std::map<unsigned int, CustomOrderedSet<std::string>> shared_function_proxy;

   /// define where the proxied functions are mapped
   std::map<std::string, unsigned int> proxied_functions;

   /// map where shared functions are allocated
   std::map<unsigned int, CustomOrderedSet<std::string>> shared_functions;

 public:
   /**
    * Constructor
    */
   functions();

   /**
    * Destructor
    */
   virtual ~functions();

   /**
    * allocate a shared function in a specified function.
    * @param funID_scope if the function id where the shared function is allocated
    * @param fun is the shared function
    */
   void map_shared_function(unsigned int funID_scope, const std::string& fun);

   /**
    * return the set of shared functions allocated in a given function.
    * @param funID_scope is the function where the shared functions have been allocated
    * @return the set of shared functions allocated in funID_scope
    */
   const CustomOrderedSet<std::string>& get_shared_functions(unsigned int funID_scope) const;

   /**
    * return true in case there are shared functions allocated in a given function.
    * @param funID_scope is the function id
    * @return true when there are shared functions in funID_scope
    */
   bool has_shared_functions(unsigned int funID_scope) const;

   /**
    * return true if a given function is a shared function allocated in a given function scope
    * @param funID_scope is the function scope
    * @param fun is the shared function
    * @return true when fun is a shared function in funID_scope, false otherwise
    */
   bool is_a_shared_function(unsigned int funID_scope, const std::string& fun) const;

   /**
    * allocate a proxy for the function referred within a given function
    * @param funID_scope is the function id
    * @param fun is the proxy function
    */
   void add_shared_function_proxy(unsigned int funID_scope, const std::string& fun);

   /**
    * return the proxied internal functions associated with the function
    * @param funID_scope is the function id
    * @return the set of functions proxied in funID_scope
    */
   const CustomOrderedSet<std::string>& get_proxied_shared_functions(unsigned int funID_scope) const;

   /**
    * check if the function has proxy shared functions
    * @param funID_scope is the function id
    * @return true when there are proxy shared functions, false otherwise
    */
   bool has_proxied_shared_functions(unsigned int funID_scope) const;

   /**
    * return true if a given function is a shared function allocated in a given function scope
    * @param funID_scope is the function scope
    * @param fun is the proxy shared function
    * @return true when fun is a proxied shared in funID_scope, false otherwise
    */
   bool is_a_proxied_shared_function(unsigned int funID_scope, const std::string& fun) const;

   /**
    * return true if the function is a proxied function
    * @param fun is the id of the function
    * @return true when fun is a proxied functions
    */
   bool is_a_proxied_function(const std::string& fun) const;

   /**
    * in case the function is a proxy function, it returns where the function is actually instantiated
    * @param fun is the proxy function
    * @return the function ID of the function having the instance of proxy function fun
    */
   unsigned int get_proxy_mapping(const std::string& fun) const;

   static std::string get_function_name_cleaned(const std::string& original_function_name);

   static std::string get_function_name_cleaned(unsigned funID, const HLS_managerRef HLSMgr);
};
/// refcount definition of the class
typedef refcount<functions> functionsRef;

#endif
