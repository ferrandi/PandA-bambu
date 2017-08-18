AC_DEFUN([AX_BOOST_DATE_TIME],
      [
      AC_ARG_WITH([boost-date-time],
         AS_HELP_STRING([--with-boost-date-time@<:@=special-lib@:>@],
            [use the Date_Time library from boost - it is possible to specify a certain library for the linker
            e.g. --with-boost-date-time=boost_date_time-gcc-mt-d-1_33_1 ]),
         [
         if test "$withval" = "no"; then
         want_boost="no"
         elif test "$withval" = "yes"; then
         want_boost="yes"
         ax_boost_user_date_time_lib=""
         else
         want_boost="yes"
         ax_boost_user_date_time_lib="$withval"
         fi
         ],
         [want_boost="yes"]
         )

      if test "x$want_boost" = "xyes"; then
         AC_REQUIRE([AC_PROG_CC])
   CPPFLAGS_SAVED="$CPPFLAGS"
   CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
   export CPPFLAGS

   LDFLAGS_SAVED="$LDFLAGS"
   LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
   export LDFLAGS

AC_CACHE_CHECK(whether the Boost::Date_Time library is available,
      ax_cv_boost_date_time,
      [AC_LANG_PUSH(C++)
      AC_COMPILE_IFELSE([AC_LANG_SOURCE([[@%:@include <boost/date_time/gregorian/gregorian_types.hpp>]],
            [[using namespace boost::gregorian; date d(2002,Jan,10);
            return 0;





            ]])],
         ax_cv_boost_date_time=yes, ax_cv_boost_date_time=no)
      AC_LANG_POP([C++])
      ])
   if test "x$ax_cv_boost_date_time" = "xyes"; then
AC_DEFINE(HAVE_BOOST_DATE_TIME,,[define if the Boost::Date_Time library is available])
   BN=boost_date_time
   if test "x$ax_boost_user_date_time_lib" = "x"; then
   for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s $BN-mt \
      lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
      $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
      AC_CHECK_LIB($ax_lib, exit, [BOOST_DATE_TIME_LIB="-l$ax_lib"; AC_SUBST(BOOST_DATE_TIME_LIB) link_date_time="yes"; break],
            [link_date_time="no"])
      done
      else
      for ax_lib in $ax_boost_user_date_time_lib $BN-$ax_boost_user_date_time_lib; do
      AC_CHECK_LIB($ax_lib, exit,
            [BOOST_DATE_TIME_LIB="-l$ax_lib"; AC_SUBST(BOOST_DATE_TIME_LIB) link_date_time="yes"; break],
            [link_date_time="no"])
      done
      fi
   if test "x$link_date_time" != "xyes"; then
AC_MSG_ERROR(Could not link against $ax_lib !)
   fi
   fi

   CPPFLAGS="$CPPFLAGS_SAVED"
   LDFLAGS="$LDFLAGS_SAVED"
   fi
   ])
