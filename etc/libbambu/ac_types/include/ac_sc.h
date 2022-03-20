/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Datatypes                                          *
 *                                                                        *
 *  Software Version: 3.7                                                 *
 *                                                                        *
 *  Release Date    : Tue May 30 14:25:58 PDT 2017                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 3.7.2                                               *
 *                                                                        *
 *  Copyright 2004-2016, Mentor Graphics Corporation,                     *
 *                                                                        *
 *  All Rights Reserved.                                                  *
 *
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

#ifndef __AC_SC_H
#define __AC_SC_H

#ifndef __cplusplus
#error C++ is required to include this header file
#endif

#if !defined(IEEE_1666_SYSTEMC) && !defined(SYSTEMC_VERSION) && !defined(SC_API_VERSION_STRING)
#error SystemC header file needs to be included before the ac_sc is included
#endif

#include <ac_complex.h>

#ifdef __AC_NAMESPACE
namespace __AC_NAMESPACE
{
#endif

   // Explicit conversion functions from ac to sc and viceversa
   template <int W>
   ac_int<W, true> to_ac(const sc_dt::sc_bigint<W>& val)
   {
      enum
      {
         N = (W + 31) / 32
      };
      sc_dt::sc_bigint<N* 32> v = val;
      ac_int<N * 32, true> r = 0;
#ifdef __BAMBU__
#pragma UNROLL y
#endif
      for(int i = 0; i < N; i++)
      {
         r.set_slc(i * 32, ac_int<32, true>(v.to_int()));
         v >>= 32;
      }
      return ac_int<W, true>(r);
   }

   template <int W>
   ac_int<W, false> to_ac(const sc_dt::sc_biguint<W>& val)
   {
      enum
      {
         N = (W + 31) / 32
      };
      sc_dt::sc_biguint<N* 32> v = val;
      ac_int<N * 32, true> r = 0;
#ifdef __BAMBU__
#pragma UNROLL y
#endif
      for(int i = 0; i < N; i++)
      {
         r.set_slc(i * 32, ac_int<32, true>(v.to_int()));
         v >>= 32;
      }
      return ac_int<W, false>(r);
   }

   template <int W>
   sc_dt::sc_bigint<W> to_sc(const ac_int<W, true>& val)
   {
      enum
      {
         N = (W + 31) / 32
      };
      ac_int<N * 32, true> v = val;
      sc_dt::sc_bigint<N * 32> r;
#ifdef __BAMBU__
#pragma UNROLL y
#endif
      for(int i = N - 1; i >= 0; i--)
      {
         r <<= 32;
         r.range(31, 0) = (v.template slc<32>(i * 32)).to_int();
      }
      return sc_dt::sc_bigint<W>(r);
   }

   template <int W>
   sc_dt::sc_biguint<W> to_sc(const ac_int<W, false>& val)
   {
      enum
      {
         N = (W + 31) / 32
      };
      ac_int<N * 32, true> v = val;
      sc_dt::sc_biguint<N * 32> r;
#ifdef __BAMBU__
#pragma UNROLL y
#endif
      for(int i = N - 1; i >= 0; i--)
      {
         r <<= 32;
         r.range(31, 0) = (v.template slc<32>(i * 32)).to_int();
      }
      return sc_dt::sc_biguint<W>(r);
   }

#ifdef SC_INCLUDE_FX
   template <int W, int I, sc_dt::sc_q_mode Q, sc_dt::sc_o_mode O, int nbits>
   ac_fixed<W, I, true> to_ac(const sc_dt::sc_fixed<W, I, Q, O, nbits>& val)
   {
      ac_fixed<W, I, true> r = 0;
      sc_dt::sc_fixed<W, W> fv;
      fv.range(W - 1, 0) = val.range(W - 1, 0);
      sc_dt::sc_bigint<W> v(fv);
      r.set_slc(0, to_ac(v));
      return r;
   }

   template <int W, int I, sc_dt::sc_q_mode Q, sc_dt::sc_o_mode O, int nbits>
   ac_fixed<W, I, false> to_ac(const sc_dt::sc_ufixed<W, I, Q, O, nbits>& val)
   {
      ac_fixed<W, I, false> r = 0;
      sc_dt::sc_ufixed<W, W> fv;
      fv.range(W - 1, 0) = val.range(W - 1, 0);
      sc_dt::sc_biguint<W> v(fv);
      r.set_slc(0, to_ac(v));
      return r;
   }

   template <int W, int I, ac_q_mode Q, ac_o_mode O>
   sc_dt::sc_fixed<W, I> to_sc(const ac_fixed<W, I, true, Q, O>& val)
   {
      ac_int<W, true> v = val.template slc<W>(0);
      sc_dt::sc_bigint<W> i = to_sc(v);
      sc_dt::sc_fixed<W, W> f(i);
      sc_dt::sc_fixed<W, I> r;
      r.range(W - 1, 0) = f.range(W - 1, 0);
      return r;
   }

   template <int W, int I, ac_q_mode Q, ac_o_mode O>
   sc_dt::sc_ufixed<W, I> to_sc(const ac_fixed<W, I, false, Q, O>& val)
   {
      ac_int<W, false> v = val.template slc<W>(0);
      sc_dt::sc_biguint<W> i = to_sc(v);
      sc_dt::sc_ufixed<W, W> f(i);
      sc_dt::sc_ufixed<W, I> r;
      r.range(W - 1, 0) = f.range(W - 1, 0);
      return r;
   }
#endif

   // Utility global functions for initialization

   template <ac_special_val V, int W>
   inline sc_dt::sc_int<W> value(sc_dt::sc_int<W>)
   {
      sc_dt::sc_int<W> r;
      if(V == AC_VAL_DC)
      {
         int t;
         r = t;
      }
      else if(V == AC_VAL_0 || V == AC_VAL_MIN || V == AC_VAL_QUANTUM)
      {
         r = 0;
         if(V == AC_VAL_MIN)
            r[W - 1] = 1;
         else if(V == AC_VAL_QUANTUM)
            r[0] = 1;
      }
      else if(AC_VAL_MAX)
      {
         r = -1;
         r[W - 1] = 0;
      }
      return r;
   }

   template <ac_special_val V, int W>
   inline sc_dt::sc_uint<W> value(sc_dt::sc_uint<W>)
   {
      sc_dt::sc_uint<W> r;
      if(V == AC_VAL_DC)
      {
         int t;
         r = t;
      }
      else if(V == AC_VAL_0 || V == AC_VAL_MIN || V == AC_VAL_QUANTUM)
      {
         r = 0;
         if(V == AC_VAL_QUANTUM)
            r[0] = 1;
      }
      else if(AC_VAL_MAX)
         r = -1;
      return r;
   }

   template <ac_special_val V, int W>
   inline sc_dt::sc_bigint<W> value(sc_dt::sc_bigint<W>)
   {
      sc_dt::sc_bigint<W> r;
      if(V == AC_VAL_DC)
      {
         int t;
         r = t;
      }
      else if(V == AC_VAL_0 || V == AC_VAL_MIN || V == AC_VAL_QUANTUM)
      {
         r = 0;
         if(V == AC_VAL_MIN)
            r[W - 1] = 1;
         else if(V == AC_VAL_QUANTUM)
            r[0] = 1;
      }
      else if(AC_VAL_MAX)
      {
         r = -1;
         r[W - 1] = 0;
      }
      return r;
   }

   template <ac_special_val V, int W>
   inline sc_dt::sc_biguint<W> value(sc_dt::sc_biguint<W>)
   {
      sc_dt::sc_biguint<W> r;
      if(V == AC_VAL_DC)
      {
         int t;
         r = t;
      }
      else if(V == AC_VAL_0 || V == AC_VAL_MIN || V == AC_VAL_QUANTUM)
      {
         r = 0;
         if(V == AC_VAL_QUANTUM)
            r[0] = 1;
      }
      else if(AC_VAL_MAX)
         r = -1;
      return r;
   }

#ifdef SC_INCLUDE_FX
   template <ac_special_val V, int W, int I, sc_dt::sc_q_mode Q, sc_dt::sc_o_mode O, int nbits>
   inline sc_dt::sc_fixed<W, I, Q, O, nbits> value(sc_dt::sc_fixed<W, I, Q, O, nbits>)
   {
      sc_dt::sc_fixed<W, I> r;
      if(V == AC_VAL_DC)
      {
         int t;
         r = t;
      }
      else if(V == AC_VAL_0 || V == AC_VAL_MIN || V == AC_VAL_QUANTUM)
      {
         r = 0;
         if(V == AC_VAL_MIN)
            r[W - 1] = 1;
         else if(V == AC_VAL_QUANTUM)
            r[0] = 1;
      }
      else if(AC_VAL_MAX)
      {
         r = ~(sc_dt::sc_fixed<W, I>)0;
         r[W - 1] = 0;
      }
      return r;
   }

   template <ac_special_val V, int W, int I, sc_dt::sc_q_mode Q, sc_dt::sc_o_mode O, int nbits>
   inline sc_dt::sc_ufixed<W, I, Q, O, nbits> value(sc_dt::sc_ufixed<W, I, Q, O, nbits>)
   {
      sc_dt::sc_ufixed<W, I> r;
      if(V == AC_VAL_DC)
      {
         int t;
         r = t;
      }
      else if(V == AC_VAL_0 || V == AC_VAL_MIN || V == AC_VAL_QUANTUM)
      {
         r = 0;
         if(V == AC_VAL_QUANTUM)
            r[0] = 1;
      }
      else if(AC_VAL_MAX)
         r = ~(sc_dt::sc_ufixed<W, I>)0;
      return r;
   }
#endif

   namespace ac
   {
      // PUBLIC FUNCTIONS
      // function to initialize (or uninitialize) arrays
      template <ac_special_val V, int W>
      inline bool init_array(sc_dt::sc_int<W>* a, int n)
      {
         sc_dt::sc_int<W> t = value<V>(*a);
         for(int i = 0; i < n; i++)
            a[i] = t;
         return true;
      }
      template <ac_special_val V, int W>
      inline bool init_array(sc_dt::sc_uint<W>* a, int n)
      {
         sc_dt::sc_uint<W> t = value<V>(*a);
         for(int i = 0; i < n; i++)
            a[i] = t;
         return true;
      }
      template <ac_special_val V, int W>
      inline bool init_array(sc_dt::sc_bigint<W>* a, int n)
      {
         sc_dt::sc_bigint<W> t = value<V>(*a);
         for(int i = 0; i < n; i++)
            a[i] = t;
         return true;
      }
      template <ac_special_val V, int W>
      inline bool init_array(sc_dt::sc_biguint<W>* a, int n)
      {
         sc_dt::sc_biguint<W> t = value<V>(*a);
         for(int i = 0; i < n; i++)
            a[i] = t;
         return true;
      }
#ifdef SC_INCLUDE_FX
      template <ac_special_val V, int W, int I, sc_dt::sc_q_mode Q, sc_dt::sc_o_mode O, int nbits>
      inline bool init_array(sc_dt::sc_fixed<W, I, Q, O, nbits>* a, int n)
      {
         sc_dt::sc_fixed<W, I> t = value<V>(*a);
         for(int i = 0; i < n; i++)
            a[i] = t;
         return true;
      }
      template <ac_special_val V, int W, int I, sc_dt::sc_q_mode Q, sc_dt::sc_o_mode O, int nbits>
      inline bool init_array(sc_dt::sc_ufixed<W, I, Q, O, nbits>* a, int n)
      {
         sc_dt::sc_ufixed<W, I> t = value<V>(*a);
         for(int i = 0; i < n; i++)
            a[i] = t;
         return true;
      }
#endif
   } // namespace ac

#ifdef __AC_NAMESPACE
}
#endif

// TRACE FUNCTIONS

// SystemC Versions - 2.2.0 20070314
//                    2.3.0 20120701
//                    2.3.1 20140417

#if(SYSTEMC_VERSION <= 20140417) && !defined(NCSC)

#if(SYSTEMC_VERSION > 20120701)
#include <sysc/tracing/sc_vcd_trace.h>
#endif

//==============================================================================
// The following block of code is copied from the file sc_vcd_trace.cpp in the
// SystemC 2.2.0 distribution. This code should have been placed in the file
// sc_vcd_trace.h to allow proper C++ derivation.
namespace sc_core
{
   class vcd_trace
   {
    public:
      vcd_trace(const std::string& name_, const std::string& vcd_name_);

      // Needs to be pure virtual as has to be defined by the particular
      // type being traced
      virtual void write(FILE* f) = 0;

      virtual void set_width();

      static const char* strip_leading_bits(const char* originalbuf);

      // Comparison function needs to be pure virtual too
      virtual bool changed() = 0;

      // Make this virtual as some derived classes may overwrite
      virtual void print_variable_declaration_line(FILE* f);

      void compose_data_line(char* rawdata, char* compdata);

#if(SYSTEMC_VERSION > 20120701)
      std::string compose_line(const std::string& data);
#else
      std::string compose_line(const std::string data);
#endif

      virtual ~vcd_trace();

      const std::string name;
      const std::string vcd_name;
      const char* vcd_var_typ_name;
      int bit_width;
   };
} // namespace sc_core
static void remove_vcd_name_problems(std::string& name)
{
   using namespace sc_core;
   char message[4000];
   static bool warned = false;

   bool braces_removed = false;
   for(unsigned int i = 0; i < name.length(); i++)
   {
      if(name[i] == '[')
      {
         name[i] = '(';
         braces_removed = true;
      }
      else if(name[i] == ']')
      {
         name[i] = ')';
         braces_removed = true;
      }
   }

   if(braces_removed && !warned)
   {
      std::sprintf(message, "Traced objects found with name containing [], which may be\n"
                            "interpreted by the waveform viewer in unexpected ways.\n"
                            "So the [] is automatically replaced by ().");
#if(SYSTEMC_VERSION <= 20120701)
      put_error_message(message, true);
#endif
      warned = true;
   }
}
//==============================================================================
#endif

#ifdef __AC_NAMESPACE
namespace __AC_NAMESPACE
{
#endif

   namespace ac_tracing
   {
      //==============================================================================
      // TRACING SUPPORT FOR AC_INT
      template <int W, bool S>
      class vcd_ac_int_trace : public sc_core::vcd_trace
      {
       public:
         vcd_ac_int_trace(const ac_int<W, S>& object_, const std::string& name_, const std::string& vcd_name_)
             : vcd_trace(name_, vcd_name_), object(object_)
         {
            vcd_var_typ_name = "wire"; // SystemC does not expose vcd_types[] in sc_vcd_trace.h
            bit_width = W;             // bit_width defined in base class 'vcd_trace'
         }

         virtual void print_variable_declaration_line(FILE* f)
         {
            char buf[2000];
            std::string namecopy = name;
#if !defined(NCSC)
            remove_vcd_name_problems(namecopy);
#endif
            std::sprintf(buf, "$var %s  % 3d  %s  %s [%d:0]  $end\n", vcd_var_typ_name, bit_width, vcd_name.c_str(),
                         namecopy.c_str(), bit_width - 1);
            std::fputs(buf, f);
         }

         virtual void write(FILE* f)
         {
            // The function to_string(AC_BIN) returns a string with the zero-radix
            // prefix (i.e. "0b"). Strip that prefix off because compose_line will add
            // its own.
            std::fprintf(f, "%s", compose_line(((ac_int<W, false>)object).to_string(AC_BIN, true).substr(3)).c_str());
            old_value = object;
         }

         virtual void set_width()
         {
            bit_width = W;
         }

         // Comparison function needs to be pure virtual too
         virtual bool changed()
         {
            return !(object == old_value);
         }

         virtual ~vcd_ac_int_trace()
         {
         }

       protected:
         const ac_int<W, S>& object;
         ac_int<W, S> old_value;
      };

      template <int W, bool S>
      inline void sc_trace(sc_core::sc_trace_file* tf, const ac_int<W, S>& a, const std::string& name)
      {
         using namespace sc_core;
         if(tf)
         {
            //--- SystemC 2.2.0 deficiency. The 'initialized' class member of
            // sc_trace_file is
            // declared as 'protected' and does not have a public access
            // 'get_initialized()' method. Therefore, we cannot check for initialized so
            // the following code is commented out.
            // if( tf->initialized ) {
            //    put_error_message(
            //   "No traces can be added once simulation has started.\n"
            //        "To add traces, create a new vcd trace file.", false );
            //}
            vcd_trace* t = (vcd_trace*)new vcd_ac_int_trace<W, S>(a, name, ((vcd_trace_file*)tf)->obtain_name());
            ((vcd_trace_file*)tf)->traces.push_back(t);
         }
      }
      //==============================================================================

#if !defined(__AC_FIXED_MTI_H)
// The ac_fixed.h shipped with ModelSim/QuestaSim has a stub for sc_trace() for
// ac_fixed so this code is not used. The stub should be removed in a future
// release of the simulator.
#if defined(__AC_FIXED_H) && !defined(SC_TRACE_AC_FIXED)
#define SC_TRACE_AC_FIXED

      //==============================================================================
      // TRACING SUPPORT FOR AC_FIXED
      template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
      class vcd_ac_fixed_trace : public sc_core::vcd_trace
      {
       public:
         vcd_ac_fixed_trace(const ac_fixed<W, I, S, Q, O>& object_, const std::string& name_,
                            const std::string& vcd_name_)
             : vcd_trace(name_, vcd_name_), object(object_)
         {
            vcd_var_typ_name = "wire"; // SystemC does not expose vcd_types[] in sc_vcd_trace.h
            bit_width = W;             // bit_width defined in base class 'vcd_trace'
         }

         virtual void print_variable_declaration_line(FILE* f)
         {
            char buf[2000];
            std::string namecopy = name;
#if !defined(NCSC)
            remove_vcd_name_problems(namecopy);
#endif
            std::sprintf(buf, "$var %s  % 3d  %s  %s [%d:0]  $end\n", vcd_var_typ_name, bit_width, vcd_name.c_str(),
                         namecopy.c_str(), bit_width - 1);
            std::fputs(buf, f);
         }

         virtual void write(FILE* f)
         {
            // The function to_string(AC_BIN) returns a string with the zero-radix
            // prefix (i.e. "0b"). Strip that prefix off because compose_line will add
            // its own.
            std::fprintf(f, "%s",
                         compose_line(((ac_fixed<W, I, false>)object).to_string(AC_BIN, true).substr(3)).c_str());
            old_value = object;
         }

         virtual void set_width()
         {
            bit_width = W;
         }

         // Comparison function needs to be pure virtual too
         virtual bool changed()
         {
            return !(object == old_value);
         }

         virtual ~vcd_ac_fixed_trace()
         {
         }

       protected:
         const ac_fixed<W, I, S, Q, O>& object;
         ac_fixed<W, I, S, Q, O> old_value;
      };

      template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
      inline void sc_trace(sc_core::sc_trace_file* tf, const ac_fixed<W, I, S, Q, O>& a, const std::string& name)
      {
         using namespace sc_core;
         if(tf)
         {
            //--- SystemC 2.2.0 deficiency. The 'initialized' class member of
            // sc_trace_file is
            // declared as 'protected' and does not have a public access
            // 'get_initialized()' method. Therefore, we cannot check for initialized so
            // the following code is commented out.
            // if( tf->initialized ) {
            //    put_error_message(
            //   "No traces can be added once simulation has started.\n"
            //        "To add traces, create a new vcd trace file.", false );
            //}
            vcd_trace* t =
                (vcd_trace*)new vcd_ac_fixed_trace<W, I, S, Q, O>(a, name, ((vcd_trace_file*)tf)->obtain_name());
            ((vcd_trace_file*)tf)->traces.push_back(t);
         }
      }
//==============================================================================
#endif
#endif

#if defined(__AC_FLOAT_H) && !defined(SC_TRACE_AC_FLOAT)
#define SC_TRACE_AC_FLOAT

      //==============================================================================
      // TRACING SUPPORT FOR AC_FLOAT
      template <int W, int I, int E, ac_q_mode Q>
      class vcd_ac_float_trace : public sc_core::vcd_trace
      {
       public:
         vcd_ac_float_trace(const ac_float<W, I, E, Q>& object_, const std::string& name_, const std::string& vcd_name_)
             : vcd_trace(name_, vcd_name_), object(object_)
         {
            vcd_var_typ_name = "wire"; // SystemC does not expose vcd_types[] in sc_vcd_trace.h
            bit_width = W;             // bit_width defined in base class 'vcd_trace'
         }

         virtual void print_variable_declaration_line(FILE* f)
         {
            char buf[2000];
            std::string namecopy = name;
#if !defined(NCSC)
            remove_vcd_name_problems(namecopy);
#endif
            std::sprintf(buf, "$var %s  % 3d  %s  %s [%d:0]  $end\n", vcd_var_typ_name, bit_width, vcd_name.c_str(),
                         namecopy.c_str(), bit_width - 1);
            std::fputs(buf, f);
         }

         virtual void write(FILE* f)
         {
            // The function to_string(AC_BIN) returns a string with the zero-radix
            // prefix (i.e. "0b"). Strip that prefix off because compose_line will add
            // its own.
            std::fprintf(f, "%s", compose_line(object.to_string(AC_BIN).substr(2)).c_str());
            old_value = object;
         }

         virtual void set_width()
         {
            bit_width = W;
         }

         // Comparison function needs to be pure virtual too
         virtual bool changed()
         {
            return !(object == old_value);
         }

         virtual ~vcd_ac_float_trace()
         {
         }

       protected:
         const ac_float<W, I, E, Q>& object;
         ac_float<W, I, E, Q> old_value;
      };

      template <int W, int I, int E, ac_q_mode Q>
      inline void sc_trace(sc_core::sc_trace_file* tf, const ac_float<W, I, E, Q>& a, const std::string& name)
      {
         using namespace sc_core;
         if(tf)
         {
            //--- SystemC 2.2.0 deficiency. The 'initialized' class member of
            // sc_trace_file is
            // declared as 'protected' and does not have a public access
            // 'get_initialized()' method. Therefore, we cannot check for initialized so
            // the following code is commented out.
            // if( tf->initialized ) {
            //    put_error_message(
            //   "No traces can be added once simulation has started.\n"
            //        "To add traces, create a new vcd trace file.", false );
            //}
            vcd_trace* t =
                (vcd_trace*)new vcd_ac_float_trace<W, I, E, Q>(a, name, ((vcd_trace_file*)tf)->obtain_name());
            ((vcd_trace_file*)tf)->traces.push_back(t);
         }
      }
//==============================================================================
#endif

#if defined(__AC_COMPLEX_H) && !defined(SC_TRACE_AC_COMPLEX)
#define SC_TRACE_AC_COMPLEX
      template <typename T>
      inline void sc_trace(sc_core::sc_trace_file* tf, const ac_complex<T>& a, const std::string& name)
      {
         sc_trace(tf, a.real(), name + ".r");
         sc_trace(tf, a.imag(), name + ".i");
      }
#endif

   } // namespace ac_tracing

#ifdef __AC_NAMESPACE
}
#endif

namespace sc_core
{
#ifdef __AC_NAMESPACE
   using __AC_NAMESPACE::ac_tracing::sc_trace;
#else
   using ac_tracing::sc_trace;
#endif
} // namespace sc_core

#endif
