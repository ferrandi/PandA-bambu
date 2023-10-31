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
                  # Try to find gengtype in the gcc install dir based on libgcc
                  I386_GCC49_LIBGCC=`$I386_GCC49_EXE -print-libgcc-file-name`
                  I386_GCC49_ROOT_DIR=`dirname $I386_GCC49_LIBGCC`/../../../..
                  I386_GCC49_GENGTYPE=`find $I386_GCC49_ROOT_DIR -name gengtype$ac_exeext | head -n1`
                  if test "x$I386_GCC49_GENGTYPE" = "x"; then
                     I386_GCC49_PLUGIN_COMPILER=
                     continue
                  fi
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
                  # Try to find gengtype in the gcc install dir based on libgcc
                  I386_GCC5_LIBGCC=`$I386_GCC5_EXE -print-libgcc-file-name`
                  I386_GCC5_ROOT_DIR=`dirname $I386_GCC5_LIBGCC`/../../../..
                  I386_GCC5_GENGTYPE=`find $I386_GCC5_ROOT_DIR -name gengtype | head -n1`
                  if test "x$I386_GCC5_GENGTYPE" = "x"; then
                     I386_GCC5_PLUGIN_COMPILER=
                     continue
                  fi
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
                  # Try to find gengtype in the gcc install dir based on libgcc
                  I386_GCC6_LIBGCC=`$I386_GCC6_EXE -print-libgcc-file-name`
                  I386_GCC6_ROOT_DIR=`dirname $I386_GCC6_LIBGCC`/../../../..
                  I386_GCC6_GENGTYPE=`find $I386_GCC6_ROOT_DIR -name gengtype | head -n1`
                  if test "x$I386_GCC6_GENGTYPE" = "x"; then
                     I386_GCC6_PLUGIN_COMPILER=
                     continue
                  fi
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
                  # Try to find gengtype in the gcc install dir based on libgcc
                  I386_GCC7_LIBGCC=`$I386_GCC7_EXE -print-libgcc-file-name`
                  I386_GCC7_ROOT_DIR=`dirname $I386_GCC7_LIBGCC`/../../../..
                  I386_GCC7_GENGTYPE=`find $I386_GCC7_ROOT_DIR -name gengtype | head -n1`
                  if test "x$I386_GCC7_GENGTYPE" = "x"; then
                     I386_GCC7_PLUGIN_COMPILER=
                     continue
                  fi
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
                  # Try to find gengtype in the gcc install dir based on libgcc
                  I386_GCC8_LIBGCC=`$I386_GCC8_EXE -print-libgcc-file-name`
                  I386_GCC8_ROOT_DIR=`dirname $I386_GCC8_LIBGCC`/../../../..
                  I386_GCC8_GENGTYPE=`find $I386_GCC8_ROOT_DIR -name gengtype$ac_exeext | head -n1`
                  if test "x$I386_GCC8_GENGTYPE" = "x"; then
                     I386_GCC8_PLUGIN_COMPILER=
                     continue
                  fi
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


