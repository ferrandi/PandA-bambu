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
 * @file utility.cpp
 * @brief This file collects some utility functions and macros.
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "utility.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>

TimeStamp::TimeStamp() : timestamp("1970-01-01T00:00:00")
{
}

TimeStamp::TimeStamp(const std::string& _timestamp) : timestamp(_timestamp)
{
}

std::string TimeStamp::GetCurrentTimeStamp()
{
   auto now = std::chrono::system_clock::now();
   auto in_time_t = std::chrono::system_clock::to_time_t(now);
#if !defined(__clang__) && __GNUC__ < 5
   char buffer[32];
   strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", std::localtime(&in_time_t));
   return std::string(buffer);
#else
   std::stringstream ss;
   ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%dT%H:%M:%S");
   return ss.str();
#endif
}

std::ostream& operator<<(std::ostream& os, const TimeStamp& t)
{
   os << t.timestamp;
   return os;
}

bool operator<=(const TimeStamp& timestamp1, const TimeStamp& timestamp2)
{
   return timestamp1.timestamp <= timestamp2.timestamp;
}
