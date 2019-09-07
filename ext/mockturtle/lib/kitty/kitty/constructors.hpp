/* kitty: C++ truth table library
 * Copyright (C) 2017-2019  EPFL
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
  \file constructors.hpp
  \brief Implements operations to construct truth tables

  \author Mathias Soeken
*/

#pragma once

#include <cctype>
#include <chrono>
#include <istream>
#include <random>
#include <stack>
#include <string>

#include "cube.hpp"
#include "detail/constants.hpp"
#include "detail/mscfix.hpp"
#include "detail/utils.hpp"
#include "dynamic_truth_table.hpp"
#include "operations.hpp"
#include "operators.hpp"
#include "static_truth_table.hpp"

namespace kitty
{

/*! \brief Creates truth table with number of variables

  If some truth table instance is given, one can create a truth table with the
  same type by calling the `construct()` method on it.  This function helps if
  only the number of variables is known and the base type and uniforms the
  creation of static and dynamic truth tables.  Note, however, that for static
  truth tables `num_vars` must be consistent to the number of variables in the
  truth table type.

  \param num_vars Number of variables
*/
template<typename TT>
inline TT create( unsigned num_vars )
{
  (void)num_vars;
  TT tt;
  assert( tt.num_vars() == static_cast<int>( num_vars ) );
  return tt;
}

/*! \cond PRIVATE */
template<>
inline dynamic_truth_table create<dynamic_truth_table>( unsigned num_vars )
{
  return dynamic_truth_table( num_vars );
}
/*! \endcond */

/*! \brief Constructs projections (single-variable functions)

  \param tt Truth table
  \param var_index Index of the variable, must be smaller than the truth table's number of variables
  \param complement If true, realize inverse projection
*/
template<typename TT>
void create_nth_var( TT& tt, uint64_t var_index, bool complement = false )
{
  if ( tt.num_vars() <= 6 )
  {
    /* assign from precomputed table */
    tt._bits[0] = complement ? ~detail::projections[var_index] : detail::projections[var_index];

    /* mask if truth table does not require all bits */
    tt.mask_bits();
  }
  else if ( var_index < 6 )
  {
    std::fill( std::begin( tt._bits ), std::end( tt._bits ), complement ? ~detail::projections[var_index] : detail::projections[var_index] );
  }
  else
  {
    const auto c = 1 << ( var_index - 6 );
    const auto zero = uint64_t( 0 );
    const auto one = ~zero;
    auto block = 0u;

    while ( block < tt.num_blocks() )
    {
      for ( auto i = 0; i < c; ++i )
      {
        tt._bits[block++] = complement ? one : zero;
      }
      for ( auto i = 0; i < c; ++i )
      {
        tt._bits[block++] = complement ? zero : one;
      }
    }
  }
}

/*! \cond PRIVATE */
template<int NumVars>
void create_nth_var( static_truth_table<NumVars, true>& tt, uint64_t var_index, bool complement = false )
{
  /* assign from precomputed table */
  tt._bits = complement ? ~detail::projections[var_index] : detail::projections[var_index];

  /* mask if truth table does not require all bits */
  tt.mask_bits();
}
/*! \endcond */

/*! \brief Constructs truth table from binary string

  Note that the first character in the string represents the most
  significant bit in the truth table.  For example, the 2-input AND
  function is represented by the binary string "1000".  The number of
  characters in `binary` must match the number of bits in `tt`.

  \param tt Truth table
  \param binary Binary string with as many characters as bits in the truth table
*/
template<typename TT>
void create_from_binary_string( TT& tt, const std::string& binary )
{
  assert( binary.size() == tt.num_bits() );

  clear( tt );

  size_t i = 0u, j = binary.size();
  do
  {
    --j;
    if ( binary[i++] == '1' )
    {
      set_bit( tt, j );
    }
  } while ( j );
}

/*! \brief Constructs truth table from hexadecimal string

  Note that the first character in the string represents the four most
  significant bit in the truth table.  For example, the 3-input
  majority function is represented by the binary string "E8" or "e8".
  The number of characters in `hex` must be one fourth the number of
  bits in `tt`.

  \param tt Truth table
  \param hex Hexadecimal string
*/
template<typename TT>
void create_from_hex_string( TT& tt, const std::string& hex )
{
  clear( tt );

  /* special case for small truth tables */
  if ( tt.num_vars() < 2 )
  {
    assert( hex.size() == 1 );
    const auto i = detail::hex_to_int[static_cast<unsigned char>( hex[0] )];
    if ( i & 1 )
    {
      set_bit( tt, 0 );
    }
    if ( tt.num_vars() == 1 && ( i & 2 ) )
    {
      set_bit( tt, 1 );
    }
    return;
  }

  assert( ( hex.size() << 2 ) == tt.num_bits() );

  auto j = tt.num_bits() - 1;

  for ( unsigned char c : hex )
  {
    const auto i = detail::hex_to_int[c];
    if ( i & 8 )
    {
      set_bit( tt, j );
    }
    if ( i & 4 )
    {
      set_bit( tt, j - 1 );
    }
    if ( i & 2 )
    {
      set_bit( tt, j - 2 );
    }
    if ( i & 1 )
    {
      set_bit( tt, j - 3 );
    }
    j -= 4;
  }
}

/*! \brief Creates string from raw character data

  Can create a truth table from the data that is produced by
  `print_raw`, e.g., from binary files or `std::stringstream`.

  \param tt Truth table
  \param in Input stream
*/
template<typename TT>
void create_from_raw( TT& tt, std::istream& in )
{
  std::for_each( tt.begin(), tt.end(), [&in]( auto& word ) { in.read( reinterpret_cast<char*>( &word ), sizeof( word ) ); } );
}

/*! \brief Constructs a truth table from random value

  Computes random words and assigns them to the truth table.  The
  number of variables is determined from the truth table.

  \param tt Truth table
  \param seed Random seed
*/
template<typename TT>
void create_random( TT& tt, std::default_random_engine::result_type seed )
{
  std::default_random_engine gen( seed );
  std::uniform_int_distribution<uint64_t> dist( 0ul, std::numeric_limits<uint64_t>::max() );

  assign_operation( tt, [&dist, &gen]() { return dist( gen ); } );
}

/*! \brief Constructs a truth table from random value

  Computes random words and assigns them to the truth table.  The
  number of variables is determined from the truth table.  Seed is
  taken from current time.

  \param tt Truth table
*/
template<typename TT>
void create_random( TT& tt )
{
  create_random( tt, std::chrono::system_clock::now().time_since_epoch().count() );
}

/*! \brief Constructs a truth table from a range of words

  The range of words is given in terms of a begin and end iterator.
  Hence, it's possible to copy words from a C++ container or a C
  array.

  \param tt Truth table
  \param begin Begin iterator
  \param end End iterator
*/
template<typename TT, typename InputIt>
void create_from_words( TT& tt, InputIt begin, InputIt end )
{
  assert( std::distance( begin, end ) == static_cast<unsigned>( tt.num_blocks() ) );
  std::copy( begin, end, tt.begin() );
}

/*! \brief Creates truth table from cubes representation

  A sum-of-product is represented as a vector of products (called
  cubes).

  An empty truth table is given as first argument to determine type
  and number of variables.  Literals in products that do not fit the
  number of variables of the truth table are ignored.

  The cube representation only allows truth table sizes up to 32
  variables.

  \param tt Truth table
  \param cubes Vector of cubes
  \param esop Use ESOP instead of SOP
*/
template<typename TT>
void create_from_cubes( TT& tt, const std::vector<cube>& cubes, bool esop = false )
{
  /* we collect product terms for an (E)SOP, start with const0 */
  clear( tt );

  for ( auto cube : cubes )
  {
    auto product = ~tt.construct(); /* const1 of same size */

    auto bits = cube._bits;
    auto mask = cube._mask;

    for ( auto i = 0; i < tt.num_vars(); ++i )
    {
      if ( mask & 1 )
      {
        auto var = tt.construct();
        create_nth_var( var, i, !( bits & 1 ) );
        product &= var;
      }
      bits >>= 1;
      mask >>= 1;
    }

    if ( esop )
    {
      tt ^= product;
    }
    else
    {
      tt |= product;
    }
  }
}

/*! \brief Creates truth table from clause representation

  A product-of-sum is represented as a vector of sums (called clauses).

  An empty truth table is given as first argument to determine type
  and number of variables.  Literals in sums that do not fit the
  number of variables of the truth table are ignored.

  The clause representation only allows truth table sizes up to 32
  variables.

  \param tt Truth table
  \param clauses Vector of clauses
  \param esop Use product of exclusive sums instead of POS
*/
template<typename TT>
void create_from_clauses( TT& tt, const std::vector<cube>& clauses, bool esop = false )
{
  /* we collect product terms for an (E)SOP, start with const0 */
  clear( tt );
  tt = ~tt;

  for ( auto clause : clauses )
  {
    auto sum = tt.construct(); /* const1 of same size */

    auto bits = clause._bits;
    auto mask = clause._mask;

    for ( auto i = 0; i < tt.num_vars(); ++i )
    {
      if ( mask & 1 )
      {
        auto var = tt.construct();
        create_nth_var( var, i, !( bits & 1 ) );

        if ( esop )
        {
          sum ^= var;
        }
        else
        {
          sum |= var;
        }
      }
      bits >>= 1;
      mask >>= 1;
    }

    tt &= sum;
  }
}

/*! \brief Constructs majority-n function

  The number of variables is determined from the truth table.

  \param tt Truth table
*/
template<typename TT>
inline void create_majority( TT& tt )
{
  create_threshold( tt, tt.num_vars() >> 1 );
}

/*! \brief Constructs threshold function

  The resulting function is true, if strictly more than `threshold` inputs are
  1. The number of variables is determined from the truth table.

  \param tt Truth table
  \param threshold threshold value
*/
template<typename TT>
void create_threshold( TT& tt, uint8_t threshold )
{
  clear( tt );

  for ( uint64_t x = 0; x < tt.num_bits(); ++x )
  {
    if ( __builtin_popcount( x ) > threshold )
    {
      set_bit( tt, x );
    }
  }
}

/*! \brief Constructs equals-k function

  The resulting function is true, if exactly `bitcount` bits are 1.  The number
  of variables is determiend from the truth table.

  \param tt Truth table
  \param bitcount equals-k value
*/
template<typename TT>
void create_equals( TT& tt, uint8_t bitcount )
{
  clear( tt );

  for ( uint64_t x = 0; x < tt.num_bits(); ++x )
  {
    if ( __builtin_popcount( x ) == bitcount )
    {
      set_bit( tt, x );
    }
  }
}

/*! \brief Constructs symmetric function

  Bits in `counts` are numbered from 0 to 63.  If bit `i` is set in `counts`,
  the created truth table will evaluate to true, if `i` bits are set in the
  input assignment.

  \param tt Truth table
  \param counts Bitcount mask
*/
template<typename TT>
void create_symmetric( TT& tt, uint64_t counts )
{
  clear( tt );

  for ( uint64_t x = 0; x < tt.num_bits(); ++x )
  {
    if ( ( counts >> __builtin_popcount( x ) ) & 1 )
    {
      set_bit( tt, x );
    }
  }
}

/*! \brief Constructs parity function over n variables

  The number of variables is determined from the truth table.

  \param tt Truth table
*/
template<typename TT>
void create_parity( TT& tt )
{
  clear( tt );

  *tt.begin() = UINT64_C( 0x6996966996696996 );

  if ( tt.num_vars() < 6 )
  {
    tt.mask_bits();
  }
  else if ( tt.num_vars() > 6 )
  {
    for ( auto i = 1u; i < tt.num_blocks(); i <<= 1 )
    {
      std::transform( tt.begin(), tt.begin() + i, tt.begin() + i, []( auto const& block ) { return ~block; } );
    }
  }
}

/*! \cond PRIVATE */
template<typename TT, typename Fn>
bool create_from_chain( TT& tt, Fn&& next_line, std::vector<TT>& steps, std::string* error )
{
  /* in case of error (makes code more readable) */
  auto fail_with = [&error]( const std::string& line, const std::string& message ) {
    if ( error )
    {
      *error = "error in \"" + line + "\": " + message;
    }
    return false;
  };

  /* initialize variable steps */
  steps.clear();
  for ( auto i = 0; i < tt.num_vars(); ++i )
  {
    auto var = tt.construct();
    create_nth_var( var, i );
    steps.push_back( var );
  }

  auto next_step = tt.num_vars() + 1;

  std::string line;
  while ( !( line = next_line() ).empty() )
  {
    detail::trim( line );

    /* first character must be an x */
    if ( line[0] != 'x' )
    {
      return fail_with( line, "variables must be prefixed with x" );
    }

    /* find equals sign */
    const auto eq = line.find( '=' );
    if ( eq == std::string::npos )
    {
      return fail_with( line, "no equal sign found" );
    }

    /* next step id */
    const auto step = std::stoi( line.substr( 1, eq - 1 ) );
    if ( step != next_step )
    {
      return fail_with( line, "steps are not in order" );
    }

    line = detail::trim_copy( line.substr( eq + 1 ) );

    if ( line.empty() )
    {
      return fail_with( line, "line uncompleted" );
    }

    /* first character must be an x */
    if ( line[0] != 'x' )
    {
      return fail_with( line, "variables must be prefixed with x" );
    }

    std::size_t op_pos = 0;
    const auto op1 = std::stoi( line.substr( 1 ), &op_pos );

    if ( op1 < 1 || op1 >= step )
    {
      return fail_with( line, "invalid operand index" );
    }

    line = detail::trim_copy( line.substr( op_pos + 1 ) );

    if ( line.empty() )
    {
      return fail_with( line, "line uncompleted" );
    }

    const auto next_x = line.find( 'x' );

    if ( next_x == std::string::npos )
    {
      return fail_with( line, "variables must be prefixed with x" );
    }

    const auto op_code = detail::trim_copy( line.substr( 0, next_x ) );

    line = detail::trim_copy( line.substr( next_x ) );

    if ( line.empty() )
    {
      return fail_with( line, "line uncompleted" );
    }

    /* first character must be an x */
    if ( line[0] != 'x' )
    {
      return fail_with( line, "variables must be prefixed with x" );
    }

    const auto op2 = std::stoi( line.substr( 1 ) );

    if ( op2 < 1 || op2 >= step )
    {
      return fail_with( line, "invalid operand index" );
    }

    /* now process arguments */
    auto tt_step = tt.construct();

    if ( op_code == "!|" )
    {
      tt_step = ~binary_or( steps[op1 - 1], steps[op2 - 1] );
    }
    else if ( op_code == ">" )
    {
      tt_step = binary_and( steps[op1 - 1], ~steps[op2 - 1] );
    }
    else if ( op_code == "<" )
    {
      tt_step = binary_and( ~steps[op1 - 1], steps[op2 - 1] );
    }
    else if ( op_code == "^" )
    {
      tt_step = binary_xor( steps[op1 - 1], steps[op2 - 1] );
    }
    else if ( op_code == "!&" )
    {
      tt_step = ~binary_and( steps[op1 - 1], steps[op2 - 1] );
    }
    else if ( op_code == "&" )
    {
      tt_step = binary_and( steps[op1 - 1], steps[op2 - 1] );
    }
    else if ( op_code == "=" )
    {
      tt_step = ~binary_xor( steps[op1 - 1], steps[op2 - 1] );
    }
    else if ( op_code == "<=" )
    {
      tt_step = binary_or( ~steps[op1 - 1], steps[op2 - 1] );
    }
    else if ( op_code == ">=" )
    {
      tt_step = binary_or( steps[op1 - 1], ~steps[op2 - 1] );
    }
    else if ( op_code == "|" )
    {
      tt_step = binary_or( steps[op1 - 1], steps[op2 - 1] );
    }
    else
    {
      return fail_with( op_code, "invalid operator" );
    }
    steps.push_back( tt_step );
    ++next_step;
  }

  return true;
}
/*! \endcond */

/*! \brief Constructs truth table from Boolean chain

  If ``tt`` has \f$n\f$ variables, then each string in ``steps`` is of the form

  \verbatim embed:rst
      ::

        x<i> = x<j> <op> x<k>
  \endverbatim

  where ``<i>`` is an increasing number starting from \f$n + 1\f$, and ``<j>``
  and ``<k>`` refer to previous steps or primary inputs where \f$j < i\f$ and
  \f$k < i\f$.  Primary inputs are indexed from \f$1\f$ to \f$n\f$.  The last
  computed step will be assigned to ``tt``.  The following operators are
  supported:

  \verbatim embed:rst
      +----------+-------------------------+-------------+
      | ``<op>`` | Operation               | Truth table |
      +==========+=========================+=============+
      | ``"!|"`` | Nondisjunction          | 0001        |
      +----------+-------------------------+-------------+
      | ``">"``  | Nonimplication          | 0010        |
      +----------+-------------------------+-------------+
      | ``"<"``  | Converse nonimplication | 0100        |
      +----------+-------------------------+-------------+
      | ``"^"``  | Exclusive disjunction   | 0110        |
      +----------+-------------------------+-------------+
      | ``"!&"`` | Nonconjunction          | 0111        |
      +----------+-------------------------+-------------+
      | ``"&"``  | Conjunction             | 1000        |
      +----------+-------------------------+-------------+
      | ``"="``  | Equivalence             | 1001        |
      +----------+-------------------------+-------------+
      | ``">="`` | Nonimplication          | 1011        |
      +----------+-------------------------+-------------+
      | ``"<="`` | Implication             | 1101        |
      +----------+-------------------------+-------------+
      | ``"|"``  | Disjunction             | 1110        |
      +----------+-------------------------+-------------+
  \endverbatim

  The following example will generate the majority function:

  \verbatim embed:rst
      .. code-block:: cpp
      
         kitty::static_truth_table<3> tt;
         kitty::create_from_chain( tt, {"x4 = x1 & x2",
                                        "x5 = x1 & x3",
                                        "x6 = x2 & x3",
                                        "x7 = x4 | x5",
                                        "x8 = x6 | x7"} );
  \endverbatim

  If parsing fails, the function returns ``false``, and if ``error`` is not
  ``nullptr``, it contains a descriptive reason, why parsing failed.  Otherwise,
  the function returns ``true``.

  \param tt Truth table
  \param steps Vector of steps
  \param error If not null, a pointer to store the error message

  \return True on success
*/
template<typename TT>
bool create_from_chain( TT& tt, const std::vector<std::string>& steps, std::string* error = nullptr )
{
  std::vector<TT> vec_steps;
  auto it = steps.begin();
  if ( !create_from_chain( tt, [&it, &steps]() {
         return ( it != steps.end() ) ? *it++ : std::string();
       },
                           vec_steps, error ) )
  {
    return false;
  }

  tt = vec_steps.back();
  return true;
}

/*! \brief Constructs truth tables from Boolean chain

  Like ``create_from_chain``, but also returns all internally computed steps.

  \param num_vars Number of input variables
  \param tts Truth table for all steps, tt[i] corresponds to step x\f$(i + 1)\f$
  \param steps Vector of steps
  \param error If not null, a pointer to store the error message

  \return True on success
*/
template<typename TT>
bool create_multiple_from_chain( unsigned num_vars, std::vector<TT>& tts, const std::vector<std::string>& steps, std::string* error = nullptr )
{
  auto tt = create<TT>( num_vars );
  tts.clear();
  auto it = steps.begin();
  if ( !create_from_chain( tt, [&it, &steps]() {
         return ( it != steps.end() ) ? *it++ : std::string();
       },
                           tts, error ) )
  {
    return false;
  }

  tt = tts.back();
  return true;
}

/*! \brief Constructs truth table from Boolean chain

  Like the other ``create_from_chain`` function, but reads chain from an input
  stream instead of a vector of strings.  Lines are separated by a new line.
  Empty lines are skipped over.

  \param tt Truth table
  \param in Input stream to read chain
  \param error If not null, a pointer to store the error message

  \return True on success
*/
template<typename TT>
bool create_from_chain( TT& tt, std::istream& in, std::string* error = nullptr )
{
  std::vector<TT> vec_steps;
  if ( !create_from_chain( tt, [&in]() {
    std::string line;
    while ( true )
    {
      if ( std::getline( in, line ) )
      {
        detail::trim( line );
        if ( !line.empty() )
        {
          return line;
        }
      }
      else
      {
        return std::string();
      }
    } }, vec_steps, error ) )
  {
    return false;
  }

  tt = vec_steps.back();
  return true;
}

/*! \brief Constructs truth tables from Boolean chain

  Like ``create_from_chain``, but also returns all internally computed steps.

  \param num_vars Number of input variables
  \param tts Truth table for all steps, tt[i] corresponds to step x\f$(i + 1)\f$
  \param in Input stream to read chain
  \param error If not null, a pointer to store the error message

  \return True on success
*/
template<typename TT>
bool create_multiple_from_chain( unsigned num_vars, std::vector<TT>& tts, std::istream& in, std::string* error = nullptr )
{
  auto tt = create<TT>( num_vars );
  tts.clear();
  if ( !create_from_chain( tt, [&in]() {
    std::string line;
    while ( true )
    {
      if ( std::getline( in, line ) )
      {
        detail::trim( line );
        if ( !line.empty() )
        {
          return line;
        }
      }
      else
      {
        return std::string();
      }
    } }, tts, error ) )
  {
    return false;
  }

  tt = tts.back();
  return true;
}

/*! \brief Creates characteristic function

  Creates the truth table of the characteristic function, which contains one
  additional variable.  The new output variable will be the most-significant
  variable of the new function.

  \param tt Truth table for characteristic function
  \param from Input truth table
*/
template<typename TT, typename TTFrom>
inline void create_characteristic( TT& tt, const TTFrom& from )
{
  assert( tt.num_vars() == from.num_vars() + 1 );

  auto var = tt.construct();
  create_nth_var( var, from.num_vars() );

  auto ext = tt.construct();
  extend_to_inplace( ext, from );

  tt = ~var ^ ext;
}

/*! \brief Creates truth table from textual expression

  An expression `E` is a constant `0` or `1`, or a variable `a`, `b`, ..., `p`,
  the negation of an expression `!E`, the conjunction of multiple expressions
  `(E...E)`, the disjunction of multiple expressions `{E...E}`, the exclusive
  OR of multiple expressions `[E...E]`, or the majority of three expressions
  `<EEE>`.  Examples are `[(ab)(!ac)]` to describe if-then-else, or `!{!a!b}`
  to describe the application of De Morgan's law to `(ab)`.  The size of the
  truth table must fit the largest variable in the expression, e.g., if `c` is
  the largest variable, then the truth table have at least three variables.

  \param tt Truth table
  \param from Expression as string
*/
template<typename TT>
bool create_from_expression( TT& tt, const std::string& expression )
{
  enum stack_symbols
  {
    FUNC,
    AND,
    OR,
    XOR,
    MAJ,
    NEG
  };
  std::stack<stack_symbols> symbols;
  std::stack<TT> truth_tables;

  const auto push_tt = [&]( TT& func ) {
    while ( !symbols.empty() && symbols.top() == NEG )
    {
      func = ~func;
      symbols.pop();
    }
    symbols.push( FUNC );
    truth_tables.push( func );
  };

  for ( auto const& c : expression )
  {
    switch ( c )
    {
    default:
      if ( c >= 'a' && c <= 'p' )
      {
        auto var = tt.construct();
        create_nth_var( var, c - 'a' );
        push_tt( var );
      }
      else
      {
        std::cerr << "[e] unexpected symbol in expression: " << c << "\n";
        return false;
      }
      break;
    case '0':
    {
      auto func = tt.construct();
      push_tt( func );
    }
    break;
    case '1':
    {
      auto func = ~tt.construct();
      push_tt( func );
    }
    break;
    case '!':
      symbols.push( NEG );
      break;
    case '(':
      symbols.push( AND );
      break;
    case '{':
      symbols.push( OR );
      break;
    case '[':
      symbols.push( XOR );
      break;
    case '<':
      symbols.push( MAJ );
      break;
    case ')':
    {
      auto func = ~tt.construct();
      while ( !symbols.empty() && symbols.top() == FUNC )
      {
        func &= truth_tables.top();
        symbols.pop();
        truth_tables.pop();
      }
      if ( symbols.empty() || symbols.top() != AND )
      {
        std::cerr << "[e] could not parse AND expression\n";
        return false;
      }
      symbols.pop();
      push_tt( func );
    }
    break;
    case '}':
    {
      auto func = tt.construct();
      while ( !symbols.empty() && symbols.top() == FUNC )
      {
        func |= truth_tables.top();
        symbols.pop();
        truth_tables.pop();
      }
      if ( symbols.empty() || symbols.top() != OR )
      {
        std::cerr << "[e] could not parse OR expression\n";
        return false;
      }
      symbols.pop();
      push_tt( func );
    }
    break;
    case ']':
    {
      auto func = tt.construct();
      while ( !symbols.empty() && symbols.top() == FUNC )
      {
        func ^= truth_tables.top();
        symbols.pop();
        truth_tables.pop();
      }
      if ( symbols.empty() || symbols.top() != XOR )
      {
        std::cerr << "[e] could not parse XOR expression\n";
        return false;
      }
      symbols.pop();
      push_tt( func );
    }
    break;
    case '>':
    {
      std::vector<TT> children;
      while ( !symbols.empty() && symbols.top() == FUNC )
      {
        children.push_back( truth_tables.top() );
        symbols.pop();
        truth_tables.pop();
      }
      if ( symbols.empty() || symbols.top() != MAJ )
      {
        std::cerr << "[e] could not parse MAJ expression\n";
        return false;
      }
      if ( children.size() != 3u )
      {
        std::cerr << "[e] MAJ expression must have three children\n";
        return false;
      }
      symbols.pop();
      auto func = ternary_majority( children[0], children[1], children[2] );
      push_tt( func );
    }
    break;
    }
  }

  if ( symbols.size() != 1 || truth_tables.size() != 1 )
  {
    std::cerr << "[e] expression parsing incomplete\n";
    return false;
  }

  tt = truth_tables.top();
  return true;
}

/*! \brief Creates function where on-set corresponds to prime numbers

  This creates a function in which \f$f(x) = 1\f$, if and only if \f$x\f$ is
  a prime number in its integer representation.  The function only works for
  truth tables with at most 10 variables.  The number of variables is determined
  from the truth table.

  \param tt Truth table
*/
template<class TT>
void create_prime( TT& tt )
{
  if ( tt.num_vars() > 10 ) return;

  clear( tt );
  auto p = detail::primes;

  while ( *p < tt.num_bits() )
  {
    set_bit( tt, *p++ );
  }
}

} // namespace kitty
