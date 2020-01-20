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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 */

#include "sig_variation.hpp"

#include <utility>

sig_variation::sig_variation(unsigned long long ts, std::string val, unsigned long long d) : time_stamp(ts), value(std::move(val)), duration(d)
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
