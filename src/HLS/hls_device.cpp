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
 *              Copyright (C) 2023 Politecnico di Milano
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
 * @file hls_device.cpp
 * @brief HLS specialization of generic_device
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
/// Header include
#include "hls_device.hpp"

#include "Parameter.hpp"
#include "fileIO.hpp"
#include "generic_device.hpp"
#include "polixml.hpp"
#include "technology_manager.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

HLS_device::HLS_device(const ParameterConstRef& _Param, const technology_managerRef& _TM) : generic_device(_Param, _TM)
{
   if(_Param->isOption(OPT_clock_period))
   {
      auto clock_period_value = _Param->getOption<double>(OPT_clock_period);
      set_parameter("clock_period", clock_period_value);
   }
}

HLS_device::~HLS_device() = default;

HLS_deviceRef HLS_device::factory(const ParameterRef& Param)
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
                  auto target = HLS_deviceRef(new HLS_device(Param, TM));
                  target->xload(node);
                  return target;
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
   return HLS_deviceRef(new HLS_device(Param, TM));
}
