#include <catch.hpp>

#include <iostream>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <mockturtle/algorithms/cut_enumeration.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>

using namespace mockturtle;

TEST_CASE( "enumerate cuts for an AIG", "[cut_enumeration]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( f1, a );
  const auto f3 = aig.create_nand( f1, b );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  const auto cuts = cut_enumeration( aig );

  const auto to_vector = []( auto const& cut ) {
    return std::vector<uint32_t>( cut.begin(), cut.end() );
  };

  /* all unit cuts are in the back */
  aig.foreach_node( [&]( auto n ) {
    if ( aig.is_constant( n ) )
      return;

    auto const& set = cuts.cuts( aig.node_to_index( n ) );
    CHECK( to_vector( set[static_cast<uint32_t>( set.size() - 1 )] ) == std::vector<uint32_t>{aig.node_to_index( n )} );
  } );

  const auto i1 = aig.node_to_index( aig.get_node( f1 ) );
  const auto i2 = aig.node_to_index( aig.get_node( f2 ) );
  const auto i3 = aig.node_to_index( aig.get_node( f3 ) );
  const auto i4 = aig.node_to_index( aig.get_node( f4 ) );

  CHECK( cuts.cuts( i1 ).size() == 2 );
  CHECK( cuts.cuts( i2 ).size() == 3 );
  CHECK( cuts.cuts( i3 ).size() == 3 );
  CHECK( cuts.cuts( i4 ).size() == 5 );

  CHECK( to_vector( cuts.cuts( i1 )[0] ) == std::vector<uint32_t>{1, 2} );

  CHECK( to_vector( cuts.cuts( i2 )[0] ) == std::vector<uint32_t>{1, 3} );
  CHECK( to_vector( cuts.cuts( i2 )[1] ) == std::vector<uint32_t>{1, 2} );

  CHECK( to_vector( cuts.cuts( i3 )[0] ) == std::vector<uint32_t>{2, 3} );
  CHECK( to_vector( cuts.cuts( i3 )[1] ) == std::vector<uint32_t>{1, 2} );

  CHECK( to_vector( cuts.cuts( i4 )[0] ) == std::vector<uint32_t>{4, 5} );
  CHECK( to_vector( cuts.cuts( i4 )[1] ) == std::vector<uint32_t>{1, 2} );
  CHECK( to_vector( cuts.cuts( i4 )[2] ) == std::vector<uint32_t>{2, 3, 4} );
  CHECK( to_vector( cuts.cuts( i4 )[3] ) == std::vector<uint32_t>{1, 3, 5} );
}

TEST_CASE( "enumerate smaller cuts for an AIG", "[cut_enumeration]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( f1, a );
  const auto f3 = aig.create_nand( f1, b );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  cut_enumeration_params ps;
  ps.cut_size = 2;
  const auto cuts = cut_enumeration( aig, ps );

  const auto to_vector = []( auto const& cut ) {
    return std::vector<uint32_t>( cut.begin(), cut.end() );
  };

  const auto i1 = aig.node_to_index( aig.get_node( f1 ) );
  const auto i2 = aig.node_to_index( aig.get_node( f2 ) );
  const auto i3 = aig.node_to_index( aig.get_node( f3 ) );
  const auto i4 = aig.node_to_index( aig.get_node( f4 ) );

  CHECK( cuts.cuts( i1 ).size() == 2 );
  CHECK( cuts.cuts( i2 ).size() == 3 );
  CHECK( cuts.cuts( i3 ).size() == 3 );
  CHECK( cuts.cuts( i4 ).size() == 3 );

  CHECK( to_vector( cuts.cuts( i1 )[0] ) == std::vector<uint32_t>{1, 2} );

  CHECK( to_vector( cuts.cuts( i2 )[0] ) == std::vector<uint32_t>{1, 3} );
  CHECK( to_vector( cuts.cuts( i2 )[1] ) == std::vector<uint32_t>{1, 2} );

  CHECK( to_vector( cuts.cuts( i3 )[0] ) == std::vector<uint32_t>{2, 3} );
  CHECK( to_vector( cuts.cuts( i3 )[1] ) == std::vector<uint32_t>{1, 2} );

  CHECK( to_vector( cuts.cuts( i4 )[0] ) == std::vector<uint32_t>{4, 5} );
  CHECK( to_vector( cuts.cuts( i4 )[1] ) == std::vector<uint32_t>{1, 2} );
}

TEST_CASE( "compute truth tables of AIG cuts", "[cut_enumeration]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( f1, a );
  const auto f3 = aig.create_nand( f1, b );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  const auto cuts = cut_enumeration<aig_network, true>( aig );

  const auto i1 = aig.node_to_index( aig.get_node( f1 ) );
  const auto i2 = aig.node_to_index( aig.get_node( f2 ) );
  const auto i3 = aig.node_to_index( aig.get_node( f3 ) );
  const auto i4 = aig.node_to_index( aig.get_node( f4 ) );

  CHECK( cuts.cuts( i1 ).size() == 2 );
  CHECK( cuts.cuts( i2 ).size() == 3 );
  CHECK( cuts.cuts( i3 ).size() == 3 );
  CHECK( cuts.cuts( i4 ).size() == 5 );

  CHECK( cuts.truth_table( cuts.cuts( i1 )[0] )._bits[0] == 0x8 );
  CHECK( cuts.truth_table( cuts.cuts( i2 )[0] )._bits[0] == 0x2 );
  CHECK( cuts.truth_table( cuts.cuts( i2 )[1] )._bits[0] == 0x2 );
  CHECK( cuts.truth_table( cuts.cuts( i3 )[0] )._bits[0] == 0x2 );
  CHECK( cuts.truth_table( cuts.cuts( i3 )[1] )._bits[0] == 0x4 );
  CHECK( cuts.truth_table( cuts.cuts( i4 )[0] )._bits[0] == 0x1 );
  CHECK( cuts.truth_table( cuts.cuts( i4 )[1] )._bits[0] == 0x9 );
  CHECK( cuts.truth_table( cuts.cuts( i4 )[2] )._bits[0] == 0x0d );
  CHECK( cuts.truth_table( cuts.cuts( i4 )[3] )._bits[0] == 0x0d );
}

TEST_CASE( "compute XOR network cuts in 2-LUT network", "[cut_enumeration]" )
{
  klut_network klut;

  const auto a = klut.create_pi();
  const auto b = klut.create_pi();

  const auto g1 = klut.create_not( a );
  const auto g2 = klut.create_and( g1, b );
  const auto g3 = klut.create_not( b );
  const auto g4 = klut.create_and( a, g3 );

  kitty::dynamic_truth_table or_func( 2u );
  kitty::create_from_binary_string( or_func, "1110" );
  const auto g5 = klut.create_node( {g2, g4}, or_func );
  klut.create_po( g5 );

  cut_enumeration_params ps;
  const auto cuts = cut_enumeration<klut_network, true>( klut, ps );

  CHECK( cuts.cuts( klut.node_to_index( klut.get_node( g1 ) ) ).size() == 2u );
  CHECK( cuts.cuts( klut.node_to_index( klut.get_node( g3 ) ) ).size() == 2u );

  for ( auto const& cut : cuts.cuts( klut.node_to_index( klut.get_node( g1 ) ) ) ) {
    CHECK( cut->size() == 1u );
  }

  for ( auto const& cut : cuts.cuts( klut.node_to_index( klut.get_node( g3 ) ) ) ) {
    CHECK( cut->size() == 1u );
  }

  for ( auto const& cut : cuts.cuts( klut.node_to_index( klut.get_node( g5 ) ) ) ) {
    if ( cut->size() == 2u && *cut->begin() == 2u ) {
      CHECK( cuts.truth_table( *cut )._bits[0] == 0x6u );
    }
  }
}

TEST_CASE( "enumerate cuts for an AIG (small graph version)", "[fast_small_cut_enumeration]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( f1, a );
  const auto f3 = aig.create_nand( f1, b );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  auto cuts_optional = mockturtle::fast_small_cut_enumeration( aig );
  // This graph is smaller than 64 nodes so the cut enumeration algorithm should
  // produce a valid cut.
  CHECK( cuts_optional );

  auto cuts = *cuts_optional;

  const auto bitcut_to_vector = [] ( uint64_t bitcut ) {
    std::vector<uint32_t> v;

    for ( auto i = 0U; i < 64; i++ ) {
      if ( bitcut & ( static_cast<uint64_t>( 1 ) << i ) ) {
        v.push_back( i );
      }
    }

    return v;
  };

  // Check primary input cuts.
  aig.foreach_pi(
    [&] ( auto n ) {
      if ( aig.is_constant( n ) ) {
        return;
      }

      auto const n_idx = aig.node_to_index( n );
      auto const& cut_set = cuts.at( n_idx );

      for ( auto const& cut_set_i : cut_set ) {
        auto const& cut_node_indices = bitcut_to_vector( cut_set_i );
        // Primary inputs only have themselves as cuts.
        CHECK( cut_node_indices == std::vector<uint32_t>{ n_idx } );
      }
    }
  );

  // Check gate cuts.
  const auto i1 = aig.node_to_index( aig.get_node( f1 ) );
  const auto i2 = aig.node_to_index( aig.get_node( f2 ) );
  const auto i3 = aig.node_to_index( aig.get_node( f3 ) );
  const auto i4 = aig.node_to_index( aig.get_node( f4 ) );

  CHECK( cuts.at( i1 ).size() == 2 );
  CHECK( cuts.at( i2 ).size() == 3 );
  CHECK( cuts.at( i3 ).size() == 3 );
  CHECK( cuts.at( i4 ).size() == 5 );

  CHECK( bitcut_to_vector( cuts.at( i1 )[0] ) == std::vector<uint32_t>{ 1, 2 } );
  CHECK( bitcut_to_vector( cuts.at( i1 )[1] ) == std::vector<uint32_t>{ 3 } );

  CHECK( bitcut_to_vector( cuts.at( i2 )[0] ) == std::vector<uint32_t>{ 1, 2 } );
  CHECK( bitcut_to_vector( cuts.at( i2 )[1] ) == std::vector<uint32_t>{ 1, 3 } );
  CHECK( bitcut_to_vector( cuts.at( i2 )[2] ) == std::vector<uint32_t>{ 4 } );

  CHECK( bitcut_to_vector( cuts.at( i3 )[0] ) == std::vector<uint32_t>{ 1, 2 } );
  CHECK( bitcut_to_vector( cuts.at( i3 )[1] ) == std::vector<uint32_t>{ 2, 3 } );
  CHECK( bitcut_to_vector( cuts.at( i3 )[2] ) == std::vector<uint32_t>{ 5 } );

  CHECK( bitcut_to_vector( cuts.at( i4 )[0] ) == std::vector<uint32_t>{ 1, 2 } );
  CHECK( bitcut_to_vector( cuts.at( i4 )[1] ) == std::vector<uint32_t>{ 2, 3, 4 } );
  CHECK( bitcut_to_vector( cuts.at( i4 )[2] ) == std::vector<uint32_t>{ 1, 3, 5 } );
  CHECK( bitcut_to_vector( cuts.at( i4 )[3] ) == std::vector<uint32_t>{ 4, 5 } );
  CHECK( bitcut_to_vector( cuts.at( i4 )[4] ) == std::vector<uint32_t>{ 6 } );
}

TEST_CASE( "enumerate smaller cuts for an AIG (small graph version)", "[fast_small_cut_enumeration]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( f1, a );
  const auto f3 = aig.create_nand( f1, b );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  auto cuts_optional = mockturtle::fast_small_cut_enumeration( aig, 2 );
  // This graph is smaller than 64 nodes so the cut enumeration algorithm should
  // produce a valid cut.
  CHECK( cuts_optional );

  auto cuts = *cuts_optional;

  const auto bitcut_to_vector = [] ( uint64_t bitcut ) {
    std::vector<uint32_t> v;

    for ( auto i = 0U; i < 64; i++ ) {
      if ( bitcut & ( static_cast<uint64_t>( 1 ) << i ) ) {
        v.push_back( i );
      }
    }

    return v;
  };

  // Check primary input cuts.
  aig.foreach_pi(
    [&] ( auto n ) {
      if ( aig.is_constant( n ) ) {
        return;
      }

      auto const n_idx = aig.node_to_index( n );
      auto const& cut_set = cuts.at( n_idx );

      for ( auto const& cut_set_i : cut_set ) {
        auto const& cut_node_indices = bitcut_to_vector( cut_set_i );
        // Primary inputs only have themselves as cuts.
        CHECK( cut_node_indices == std::vector<uint32_t>{ n_idx } );
      }
    }
  );

  // Check gate cuts.
  const auto i1 = aig.node_to_index( aig.get_node( f1 ) );
  const auto i2 = aig.node_to_index( aig.get_node( f2 ) );
  const auto i3 = aig.node_to_index( aig.get_node( f3 ) );
  const auto i4 = aig.node_to_index( aig.get_node( f4 ) );

  CHECK( cuts.at( i1 ).size() == 2 );
  CHECK( cuts.at( i2 ).size() == 3 );
  CHECK( cuts.at( i3 ).size() == 3 );
  CHECK( cuts.at( i4 ).size() == 3 );

  CHECK( bitcut_to_vector( cuts.at( i1 )[0] ) == std::vector<uint32_t>{ 1, 2 } );
  CHECK( bitcut_to_vector( cuts.at( i1 )[1] ) == std::vector<uint32_t>{ 3 } );

  CHECK( bitcut_to_vector( cuts.at( i2 )[0] ) == std::vector<uint32_t>{ 1, 2 } );
  CHECK( bitcut_to_vector( cuts.at( i2 )[1] ) == std::vector<uint32_t>{ 1, 3 } );
  CHECK( bitcut_to_vector( cuts.at( i2 )[2] ) == std::vector<uint32_t>{ 4 } );

  CHECK( bitcut_to_vector( cuts.at( i3 )[0] ) == std::vector<uint32_t>{ 1, 2 } );
  CHECK( bitcut_to_vector( cuts.at( i3 )[1] ) == std::vector<uint32_t>{ 2, 3 } );
  CHECK( bitcut_to_vector( cuts.at( i3 )[2] ) == std::vector<uint32_t>{ 5 } );

  CHECK( bitcut_to_vector( cuts.at( i4 )[0] ) == std::vector<uint32_t>{ 1, 2 } );
  CHECK( bitcut_to_vector( cuts.at( i4 )[1] ) == std::vector<uint32_t>{ 4, 5 } );
  CHECK( bitcut_to_vector( cuts.at( i4 )[2] ) == std::vector<uint32_t>{ 6 } );
}
