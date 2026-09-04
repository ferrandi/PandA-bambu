/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2015-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
