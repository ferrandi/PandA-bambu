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
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
