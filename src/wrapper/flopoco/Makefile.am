# src/wrapper/flopoco/Makefile.am
METASOURCES = AUTO
noinst_LTLIBRARIES = lib_flopocowrapper.la
noinst_HEADERS = FPAssign.hpp FPgt_expr.hpp FPlt_expr.hpp FPge_expr.hpp FPle_expr.hpp flopoco_wrapper.hpp

lib_flopocowrapper_la_SOURCES = FPAssign.cpp FPgt_expr.cpp FPlt_expr.cpp FPge_expr.cpp FPle_expr.cpp flopoco_wrapper.cpp

lib_flopocowrapper_la_CPPFLAGS = \
    -I$(top_srcdir)/src/circuit \
    -I$(top_srcdir)/src/utility \
    -I$(top_srcdir)/src/technology/physical_library \
    -I$(top_srcdir)/src/technology/target_device \
    -I$(FLOPOCO_DIR)/src/ \
    -I$(BOOST_DIR) $(all_includes)


#DOCUMENTATION STUFF

DOX_TAGFILES=

PRJ_DOC =
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line
include $(top_srcdir)/panda.am
