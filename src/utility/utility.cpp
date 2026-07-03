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
 *              Copyright (C) 2015-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
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
