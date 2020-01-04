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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
 * @file aadl_information.cpp
 * @brief The information collected from aadl file
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "aadl_information.hpp"

/// boost include
#include "exceptions.hpp"
#include <boost/algorithm/string/replace.hpp>

AadlInformation::AadlParameter::AadlParameter() : endianess(NATIVE_ENDIANESS), direction(INOUT), num_registers(0), bambu_address(0), pointer(false)
{
}

AadlInformation::AadlParameter::EndianessType AadlInformation::AadlParameter::Endianess(const std::string& endianess_string)
{
   if(endianess_string == "UPER")
   {
      return BIG_ENDIANESS;
   }
   if(endianess_string == "LOWER")
   {
      return LITTLE_ENDIANESS;
   }
   if(endianess_string == "NATIVE")
   {
      return NATIVE_ENDIANESS;
   }
   THROW_UNREACHABLE(endianess_string);
   return BIG_ENDIANESS;
}

AadlInformation::AadlParameter::Direction AadlInformation::AadlParameter::GetDirection(const std::string& direction_string)
{
   if(direction_string == "IN")
   {
      return IN;
   }
   if(direction_string == "OUT")
   {
      return OUT;
   }
   if(direction_string == "IN OUT")
   {
      return INOUT;
   }
   THROW_UNREACHABLE(direction_string);
   return IN;
}

std::string AadlInformation::Normalize(const std::string& name) const
{
   return boost::replace_all_copy(name, "-", "_");
}

void AadlInformation::AddAsnType(const std::string& name, const AsnTypeRef& asn_type)
{
   asn_types[Normalize(name)] = asn_type;
}

AsnTypeRef AadlInformation::CGetAsnType(const std::string& name) const
{
   const auto normalized = Normalize(name);
   THROW_ASSERT(asn_types.find(normalized) != asn_types.end(), name);
   return asn_types.find(normalized)->second;
}
