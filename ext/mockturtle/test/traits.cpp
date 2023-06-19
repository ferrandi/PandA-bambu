#include <catch.hpp>

#include <cstdint>
#include <functional>
#include <numeric>
#include <vector>

#include <mockturtle/traits.hpp>

#include "test_networks.hpp"

using namespace mockturtle;

TEST_CASE( "dummy_network is a network", "[traits]" )
{
  CHECK( is_network_type_v<dummy_network> == true );
  CHECK( is_network_type<dummy_network>::value == true );
}

class dummy_network_with_compute : public dummy_network
{
public:
  dummy_network_with_compute( uint64_t& storage ) : dummy_network( storage )
  {
  }

  template<typename Iterator>
  iterates_over_t<Iterator, bool>
  compute( node const&, Iterator begin, Iterator end ) const
  {
    return std::accumulate( begin, end, true, std::logical_and<bool>() );
  }

  template<typename Iterator>
  iterates_over_t<Iterator, uint32_t>
  compute( node const&, Iterator begin, Iterator end ) const
  {
    return std::accumulate( begin, end, UINT32_C( 0 ), std::plus<uint32_t>() );
  }
};

TEST_CASE( "specialize functions with enable_if", "[traits]" )
{
  uint64_t storage;
  dummy_network_with_compute s( storage );

  std::vector<bool> bools{ { true, false, true } };
  std::vector<uint32_t> ints{ { 1, 2, 3, 4 } };

  CHECK( is_network_type_v<dummy_network_with_compute> == true );
  CHECK( has_compute_v<dummy_network_with_compute, bool> == true );
  CHECK( has_compute_v<dummy_network_with_compute, uint32_t> == true );
  CHECK( has_compute_v<dummy_network_with_compute, uint64_t> == false );

  CHECK( s.compute( 0, bools.begin(), bools.end() ) == false );
  CHECK( s.compute( 0, ints.begin(), ints.end() ) == 10u );
}
