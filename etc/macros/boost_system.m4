
dnl
dnl Test for System library from the Boost C++ libraries
dnl
AC_DEFUN([AX_BOOST_SYSTEM],
      [
      AC_ARG_WITH([boost-system],
         AS_HELP_STRING([--with-boost-system@<:@=special-lib@:>@],
            [use the System library from boost - it is possible to specify a certain library for the linker e.g. --with-boost-system=boost_system-gcc-mt ]),
         [
         if test "$withval" = "no"; then
         want_boost="no"
         elif test "$withval" = "yes"; then
         want_boost="yes"
         ax_boost_user_system_lib=""
         else
         want_boost="yes"
         ax_boost_user_system_lib="$withval"
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

AC_CACHE_CHECK(whether the Boost::System library is available,
      ax_cv_boost_system,
      [AC_LANG_PUSH(C++)
      AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
            @%:@include <boost/version.hpp>
            ]], [[
#if BOOST_VERSION >= 103600
            // Everything is okay
#else
#  error Boost version is too old
#endif
            ]])],
         ax_cv_boost_system=yes, ax_cv_boost_system=no)
      AC_LANG_POP([C++])
      ])
   if test "x$ax_cv_boost_system" = "xyes"; then
AC_DEFINE(HAVE_BOOST_SYSTEM,1,[define if the Boost::System library is available])
   BN=boost_system
   if test "x$ax_boost_user_system_lib" = "x"; then
   for ax_lib in $BN $BN-mt $BN-st $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s lib$BN-mgw lib$BN-mgw-mt lib$BN-mgw-mt-s lib$BN-mgw-s $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s $BN-gcc42-mt $BN-gcc41-mt; do
   AC_CHECK_LIB($ax_lib, main,
         [BOOST_SYSTEM_LIB="-l$ax_lib"; AC_SUBST(BOOST_SYSTEM_LIB) link_system="yes"; break],
         [link_system="no"])
   done
   else
   for ax_lib in $ax_boost_user_system_lib $BN-$ax_boost_user_system_lib; do
   AC_CHECK_LIB($ax_lib, main,
         [BOOST_SYSTEM_LIB="-l$ax_lib"; AC_SUBST(BOOST_SYSTEM_LIB) link_system="yes"; break],
         [link_system="no"])
   done

   fi
   if test "x$link_system" = "xno"; then
AC_MSG_ERROR(Could not link against $ax_lib !)
   fi
   else
AC_MSG_NOTICE(Boost version does not require the system library)
   BOOST_SYSTEM_LIB=""
AC_SUBST(BOOST_SYSTEM_LIB)

   fi

   CPPFLAGS="$CPPFLAGS_SAVED"
   LDFLAGS="$LDFLAGS_SAVED"
   fi
   ])

