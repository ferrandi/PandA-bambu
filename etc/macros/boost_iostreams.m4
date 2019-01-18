
dnl
dnl Test for IOStreams library from the Boost C++ libraries
dnl
AC_DEFUN([AX_BOOST_IOSTREAMS],
[
        AC_ARG_WITH([boost-iostreams],
                AS_HELP_STRING([--with-boost-iostreams@<:@=special-lib@:>@],
                               [use the IOStreams library from boost - it is possible to specify a certain library for the linker e.g. --with-boost-iostreams=boost_iostreams-gcc-mt ]),
                [
                        if test "$withval" = "no"; then
                                want_boost="no"
                        elif test "$withval" = "yes"; then
                                want_boost="yes"
                                ax_boost_user_iostreams_lib=""
                        else
                                want_boost="yes"
                                ax_boost_user_iostreams_lib="$withval"
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
                LDFLAGS="$LDFLAGS $BOOST_LDFLAGS $BOOST_SYSTEM_LIB"
                export LDFLAGS

                AC_CACHE_CHECK(whether the Boost::IOStreams library is available,
                        ax_cv_boost_iostreams,
                        [AC_LANG_PUSH(C++)
                                AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
                                        @%:@include <boost/iostreams/filtering_stream.hpp>
											 @%:@include <boost/range/iterator_range.hpp>
                                        ]],[[
                                        std::string  input = "Hello World!";
								 namespace io = boost::iostreams;
									 io::filtering_istream  in(boost::make_iterator_range(input));
									 return 0;
                                        ]])],
                                        ax_cv_boost_iostreams=yes, ax_cv_boost_iostreams=no)
                                AC_LANG_POP([C++])
                        ])
                if test "x$ax_cv_boost_iostreams" = "xyes"; then
                        AC_DEFINE(HAVE_BOOST_IOSTREAMS,1,[define if the Boost::Filesystem library is available])
                        BN=boost_iostreams
                        if test "x$ax_boost_user_iostreams_lib" = "x"; then
                                for ax_lib in  $BN $BN-mt $BN-st $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s lib$BN-mgw lib$BN-mgw-mt lib$BN-mgw-mt-s lib$BN-mgw-s $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s $BN-gcc42-mt $BN-gcc41-mt; do
                                        AC_CHECK_LIB($ax_lib, main,
                                        [BOOST_IOSTREAMS_LIB="-l$ax_lib"; AC_SUBST(BOOST_IOSTREAMS_LIB) link_iostreams="yes"; break],
                                        [link_iostreams="no"])
                                done
                        else
                                for ax_lib in $ax_boost_user_iostreams_lib $BN-$ax_boost_user_iostreams_lib; do
                                        AC_CHECK_LIB($ax_lib, main,
                                                [BOOST_IOSTREAMS_LIB="-l$ax_lib"; AC_SUBST(BOOST_IOSTREAMS_LIB) link_iostreams="yes"; break],
                                                [link_iostreams="no"])
                                done

                        fi
                        if test "x$link_iostreams" = "xno"; then
                                AC_MSG_ERROR(Could not link against $ax_lib !)
                        fi
                else
                        AC_MSG_ERROR(Could not find boost iostreams header)
                fi

                CPPFLAGS="$CPPFLAGS_SAVED"
                LDFLAGS="$LDFLAGS_SAVED"
        fi
])

