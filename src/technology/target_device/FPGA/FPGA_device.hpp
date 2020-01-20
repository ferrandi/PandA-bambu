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
 * @file FPGA_device.hpp
 * @brief This class represents an FPGA as target device for the synthesis process
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 */
#ifndef _FPGA_DEVICE_HPP_
#define _FPGA_DEVICE_HPP_

#include "target_device.hpp"

class FPGA_device : public target_device
{
 public:
   /**
    * Constructor
    * @param Param is the reference to the class that contains all the parameters
    * @param TM is the reference to the class containing all the technology libraries
    */
   FPGA_device(const ParameterConstRef& Param, const technology_managerRef& TM);

   /**
    * Destructor
    */
   ~FPGA_device() override;

   /**
    * Initializes the target device based on the given parameters
    */
   void initialize() override;

   /**
    * Method to write an XML node
    * @param node is the node for writing the information
    */
   void xwrite(xml_element* node) override;

   /**
    * load all the data for the given FPGA
    */
   void load_devices(const target_deviceRef device) override;
};

#endif
