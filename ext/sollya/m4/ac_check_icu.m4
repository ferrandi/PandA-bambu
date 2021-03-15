# SYNOPSIS
#
#   AC_CHECK_ICU(version, action-if, action-if-not)
#
# DESCRIPTION
#
#   Defines ICU_LIBS, ICU_CFLAGS, ICU_CPPFLAGS
#
# COPYLEFT
#
#   Copyright (c) 2005 Akos Maroy <darkeye@tyrell.hu>
#
#   Copying and distribution of this file, with or without
#   modification, are permitted in any medium without royalty provided
#   the copyright notice and this notice are preserved.
AC_DEFUN([AC_CHECK_ICU],
	 [icudir=default
	  AC_ARG_WITH(icu,[  --with-icu[=PREFIX]       use ICU libs in PREFIX], icudir=$withval)
	  if test "$icudir" != "no"; then
	      if test "$icudir" = "yes" -o "$icudir" = "default"; then
		  AC_PATH_PROG(pkgconfigpath, pkg-config, NONE)
		  if test -x $pkgconfigpath; then
		      AC_MSG_CHECKING([for icu-i18n via pkg-config])
		      if $pkgconfigpath --exists icu-i18n; then
			  AC_MSG_RESULT([found])
			  ICU_VERSION=`$pkgconfigpath --modversion icu-i18n`
			  ICU_CPPFLAGS=""
			  ICU_CFLAGS=`$pkgconfigpath --cflags icu-i18n`
			  ICU_LIBS=`$pkgconfigpath --libs icu-i18n`
		      else
			  AC_MSG_RESULT([not found])
		      fi
		  fi
	      fi
	      if test -z "$ICU_VERSION"; then
		  if test "$icudir" = "yes" -o "$icudir" = "default"; then
		      AC_PATH_PROG(ICU_CONFIG, icu-config, NONE)
		  else
		      ICU_CONFIG=${icudir}/bin/icu-config
		  fi
		  AC_MSG_CHECKING([$ICU_CONFIG])
		  if test -x "$ICU_CONFIG"; then
		      AC_MSG_RESULT([found])
		      ICU_VERSION=`$ICU_CONFIG --version`
		      ICU_PREFIX=`$ICU_CONFIG --prefix`
		      ICU_CPPFLAGS=`$ICU_CONFIG --cppflags-searchpath`""
		      if test "$ICU_PREFIX" = "/usr"; then
			  ICU_CPPFLAGS=`echo $ICU_CPPFLAGS|sed 's@-I/usr/include@@'`
		      fi
		      ICU_CFLAGS=`$ICU_CONFIG --cflags`
		      if test "$ICU_PREFIX" = "/usr"; then
			  ICU_LIBS=`$ICU_CONFIG --ldflags-libsonly`
		      else
			  ICU_LIBS=`$ICU_CONFIG --ldflags`
		      fi
		      case $host_os in
			  solaris*)
			      AC_CHECK_LIB([Crun],[_fini],[ICU_LIBS="$ICU_LIBS -lCrun"])
			      ;;
		      esac
		  else
		      AC_MSG_RESULT([not found])
		  fi
	      fi
	      if test -n "$ICU_VERSION"; then
		  AC_MSG_CHECKING([for ICU >= $1])
		  VERSION_CHECK=`expr $ICU_VERSION \>\= $1`
		  if test "$VERSION_CHECK" = "1" ; then
		      AC_MSG_RESULT([$ICU_VERSION])
		  else
		      AC_MSG_RESULT([can not find ICU >= $1])
		      ICU_CPPFLAGS=""
		      ICU_CFLAGS=""
		      ICU_LIBS=""
		      ICU_VERSION=""
		  fi
		  AC_SUBST(ICU_CPPFLAGS)
		  AC_SUBST(ICU_CFLAGS)
		  AC_SUBST(ICU_LIBS)
	      fi
	      if test -z "$ICU_VERSION"; then
		  if test "$icudir" != "default"; then
		      AC_MSG_ERROR([libicu development libraries not found.])
		  fi
	      fi
	  fi
	  if test -n "$ICU_VERSION"; then
	      ifelse([$2], , :, [$2])
	  else
	      ifelse([$3], , :, [$3])
	  fi
	 ])
dnl Local Variables:
dnl mode:shell-script
dnl sh-indentation:4
dnl sh-basic-offset: 4
dnl End:
