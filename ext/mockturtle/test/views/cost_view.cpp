#include <catch.hpp>

#include <mockturtle/networks/xag.hpp>
#include <mockturtle/utils/recursive_cost_functions.hpp>
#include <mockturtle/views/cost_view.hpp>

using namespace mockturtle;

template<typename Ntk>
void test_cost_view()
{
  CHECK( is_network_type_v<Ntk> );
  CHECK( !has_cost_v<Ntk> );

  using cost_ntk = cost_view<Ntk, xag_size_cost_function<Ntk>>;

  CHECK( is_network_type_v<cost_ntk> );
  CHECK( has_cost_v<cost_ntk> );

  using cost_cost_ntk = cost_view<cost_ntk, xag_size_cost_function<cost_ntk>>;

  CHECK( is_network_type_v<cost_cost_ntk> );
  CHECK( has_cost_v<cost_cost_ntk> );
};

TEST_CASE( "create different cost views", "[cost_view]" )
{
  test_cost_view<xag_network>();
}

TEST_CASE( "compute size cost for xag network", "[cost_view]" )
{
  xag_network xag;
  const auto a = xag.create_pi();
  const auto b = xag.create_pi();
  const auto f1 = xag.create_and( a, b );
  const auto f2 = xag.create_and( a, f1 );
  const auto f3 = xag.create_and( b, f1 );
  const auto f4 = xag.create_and( f2, f3 );
  xag.create_po( f4 );

  cost_view cost_xag( xag, xag_size_cost_function<xag_network>() );
  CHECK( cost_xag.get_cost() == 4 );
  CHECK( cost_xag.get_cost( xag.get_node( a ) ) == 0 );
  CHECK( cost_xag.get_cost( xag.get_node( b ) ) == 0 );
  CHECK( cost_xag.get_cost( xag.get_node( f1 ) ) == 1 );
  CHECK( cost_xag.get_cost( xag.get_node( f2 ) ) == 2 );
  CHECK( cost_xag.get_cost( xag.get_node( f3 ) ) == 2 );
  CHECK( cost_xag.get_cost( xag.get_node( f4 ) ) == 4 );
}

TEST_CASE( "compute size cost for xag window", "[cost_view]" )
{
  xag_network xag;
  const auto a = xag.create_pi();
  const auto b = xag.create_pi();
  const auto f1 = xag.create_and( a, b );
  const auto f2 = xag.create_and( a, f1 );
  const auto f3 = xag.create_and( b, f1 );
  const auto f4 = xag.create_and( f2, f3 );
  xag.create_po( f4 );

  cost_view cost_xag( xag, xag_size_cost_function<xag_network>() );
  CHECK( cost_xag.get_cost() == 4 );
  CHECK( cost_xag.get_cost( xag.get_node( f1 ), std::vector( { a, b } ) ) == 1 );
  CHECK( cost_xag.get_cost( xag.get_node( f2 ), std::vector( { a, b } ) ) == 2 );
  CHECK( cost_xag.get_cost( xag.get_node( f3 ), std::vector( { a, b } ) ) == 2 );
  CHECK( cost_xag.get_cost( xag.get_node( f4 ), std::vector( { a, b } ) ) == 4 );
  CHECK( cost_xag.get_cost( xag.get_node( f1 ), std::vector( { f1, f2, f3 } ) ) == 0 );
  CHECK( cost_xag.get_cost( xag.get_node( f2 ), std::vector( { f1, f2, f3 } ) ) == 0 );
  CHECK( cost_xag.get_cost( xag.get_node( f3 ), std::vector( { f1, f2, f3 } ) ) == 0 );
  CHECK( cost_xag.get_cost( xag.get_node( f4 ), std::vector( { f1, f2, f3 } ) ) == 1 );
}

TEST_CASE( "compute depth cost for xag network", "[cost_view]" )
{
  xag_network xag;
  const auto a = xag.create_pi();
  const auto b = xag.create_pi();
  const auto f1 = xag.create_and( a, b );
  const auto f2 = xag.create_and( a, f1 );
  const auto f3 = xag.create_and( b, f1 );
  const auto f4 = xag.create_and( f2, f3 );
  xag.create_po( f4 );

  cost_view cost_xag( xag, xag_depth_cost_function<xag_network>() );
  CHECK( cost_xag.get_cost() == 3 );
  CHECK( cost_xag.get_cost( xag.get_node( a ) ) == 0 );
  CHECK( cost_xag.get_cost( xag.get_node( b ) ) == 0 );
  CHECK( cost_xag.get_cost( xag.get_node( f1 ) ) == 1 );
  CHECK( cost_xag.get_cost( xag.get_node( f2 ) ) == 2 );
  CHECK( cost_xag.get_cost( xag.get_node( f3 ) ) == 2 );
  CHECK( cost_xag.get_cost( xag.get_node( f4 ) ) == 3 );
}

TEST_CASE( "compute depth cost for xag window", "[cost_view]" )
{
  xag_network xag;
  const auto a = xag.create_pi();
  const auto b = xag.create_pi();
  const auto f1 = xag.create_and( a, b );
  const auto f2 = xag.create_and( a, f1 );
  const auto f3 = xag.create_and( b, f1 );
  const auto f4 = xag.create_and( f2, f3 );
  xag.create_po( f4 );

  cost_view cost_xag( xag, xag_depth_cost_function<xag_network>() );
  CHECK( cost_xag.get_cost() == 3 );
  CHECK( cost_xag.get_cost( xag.get_node( f1 ), std::vector( { a, b } ) ) == 1 );
  CHECK( cost_xag.get_cost( xag.get_node( f2 ), std::vector( { a, b } ) ) == 2 );
  CHECK( cost_xag.get_cost( xag.get_node( f3 ), std::vector( { a, b } ) ) == 2 );
  CHECK( cost_xag.get_cost( xag.get_node( f4 ), std::vector( { a, b } ) ) == 3 );
  CHECK( cost_xag.get_cost( xag.get_node( f1 ), std::vector( { f1, f2, f3 } ) ) == 1 );
  CHECK( cost_xag.get_cost( xag.get_node( f2 ), std::vector( { f1, f2, f3 } ) ) == 2 );
  CHECK( cost_xag.get_cost( xag.get_node( f3 ), std::vector( { f1, f2, f3 } ) ) == 2 );
  CHECK( cost_xag.get_cost( xag.get_node( f4 ), std::vector( { f1, f2, f3 } ) ) == 3 );
}
