#this file is src/wrapper/simulation/Makefile.am
SUBDIRS = isim modelsim icarus verilator xsim
DIST_SUBDIRS = isim modelsim icarus verilator xsim

METASOURCES = AUTO

noinst_LTLIBRARIES = lib_simulation.la

lib_simulation_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/wrapper \
   -I$(top_srcdir)/src/wrapper/simulation/icarus \
   -I$(top_srcdir)/src/wrapper/simulation/modelsim \
   -I$(top_srcdir)/src/wrapper/simulation/isim \
   -I$(top_srcdir)/src/wrapper/simulation/xsim \
   -I$(top_srcdir)/src/wrapper/simulation/verilator \
   -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS = SimulationTool.hpp
lib_simulation_la_SOURCES = SimulationTool.cpp
lib_simulation_la_LIBADD = \
   $(top_builddir)/src/wrapper/simulation/verilator/lib_verilatorwrapper.la \
   $(top_builddir)/src/wrapper/simulation/isim/lib_isimwrapper.la \
   $(top_builddir)/src/wrapper/simulation/xsim/lib_xsimwrapper.la \
   $(top_builddir)/src/wrapper/simulation/modelsim/lib_modelsimwrapper.la \
   $(top_builddir)/src/wrapper/simulation/icarus/lib_icaruswrapper.la

#do not touch the following line
include $(top_srcdir)/panda.am
