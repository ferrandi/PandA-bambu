
dnl
dnl Test for Filesystem library from the Boost C++ libraries
dnl
AC_DEFUN([AX_BOOST_FILESYSTEM],
[
        AC_ARG_WITH([boost-filesystem],
                AS_HELP_STRING([--with-boost-filesystem@<:@=special-lib@:>@],
                               [use the Filesystem library from boost - it is possible to specify a certain library for the linker e.g. --with-boost-filesystem=boost_filesystem-gcc-mt ]),
                [
                        if test "$withval" = "no"; then
                                want_boost="no"
                        elif test "$withval" = "yes"; then
                                want_boost="yes"
                                ax_boost_user_filesystem_lib=""
                        else
                                want_boost="yes"
                                ax_boost_user_filesystem_lib="$withval"
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

                AC_CACHE_CHECK(whether the Boost::Filesystem library is available,
                        ax_cv_boost_filesystem,
                        [AC_LANG_PUSH(C++)
                                AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
                                        @%:@include <boost/filesystem/path.hpp>
                                        ]],[[
                                        using namespace boost::filesystem;
                                        path my_path( "foo/bar/data.txt" );
                                        return 0;

                                        ]])],
                                        ax_cv_boost_filesystem=yes, ax_cv_boost_filesystem=no)
                                AC_LANG_POP([C++])
                        ])
                if test "x$ax_cv_boost_filesystem" = "xyes"; then
                        AC_DEFINE(HAVE_BOOST_FILESYSTEM,1,[define if the Boost::Filesystem library is available])
                        BN=boost_filesystem
                        if test "x$ax_boost_user_filesystem_lib" = "x"; then
                                for ax_lib in  $BN $BN-mt $BN-st $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s lib$BN-mgw lib$BN-mgw-mt lib$BN-mgw-mt-s lib$BN-mgw-s $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s $BN-gcc42-mt $BN-gcc41-mt; do
                                        AC_CHECK_LIB($ax_lib, main,
                                        [BOOST_FILESYSTEM_LIB="-l$ax_lib"; AC_SUBST(BOOST_FILESYSTEM_LIB) link_filesystem="yes"; break],
                                        [link_filesystem="no"])
                                done
                        else
                                for ax_lib in $ax_boost_user_filesystem_lib $BN-$ax_boost_user_filesystem_lib; do
                                        AC_CHECK_LIB($ax_lib, main,
                                                [BOOST_FILESYSTEM_LIB="-l$ax_lib"; AC_SUBST(BOOST_FILESYSTEM_LIB) link_filesystem="yes"; break],
                                                [link_filesystem="no"])
                                done

                        fi
                        if test "x$link_filesystem" = "xno"; then
                                AC_MSG_ERROR(Could not link against $ax_lib !)
                        fi
                else
                        AC_MSG_ERROR(Could not find boost filesystem header)
                fi

                CPPFLAGS="$CPPFLAGS_SAVED"
                LDFLAGS="$LDFLAGS_SAVED"
        fi
])

dnl
dnl Test if boost headers use scoped enums, but not the boost filesystem library
dnl
AC_DEFUN([PANDA_CHECK_BOOST_SCOPED_ENUMS],
[
   AC_REQUIRE([AC_PROG_CC])
   AC_LANG_SAVE
   AC_LANG_CPLUSPLUS
   CXXFLAGS_SAVED="$CXXFLAGS"
   CPPFLAGS_SAVED="$CPPFLAGS"
   CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
   export CPPFLAGS

   LDFLAGS_SAVED="$LDFLAGS"
   LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
   export LDFLAGS

   ac_save_LIBS="$LIBS"
   LIBS="$LIBS $BOOST_FILESYSTEM_LIB  $BOOST_SYSTEM_LIB"
   export LIBS
   AC_LINK_IFELSE([
      AC_LANG_SOURCE([
         #include <boost/filesystem.hpp>
         int main()
         {
            boost::filesystem::copy_file("src", "dst");
            return 0;
         }
      ])],
      [echo "checking if -DBOOST_NO_SCOPED_ENUMS or BOOST_NO_CXX11_SCOPED_ENUMS is required... no"],
      [
         CXXFLAGS="$CXXFLAGS -DBOOST_NO_SCOPED_ENUMS -DBOOST_NO_CXX11_SCOPED_ENUMS"
         AC_LINK_IFELSE([
            AC_LANG_SOURCE([
               #include <boost/filesystem.hpp>
               int main()
               {
                  boost::filesystem::copy_file("src", "dst");
                 return 0;
               }
            ])],
         [
            panda_boost_no_scoped_enums="yes";
            echo "checking if -DBOOST_NO_SCOPED_ENUMS of BOOST_NO_CXX11_SCOPED_ENUMS is required... yes"
         ],
         [AC_MSG_ERROR("Error in linking boost filesystem library")]
         )
      ]
   )
   CXXFLAGS="$CXXFLAGS_SAVED"
   CPPFLAGS="$CPPFLAGS_SAVED"
   LDFLAGS="$LDFLAGS_SAVED"
   LIBS="$ac_save_LIBS"
   AC_LANG_RESTORE
])

dnl
dnl check which -rpath / R flawor is needed
dnl
AC_DEFUN([PANDA_CHECK_RPATH_FLAVOR],
[
   AC_REQUIRE([AC_PROG_CC])
   AC_LANG_SAVE
   AC_LANG_CPLUSPLUS
   CPPFLAGS_SAVED="$CPPFLAGS"
   CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
   export CPPFLAGS

   LDFLAGS_SAVED="$LDFLAGS"
   LDFLAGS="$LDFLAGS $BOOST_LDFLAGS -Wl,-rpath,$BOOST_LIB_DIR"
   export LDFLAGS

   ac_save_LIBS="$LIBS"
   LIBS="$LIBS $BOOST_FILESYSTEM_LIB $BOOST_SYSTEM_LIB"
   export LIBS
   AC_LINK_IFELSE([
      AC_LANG_SOURCE([
         #include <boost/filesystem.hpp>
         int main()
         {
            boost::filesystem::copy_file("src", "dst");
            return 0;
         }
      ])],
      [
         BOOST_LDFLAGS="-Wl,-rpath,$BOOST_LIB_DIR $BOOST_LDFLAGS";
         echo "checking if -rpath is fine... yes"
      ],
      [
         LDFLAGS="$LDFLAGS_SAVED"
         LDFLAGS="$LDFLAGS $BOOST_LDFLAGS -Wl,-R,$BOOST_LIB_DIR"
	 export LDFLAGS
         AC_LINK_IFELSE([
            AC_LANG_SOURCE([
               #include <boost/filesystem.hpp>
               int main()
               {
                  boost::filesystem::copy_file("src", "dst");
                  return 0;
               }
            ])],
            [
               BOOST_LDFLAGS="-Wl,-R,$BOOST_LIB_DIR $BOOST_LDFLAGS";
               echo "checking if -R is fine... yes"
            ],
            [AC_MSG_ERROR("Error in linking boost filesystem library")]
         )

      ]
   )
   CPPFLAGS="$CPPFLAGS_SAVED"
   LDFLAGS="$LDFLAGS_SAVED"
   LIBS="$ac_save_LIBS"
   AC_LANG_RESTORE
])
