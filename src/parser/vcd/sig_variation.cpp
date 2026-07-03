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

#include "sig_variation.hpp"

#include <utility>

sig_variation::sig_variation(unsigned long long ts, std::string val, unsigned long long d)
    : time_stamp(ts), value(std::move(val)), duration(d)
{
}

bool operator<(const sig_variation& w, const unsigned long long t)
{
   return w.time_stamp < t;
}

bool operator<=(const sig_variation& w, const unsigned long long t)
{
   return w.time_stamp <= t;
}

bool operator>(const sig_variation& w, const unsigned long long t)
{
   return w.time_stamp > t;
}

bool operator>=(const sig_variation& w, const unsigned long long t)
{
   return w.time_stamp >= t;
}

bool operator==(const sig_variation& w, const unsigned long long t)
{
   return w.time_stamp == t;
}

bool operator!=(const sig_variation& w, const unsigned long long t)
{
   return w.time_stamp != t;
}

bool operator<(const unsigned long long t, const sig_variation& v)
{
   return t < v.time_stamp;
}

bool operator<=(const unsigned long long t, const sig_variation& v)
{
   return t <= v.time_stamp;
}

bool operator>(const unsigned long long t, const sig_variation& v)
{
   return t > v.time_stamp;
}

bool operator>=(const unsigned long long t, const sig_variation& v)
{
   return t >= v.time_stamp;
}

bool operator==(const unsigned long long t, const sig_variation& v)
{
   return t == v.time_stamp;
}

bool operator!=(const unsigned long long t, const sig_variation& v)
{
   return t != v.time_stamp;
}

bool operator<(const sig_variation& w, const sig_variation& v)
{
   return w.time_stamp < v.time_stamp;
}

bool operator<=(const sig_variation& w, const sig_variation& v)
{
   return w.time_stamp <= v.time_stamp;
}

bool operator>(const sig_variation& w, const sig_variation& v)
{
   return w.time_stamp > v.time_stamp;
}

bool operator>=(const sig_variation& w, const sig_variation& v)
{
   return w.time_stamp >= v.time_stamp;
}

bool operator==(const sig_variation& w, const sig_variation& v)
{
   return w.time_stamp == v.time_stamp;
}

bool operator!=(const sig_variation& w, const sig_variation& v)
{
   return w.time_stamp != v.time_stamp;
}
