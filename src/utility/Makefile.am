#this file is src/utility/Makefile.am
METASOURCES = AUTO
SUBDIRS=
DIST_SUBDIRS=

noinst_LTLIBRARIES = lib_utility.la


lib_utility_la_CPPFLAGS = \
                     -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS = \
   augmented_vector.hpp \
   cpu_stats.hpp \
   cpu_time.hpp \
   custom_map.hpp \
   custom_set.hpp \
   dbgPrintHelper.hpp \
   exceptions.hpp \
   fileIO.hpp \
   gzstream.hpp \
   hash_helper.hpp \
   indented_output_stream.hpp \
   Lexer_utilities.hpp \
   math_function.hpp \
   refcount.hpp \
   simple_indent.hpp \
   Statistics.hpp \
   string_manipulation.hpp \
   strong_typedef.hpp \
   utility.hpp \
   visitor.hpp \
   xml_helper.hpp

lib_utility_la_SOURCES = \
   cpu_stats.cpp \
   exceptions.cpp \
   indented_output_stream.cpp \
   math_function.cpp \
   simple_indent.cpp \
   Statistics.cpp \
   string_manipulation.cpp \
   utility.cpp

lib_utility_la_LIBADD= $(BOOST_DATE_TIME_LIB)

if MINGW64
   lib_utility_la_LIBADD += -lws2_32 -lpsapi
endif

if BUILD_LIB_PROBABILITY_DISTRIBUTION
  SUBDIRS += probability_distribution
  DIST_SUBDIRS += probability_distribution
  lib_utility_la_CPPFLAGS += -I$(top_srcdir)/src/utility/probability_distribution
  lib_utility_la_LIBADD += $(top_builddir)/src/utility/probability_distribution/lib_ProbabilityDistribution.la
endif

#do not touch the following line

