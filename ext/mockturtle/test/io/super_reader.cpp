#include <catch.hpp>

#include <lorina/super.hpp>
#include <mockturtle/io/super_reader.hpp>

#include <sstream>
#include <string>

TEST_CASE( "read super file", "[super_reader]" )
{
  std::string const file{
      "test.genlib\n"
      "3\n"
      "8\n"
      "14\n"
      "* and 1 0\n"
      "* and 2 3\n"
      "and 2 0\n"
      "* and 1 5\n"
      "or 2 1\n"
      "* and 0 7\n"
      "* or 1 0\n"
      "* and 0 9\n"
      "or 2 0\n"
      "* and 1 11\n"
      "* and 2 9\n"
      "\0" };

  mockturtle::super_lib super_data;

  std::istringstream in( file );
  auto const result = lorina::read_super( in, mockturtle::super_reader( super_data ) );
  CHECK( result == lorina::return_code::success );

  CHECK( super_data.genlib_name == "test.genlib" );
  CHECK( super_data.max_num_vars == 3 );
  CHECK( super_data.num_supergates == 8 );
  CHECK( super_data.num_lines == 14 );
  CHECK( super_data.supergates.size() == 11 );

  CHECK( super_data.supergates[0].id == 0 );
  CHECK( super_data.supergates[0].name == "and" );
  CHECK( super_data.supergates[0].is_super == true );
  CHECK( super_data.supergates[0].fanin_id.size() == 2 );
  CHECK( super_data.supergates[0].fanin_id[0] == 1 );
  CHECK( super_data.supergates[0].fanin_id[1] == 0 );

  CHECK( super_data.supergates[1].id == 1 );
  CHECK( super_data.supergates[1].name == "and" );
  CHECK( super_data.supergates[1].is_super == true );
  CHECK( super_data.supergates[1].fanin_id.size() == 2 );
  CHECK( super_data.supergates[1].fanin_id[0] == 2 );
  CHECK( super_data.supergates[1].fanin_id[1] == 3 );

  CHECK( super_data.supergates[2].id == 2 );
  CHECK( super_data.supergates[2].name == "and" );
  CHECK( super_data.supergates[2].is_super == false );
  CHECK( super_data.supergates[2].fanin_id.size() == 2 );
  CHECK( super_data.supergates[2].fanin_id[0] == 2 );
  CHECK( super_data.supergates[2].fanin_id[1] == 0 );

  CHECK( super_data.supergates[3].id == 3 );
  CHECK( super_data.supergates[3].name == "and" );
  CHECK( super_data.supergates[3].is_super == true );
  CHECK( super_data.supergates[3].fanin_id.size() == 2 );
  CHECK( super_data.supergates[3].fanin_id[0] == 1 );
  CHECK( super_data.supergates[3].fanin_id[1] == 5 );

  CHECK( super_data.supergates[4].id == 4 );
  CHECK( super_data.supergates[4].name == "or" );
  CHECK( super_data.supergates[4].is_super == false );
  CHECK( super_data.supergates[4].fanin_id.size() == 2 );
  CHECK( super_data.supergates[4].fanin_id[0] == 2 );
  CHECK( super_data.supergates[4].fanin_id[1] == 1 );
}
