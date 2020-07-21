dnl
dnl check gcc with plugin support enabled and plugins
dnl
AC_DEFUN([AC_CHECK_GCC45_I386_VERSION],[
    AC_ARG_WITH(gcc45,
    [  --with-gcc45=executable-path path where the GCC 4.5 is installed ],
    [
       ac_gcc45="$withval"
    ])

dnl switch to c
AC_LANG_PUSH([C])

if test "x$ac_gcc45" = x; then
   GCC_TO_BE_CHECKED="/usr/bin/gcc-4.5 /usr/bin/gcc"
else
   GCC_TO_BE_CHECKED=$ac_gcc45;
fi

echo "looking for gcc 4.5..."
for compiler in $GCC_TO_BE_CHECKED; do
   if test -f $compiler; then
      echo "checking $compiler..."
      dnl check for gcc
      I386_GCC45_VERSION=`$compiler -dumpspecs | grep \*version -A1 | tail -1`
      I386_GCC45_FULL_VERSION=`$compiler --version`
      AS_VERSION_COMPARE($1, [4.5.0], MIN_GCC45=[4.5.0], MIN_GCC45=$1, MIN_GCC45=$1)
      AS_VERSION_COMPARE([4.6.0], $2, MAX_GCC45=[4.6.0], MAX_GCC45=$2, MAX_GCC45=$2)
      AS_VERSION_COMPARE($I386_GCC45_VERSION, $MIN_GCC45, echo "checking $compiler >= $MIN_GCC45... no"; min=no, echo "checking $compiler >= $MIN_GCC45... yes"; min=yes, echo "checking $compiler >= $MIN_GCC45... yes"; min=yes)
      if test "$min" = "no" ; then
         continue;
      fi
      AS_VERSION_COMPARE($I386_GCC45_VERSION, $MAX_GCC45, echo "checking $compiler < $MAX_GCC45... yes"; max=yes, echo "checking $compiler < $MAX_GCC45... no"; max=no, echo "checking $compiler < $MAX_GCC45... no"; max=no)
      if test "$max" = "no" ; then
         continue;
      fi
      I386_GCC45_EXE=$compiler;
      I386_GCC45_PLUGIN_DIR=`$I386_GCC45_EXE -print-file-name=plugin`
      if test "x$I386_GCC45_PLUGIN_DIR" = "xplugin"; then
         echo "checking plugin support... no. Package gcc-4.5-plugin-dev missing?"
         break;
      fi
      echo "checking plugin directory...$I386_GCC45_PLUGIN_DIR"
      gcc_file=`basename $I386_GCC45_EXE`
      gcc_dir=`dirname $I386_GCC45_EXE`
      cpp=`echo $gcc_file | sed s/gcc/cpp/`
      I386_CPP45_EXE=$gcc_dir/$cpp
      if test -f $I386_CPP45_EXE; then
         echo "checking cpp...$I386_CPP45_EXE"
      else
         echo "checking cpp...no"
         I386_GCC45_EXE=""
         continue
      fi
      gpp=`echo $gcc_file | sed s/gcc/g\+\+/`
      I386_GPP45_EXE=$gcc_dir/$gpp
      if test -f $I386_GPP45_EXE; then
         echo "checking g++...$I386_GPP45_EXE"
      else
         echo "checking g++...no"
         continue
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_GCC45_EXE
      CFLAGS="-m32"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_GCC45_MULTIARCH=yes,I386_GCC45_MULTIARCH=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_GCC45_MULTIARCH" != xyes; then
         echo "checking support to -m32... no"
         continue
      fi
      cat > plugin_test.c <<PLUGIN_TEST
      #include "plugin_includes.h"

      int plugin_is_GPL_compatible;

      extern struct cpp_reader *parse_in;

      void do_nothing(void * first, void * second)
      {
         cpp_define (parse_in, "TEST_GCC_PLUGIN=1");
      }

      int
      plugin_init (struct plugin_name_args * plugin_info, struct plugin_gcc_version * version)
      {
         if (!plugin_default_version_check(version, &gcc_version))
            return 1;
         register_callback("plugin_test", PLUGIN_START_UNIT, do_nothing, NULL);
         return 0;
      }
PLUGIN_TEST
      for plugin_compiler in $I386_GCC45_EXE $I386_GPP45_EXE; do
         if test -f plugin_test.so; then
            rm plugin_test.so
         fi
         $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ -fPIC -shared plugin_test.c -o plugin_test.so -I$I386_GCC45_PLUGIN_DIR/include
         if test ! -f plugin_test.so; then
            echo "checking $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ -fPIC -shared plugin_test.c -o plugin_test.so -I$I386_GCC45_PLUGIN_DIR/include... no"
            continue
         fi
         echo "checking $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ -fPIC -shared plugin_test.c -o plugin_test.so -I$I386_GCC45_PLUGIN_DIR/include... yes"
         ac_save_CC="$CC"
         ac_save_CFLAGS="$CFLAGS"
         CC=$plugin_compiler
         CFLAGS="-fplugin=$BUILDDIR/plugin_test.so"
         AC_LANG_PUSH([C])
         AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
               ]],[[
                  return 0;
               ]])],
         I386_GCC45_PLUGIN_COMPILER=$plugin_compiler,I386_GCC45_PLUGIN_COMPILER=)
         AC_LANG_POP([C])
         CC=$ac_save_CC
         CFLAGS=$ac_save_CFLAGS
         if test "x$I386_GCC45_PLUGIN_COMPILER" != x; then
            break;
         fi
         ac_save_CC="$CC"
         ac_save_CFLAGS="$CFLAGS"
      done
      if test "x$I386_GCC45_PLUGIN_COMPILER" != x; then
         build_I386_GCC45=yes;
         build_I386_GCC45_EMPTY_PLUGIN=yes;
         build_I386_GCC45_SSA_PLUGIN=yes;
         build_I386_GCC45_SSA_PLUGINCPP=yes;
         build_I386_GCC45_SSAVRP_PLUGIN=yes;
         build_I386_GCC45_TOPFNAME_PLUGIN=yes;
         break;
      fi
   else
      echo "checking $compiler... not found"
   fi
done

if test x$I386_GCC45_PLUGIN_COMPILER != x; then
dnl set configure and makefile variables
  I386_GCC45_EMPTY_PLUGIN=gcc45_plugin_dumpGimpleEmpty
  I386_GCC45_SSA_PLUGIN=gcc45_plugin_dumpGimpleSSA
  I386_GCC45_SSA_PLUGINCPP=gcc45_plugin_dumpGimpleSSACpp
  I386_GCC45_SSAVRP_PLUGIN=gcc45_plugin_dumpGimpleSSAVRP
  I386_GCC45_TOPFNAME_PLUGIN=gcc45_plugin_topfname
  AC_SUBST(I386_GCC45_EMPTY_PLUGIN)
  AC_SUBST(I386_GCC45_SSA_PLUGIN)
  AC_SUBST(I386_GCC45_SSA_PLUGINCPP)
  AC_SUBST(I386_GCC45_SSAVRP_PLUGIN)
  AC_SUBST(I386_GCC45_TOPFNAME_PLUGIN)
  AC_SUBST(I386_GCC45_PLUGIN_DIR)
  AC_SUBST(I386_GCC45_EXE)
  AC_SUBST(I386_GCC45_VERSION)
  AC_SUBST(I386_GCC45_PLUGIN_COMPILER)
  AC_DEFINE(HAVE_I386_GCC45_COMPILER, 1, "Define if GCC 4.5 I386 compiler is compliant")
  AC_DEFINE_UNQUOTED(I386_GCC45_EXE, "${I386_GCC45_EXE}", "Define the plugin gcc")
  AC_DEFINE_UNQUOTED(I386_CPP45_EXE, "${I386_CPP45_EXE}", "Define the plugin cpp")
  AC_DEFINE_UNQUOTED(I386_GPP45_EXE, "${I386_GPP45_EXE}", "Define the plugin g++")
  AC_DEFINE_UNQUOTED(I386_GCC45_EMPTY_PLUGIN, "${I386_GCC45_EMPTY_PLUGIN}", "Define the filename of the GCC PandA Empty plugin")
  AC_DEFINE_UNQUOTED(I386_GCC45_SSA_PLUGIN, "${I386_GCC45_SSA_PLUGIN}", "Define the filename of the GCC PandA SSA plugin")
  AC_DEFINE_UNQUOTED(I386_GCC45_SSA_PLUGINCPP, "${I386_GCC45_SSA_PLUGINCPP}", "Define the filename of the GCC PandA C++ SSA plugin")
  AC_DEFINE_UNQUOTED(I386_GCC45_SSAVRP_PLUGIN, "${I386_GCC45_SSAVRP_PLUGIN}", "Define the filename of the GCC PandA SSAVRP plugin")
  AC_DEFINE_UNQUOTED(I386_GCC45_TOPFNAME_PLUGIN, "${I386_GCC45_TOPFNAME_PLUGIN}", "Define the filename of the GCC PandA topfname plugin")
  AC_DEFINE_UNQUOTED(I386_GCC45_VERSION, "${I386_GCC45_VERSION}", "Define the gcc version")
  AC_DEFINE_UNQUOTED(I386_GCC45_PLUGIN_COMPILER, "${I386_GCC45_PLUGIN_COMPILER}", "Define the plugin compiler")
fi


dnl switch back to old language
AC_LANG_POP([C])

])

dnl
dnl check gcc with plugin support enabled and plugins
dnl
AC_DEFUN([AC_CHECK_GCC46_I386_VERSION],[
    AC_ARG_WITH(gcc46,
    [  --with-gcc46=executable-path path where the GCC 4.6 is installed ],
    [
       ac_gcc46="$withval"
    ])

dnl switch to c
AC_LANG_PUSH([C])

if test "x$ac_gcc46" = x; then
   GCC_TO_BE_CHECKED="/usr/bin/gcc-4.6 /usr/bin/gcc"
else
   GCC_TO_BE_CHECKED=$ac_gcc46;
fi

echo "looking for gcc 4.6..."
for compiler in $GCC_TO_BE_CHECKED; do
   if test -f $compiler; then
      echo "checking $compiler..."
      dnl check for gcc
      I386_GCC46_VERSION=`$compiler -dumpspecs | grep \*version -A1 | tail -1`
      I386_GCC46_FULL_VERSION=`$compiler --version`
      AS_VERSION_COMPARE($1, [4.6.0], MIN_GCC46=[4.6.0], MIN_GCC46=$1, MIN_GCC46=$1)
      AS_VERSION_COMPARE([4.7.0], $2, MAX_GCC46=[4.7.0], MAX_GCC46=$2, MAX_GCC46=$2)
      AS_VERSION_COMPARE($I386_GCC46_VERSION, $MIN_GCC46, echo "checking $compiler >= $MIN_GCC46... no"; min=no, echo "checking $compiler >= $MIN_GCC46... yes"; min=yes, echo "checking $compiler >= $MIN_GCC46... yes"; min=yes)
      if test "$min" = "no" ; then
         continue;
      fi
      AS_VERSION_COMPARE($I386_GCC46_VERSION, $MAX_GCC46, echo "checking $compiler < $MAX_GCC46... yes"; max=yes, echo "checking $compiler < $MAX_GCC46... no"; max=no, echo "checking $compiler < $MAX_GCC46... no"; max=no)
      if test "$max" = "no" ; then
         continue;
      fi
      I386_GCC46_EXE=$compiler;
      I386_GCC46_PLUGIN_DIR=`$I386_GCC46_EXE -print-file-name=plugin`
      if test "x$I386_GCC46_PLUGIN_DIR" = "xplugin"; then
         echo "checking plugin support... no. Package gcc-4.6-plugin-dev missing?"
         break;
      fi
      echo "checking plugin directory...$I386_GCC46_PLUGIN_DIR"
      gcc_file=`basename $I386_GCC46_EXE`
      gcc_dir=`dirname $I386_GCC46_EXE`
      cpp=`echo $gcc_file | sed s/gcc/cpp/`
      I386_CPP46_EXE=$gcc_dir/$cpp
      if test -f $I386_CPP46_EXE; then
         echo "checking cpp...$I386_CPP46_EXE"
      else
         echo "checking cpp...no"
         I386_GCC46_EXE=""
         continue
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_GCC46_EXE
      CFLAGS="-m32"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_GCC46_MULTIARCH=yes,I386_GCC46_MULTIARCH=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_GCC46_MULTIARCH" != xyes; then
         echo "checking support to -m32... no"
         continue
      fi
      gpp=`echo $gcc_file | sed s/gcc/g\+\+/`
      I386_GPP46_EXE=$gcc_dir/$gpp
      if test -f $I386_GPP46_EXE; then
         echo "checking g++...$I386_GPP46_EXE"
      else
         echo "checking g++...no"
         continue
      fi
      cat > plugin_test.c <<PLUGIN_TEST
      #include "plugin_includes.h"

      int plugin_is_GPL_compatible;

      extern struct cpp_reader *parse_in;

      void do_nothing(void * first, void * second)
      {
         cpp_define (parse_in, "TEST_GCC_PLUGIN=1");
      }

      int
      plugin_init (struct plugin_name_args * plugin_info, struct plugin_gcc_version * version)
      {
         if (!plugin_default_version_check(version, &gcc_version))
            return 1;
         register_callback("plugin_test", PLUGIN_START_UNIT, do_nothing, NULL);
         return 0;
      }
PLUGIN_TEST
      for plugin_compiler in $I386_GCC46_EXE $I386_GPP46_EXE; do
         if test -f plugin_test.so; then
            rm plugin_test.so
         fi
         $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ -fPIC -shared plugin_test.c -o plugin_test.so -I$I386_GCC46_PLUGIN_DIR/include 2> /dev/null
         if test ! -f plugin_test.so; then
            echo "checking $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ -fPIC -shared plugin_test.c -o plugin_test.so -I$I386_GCC46_PLUGIN_DIR/include... no"
            continue
         fi
         echo "checking $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ -fPIC -shared plugin_test.c -o plugin_test.so -I$I386_GCC46_PLUGIN_DIR/include... yes"
         ac_save_CC="$CC"
         ac_save_CFLAGS="$CFLAGS"
         CC=$plugin_compiler
         CFLAGS="-fplugin=$BUILDDIR/plugin_test.so"
         AC_LANG_PUSH([C])
         AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
               ]],[[
                  return 0;
               ]])],
         I386_GCC46_PLUGIN_COMPILER=$plugin_compiler,I386_GCC46_PLUGIN_COMPILER=)
         AC_LANG_POP([C])
         CC=$ac_save_CC
         CFLAGS=$ac_save_CFLAGS
         #If plugin compilation fails, skip this executable
         if test "x$I386_GCC46_PLUGIN_COMPILER" = x; then
            continue
         fi
         echo "Looking for gengtype"
         I386_GCC46_GENGTYPE=`$I386_GCC46_EXE -print-file-name=gengtype`
         if test "x$I386_GCC46_GENGTYPE" = "xgengtype"; then
            I386_GCC46_GENGTYPE=`$I386_GCC46_EXE -print-file-name=plugin/gengtype`
            if test "x$I386_GCC46_GENGTYPE" = "xplugin/gengtype"; then
               I386_GCC46_ROOT_DIR=`dirname $I386_GCC46_EXE`/..
               I386_GCC46_GENGTYPE=`find $I386_GCC46_ROOT_DIR -name gengtype | head -n1`
               if test "x$I386_GCC46_GENGTYPE" = "x"; then
                  I386_GCC46_PLUGIN_COMPILER=
                  continue
               fi
            fi
         fi
         echo "Looking for gtype.state"
         I386_GCC46_GTYPESTATE=`$I386_GCC46_EXE -print-file-name=gtype.state`
         if test "x$I386_GCC46_GTYPESTATE" = "xgtype.state"; then
            I386_GCC46_GTYPESTATE=`$I386_GCC46_EXE -print-file-name=plugin/gtype.state`
            if test "x$I386_GCC46_GTYPESTATE" = "xplugin/gtype.state"; then
               I386_GCC46_PLUGIN_COMPILER=
               continue
            fi
         fi
         echo "OK, we have found the compiler"
         build_I386_GCC46=yes;
         build_I386_GCC46_EMPTY_PLUGIN=yes;
         build_I386_GCC46_SSA_PLUGIN=yes;
         build_I386_GCC46_SSA_PLUGINCPP=yes;
         build_I386_GCC46_SSAVRP_PLUGIN=yes;
         build_I386_GCC46_TOPFNAME_PLUGIN=yes;
         break;
      done
      if test "x$I386_GCC46_PLUGIN_COMPILER" != x; then
         break;
      fi
   else
      echo "checking $compiler... not found"
   fi
done

if test x$I386_GCC46_PLUGIN_COMPILER != x; then
  dnl set configure and makefile variables
  I386_GCC46_EMPTY_PLUGIN=gcc46_plugin_dumpGimpleEmpty
  I386_GCC46_SSA_PLUGIN=gcc46_plugin_dumpGimpleSSA
  I386_GCC46_SSA_PLUGINCPP=gcc46_plugin_dumpGimpleSSACpp
  I386_GCC46_SSAVRP_PLUGIN=gcc46_plugin_dumpGimpleSSAVRP
  I386_GCC46_TOPFNAME_PLUGIN=gcc46_plugin_topfname
  AC_SUBST(I386_GCC46_EMPTY_PLUGIN)
  AC_SUBST(I386_GCC46_SSA_PLUGIN)
  AC_SUBST(I386_GCC46_SSA_PLUGINCPP)
  AC_SUBST(I386_GCC46_SSAVRP_PLUGIN)
  AC_SUBST(I386_GCC46_TOPFNAME_PLUGIN)
  AC_SUBST(I386_GCC46_PLUGIN_DIR)
  AC_SUBST(I386_GCC46_GENGTYPE)
  AC_SUBST(I386_GCC46_GTYPESTATE)
  AC_SUBST(I386_GCC46_EXE)
  AC_SUBST(I386_GCC46_VERSION)
  AC_SUBST(I386_GCC46_PLUGIN_COMPILER)
  AC_DEFINE(HAVE_I386_GCC46_COMPILER, 1, "Define if GCC 4.6 I386 compiler is compliant")
  AC_DEFINE_UNQUOTED(I386_GCC46_EXE, "${I386_GCC46_EXE}", "Define the plugin gcc")
  AC_DEFINE_UNQUOTED(I386_CPP46_EXE, "${I386_CPP46_EXE}", "Define the plugin cpp")
  AC_DEFINE_UNQUOTED(I386_GPP46_EXE, "${I386_GPP46_EXE}", "Define the plugin g++")
  AC_DEFINE_UNQUOTED(I386_GCC46_EMPTY_PLUGIN, "${I386_GCC46_EMPTY_PLUGIN}", "Define the filename of the GCC PandA Empty plugin")
  AC_DEFINE_UNQUOTED(I386_GCC46_SSA_PLUGIN, "${I386_GCC46_SSA_PLUGIN}", "Define the filename of the GCC PandA SSA plugin")
  AC_DEFINE_UNQUOTED(I386_GCC46_SSA_PLUGINCPP, "${I386_GCC46_SSA_PLUGINCPP}", "Define the filename of the GCC PandA C++ SSA plugin")
  AC_DEFINE_UNQUOTED(I386_GCC46_SSAVRP_PLUGIN, "${I386_GCC46_SSAVRP_PLUGIN}", "Define the filename of the GCC PandA SSAVRP plugin")
  AC_DEFINE_UNQUOTED(I386_GCC46_TOPFNAME_PLUGIN, "${I386_GCC46_TOPFNAME_PLUGIN}", "Define the filename of the GCC PandA topfname plugin")
  AC_DEFINE_UNQUOTED(I386_GCC46_VERSION, "${I386_GCC46_VERSION}", "Define the gcc version")
  AC_DEFINE_UNQUOTED(I386_GCC46_PLUGIN_COMPILER, "${I386_GCC46_PLUGIN_COMPILER}", "Define the plugin compiler")
fi

dnl switch back to old language
AC_LANG_POP([C])

])
dnl
dnl check gcc with plugin support enabled and plugins
dnl
AC_DEFUN([AC_CHECK_GCC47_I386_VERSION],[
    AC_ARG_WITH(gcc47,
    [  --with-gcc47=executable-path path where the GCC 4.7 is installed ],
    [
       ac_gcc47="$withval"
    ])

dnl switch to c
AC_LANG_PUSH([C])

if test "x$ac_gcc47" = x; then
   GCC_TO_BE_CHECKED="/usr/bin/gcc-4.7 /usr/bin/gcc"
else
   GCC_TO_BE_CHECKED=$ac_gcc47;
fi

echo "looking for gcc 4.7..."
for compiler in $GCC_TO_BE_CHECKED; do
   if test -f $compiler; then
      echo "checking $compiler..."
      dnl check for gcc
      I386_GCC47_VERSION=`$compiler -dumpspecs | grep \*version -A1 | tail -1`
      I386_GCC47_FULL_VERSION=`$compiler --version`
      AS_VERSION_COMPARE($1, [4.7.0], MIN_GCC47=[4.7.0], MIN_GCC47=$1, MIN_GCC47=$1)
      AS_VERSION_COMPARE([4.8.0], $2, MAX_GCC47=[4.8.0], MAX_GCC47=$2, MAX_GCC47=$2)
      AS_VERSION_COMPARE($I386_GCC47_VERSION, $MIN_GCC47, echo "checking $compiler >= $MIN_GCC47... no"; min=no, echo "checking $compiler >= $MIN_GCC47... yes"; min=yes, echo "checking $compiler >= $MIN_GCC47... yes"; min=yes)
      if test "$min" = "no" ; then
         continue;
      fi
      AS_VERSION_COMPARE($I386_GCC47_VERSION, $MAX_GCC47, echo "checking $compiler < $MAX_GCC47... yes"; max=yes, echo "checking $compiler < $MAX_GCC47... no"; max=no, echo "checking $compiler < $MAX_GCC47... no"; max=no)
      if test "$max" = "no" ; then
         continue;
      fi
      I386_GCC47_EXE=$compiler;
      I386_GCC47_PLUGIN_DIR=`$I386_GCC47_EXE -print-file-name=plugin`
      if test "x$I386_GCC47_PLUGIN_DIR" = "xplugin"; then
         echo "checking plugin support... no. Package gcc-4.7-plugin-dev missing?"
         break;
      fi
      echo "checking plugin directory...$I386_GCC47_PLUGIN_DIR"
      gcc_file=`basename $I386_GCC47_EXE`
      gcc_dir=`dirname $I386_GCC47_EXE`
      cpp=`echo $gcc_file | sed s/gcc/cpp/`
      I386_CPP47_EXE=$gcc_dir/$cpp
      if test -f $I386_CPP47_EXE; then
         echo "checking cpp...$I386_CPP47_EXE"
      else
         echo "checking cpp...no"
         I386_GCC47_EXE=""
         continue
      fi
      gpp=`echo $gcc_file | sed s/gcc/g\+\+/`
      I386_GPP47_EXE=$gcc_dir/$gpp
      if test -f $I386_GPP47_EXE; then
         echo "checking g++...$I386_GPP47_EXE"
      else
         echo "checking g++...no"
         continue
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_GCC47_EXE
      CFLAGS="-m32"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_GCC47_M32=yes,I386_GCC47_M32=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_GCC47_M32" == xyes; then
         AC_DEFINE(HAVE_I386_GCC47_M32,1,[Define if gcc 4.7 supports -m32 ])
         echo "checking support to -m32... yes"
      else
         echo "checking support to -m32... no"
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_GCC47_EXE
      CFLAGS="-mx32"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_GCC47_MX32=yes,I386_GCC47_MX32=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_GCC47_MX32" == xyes; then
         AC_DEFINE(HAVE_I386_GCC47_MX32,1,[Define if gcc 4.7 supports -mx32 ])
         echo "checking support to -mx32... yes"
      else
         echo "checking support to -mx32... no"
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_GCC47_EXE
      CFLAGS="-m64"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_GCC47_M64=yes,I386_GCC47_M64=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_GCC47_M64" == xyes; then
         AC_DEFINE(HAVE_I386_GCC47_M64,1,[Define if gcc 4.7 supports -m64 ])
         echo "checking support to -m64... yes"
      else
         echo "checking support to -m64... no"
      fi
      cat > plugin_test.c <<PLUGIN_TEST
      #include "plugin_includes.h"

      int plugin_is_GPL_compatible;

      extern struct cpp_reader *parse_in;

      void do_nothing(void * first, void * second)
      {
         cpp_define (parse_in, "TEST_GCC_PLUGIN=1");
      }

      int
      plugin_init (struct plugin_name_args * plugin_info, struct plugin_gcc_version * version)
      {
         if (!plugin_default_version_check(version, &gcc_version))
            return 1;
         register_callback("plugin_test", PLUGIN_START_UNIT, do_nothing, NULL);
         return 0;
      }
PLUGIN_TEST
      for plugin_compiler in $I386_GCC47_EXE $I386_GPP47_EXE; do
         if test -f plugin_test.so; then
            rm plugin_test.so
         fi
         $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ -fPIC -shared plugin_test.c -o plugin_test.so -I$I386_GCC47_PLUGIN_DIR/include 2> /dev/null
         if test ! -f plugin_test.so; then
            echo "checking $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ -fPIC -shared plugin_test.c -o plugin_test.so -I$I386_GCC47_PLUGIN_DIR/include... no"
            continue
         fi
         echo "checking $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ -fPIC -shared plugin_test.c -o plugin_test.so -I$I386_GCC47_PLUGIN_DIR/include... yes"
         ac_save_CC="$CC"
         ac_save_CFLAGS="$CFLAGS"
         CC=$plugin_compiler
         CFLAGS="-fplugin=$BUILDDIR/plugin_test.so"
         AC_LANG_PUSH([C])
         AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
               ]],[[
                  return 0;
               ]])],
         I386_GCC47_PLUGIN_COMPILER=$plugin_compiler,I386_GCC47_PLUGIN_COMPILER=)
         AC_LANG_POP([C])
         CC=$ac_save_CC
         CFLAGS=$ac_save_CFLAGS
         #If plugin compilation fails, skip this executable
         if test "x$I386_GCC47_PLUGIN_COMPILER" = x; then
            continue
         fi
         echo "Looking for gengtype"
         I386_GCC47_GENGTYPE=`$I386_GCC47_EXE -print-file-name=gengtype`
         if test "x$I386_GCC47_GENGTYPE" = "xgengtype"; then
            I386_GCC47_GENGTYPE=`$I386_GCC47_EXE -print-file-name=plugin/gengtype`
            if test "x$I386_GCC47_GENGTYPE" = "xplugin/gengtype"; then
               I386_GCC47_ROOT_DIR=`dirname $I386_GCC47_EXE`/..
               I386_GCC47_GENGTYPE=`find $I386_GCC47_ROOT_DIR -name gengtype | head -n1`
               if test "x$I386_GCC47_GENGTYPE" = "x"; then
                  I386_GCC47_PLUGIN_COMPILER=
                  continue
               fi
            fi
         fi
         echo "Looking for gtype.state"
         I386_GCC47_GTYPESTATE=`$I386_GCC47_EXE -print-file-name=gtype.state`
         if test "x$I386_GCC47_GTYPESTATE" = "xgtype.state"; then
            I386_GCC47_GTYPESTATE=`$I386_GCC47_EXE -print-file-name=plugin/gtype.state`
            if test "x$I386_GCC47_GTYPESTATE" = "xplugin/gtype.state"; then
               I386_GCC47_PLUGIN_COMPILER=
               continue
            fi
         fi
         echo "OK, we have found the compiler"
         build_I386_GCC47=yes;
         build_I386_GCC47_EMPTY_PLUGIN=yes;
         build_I386_GCC47_SSA_PLUGIN=yes;
         build_I386_GCC47_SSA_PLUGINCPP=yes;
         build_I386_GCC47_SSAVRP_PLUGIN=yes;
         build_I386_GCC47_TOPFNAME_PLUGIN=yes;
         break;
      done
      if test "x$I386_GCC47_PLUGIN_COMPILER" != x; then
         break;
      fi
   else
      echo "checking $compiler... not found"
   fi
done

if test x$I386_GCC47_PLUGIN_COMPILER != x; then
  dnl set configure and makefile variables
  I386_GCC47_EMPTY_PLUGIN=gcc47_plugin_dumpGimpleEmpty
  I386_GCC47_SSA_PLUGIN=gcc47_plugin_dumpGimpleSSA
  I386_GCC47_SSA_PLUGINCPP=gcc47_plugin_dumpGimpleSSACpp
  I386_GCC47_SSAVRP_PLUGIN=gcc47_plugin_dumpGimpleSSAVRP
  I386_GCC47_TOPFNAME_PLUGIN=gcc47_plugin_topfname
  AC_SUBST(I386_GCC47_EMPTY_PLUGIN)
  AC_SUBST(I386_GCC47_SSA_PLUGIN)
  AC_SUBST(I386_GCC47_SSA_PLUGINCPP)
  AC_SUBST(I386_GCC47_SSAVRP_PLUGIN)
  AC_SUBST(I386_GCC47_TOPFNAME_PLUGIN)
  AC_SUBST(I386_GCC47_PLUGIN_DIR)
  AC_SUBST(I386_GCC47_GENGTYPE)
  AC_SUBST(I386_GCC47_GTYPESTATE)
  AC_SUBST(I386_GCC47_EXE)
  AC_SUBST(I386_GCC47_VERSION)
  AC_SUBST(I386_GCC47_PLUGIN_COMPILER)
  AC_DEFINE(HAVE_I386_GCC47_COMPILER, 1, "Define if GCC 4.7 I386 compiler is compliant")
  AC_DEFINE_UNQUOTED(I386_GCC47_EXE, "${I386_GCC47_EXE}", "Define the plugin gcc")
  AC_DEFINE_UNQUOTED(I386_CPP47_EXE, "${I386_CPP47_EXE}", "Define the plugin cpp")
  AC_DEFINE_UNQUOTED(I386_GPP47_EXE, "${I386_GPP47_EXE}", "Define the plugin g++")
  AC_DEFINE_UNQUOTED(I386_GCC47_EMPTY_PLUGIN, "${I386_GCC47_EMPTY_PLUGIN}", "Define the filename of the GCC PandA Empty plugin")
  AC_DEFINE_UNQUOTED(I386_GCC47_SSA_PLUGIN, "${I386_GCC47_SSA_PLUGIN}", "Define the filename of the GCC PandA SSA plugin")
  AC_DEFINE_UNQUOTED(I386_GCC47_SSA_PLUGINCPP, "${I386_GCC47_SSA_PLUGINCPP}", "Define the filename of the GCC PandA C++ SSA plugin")
  AC_DEFINE_UNQUOTED(I386_GCC47_SSAVRP_PLUGIN, "${I386_GCC47_SSAVRP_PLUGIN}", "Define the filename of the GCC PandA SSAVRP plugin")
  AC_DEFINE_UNQUOTED(I386_GCC47_TOPFNAME_PLUGIN, "${I386_GCC47_TOPFNAME_PLUGIN}", "Define the filename of the GCC PandA topfname plugin")
  AC_DEFINE_UNQUOTED(I386_GCC47_VERSION, "${I386_GCC47_VERSION}", "Define the gcc version")
  AC_DEFINE_UNQUOTED(I386_GCC47_PLUGIN_COMPILER, "${I386_GCC47_PLUGIN_COMPILER}", "Define the plugin compiler")
fi

dnl switch back to old language
AC_LANG_POP([C])

])

dnl
dnl check gcc with plugin support enabled and plugins
dnl
AC_DEFUN([AC_CHECK_GCC48_I386_VERSION],[
    AC_ARG_WITH(gcc48,
    [  --with-gcc48=executable-path path where the GCC 4.8 is installed ],
    [
       ac_gcc48="$withval"
    ])

dnl switch to c
AC_LANG_PUSH([C])

if test "x$ac_gcc48" = x; then
   GCC_TO_BE_CHECKED="/usr/bin/gcc-4.8 /usr/bin/gcc"
else
   GCC_TO_BE_CHECKED=$ac_gcc48;
fi

echo "looking for gcc 4.8..."
for compiler in $GCC_TO_BE_CHECKED; do
   if test -f $compiler; then
      echo "checking $compiler..."
      dnl check for gcc
      I386_GCC48_VERSION=`$compiler -dumpspecs | grep \*version -A1 | tail -1`
      I386_GCC48_FULL_VERSION=`$compiler --version`
      AS_VERSION_COMPARE($1, [4.8.0], MIN_GCC48=[4.8.0], MIN_GCC48=$1, MIN_GCC48=$2)
      AS_VERSION_COMPARE([4.9.0], $2, MAX_GCC48=[4.9.0], MAX_GCC48=$2, MAX_GCC48=$2)
      AS_VERSION_COMPARE($I386_GCC48_VERSION, $MIN_GCC48, echo "checking $compiler >= $MIN_GCC48... no"; min=no, echo "checking $compiler >= $MIN_GCC48... yes"; min=yes, echo "checking $compiler >= $MIN_GCC48... yes"; min=yes)
      if test "$min" = "no" ; then
         continue;
      fi
      AS_VERSION_COMPARE($I386_GCC48_VERSION, $MAX_GCC48, echo "checking $compiler < $MAX_GCC48... yes"; max=yes, echo "checking $compiler < $MAX_GCC48... no"; max=no, echo "checking $compiler < $MAX_GCC48... no"; max=no)
      if test "$max" = "no" ; then
         continue;
      fi
      I386_GCC48_EXE=$compiler;
      I386_GCC48_PLUGIN_DIR=`$I386_GCC48_EXE -print-file-name=plugin`
      if test "x$I386_GCC48_PLUGIN_DIR" = "xplugin"; then
         echo "checking plugin support... no. Package gcc-4.8-plugin-dev missing?"
         break;
      fi
      echo "checking plugin directory...$I386_GCC48_PLUGIN_DIR"
      gcc_file=`basename $I386_GCC48_EXE`
      gcc_dir=`dirname $I386_GCC48_EXE`
      cpp=`echo $gcc_file | sed s/gcc/cpp/`
      I386_CPP48_EXE=$gcc_dir/$cpp
      if test -f $I386_CPP48_EXE; then
         echo "checking cpp...$I386_CPP48_EXE"
      else
         echo "checking cpp...no"
         I386_GCC48_EXE=""
         continue
      fi
      gpp=`echo $gcc_file | sed s/gcc/g\+\+/`
      I386_GPP48_EXE=$gcc_dir/$gpp
      if test -f $I386_GPP48_EXE; then
         echo "checking g++...$I386_GPP48_EXE"
      else
         echo "checking g++...no"
         continue
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_GCC48_EXE
      CFLAGS="-m32"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_GCC48_M32=yes,I386_GCC48_M32=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_GCC48_M32" == xyes; then
         AC_DEFINE(HAVE_I386_GCC48_M32,1,[Define if gcc 4.8 supports -m32 ])
         echo "checking support to -m32... yes"
      else
         echo "checking support to -m32... no"
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_GCC48_EXE
      CFLAGS="-mx32"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_GCC48_MX32=yes,I386_GCC48_MX32=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_GCC48_MX32" == xyes; then
         AC_DEFINE(HAVE_I386_GCC48_MX32,1,[Define if gcc 4.8 supports -mx32 ])
         echo "checking support to -mx32... yes"
      else
         echo "checking support to -mx32... no"
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_GCC48_EXE
      CFLAGS="-m64"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_GCC48_M64=yes,I386_GCC48_M64=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_GCC48_M64" == xyes; then
         AC_DEFINE(HAVE_I386_GCC48_M64,1,[Define if gcc 4.8 supports -m64 ])
         echo "checking support to -m64... yes"
      else
         echo "checking support to -m64... no"
      fi
      cat > plugin_test.c <<PLUGIN_TEST
      #include "plugin_includes.h"

      int plugin_is_GPL_compatible;

      extern struct cpp_reader *parse_in;

      void do_nothing(void * first, void * second)
      {
         cpp_define (parse_in, "TEST_GCC_PLUGIN=1");
      }

      int
      plugin_init (struct plugin_name_args * plugin_info, struct plugin_gcc_version * version)
      {
         if (!plugin_default_version_check(version, &gcc_version))
            return 1;
         register_callback("plugin_test", PLUGIN_START_UNIT, do_nothing, NULL);
         return 0;
      }
PLUGIN_TEST
      for plugin_compiler in $I386_GCC48_EXE $I386_GPP48_EXE; do
         if test -f plugin_test.so; then
            rm plugin_test.so
         fi
         $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ -fPIC -shared plugin_test.c -o plugin_test.so -I$I386_GCC48_PLUGIN_DIR/include 2> /dev/null
         if test ! -f plugin_test.so; then
            echo "checking $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ -fPIC -shared plugin_test.c -o plugin_test.so -I$I386_GCC48_PLUGIN_DIR/include... no"
            continue
         fi
         echo "checking $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ -fPIC -shared plugin_test.c -o plugin_test.so -I$I386_GCC48_PLUGIN_DIR/include... yes"
         ac_save_CC="$CC"
         ac_save_CFLAGS="$CFLAGS"
         CC=$plugin_compiler
         CFLAGS="-fplugin=$BUILDDIR/plugin_test.so"
         AC_LANG_PUSH([C])
         AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
               ]],[[
                  return 0;
               ]])],
         I386_GCC48_PLUGIN_COMPILER=$plugin_compiler,I386_GCC48_PLUGIN_COMPILER=)
         AC_LANG_POP([C])
         CC=$ac_save_CC
         CFLAGS=$ac_save_CFLAGS
         #If plugin compilation fails, skip this executable
         if test "x$I386_GCC48_PLUGIN_COMPILER" = x; then
            continue
         fi
         echo "Looking for gengtype"
         I386_GCC48_GENGTYPE=`$I386_GCC48_EXE -print-file-name=gengtype`
         if test "x$I386_GCC48_GENGTYPE" = "xgengtype"; then
            I386_GCC48_GENGTYPE=`$I386_GCC48_EXE -print-file-name=plugin/gengtype`
            if test "x$I386_GCC48_GENGTYPE" = "xplugin/gengtype"; then
               I386_GCC48_ROOT_DIR=`dirname $I386_GCC48_EXE`/..
               I386_GCC48_GENGTYPE=`find $I386_GCC48_ROOT_DIR -name gengtype | head -n1`
               if test "x$I386_GCC48_GENGTYPE" = "x"; then
                  I386_GCC48_PLUGIN_COMPILER=
                  continue
               fi
            fi
         fi
         echo "Looking for gtype.state"
         I386_GCC48_GTYPESTATE=`$I386_GCC48_EXE -print-file-name=gtype.state`
         if test "x$I386_GCC48_GTYPESTATE" = "xgtype.state"; then
            I386_GCC48_GTYPESTATE=`$I386_GCC48_EXE -print-file-name=plugin/gtype.state`
            if test "x$I386_GCC48_GTYPESTATE" = "xplugin/gtype.state"; then
               I386_GCC48_PLUGIN_COMPILER=
               continue
            fi
         fi
         echo "OK, we have found the compiler"
         build_I386_GCC48=yes;
         build_I386_GCC48_EMPTY_PLUGIN=yes;
         build_I386_GCC48_SSA_PLUGIN=yes;
         build_I386_GCC48_SSA_PLUGINCPP=yes;
         build_I386_GCC48_SSAVRP_PLUGIN=yes;
         build_I386_GCC48_TOPFNAME_PLUGIN=yes;
         break;
      done
      if test "x$I386_GCC48_PLUGIN_COMPILER" != x; then
         break;
      fi
   else
      echo "checking $compiler... not found"
   fi
done

if test x$I386_GCC48_PLUGIN_COMPILER != x; then
  dnl set configure and makefile variables
  I386_GCC48_EMPTY_PLUGIN=gcc48_plugin_dumpGimpleEmpty
  I386_GCC48_SSA_PLUGIN=gcc48_plugin_dumpGimpleSSA
  I386_GCC48_SSA_PLUGINCPP=gcc48_plugin_dumpGimpleSSACpp
  I386_GCC48_SSAVRP_PLUGIN=gcc48_plugin_dumpGimpleSSAVRP
  I386_GCC48_TOPFNAME_PLUGIN=gcc48_plugin_topfname
  AC_SUBST(I386_GCC48_EMPTY_PLUGIN)
  AC_SUBST(I386_GCC48_SSA_PLUGIN)
  AC_SUBST(I386_GCC48_SSA_PLUGINCPP)
  AC_SUBST(I386_GCC48_SSAVRP_PLUGIN)
  AC_SUBST(I386_GCC48_TOPFNAME_PLUGIN)
  AC_SUBST(I386_GCC48_PLUGIN_DIR)
  AC_SUBST(I386_GCC48_GENGTYPE)
  AC_SUBST(I386_GCC48_GTYPESTATE)
  AC_SUBST(I386_GCC48_EXE)
  AC_SUBST(I386_GCC48_VERSION)
  AC_SUBST(I386_GCC48_PLUGIN_COMPILER)
  AC_DEFINE(HAVE_I386_GCC48_COMPILER, 1, "Define if GCC 4.8 I386 compiler is compliant")
  AC_DEFINE_UNQUOTED(I386_GCC48_EXE, "${I386_GCC48_EXE}", "Define the plugin gcc")
  AC_DEFINE_UNQUOTED(I386_CPP48_EXE, "${I386_CPP48_EXE}", "Define the plugin cpp")
  AC_DEFINE_UNQUOTED(I386_GPP48_EXE, "${I386_GPP48_EXE}", "Define the plugin g++")
  AC_DEFINE_UNQUOTED(I386_GCC48_EMPTY_PLUGIN, "${I386_GCC48_EMPTY_PLUGIN}", "Define the filename of the GCC PandA Empty plugin")
  AC_DEFINE_UNQUOTED(I386_GCC48_SSA_PLUGIN, "${I386_GCC48_SSA_PLUGIN}", "Define the filename of the GCC PandA SSA plugin")
  AC_DEFINE_UNQUOTED(I386_GCC48_SSA_PLUGINCPP, "${I386_GCC48_SSA_PLUGINCPP}", "Define the filename of the GCC PandA C++ SSA plugin")
  AC_DEFINE_UNQUOTED(I386_GCC48_SSAVRP_PLUGIN, "${I386_GCC48_SSAVRP_PLUGIN}", "Define the filename of the GCC PandA SSAVRP plugin")
  AC_DEFINE_UNQUOTED(I386_GCC48_TOPFNAME_PLUGIN, "${I386_GCC48_TOPFNAME_PLUGIN}", "Define the filename of the GCC PandA topfname plugin")
  AC_DEFINE_UNQUOTED(I386_GCC48_VERSION, "${I386_GCC48_VERSION}", "Define the gcc version")
  AC_DEFINE_UNQUOTED(I386_GCC48_PLUGIN_COMPILER, "${I386_GCC48_PLUGIN_COMPILER}", "Define the plugin compiler")
fi

dnl switch back to old language
AC_LANG_POP([C])

])

dnl
dnl check gcc with plugin support enabled and plugins
dnl
AC_DEFUN([AC_CHECK_GCC49_I386_VERSION],[
    AC_ARG_WITH(gcc49,
    [  --with-gcc49=executable-path path where the GCC 4.9 is installed ],
    [
       ac_gcc49="$withval"
    ])

dnl switch to c
AC_LANG_PUSH([C])

if test "x$ac_gcc49" = x; then
   GCC_TO_BE_CHECKED="/usr/bin/gcc-4.9 /usr/local/bin/gcc-4.9 /usr/bin/gcc"
else
   GCC_TO_BE_CHECKED=$ac_gcc49;
fi

echo "looking for gcc 4.9..."
for compiler in $GCC_TO_BE_CHECKED; do
   if test -f $compiler; then
      echo "checking $compiler..."
      dnl check for gcc
      I386_GCC49_VERSION=`$compiler -dumpspecs | grep \*version -A1 | tail -1`
      I386_GCC49_FULL_VERSION=`$compiler --version`
      AS_VERSION_COMPARE($1, [4.9.0], MIN_GCC49=[4.9.0], MIN_GCC49=$1, MIN_GCC49=$1)
      AS_VERSION_COMPARE([5.0.0], $2, MAX_GCC49=[5.0.0], MAX_GCC49=$2, MAX_GCC49=$2)
      AS_VERSION_COMPARE($I386_GCC49_VERSION, $MIN_GCC49, echo "checking $compiler >= $MIN_GCC49... no"; min=no, echo "checking $compiler >= $MIN_GCC49... yes"; min=yes, echo "checking $compiler >= $MIN_GCC49... yes"; min=yes)
      if test "$min" = "no" ; then
         continue;
      fi
      AS_VERSION_COMPARE($I386_GCC49_VERSION, $MAX_GCC49, echo "checking $compiler < $MAX_GCC49... yes"; max=yes, echo "checking $compiler < $MAX_GCC49... no"; max=no, echo "checking $compiler < $MAX_GCC49... no"; max=no)
      if test "$max" = "no" ; then
         continue;
      fi
      I386_GCC49_EXE=$compiler;
      I386_GCC49_PLUGIN_DIR=`$I386_GCC49_EXE -print-file-name=plugin`
      if test "x$I386_GCC49_PLUGIN_DIR" = "xplugin"; then
         echo "checking plugin support... no. Package gcc-4.9-plugin-dev missing?"
         break;
      fi
      echo "checking plugin directory...$I386_GCC49_PLUGIN_DIR"
      gcc_file=`basename $I386_GCC49_EXE`
      gcc_dir=`dirname $I386_GCC49_EXE`
      cpp=`echo $gcc_file | sed s/gcc/cpp/`
      I386_CPP49_EXE=$gcc_dir/$cpp
      if test -f $I386_CPP49_EXE; then
         echo "checking cpp...$I386_CPP49_EXE"
      else
         echo "checking cpp...no"
         I386_GCC49_EXE=""
         continue
      fi
      gpp=`echo $gcc_file | sed s/gcc/g\+\+/`
      I386_GPP49_EXE=$gcc_dir/$gpp
      if test -f $I386_GPP49_EXE; then
         echo "checking g++...$I386_GPP49_EXE"
      else
         echo "checking g++...no"
         continue
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_GCC49_EXE
      CFLAGS="-m32"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_GCC49_M32=yes,I386_GCC49_M32=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_GCC49_M32" == xyes; then
         AC_DEFINE(HAVE_I386_GCC49_M32,1,[Define if gcc 4.9 supports -m32 ])
         echo "checking support to -m32... yes"
      else
         echo "checking support to -m32... no"
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_GCC49_EXE
      CFLAGS="-mx32"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_GCC49_MX32=yes,I386_GCC49_MX32=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_GCC49_MX32" == xyes; then
         AC_DEFINE(HAVE_I386_GCC49_MX32,1,[Define if gcc 4.9 supports -mx32 ])
         echo "checking support to -mx32... yes"
      else
         echo "checking support to -mx32... no"
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_GCC49_EXE
      CFLAGS="-m64"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_GCC49_M64=yes,I386_GCC49_M64=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_GCC49_M64" == xyes; then
         AC_DEFINE(HAVE_I386_GCC49_M64,1,[Define if gcc 4.9 supports -m64 ])
         echo "checking support to -m64... yes"
      else
         echo "checking support to -m64... no"
      fi
      cat > plugin_test.c <<PLUGIN_TEST
      #include "plugin_includes.h"

      int plugin_is_GPL_compatible;

      extern struct cpp_reader *parse_in;

      void do_nothing(void * first, void * second)
      {
         cpp_define (parse_in, "TEST_GCC_PLUGIN=1");
      }

      int
      plugin_init (struct plugin_name_args * plugin_info, struct plugin_gcc_version * version)
      {
         if (!plugin_default_version_check(version, &gcc_version))
            return 1;
         register_callback("plugin_test", PLUGIN_START_UNIT, do_nothing, NULL);
         return 0;
      }
PLUGIN_TEST
      for plugin_compiler in $I386_GCC49_EXE $I386_GPP49_EXE; do
         plugin_file_name=plugin_test.so
         plugin_option=
         case $host_os in
           mingw*) 
             plugin_option="-shared"
           ;;
           *)
             plugin_option='-fPIC -shared'
           ;;
         esac
         if test -f $plugin_file_name; then
            rm $plugin_file_name
         fi
         case $host_os in
           mingw*) 
             echo "$plugin_compiler -I$I386_GCC49_PLUGIN_DIR/include -I$TOPSRCDIR/etc/gcc_plugin/ -o plugin_test.o -c  plugin_test.c $plugin_option"
             $plugin_compiler -I$I386_GCC49_PLUGIN_DIR/include -I$TOPSRCDIR/etc/gcc_plugin/ -o plugin_test.o -c  plugin_test.c $plugin_option 2> /dev/null
             echo "flexlink -chain mingw -o $plugin_file_name  plugin_test.o"
             flexlink -chain mingw -o $plugin_file_name  plugin_test.o 2> /dev/null
           ;;
           *)
             $plugin_compiler -I$I386_GCC49_PLUGIN_DIR/include -I$TOPSRCDIR/etc/gcc_plugin/ -o $plugin_file_name  plugin_test.c $plugin_option 2> /dev/null
           ;;
         esac
         if test ! -f $plugin_file_name; then
            echo "checking $plugin_compiler -I$I386_GCC49_PLUGIN_DIR/include -I$TOPSRCDIR/etc/gcc_plugin/ -o $plugin_file_name  plugin_test.c $plugin_option ... no"
            continue
         fi
         echo "checking $plugin_compiler -I$I386_GCC49_PLUGIN_DIR/include -I$TOPSRCDIR/etc/gcc_plugin/ -o $plugin_file_name  plugin_test.c $plugin_option... yes"

         ac_save_CC="$CC"
         ac_save_CFLAGS="$CFLAGS"
         CC=$plugin_compiler
         CFLAGS="-fplugin=$BUILDDIR/$plugin_file_name"
         AC_LANG_PUSH([C])
         AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
               ]],[[
                  return 0;
               ]])],
         I386_GCC49_PLUGIN_COMPILER=$plugin_compiler,I386_GCC49_PLUGIN_COMPILER=)
         AC_LANG_POP([C])
         CC=$ac_save_CC
         CFLAGS=$ac_save_CFLAGS
         #If plugin compilation fails, skip this executable
         if test "x$I386_GCC49_PLUGIN_COMPILER" = x; then
            continue
         fi
         echo "Looking for gengtype"
         I386_GCC49_GENGTYPE=`$I386_GCC49_EXE -print-file-name=gengtype$ac_exeext`
         if test "x$I386_GCC49_GENGTYPE" = "xgengtype$ac_exeext"; then
            I386_GCC49_GENGTYPE=`$I386_GCC49_EXE -print-file-name=plugin/gengtype$ac_exeext`
            if test "x$I386_GCC49_GENGTYPE" = "xplugin/gengtype$ac_exeext"; then
               I386_GCC49_ROOT_DIR=`dirname $I386_GCC49_EXE`/..
               I386_GCC49_GENGTYPE=`find $I386_GCC49_ROOT_DIR -name gengtype$ac_exeext | head -n1`
               if test "x$I386_GCC49_GENGTYPE" = "x"; then
                  I386_GCC49_PLUGIN_COMPILER=
                  continue
               fi
            fi
         fi
         echo "Looking for gtype.state"
         I386_GCC49_GTYPESTATE=`$I386_GCC49_EXE -print-file-name=gtype.state`
         if test "x$I386_GCC49_GTYPESTATE" = "xgtype.state"; then
            I386_GCC49_GTYPESTATE=`$I386_GCC49_EXE -print-file-name=plugin/gtype.state`
            if test "x$I386_GCC49_GTYPESTATE" = "xplugin/gtype.state"; then
               I386_GCC49_PLUGIN_COMPILER=
               continue
            fi
         fi
         echo "OK, we have found the compiler"
         build_I386_GCC49=yes;
         build_I386_GCC49_EMPTY_PLUGIN=yes;
         build_I386_GCC49_SSA_PLUGIN=yes;
         build_I386_GCC49_SSA_PLUGINCPP=yes;
         build_I386_GCC49_TOPFNAME_PLUGIN=yes;
      done
      if test "x$I386_GCC49_PLUGIN_COMPILER" != x; then
         break;
      fi
   else
      echo "checking $compiler... not found"
   fi
done

if test x$I386_GCC49_PLUGIN_COMPILER != x; then
  dnl set configure and makefile variables
  I386_GCC49_EMPTY_PLUGIN=gcc49_plugin_dumpGimpleEmpty
  I386_GCC49_SSA_PLUGIN=gcc49_plugin_dumpGimpleSSA
  I386_GCC49_SSA_PLUGINCPP=gcc49_plugin_dumpGimpleSSACpp
  I386_GCC49_TOPFNAME_PLUGIN=gcc49_plugin_topfname
  AC_SUBST(I386_GCC49_EMPTY_PLUGIN)
  AC_SUBST(I386_GCC49_SSA_PLUGIN)
  AC_SUBST(I386_GCC49_SSA_PLUGINCPP)
  AC_SUBST(I386_GCC49_TOPFNAME_PLUGIN)
  AC_SUBST(I386_GCC49_PLUGIN_DIR)
  AC_SUBST(I386_GCC49_GENGTYPE)
  AC_SUBST(I386_GCC49_GTYPESTATE)
  AC_SUBST(I386_GCC49_EXE)
  AC_SUBST(I386_GCC49_VERSION)
  AC_SUBST(I386_GCC49_PLUGIN_COMPILER)
  AC_DEFINE(HAVE_I386_GCC49_COMPILER, 1, "Define if GCC 4.9 I386 compiler is compliant")
  AC_DEFINE_UNQUOTED(I386_GCC49_EXE, "${I386_GCC49_EXE}", "Define the plugin gcc")
  AC_DEFINE_UNQUOTED(I386_CPP49_EXE, "${I386_CPP49_EXE}", "Define the plugin cpp")
  AC_DEFINE_UNQUOTED(I386_GPP49_EXE, "${I386_GPP49_EXE}", "Define the plugin g++")
  AC_DEFINE_UNQUOTED(I386_GCC49_EMPTY_PLUGIN, "${I386_GCC49_EMPTY_PLUGIN}", "Define the filename of the GCC PandA Empty plugin")
  AC_DEFINE_UNQUOTED(I386_GCC49_SSA_PLUGIN, "${I386_GCC49_SSA_PLUGIN}", "Define the filename of the GCC PandA SSA plugin")
  AC_DEFINE_UNQUOTED(I386_GCC49_SSA_PLUGINCPP, "${I386_GCC49_SSA_PLUGINCPP}", "Define the filename of the GCC PandA C++ SSA plugin")
  AC_DEFINE_UNQUOTED(I386_GCC49_TOPFNAME_PLUGIN, "${I386_GCC49_TOPFNAME_PLUGIN}", "Define the filename of the GCC PandA topfname plugin")
  AC_DEFINE_UNQUOTED(I386_GCC49_VERSION, "${I386_GCC49_VERSION}", "Define the gcc version")
  AC_DEFINE_UNQUOTED(I386_GCC49_PLUGIN_COMPILER, "${I386_GCC49_PLUGIN_COMPILER}", "Define the plugin compiler")
fi

dnl switch back to old language
AC_LANG_POP([C])

])

dnl
dnl check gcc with plugin support enabled and plugins
dnl
AC_DEFUN([AC_CHECK_GCC5_I386_VERSION],[
    AC_ARG_WITH(gcc5,
    [  --with-gcc5=executable-path path where the GCC 5 is installed ],
    [
       ac_gcc5="$withval"
    ])

dnl switch to c
AC_LANG_PUSH([C])

if test "x$ac_gcc5" = x; then
   GCC_TO_BE_CHECKED="/usr/bin/gcc-5 /usr/local/bin/gcc-5 /usr/bin/gcc-5 /usr/bin/gcc"
else
   GCC_TO_BE_CHECKED=$ac_gcc5;
fi

echo "looking for gcc 5..."
for compiler in $GCC_TO_BE_CHECKED; do
   if test -f $compiler; then
      echo "checking $compiler..."
      dnl check for gcc
      I386_GCC5_VERSION=`$compiler -dumpspecs | grep \*version -A1 | tail -1`
      I386_GCC5_FULL_VERSION=`$compiler --version`
      AS_VERSION_COMPARE($1, [5.1.0], MIN_GCC5=[5.1.0], MIN_GCC5=$1, MIN_GCC5=$1)
      AS_VERSION_COMPARE([6.0.0], $2, MAX_GCC5=[6.0.0], MAX_GCC5=$2, MAX_GCC5=$2)
      AS_VERSION_COMPARE($I386_GCC5_VERSION, $MIN_GCC5, echo "checking $compiler >= $MIN_GCC5... no"; min=no, echo "checking $compiler >= $MIN_GCC5... yes"; min=yes, echo "checking $compiler >= $MIN_GCC5... yes"; min=yes)
      if test "$min" = "no" ; then
         continue;
      fi
      AS_VERSION_COMPARE($I386_GCC5_VERSION, $MAX_GCC5, echo "checking $compiler < $MAX_GCC5... yes"; max=yes, echo "checking $compiler < $MAX_GCC5... no"; max=no, echo "checking $compiler < $MAX_GCC5... no"; max=no)
      if test "$max" = "no" ; then
         continue;
      fi
      I386_GCC5_EXE=$compiler;
      I386_GCC5_PLUGIN_DIR=`$I386_GCC5_EXE -print-file-name=plugin`
      if test "x$I386_GCC5_PLUGIN_DIR" = "xplugin"; then
         echo "checking plugin support... no. Package gcc-5-plugin-dev missing?"
         break;
      fi
      echo "checking plugin directory...$I386_GCC5_PLUGIN_DIR"
      gcc_file=`basename $I386_GCC5_EXE`
      gcc_dir=`dirname $I386_GCC5_EXE`
      cpp=`echo $gcc_file | sed s/gcc/cpp/`
      I386_CPP5_EXE=$gcc_dir/$cpp
      if test -f $I386_CPP5_EXE; then
         echo "checking cpp...$I386_CPP5_EXE"
      else
         echo "checking cpp...no"
         I386_GCC5_EXE=""
         continue
      fi
      gpp=`echo $gcc_file | sed s/gcc/g\+\+/`
      I386_GPP5_EXE=$gcc_dir/$gpp
      if test -f $I386_GPP5_EXE; then
         echo "checking g++...$I386_GPP5_EXE"
      else
         echo "checking g++...no"
         continue
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_GCC5_EXE
      CFLAGS="-m32"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_GCC5_M32=yes,I386_GCC5_M32=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_GCC5_M32" == xyes; then
         AC_DEFINE(HAVE_I386_GCC5_M32,1,[Define if gcc 5 supports -m32 ])
         echo "checking support to -m32... yes"
      else
         echo "checking support to -m32... no"
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_GCC5_EXE
      CFLAGS="-mx32"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_GCC5_MX32=yes,I386_GCC5_MX32=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_GCC5_MX32" == xyes; then
         AC_DEFINE(HAVE_I386_GCC5_MX32,1,[Define if gcc 5 supports -mx32 ])
         echo "checking support to -mx32... yes"
      else
         echo "checking support to -mx32... no"
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_GCC5_EXE
      CFLAGS="-m64"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_GCC5_M64=yes,I386_GCC5_M64=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_GCC5_M64" == xyes; then
         AC_DEFINE(HAVE_I386_GCC5_M64,1,[Define if gcc 5 supports -m64 ])
         echo "checking support to -m64... yes"
      else
         echo "checking support to -m64... no"
      fi
      cat > plugin_test.c <<PLUGIN_TEST
      #include "plugin_includes.h"

      int plugin_is_GPL_compatible;

      extern struct cpp_reader *parse_in;

      void do_nothing(void * first, void * second)
      {
         cpp_define (parse_in, "TEST_GCC_PLUGIN=1");
      }

      int
      plugin_init (struct plugin_name_args * plugin_info, struct plugin_gcc_version * version)
      {
         if (!plugin_default_version_check(version, &gcc_version))
            return 1;
         register_callback("plugin_test", PLUGIN_START_UNIT, do_nothing, NULL);
         return 0;
      }
PLUGIN_TEST
      for plugin_compiler in $I386_GCC5_EXE $I386_GPP5_EXE; do
         if test -f plugin_test.so; then
            rm plugin_test.so
         fi
         $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ -fPIC -shared plugin_test.c -o plugin_test.so -I$I386_GCC5_PLUGIN_DIR/include 2> /dev/null
         if test ! -f plugin_test.so; then
            echo "checking $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ -fPIC -shared plugin_test.c -o plugin_test.so -I$I386_GCC5_PLUGIN_DIR/include... no"
            continue
         fi
         echo "checking $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ -fPIC -shared plugin_test.c -o plugin_test.so -I$I386_GCC5_PLUGIN_DIR/include... yes"
         ac_save_CC="$CC"
         ac_save_CFLAGS="$CFLAGS"
         CC=$plugin_compiler
         CFLAGS="-fplugin=$BUILDDIR/plugin_test.so"
         AC_LANG_PUSH([C])
         AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
               ]],[[
                  return 0;
               ]])],
         I386_GCC5_PLUGIN_COMPILER=$plugin_compiler,I386_GCC5_PLUGIN_COMPILER=)
         AC_LANG_POP([C])
         CC=$ac_save_CC
         CFLAGS=$ac_save_CFLAGS
         #If plugin compilation fails, skip this executable
         if test "x$I386_GCC5_PLUGIN_COMPILER" = x; then
            continue
         fi
         echo "Looking for gengtype"
         I386_GCC5_GENGTYPE=`$I386_GCC5_EXE -print-file-name=gengtype`
         if test "x$I386_GCC5_GENGTYPE" = "xgengtype"; then
            I386_GCC5_GENGTYPE=`$I386_GCC5_EXE -print-file-name=plugin/gengtype`
            if test "x$I386_GCC5_GENGTYPE" = "xplugin/gengtype"; then
               I386_GCC5_ROOT_DIR=`dirname $I386_GCC5_EXE`/..
               I386_GCC5_GENGTYPE=`find $I386_GCC5_ROOT_DIR -name gengtype | head -n1`
               if test "x$I386_GCC5_GENGTYPE" = "x"; then
                  I386_GCC5_PLUGIN_COMPILER=
                  continue
               fi
            fi
         fi
         echo "Looking for gtype.state"
         I386_GCC5_GTYPESTATE=`$I386_GCC5_EXE -print-file-name=gtype.state`
         if test "x$I386_GCC5_GTYPESTATE" = "xgtype.state"; then
            I386_GCC5_GTYPESTATE=`$I386_GCC5_EXE -print-file-name=plugin/gtype.state`
            if test "x$I386_GCC5_GTYPESTATE" = "xplugin/gtype.state"; then
               I386_GCC5_PLUGIN_COMPILER=
               continue
            fi
         fi
         echo "OK, we have found the compiler" 
         build_I386_GCC5=yes;
         build_I386_GCC5_EMPTY_PLUGIN=yes;
         build_I386_GCC5_SSA_PLUGIN=yes;
         build_I386_GCC5_SSA_PLUGINCPP=yes;
         build_I386_GCC5_TOPFNAME_PLUGIN=yes;
      done
      if test "x$I386_GCC5_PLUGIN_COMPILER" != x; then
         break;
      fi
   else
      echo "checking $compiler... not found"
   fi
done

if test x$I386_GCC5_PLUGIN_COMPILER != x; then
  dnl set configure and makefile variables
  I386_GCC5_EMPTY_PLUGIN=gcc5_plugin_dumpGimpleEmpty
  I386_GCC5_SSA_PLUGIN=gcc5_plugin_dumpGimpleSSA
  I386_GCC5_SSA_PLUGINCPP=gcc5_plugin_dumpGimpleSSACpp
  I386_GCC5_TOPFNAME_PLUGIN=gcc5_plugin_topfname
  AC_SUBST(I386_GCC5_EMPTY_PLUGIN)
  AC_SUBST(I386_GCC5_SSA_PLUGIN)
  AC_SUBST(I386_GCC5_SSA_PLUGINCPP)
  AC_SUBST(I386_GCC5_TOPFNAME_PLUGIN)
  AC_SUBST(I386_GCC5_PLUGIN_DIR)
  AC_SUBST(I386_GCC5_GENGTYPE)
  AC_SUBST(I386_GCC5_GTYPESTATE)
  AC_SUBST(I386_GCC5_EXE)
  AC_SUBST(I386_GCC5_VERSION)
  AC_SUBST(I386_GCC5_PLUGIN_COMPILER)
  AC_DEFINE(HAVE_I386_GCC5_COMPILER, 1, "Define if GCC 5 I386 compiler is compliant")
  AC_DEFINE_UNQUOTED(I386_GCC5_EXE, "${I386_GCC5_EXE}", "Define the plugin gcc")
  AC_DEFINE_UNQUOTED(I386_CPP5_EXE, "${I386_CPP5_EXE}", "Define the plugin cpp")
  AC_DEFINE_UNQUOTED(I386_GPP5_EXE, "${I386_GPP5_EXE}", "Define the plugin g++")
  AC_DEFINE_UNQUOTED(I386_GCC5_EMPTY_PLUGIN, "${I386_GCC5_EMPTY_PLUGIN}", "Define the filename of the GCC PandA Empty plugin")
  AC_DEFINE_UNQUOTED(I386_GCC5_SSA_PLUGIN, "${I386_GCC5_SSA_PLUGIN}", "Define the filename of the GCC PandA SSA plugin")
  AC_DEFINE_UNQUOTED(I386_GCC5_SSA_PLUGINCPP, "${I386_GCC5_SSA_PLUGINCPP}", "Define the filename of the GCC PandA C++ SSA plugin")
  AC_DEFINE_UNQUOTED(I386_GCC5_TOPFNAME_PLUGIN, "${I386_GCC5_TOPFNAME_PLUGIN}", "Define the filename of the GCC PandA topfname plugin")
  AC_DEFINE_UNQUOTED(I386_GCC5_VERSION, "${I386_GCC5_VERSION}", "Define the gcc version")
  AC_DEFINE_UNQUOTED(I386_GCC5_PLUGIN_COMPILER, "${I386_GCC5_PLUGIN_COMPILER}", "Define the plugin compiler")
fi

dnl switch back to old language
AC_LANG_POP([C])

])

dnl
dnl check gcc with plugin support enabled and plugins
dnl
AC_DEFUN([AC_CHECK_GCC6_I386_VERSION],[
    AC_ARG_WITH(gcc6,
    [  --with-gcc6=executable-path path where the GCC 6 is installed ],
    [
       ac_gcc6="$withval"
    ])

dnl switch to c
AC_LANG_PUSH([C])

if test "x$ac_gcc6" = x; then
   GCC_TO_BE_CHECKED="/usr/lib/gcc-snapshot/bin/gcc /usr/bin/gcc-6 /usr/local/bin/gcc-6 /usr/bin/gcc-6 /usr/bin/gcc"
else
   GCC_TO_BE_CHECKED=$ac_gcc6;
fi

echo "looking for gcc 6..."
for compiler in $GCC_TO_BE_CHECKED; do
   if test -f $compiler; then
      echo "checking $compiler..."
      dnl check for gcc
      I386_GCC6_VERSION=`$compiler -dumpspecs | grep \*version -A1 | tail -1`
      I386_GCC6_FULL_VERSION=`$compiler --version`
      AS_VERSION_COMPARE($1, [6.0.0], MIN_GCC6=[6.0.0], MIN_GCC6=$1, MIN_GCC6=$1)
      AS_VERSION_COMPARE([7.0.0], $2, MAX_GCC6=[7.0.0], MAX_GCC6=$2, MAX_GCC6=$2)
      AS_VERSION_COMPARE($I386_GCC6_VERSION, $MIN_GCC6, echo "checking $compiler >= $MIN_GCC6... no"; min=no, echo "checking $compiler >= $MIN_GCC6... yes"; min=yes, echo "checking $compiler >= $MIN_GCC6... yes"; min=yes)
      if test "$min" = "no" ; then
         continue;
      fi
      AS_VERSION_COMPARE($I386_GCC6_VERSION, $MAX_GCC6, echo "checking $compiler < $MAX_GCC6... yes"; max=yes, echo "checking $compiler < $MAX_GCC6... no"; max=no, echo "checking $compiler < $MAX_GCC6... no"; max=no)
      if test "$max" = "no" ; then
         continue;
      fi
      I386_GCC6_EXE=$compiler;
      I386_GCC6_PLUGIN_DIR=`$I386_GCC6_EXE -print-file-name=plugin`
      if test "x$I386_GCC6_PLUGIN_DIR" = "xplugin"; then
         echo "checking plugin support... no. Package gcc-6-plugin-dev missing?"
         break;
      fi
      echo "checking plugin directory...$I386_GCC6_PLUGIN_DIR"
      gcc_file=`basename $I386_GCC6_EXE`
      gcc_dir=`dirname $I386_GCC6_EXE`
      cpp=`echo $gcc_file | sed s/gcc/cpp/`
      I386_CPP6_EXE=$gcc_dir/$cpp
      if test -f $I386_CPP6_EXE; then
         echo "checking cpp...$I386_CPP6_EXE"
      else
         echo "checking cpp...no"
         I386_GCC6_EXE=""
         continue
      fi
      gpp=`echo $gcc_file | sed s/gcc/g\+\+/`
      I386_GPP6_EXE=$gcc_dir/$gpp
      if test -f $I386_GPP6_EXE; then
         echo "checking g++...$I386_GPP6_EXE"
      else
         echo "checking g++...no"
         continue
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_GCC6_EXE
      CFLAGS="-m32"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_GCC6_M32=yes,I386_GCC6_M32=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_GCC6_M32" == xyes; then
         AC_DEFINE(HAVE_I386_GCC6_M32,1,[Define if gcc 6 supports -m32 ])
         echo "checking support to -m32... yes"
      else
         echo "checking support to -m32... no"
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_GCC6_EXE
      CFLAGS="-mx32"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_GCC6_MX32=yes,I386_GCC6_MX32=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_GCC6_MX32" == xyes; then
         AC_DEFINE(HAVE_I386_GCC6_MX32,1,[Define if gcc 6 supports -mx32 ])
         echo "checking support to -mx32... yes"
      else
         echo "checking support to -mx32... no"
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_GCC6_EXE
      CFLAGS="-m64"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_GCC6_M64=yes,I386_GCC6_M64=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_GCC6_M64" == xyes; then
         AC_DEFINE(HAVE_I386_GCC6_M64,1,[Define if gcc 6 supports -m64 ])
         echo "checking support to -m64... yes"
      else
         echo "checking support to -m64... no"
      fi
      cat > plugin_test.c <<PLUGIN_TEST
      #include "plugin_includes.h"

      int plugin_is_GPL_compatible;

      extern struct cpp_reader *parse_in;

      void do_nothing(void * first, void * second)
      {
         cpp_define (parse_in, "TEST_GCC_PLUGIN=1");
      }

      int
      plugin_init (struct plugin_name_args * plugin_info, struct plugin_gcc_version * version)
      {
         if (!plugin_default_version_check(version, &gcc_version))
            return 1;
         register_callback("plugin_test", PLUGIN_START_UNIT, do_nothing, NULL);
         return 0;
      }
PLUGIN_TEST
      for plugin_compiler in $I386_GCC6_EXE $I386_GPP6_EXE; do
         if test -f plugin_test.so; then
            rm plugin_test.so
         fi
         $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ -fPIC -shared plugin_test.c -o plugin_test.so -I$I386_GCC6_PLUGIN_DIR/include 2> /dev/null
         if test ! -f plugin_test.so; then
            echo "checking $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ -fPIC -shared plugin_test.c -o plugin_test.so -I$I386_GCC6_PLUGIN_DIR/include... no"
            continue
         fi
         echo "checking $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ -fPIC -shared plugin_test.c -o plugin_test.so -I$I386_GCC6_PLUGIN_DIR/include... yes"
         ac_save_CC="$CC"
         ac_save_CFLAGS="$CFLAGS"
         CC=$plugin_compiler
         CFLAGS="-fplugin=$BUILDDIR/plugin_test.so"
         AC_LANG_PUSH([C])
         AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
               ]],[[
                  return 0;
               ]])],
         I386_GCC6_PLUGIN_COMPILER=$plugin_compiler,I386_GCC6_PLUGIN_COMPILER=)
         AC_LANG_POP([C])
         CC=$ac_save_CC
         CFLAGS=$ac_save_CFLAGS
         #If plugin compilation fails, skip this executable
         if test "x$I386_GCC6_PLUGIN_COMPILER" = x; then
            continue
         fi
         echo "Looking for gengtype"
         I386_GCC6_GENGTYPE=`$I386_GCC6_EXE -print-file-name=gengtype`
         if test "x$I386_GCC6_GENGTYPE" = "xgengtype"; then
            I386_GCC6_GENGTYPE=`$I386_GCC6_EXE -print-file-name=plugin/gengtype`
            if test "x$I386_GCC6_GENGTYPE" = "xplugin/gengtype"; then
               I386_GCC6_ROOT_DIR=`dirname $I386_GCC6_EXE`/..
               I386_GCC6_GENGTYPE=`find $I386_GCC6_ROOT_DIR -name gengtype | head -n1`
               if test "x$I386_GCC6_GENGTYPE" = "x"; then
                  I386_GCC6_PLUGIN_COMPILER=
                  continue
               fi
            fi
         fi
         echo "Looking for gtype.state"
         I386_GCC6_GTYPESTATE=`$I386_GCC6_EXE -print-file-name=gtype.state`
         if test "x$I386_GCC6_GTYPESTATE" = "xgtype.state"; then
            I386_GCC6_GTYPESTATE=`$I386_GCC6_EXE -print-file-name=plugin/gtype.state`
            if test "x$I386_GCC6_GTYPESTATE" = "xplugin/gtype.state"; then
               I386_GCC6_PLUGIN_COMPILER=
               continue
            fi
         fi
         echo "OK, we have found the compiler" 
         build_I386_GCC6=yes;
         build_I386_GCC6_EMPTY_PLUGIN=yes;
         build_I386_GCC6_SSA_PLUGIN=yes;
         build_I386_GCC6_SSA_PLUGINCPP=yes;
         build_I386_GCC6_TOPFNAME_PLUGIN=yes;
      done
      if test "x$I386_GCC6_PLUGIN_COMPILER" != x; then
         break;
      fi
   else
      echo "checking $compiler... not found"
   fi
done

if test x$I386_GCC6_PLUGIN_COMPILER != x; then
  dnl set configure and makefile variables
  I386_GCC6_EMPTY_PLUGIN=gcc6_plugin_dumpGimpleEmpty
  I386_GCC6_SSA_PLUGIN=gcc6_plugin_dumpGimpleSSA
  I386_GCC6_SSA_PLUGINCPP=gcc6_plugin_dumpGimpleSSACpp
  I386_GCC6_TOPFNAME_PLUGIN=gcc6_plugin_topfname
  AC_SUBST(I386_GCC6_EMPTY_PLUGIN)
  AC_SUBST(I386_GCC6_SSA_PLUGIN)
  AC_SUBST(I386_GCC6_SSA_PLUGINCPP)
  AC_SUBST(I386_GCC6_TOPFNAME_PLUGIN)
  AC_SUBST(I386_GCC6_PLUGIN_DIR)
  AC_SUBST(I386_GCC6_GENGTYPE)
  AC_SUBST(I386_GCC6_GTYPESTATE)
  AC_SUBST(I386_GCC6_EXE)
  AC_SUBST(I386_GCC6_VERSION)
  AC_SUBST(I386_GCC6_PLUGIN_COMPILER)
  AC_DEFINE(HAVE_I386_GCC6_COMPILER, 1, "Define if GCC 6 I386 compiler is compliant")
  AC_DEFINE_UNQUOTED(I386_GCC6_EXE, "${I386_GCC6_EXE}", "Define the plugin gcc")
  AC_DEFINE_UNQUOTED(I386_CPP6_EXE, "${I386_CPP6_EXE}", "Define the plugin cpp")
  AC_DEFINE_UNQUOTED(I386_GPP6_EXE, "${I386_GPP6_EXE}", "Define the plugin g++")
  AC_DEFINE_UNQUOTED(I386_GCC6_EMPTY_PLUGIN, "${I386_GCC6_EMPTY_PLUGIN}", "Define the filename of the GCC PandA Empty plugin")
  AC_DEFINE_UNQUOTED(I386_GCC6_SSA_PLUGIN, "${I386_GCC6_SSA_PLUGIN}", "Define the filename of the GCC PandA SSA plugin")
  AC_DEFINE_UNQUOTED(I386_GCC6_SSA_PLUGINCPP, "${I386_GCC6_SSA_PLUGINCPP}", "Define the filename of the GCC PandA C++ SSA plugin")
  AC_DEFINE_UNQUOTED(I386_GCC6_TOPFNAME_PLUGIN, "${I386_GCC6_TOPFNAME_PLUGIN}", "Define the filename of the GCC PandA topfname plugin")
  AC_DEFINE_UNQUOTED(I386_GCC6_VERSION, "${I386_GCC6_VERSION}", "Define the gcc version")
  AC_DEFINE_UNQUOTED(I386_GCC6_PLUGIN_COMPILER, "${I386_GCC6_PLUGIN_COMPILER}", "Define the plugin compiler")
fi

dnl switch back to old language
AC_LANG_POP([C])

])

dnl
dnl check gcc with plugin support enabled and plugins
dnl
AC_DEFUN([AC_CHECK_GCC7_I386_VERSION],[
    AC_ARG_WITH(gcc7,
    [  --with-gcc7=executable-path path where the GCC 7 is installed ],
    [
       ac_gcc7="$withval"
    ])

dnl switch to c
AC_LANG_PUSH([C])

if test "x$ac_gcc7" = x; then
   GCC_TO_BE_CHECKED="/usr/lib/gcc-snapshot/bin/gcc /usr/bin/gcc-7 /usr/local/bin/gcc-7 /usr/bin/gcc-7 /usr/bin/gcc"
else
   GCC_TO_BE_CHECKED=$ac_gcc7;
fi

echo "looking for gcc 7..."
for compiler in $GCC_TO_BE_CHECKED; do
   if test -f $compiler; then
      echo "checking $compiler..."
      dnl check for gcc
      I386_GCC7_VERSION=`$compiler -dumpspecs | grep \*version -A1 | tail -1`
      I386_GCC7_FULL_VERSION=`$compiler --version`
      AS_VERSION_COMPARE($1, [7.0.0], MIN_GCC7=[7.0.0], MIN_GCC7=$1, MIN_GCC7=$1)
      AS_VERSION_COMPARE([8.0.0], $2, MAX_GCC7=[8.0.0], MAX_GCC7=$2, MAX_GCC7=$2)
      AS_VERSION_COMPARE($I386_GCC7_VERSION, $MIN_GCC7, echo "checking $compiler >= $MIN_GCC7... no"; min=no, echo "checking $compiler >= $MIN_GCC7... yes"; min=yes, echo "checking $compiler >= $MIN_GCC7... yes"; min=yes)
      if test "$min" = "no" ; then
         continue;
      fi
      AS_VERSION_COMPARE($I386_GCC7_VERSION, $MAX_GCC7, echo "checking $compiler < $MAX_GCC7... yes"; max=yes, echo "checking $compiler < $MAX_GCC7... no"; max=no, echo "checking $compiler < $MAX_GCC7... no"; max=no)
      if test "$max" = "no" ; then
         continue;
      fi
      I386_GCC7_EXE=$compiler;
      I386_GCC7_PLUGIN_DIR=`$I386_GCC7_EXE -print-file-name=plugin`
      if test "x$I386_GCC7_PLUGIN_DIR" = "xplugin"; then
         echo "checking plugin support... no. Package gcc-7-plugin-dev missing?"
         break;
      fi
      echo "checking plugin directory...$I386_GCC7_PLUGIN_DIR"
      gcc_file=`basename $I386_GCC7_EXE`
      gcc_dir=`dirname $I386_GCC7_EXE`
      cpp=`echo $gcc_file | sed s/gcc/cpp/`
      I386_CPP7_EXE=$gcc_dir/$cpp
      if test -f $I386_CPP7_EXE; then
         echo "checking cpp...$I386_CPP7_EXE"
      else
         echo "checking cpp...no"
         I386_GCC7_EXE=""
         continue
      fi
      gpp=`echo $gcc_file | sed s/gcc/g\+\+/`
      I386_GPP7_EXE=$gcc_dir/$gpp
      if test -f $I386_GPP7_EXE; then
         echo "checking g++...$I386_GPP7_EXE"
      else
         echo "checking g++...no"
         continue
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_GCC7_EXE
      CFLAGS="-m32"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_GCC7_M32=yes,I386_GCC7_M32=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_GCC7_M32" == xyes; then
         AC_DEFINE(HAVE_I386_GCC7_M32,1,[Define if gcc 7 supports -m32 ])
         echo "checking support to -m32... yes"
      else
         echo "checking support to -m32... no"
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_GCC7_EXE
      CFLAGS="-mx32"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_GCC7_MX32=yes,I386_GCC7_MX32=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_GCC7_MX32" == xyes; then
         AC_DEFINE(HAVE_I386_GCC7_MX32,1,[Define if gcc 7 supports -mx32 ])
         echo "checking support to -mx32... yes"
      else
         echo "checking support to -mx32... no"
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_GCC7_EXE
      CFLAGS="-m64"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_GCC7_M64=yes,I386_GCC7_M64=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_GCC7_M64" == xyes; then
         AC_DEFINE(HAVE_I386_GCC7_M64,1,[Define if gcc 7 supports -m64 ])
         echo "checking support to -m64... yes"
      else
         echo "checking support to -m64... no"
      fi
      cat > plugin_test.c <<PLUGIN_TEST
      #include "plugin_includes.h"

      int plugin_is_GPL_compatible;

      extern struct cpp_reader *parse_in;

      void do_nothing(void * first, void * second)
      {
         cpp_define (parse_in, "TEST_GCC_PLUGIN=1");
      }

      int
      plugin_init (struct plugin_name_args * plugin_info, struct plugin_gcc_version * version)
      {
         if (!plugin_default_version_check(version, &gcc_version))
            return 1;
         register_callback("plugin_test", PLUGIN_START_UNIT, do_nothing, NULL);
         return 0;
      }
PLUGIN_TEST
      for plugin_compiler in $I386_GCC7_EXE $I386_GPP7_EXE; do
         if test -f plugin_test.so; then
            rm plugin_test.so
         fi
         $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ -fPIC -shared plugin_test.c -o plugin_test.so -I$I386_GCC7_PLUGIN_DIR/include 2> /dev/null
         if test ! -f plugin_test.so; then
            echo "checking $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ -fPIC -shared plugin_test.c -o plugin_test.so -I$I386_GCC7_PLUGIN_DIR/include... no"
            continue
         fi
         echo "checking $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ -fPIC -shared plugin_test.c -o plugin_test.so -I$I386_GCC7_PLUGIN_DIR/include... yes"
         ac_save_CC="$CC"
         ac_save_CFLAGS="$CFLAGS"
         CC=$plugin_compiler
         CFLAGS="-fplugin=$BUILDDIR/plugin_test.so"
         AC_LANG_PUSH([C])
         AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
               ]],[[
                  return 0;
               ]])],
         I386_GCC7_PLUGIN_COMPILER=$plugin_compiler,I386_GCC7_PLUGIN_COMPILER=)
         AC_LANG_POP([C])
         CC=$ac_save_CC
         CFLAGS=$ac_save_CFLAGS
         #If plugin compilation fails, skip this executable
         if test "x$I386_GCC7_PLUGIN_COMPILER" = x; then
            continue
         fi
         echo "Looking for gengtype"
         I386_GCC7_GENGTYPE=`$I386_GCC7_EXE -print-file-name=gengtype`
         if test "x$I386_GCC7_GENGTYPE" = "xgengtype"; then
            I386_GCC7_GENGTYPE=`$I386_GCC7_EXE -print-file-name=plugin/gengtype`
            if test "x$I386_GCC7_GENGTYPE" = "xplugin/gengtype"; then
               I386_GCC7_ROOT_DIR=`dirname $I386_GCC7_EXE`/..
               I386_GCC7_GENGTYPE=`find $I386_GCC7_ROOT_DIR -name gengtype | head -n1`
               if test "x$I386_GCC7_GENGTYPE" = "x"; then
                  I386_GCC7_PLUGIN_COMPILER=
                  continue
               fi
            fi
         fi
         echo "Looking for gtype.state"
         I386_GCC7_GTYPESTATE=`$I386_GCC7_EXE -print-file-name=gtype.state`
         if test "x$I386_GCC7_GTYPESTATE" = "xgtype.state"; then
            I386_GCC7_GTYPESTATE=`$I386_GCC7_EXE -print-file-name=plugin/gtype.state`
            if test "x$I386_GCC7_GTYPESTATE" = "xplugin/gtype.state"; then
               I386_GCC7_PLUGIN_COMPILER=
               continue
            fi
         fi
         echo "OK, we have found the compiler"
         build_I386_GCC7=yes;
         build_I386_GCC7_EMPTY_PLUGIN=yes;
         build_I386_GCC7_SSA_PLUGIN=yes;
         build_I386_GCC7_SSA_PLUGINCPP=yes;
         build_I386_GCC7_TOPFNAME_PLUGIN=yes;
      done
      if test "x$I386_GCC7_PLUGIN_COMPILER" != x; then
         break;
      fi
   else
      echo "checking $compiler... not found"
   fi
done

if test x$I386_GCC7_PLUGIN_COMPILER != x; then
  dnl set configure and makefile variables
  I386_GCC7_EMPTY_PLUGIN=gcc7_plugin_dumpGimpleEmpty
  I386_GCC7_SSA_PLUGIN=gcc7_plugin_dumpGimpleSSA
  I386_GCC7_SSA_PLUGINCPP=gcc7_plugin_dumpGimpleSSACpp
  I386_GCC7_TOPFNAME_PLUGIN=gcc7_plugin_topfname
  AC_SUBST(I386_GCC7_EMPTY_PLUGIN)
  AC_SUBST(I386_GCC7_SSA_PLUGIN)
  AC_SUBST(I386_GCC7_SSA_PLUGINCPP)
  AC_SUBST(I386_GCC7_TOPFNAME_PLUGIN)
  AC_SUBST(I386_GCC7_PLUGIN_DIR)
  AC_SUBST(I386_GCC7_GENGTYPE)
  AC_SUBST(I386_GCC7_GTYPESTATE)
  AC_SUBST(I386_GCC7_EXE)
  AC_SUBST(I386_GCC7_VERSION)
  AC_SUBST(I386_GCC7_PLUGIN_COMPILER)
  AC_DEFINE(HAVE_I386_GCC7_COMPILER, 1, "Define if GCC 7 I386 compiler is compliant")
  AC_DEFINE_UNQUOTED(I386_GCC7_EXE, "${I386_GCC7_EXE}", "Define the plugin gcc")
  AC_DEFINE_UNQUOTED(I386_CPP7_EXE, "${I386_CPP7_EXE}", "Define the plugin cpp")
  AC_DEFINE_UNQUOTED(I386_GPP7_EXE, "${I386_GPP7_EXE}", "Define the plugin g++")
  AC_DEFINE_UNQUOTED(I386_GCC7_EMPTY_PLUGIN, "${I386_GCC7_EMPTY_PLUGIN}", "Define the filename of the GCC PandA Empty plugin")
  AC_DEFINE_UNQUOTED(I386_GCC7_SSA_PLUGIN, "${I386_GCC7_SSA_PLUGIN}", "Define the filename of the GCC PandA SSA plugin")
  AC_DEFINE_UNQUOTED(I386_GCC7_SSA_PLUGINCPP, "${I386_GCC7_SSA_PLUGINCPP}", "Define the filename of the GCC PandA C++ SSA plugin")
  AC_DEFINE_UNQUOTED(I386_GCC7_TOPFNAME_PLUGIN, "${I386_GCC7_TOPFNAME_PLUGIN}", "Define the filename of the GCC PandA topfname plugin")
  AC_DEFINE_UNQUOTED(I386_GCC7_VERSION, "${I386_GCC7_VERSION}", "Define the gcc version")
  AC_DEFINE_UNQUOTED(I386_GCC7_PLUGIN_COMPILER, "${I386_GCC7_PLUGIN_COMPILER}", "Define the plugin compiler")
fi

dnl switch back to old language
AC_LANG_POP([C])

])

dnl
dnl check gcc with plugin support enabled and plugins
dnl
AC_DEFUN([AC_CHECK_GCC8_I386_VERSION],[
    AC_ARG_WITH(gcc8,
    [  --with-gcc8=executable-path path where the GCC 8 is installed ],
    [
       ac_gcc8="$withval"
    ])

dnl switch to c
AC_LANG_PUSH([C])

if test "x$ac_gcc8" = x; then
   GCC_TO_BE_CHECKED="/usr/lib/gcc-snapshot/bin/gcc /usr/bin/gcc-8 /usr/local/bin/gcc-8 /usr/bin/gcc-8 /usr/bin/gcc"
else
   GCC_TO_BE_CHECKED=$ac_gcc8;
fi

echo "looking for gcc 8..."
for compiler in $GCC_TO_BE_CHECKED; do
   if test -f $compiler; then
      echo "checking $compiler..."
      dnl check for gcc
      I386_GCC8_VERSION=`$compiler -dumpspecs | grep \*version -A1 | tail -1`
      I386_GCC8_FULL_VERSION=`$compiler --version`
      AS_VERSION_COMPARE($1, [8.0.0], MIN_GCC8=[8.0.0], MIN_GCC8=$1, MIN_GCC8=$1)
      AS_VERSION_COMPARE([9.0.0], $2, MAX_GCC8=[9.0.0], MAX_GCC8=$2, MAX_GCC8=$2)
      AS_VERSION_COMPARE($I386_GCC8_VERSION, $MIN_GCC8, echo "checking $compiler >= $MIN_GCC8... no"; min=no, echo "checking $compiler >= $MIN_GCC8... yes"; min=yes, echo "checking $compiler >= $MIN_GCC8... yes"; min=yes)
      if test "$min" = "no" ; then
         continue;
      fi
      AS_VERSION_COMPARE($I386_GCC8_VERSION, $MAX_GCC8, echo "checking $compiler < $MAX_GCC8... yes"; max=yes, echo "checking $compiler < $MAX_GCC8... no"; max=no, echo "checking $compiler < $MAX_GCC8... no"; max=no)
      if test "$max" = "no" ; then
         continue;
      fi
      I386_GCC8_EXE=$compiler;
      I386_GCC8_PLUGIN_DIR=`$I386_GCC8_EXE -print-file-name=plugin`
      if test "x$I386_GCC8_PLUGIN_DIR" = "xplugin"; then
         echo "checking plugin support... no. Package gcc-8-plugin-dev missing?"
         break;
      fi
      echo "checking plugin directory...$I386_GCC8_PLUGIN_DIR"
      gcc_file=`basename $I386_GCC8_EXE`
      gcc_dir=`dirname $I386_GCC8_EXE`
      cpp=`echo $gcc_file | sed s/gcc/cpp/`
      I386_CPP8_EXE=$gcc_dir/$cpp
      if test -f $I386_CPP8_EXE; then
         echo "checking cpp...$I386_CPP8_EXE"
      else
         echo "checking cpp...no"
         I386_GCC8_EXE=""
         continue
      fi
      gpp=`echo $gcc_file | sed s/gcc/g\+\+/`
      I386_GPP8_EXE=$gcc_dir/$gpp
      if test -f $I386_GPP8_EXE; then
         echo "checking g++...$I386_GPP8_EXE"
      else
         echo "checking g++...no"
         continue
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_GCC8_EXE
      CFLAGS="-m32"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_GCC8_M32=yes,I386_GCC8_M32=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_GCC8_M32" == xyes; then
         AC_DEFINE(HAVE_I386_GCC8_M32,1,[Define if gcc 8 supports -m32 ])
         echo "checking support to -m32... yes"
      else
         echo "checking support to -m32... no"
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_GCC8_EXE
      CFLAGS="-mx32"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_GCC8_MX32=yes,I386_GCC8_MX32=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_GCC8_MX32" == xyes; then
         AC_DEFINE(HAVE_I386_GCC8_MX32,1,[Define if gcc 8 supports -mx32 ])
         echo "checking support to -mx32... yes"
      else
         echo "checking support to -mx32... no"
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_GCC8_EXE
      CFLAGS="-m64"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_GCC8_M64=yes,I386_GCC8_M64=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_GCC8_M64" == xyes; then
         AC_DEFINE(HAVE_I386_GCC8_M64,1,[Define if gcc 8 supports -m64 ])
         echo "checking support to -m64... yes"
      else
         echo "checking support to -m64... no"
      fi
      cat > plugin_test.c <<PLUGIN_TEST
      #include "plugin_includes.h"

      int plugin_is_GPL_compatible;

      extern struct cpp_reader *parse_in;

      void do_nothing(void * first, void * second)
      {
         cpp_define (parse_in, "TEST_GCC_PLUGIN=1");
      }

      int
      plugin_init (struct plugin_name_args * plugin_info, struct plugin_gcc_version * version)
      {
         if (!plugin_default_version_check(version, &gcc_version))
            return 1;
         register_callback("plugin_test", PLUGIN_START_UNIT, do_nothing, NULL);
         return 0;
      }
PLUGIN_TEST
      for plugin_compiler in $I386_GCC8_EXE $I386_GPP8_EXE; do
         if test -f plugin_test.so; then
            rm plugin_test.so
         fi
         plugin_option=
         case $host_os in
           mingw*) 
             plugin_option="-shared -Wl,--export-all-symbols $I386_GCC8_PLUGIN_DIR/cc1plus.exe.a"
           ;;
           darwin*)
             plugin_option='-fPIC -shared -undefined dynamic_lookup'
           ;;
           *)
             plugin_option='-fPIC -shared'
           ;;
         esac
         $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ plugin_test.c -o plugin_test.so -I$I386_GCC8_PLUGIN_DIR/include $plugin_option 2> /dev/null
         if test ! -f plugin_test.so; then
            echo "checking $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ plugin_test.c -o plugin_test.so -I$I386_GCC8_PLUGIN_DIR/include $plugin_option ... no"
            continue
         fi
         echo "checking $plugin_compiler -I$TOPSRCDIR/etc/gcc_plugin/ plugin_test.c -o plugin_test.so -I$I386_GCC8_PLUGIN_DIR/include $plugin_option... yes"
         ac_save_CC="$CC"
         ac_save_CFLAGS="$CFLAGS"
         CC=$plugin_compiler
         CFLAGS="-fplugin=$BUILDDIR/plugin_test.so"
         AC_LANG_PUSH([C])
         AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
               ]],[[
                  return 0;
               ]])],
         I386_GCC8_PLUGIN_COMPILER=$plugin_compiler,I386_GCC8_PLUGIN_COMPILER=)
         AC_LANG_POP([C])
         CC=$ac_save_CC
         CFLAGS=$ac_save_CFLAGS
         #If plugin compilation fails, skip this executable
         if test "x$I386_GCC8_PLUGIN_COMPILER" = x; then
            continue
         fi
         echo "Looking for gengtype"
         I386_GCC8_GENGTYPE=`$I386_GCC8_EXE -print-file-name=gengtype$ac_exeext`
         if test "x$I386_GCC8_GENGTYPE" = "xgengtype$ac_exeext"; then
            I386_GCC8_GENGTYPE=`$I386_GCC8_EXE -print-file-name=plugin/gengtype$ac_exeext`
            if test "x$I386_GCC8_GENGTYPE" = "xplugin/gengtype$ac_exeext"; then
               I386_GCC8_ROOT_DIR=`dirname $I386_GCC8_EXE`/..
               I386_GCC8_GENGTYPE=`find $I386_GCC8_ROOT_DIR -name gengtype$ac_exeext | head -n1`
               if test "x$I386_GCC8_GENGTYPE" = "x"; then
                  I386_GCC8_PLUGIN_COMPILER=
                  continue
               fi
            fi
         fi
         echo "Looking for gtype.state"
         I386_GCC8_GTYPESTATE=`$I386_GCC8_EXE -print-file-name=gtype.state`
         if test "x$I386_GCC8_GTYPESTATE" = "xgtype.state"; then
            I386_GCC8_GTYPESTATE=`$I386_GCC8_EXE -print-file-name=plugin/gtype.state`
            if test "x$I386_GCC8_GTYPESTATE" = "xplugin/gtype.state"; then
               I386_GCC8_PLUGIN_COMPILER=
               continue
            fi
         fi
         echo "OK, we have found the compiler"
         build_I386_GCC8=yes;
         build_I386_GCC8_EMPTY_PLUGIN=yes;
         build_I386_GCC8_SSA_PLUGIN=yes;
         build_I386_GCC8_SSA_PLUGINCPP=yes;
         build_I386_GCC8_TOPFNAME_PLUGIN=yes;
      done
      if test "x$I386_GCC8_PLUGIN_COMPILER" != x; then
         break;
      fi
   else
      echo "checking $compiler... not found"
   fi
done

if test x$I386_GCC8_PLUGIN_COMPILER != x; then
  dnl set configure and makefile variables
  I386_GCC8_EMPTY_PLUGIN=gcc8_plugin_dumpGimpleEmpty
  I386_GCC8_SSA_PLUGIN=gcc8_plugin_dumpGimpleSSA
  I386_GCC8_SSA_PLUGINCPP=gcc8_plugin_dumpGimpleSSACpp
  I386_GCC8_TOPFNAME_PLUGIN=gcc8_plugin_topfname
  AC_SUBST(I386_GCC8_EMPTY_PLUGIN)
  AC_SUBST(I386_GCC8_SSA_PLUGIN)
  AC_SUBST(I386_GCC8_SSA_PLUGINCPP)
  AC_SUBST(I386_GCC8_TOPFNAME_PLUGIN)
  AC_SUBST(I386_GCC8_PLUGIN_DIR)
  AC_SUBST(I386_GCC8_GENGTYPE)
  AC_SUBST(I386_GCC8_GTYPESTATE)
  AC_SUBST(I386_GCC8_EXE)
  AC_SUBST(I386_GCC8_VERSION)
  AC_SUBST(I386_GCC8_PLUGIN_COMPILER)
  AC_DEFINE(HAVE_I386_GCC8_COMPILER, 1, "Define if GCC 8 I386 compiler is compliant")
  AC_DEFINE_UNQUOTED(I386_GCC8_EXE, "${I386_GCC8_EXE}", "Define the plugin gcc")
  AC_DEFINE_UNQUOTED(I386_CPP8_EXE, "${I386_CPP8_EXE}", "Define the plugin cpp")
  AC_DEFINE_UNQUOTED(I386_GPP8_EXE, "${I386_GPP8_EXE}", "Define the plugin g++")
  AC_DEFINE_UNQUOTED(I386_GCC8_EMPTY_PLUGIN, "${I386_GCC8_EMPTY_PLUGIN}", "Define the filename of the GCC PandA Empty plugin")
  AC_DEFINE_UNQUOTED(I386_GCC8_SSA_PLUGIN, "${I386_GCC8_SSA_PLUGIN}", "Define the filename of the GCC PandA SSA plugin")
  AC_DEFINE_UNQUOTED(I386_GCC8_SSA_PLUGINCPP, "${I386_GCC8_SSA_PLUGINCPP}", "Define the filename of the GCC PandA C++ SSA plugin")
  AC_DEFINE_UNQUOTED(I386_GCC8_TOPFNAME_PLUGIN, "${I386_GCC8_TOPFNAME_PLUGIN}", "Define the filename of the GCC PandA topfname plugin")
  AC_DEFINE_UNQUOTED(I386_GCC8_VERSION, "${I386_GCC8_VERSION}", "Define the gcc version")
  AC_DEFINE_UNQUOTED(I386_GCC8_PLUGIN_COMPILER, "${I386_GCC8_PLUGIN_COMPILER}", "Define the plugin compiler")
fi

dnl switch back to old language
AC_LANG_POP([C])

])

dnl
dnl check gcc with plugin support enabled and plugins
dnl
AC_DEFUN([AC_CHECK_GCC_ARM_VERSION],[

echo "checking for arm cross compiler..."
dnl the directory where plugins will be put
if test "$prefix" != "NONE"; then
   ARM_PLUGIN_DIR=$prefix/gcc_plugins
else
   ARM_PLUGIN_DIR=/opt/gcc_plugins
fi

dnl checking for host gcc-4.5
AC_CHECK_PROG(ARM_HOST_PLUGIN_COMPILER, gcc-4.5, gcc-4.5,)
if test "x$ARM_HOST_PLUGIN_COMPILER" = x; then
   AC_MSG_ERROR("gcc-4.5 not found - arm plugins cannot be compiled)
fi


dnl switch to c
AC_LANG_PUSH([C])

dnl the list of gccs to be tested
GCC_TO_BE_CHECKED="$ac_arm_gcc /opt/x-tools/arm/bin/armeb-unknown-linux-gnueabi-gcc"

for compiler in $GCC_TO_BE_CHECKED; do
   if test -f $compiler; then
      echo "checking $compiler..."
      dnl check for minimum gcc
      ARM_GCC_VERSION=`$compiler -dumpspecs | grep \*version -A1 | tail -1`
      AS_VERSION_COMPARE($ARM_GCC_VERSION, $1, echo "checking $compiler >= $1... no"; min=no, echo "checking $compiler >= $1... yes"; min=yes, echo "checking $compiler >= $1... yes"; min=yes)
      if test "$min" = "no" ; then
         continue;
      fi
      AS_VERSION_COMPARE($ARM_GCC_VERSION, $2, echo "checking $compiler < $2... yes"; max=yes, echo "checking $compiler < $2... no"; max=no, echo "checking $compiler < $2... no"; max=no)
      if test "$max" = "no" ; then
         continue;
      fi
      ARM_GCC_VERSION=`echo $ARM_GCC_VERSION | sed 's/\./_/g'`
      echo "version is $ARM_GCC_VERSION"
      ARM_GCC_EXE=$compiler;
      ARM_GCC_PLUGIN_DIR=`$ARM_GCC_EXE -print-file-name=plugin`
      if test "x$ARM_GCC_PLUGIN_DIR" = "xplugin"; then
         ARM_GCC_EXE=""
         continue
      fi
      echo "checking plugin directory...$ARM_GCC_PLUGIN_DIR"
      ARM_GCC_CONFIGURE=`$TOPSRCDIR/../etc/macros/get_configure.sh $ARM_GCC_EXE`
      echo "checking arm gcc configure... $ARM_GCC_CONFIGURE"
      ARM_GCC_TARGET=`$TOPSRCDIR/../etc/macros/get_target.sh $ARM_GCC_EXE`
      echo "checking arm gcc target... $ARM_GCC_TARGET"
      ARM_GCC_SRC_DIR=`dirname $ARM_GCC_CONFIGURE`
      ARM_GCC_BUILT_DIR=$ARM_GCC_SRC_DIR/../../$ARM_GCC_TARGET/build/build-cc-final
      echo "checking arm gcc build... $ARM_GCC_BUILT_DIR"
      if test -d $ARM_GCC_SRC_DIR; then
         echo "checking arm gcc source directory...$ARM_GCC_SRC_DIR"
      else
         echo "checking arm gcc source directory...not found"
         ARM_GCC_EXE=""
         continue
      fi
      arm_gcc_file=`basename $ARM_GCC_EXE`
      arm_gcc_dir=`dirname $ARM_GCC_EXE`
      arm_cpp=`echo $arm_gcc_file | sed s/gcc/cpp/`
      ARM_CPP_EXE=$arm_gcc_dir/$arm_cpp
      if test -f $ARM_CPP_EXE; then
         echo "checking cpp...$ARM_CPP_EXE"
      else
         echo "checking cpp...no"
         ARM_GCC_EXE=""
         continue
      fi
      arm_gpp=`echo $arm_gcc_file | sed s/gcc/gpp/`
      ARM_GPP_EXE=$arm_gcc_dir/$arm_gpp
      if test -f $ARM_GPP_EXE; then
         echo "checking g++...$ARM_GPP_EXE"
         break
      else
         echo "checking g++...no"
         break
      fi
   else
      echo "checking $compiler... not found"
   fi
done

dnl if gcc has not been found, create in /opt/gcc-4.5.2
if test "x$ARM_GCC_EXE" = x; then
  build_ARM_GCC=yes;
  ARM_GCC_EXE=/opt/x-tools/arm/bin/armeb-unknown-linux-gnueabi-gcc
  ARM_CPP_EXE=/opt/x-tools/arm/bin/armeb-unknown-linux-gnueabi-cpp
  ARM_GCC_PLUGIN_DIR=/opt/x-tools/arm/lib/gcc/armeb-unknown-linux-gnueabi/4.5.2/plugin
  ARM_GCC_SRC_DIR=/opt/x-tools/build_arm/src/gcc-4.5.2
  ARM_GCC_BUILT_DIR=/opt/x-tools/build_arm/armeb-unknown-linux-gnueabi/build/build-cc-final
  ARM_GCC_VERSION=4_5_2
fi

dnl set configure and makefile variables
build_ARM_EMPTY_PLUGIN=yes;
build_ARM_SSA_PLUGIN=yes;
build_ARM_SSA_PLUGINCPP=yes;
build_ARM_RTL_PLUGIN=yes;
ARM_EMPTY_PLUGIN=ARM_plugin_dumpGimpleEmpty
ARM_SSA_PLUGIN=ARM_plugin_dumpGimpleSSA
ARM_SSA_PLUGINCPP=ARM_plugin_dumpGimpleSSACpp
ARM_RTL_PLUGIN=ARM_plugin_dumpRTL
AC_SUBST(ARM_EMPTY_PLUGIN)
AC_SUBST(ARM_SSA_PLUGIN)
AC_SUBST(ARM_SSA_PLUGINCPP)
AC_SUBST(ARM_RTL_PLUGIN)
AC_SUBST(ARM_GCC_PLUGIN_DIR)
AC_SUBST(ARM_GCC_EXE)
AC_SUBST(ARM_GCC_SRC_DIR)
AC_SUBST(ARM_PLUGIN_DIR)
AC_SUBST(ARM_GCC_VERSION)
AC_SUBST(ARM_GCC_BUILT_DIR)
AC_DEFINE(HAVE_ARM_COMPILER, 1, "Define if ARM cross compiler is already present")
AC_DEFINE_UNQUOTED(ARM_GCC_EXE, "${ARM_GCC_EXE}", "Define the plugin gcc")
AC_DEFINE_UNQUOTED(ARM_CPP_EXE, "${ARM_CPP_EXE}", "Define the plugin cpp")
AC_DEFINE_UNQUOTED(ARM_GPP_EXE, "${ARM_GPP_EXE}", "Define the plugin g++")
AC_DEFINE_UNQUOTED(ARM_EMPTY_PLUGIN, "${ARM_EMPTY_PLUGIN}", "Define the filename of the GCC PandA Empty plugin")
AC_DEFINE_UNQUOTED(ARM_SSA_PLUGIN, "${ARM_SSA_PLUGIN}", "Define the filename of the GCC PandA SSA plugin")
AC_DEFINE_UNQUOTED(ARM_SSA_PLUGINCPP, "${ARM_SSA_PLUGINCPP}", "Define the filename of the GCC PandA C++ SSA plugin")
AC_DEFINE_UNQUOTED(ARM_RTL_PLUGIN, "${ARM_RTL_PLUGIN}", "Define the filename of the GCC PandA RTL plugin")
AC_DEFINE_UNQUOTED(ARM_GCC_VERSION, "${ARM_GCC_VERSION}", "Define the arm gcc version")

])

dnl
dnl check gcc with plugin support enabled and plugins
dnl
AC_DEFUN([AC_CHECK_GCC_SPARC_VERSION],[

echo "checking for sparc cross compiler..."
dnl the directory where plugins will be put
if test "$prefix" != "NONE"; then
   SPARC_PLUGIN_DIR=$prefix/gcc_plugins
else
   SPARC_PLUGIN_DIR=/opt/gcc_plugins
fi

dnl checking for host gcc-4.5
AC_CHECK_PROG(SPARC_HOST_PLUGIN_COMPILER, gcc-4.5, gcc-4.5,)
if test "x$SPARC_HOST_PLUGIN_COMPILER" = x; then
   AC_MSG_ERROR("gcc-4.5 not found - sparc plugins cannot be compiled)
fi

dnl switch to c
AC_LANG_PUSH([C])

dnl the list of gccs to be tested
GCC_TO_BE_CHECKED="$ac_sparc_gcc /opt/x-tools/sparc/bin/sparc-unknown-linux-gnu-gcc"

for compiler in $GCC_TO_BE_CHECKED; do
   if test -f $compiler; then
      echo "checking $compiler..."
      dnl check for minimum gcc
      SPARC_GCC_VERSION=`$compiler -dumpspecs | grep \*version -A1 | tail -1`
      AS_VERSION_COMPARE($SPARC_GCC_VERSION, $1, echo "checking $compiler >= $1... no"; min=no, echo "checking $compiler >= $1... yes"; min=yes, echo "checking $compiler >= $1... yes"; min=yes)
      if test "$min" = "no" ; then
         continue;
      fi
      AS_VERSION_COMPARE($SPARC_GCC_VERSION, $2, echo "checking $compiler < $2... yes"; max=yes, echo "checking $compiler < $2... no"; max=no, echo "checking $compiler < $2... no"; max=no)
      if test "$max" = "no" ; then
         continue;
      fi
      SPARC_GCC_EXE=$compiler;
      SPARC_GCC_PLUGIN_DIR=`$SPARC_GCC_EXE -print-file-name=plugin`
      if test "x$SPARC_GCC_PLUGIN_DIR" = "xplugin"; then
         SPARC_GCC_EXE=""
         continue
      fi
      SPARC_GCC_VERSION=`echo $SPARC_GCC_VERSION | sed 's/\./_/g'`
      echo "version is $SPARC_GCC_VERSION"
      echo "checking plugin directory...$SPARC_GCC_PLUGIN_DIR"
      SPARC_GCC_CONFIGURE=`$TOPSRCDIR/../etc/macros/get_configure.sh $SPARC_GCC_EXE`
      echo "checking sparc gcc configure... $SPARC_GCC_CONFIGURE"
      SPARC_GCC_TARGET=`$TOPSRCDIR/../etc/macros/get_target.sh $SPARC_GCC_EXE`
      echo "checking sparc gcc target... $SPARC_GCC_TARGET"
      SPARC_GCC_SRC_DIR=`dirname $SPARC_GCC_CONFIGURE`
      SPARC_GCC_BUILT_DIR=$SPARC_GCC_SRC_DIR/../../$SPARC_GCC_TARGET/build/build-cc-final
      echo "checking sparc gcc build... $SPARC_GCC_BUILT_DIR"
      if test -d $SPARC_GCC_SRC_DIR; then
         echo "checking sparc gcc source directory...$SPARC_GCC_SRC_DIR"
      else
         echo "checking sparc gcc source directory...not found"
         SPARC_GCC_EXE=""
         continue
      fi
      sparc_gcc_file=`basename $SPARC_GCC_EXE`
      sparc_gcc_dir=`dirname $SPARC_GCC_EXE`
      sparc_cpp=`echo $sparc_gcc_file | sed s/gcc/cpp/`
      SPARC_CPP_EXE=$sparc_gcc_dir/$sparc_cpp
      if test -f $SPARC_CPP_EXE; then
         echo "checking cpp...$SPARC_CPP_EXE"
      else
         echo "checking cpp...no"
         SPARC_GCC_EXE=""
         continue
      fi
      sparc_gpp=`echo $sparc_gcc_file | sed s/gcc/gpp/`
      SPARC_GPP_EXE=$sparc_gcc_dir/$sparc_gpp
      if test -f $SPARC_GPP_EXE; then
         echo "checking gpp...$SPARC_GPP_EXE"
         break
      else
         echo "checking gpp...no"
         break
      fi
   else
      echo "checking $compiler... not found"
   fi
done

dnl if gcc has not been found, create in /opt/gcc-4.5.2
if test "x$SPARC_GCC_EXE" = x; then
  build_SPARC_GCC=yes;
  SPARC_GCC_EXE=/opt/x-tools/sparc/bin/sparc-unknown-linux-gnu-gcc
  SPARC_CPP_EXE=/opt/x-tools/sparc/bin/sparc-unknown-linux-gnu-cpp
  SPARC_GCC_PLUGIN_DIR=/opt/x-tools/sparc/lib/gcc/sparc-unknown-linux-gnu/4.5.2/plugin
  SPARC_GCC_SRC_DIR=/opt/x-tools/build_sparc/src/gcc-4.5.2
  SPARC_GCC_BUILT_DIR=/opt/x-tools/build_sparc/sparc-unknown-linux-gnu/build/build-cc-final
  SPARC_GCC_VERSION=4_5_2
fi

dnl set configure and makefile variables
build_SPARC_EMPTY_PLUGIN=yes;
build_SPARC_SSA_PLUGIN=yes;
build_SPARC_SSA_PLUGINCPP=yes;
build_SPARC_RTL_PLUGIN=yes;
SPARC_EMPTY_PLUGIN=SPARC_plugin_dumpGimpleEmpty
SPARC_SSA_PLUGIN=SPARC_plugin_dumpGimpleSSA
SPARC_SSA_PLUGINCPP=SPARC_plugin_dumpGimpleSSACpp
SPARC_RTL_PLUGIN=SPARC_plugin_dumpRTL
AC_SUBST(SPARC_EMPTY_PLUGIN)
AC_SUBST(SPARC_SSA_PLUGIN)
AC_SUBST(SPARC_SSA_PLUGINCPP)
AC_SUBST(SPARC_RTL_PLUGIN)
AC_SUBST(SPARC_GCC_PLUGIN_DIR)
AC_SUBST(SPARC_GCC_EXE)
AC_SUBST(SPARC_GCC_SRC_DIR)
AC_SUBST(SPARC_PLUGIN_DIR)
AC_SUBST(SPARC_GCC_VERSION)
AC_SUBST(SPARC_GCC_BUILT_DIR)
AC_DEFINE(HAVE_SPARC_COMPILER, 1, "Define if the sparc cross compiler exists")
AC_DEFINE_UNQUOTED(SPARC_GCC_EXE, "${SPARC_GCC_EXE}", "Define the plugin GCC")
AC_DEFINE_UNQUOTED(SPARC_CPP_EXE, "${SPARC_CPP_EXE}", "Define the plugin CPP")
AC_DEFINE_UNQUOTED(SPARC_GPP_EXE, "${SPARC_GPP_EXE}", "Define the plugin GPP")
AC_DEFINE_UNQUOTED(SPARC_EMPTY_PLUGIN, "${SPARC_EMPTY_PLUGIN}", "Define the filename of the GCC PandA Empty plugin")
AC_DEFINE_UNQUOTED(SPARC_SSA_PLUGIN, "${SPARC_SSA_PLUGIN}", "Define the filename of the GCC PandA SSA plugin")
AC_DEFINE_UNQUOTED(SPARC_SSA_PLUGINCPP, "${SPARC_SSA_PLUGINCPP}", "Define the filename of the GCC PandA C++ SSA plugin")
AC_DEFINE_UNQUOTED(SPARC_RTL_PLUGIN, "${SPARC_RTL_PLUGIN}", "Define the filename of the GCC PandA RTL plugin")
AC_DEFINE_UNQUOTED(SPARC_GCC_VERSION, "${SPARC_GCC_VERSION}", "Define the sparc gcc version")


])

#
# Check if gcc supports -Wpedantic
#
AC_DEFUN([AC_COMPILE_WPEDANTIC], [
  AC_CACHE_CHECK(if g++ supports -Wpedantic,
  ac_cv_cxx_compile_wpedantic_cxx,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  ac_save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -Wpedantic"
  AC_TRY_COMPILE([
  int a;],,
  ac_cv_cxx_compile_wpedantic_cxx=yes, ac_cv_cxx_compile_wpedantic_cxx=no)
  CXXFLAGS="$ac_save_CXXFLAGS"
  AC_LANG_RESTORE
  ])

  if test "$ac_cv_cxx_compile_wpedantic_cxx" = yes; then
    AC_DEFINE(WPEDANTIC,1,[Define if g++ supports -Wpedantic ])
    panda_WPEDANTIC=yes
  fi
])



dnl
dnl checks if the plugin directory exists and is writable
dnl
AC_DEFUN([AC_CHECK_GCC_PLUGIN_DIR],[

if test "$prefix" != "NONE"; then
   GCC_PLUGIN_DIR=$prefix/gcc_plugins
else
   GCC_PLUGIN_DIR=/opt/gcc_plugins
fi

AC_DEFINE_UNQUOTED(GCC_PLUGIN_DIR, "${GCC_PLUGIN_DIR}", "Define the plugin dir")
AC_SUBST(GCC_PLUGIN_DIR)

])

dnl
dnl checks if the plugin directory exists and is writable
dnl
AC_DEFUN([AC_CHECK_CLANG_PLUGIN_DIR],[

if test "$prefix" != "NONE"; then
   CLANG_PLUGIN_DIR=$prefix/gcc_plugins
else
   CLANG_PLUGIN_DIR=/opt/gcc_plugins
fi

AC_DEFINE_UNQUOTED(CLANG_PLUGIN_DIR, "${CLANG_PLUGIN_DIR}", "Define the plugin dir")
AC_SUBST(CLANG_PLUGIN_DIR)

])


