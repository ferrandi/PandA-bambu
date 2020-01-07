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
 * @file target_manager.hpp
 * @brief Definition of the class representing a target for the synthesis
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _TARGET_MANAGER_HPP_
#define _TARGET_MANAGER_HPP_

/// Utility include
#include "refcount.hpp"
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(technology_manager);
REF_FORWARD_DECL(target_device);
REF_FORWARD_DECL(target_technology);

class target_manager
{
 protected:
   /// class containing all the parameters
   const ParameterConstRef Param;

   /// information about the technology library
   technology_managerRef TM;

   /// reference to the information about the target device
   target_deviceRef device;

   /// reference to the target technology
   target_technologyRef target;

 public:
   /**
    * Constructor
    * @param _Param is the reference to the class containing all the parameters
    */
   target_manager(const ParameterConstRef& _Param, const technology_managerRef& _TM, const target_deviceRef& device);

   /**
    * Destructor
    */
   virtual ~target_manager();

   /**
    * Sets the technology manager
    */
   void set_technology_manager(const technology_managerRef& _TM);

   /**
    * Returns the current technology manager
    */
   const technology_managerRef get_technology_manager() const;

   /**
    * Sets the reference to the target technology manager class
    */
   void set_target_technology(const target_technologyRef& _target);

   /**
    * Returns the reference to the class representing the target technology
    */
   const target_technologyRef get_target_technology() const;

   /**
    * Sets the reference to the target device class
    */
   void set_target_device(const target_deviceRef& _device);

   /**
    * Returns the reference to the class containing information about the target device
    */
   const target_deviceRef get_target_device() const;
};
/// refcount definition of the class
typedef refcount<target_manager> target_managerRef;
/// constant refcount definition of the class
typedef refcount<const target_manager> target_managerConstRef;

#endif
