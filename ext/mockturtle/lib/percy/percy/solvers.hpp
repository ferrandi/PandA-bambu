#pragma once

#include "solvers/bsat2.hpp"
#include "solvers/bmcg_sat.hpp"
#ifdef USE_CMS
#include "solvers/cmsat.hpp"
#endif
#if defined(USE_GLUCOSE) || defined(USE_SYRUP)
#include "solvers/glucose.hpp"
#endif
#ifdef USE_SATOKO
#include "solvers/satoko.hpp"
#endif