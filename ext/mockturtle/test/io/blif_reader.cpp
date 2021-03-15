#include <catch.hpp>

#include <sstream>
#include <string>

#include <mockturtle/io/blif_reader.hpp>
#include <mockturtle/io/write_blif.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/algorithms/node_resynthesis.hpp>
#include <mockturtle/algorithms/node_resynthesis/mig_npn.hpp>

#include <kitty/kitty.hpp>
#include <lorina/blif.hpp>

using namespace mockturtle;

TEST_CASE( "read a combinational BLIF file into KLUT network", "[blif_reader]" )
{
  klut_network klut;

  std::string file{
    ".model top\n"
    ".inputs a b c\n"
    ".outputs y1 y2\n"
    ".names a b n1\n"
    "11 1\n"
    ".names c n1 n2\n"
    "1- 1\n"
    "-1 1\n"
    ".names n2 y1\n"
    "0 1\n"
    ".names n2 y2\n"
    "1 1\n"
    ".end\n"};

  std::istringstream in( file );
  auto result = lorina::read_blif( in, blif_reader( klut ) );

  /* structural checks */
  CHECK( result == lorina::return_code::success );
  CHECK( klut.size() == 9 );
  CHECK( klut.num_pis() == 3 );
  CHECK( klut.num_pos() == 2 );
  CHECK( klut.num_gates() == 4 );

  /* functional checks */
  default_simulator<kitty::dynamic_truth_table> sim( klut.num_pis() );
  const auto tts = simulate<kitty::dynamic_truth_table>( klut, sim );
  klut.foreach_po( [&]( auto const&, auto i ) {
    switch ( i )
    {
    case 0:
      CHECK( kitty::to_hex( tts[i] ) == "07" );
      break;
    case 1:
      CHECK( kitty::to_hex( tts[i] ) == "f8" );
      break;
    }
    } );
}

TEST_CASE( "read a sequential BLIF file with 5 parameter latches that is not in topological order", "[blif_reader]" )
{
  klut_network klut;

  std::string file{
    ".model top\n"
    ".inputs clock a b c d\n"
    ".outputs f\n"
    ".names li1 a li0\n"
    "01 1\n"
    ".names lo0 new_new_n19__ li1\n"
    "01 1\n"
    ".names a lo1 new_new_n15__\n"
    "01 1\n"
    ".names d new_new_n15__ new_new_n16__\n"
    "00 1\n"
    ".names b lo2 new_new_n17__\n"
    "00 1\n"
    ".names new_new_n15__ new_new_n17__ new_new_n18__\n"
    "00 1\n"
    ".names new_new_n16__ new_new_n18__ new_new_n19__\n"
    "00 1\n"
    ".names new_new_n19__ new_new_n17__ li3\n"
    "00 1\n"
    ".names lo3 lo4 new_new_n20__\n"
    "00 1\n"
    ".names new_new_n20__ new_new_n18__ li4\n"
    "00 1\n"
    ".names c new_new_n17__ li2\n"
    "00 1\n"
    ".names li1 li4 f\n"
    "00 1\n"
    ".latch li0 lo0 fe clock 0\n"
    ".latch li1 lo1 re clock 1\n"
    ".latch li2 lo2 ah clock 2\n"
    ".latch li3 lo3 al clock 3\n"
    ".latch li4 lo4 as clock 1\n"
    ".end\n"};

  std::istringstream in( file );
  const auto result = lorina::read_blif( in, blif_reader( klut ) );

  /* structural checks */
  CHECK( result == lorina::return_code::success );
  CHECK( klut.num_pis() == 5 );
  CHECK( klut.num_pos() == 1 );
  CHECK( klut.num_latches() == 5 );
  CHECK( klut.num_gates() == 12 );

  klut.foreach_ro([&](auto ro, auto i){
    latch_info l_info = klut._storage->latch_information[ro];
    switch ( i )
    {
    case 0:
      CHECK( l_info.type == "fe" );
      CHECK( l_info.control == "clock" );
      CHECK( l_info.init == 0 );
      break;
    case 1:
      CHECK( l_info.type == "re" );
      CHECK( l_info.control == "clock" );
      CHECK( l_info.init == 1 );
      break;
    case 2:
      CHECK( l_info.type == "ah" );
      CHECK( l_info.control == "clock" );
      CHECK( l_info.init == 2 );
      break;
    case 3:
      CHECK( l_info.type == "al" );
      CHECK( l_info.control == "clock" );
      CHECK( l_info.init == 3 );
      break;
    case 4:
      CHECK( l_info.type == "as" );
      CHECK( l_info.control == "clock" );
      CHECK( l_info.init == 1 );
      break;
    }
  });

  mig_npn_resynthesis resyn;
  mig_network mig;
  node_resynthesis( mig, klut, resyn );
  CHECK( mig.num_pis() == 5 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_latches() == 5 );
  CHECK( mig.num_gates() == 12 );

  mig.foreach_ro([&](auto ro, auto i){
    latch_info l_info = mig._storage->latch_information[ro];
    switch ( i )
    {
    case 0:
      CHECK( l_info.type == "fe" );
      CHECK( l_info.control == "clock" );
      CHECK( l_info.init == 0 );
      break;
    case 1:
      CHECK( l_info.type == "re" );
      CHECK( l_info.control == "clock" );
      CHECK( l_info.init == 1 );
      break;
    case 2:
      CHECK( l_info.type == "ah" );
      CHECK( l_info.control == "clock" );
      CHECK( l_info.init == 2 );
      break;
    case 3:
      CHECK( l_info.type == "al" );
      CHECK( l_info.control == "clock" );
      CHECK( l_info.init == 3 );
      break;
    case 4:
      CHECK( l_info.type == "as" );
      CHECK( l_info.control == "clock" );
      CHECK( l_info.init == 1 );
      break;
    }
  });
}

TEST_CASE( "read a BLIF file containing latch declaration bug that requires updated 'split' function", "[blif_reader]" )
{
  klut_network klut;

  std::string file{
    ".model top\n"
    ".inputs clock a b c d\n"
    ".outputs f\n"
    ".latch     lo0_in        lo0  1\n"
    ".latch     lo1_in        lo1  1\n"
    ".latch     lo2_in        lo2  1\n"
    ".names a lo1 new_n16_\n"
    "01 1\n"
    ".names d new_n16_ new_n17_\n"
    "00 1\n"
    ".names b lo2 new_n18_\n"
    "00 1\n"
    ".names new_n16_ new_n18_ new_n19_\n"
    "00 1\n"
    ".names new_n17_ new_n19_ new_n20_\n"
    "00 1\n"
    ".names lo0 new_n20_ lo1_in\n"
    "01 1\n"
    ".names a lo1_in lo0_in\n"
    "10 1\n"
    ".names c new_n18_ lo2_in\n"
    "00 1\n"
    ".names lo1_in f\n"
    "0 1\n"
    ".end\n"};

  std::istringstream in( file );
  const auto result = lorina::read_blif( in, blif_reader( klut ) );

  /* structural checks */
  CHECK( result == lorina::return_code::success );
  CHECK( klut.num_pis() == 5 );
  CHECK( klut.num_pos() == 1 );
  CHECK( klut.num_latches() == 3 );
  CHECK( klut.num_gates() == 9 );
}

TEST_CASE( "read a combinational BLIF file with max terms", "[blif_reader]" )
{
  klut_network klut, klut2;

  std::string file{
    ".model top\n"
    ".inputs a b c\n"
    ".outputs f g\n"
    ".names a b c f\n"
    "11- 1\n"
    "0-1 1\n"
    ".names a b c g\n"
    "0-0 0\n"
    "10- 0\n"
    ".end\n"};

  std::istringstream in( file );
  auto result = lorina::read_blif( in, blif_reader( klut ) );
  CHECK( result == lorina::return_code::success );

  /* structural checks */
  CHECK( klut.size() == 6 );
  CHECK( klut.num_pis() == 3 );
  CHECK( klut.num_pos() == 2 );
  CHECK( klut.num_gates() == 1 );

  std::stringstream out;
  write_blif( klut, out );
  result = lorina::read_blif( out, blif_reader( klut2 ) );
  CHECK( result == lorina::return_code::success );

  /* structural checks */
  CHECK( klut2.size() == 7 );
  CHECK( klut2.num_pis() == 3 );
  CHECK( klut2.num_pos() == 2 );
  CHECK( klut2.num_gates() == 2 );

  /* functional checks */
  default_simulator<kitty::dynamic_truth_table> sim( klut.num_pis() );
  const auto tts = simulate<kitty::dynamic_truth_table>( klut, sim );
  CHECK( kitty::to_hex( tts[0] ) == "d8" );
  CHECK( kitty::to_hex( tts[1] ) == "d8" );
  CHECK( tts == simulate<kitty::dynamic_truth_table>( klut2, sim ) );
}
