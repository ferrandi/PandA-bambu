check_PROGRAMS = program_tests

program_tests_CPPFLAGS = \
   $(all_includes) \
   -I$(BOOST_DIR) \
   ${BOOST_CPPFLAGS} \
   -I$(top_srcdir)/src/utility \
   $(AM_CPPFLAGS)

program_tests_SOURCES = \
   main_tests.cpp \
   utility/APInt.cpp \
   utility/NaturalVersionOrder.cpp \
   utility/Range.cpp

program_tests_LDFLAGS = $(BOOST_LDFLAGS)

program_tests_LDADD = \
   -lboost_unit_test_framework \
   ../src/lib_utility.la \
   @PTHREAD_HACK@ \
   ../src/bambu-global_variables.o

TESTS = program_tests
