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
 * @file area_model.cpp
 * @brief Implementation of the base methods for the class area_model
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "area_model.hpp"

/// Autoheader include
#include "config_HAVE_CMOS_BUILT.hpp"

/// available models
#if HAVE_CMOS_BUILT
#include "cell_model.hpp"
#endif
#include "clb_model.hpp"

#include "target_device.hpp"

#include "exceptions.hpp"

const double area_model::area_DEFAULT = 1.0;

area_model::area_model(const ParameterConstRef& _Param_) : Param(_Param_)
{
}

area_model::~area_model() = default;

area_modelRef area_model::create_model(const TargetDevice_Type type, const ParameterConstRef& Param)
{
   switch(type)
   {
      case TargetDevice_Type::FPGA:
         return area_modelRef(new clb_model(Param));
#if HAVE_CMOS_BUILT
      case TargetDevice_Type::IC:
         return area_modelRef(new cell_model(Param));
#endif
      default:
         THROW_UNREACHABLE("");
   }
   return area_modelRef();
}
