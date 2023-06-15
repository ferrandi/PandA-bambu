#include <catch.hpp>

#include <algorithm>
#include <sstream>
#include <vector>

#include <lorina/blif.hpp>
#include <mockturtle/views/names_view.hpp>
#include <mockturtle/io/write_blif.hpp>
#include <mockturtle/io/blif_reader.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/sequential.hpp>

using namespace mockturtle;

template<class Ntk>
void blif_read_after_write_test( const Ntk& written_ntk, const write_blif_params& ps = {} )
{
  std::ostringstream out;

  write_blif( written_ntk, out, ps );

  Ntk read_ntk;
  std::istringstream in( out.str() );
  auto const ret = lorina::read_blif( in, blif_reader( read_ntk ) );

  CHECK( ret == lorina::return_code::success );

  CHECK( read_ntk.num_pis() == written_ntk.num_pis() );
  CHECK( read_ntk.num_pos() == written_ntk.num_pos() );

  if constexpr ( has_num_registers_v<Ntk> )
  {
    CHECK( read_ntk.num_registers() == written_ntk.num_registers() );  
  }

  /* check the names of the inputs and outputs */
  if constexpr ( has_get_name_v<Ntk> )
  {
    for ( auto i = 0u; i < read_ntk.num_pis(); ++i )
    {
      if ( read_ntk.has_name( read_ntk.pi_at(i) ) && written_ntk.has_name( read_ntk.pi_at(i) ) )
      {
        auto const read_pi_name = read_ntk.get_name( read_ntk.pi_at( i ) );
        auto const write_pi_name = written_ntk.get_name( written_ntk.pi_at( i ) );
        CHECK( read_pi_name == write_pi_name );
      }
    }

    for ( auto i = 0u; i < read_ntk.num_pos(); ++i )
    {
      if ( read_ntk.has_output_name( i ) && written_ntk.has_output_name( i ) )
      {
        auto const read_po_name = read_ntk.get_output_name( i );
        auto const write_po_name = written_ntk.get_output_name( i );
        CHECK( read_po_name == write_po_name );
      }
    }
  }
}

TEST_CASE( "write a simple combinational k-LUT into BLIF file", "[write_blif]" )
{
  klut_network klut;

  const auto a = klut.create_pi();
  const auto b = klut.create_pi();

  const auto f1 = klut.create_or( a, b );
  klut.create_po( f1 );

  CHECK( klut.num_gates() == 1 );
  CHECK( klut.num_pis() == 2 );
  CHECK( klut.num_pos() == 1 );

  std::ostringstream out;
  write_blif( klut, out );

  /* additional spaces at the end of .inputs and .outputs */
  CHECK( out.str() == ".model top\n"
                      ".inputs pi2 pi3 \n"
                      ".outputs po0 \n"
                      ".names new_n0\n"
                      "0\n"
                      ".names new_n1\n"
                      "1\n"
                      ".names pi2 pi3 new_n4\n"
                      "-1 1\n"
                      "1- 1\n"
                      ".names new_n4 po0\n"
                      "1 1\n"
                      ".end\n" );

  write_blif_params ps;
  ps.rename_ri_using_node = true;
  blif_read_after_write_test( klut, ps );
  ps.rename_ri_using_node = false;
  blif_read_after_write_test( klut, ps );
}

TEST_CASE( "write a k-LUT with dual outputs node into BLIF file", "[write_blif]" )
{
  klut_network klut;

  const auto a = klut.create_pi();
  const auto b = klut.create_pi();

  const auto f1 = klut.create_or( a, b );
  klut.create_po( f1 );
  klut.create_po( f1 );

  CHECK( klut.num_gates() == 1 );
  CHECK( klut.num_pis() == 2 );
  CHECK( klut.num_pos() == 2 );

  std::ostringstream out;
  write_blif( klut, out );

  /* additional spaces at the end of .inputs and .outputs */
  CHECK( out.str() == ".model top\n"
                      ".inputs pi2 pi3 \n"
                      ".outputs po0 po1 \n"
                      ".names new_n0\n"
                      "0\n"
                      ".names new_n1\n"
                      "1\n"
                      ".names pi2 pi3 new_n4\n"
                      "-1 1\n"
                      "1- 1\n"
                      ".names new_n4 po0\n"
                      "1 1\n"
                      ".names new_n4 po1\n"
                      "1 1\n"
                      ".end\n" );

  write_blif_params ps;
  ps.rename_ri_using_node = true;
  blif_read_after_write_test( klut, ps );
  ps.rename_ri_using_node = false;
  blif_read_after_write_test( klut, ps );
}

TEST_CASE( "write a k-LUT with name view into BLIF file", "[write_blif]" )
{
  names_view<klut_network> klut;

  const auto a = klut.create_pi( "a" );
  const auto b = klut.create_pi( "b" );

  const auto f1 = klut.create_or( a, b );
  klut.create_po( f1, "output1" );
  klut.create_po( f1, "output2" );

  klut.set_name( f1, "f1" );

  CHECK( klut.num_gates() == 1 );
  CHECK( klut.num_pis() == 2 );
  CHECK( klut.num_pos() == 2 );

  std::ostringstream out;
  write_blif( klut, out );

  /* additional spaces at the end of .inputs and .outputs */
  CHECK( out.str() == ".model top\n"
                      ".inputs a b \n"
                      ".outputs output1 output2 \n"
                      ".names new_n0\n"
                      "0\n"
                      ".names new_n1\n"
                      "1\n"
                      ".names a b f1\n"
                      "-1 1\n"
                      "1- 1\n"
                      ".names f1 output1\n"
                      "1 1\n"
                      ".names f1 output2\n"
                      "1 1\n"
                      ".end\n");

  write_blif_params ps;
  ps.rename_ri_using_node = true;
  blif_read_after_write_test( klut, ps );
  ps.rename_ri_using_node = false;
  blif_read_after_write_test( klut, ps );
}


TEST_CASE( "write a simple combinational k-LUT with multiple POs assigned by the same signal into BLIF file", "[write_blif]" )
{
  klut_network klut;

  const auto a = klut.create_pi();
  const auto b = klut.create_pi();

  const auto f1 = klut.create_or( a, b );
  klut.create_po( f1 );
  klut.create_po( f1 );

  CHECK( klut.num_gates() == 1 );
  CHECK( klut.num_pis() == 2 );
  CHECK( klut.num_pos() == 2 );

  write_blif_params ps;
  ps.rename_ri_using_node = true;
  blif_read_after_write_test( klut, ps );
  ps.rename_ri_using_node = false;
  blif_read_after_write_test( klut, ps );
}


TEST_CASE( "write a simple sequential k-LUT into BLIF file", "[write_blif]" )
{
  sequential<klut_network> klut;

  const auto a = klut.create_pi();
  const auto b = klut.create_pi();
  const auto c = klut.create_pi();

  const auto f1 = klut.create_or( a, b );
  const auto f2 = klut.create_ro(); // f2 <- f1
  const auto f3 = klut.create_or( f2, c );

  klut.create_po( f3 );
  klut.create_ri( f1 ); // f2 <- f1

  CHECK( klut.num_gates() == 2 );
  CHECK( klut.num_registers() == 1 );
  CHECK( klut.num_pis() == 3 );
  CHECK( klut.num_pos() == 1 );

  std::ostringstream out;
  write_blif( klut, out );

  /* additional spaces at the end of .inputs and .outputs */
  CHECK( out.str() == ".model top\n"
                      ".inputs pi2 pi3 pi4 \n"
                      ".outputs po0 \n"
                      ".latch li0 new_n6   3\n"
                      ".names new_n0\n"
                      "0\n"
                      ".names new_n1\n"
                      "1\n"
                      ".names new_n6 pi4 new_n7\n"
                      "-1 1\n"
                      "1- 1\n"
                      ".names pi2 pi3 new_n5\n"
                      "-1 1\n"
                      "1- 1\n"
                      ".names new_n7 po0\n"
                      "1 1\n"
                      ".names new_n5 li0\n"
                      "1 1\n"
                      ".end\n" );

  write_blif_params ps;
  ps.rename_ri_using_node = true;
  blif_read_after_write_test( klut, ps );
  ps.rename_ri_using_node = false;
  blif_read_after_write_test( klut, ps );
}

TEST_CASE( "write a sequential k-LUT with node being both RI and PO into BLIF file", "[write_blif]" )
{
  sequential<klut_network> klut;

  const auto a = klut.create_pi();
  const auto b = klut.create_pi();
  const auto c = klut.create_pi();

  const auto f1 = klut.create_or( a, b );
  const auto f2 = klut.create_ro(); // f2 <- f1
  const auto f3 = klut.create_or( f2, c );

  klut.create_po( f3 );
  klut.create_po( f1 );
  klut.create_ri( f1 ); // f2 <- f1

  CHECK( klut.num_gates() == 2 );
  CHECK( klut.num_registers() == 1 );
  CHECK( klut.num_pis() == 3 );
  CHECK( klut.num_pos() == 2 );

  std::ostringstream out;
  write_blif( klut, out );

  /* additional spaces at the end of .inputs and .outputs */
  CHECK( out.str() == ".model top\n"
                      ".inputs pi2 pi3 pi4 \n"
                      ".outputs po0 po1 \n"
                      ".latch li0 new_n6   3\n"
                      ".names new_n0\n"
                      "0\n"
                      ".names new_n1\n"
                      "1\n"
                      ".names new_n6 pi4 new_n7\n"
                      "-1 1\n"
                      "1- 1\n"
                      ".names pi2 pi3 new_n5\n"
                      "-1 1\n"
                      "1- 1\n"
                      ".names new_n7 po0\n"
                      "1 1\n"
                      ".names new_n5 po1\n"
                      "1 1\n"
                      ".names new_n5 li0\n"
                      "1 1\n"
                      ".end\n" );

  write_blif_params ps;
  ps.rename_ri_using_node = true;
  blif_read_after_write_test( klut, ps );
  ps.rename_ri_using_node = false;
  blif_read_after_write_test( klut, ps );
}

TEST_CASE( "write a sequential k-LUT with node being both RI and PO, and with name_view, into BLIF file", "[write_blif]" )
{
  names_view<sequential<klut_network>> klut;

  const auto a = klut.create_pi( "a" );
  const auto b = klut.create_pi( "b" );
  const auto c = klut.create_pi( "c" );

  const auto f1 = klut.create_or( a, b );
  const auto f2 = klut.create_ro(); // f2 <- f1
  const auto f3 = klut.create_or( f2, c );

  klut.set_name( f1, "f1" );
  klut.set_name( f2, "f2" );
  klut.set_name( f3, "f3" );

  klut.create_po( f3 );
  klut.create_po( f1 );
  klut.create_ri( f1 ); // f2 <- f1

  CHECK( klut.num_gates() == 2 );
  CHECK( klut.num_registers() == 1 );
  CHECK( klut.num_pis() == 3 );
  CHECK( klut.num_pos() == 2 );

  std::ostringstream out;
  write_blif( klut, out );

  /* additional spaces at the end of .inputs and .outputs */
  CHECK( out.str() == ".model top\n"
                      ".inputs a b c \n"
                      ".outputs po0 po1 \n"
                      ".latch li0 f2   3\n"
                      ".names new_n0\n"
                      "0\n"
                      ".names new_n1\n"
                      "1\n"
                      ".names f2 c f3\n"
                      "-1 1\n"
                      "1- 1\n"
                      ".names a b f1\n"
                      "-1 1\n"
                      "1- 1\n"
                      ".names f3 po0\n"
                      "1 1\n"
                      ".names f1 po1\n"
                      "1 1\n"
                      ".names f1 li0\n"
                      "1 1\n"
                      ".end\n");

  write_blif_params ps;
  ps.rename_ri_using_node = true;
  blif_read_after_write_test( klut, ps );
  ps.rename_ri_using_node = false;
  blif_read_after_write_test( klut, ps );
}

TEST_CASE( "write a sequential k-LUT with multiple fanout registers into BLIF file", "[write_blif]" )
{
  sequential<klut_network> klut;

  const auto a = klut.create_pi();
  const auto b = klut.create_pi();
  const auto c = klut.create_pi();

  const auto f1 = klut.create_maj( a, b, c );
  const auto f2 = klut.create_ro(); // f2 <- f1
  const auto f3 = klut.create_ro(); // f3 <- f1
  const auto f4 = klut.create_xor( f2, f3 );

  klut.create_po( f4 );
  klut.create_ri( f1 ); // f2 <- f1
  klut.create_ri( f1 ); // f3 <- f1

  CHECK( klut.num_gates() == 2 );
  CHECK( klut.num_registers() == 2 );
  CHECK( klut.num_pis() == 3 );
  CHECK( klut.num_pos() == 1 );

  std::ostringstream out;
  write_blif( klut, out );

  /* additional spaces at the end of .inputs and .outputs */
  CHECK( out.str() == ".model top\n"
                      ".inputs pi2 pi3 pi4 \n"
                      ".outputs po0 \n"
                      ".latch li0 new_n6   3\n"
                      ".latch li1 new_n7   3\n"
                      ".names new_n0\n"
                      "0\n"
                      ".names new_n1\n"
                      "1\n"
                      ".names new_n6 new_n7 new_n8\n"
                      "10 1\n"
                      "01 1\n"
                      ".names pi2 pi3 pi4 new_n5\n"
                      "-11 1\n"
                      "1-1 1\n"
                      "11- 1\n"
                      ".names new_n8 po0\n"
                      "1 1\n"
                      ".names new_n5 li0\n"
                      "1 1\n"
                      ".names new_n5 li1\n"
                      "1 1\n"
                      ".end\n" );

  write_blif_params ps;
  ps.rename_ri_using_node = true;
  blif_read_after_write_test( klut, ps );
  ps.rename_ri_using_node = false;
  blif_read_after_write_test( klut, ps );
}

TEST_CASE( "write a sequential k-LUT with name view", "[write_blif]" )
{
  names_view<sequential<klut_network>> klut;

  const auto a = klut.create_pi( "a" );
  const auto b = klut.create_pi( "b" );
  const auto c = klut.create_pi( "c" );

  const auto f1 = klut.create_maj( a, b, c );
  const auto f2 = klut.create_ro(); // f2 <- f1
  const auto f3 = klut.create_ro(); // f3 <- f1
  const auto f4 = klut.create_xor( f2, f3 );
  
  klut.set_name( f1, "f1" );
  klut.set_name( f2, "f2" );
  klut.set_name( f3, "f3" );
  klut.set_name( f4, "f4" );

  klut.create_po( f4, "output" );
  klut.create_ri( f1 ); // f2 <- f1
  klut.create_ri( f1 ); // f3 <- f1

  CHECK( klut.num_gates() == 2 );
  CHECK( klut.num_registers() == 2 );
  CHECK( klut.num_pis() == 3 );
  CHECK( klut.num_pos() == 1 );

  std::ostringstream out;
  write_blif( klut, out );

  /* additional spaces at the end of .inputs and .outputs */
  CHECK( out.str() == ".model top\n"
                      ".inputs a b c \n"
                      ".outputs output \n"
                      ".latch li0 f2   3\n"
                      ".latch li1 f3   3\n"
                      ".names new_n0\n"
                      "0\n"
                      ".names new_n1\n"
                      "1\n"
                      ".names f2 f3 f4\n"
                      "10 1\n"
                      "01 1\n"
                      ".names a b c f1\n"
                      "-11 1\n"
                      "1-1 1\n"
                      "11- 1\n"
                      ".names f4 output\n"
                      "1 1\n"
                      ".names f1 li0\n"
                      "1 1\n"
                      ".names f1 li1\n"
                      "1 1\n"
                      ".end\n" );

  write_blif_params ps;
  ps.rename_ri_using_node = true;
  blif_read_after_write_test( klut, ps );
  ps.rename_ri_using_node = false;
  blif_read_after_write_test( klut, ps );
}

TEST_CASE( "write a sequential k-LUT with name view and rename ris", "[write_blif]" )
{
  names_view<sequential<klut_network>> klut;

  const auto a = klut.create_pi( "a" );
  const auto b = klut.create_pi( "b" );
  const auto c = klut.create_pi( "c" );

  const auto f1 = klut.create_maj( a, b, c );
  const auto f2 = klut.create_ro(); // f2 <- f1
  const auto f3 = klut.create_ro(); // f3 <- f1
  const auto f4 = klut.create_xor( f2, f3 );
  
  klut.set_name( f1, "maj(a,b,c)" );
  klut.set_name( f2, "dff1" );
  klut.set_name( f3, "dff2" );
  klut.set_name( f4, "xor(dff1,dff2)" );

  klut.create_po( f4, "output" );
  klut.create_ri( f1 ); // f2 <- f1
  klut.create_ri( f1 ); // f3 <- f1

  CHECK( klut.num_gates() == 2 );
  CHECK( klut.num_registers() == 2 );
  CHECK( klut.num_pis() == 3 );
  CHECK( klut.num_pos() == 1 );

  std::ostringstream out;
  write_blif_params ps;
  ps.rename_ri_using_node = true;
  write_blif( klut, out, ps );

  /* additional spaces at the end of .inputs and .outputs */
  CHECK( out.str() == ".model top\n"
                      ".inputs a b c \n"
                      ".outputs output \n"
                      ".latch maj(a,b,c) dff1   3\n"
                      ".latch maj(a,b,c) dff2   3\n"
                      ".names new_n0\n"
                      "0\n"
                      ".names new_n1\n"
                      "1\n"
                      ".names dff1 dff2 xor(dff1,dff2)\n"
                      "10 1\n"
                      "01 1\n"
                      ".names a b c maj(a,b,c)\n"
                      "-11 1\n"
                      "1-1 1\n"
                      "11- 1\n"
                      ".names xor(dff1,dff2) output\n"
                      "1 1\n"
                      ".end\n" );

  ps.rename_ri_using_node = true;
  blif_read_after_write_test( klut, ps );
  ps.rename_ri_using_node = false;
  blif_read_after_write_test( klut, ps );
}