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
 * @file hls_target.cpp
 * @brief Implementation of some methods to manage the target for the HLS
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "hls_target.hpp"

/// ----------- Resource Library ----------- ///
/// Resource Library Datastructure
#include "technology_manager.hpp"
/// ----------- Target Device ----------- ///
#include "target_device.hpp"
#include "target_technology.hpp"

#include "Parameter.hpp"
#include "fileIO.hpp"
#include "polixml.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

HLS_target::HLS_target(const ParameterConstRef& _Param, const technology_managerRef& _TM, const target_deviceRef& _target) : target_manager(_Param, _TM, _target)
{
   if(Param->isOption(OPT_clock_period))
   {
      auto clock_period_value = Param->getOption<double>(OPT_clock_period);
      device->set_parameter("clock_period", clock_period_value);
   }
   auto output_level = Param->getOption<unsigned int>(OPT_output_level);
   PRINT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "Target technology = " + target->get_string_type());
}

HLS_target::~HLS_target() = default;

HLS_targetRef HLS_target::create_target(const ParameterRef& Param)
{
   technology_managerRef TM = technology_managerRef(new technology_manager(Param));
   if(Param->isOption(OPT_xml_input_configuration))
   {
      try
      {
         auto fn = Param->getOption<std::string>(OPT_xml_input_configuration);
         XMLDomParser parser(fn);
         parser.Exec();
         if(parser)
         {
            // Walk the tree:
            const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.

            const xml_node::node_list list = node->get_children();
            for(const auto& iter : list)
            {
               const auto* Enode = GetPointer<const xml_element>(iter);
               if(!Enode)
               {
                  continue;
               }
               if(Enode->get_name() == "device")
               {
                  std::string type = "FPGA";
                  if(CE_XVM(type, Enode))
                  {
                     LOAD_XVM(type, Enode);
                  }
                  TargetDevice_Type type_device = TargetDevice_Type::FPGA;
                  if(type == "IC")
                  {
                     type_device = TargetDevice_Type::IC;
                  }
                  Param->setOption(OPT_target_device_type, static_cast<int>(type_device));

                  target_deviceRef target = target_device::create_device(type_device, Param, TM);
                  target->xload(target, node);
                  return HLS_targetRef(new HLS_target(Param, TM, target));
               }
            }
         }
      }
      catch(const char* msg)
      {
         THROW_ERROR("Error during technology file parsing: " + std::string(msg));
      }
      catch(const std::string& msg)
      {
         THROW_ERROR("Error during technology file parsing: " + msg);
      }
      catch(const std::exception& ex)
      {
         THROW_ERROR("Error during technology file parsing: " + std::string(ex.what()));
      }
      catch(...)
      {
         THROW_ERROR("Error during technology file parsing");
      }
   }
   auto type_device = Param->getOption<unsigned int>(OPT_target_device_type);
   target_deviceRef target = target_device::create_device(static_cast<TargetDevice_Type>(type_device), Param, TM);
   return HLS_targetRef(new HLS_target(Param, TM, target));
}
