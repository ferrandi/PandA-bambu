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
 *              Copyright (c) 2015-2017 Politecnico di Milano
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
 * @file utility.cpp
 * @brief This file collects some utility functions and macros.
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
*/

///Header include
#include "utility.hpp"

TimeStamp::TimeStamp() :
   boost::posix_time::ptime(boost::date_time::parse_delimited_time<boost::posix_time::ptime>("1970-01-01T00:00:00", 'T'))
{}

TimeStamp::TimeStamp(boost::posix_time::ptime timestamp) :
   boost::posix_time::ptime(timestamp)
{}

TimeStamp::TimeStamp(const std::string timestamp) :
   boost::posix_time::ptime(boost::date_time::parse_delimited_time<boost::posix_time::ptime>(timestamp, 'T'))
{}

TimeStamp TimeStamp::GetCurrentTimeStamp()
{
   return boost::posix_time::second_clock::local_time();
}

std::ostream & operator<<(std::ostream & os, const TimeStamp & timestamp)
{
   if(timestamp.is_special())
   {
      os << "Unknown";
   }
   else
   {
      os << boost::posix_time::to_iso_extended_string(timestamp);
   }
   return os;
}
