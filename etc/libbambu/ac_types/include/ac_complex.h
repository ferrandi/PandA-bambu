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
 *  Copyright 2008-2016, Mentor Graphics Corporation,                     *
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

/*
//  Source:          ac_complex.h
//  Description:     complex type with parameterized type that can be:
//                     - C integer types
//                     - C floating point types
//                     - ac_int
//                     - ac_fixed
//                     - ac_float
//                   ac_complex based on C integers, ac_int, ac_fixed and
ac_float can
//                   be mixed
//  Original Author: Andres Takach, Ph.D.
//  Modified by:     Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
*/

#ifndef __AC_COMPLEX_H
#define __AC_COMPLEX_H

#include <ac_float.h>

#ifdef __AC_NAMESPACE
namespace __AC_NAMESPACE
{
#endif

   template <typename T>
   class ac_complex;

   namespace ac_private
   {
      // specializations after definition of ac_complex
      template <typename T>
      struct rt_ac_complex_T
      {
         template <typename T2>
         struct op1
         {
            using mult = typename T::template rt_T<ac_complex<T2>>::mult;
            using plus = typename T::template rt_T<ac_complex<T2>>::plus;
            using minus = typename T::template rt_T<ac_complex<T2>>::minus2;
            using minus2 = typename T::template rt_T<ac_complex<T2>>::minus;
            using logic = typename T::template rt_T<ac_complex<T2>>::logic;
            using div = typename T::template rt_T<ac_complex<T2>>::div2;
            using div2 = typename T::template rt_T<ac_complex<T2>>::div;
         };
      };
   } // namespace ac_private

   template <typename T>
   class ac_complex
   {
    public: // temporary workaround
      T _r;
      T _i;
      using map_T = typename ac_private::map<T>::t;
      using T_sqr = typename map_T::rt_unary::mag_sqr;
      using map_T_sqr = typename ac_private::map<T_sqr>::t;
      using map_T_mag = typename ac_private::map<typename map_T::rt_unary::mag>::t;

    public:
      using element_type = T;
      template <typename T2>
      struct rt_T
      {
         using map_T2 = typename ac_private::map<T2>::t;
         using mult = typename ac_private::rt_ac_complex_T<map_T2>::template op1<map_T>::mult;
         using plus = typename ac_private::rt_ac_complex_T<map_T2>::template op1<map_T>::plus;
         using minus = typename ac_private::rt_ac_complex_T<map_T2>::template op1<map_T>::minus;
         using minus2 = typename ac_private::rt_ac_complex_T<map_T2>::template op1<map_T>::minus2;
         using logic = typename ac_private::rt_ac_complex_T<map_T2>::template op1<map_T>::logic;
         using div = typename ac_private::rt_ac_complex_T<map_T2>::template op1<map_T>::div;
         using div2 = typename ac_private::rt_ac_complex_T<map_T2>::template op1<map_T>::div2;
         using arg1 = ac_complex<T>;
      };

      struct rt_unary
      {
         using mag_sqr = typename map_T_sqr::template rt_T<map_T_sqr>::plus;
         using mag = typename map_T_mag::template rt_T<map_T_mag>::plus; // overly conservative for signed
         using neg = ac_complex<typename map_T::rt_unary::neg>;
         template <unsigned N>
         struct set
         {
            using sum = ac_complex<typename map_T::rt_unary::template set<N>::sum>;
         };
      };

      ac_complex() = default;
      template <typename T2>
      ac_complex(const ac_complex<T2>& c)
      {
         _r = c.r();
         _i = c.i();
      }
      template <typename T2>
      ac_complex(const T2& r)
      {
         _r = r;
         _i = 0;
      }
      template <typename T2, typename T3>
      ac_complex(const T2& r, const T3& i)
      {
         _r = r;
         _i = i;
      }
      const T& r() const
      {
         return _r;
      }
      const T& i() const
      {
         return _i;
      }
      T& r()
      {
         return _r;
      }
      T& i()
      {
         return _i;
      }
      const T& real() const
      {
         return _r;
      }
      const T& imag() const
      {
         return _i;
      }
      T& real()
      {
         return _r;
      }
      T& imag()
      {
         return _i;
      }
      template <typename T2>
      void set_r(const T2& r)
      {
         _r = r;
      }
      template <typename T2>
      void set_i(const T2& i)
      {
         _i = i;
      }

      // const binary operators are global rather than members because of compiler
      // errors due to ambiguity (would appear as a compiler bug)

      template <typename T2>
      ac_complex& operator+=(const ac_complex<T2>& op2)
      {
         _r += op2.r();
         _i += op2.i();
         return *this;
      }

      template <typename T2>
      ac_complex& operator+=(const T2& op2)
      {
         _r += op2;
         return *this;
      }

      template <typename T2>
      ac_complex& operator-=(const ac_complex<T2>& op2)
      {
         _r -= op2.r();
         _i -= op2.i();
         return *this;
      }

      template <typename T2>
      ac_complex& operator-=(const T2& op2)
      {
         _r -= op2;
         return *this;
      }

      template <typename T2>
      ac_complex& operator*=(const ac_complex<T2>& op2)
      {
         T r0 = _r * op2.r() - _i * op2.i();
         _i = _r * op2.i() + _i * op2.r();
         _r = r0;
         return *this;
      }

      template <typename T2>
      ac_complex& operator*=(const T2& op2)
      {
         _r = _r * op2;
         _i = _i * op2;
         return *this;
      }

      template <typename T2>
      ac_complex& operator/=(const ac_complex<T2>& op2)
      {
         typename ac_complex<T2>::rt_unary::mag_sqr d = op2.mag_sqr();
         T r0 = (_r * op2.r() + _i * op2.i()) / d;
         _i = (_i * op2.r() - _r * op2.i()) / d;
         _r = r0;
         return *this;
      }

      template <typename T2>
      ac_complex& operator/=(const T2& op2)
      {
         _r = _r / op2;
         _i = _i / op2;
         return *this;
      }

      // Arithmetic Unary --------------------------------------------------------
      ac_complex operator+()
      {
         return *this;
      }
      typename rt_unary::neg operator-() const
      {
         typename rt_unary::neg res(-_r, -_i);
         return res;
      }

      // ! ------------------------------------------------------------------------
      bool operator!() const
      {
         return !_r && !_i;
      }

      typename rt_unary::neg conj() const
      {
         typename rt_unary::neg res(_r, -_i);
         return res;
      }

      typename rt_unary::mag_sqr mag_sqr() const
      {
         return _r * _r + _i * _i;
      }

      ac_complex<ac_int<2, true>> sign_conj() const
      {
         return ac_complex<ac_int<2, true>>(_r ? (_r < 0 ? -1 : 1) : 0, _i ? (_i < 0 ? 1 : -1) : 0);
      }

      __FORCE_INLINE static std::string type_name()
      {
         typedef typename ac_private::map<T>::t map_T;
         std::string r = "ac_complex<";
         r += map_T::type_name();
         r += '>';
         return r;
      }
   };

   namespace ac_private
   {
      // with T2 == ac_complex
      template <typename T2>
      struct rt_ac_complex_T<ac_complex<T2>>
      {
         template <typename T>
         struct op1
         {
            using plus = ac_complex<typename ac::rt_2T<T, T2>::plus>;
            using minus = ac_complex<typename ac::rt_2T<T, T2>::minus>;
            using minus2 = ac_complex<typename ac::rt_2T<T, T2>::minus2>;
            using logic = ac_complex<typename ac::rt_2T<T, T2>::logic>;
            using div = ac_complex<typename ac::rt_2T<T, T2>::div>;
            using div2 = ac_complex<typename ac::rt_2T<T, T2>::div2>;
            using mult = ac_complex<typename ac::rt_2T<
                typename ac::rt_2T<typename ac::rt_2T<T, T2>::mult, typename ac::rt_2T<T, T2>::mult>::plus,
                typename ac::rt_2T<typename ac::rt_2T<T, T2>::mult, typename ac::rt_2T<T, T2>::mult>::minus>::logic>;
         };
      };
      // with T2 == ac_float
      template <AC_FL_T0(2)>
      struct rt_ac_complex_T<AC_FL0(2)>
      {
         using T2 = ac_float<W2, I2, E2>;
         template <typename T>
         struct op1
         {
            using plus = ac_complex<typename T::template rt_T<T2>::plus>;
            using minus = ac_complex<typename T::template rt_T<T2>::minus>;
            using minus2 = ac_complex<typename T::template rt_T<T2>::minus2>;
            using logic = ac_complex<typename T::template rt_T<T2>::logic>;
            using div = ac_complex<typename T::template rt_T<T2>::div>;
            using div2 = ac_complex<typename T::template rt_T<T2>::div2>;
            using mult = ac_complex<typename T::template rt_T<T2>::mult>;
         };
      };
      // with T2 == ac_fixed
      template <int W2, int I2, bool S2>
      struct rt_ac_complex_T<ac_fixed<W2, I2, S2>>
      {
         using T2 = ac_fixed<W2, I2, S2>;
         template <typename T>
         struct op1
         {
            using plus = ac_complex<typename T::template rt_T<T2>::plus>;
            using minus = ac_complex<typename T::template rt_T<T2>::minus>;
            using minus2 = ac_complex<typename T::template rt_T<T2>::minus2>;
            using logic = ac_complex<typename T::template rt_T<T2>::logic>;
            using div = ac_complex<typename T::template rt_T<T2>::div>;
            using div2 = ac_complex<typename T::template rt_T<T2>::div2>;
            using mult = ac_complex<typename T::template rt_T<T2>::mult>;
         };
      };
      // with T2 == ac_int
      template <int W2, bool S2>
      struct rt_ac_complex_T<ac_int<W2, S2>>
      {
         using T2 = ac_int<W2, S2>;
         template <typename T>
         struct op1
         {
            using plus = ac_complex<typename T::template rt_T<T2>::plus>;
            using minus = ac_complex<typename T::template rt_T<T2>::minus>;
            using minus2 = ac_complex<typename T::template rt_T<T2>::minus2>;
            using logic = ac_complex<typename T::template rt_T<T2>::logic>;
            using div = ac_complex<typename T::template rt_T<T2>::div>;
            using div2 = ac_complex<typename T::template rt_T<T2>::div2>;
            using mult = ac_complex<typename T::template rt_T<T2>::mult>;
         };
      };
      // with T2 == c_type<TC>
      template <typename TC>
      struct rt_ac_complex_T<c_type<TC>>
      {
         using T2 = c_type<TC>;
         template <typename T>
         struct op1
         {
            using plus = ac_complex<typename T::template rt_T<T2>::plus>;
            using minus = ac_complex<typename T::template rt_T<T2>::minus>;
            using minus2 = ac_complex<typename T::template rt_T<T2>::minus2>;
            using logic = ac_complex<typename T::template rt_T<T2>::logic>;
            using div = ac_complex<typename T::template rt_T<T2>::div>;
            using div2 = ac_complex<typename T::template rt_T<T2>::div2>;
            using mult = ac_complex<typename T::template rt_T<T2>::mult>;
         };
      };
   } // namespace ac_private

   template <typename T, typename T2>
   __FORCE_INLINE typename ac_complex<T>::template rt_T<ac_complex<T2>>::plus operator+(const ac_complex<T>& op,
                                                                                        const ac_complex<T2>& op2)
   {
      typename ac_complex<T>::template rt_T<ac_complex<T2>>::plus res(op.r() + op2.r(), op.i() + op2.i());
      return res;
   }

   template <typename T, typename T2>
   __FORCE_INLINE typename ac_complex<T2>::template rt_T<T>::plus operator+(const T& op, const ac_complex<T2>& op2)
   {
      typename ac_complex<T2>::template rt_T<T>::plus res(op + op2.r(), op2.i());
      return res;
   }

   template <typename T, typename T2>
   __FORCE_INLINE typename ac_complex<T>::template rt_T<T2>::plus operator+(const ac_complex<T>& op, const T2& op2)
   {
      typename ac_complex<T>::template rt_T<T2>::plus res(op.r() + op2, op.i());
      return res;
   }

   template <typename T, typename T2>
   __FORCE_INLINE typename ac_complex<T>::template rt_T<ac_complex<T2>>::minus operator-(const ac_complex<T>& op,
                                                                                         const ac_complex<T2>& op2)
   {
      typename ac_complex<T>::template rt_T<ac_complex<T2>>::minus res(op.r() - op2.r(), op.i() - op2.i());
      return res;
   }

   template <typename T, typename T2>
   __FORCE_INLINE typename ac_complex<T2>::template rt_T<T>::minus2 operator-(const T& op, const ac_complex<T2>& op2)
   {
      typename ac_complex<T2>::template rt_T<T>::minus2 res(op - op2.r(), -op2.i());
      return res;
   }

   template <typename T, typename T2>
   __FORCE_INLINE typename ac_complex<T>::template rt_T<T2>::minus operator-(const ac_complex<T>& op, const T2& op2)
   {
      typename ac_complex<T>::template rt_T<T2>::minus res(op.r() - op2, op.i());
      return res;
   }

   template <typename T, typename T2>
   __FORCE_INLINE typename ac_complex<T>::template rt_T<ac_complex<T2>>::mult operator*(const ac_complex<T>& op,
                                                                                        const ac_complex<T2>& op2)
   {
      typename ac_complex<T>::template rt_T<ac_complex<T2>>::mult res(op.r() * op2.r() - op.i() * op2.i(),
                                                                      op.i() * op2.r() + op.r() * op2.i());
      return res;
   }

   template <typename T, typename T2>
   __FORCE_INLINE typename ac_complex<T2>::template rt_T<T>::mult operator*(const T& op, const ac_complex<T2>& op2)
   {
      typename ac_complex<T2>::template rt_T<T>::mult res(op * op2.r(), op * op2.i());
      return res;
   }

   template <typename T, typename T2>
   __FORCE_INLINE typename ac_complex<T>::template rt_T<T2>::mult operator*(const ac_complex<T>& op, const T2& op2)
   {
      typename ac_complex<T>::template rt_T<T2>::mult res(op.r() * op2, op.i() * op2);
      return res;
   }

   template <typename T, typename T2>
   __FORCE_INLINE typename ac_complex<T>::template rt_T<ac_complex<T2>>::div operator/(const ac_complex<T>& op,
                                                                                       const ac_complex<T2>& op2)
   {
      typename ac_complex<T2>::rt_unary::mag_sqr d = op2.mag_sqr();
      typename ac_complex<T>::template rt_T<ac_complex<T2>>::div res((op.r() * op2.r() + op.i() * op2.i()) / d,
                                                                     (op.i() * op2.r() - op.r() * op2.i()) / d);
      return res;
   }

   template <typename T, typename T2>
   __FORCE_INLINE typename ac_complex<T>::template rt_T<T2>::div operator/(const ac_complex<T>& op, const T2& op2)
   {
      typename ac_complex<T>::template rt_T<T2>::div res(op.r() / op2, op.i() / op2);
      return res;
   }

   template <typename T, typename T2>
   __FORCE_INLINE typename ac_complex<T2>::template rt_T<T>::div2 operator/(const T& op, const ac_complex<T2>& op2)
   {
      typename ac_complex<T2>::rt_unary::mag_sqr d = op2.mag_sqr();
      typename ac_complex<T2>::template rt_T<T>::div2 res(op * op2.r() / d, -op * op2.i() / d);
      return res;
   }

   template <typename T, typename T2>
   __FORCE_INLINE bool operator==(const ac_complex<T>& op, const ac_complex<T2>& op2)
   {
      return op.r() == op2.r() && op.i() == op2.i();
   }

   template <typename T, typename T2>
   __FORCE_INLINE bool operator==(const T& op, const ac_complex<T2>& op2)
   {
      return op == op2.r() && op2.i() == 0;
   }

   template <typename T, typename T2>
   __FORCE_INLINE bool operator==(const ac_complex<T>& op, const T2& op2)
   {
      return op.r() == op2 && op.i() == 0;
   }

   template <typename T, typename T2>
   __FORCE_INLINE bool operator!=(const ac_complex<T>& op, const ac_complex<T2>& op2)
   {
      return op.r() != op2.r() || op.i() != op2.i();
   }

   template <typename T, typename T2>
   __FORCE_INLINE bool operator!=(const T& op, const ac_complex<T2>& op2)
   {
      return op != op2.r() || op2.i() != 0;
   }

   template <typename T, typename T2>
   __FORCE_INLINE bool operator!=(const ac_complex<T>& op, const T2& op2)
   {
      return op.r() != op2 || op.i() != 0;
   }

   // Stream --------------------------------------------------------------------

   template <typename T>
   __FORCE_INLINE std::ostream& operator<<(std::ostream& os, const ac_complex<T>& x)
   {
#ifndef __BAMBU__
      os << "(" << x.r() << ", " << x.i() << ")";
#endif
      return os;
   }

   template <ac_special_val V, typename T>
   __FORCE_INLINE ac_complex<T> value(ac_complex<T>)
   {
      T val = value<V>((T)0);
      ac_complex<T> r(val, val);
      return r;
   }

   namespace ac
   {
      template <ac_special_val V, typename T>
      __FORCE_INLINE bool init_array(ac_complex<T>* a, int n)
      {
         ac_complex<T> t = value<V>(*a);
         for(int i = 0; i < n; i++)
         {
            a[i] = t;
         }
         return true;
      }
   } // namespace ac

#ifdef __AC_NAMESPACE
}
#endif

#endif // __AC_COMPLEX_H
