
dnl
dnl Test for Program Options library from the Boost C++ libraries
dnl
AC_DEFUN([AX_BOOST_PROGRAM_OPTIONS],
[
        AC_ARG_WITH([boost-program_options],
                AS_HELP_STRING([--with-boost-program_options@<:@=special-lib@:>@],
                               [use the Program Options library from boost - it is possible to specify a certain library for the linker e.g. --with-boost-program_options=boost_program_options-gcc-mt ]),
                [
                        if test "$withval" = "no"; then
                                want_boost="no"
                        elif test "$withval" = "yes"; then
                                want_boost="yes"
                                ax_boost_user_program_options_lib=""
                        else
                                want_boost="yes"
                                ax_boost_user_program_options_lib="$withval"
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

                AC_CACHE_CHECK(whether the Boost::Program Options library is available,
                        ax_cv_boost_program_options,
                        [
                                if test -e "$BOOST_DIR/boost/program_options.hpp"; then
                                        ax_cv_boost_program_options=yes
                                else
                                        ax_cv_boost_program_options=no
                                fi
                        ])
                if test "x$ax_cv_boost_program_options" = "xyes"; then
                        AC_DEFINE(HAVE_BOOST_PROGRAM_OPTIONS,,[define if the Boost::Program Options library is available])
                        BN=boost_program_options
                        if test "x$ax_boost_user_program_options_lib" = "x"; then
                                for ax_lib in $BN $BN-mt $BN-st $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s lib$BN-mgw lib$BN-mgw-mt lib$BN-mgw-mt-s lib$BN-mgw-s $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
                                        AC_CHECK_LIB($ax_lib, main,
                                        [BOOST_PROGRAM_OPTIONS_LIB="-l$ax_lib"; AC_SUBST(BOOST_PROGRAM_OPTIONS_LIB) link_program_options="yes"; break],
                                        [link_program_options="no"])
                                done
                        else
                                for ax_lib in $ax_boost_user_program_options_lib $BN-$ax_boost_user_program_options_lib; do
                                        AC_CHECK_LIB($ax_lib, main,
                                                [BOOST_PROGRAM_OPTIONS_LIB="-l$ax_lib"; AC_SUBST(BOOST_PROGRAM_OPTIONS_LIB) link_program_options="yes"; break],
                                                [link_program_options="no"])
                                done

                        fi
                        if test "x$link_program_options" = "xno"; then
                                AC_MSG_ERROR(Could not link against $ax_lib !)
                        fi
                else
                        AC_MSG_ERROR(Could not find boost program options header)
                fi

                CPPFLAGS="$CPPFLAGS_SAVED"
                LDFLAGS="$LDFLAGS_SAVED"
        fi
])

