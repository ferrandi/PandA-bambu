dnl
dnl check gcc with plugin support enabled and plugins
dnl ARGS:
dnl   gcc_exe
dnl   gmin_ver
dnl   gmax_ver
dnl
AC_DEFUN([ACX_PROG_GCC],[
AC_REQUIRE([AC_PROG_AWK])
AC_REQUIRE([AC_PROG_SED])
m4_define([gcc_id], m4_esyscmd([echo -n $1 | sed -e 's/-//;s/\.//']))
m4_define([gcc_macro_prefix], m4_toupper(I386_[]gcc_id))
m4_define([req_version], m4_esyscmd([echo -n $1 | tr -d "gcc-" | awk -F. '{printf $(1)"."($(2)?$(2):0)"."($(3)?$(3):0)}']))
m4_define([max_version], m4_esyscmd([echo -n $1 | tr -d "gcc-" | awk -F. '{printf $(1)"."($(2)?$(2):0)"."($(3)?$(3):0)}' | awk -F. '{if($(2) && $(2) < 9) {printf $(1)"."$(2)+1"."0;} else {printf $(1)+1"."0"."0;}}']))
m4_define([req_version_pretty], m4_esyscmd([echo -n $1 | tr -d "gcc-"]))
AC_ARG_WITH(gcc_id,
   [AS_HELP_STRING([--with-]gcc_id, [absolute path for the gcc ]req_version_pretty[ executable])],
   [exe_to_check="$withval"],
   [exe_to_check="$1 gcc"])
AC_LANG_PUSH([C])

echo "looking for gcc []req_version_pretty[]..."
for compiler in $exe_to_check; do
   if command -v "$compiler" > /dev/null; then
      echo "checking $compiler..."
      GCC_VERSION=`$compiler -dumpspecs | grep \*version -A1 | tail -1`
      AS_VERSION_COMPARE($2, [req_version], MIN_GCC=[req_version], MIN_GCC=$2, MIN_GCC=$2)
      AS_VERSION_COMPARE([max_version], $3, MAX_GCC=[max_version], MAX_GCC=$3, MAX_GCC=$3)
      AS_VERSION_COMPARE($GCC_VERSION, $MIN_GCC, echo "checking $compiler >= $MIN_GCC... no"; min=no, echo "checking $compiler >= $MIN_GCC... yes"; min=yes, echo "checking $compiler >= $MIN_GCC... yes"; min=yes)
      if test "$min" = "no" ; then
         continue;
      fi
      AS_VERSION_COMPARE($GCC_VERSION, $MAX_GCC, echo "checking $compiler < $MAX_GCC... yes"; max=yes, echo "checking $compiler < $MAX_GCC... no"; max=no, echo "checking $compiler < $MAX_GCC... no"; max=no)
      if test "$max" = "no" ; then
         continue;
      fi
      GCC_EXE=$compiler;
      GCC_PLUGIN_INC=`$GCC_EXE -print-file-name=plugin`
      if test "x$GCC_PLUGIN_INC" = "xplugin"; then
         echo "checking plugin support... no. Package gcc-[]req_version_pretty[]-plugin-dev missing?"
         break;
      fi
      echo "checking plugin directory...$GCC_PLUGIN_INC"
      CPP_EXE=`echo $GCC_EXE | sed s/gcc/cpp/`
      if command -v "$CPP_EXE" > /dev/null; then
         echo "checking cpp...$CPP_EXE"
      else
         echo "checking cpp...no"
         GCC_EXE=""
         continue
      fi
      GPP_EXE=`echo $GCC_EXE | sed s/gcc/g\+\+/`
      if command -v "$GPP_EXE" > /dev/null; then
         echo "checking g++...$GPP_EXE"
      else
         echo "checking g++...no"
         continue
      fi
      AC_CACHE_CHECK([$compiler supports -m32],
         ax_cv_[]gcc_id[]_m32,
         [
         ac_save_CC="$CC"
         ac_save_CFLAGS="$CFLAGS"
         ac_save_LDFLAGS="$LDFLAGS"
         ac_save_LIBS="$LIBS"
         CC=$GCC_EXE
         CFLAGS="-m32"
         LDFLAGS=
         LIBS=
         AC_LANG_PUSH([C])
         AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],ax_cv_[]gcc_id[]_m32=yes,ax_cv_[]gcc_id[]_m32=no)
         AC_LANG_POP([C])
         CC=$ac_save_CC
         CFLAGS=$ac_save_CFLAGS
         LDFLAGS=$ac_save_LDFLAGS
         LIBS=$ac_save_LIBS
      ])
      if test "x$ax_cv_[]gcc_id[]_m32" == xyes; then
         AC_DEFINE(HAVE_[]gcc_macro_prefix[]_M32,1,[Define if gcc []req_version_pretty[] supports -m32 ])
      fi
      AC_CACHE_CHECK([$compiler supports -mx32],
         ax_cv_[]gcc_id[]_mx32,
         [
         ac_save_CC="$CC"
         ac_save_CFLAGS="$CFLAGS"
         ac_save_LDFLAGS="$LDFLAGS"
         ac_save_LIBS="$LIBS"
         CC=$GCC_EXE
         CFLAGS="-mx32"
         LDFLAGS=
         LIBS=
         AC_LANG_PUSH([C])
         AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],ax_cv_[]gcc_id[]_mx32=yes,ax_cv_[]gcc_id[]_mx32=no)
         AC_LANG_POP([C])
         CC=$ac_save_CC
         CFLAGS=$ac_save_CFLAGS
         LDFLAGS=$ac_save_LDFLAGS
         LIBS=$ac_save_LIBS
      ])
      if test "x$ax_cv_[]gcc_id[]_mx32" == xyes; then
         AC_DEFINE(HAVE_[]gcc_macro_prefix[]_MX32,1,[Define if gcc []req_version_pretty[] supports -mx32 ])
      fi
      AC_CACHE_CHECK([$compiler supports -m64],
      ax_cv_[]gcc_id[]_m64,
      [
         ac_save_CC="$CC"
         ac_save_CFLAGS="$CFLAGS"
         ac_save_LDFLAGS="$LDFLAGS"
         ac_save_LIBS="$LIBS"
         CC=$GCC_EXE
         CFLAGS="-m64"
         LDFLAGS=
         LIBS=
         AC_LANG_PUSH([C])
         AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],ax_cv_[]gcc_id[]_m64=yes,ax_cv_[]gcc_id[]_m64=no)
         AC_LANG_POP([C])
         CC=$ac_save_CC
         CFLAGS=$ac_save_CFLAGS
         LDFLAGS=$ac_save_LDFLAGS
         LIBS=$ac_save_LIBS
      ])
      if test "x$ax_cv_[]gcc_id[]_m64" == xyes; then
         AC_DEFINE(HAVE_[]gcc_macro_prefix[]_M64,1,[Define if gcc []req_version_pretty[] supports -m64 ])
      fi
      AC_CACHE_CHECK([$compiler plugin gengtype], 
         ax_cv_[]gcc_id[]_gengtype,
         [
         ax_cv_[]gcc_id[]_gengtype=`$GCC_EXE -print-file-name=gengtype$ac_exeext`
         if test "x$ax_cv_[]gcc_id[]_gengtype" = "xgengtype$ac_exeext"; then
            ax_cv_[]gcc_id[]_gengtype=`$GCC_EXE -print-file-name=plugin/gengtype$ac_exeext`
            if test "x$ax_cv_[]gcc_id[]_gengtype" = "xplugin/gengtype$ac_exeext"; then
               GCC_ROOT_DIR=`dirname $GCC_EXE`/..
               ax_cv_[]gcc_id[]_gengtype=`find $GCC_ROOT_DIR -name gengtype$ac_exeext | head -n1`
               if test "x$ax_cv_[]gcc_id[]_gengtype" = "x"; then
                  # Try to find gengtype in the gcc install dir based on libgcc
                  GCC_LIBGCC=`$GCC_EXE -print-libgcc-file-name`
                  GCC_ROOT_DIR=`dirname $GCC_LIBGCC`/../../../..
                  ax_cv_[]gcc_id[]_gengtype=`find $GCC_ROOT_DIR -name gengtype$ac_exeext | head -n1`
                  if test "x$ax_cv_[]gcc_id[]_gengtype" = "x"; then
                     ax_cv_[]gcc_id[]_gengtype=no
                  fi
               fi
            fi
         fi
      ])
      if test "x$ax_cv_[]gcc_id[]_gengtype" = "xno"; then
         continue
      fi
      AC_CACHE_CHECK([$compiler plugin gtype.state], 
         ax_cv_[]gcc_id[]_gtypestate,
         [
         ax_cv_[]gcc_id[]_gtypestate=`$GCC_EXE -print-file-name=gtype.state`
         if test "x$ax_cv_[]gcc_id[]_gtypestate" = "xgtype.state"; then
            ax_cv_[]gcc_id[]_gtypestate=`$GCC_EXE -print-file-name=plugin/gtype.state`
            if test "x$ax_cv_[]gcc_id[]_gtypestate" = "xplugin/gtype.state"; then
               ax_cv_[]gcc_id[]_gtypestate=no
            fi
         fi
      ])
      if test "x$ax_cv_[]gcc_id[]_gtypestate" = "xno"; then
         continue
      fi
      AC_CACHE_CHECK([$compiler supports plugins], 
         ax_cv_[]gcc_id[]_plugin_compiler,
         [
         cat > plugin_test.c <<PLUGIN_TEST
#include "gcc-plugin.h"
#include "plugin-version.h"
#include "cp/cp-tree.h"
int plugin_is_GPL_compatible;
extern struct cpp_reader *parse_in;
void do_nothing(void* first, void* second){ cpp_define(parse_in, "TEST_GCC_PLUGIN=1");}
int plugin_init(struct plugin_name_args* plugin_info, struct plugin_gcc_version* version){if(!plugin_default_version_check(version,&gcc_version)) return 1; register_callback("plugin_test",PLUGIN_START_UNIT,do_nothing,NULL); return 0;}
PLUGIN_TEST
         ax_cv_[]gcc_id[]_plugin_compiler=no
         for plugin_compiler in $GCC_EXE $GPP_EXE; do
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
               $plugin_compiler -I$GCC_PLUGIN_INC/include -o plugin_test.o -c  plugin_test.c $plugin_option 2> /dev/null
               flexlink -chain mingw -o $plugin_file_name  plugin_test.o 2> /dev/null
            ;;
            *)
               $plugin_compiler -I$GCC_PLUGIN_INC/include -o $plugin_file_name  plugin_test.c $plugin_option 2> /dev/null
            ;;
            esac
            if test ! -f $plugin_file_name; then
               continue
            fi

            ac_save_CC="$CC"
            ac_save_CFLAGS="$CFLAGS"
            CC=$plugin_compiler
            CFLAGS="-fplugin=$BUILDDIR/$plugin_file_name"
            AC_LANG_PUSH([C])
            AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
                  ]],[[
                     return 0;
                  ]])],
            ax_cv_[]gcc_id[]_plugin_compiler=$plugin_compiler,ax_cv_[]gcc_id[]_plugin_compiler=no)
            AC_LANG_POP([C])
            CC=$ac_save_CC
            CFLAGS=$ac_save_CFLAGS
            #If plugin compilation fails, skip this executable
            if test "x$ax_cv_[]gcc_id[]_plugin_compiler" = xno; then
               continue
            fi
         done
      ])
      if test "x$ax_cv_[]gcc_id[]_plugin_compiler" != xno; then
         echo "OK, we have found the compiler"
         AC_SUBST(gcc_macro_prefix[]_EXE, ${GCC_EXE})
         AC_SUBST(gcc_macro_prefix[]_VERSION, ${GCC_VERSION})
         AC_SUBST(gcc_macro_prefix[]_GENGTYPE, ${ax_cv_[]gcc_id[]_gengtype})
         AC_SUBST(gcc_macro_prefix[]_GTYPESTATE, ${ax_cv_[]gcc_id[]_gtypestate})
         AC_SUBST(gcc_macro_prefix[]_PLUGIN_DIR, $1)
         AC_SUBST(gcc_macro_prefix[]_PLUGIN_COMPILER, ${ax_cv_[]gcc_id[]_plugin_compiler})
         AC_SUBST(gcc_macro_prefix[]_PLUGIN_INC, ${GCC_PLUGIN_INC})
         AC_DEFINE(HAVE_[]gcc_macro_prefix[]_COMPILER, 1, "Define if gcc []req_version_pretty compiler is compliant")
         AC_DEFINE_UNQUOTED(gcc_macro_prefix[]_EXE, "${GCC_EXE}", "Define the gcc executable name")
         AC_DEFINE_UNQUOTED(m4_bpatsubsts(gcc_macro_prefix[_EXE],[GCC],[GPP]), "${GPP_EXE}", "Define the g++ executable name")
         AC_DEFINE_UNQUOTED(m4_bpatsubsts(gcc_macro_prefix[_EXE],[GCC],[CPP]), "${CPP_EXE}", "Define the cpp executable name")
         AC_DEFINE_UNQUOTED(gcc_macro_prefix[]_VERSION, "${GCC_VERSION}", "Define the gcc version")
         AC_DEFINE_UNQUOTED(gcc_macro_prefix[]_PLUGIN_DIR, "$1", "Define the compiler plugins directory")
         AC_DEFINE_UNQUOTED(gcc_macro_prefix[]_PLUGIN_COMPILER, "${ax_cv_[]gcc_id[]_plugin_compiler}", "Define the plugin compiler")
         break;
      fi
   else
      echo "checking $compiler... not found"
   fi
done
AC_LANG_POP([C])
])
