/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2019  EPFL
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*!
  \file modular_arithmetic.hpp
  \brief Generate modular arithmetic logic networks

  \author Mathias Soeken
*/

#pragma once

#include <cstdint>
#include <vector>

#include "../traits.hpp"
#include "arithmetic.hpp"
#include "control.hpp"

namespace mockturtle
{

namespace detail
{

template<class IntType = int64_t>
inline std::pair<IntType, IntType> compute_montgomery_parameters( IntType c, IntType k = 0 )
{
  if ( k == 0 )
  {
    k = 1 << ( static_cast<IntType>( std::ceil( std::log2( c ) ) ) + 1 );
  }

  // egcd
  IntType y = k % c;
  IntType x = c;
  IntType a{0}, b{1};

  while ( y )
  {
    std::tie( a, b ) = std::pair<IntType, IntType>{b, a - ( x / y ) * b};
    std::tie( x, y ) = std::pair<IntType, IntType>{y, x % y};
  }

  const IntType ki = ( a > 0 ) ? ( a % c ) : ( c + (a % c) % c );
  const IntType factor = ( k * ki - 1 ) / c;

  return {k, factor};
}

template<class Ntk>
std::vector<signal<Ntk>> to_montgomery_form( Ntk& ntk, std::vector<signal<Ntk>> const& t, int32_t mod, uint32_t rbits, int64_t np )
{
  /* bit-width of original mod */
  uint32_t nbits = t.size() - rbits;

  std::vector<signal<Ntk>> t_rpart( t.begin(), t.begin() + rbits );
  auto m = carry_ripple_multiplier( ntk, t_rpart, constant_word( ntk, np, rbits) );
  assert( m.size() == 2 * rbits );
  m.resize( rbits );
  assert( m.size() == rbits );

  m = carry_ripple_multiplier( ntk, m, constant_word( ntk, mod, nbits ) );
  assert( m.size() == t.size() );

  auto carry = ntk.get_constant( false );
  carry_ripple_adder_inplace( ntk, m, t, carry );

  m.erase( m.begin(), m.begin() + rbits );
  assert( m.size() == nbits );

  std::vector<signal<Ntk>> sum( m.begin(), m.end() );
  auto carry_inv = ntk.get_constant( true );
  carry_ripple_subtractor_inplace( ntk, sum, constant_word( ntk, mod, nbits ), carry_inv );

  mux_inplace( ntk, !carry, m, sum );
  return m;
}

inline void invert_modulus( std::vector<bool>& m )
{
  m.flip();
  auto it = m.begin();
  do {
    *it = !*it;
  } while ( !*it++ );
}

} /* namespace detail */

/*! \brief Creates modular adder
 *
 * Given two input words of the same size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(a + b) \bmod 2^k\f$.
 * The first input word `a` is overriden and stores the output signals.
 */
template<class Ntk>
inline void modular_adder_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b )
{
  auto carry = ntk.get_constant( false );
  carry_ripple_adder_inplace( ntk, a, b, carry );
}

/*! \brief Creates modular adder
 *
 * Given two input words of the same size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(a + b) \bmod m\f$. 
 * The modulus `m` is passed as a vector of Booleans to support large bitsizes.
 * The first input word `a` is overriden and stores the output signals.
 */
template<class Ntk>
inline void modular_adder_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b, std::vector<bool> const& m )
{
  // bit-size for corrected addition
  const uint32_t bitsize = static_cast<uint32_t>( m.size() );
  assert( bitsize <= a.size() );

  // corrected registers
  std::vector<signal<Ntk>> a_trim( a.begin(), a.begin() + bitsize );
  std::vector<signal<Ntk>> b_trim( b.begin(), b.begin() + bitsize );

  // 1. Compute (a + b) on bitsize bits
  auto carry = ntk.get_constant( false );
  carry_ripple_adder_inplace( ntk, a_trim, b_trim, carry ); /* a_trim <- a + b */

  // store result in sum (and extend it to bitsize + 1 bits)
  auto sum = a_trim; /* sum <- a + b */
  sum.emplace_back( ntk.get_constant( false ) );

  // 2. Compute (a + b) - m (m is represented as word) on (bitsize + 1) bits
  std::vector<signal<Ntk>> word( bitsize + 1, ntk.get_constant( false ) );
  std::transform( m.begin(), m.end(), word.begin(), [&]( auto b ) { return ntk.get_constant( b ); } );
  auto carry_inv = ntk.get_constant( true );
  a_trim.emplace_back( carry );
  carry_ripple_subtractor_inplace( ntk, a_trim, word, carry_inv ); /* a_trim <- a + b - c */

  // if overflow occurred in step 2, return result from step 2, otherwise, result from step 1.
  mux_inplace( ntk, carry_inv, a_trim, sum );

  // copy corrected register back into input register
  std::copy_n( a_trim.begin(), bitsize, a.begin() );
}

/*! \brief Creates modular adder
 *
 * Given two input words of the same size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(a + b) \bmod m\f$. 
 * The first input word `a` is overriden and stores the output signals.
 */
template<class Ntk>
inline void modular_adder_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b, uint64_t m )
{
  // simpler case
  if ( m == ( UINT64_C( 1 ) << a.size() ) )
  {
    modular_adder_inplace( ntk, a, b );
    return;
  }

  // bit-size for corrected addition
  const auto bitsize = static_cast<uint32_t>( std::ceil( std::log2( m ) ) );
  std::vector<bool> mvec( bitsize );
  for ( auto i = 0u; i < bitsize; ++i )
  {
    mvec[i] = static_cast<bool>( ( m >> i ) & 1 );
  }

  modular_adder_inplace( ntk, a, b, mvec );
}

template<class Ntk>
inline void modular_adder_hiasat_inplace( Ntk& ntk, std::vector<signal<Ntk>>& x, std::vector<signal<Ntk>> const& y, std::vector<bool> const& m )
{
  assert( m.size() <= x.size() );
  assert( x.size() == y.size() );

  const uint32_t bitsize = static_cast<uint32_t>( m.size() );

  // corrected registers
  std::vector<signal<Ntk>> x_trim( x.begin(), x.begin() + bitsize );
  std::vector<signal<Ntk>> y_trim( y.begin(), y.begin() + bitsize );

  // compute Z-vector from m-vector (Z = 2^bitsize - m)
  auto z = m;
  detail::invert_modulus( z );

  /* SAC unit */
  std::vector<signal<Ntk>> A( bitsize ), B( bitsize + 1 ), a( bitsize ), b( bitsize + 1 );

  B[0] = b[0] = ntk.get_constant( false );
  for ( auto i = 0u; i < bitsize; ++i )
  {
    A[i] = ntk.create_xor( x_trim[i], y_trim[i] );
    B[i + 1] = ntk.create_and( x_trim[i], y_trim[i] );
    a[i] = z[i] ? ntk.create_xnor( x_trim[i], y_trim[i] ) : A[i];
    b[i + 1] = z[i] ? ntk.create_or( x_trim[i], y_trim[i] ) : B[i + 1];
  }

  /* CPG unit */
  std::vector<signal<Ntk>> G( bitsize ), P( bitsize + 1 ), g( bitsize ), p( bitsize + 1 );
  for ( auto i = 0u; i < bitsize; ++i )
  {
    G[i] = ntk.create_and( A[i], B[i] );
    P[i] = ntk.create_xor( A[i], B[i] );
    g[i] = ntk.create_and( a[i], b[i] );
    p[i] = ntk.create_xor( a[i], b[i] );
  }
  P[bitsize] = B[bitsize];
  p[bitsize] = b[bitsize];

  /* CLA for C_out */
  std::vector<signal<Ntk>> C( bitsize );
  C[0] = p[bitsize];
  for ( auto i = 1u; i < bitsize; ++i )
  {
    std::vector<signal<Ntk>> cube;
    cube.push_back( g[i] );
    for ( auto j = i + 1u; j < bitsize; ++j )
    {
      cube.push_back( p[j] );
    }
    C[i] = ntk.create_nary_and( cube );
  }
  const auto Cout = ntk.create_nary_or( C );
  //ntk.create_po( Cout );

  /* MUX store result in p and g */
  p.pop_back();
  P.pop_back();
  mux_inplace( ntk, Cout, g, G );
  mux_inplace( ntk, Cout, p, P );

  /* CLAS */
  C[0] = ntk.get_constant( false );
  for ( auto i = 1u; i < bitsize; ++i )
  {
    C[i] = ntk.create_or( g[i - 1], ntk.create_and( p[i - 1], C[i - 1] ) );
  }

  for ( auto i = 0u; i < bitsize; ++i )
  {
    x[i] = ntk.create_xor( p[i], C[i] );
  }
}

template<class Ntk>
inline void modular_adder_hiasat_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b, uint64_t m )
{
  // simpler case
  if ( m == ( UINT64_C( 1 ) << a.size() ) )
  {
    modular_adder_inplace( ntk, a, b );
    return;
  }

  // bit-size for corrected addition
  const auto bitsize = static_cast<uint32_t>( std::ceil( std::log2( m ) ) );
  std::vector<bool> mvec( bitsize );
  for ( auto i = 0u; i < bitsize; ++i )
  {
    mvec[i] = static_cast<bool>( ( m >> i ) & 1 );
  }

  modular_adder_hiasat_inplace( ntk, a, b, mvec );
}

/*! \brief Creates modular subtractor
 *
 * Given two input words of the same size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(a - b) \bmod 2^k\f$.
 * The first input word `a` is overriden and stores the output signals.
 */
template<class Ntk>
inline void modular_subtractor_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b )
{
  auto carry = ntk.get_constant( true );
  carry_ripple_subtractor_inplace( ntk, a, b, carry );
}

/*! \brief Creates modular subtractor
 *
 * Given two input words of the same size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(a - b) \bmod m\f$. 
 * The modulus `m` is passed as a vector of Booleans to support large bitsizes.
 * The first input word `a` is overriden and stores the output signals.
 */
template<class Ntk>
inline void modular_subtractor_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b, std::vector<bool> const& m )
{
  // bit-size for corrected addition
  const uint32_t bitsize = static_cast<uint32_t>( m.size() );
  assert( bitsize <= a.size() );

  // corrected registers
  std::vector<signal<Ntk>> a_trim( a.begin(), a.begin() + bitsize );
  std::vector<signal<Ntk>> b_trim( b.begin(), b.begin() + bitsize );

  // 1. Compute (a - b) on bitsize bits
  auto carry_inv = ntk.get_constant( true );
  carry_ripple_subtractor_inplace( ntk, a_trim, b_trim, carry_inv ); /* a_trim <- a - b */

  // store result in sum (and extend it to bitsize + 1 bits)
  auto sum = a_trim; /* sum <- a - b */

  sum.emplace_back( ntk.get_constant( false ) );

  // 2. Compute (a - b) + m (m is represented as word) on (bitsize + 1) bits
  std::vector<signal<Ntk>> word( bitsize + 1, ntk.get_constant( false ) );
  std::transform( m.begin(), m.end(), word.begin(), [&]( auto b ) { return ntk.get_constant( b ); } );
  auto carry = ntk.get_constant( false );
  a_trim.emplace_back( ntk.create_not( carry_inv ) );
  carry_ripple_adder_inplace( ntk, a_trim, word, carry ); /* a_trim <- (a - b) + c */

  // if overflow occurred in step 2, return result from step 2, otherwise, result from step 1.
  mux_inplace( ntk, carry, a_trim, sum );

  // copy corrected register back into input register
  std::copy_n( a_trim.begin(), bitsize, a.begin() );
}

/*! \brief Creates modular subtractor
 *
 * Given two input words of the same size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(a - b) \bmod m\f$. 
 * The first input word `a` is overriden and stores the output signals.
 */
template<class Ntk>
inline void modular_subtractor_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b, uint64_t m )
{
  // simpler case
  if ( m == ( UINT64_C( 1 ) << a.size() ) )
  {
    modular_subtractor_inplace( ntk, a, b );
    return;
  }

  // bit-size for corrected subtraction
  const auto bitsize = static_cast<uint32_t>( std::ceil( std::log2( m ) ) );
  std::vector<bool> mvec( bitsize );
  for ( auto i = 0u; i < bitsize; ++i )
  {
    mvec[i] = static_cast<bool>( ( m >> i ) & 1 );
  }

  modular_subtractor_inplace( ntk, a, b, mvec );
}

/*! \brief Creates modular doubling (multiplication by 2)
 *
 * Given one input word \f$a\f$ of size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(2 * a) \bmod m\f$. 
 * The modulus `m` is passed as a vector of Booleans to support large bitsizes.
 * The input word `a` is overriden and stores the output signals.
 */
template<class Ntk>
inline void modular_doubling_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<bool> const& m )
{
  assert( a.size() >= m.size() );
  const auto bitsize = m.size();
  std::vector<signal<Ntk>> a_trim( a.begin(), a.begin() + bitsize );

  std::vector<signal<Ntk>> shifted( bitsize + 1u, ntk.get_constant( false ) );
  std::copy( a_trim.begin(), a_trim.end(), shifted.begin() + 1u );
  std::copy_n( shifted.begin(), bitsize, a_trim.begin() );

  std::vector<signal<Ntk>> word( bitsize + 1, ntk.get_constant( false ) );
  std::transform( m.begin(), m.end(), word.begin(), [&]( auto b ) { return ntk.get_constant( b ); } );

  auto carry_inv = ntk.get_constant( true );
  carry_ripple_subtractor_inplace( ntk, shifted, word, carry_inv );

  mux_inplace( ntk, ntk.create_not( carry_inv ), a_trim, std::vector<signal<Ntk>>( shifted.begin(), shifted.begin() + bitsize ) );
  std::copy( a_trim.begin(), a_trim.end(), a.begin() );
}

/*! \brief Creates modular doubling (multiplication by 2)
 *
 * Given one input word \f$a\f$ of size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(2 * a) \bmod m\f$. 
 * The input word `a` is overriden and stores the output signals.
 */
template<class Ntk>
inline void modular_doubling_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, uint64_t m )
{
  const auto bitsize = static_cast<uint32_t>( std::ceil( std::log2( m ) ) );
  std::vector<bool> mvec( bitsize );
  for ( auto i = 0u; i < bitsize; ++i )
  {
    mvec[i] = static_cast<bool>( ( m >> i ) & 1 );
  }

  modular_doubling_inplace( ntk, a, mvec );
}

/*! \brief Creates modular halving (corrected division by 2)
 *
 * Given one input word \f$a\f$ of size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(a / 2) \bmod m\f$.  The
 * modulus must be odd, and the function is evaluated to \f$a / 2\f$, if `a` is
 * even and to \f$(a + m) / 2\f$, if `a` is odd.
 * The modulus `m` is passed as a vector of Booleans to support large bitsizes.
 * The input word `a` is overriden and stores the output signals.
 */
template<class Ntk>
inline void modular_halving_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<bool> const& m )
{
  assert( a.size() >= m.size() );
  assert( m.size() > 0u );
  assert( m[0] );
  const auto bitsize = m.size();
  std::vector<signal<Ntk>> a_trim( a.begin(), a.begin() + bitsize );

  std::vector<signal<Ntk>> extended( bitsize + 1u, ntk.get_constant( false ) ), a_extended( bitsize + 1u, ntk.get_constant( false ) );
  std::copy( a_trim.begin(), a_trim.end(), extended.begin() );
  std::copy( a_trim.begin(), a_trim.end(), a_extended.begin() );

  std::vector<signal<Ntk>> word( bitsize + 1, ntk.get_constant( false ) );
  std::transform( m.begin(), m.end(), word.begin(), [&]( auto b ) { return ntk.get_constant( b ); } );

  auto carry = ntk.get_constant( false );
  carry_ripple_adder_inplace( ntk, extended, word, carry );

  mux_inplace( ntk, a_trim[0], extended, a_extended );

  std::copy_n( extended.begin() + 1, bitsize, a.begin() );
}

/*! \brief Creates modular halving (corrected division by 2)
 *
 * Given one input word \f$a\f$ of size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(a / 2) \bmod m\f$.  The
 * modulus must be odd, and the function is evaluated to \f$a / 2\f$, if `a` is
 * even and to \f$(a + m) / 2\f$, if `a` is odd.
 * The input word `a` is overriden and stores the output signals.
 */
template<class Ntk>
inline void modular_halving_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, uint64_t m )
{
  const auto bitsize = static_cast<uint32_t>( std::ceil( std::log2( m ) ) );
  std::vector<bool> mvec( bitsize );
  for ( auto i = 0u; i < bitsize; ++i )
  {
    mvec[i] = static_cast<bool>( ( m >> i ) & 1 );
  }

  modular_halving_inplace( ntk, a, mvec );
}

/*! \brief Creates modular multiplication
 *
 * Given two inputs words of the same size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(ab) \bmod c\f$.
 * The modulus `m` is passed as a vector of Booleans to support large bitsizes.
 * The first input word `a` is overriden and stores the output signals.
 */
template<class Ntk>
inline void modular_multiplication_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b, std::vector<bool> const& m )
{
  assert( a.size() >= m.size() );
  assert( a.size() == b.size() );

  const auto bitsize = m.size();
  std::vector<signal<Ntk>> a_trim( a.begin(), a.begin() + bitsize );
  std::vector<signal<Ntk>> b_trim( b.begin(), b.begin() + bitsize );

  std::vector<signal<Ntk>> accu( bitsize );
  auto itA = a_trim.rbegin();
  std::transform( b_trim.begin(), b_trim.end(), accu.begin(), [&]( auto const& f ) { return ntk.create_and( *itA, f ); } );

  while ( ++itA != a_trim.rend() )
  {
    modular_doubling_inplace( ntk, accu, m );
    std::vector<signal<Ntk>> summand( bitsize );
    std::transform( b_trim.begin(), b_trim.end(), summand.begin(), [&]( auto const& f ) { return ntk.create_and( *itA, f ); } );
    modular_adder_inplace( ntk, accu, summand, m );
  }

  std::copy( accu.begin(), accu.end(), a.begin() );
}

/*! \brief Creates modular multiplication
 *
 * Given two inputs words of the same size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(ab) \bmod c\f$.
 * The first input word `a` is overriden and stores the output signals.
 */
template<class Ntk>
inline void modular_multiplication_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b, uint64_t m )
{
  const auto bitsize = static_cast<uint32_t>( std::ceil( std::log2( m ) ) );
  std::vector<bool> mvec( bitsize );
  for ( auto i = 0u; i < bitsize; ++i )
  {
    mvec[i] = static_cast<bool>( ( m >> i ) & 1 );
  }

  modular_multiplication_inplace( ntk, a, b, mvec );
}

/*! \brief Creates vector of Booleans from hex string
 *
 * This function can be used to create moduli for very large numbers that cannot
 * be represented using any of the integer built-in data types.  If the vector
 * `res` is too small for the value `hex` most-significant digits will be
 * ignored.
 */
void bool_vector_from_hex( std::vector<bool>& res, std::string_view hex, bool shrink_to_fit = true )
{
  auto itR = res.begin();
  auto itS = hex.rbegin();

  while ( itR != res.end() && itS != hex.rend() )
  {
    uint32_t number{0};
    if ( *itS >= '0' && *itS <= '9' )
    {
      number = *itS - '0';
    }
    else if ( *itS >= 'a' && *itS <= 'f' )
    {
      number = *itS - 'a' + 10;
    }
    else if ( *itS >= 'A' && *itS <= 'F' )
    {
      number = *itS - 'A' + 10;
    }
    else
    {
      assert( false && "invalid hex number" );
    }

    for ( auto i = 0u; i < 4u; ++i )
    {
      *itR++ = ( number >> i ) & 1;
      if ( itR == res.end() )
      {
        break;
      }
    }

    ++itS;
  }

  if ( shrink_to_fit )
  {
    auto find_last = []( std::vector<bool>::const_iterator itFirst,
                         std::vector<bool>::const_iterator itLast,
                         bool value ) -> std::vector<bool>::const_iterator {
      auto cur = itLast;
      while ( itFirst != itLast )
      {
        if ( *itFirst == value )
        {
          cur = itFirst;
        }
        ++itFirst;
      }
      return cur;
    };

    const auto itLast = find_last( res.begin(), res.end(), true );
    if ( itLast == res.end() )
    {
      res.clear();
    }
    else
    {
      res.erase( find_last( res.begin(), res.end(), true ) + 1u, res.end() );
    }
  }
  else
  {
    /* in case the hex string was short, fill remaining values with false */
    std::fill( itR, res.end(), false );
  }
}



namespace legacy
{

/*! \brief Creates modular adder
 *
 * Given two input words of the same size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(a + b) \bmod (2^k -
 * c)\f$.  The first input word `a` is overriden and stores the output signals.
 */
template<class Ntk>
inline void modular_adder_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b, uint64_t c )
{
  /* c must be smaller than 2^k */
  assert( c < ( UINT64_C( 1 ) << a.size() ) );

  /* refer to simpler case */
  if ( c == 0 )
  {
    modular_adder_inplace( ntk, a, b );
    return;
  }

  const auto word = constant_word( ntk, c, static_cast<uint32_t>( a.size() ) );
  auto carry = ntk.get_constant( false );
  carry_ripple_adder_inplace( ntk, a, word, carry );

  carry = ntk.get_constant( false );
  carry_ripple_adder_inplace( ntk, a, b, carry );

  std::vector<signal<Ntk>> sum( a.begin(), a.end() );
  auto carry_inv = ntk.get_constant( true );
  carry_ripple_subtractor_inplace( ntk, a, word, carry_inv );

  mux_inplace( ntk, !carry, a, sum );
}

/*! \brief Creates modular subtractor
 *
 * Given two input words of the same size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(a - b) \bmod (2^k -
 * c)\f$.  The first input word `a` is overriden and stores the output signals.
 */
template<class Ntk>
inline void modular_subtractor_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b, uint64_t c )
{
  /* c must be smaller than 2^k */
  assert( c < ( UINT64_C( 1 ) << a.size() ) );

  /* refer to simpler case */
  if ( c == 0 )
  {
    modular_subtractor_inplace( ntk, a, b );
    return;
  }

  auto carry = ntk.get_constant( true );
  carry_ripple_subtractor_inplace( ntk, a, b, carry );

  const auto word = constant_word( ntk, c, static_cast<uint32_t>( a.size() ) );
  std::vector<signal<Ntk>> sum( a.begin(), a.end() );
  auto carry_inv = ntk.get_constant( true );
  carry_ripple_subtractor_inplace( ntk, sum, word, carry_inv );

  mux_inplace( ntk, carry, a, sum );
}

/*! \brief Creates modular multiplication based on Montgomery multiplication
 *
 * Given two inputs words of the same size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(ab) \bmod (2^k - c)\f$.
 * The first input word `a` is overriden and stores the output signals.
 *
 * The implementation is based on Montgomery multiplication and includes the
 * encoding and decoding in and from the Montgomery number representation.
 * Correct functionality is only ensured if both `a` and `b` are smaller than
 * \f$2^k - c\f$.
 */
template<class Ntk>
inline void modular_multiplication_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b, uint64_t c )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );

  const auto n = ( 1 << a.size() ) - c;
  const auto nbits = static_cast<uint64_t>( std::ceil( std::log2( n ) ) );

  auto [r, np] = detail::compute_montgomery_parameters<int64_t>( n );
  const auto rbits = static_cast<uint64_t>( std::log2( r ) );

  const auto f2 = constant_word( ntk, ( r * r ) % n, rbits );

  const auto ma = detail::to_montgomery_form( ntk, carry_ripple_multiplier( ntk, a, f2 ), n, rbits, np );
  const auto mb = detail::to_montgomery_form( ntk, carry_ripple_multiplier( ntk, b, f2 ), n, rbits, np );

  assert( ma.size() == nbits );
  assert( mb.size() == nbits );

  a = detail::to_montgomery_form( ntk, zero_extend( ntk, carry_ripple_multiplier( ntk, ma, mb ), nbits + rbits ), n, rbits, np );
  a = detail::to_montgomery_form( ntk, zero_extend( ntk, a, nbits + rbits ), n, rbits, np );
}

}

} // namespace mockturtle
