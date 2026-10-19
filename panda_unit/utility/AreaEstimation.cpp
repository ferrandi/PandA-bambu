#include "HLS/module_allocation/area_estimation.hpp"

#include <boost/test/unit_test.hpp>

#include <cmath>
#include <memory>

BOOST_AUTO_TEST_CASE(area_estimation_dsp_weight_formula)
{
   constexpr double lut_area = 100.0;
   constexpr double dsp_count = 2.0;
   constexpr double scale = 350.0;

   const HLS_deviceConstRef no_device;
   const double k1 = area_estimation::get_lut_equivalent_area_weighted(no_device, lut_area, dsp_count, 1.0);
   const double k2 = area_estimation::get_lut_equivalent_area_weighted(no_device, lut_area, dsp_count, 2.0);
   const double k0 = area_estimation::get_lut_equivalent_area_weighted(no_device, lut_area, dsp_count, 0.0);

   BOOST_CHECK_SMALL(std::abs(k1 - (lut_area + dsp_count * scale)), 1e-9);
   BOOST_CHECK_SMALL(std::abs(k2 - (lut_area + dsp_count * scale * 2.0)), 1e-9);
   BOOST_CHECK_SMALL(std::abs(k0 - lut_area), 1e-9);
}

BOOST_AUTO_TEST_CASE(area_estimation_weighted_matches_legacy_when_k_is_one)
{
   const auto area = std::make_shared<area_info>();
   area->resources[area_info::SLICE_LUTS] = 100.0;
   area->resources[area_info::DSP] = 2.0;

   const HLS_deviceConstRef no_device;
   const double legacy = area_estimation::get_lut_equivalent_area(no_device, area);
   const double weighted = area_estimation::get_lut_equivalent_area_weighted(no_device, area, 1.0);
   const double weighted_k2 = area_estimation::get_lut_equivalent_area_weighted(no_device, area, 2.0);

   BOOST_CHECK_SMALL(std::abs(legacy - weighted), 1e-9);
   BOOST_CHECK_SMALL(std::abs(weighted_k2 - 1500.0), 1e-9);
   BOOST_CHECK_SMALL(std::abs(area_estimation::get_lut_component(area) - 100.0), 1e-9);
}

BOOST_AUTO_TEST_CASE(area_estimation_dsp_weight_changes_only_dsp_candidate)
{
   const HLS_deviceConstRef no_device;
   const double lut_only_k1 = area_estimation::get_lut_equivalent_area_weighted(no_device, 500.0, 0.0, 1.0);
   const double lut_only_k10 = area_estimation::get_lut_equivalent_area_weighted(no_device, 500.0, 0.0, 10.0);
   const double dsp_mixed_k1 = area_estimation::get_lut_equivalent_area_weighted(no_device, 100.0, 1.0, 1.0);
   const double dsp_mixed_k10 = area_estimation::get_lut_equivalent_area_weighted(no_device, 100.0, 1.0, 10.0);

   BOOST_CHECK_SMALL(std::abs(lut_only_k1 - lut_only_k10), 1e-9);
   BOOST_CHECK(dsp_mixed_k10 > dsp_mixed_k1);
   BOOST_CHECK(dsp_mixed_k1 < lut_only_k1);
   BOOST_CHECK(dsp_mixed_k10 > lut_only_k10);
}
