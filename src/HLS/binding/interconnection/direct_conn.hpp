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
 * @file direct_conn.hpp
 * @brief Class adopted to represent direct connections inside the datapath
 *
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */

#ifndef _DIRECT_CONN_HPP_
#define _DIRECT_CONN_HPP_

#include "connection_obj.hpp"
#include "refcount.hpp"

/**
 * Class to represent direct connections
 */
class direct_conn : public connection_obj
{
 public:
   /**
    * Costructor.
    */
   explicit direct_conn(const CustomOrderedSet<data_transfer>& _live_variable)
       : connection_obj(DIRECT_CONN, _live_variable)
   {
   }

   /**
    * Returns the name associated with the element
    * @return a string containing the identifier
    */
   const std::string get_string() const override
   {
      return "DIRECT_CONNECTION";
   }
};
/// refcount definition of the class
using direct_connRef = refcount<direct_conn>;

#endif
