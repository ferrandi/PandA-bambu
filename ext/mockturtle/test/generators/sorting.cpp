#include <catch.hpp>

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <random>
#include <vector>

#include <mockturtle/generators/sorting.hpp>

using namespace mockturtle;

TEST_CASE( "sorting networks based on bubble sort", "[sorting]" )
{
  for ( auto n = 0u; n < 17u; ++n )
  {
    std::vector<uint32_t> list( n );
    std::iota( list.begin(), list.end(), 0u );

    for ( auto r = 0u; r < std::max( n, 1u ); ++r )
    {
      auto copy = list;
      std::shuffle( copy.begin(), copy.end(), std::default_random_engine( 0 ) );

      bubble_sorting_network( n, [&]( auto a, auto b ) {
        if ( copy[a] > copy[b] )
        {
          std::swap( copy[a], copy[b] );
        }
      } );

      CHECK( copy == list );
    }
  }
}

TEST_CASE( "sorting networks based on insertion sort", "[sorting]" )
{
  for ( auto n = 0u; n < 17u; ++n )
  {
    std::vector<uint32_t> list( n );
    std::iota( list.begin(), list.end(), 0u );

    for ( auto r = 0u; r < std::max( n, 1u ); ++r )
    {
      auto copy = list;
      std::shuffle( copy.begin(), copy.end(), std::default_random_engine( 0 ) );

      insertion_sorting_network( n, [&]( auto a, auto b ) {
        if ( copy[a] > copy[b] )
        {
          std::swap( copy[a], copy[b] );
        }
      } );

      CHECK( copy == list );
    }
  }
}

TEST_CASE( "sorting networks based on batcher sort", "[sorting]" )
{
  for ( auto n = 1u; n < 4u; ++n )
  {
    const auto N = 1u << n;
    std::vector<uint32_t> list( N );
    std::iota( list.begin(), list.end(), 0u );

    for ( auto r = 0u; r < std::max( N, 1u ); ++r )
    {
      auto copy = list;
      std::shuffle( copy.begin(), copy.end(), std::default_random_engine( 0 ) );

      insertion_sorting_network( N, [&]( auto a, auto b ) {
        if ( copy[a] > copy[b] )
        {
          std::swap( copy[a], copy[b] );
        }
      } );

      CHECK( copy == list );
    }
  }
}
