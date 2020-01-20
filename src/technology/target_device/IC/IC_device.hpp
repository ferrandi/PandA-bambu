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
 * @file IC_device.hpp
 * @brief This class represents an integrated circuit as target device for the logic synthesis process
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 */

#ifndef _IC_DEVICE_HPP_
#define _IC_DEVICE_HPP_

#include "target_device.hpp"

class IC_device : public target_device
{
 protected:
   /**
    * Initialize the target device based on the given parameters
    */
   void initialize() override;

 public:
   /**
    * Constructor of the class
    * @param Param is the reference to the class that contains all the parameters
    */
   IC_device(const ParameterConstRef Param, const technology_managerRef TM);

   /**
    * Destructor of the class
    */
   ~IC_device() override;

   /**
    * Load all data specific for a given technology
    */
   void load_devices(const target_deviceRef device) override;

   /**
    * Set the proper dimension for the target device
    */
   virtual void set_dimension(double area);

   /**
    * Method to write an XML node
    * @param node is the node for writing the information
    */
   void xwrite(xml_element* node) override;
};

#endif
