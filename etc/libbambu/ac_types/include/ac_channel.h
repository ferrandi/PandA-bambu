/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Datatypes                                          *
 *                                                                        *
 *  Software Version: 4.6                                                 *
 *                                                                        *
 *  Release Date    : Fri Aug 19 11:20:11 PDT 2022                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 4.6.1                                               *
 *                                                                        *
 *  Copyright 2004-2020, Mentor Graphics Corporation,                     *
 *                                                                        *
 *  All Rights Reserved.                                                  *
 *                                                                        *
 **************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
 *                                                                        *
 *      http://www.apache.org/licenses/LICENSE-2.0                        *
 *                                                                        *
 *  Unless required by applicable law or agreed to in writing, software   *
 *  distributed under the License is distributed on an "AS IS" BASIS,     *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or       *
 *  implied.                                                              *
 *  See the License for the specific language governing permissions and   *
 *  limitations under the License.                                        *
 **************************************************************************
 *                                                                        *
 *  This file was modified by PandA team from Politecnico di Milano to    *
 *  generate efficient hardware for the PandA/bambu HLS tool.             *
 *  The API remains the same as defined by Mentor Graphics.               *
 *                                                                        *
 *************************************************************************/

/*
//  Source:         ac_channel.h
//  Description:    templatized channel communication class
//  Original Author:  Andres Takach, Ph.D.
//  Modified by:      Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
//                    Michele Fiorito <michele.fiorito@polimi.it>
*/

#ifndef __AC_CHANNEL_H
#define __AC_CHANNEL_H

#ifndef __cplusplus
#error C++ is required to include this header file
#endif

#include <deque>
#include <fstream>
#include <initializer_list>
#if !defined(__BAMBU__) || defined(__BAMBU_SIM__)
#include <iostream>
#endif
#include <string>

#if !defined(AC_USER_DEFINED_ASSERT) && !defined(AC_ASSERT_THROW_EXCEPTION)
#include <cassert>
#endif

// not directly used by this include
#include <cstdio>
#include <cstdlib>

// Macro Definitions (obsolete - provided here for backward compatibility)
#define AC_CHAN_CTOR(varname) varname
#define AC_CHAN_CTOR_INIT(varname, init) varname(init)
#define AC_CHAN_CTOR_VAL(varname, init, val) varname(init, val)

#ifndef __FORCE_INLINE
#define __FORCE_INLINE __attribute__((always_inline)) inline
#endif

////////////////////////////////////////////////
// Struct: ac_exception / ac_channel_exception
////////////////////////////////////////////////

#ifndef __INCLUDED_AC_EXCEPTION
#define __INCLUDED_AC_EXCEPTION
struct ac_exception
{
   const char* const file;
   const unsigned int line;
   const int code;
   const char* const msg;
   ac_exception(const char* file_, const unsigned int& line_, const int& code_, const char* msg_)
       : file(file_), line(line_), code(code_), msg(msg_)
   {
   }
};
#endif

struct ac_channel_exception
{
   enum
   {
      code_begin = 1024
   };
   enum code
   {
      read_from_empty_channel = code_begin,
      fifo_not_empty_when_reset,
      no_operator_sb_defined_for_channel_type,
      no_insert_defined_for_channel_type,
      no_size_in_connections,
      no_num_free_in_connections,
      no_output_empty_in_connections,
      write_to_full_channel
   };
   static inline const char* msg(const code& code_)
   {
      static const char* const s[] = {"Read from empty channel",
                                      "fifo not empty when reset",
                                      "No operator[] defined for channel type",
                                      "No insert defined for channel type",
                                      "Connections does not support size()",
                                      "Connections does not support num_free()",
                                      "Connections::Out does not support empty()",
                                      "Write to full channel"};
      return s[code_ - code_begin];
   }
};

///////////////////////////////////////////
// Class: ac_channel
//////////////////////////////////////////

template <class T>
class ac_channel
{
 public:
   using element_type = T;

   // constructors
   ac_channel();
#if !defined(__BAMBU__) || defined(__BAMBU_SIM__)
   ac_channel(const ac_channel<T>&) = default;

   ac_channel(int init);
   ac_channel(int init, T val);
   ac_channel(std::initializer_list<T> val);
   ac_channel(const char* bin_file);

   ac_channel& operator=(const ac_channel<T>&) = default;
#endif

   __FORCE_INLINE T read()
   {
      return chan.read();
   }
   __FORCE_INLINE void read(T& t)
   {
      t = read();
   }
   __FORCE_INLINE bool nb_read(T& t)
   {
      return chan.nb_read(t);
   }

   __FORCE_INLINE void write(const T& t)
   {
      chan.write(t);
   }
   __FORCE_INLINE bool nb_write(T& t)
   {
#if !defined(__BAMBU__) || defined(__BAMBU_SIM__)
      chan.incr_size_call_count();
#endif
      return chan.nb_write(t);
   }

   __FORCE_INLINE unsigned int size()
   {
#if !defined(__BAMBU__) || defined(__BAMBU_SIM__)
      chan.incr_size_call_count();
#endif
      return chan.size();
   }

#if !defined(__BAMBU__) || defined(__BAMBU_SIM__)
   __FORCE_INLINE bool empty()
   {
      return chan.empty();
   }

   __FORCE_INLINE unsigned int num_free()
   {
      return chan.num_free();
   }

   // Return true if channel has at least k entries
   __FORCE_INLINE bool available(unsigned int k) const
   {
      return chan.available(k);
   }

   __FORCE_INLINE void reset()
   {
      chan.reset();
   }

   __FORCE_INLINE unsigned int debug_size() const
   {
      return chan.size();
   }

   __FORCE_INLINE const T& operator[](unsigned int pos) const
   {
      return chan[pos];
   }

   __FORCE_INLINE T& operator[](unsigned int pos)
   {
      return chan[pos];
   }

   __FORCE_INLINE int get_size_call_count()
   {
      return chan.get_size_call_count();
   }
#endif

#ifdef SYSTEMC_INCLUDED
   __FORCE_INLINE void bind(sc_core::sc_fifo_in<T>& f)
   {
      chan.bind(f);
   }
   __FORCE_INLINE void bind(sc_core::sc_fifo_out<T>& f)
   {
      chan.bind(f);
   }
#endif

#ifdef __CONNECTIONS__CONNECTIONS_H__
   __FORCE_INLINE void bind(Connections::Out<T>& c)
   {
      chan.bind(c);
   }
   __FORCE_INLINE void bind(Connections::In<T>& c)
   {
      chan.bind(c);
   }
   __FORCE_INLINE void bind(Connections::SyncIn& c)
   {
      chan.bind(c);
   }
   __FORCE_INLINE void bind(Connections::SyncOut& c)
   {
      chan.bind(c);
   }
#endif

 private:
#if !defined(__BAMBU__) || defined(__BAMBU_SIM__)
#ifndef AC_CHANNEL_ASSERT
#define AC_CHANNEL_ASSERT(cond, code) ac_assert(cond, __FILE__, __LINE__, code)
   static inline void ac_assert(bool condition, const char* file, int line, const ac_channel_exception::code& code)
   {
#ifndef AC_USER_DEFINED_ASSERT
      if(!condition)
      {
         const ac_exception e(file, line, code, ac_channel_exception::msg(code));
#ifdef AC_ASSERT_THROW_EXCEPTION
#ifdef AC_ASSERT_THROW_EXCEPTION_AS_CONST_CHAR
         throw(e.msg);
#else
         throw(e);
#endif
#else
         std::cerr << "Assert";
         if(e.file)
         {
            std::cerr << " in file " << e.file << ":" << e.line;
         }
         std::cerr << " " << e.msg << std::endl;
         assert(0);
#endif
      }
#else
      AC_USER_DEFINED_ASSERT(condition, file, line, ac_channel_exception::msg(code));
#endif
   }
#else
#error "private use only - AC_CHANNEL_ASSERT macro already defined"
#endif
#else
#define AC_CHANNEL_ASSERT(cond, code)
#endif

 public:
   class fifo
   {
      enum fifo_type
      {
         fifo_ac_channel_type,
         fifo_sc_fifo_type,
         fifo_connections_type,
         fifo_connections_sync_type
      };

#if defined(__BAMBU__) && !defined(__BAMBU_SIM__)
      const T _read(void);
      const T _read(bool& res);
      bool _write(const T t);
#else
      struct fifo_abstract
      {
         virtual ~fifo_abstract() = default;
         virtual fifo_type get_fifo_type() const = 0;
         virtual T read() = 0;
         virtual bool nb_read(T& t) = 0;
         virtual void write(const T& t) = 0;
         virtual bool nb_write(T& t) = 0;
         virtual bool empty() = 0;
         virtual bool available(unsigned int k) const = 0;
         virtual unsigned int size() const = 0;
         virtual unsigned int num_free() const = 0;
         virtual void reset() = 0;
         virtual const T& operator_sb(const unsigned int& pos, const T& default_value) const = 0;
         virtual T& operator_sb(const unsigned int& pos, const T& default_value) = 0;
      };

      struct fifo_ac_channel : fifo_abstract
      {
         std::deque<T> ch;

         ~fifo_ac_channel(){};

         static inline fifo_type ftype()
         {
            return fifo_ac_channel_type;
         }

         fifo_type get_fifo_type() const
         {
            return ftype();
         }

         T read()
         {
            {
               // If you hit this assert you attempted a read on an empty channel. Perhaps
               // you need to guard the execution of the read with a call to the available()
               // function:
               //    if (myInputChan.available(2)) {
               //      // it is safe to read two values
               //      cout << myInputChan.read();
               //      cout << myInputChan.read();
               //    }
               AC_CHANNEL_ASSERT(!empty(), ac_channel_exception::read_from_empty_channel);
            }
            T t = ch.front();
            ch.pop_front();
            return t;
         }
         bool nb_read(T& t)
         {
            return empty() ? false : (t = read(), true);
         }

         void write(const T& t)
         {
            AC_CHANNEL_ASSERT(num_free(), ac_channel_exception::write_to_full_channel);
            ch.push_back(t);
         }
         bool nb_write(T& t)
         {
            return !num_free() ? false : (write(t), true);
         }

         bool empty()
         {
            return size() == 0;
         }
         bool available(unsigned int k) const
         {
            return size() >= k;
         }
         unsigned int size() const
         {
            return (int)ch.size();
         }
         unsigned int num_free() const
         {
            return ch.max_size() - ch.size();
         }

         void reset()
         {
            ch.clear();
         }

         const T& operator_sb(const unsigned int& pos, const T&) const
         {
            return ch[pos];
         }

         T& operator_sb(const unsigned int& pos, const T&)
         {
            return ch[pos];
         }
      };

#ifdef SYSTEMC_INCLUDED
      struct fifo_sc_fifo : fifo_abstract
      {
         sc_core::sc_fifo_in<T>* fifo_in;
         sc_core::sc_fifo_out<T>* fifo_out;

         ~fifo_sc_fifo()
         {
         }

         static inline fifo_type ftype()
         {
            return fifo_sc_fifo_type;
         }

         fifo_type get_fifo_type() const
         {
            return ftype();
         }

         T read()
         {
            return fifo_in->read();
         }
         bool nb_read(T& t)
         {
            return empty() ? false : (t = read(), true);
         }

         void write(const T& t)
         {
            fifo_out->write(t);
         }
         bool nb_write(T& t)
         {
            return !num_free() ? false : (write(t), true);
         }

         bool empty()
         {
            return size() == 0;
         }
         bool available(unsigned int k) const
         {
            return size() >= k;
         }
         unsigned int size() const
         {
            return fifo_in->num_available();
         }
         unsigned int num_free() const
         {
            return fifo_out->num_free();
         }

         void reset()
         {
            AC_CHANNEL_ASSERT(empty(), ac_channel_exception::fifo_not_empty_when_reset);
         }

         const T& operator_sb(const unsigned int&, const T& default_value) const
         {
            AC_CHANNEL_ASSERT(0, ac_channel_exception::no_operator_sb_defined_for_channel_type);
            return default_value;
         }
      };

    public:
      void bind(sc_core::sc_fifo_in<T>& f)
      {
         get_fifo<fifo_sc_fifo>().fifo_in = &f;
      }
      void bind(sc_core::sc_fifo_out<T>& f)
      {
         get_fifo<fifo_sc_fifo>().fifo_out = &f;
      }

    private:
#endif

#ifdef __CONNECTIONS__CONNECTIONS_H__
      struct fifo_connections : fifo_abstract
      {
         Connections::In<T>* fifo_in;
         Connections::Out<T>* fifo_out;

         ~fifo_connections()
         {
         }
         static inline fifo_type ftype()
         {
            return fifo_connections_type;
         }
         fifo_type get_fifo_type() const
         {
            return ftype();
         }

         T read()
         {
            return fifo_in->Pop();
         }
         bool nb_read(T& t)
         {
            return fifo_in->PopNB(t);
         }

         void write(const T& t)
         {
            fifo_out->Push(t);
         }
         bool nb_write(T& t)
         {
            return fifo_out->PushNB(t);
         }

         bool empty()
         {
            if(fifo_in)
               return fifo_in->Empty();
            else
               AC_CHANNEL_ASSERT(0, ac_channel_exception::no_output_empty_in_connections);
            return false;
         }
         bool available(unsigned int k) const
         {
            return true;
         }
         unsigned int size() const
         {
            AC_CHANNEL_ASSERT(0, ac_channel_exception::no_size_in_connections);
            return 0;
         }
         unsigned int num_free() const
         {
            AC_CHANNEL_ASSERT(0, ac_channel_exception::no_num_free_in_connections);
            return 0;
         }

         void reset()
         {
            AC_CHANNEL_ASSERT(empty(), ac_channel_exception::fifo_not_empty_when_reset);
         }

         const T& operator_sb(const unsigned int&, const T& default_value) const
         {
            AC_CHANNEL_ASSERT(0, ac_channel_exception::no_operator_sb_defined_for_channel_type);
            return default_value;
         }
      };

      struct fifo_connections_sync : fifo_abstract
      {
         Connections::SyncIn* sync_in;
         Connections::SyncOut* sync_out;

         ~fifo_connections_sync()
         {
         }
         static inline fifo_type ftype()
         {
            return fifo_connections_sync_type;
         }
         fifo_type get_fifo_type() const
         {
            return ftype();
         }

         bool read()
         {
            sync_in->sync_in();
            return true;
         }
         bool nb_read(T& t)
         {
            t = true;
            return (sync_in->nb_sync_in());
         }

         void write(const T& t)
         {
            sync_out->sync_out();
         }
         bool nb_write(T& t)
         {
            sync_out->sync_out();
            return true;
         }

         bool empty()
         {
            AC_CHANNEL_ASSERT(0, ac_channel_exception::no_output_empty_in_connections);
            return (false);
         }
         bool available(unsigned int k) const
         {
            return true;
         }
         unsigned int size() const
         {
            AC_CHANNEL_ASSERT(0, ac_channel_exception::no_size_in_connections);
            return 0;
         }
         unsigned int num_free() const
         {
            AC_CHANNEL_ASSERT(0, ac_channel_exception::no_num_free_in_connections);
            return 0;
         }
         void reset()
         {
            if(sync_in)
               sync_in->reset_sync_in();
            if(sync_out)
               sync_out->reset_sync_out();
         }
         const T& operator_sb(const unsigned int&, const T& default_value) const
         {
            AC_CHANNEL_ASSERT(0, ac_channel_exception::no_operator_sb_defined_for_channel_type);
            return default_value;
         }
      };

    public:
      void bind(Connections::In<T>& c)
      {
         get_fifo<fifo_connections>().fifo_in = &c;
      }
      void bind(Connections::Out<T>& c)
      {
         get_fifo<fifo_connections>().fifo_out = &c;
      }

      void bind(Connections::SyncIn& c)
      {
         get_fifo<fifo_connections_sync>().sync_in = &c;
      }
      void bind(Connections::SyncOut& c)
      {
         get_fifo<fifo_connections_sync>().sync_out = &c;
      }

    private:
#endif

      template <typename fifo_T, typename... Args>
      fifo_T& get_fifo()
      {
         if(!f || f->get_fifo_type() != fifo_T::ftype())
         {
            if(f)
            {
               AC_CHANNEL_ASSERT(f->empty(), ac_channel_exception::fifo_not_empty_when_reset);
               delete f;
            }
            f = new fifo_T;
         }
         return static_cast<fifo_T&>(*f);
      }

      fifo_abstract* f;
      unsigned int rSz; // reset size
      T rVal;           // resetValue
      int size_call_count;
#endif

    public:
#if defined(__BAMBU__) && !defined(__BAMBU_SIM__)
      __FORCE_INLINE T read()
      {
         return _read();
      }
      __FORCE_INLINE bool nb_read(T& t)
      {
         bool res;
         T temp = _read(res);
         t = res ? temp : t;
         return res;
      }

      __FORCE_INLINE void write(const T& t)
      {
         _write(t);
      }
      __FORCE_INLINE bool nb_write(const T& t)
      {
         return _write(t);
      }
#else
      fifo() : f(0), rSz(0), size_call_count(0)
      {
         get_fifo<fifo_ac_channel>();
      }
      fifo(int init) : f(0), rSz(init), size_call_count(0)
      {
         get_fifo<fifo_ac_channel>();
      }
      fifo(int init, T val) : f(0), rSz(init), rVal(val), size_call_count(0)
      {
         get_fifo<fifo_ac_channel>();
      }
      fifo(const fifo& other)
      {
         if(!other.f || other.f->get_fifo_type() != fifo_ac_channel::ftype())
         {
            get_fifo<fifo_ac_channel>();
         }
         else
         {
            f = new fifo_ac_channel(*static_cast<fifo_ac_channel*>(other.f));
         }
      }
      ~fifo()
      {
         delete f;
      }

      inline T read()
      {
         return f->read();
      }
      inline bool nb_read(T& t)
      {
         return f->nb_read(t);
      }

      inline void write(const T& t)
      {
         f->write(t);
      }
      inline bool nb_write(T& t)
      {
         return f->nb_write(t);
      }

      inline bool empty()
      {
         return f->empty();
      }
      inline bool available(unsigned int k) const
      {
         return f->available(k);
      }
      inline unsigned int size() const
      {
         return f->size();
      }
      inline unsigned int num_free() const
      {
         return f->num_free();
      }

      inline void reset()
      {
         f->reset();
         for(int i = 0; i < (int)rSz; i++)
         {
            write(rVal);
         }
      }

      inline const T& operator[](unsigned int pos) const
      {
         return f->operator_sb(pos, rVal);
      }

      inline T& operator[](unsigned int pos)
      {
         return f->operator_sb(pos, rVal);
      }

      void incr_size_call_count()
      {
         ++size_call_count;
      }
      int get_size_call_count()
      {
         int tmp = size_call_count;
         size_call_count = 0;
         return tmp;
      }

      // obsolete - provided here for backward compatibility with ac_channel
      struct iterator
      {
         iterator operator+(unsigned int pos_) const
         {
            return iterator(itr, pos_);
         }

       private:
         friend class fifo;
         iterator(const typename std::deque<T>::iterator& itr_, unsigned int pos = 0) : itr(itr_)
         {
            if(pos)
            {
               itr += pos;
            }
         }
         typename std::deque<T>::iterator itr;
      };
      iterator begin()
      {
         AC_CHANNEL_ASSERT(f->get_fifo_type() == fifo_ac_channel_type,
                           ac_channel_exception::no_insert_defined_for_channel_type);
         return iterator(get_fifo<fifo_ac_channel>().ch.begin());
      }
      void insert(iterator itr, const T& t)
      {
         AC_CHANNEL_ASSERT(f->get_fifo_type() == fifo_ac_channel_type,
                           ac_channel_exception::no_insert_defined_for_channel_type);
         get_fifo<fifo_ac_channel>().ch.insert(itr.itr, t);
      }
#endif
   };
   fifo chan;

 private:
#if defined(__BAMBU__) && !defined(__BAMBU_SIM__)
   // Prevent the compiler from autogenerating these.
   //  (This enforces that channels are always passed by reference.)
   ac_channel(const ac_channel<T>&) = delete;
   ac_channel& operator=(const ac_channel<T>&) = delete;
#endif
};

template <class T>
ac_channel<T>::ac_channel() : chan()
{
}

#if !defined(__BAMBU__) || defined(__BAMBU_SIM__)
template <class T>
ac_channel<T>::ac_channel(int init) : chan(init)
{
   for(int i = init; i > 0; i--)
   {
      T dc;
      write(dc);
   }
}

template <class T>
ac_channel<T>::ac_channel(int init, T val) : chan(init, val)
{
   for(int i = init; i > 0; i--)
   {
      write(val);
   }
}

template <class T>
ac_channel<T>::ac_channel(std::initializer_list<T> val) : chan(val.size())
{
   for(auto& v : val)
   {
      write(v);
   }
}

template <class T>
ac_channel<T>::ac_channel(const char* bin_file)
    : chan(std::ifstream(bin_file, std::ifstream::ate | std::ifstream::binary).tellg() / sizeof(T))
{
   std::ifstream init_file(bin_file, std::ifstream::in | std::ifstream::binary);
   T v;
   while(chan.num_free())
   {
      init_file.read((char*)&v, sizeof(T));
      write(v);
   }
}

template <class T>
__FORCE_INLINE std::ostream& operator<<(std::ostream& os, ac_channel<T>& a)
{
   for(unsigned int i = 0; i < a.size(); i++)
   {
      if(i > 0)
      {
         os << " ";
      }
      os << a[i];
   }
   return os;
}
#endif

// This general case is meant to cover non channel (or array of them) args
//   Its result will be ignored
template <typename T>
bool nb_read_chan_rdy(T& x)
{
   return true;
}

template <typename T>
bool nb_read_chan_rdy(ac_channel<T>& chan)
{
   return !chan.empty();
}

template <typename T, int N>
bool nb_read_chan_rdy(ac_channel<T> (&chan)[N])
{
   bool r = true;
   for(int i = 0; i < N; i++)
   {
      r &= !chan[i].empty();
   }
   return r;
}

#if __cplusplus > 199711L
template <typename... Args>
bool nb_read_chan_rdy(Args&... args)
{
   const int n_args = sizeof...(args);
   // only every other arg is a channel (or an array of channels)
   bool rdy[n_args] = {(nb_read_chan_rdy(args))...};
   bool r = true;
   for(int i = 0; i < n_args; i += 2)
   {
      r &= rdy[i];
   }
   return r;
}
#endif

template <typename T>
void nb_read_r(ac_channel<T>& chan, T& var)
{
   chan.nb_read(var);
}

template <typename T, int N>
void nb_read_r(ac_channel<T> (&chan)[N], T (&var)[N])
{
   for(int i = 0; i < N; i++)
   {
      chan[i].nb_read(var[i]);
   }
}

#if __cplusplus > 199711L
template <typename T, typename... Args>
void nb_read_r(ac_channel<T>& chan, T& var, Args&... args)
{
   chan.nb_read(var);
   nb_read_r(args...);
}

template <typename T, int N, typename... Args>
void nb_read_r(ac_channel<T> (&chan)[N], T (&var)[N], Args&... args)
{
   for(int i = 0; i < N; i++)
   {
      chan[i].nb_read(var[i]);
   }
   nb_read_r(args...);
}

template <typename... Args>
bool nb_read_join(Args&... args)
{
   if(nb_read_chan_rdy(args...))
   {
      nb_read_r(args...);
      return true;
   }
   return false;
}
#endif

/* undo macro adjustments */
#ifdef AC_CHANNEL_ASSERT
#undef AC_CHANNEL_ASSERT
#endif

#endif
