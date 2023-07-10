#include <catch.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operations.hpp>
#include <kitty/print.hpp>

#include <mockturtle/algorithms/akers_synthesis.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xmg.hpp>

using namespace mockturtle;

TEST_CASE( "Check Akers for MAJ-3", "[akers_synthesis]" )
{
  std::vector<kitty::dynamic_truth_table> xs{ 5, kitty::dynamic_truth_table( 3 ) };

  create_majority( xs[0] );
  for ( auto i = 0u; i < unsigned( xs[0].num_bits() ); i++ )
  {
    set_bit( xs[1], i );
  }

  auto mig = akers_synthesis<mig_network>( xs[0], xs[1] );

  kitty::create_nth_var( xs[2], 0 );
  kitty::create_nth_var( xs[3], 1 );
  kitty::create_nth_var( xs[4], 2 );

  CHECK( mig.compute( mig.index_to_node( mig.size() - 1 ), xs.begin() + 2, xs.end() ) == xs[0] );
  CHECK( mig.size() == 5 );
}

TEST_CASE( "Check Akers for MAJ-5", "[akers_synthesis]" )
{
  std::vector<kitty::dynamic_truth_table> xs{ 7, kitty::dynamic_truth_table( 5 ) };

  create_majority( xs[0] );
  for ( auto i = 0u; i < unsigned( xs[0].num_bits() ); i++ )
  {
    set_bit( xs[1], i );
  }

  auto mig = akers_synthesis<mig_network>( xs[0], xs[1] );

  kitty::create_nth_var( xs[2], 0 );
  kitty::create_nth_var( xs[3], 1 );
  kitty::create_nth_var( xs[4], 2 );
  kitty::create_nth_var( xs[5], 3 );
  kitty::create_nth_var( xs[6], 4 );

  mig.foreach_gate( [&]( auto n ) {
    std::vector<kitty::dynamic_truth_table> fanin{ 3, kitty::dynamic_truth_table( 5 ) };
    mig.foreach_fanin( n, [&]( auto s, auto j ) {
      fanin[j] = xs[mig.node_to_index( mig.get_node( s ) ) + 1];
    } );
    xs.push_back( mig.compute( n, fanin.begin(), fanin.end() ) );
  } );
  CHECK( xs[xs.size() - 1] == xs[0] );
}

TEST_CASE( "Check Akers for MAJ-5 in XMG", "[akers_synthesis]" )
{
  std::vector<kitty::dynamic_truth_table> xs{ 7, kitty::dynamic_truth_table( 5 ) };

  create_majority( xs[0] );
  for ( auto i = 0u; i < unsigned( xs[0].num_bits() ); i++ )
  {
    set_bit( xs[1], i );
  }

  auto xmg = akers_synthesis<xmg_network>( xs[0], xs[1] );

  kitty::create_nth_var( xs[2], 0 );
  kitty::create_nth_var( xs[3], 1 );
  kitty::create_nth_var( xs[4], 2 );
  kitty::create_nth_var( xs[5], 3 );
  kitty::create_nth_var( xs[6], 4 );

  xmg.foreach_gate( [&]( auto n ) {
    std::vector<kitty::dynamic_truth_table> fanin{ 3, kitty::dynamic_truth_table( 5 ) };
    xmg.foreach_fanin( n, [&]( auto s, auto j ) {
      fanin[j] = xs[xmg.node_to_index( xmg.get_node( s ) ) + 1];
    } );
    xs.push_back( xmg.compute( n, fanin.begin(), fanin.end() ) );
  } );
  CHECK( xs[xs.size() - 1] == xs[0] );
}

TEST_CASE( "Check Akers for random - 4 inputs", "[akers_synthesis]" )
{
  std::array<std::string, 5> tts = { "d5d0", "fe52", "ad1b", "401a", "79e2" };

  for ( auto y = 0; y < 5; y++ )
  {
    std::vector<kitty::dynamic_truth_table> xs{ 6, kitty::dynamic_truth_table( 4 ) };
    kitty::create_nth_var( xs[2], 0 );
    kitty::create_nth_var( xs[3], 1 );
    kitty::create_nth_var( xs[4], 2 );
    kitty::create_nth_var( xs[5], 3 );

    create_from_hex_string( xs[0], tts[y] );

    for ( auto i = 0u; i < unsigned( xs[0].num_bits() ); i++ )
    {
      set_bit( xs[1], i );
    }

    auto mig = akers_synthesis<mig_network>( xs[0], xs[1] );
    if ( mig.size() > 4 )
    {
      mig.foreach_gate( [&]( auto n ) {
        std::vector<kitty::dynamic_truth_table> fanin{ 3, kitty::dynamic_truth_table( 4 ) };
        mig.foreach_fanin( n, [&]( auto s, auto j ) {
          if ( mig.node_to_index( mig.get_node( s ) ) == 0 )
          {
            fanin[j] = ~xs[1];
          }
          else
          {
            fanin[j] = xs[mig.node_to_index( mig.get_node( s ) ) + 1];
          }
        } );
        xs.push_back( mig.compute( n, fanin.begin(), fanin.end() ) );
      } );

      mig.foreach_po( [&]( auto n ) {
        if ( mig.is_complemented( n ) )
          CHECK( ~xs[xs.size() - 1] == xs[0] );
        else
          CHECK( xs[xs.size() - 1] == xs[0] );
      } );
    }
  }
}

TEST_CASE( "Check Akers for random - 5 inputs", "[akers_synthesis]" )
{
  std::array<std::string, 5> tts = { "e3cee67b", "bb5bee39", "b220ff4c", "fa43751f", "9ec83bf4" };

  for ( auto y = 0; y < 5; y++ )
  {
    std::vector<kitty::dynamic_truth_table> xs{ 7, kitty::dynamic_truth_table( 5 ) };
    kitty::create_nth_var( xs[2], 0 );
    kitty::create_nth_var( xs[3], 1 );
    kitty::create_nth_var( xs[4], 2 );
    kitty::create_nth_var( xs[5], 3 );
    kitty::create_nth_var( xs[6], 4 );

    create_from_hex_string( xs[0], tts[y] );

    for ( auto i = 0u; i < unsigned( xs[0].num_bits() ); i++ )
    {
      set_bit( xs[1], i );
    }

    auto mig = akers_synthesis<mig_network>( xs[0], xs[1] );
    if ( mig.size() > 6 )
    {
      mig.foreach_gate( [&]( auto n ) {
        std::vector<kitty::dynamic_truth_table> fanin{ 3, kitty::dynamic_truth_table( 5 ) };
        mig.foreach_fanin( n, [&]( auto s, auto j ) {
          if ( mig.node_to_index( mig.get_node( s ) ) == 0 )
          {
            fanin[j] = ~xs[1];
          }
          else
          {
            fanin[j] = xs[mig.node_to_index( mig.get_node( s ) ) + 1];
          }
        } );
        xs.push_back( mig.compute( n, fanin.begin(), fanin.end() ) );
      } );

      mig.foreach_po( [&]( auto n ) {
        if ( mig.is_complemented( n ) )
          CHECK( ~xs[xs.size() - 1] == xs[0] );
        else
          CHECK( xs[xs.size() - 1] == xs[0] );
      } );
    }
  }
}

TEST_CASE( "Check Akers for random - 6 inputs", "[akers_synthesis]" )
{
  for ( auto y = 0; y < 1; y++ )
  {
    std::vector<kitty::dynamic_truth_table> xs{ 8, kitty::dynamic_truth_table( 6 ) };
    kitty::create_nth_var( xs[2], 0 );
    kitty::create_nth_var( xs[3], 1 );
    kitty::create_nth_var( xs[4], 2 );
    kitty::create_nth_var( xs[5], 3 );
    kitty::create_nth_var( xs[6], 4 );
    kitty::create_nth_var( xs[7], 5 );

    create_from_hex_string( xs[0], "32b43db39dde2b16" );

    for ( auto i = 0u; i < unsigned( xs[0].num_bits() ); i++ )
    {
      set_bit( xs[1], i );
    }

    auto mig = akers_synthesis<mig_network>( xs[0], xs[1] );
    if ( mig.size() > 6 )
    {
      mig.foreach_gate( [&]( auto n ) {
        std::vector<kitty::dynamic_truth_table> fanin{ 3, kitty::dynamic_truth_table( 6 ) };
        mig.foreach_fanin( n, [&]( auto s, auto j ) {
          if ( mig.node_to_index( mig.get_node( s ) ) == 0 )
          {
            fanin[j] = ~xs[1];
          }
          else
          {
            fanin[j] = xs[mig.node_to_index( mig.get_node( s ) ) + 1];
          }
        } );
        xs.push_back( mig.compute( n, fanin.begin(), fanin.end() ) );
      } );
      mig.foreach_po( [&]( auto n ) {
        if ( mig.is_complemented( n ) )
          CHECK( ~xs[xs.size() - 1] == xs[0] );
        else
          CHECK( xs[xs.size() - 1] == xs[0] );
      } );
    }
  }
}

TEST_CASE( "Check leaves iterator -- easy case ", "[akers_synthesis]" )
{
  mig_network mig;
  auto a = mig.create_pi();
  auto b = mig.create_pi();
  auto c = mig.create_pi();
  auto d = mig.create_pi();

  std::vector<mig_network::signal> operations;
  operations.push_back( mig.create_and( a, b ) );
  operations.push_back( mig.create_and( c, d ) );

  std::vector<kitty::dynamic_truth_table> xs_in{ 2, kitty::dynamic_truth_table( 2 ) };
  std::vector<kitty::dynamic_truth_table> xs{ 5, kitty::dynamic_truth_table( 4 ) };
  create_from_binary_string( xs_in[0], "0110" );
  for ( auto i = 0u; i < unsigned( xs_in[0].num_bits() ); i++ )
  {
    set_bit( xs_in[1], i );
  }
  auto t = akers_synthesis( mig, xs_in[0], xs_in[1], operations.begin(), operations.end() );
  mig.create_po( t );

  kitty::create_nth_var( xs[1], 0 );
  kitty::create_nth_var( xs[2], 1 );
  kitty::create_nth_var( xs[3], 2 );
  kitty::create_nth_var( xs[4], 3 );

  for ( auto i = 0u; i < unsigned( xs[1].num_bits() ); i++ )
  {
    set_bit( xs[0], i );
  }

  CHECK( mig.num_gates() == 5 );

  if ( mig.size() > 6 )
  {
    mig.foreach_gate( [&]( auto n ) {
      std::vector<kitty::dynamic_truth_table> fanin{ 3, kitty::dynamic_truth_table( 4 ) };
      mig.foreach_fanin( n, [&]( auto s, auto j ) {
        if ( mig.node_to_index( mig.get_node( s ) ) == 0 )
        {
          fanin[j] = ~xs[0];
        }
        else
        {
          fanin[j] = xs[mig.get_node( s )];
        }
      } );
      xs.push_back( mig.compute( n, fanin.begin(), fanin.end() ) );
    } );
    mig.foreach_po( [&]( auto n ) {
      if ( mig.is_complemented( n ) )
        CHECK( ~xs[xs.size() - 1] == binary_xor( binary_and( xs[1], xs[2] ), binary_and( xs[4], xs[3] ) ) );
      else
        CHECK( xs[xs.size() - 1] == binary_xor( binary_and( xs[1], xs[2] ), binary_and( xs[4], xs[3] ) ) );
    } );
  }
}
