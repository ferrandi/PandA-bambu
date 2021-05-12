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
 *              Copyright (C) 2020-2020 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
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
 * @file hls_stream.h
 * @brief Implementation of hls::stream object.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#ifndef __HLS_STREAM_H
#define __HLS_STREAM_H

#ifndef __BAMBU__
#include <algorithm>
#include <deque>
#endif

#define __FORCE_INLINE __attribute__((always_inline)) inline

namespace hls {

template<typename T>
class stream
{
 public:
   /// Constructors
   __FORCE_INLINE stream() {
    }

   __FORCE_INLINE explicit stream(const char* name) {
   }

   /// Make copy constructor and assignment operator private
 private:
    __FORCE_INLINE stream(const stream< T >& chn):V(chn.V) {
   }

   __FORCE_INLINE stream& operator= (const stream< T >& chn) {
      V = chn.V;
      return *this;
   }

 public:
   /// Overload >> and << operators to implement read() and write()
   __FORCE_INLINE void operator >> (T& rdata) {
      read(rdata);
   }

   __FORCE_INLINE void operator << (const T& wdata) {
      write(wdata);
   }

   /// Public API
 public:
   /// Empty & Full
   __FORCE_INLINE bool empty() const {
#ifndef __BAMBU__
      return V.empty();
#else
      return __bambu_fifo_empty(&V);
#endif
   }

   __FORCE_INLINE bool full() const {
#ifndef __BAMBU__
      return false;
#else
      return __bambu_fifo_full(&V);
#endif
   }

   /// Blocking read
   __FORCE_INLINE void read(T& dout) {
#ifndef __BAMBU__
#else
      dout = __bambu_fifo_read(&V);
#endif
   }

   __FORCE_INLINE T read() {
      T tmp;
      read(tmp);
      return tmp;
   }

   /// Nonblocking read
   __FORCE_INLINE bool read_nb(T& dout) {
      bool has_value = !empty(&V);
      if (has_value)
         dout = read();
      return has_value;
   }

   /// Blocking write
   __FORCE_INLINE void write(const T& din) {
#ifndef __BAMBU__
      V.push_back(din);
#else
      __bambu_fifo_write(&V, din);
#endif
   }

   /// Nonblocking write
   __FORCE_INLINE bool write_nb(const T& din) {
      bool is_not_full = !full(&V);
      if (is_not_full)
         write(din);
      return is_not_full;
   }

    public:
#ifndef __BAMBU__
      std::deque<T> V;
#else
      T V;
#endif


};

} // namespace hls
#endif

