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
 *              Copyright (C) 2004-2026 Politecnico di Milano
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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 */

#ifndef VCD_DATA_HPP
#define VCD_DATA_HPP

#include <limits>
#include <string>

/**
 * This class models a single variation of a signal in vcd
 */
struct sig_variation
{
   /**
    * time stamp of the variation
    */
   unsigned long long time_stamp;

   /**
    * new value of the signal. it's a string because it can have non-binary
    * values like X or Z
    */
   std::string value;

   /**
    * time duration of the value. it's the difference between the time_stamp of
    * the next variation (if any) and the current
    */
   unsigned long long duration;

   sig_variation(unsigned long long ts = 0, std::string val = "",
                 unsigned long long d = std::numeric_limits<decltype(sig_variation::duration)>::max());
};

bool operator<(const sig_variation& w, const sig_variation& v);
bool operator<=(const sig_variation& w, const sig_variation& v);
bool operator>(const sig_variation& w, const sig_variation& v);
bool operator>=(const sig_variation& w, const sig_variation& v);
bool operator==(const sig_variation& w, const sig_variation& v);
bool operator!=(const sig_variation& w, const sig_variation& v);

bool operator<(const unsigned long long t, const sig_variation& v);
bool operator<=(const unsigned long long t, const sig_variation& v);
bool operator>(const unsigned long long t, const sig_variation& v);
bool operator>=(const unsigned long long t, const sig_variation& v);
bool operator==(const unsigned long long t, const sig_variation& v);
bool operator!=(const unsigned long long t, const sig_variation& v);

bool operator<(const sig_variation& w, const unsigned long long t);
bool operator<=(const sig_variation& w, const unsigned long long t);
bool operator>(const sig_variation& w, const unsigned long long t);
bool operator>=(const sig_variation& w, const unsigned long long t);
bool operator==(const sig_variation& w, const unsigned long long t);
bool operator!=(const sig_variation& w, const unsigned long long t);

#endif
