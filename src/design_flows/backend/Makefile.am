#this file is src/design_flows/backend/Makefile.am
SUBDIRS =
DIST_SUBDIRS =
METASOURCES = AUTO
noinst_LTLIBRARIES = lib_backend.la 
lib_backend_la_SOURCES =
lib_backend_la_LIBADD =
noinst_HEADERS =

if BUILD_LIB_TO_C
  SUBDIRS += ToC
  DIST_SUBDIRS += ToC
  lib_backend_la_LIBADD += $(top_builddir)/src/design_flows/backend/ToC/lib_ToC.la
endif

if BUILD_LIB_TO_HDL
  SUBDIRS += ToHDL
  DIST_SUBDIRS += ToHDL
  lib_backend_la_LIBADD += $(top_builddir)/src/design_flows/backend/ToHDL/lib_ToHDL.la
endif

if BUILD_LIB_TO_DATAFILE
  SUBDIRS += ToDataFile
  DIST_SUBDIRS += ToDataFile
  lib_backend_la_LIBADD += $(top_builddir)/src/design_flows/backend/ToDataFile/lib_ToDataFile.la
endif

#do not touch the following line

