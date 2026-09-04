/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file functions.hpp
 * @brief Datastructure to describe functions allocation in high-level synthesis
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
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

   /// reverse map of proxied_functions
   std::map<unsigned int, CustomOrderedSet<std::string>> shared_functions;

 public:
   functions();

   virtual ~functions() = default;

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

   /**
    * Return FU used to implement given function
    * @param fname function name
    * @param HLSMgr HLS manager reference
    * @return std::string Functional unit with fname in the supported operations' set
    */
   static std::string GetFUName(const std::string& fname, const HLS_managerRef HLSMgr);

   static std::string GetFUName(unsigned funID, const HLS_managerRef HLSMgr);
};

#endif
