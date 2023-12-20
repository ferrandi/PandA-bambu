#include "fileIO.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(natural_version_order)
{
   std::vector<std::filesystem::path> vers = {
       "/opt/NanoXplore/2.9.0",          "/opt/NanoXplore/2.9.5",          "/opt/NanoXplore/2.9.4",
       "/opt/NanoXplore/2.9.7",          "/opt/NanoXplore/2.9.2",          "/opt/NanoXplore/NXLMD",
       "/opt/NanoXplore/nxmap-3.9.0.5",  "/opt/NanoXplore/nxmap-3.11.1.4", "/opt/NanoXplore/nxmdesignsuite-22.1.0.1",
       "/opt/NanoXplore/nxmap-23.3.0.2", "/opt/NanoXplore/nxmap-23.1.0.2"};

   const std::vector<std::filesystem::path> expected = {
       "/opt/NanoXplore/NXLMD",          "/opt/NanoXplore/2.9.0",          "/opt/NanoXplore/2.9.2",
       "/opt/NanoXplore/2.9.4",          "/opt/NanoXplore/2.9.5",          "/opt/NanoXplore/2.9.7",
       "/opt/NanoXplore/nxmap-3.9.0.5",  "/opt/NanoXplore/nxmap-3.11.1.4", "/opt/NanoXplore/nxmdesignsuite-22.1.0.1",
       "/opt/NanoXplore/nxmap-23.1.0.2", "/opt/NanoXplore/nxmap-23.3.0.2"};

   std::sort(vers.begin(), vers.end(), NaturalVersionOrder);

   BOOST_CHECK_EQUAL_COLLECTIONS(vers.begin(), vers.end(), expected.begin(), expected.end());
}