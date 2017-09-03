#this file is src/wrapper/synthesis/synopsys/Makefile.am
METASOURCES = AUTO
SUBDIRS = design_compiler
DIST_SUBDIRS = design_compiler

noinst_LTLIBRARIES = lib_Synopsyswrapper.la

noinst_HEADERS = SynopsysWrapper.hpp
lib_Synopsyswrapper_la_SOURCES = SynopsysWrapper.cpp
lib_Synopsyswrapper_la_CPPFLAGS = \
                                 -I$(top_srcdir)/src \
                                 -I$(top_srcdir)/src/polixml \
                                 -I$(top_srcdir)/src/utility \
                                 -I$(top_srcdir)/src/wrapper/synthesis \
                                 -I$(top_srcdir)/src/wrapper/synthesis/synopsys/design_compiler \
                                 -I$(BOOST_DIR) $(all_includes)

lib_Synopsyswrapper_la_LIBADD = \
                              $(top_builddir)/src/wrapper/synthesis/synopsys/design_compiler/lib_designcompiler.la

if BUILD_CORE_TOOLS
  SUBDIRS += core_tools
  lib_Synopsyswrapper_la_LIBADD += $(top_builddir)/src/wrapper/synthesis/synopsys/core_tools/lib_core_tools.la
endif

if BUILD_LIB_EXPERIMENTAL
  SUBDIRS += formality library_compiler primetime
  DIST_SUBDIRS += formality library_compiler primetime
  lib_Synopsyswrapper_la_LIBADD += \
                  $(top_builddir)/src/wrapper/synthesis/synopsys/formality/lib_formality.la \
                  $(top_builddir)/src/wrapper/synthesis/synopsys/library_compiler/lib_librarycompiler.la \
                  $(top_builddir)/src/wrapper/synthesis/synopsys/primetime/lib_primetime.la
  lib_Synopsyswrapper_la_CPPFLAGS += \
                 -I$(top_srcdir)/src/wrapper/synthesis/synopsys/formality \
                 -I$(top_srcdir)/src/wrapper/synthesis/synopsys/library_compiler \
                 -I$(top_srcdir)/src/wrapper/synthesis/synopsys/primetime
endif

#do not touch the following line

