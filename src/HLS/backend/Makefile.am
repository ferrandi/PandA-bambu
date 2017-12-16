#this file is src/HLS/backend/Makefile.am
METASOURCES = AUTO
noinst_LTLIBRARIES = lib_hls_backend.la

lib_hls_backend_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/design_flows/backend/ToHDL \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/evaluation \
   -I$(top_srcdir)/src/HLS/simulation \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/wrapper/synthesis \
   -I$(top_srcdir)/src/wrapper/simulation \
   -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS = generate_hdl.hpp
lib_hls_backend_la_SOURCES = generate_hdl.cpp

if BUILD_SIMULATION
  noinst_HEADERS += generate_simulation_scripts.hpp
  lib_hls_backend_la_SOURCES += generate_simulation_scripts.cpp
endif

if BUILD_SYNTHESIS
  noinst_HEADERS += generate_synthesis_scripts.hpp
  lib_hls_backend_la_SOURCES += generate_synthesis_scripts.cpp
endif

if BUILD_TASTE
  lib_hls_backend_la_CPPFLAGS += \
     -I$(top_srcdir)/src/intermediate_representations/aadl_asn
  noinst_HEADERS += \
     generate_taste_hdl_architecture.hpp \
     generate_taste_synthesis_script.hpp
  lib_hls_backend_la_SOURCES += \
     generate_taste_hdl_architecture.cpp \
     generate_taste_synthesis_script.cpp
endif

#do not touch the following line

