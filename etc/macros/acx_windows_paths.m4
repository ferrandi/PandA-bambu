# (c) 2004 Martin Preuss<martin@libchipcard.de>
# These functions retrieve some important paths


AC_DEFUN([AQ_WINDOZE_GETPATH], [
dnl IN:
dnl   - $1: type of path to get:
dnl         - windows: windows path
dnl         - system:  windows/system directory
dnl         - home:    users home directory
dnl   - $2: default value
dnl OUT:
dnl   - aq_windoze_path: path retrieved
dnl

rm -f conf.winpath

AC_TRY_RUN([
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main (){
  char buffer[260];
  const char *choice = "$1";
  FILE *f;

  buffer[0]=0;
  
  if (strlen("$2")) {
    if (strlen("$2")>=sizeof(buffer)) {
      printf("path is too long ($2)\n");
      exit(1);
    }
    strcpy(buffer, "$2");
  }
  else {
    if (strcasecmp(choice, "windows")==0) {
      GetWindowsDirectory(buffer, sizeof(buffer));
    }
    else if (strcasecmp(choice, "system")==0) {
      GetSystemDirectory(buffer, sizeof(buffer));
    }
    else if (strcasecmp(choice, "home")==0) {
      GetWindowsDirectory(buffer, sizeof(buffer));
    }
    else {
      printf("Unknown type \"$1\"\n");
      exit(1);
    }
  }
  
  f=fopen("conf.winpath", "w+");
  if (!f) {
    printf("Could not create file conf.winpath\n");
    exit(1);
  }
  fprintf(f, "%s", buffer);
  if (fclose(f)) {
   printf("Could not close file.\n");
   exit(1);
  }
  exit(0);
}
 ],
 [aq_windoze_path="`cat conf.winpath`"],
 [AC_MSG_ERROR(Could not determine path for $1)],
 [aq_windoze_path="$2"; AC_MSG_RESULT([Crosscompiling, assuming $2])]
)

rm -f conf.winpath
])


AC_DEFUN([AQ_WINDOZE_GETPATH_MINGW], [
dnl IN:
dnl   - $1: type of path to get:
dnl         - windows: windows path
dnl         - system:  windows/system directory
dnl         - home:    users home directory
dnl   - $2: default value
dnl OUT:
dnl   - aq_windoze_path: path retrieved
dnl

rm -f conf.winpath

AC_TRY_RUN([
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/* #include <shlobj.h> */

int main (){
  char buffer[260];
  char buffer2[260+2];
  const char *choice = "$1";
  char *p;
  char *tp;
  FILE *f;
  int lastWasSlash;

  buffer[0]=0;

  if (strlen("$2")) {
    if (strlen("$2")>=sizeof(buffer)) {
      printf("path is too long ($2)\n");
      exit(1);
    }
    strcpy(buffer, "$2");
  }
  else {
    if (strcasecmp(choice, "windows")==0) {
      GetWindowsDirectory(buffer, sizeof(buffer));
    }
    else if (strcasecmp(choice, "system")==0) {
      GetSystemDirectory(buffer, sizeof(buffer));
    }
    else if (strcasecmp(choice, "home")==0) {
      GetWindowsDirectory(buffer, sizeof(buffer));
    }
    else {
      printf("Unknown type \"$1\"\n");
      exit(1);
    }
  }
  

  /* create mingw path */
  tp=buffer2;
  p=buffer;
  if (strlen(buffer)>1) {
    if (buffer[1]==':') {
      *tp='/';
      tp++;
      *tp=buffer[0];
      tp++;
      p+=2;
    }
  }
  
  lastWasSlash=0;
  while(*p) {
    if (*p=='\\\\' || *p=='/') {
      if (!lastWasSlash) {
        *tp='/';
        tp++;
        lastWasSlash=1;
      }
    }
    else {
      lastWasSlash=0;
      *tp=*p;
      tp++;
    }
    p++;
  } /* while */
  *tp=0;
  
  f=fopen("conf.winpath", "w+");
  if (!f) {
    printf("Could not create file conf.winpath\n");
    exit(1);
  }
  fprintf(f, "%s", buffer2);
  if (fclose(f)) {
   printf("Could not close file.\n");
   exit(1);
  }
  exit(0);
}
 ],
 [aq_windoze_path=`cat conf.winpath`],
 [AC_MSG_ERROR(Could not determine path for $1)],
 [aq_windoze_path="$2"; AC_MSG_RESULT([Crosscompiling, assuming $2])]
)

rm -f conf.winpath
])


AC_DEFUN([ACX_WINDOWS_PATHS],[
dnl IN: 
dnl   - AC_CANONICAL_SYSTEM muste be called before
dnl OUT:
dnl   Variables (subst):
dnl     WIN_PATH_HOME          : path and name of the Windoze home folder
dnl     WIN_PATH_HOME_MINGW    : path and name of the Windoze home folder
dnl     WIN_PATH_WINDOWS       : path and name of the Windoze system folder
dnl     WIN_PATH_WINDOWS_MINGW : path and name of the Windoze system folder
dnl     WIN_PATH_SYSTEM        : path and name of the Windoze folder
dnl     WIN_PATH_SYSTEM_MINGW  : path and name of the Windoze folder
dnl   Defines:
dnl     WIN_PATH_HOME          : path and name of the Windoze home folder
dnl     WIN_PATH_WINDOWS       : path and name of the Windoze system folder
dnl     WIN_PATH_SYSTEM        : path and name of the Windoze folder

# presets
AC_ARG_WITH(home-path,    [  --with-home-path=DIR    specify the home directory for a user],
  [aq_windoze_path_home="$withval"])
AC_ARG_WITH(system-path,  [  --with-system-path=DIR  specify the system directory],
  [aq_windoze_path_system="$withval"])
AC_ARG_WITH(windows-path, [  --with-windows-path=DIR specify the windows directory],
  [aq_windoze_path_windows="$withval"])

# home directory
AC_MSG_CHECKING([for windoze home path (program)])
AC_CACHE_VAL(gwenhywfar_cv_path_home,
[
  AQ_WINDOZE_GETPATH(home, [$aq_windoze_path_home])
  gwenhywfar_cv_path_home="$aq_windoze_path"
])
WIN_PATH_HOME="$gwenhywfar_cv_path_home"
AC_MSG_RESULT([$WIN_PATH_HOME])

AC_MSG_CHECKING([for windoze home path (mingw)])
AC_CACHE_VAL(gwenhywfar_cv_path_home_mingw,
[
  AQ_WINDOZE_GETPATH_MINGW(home, [$aq_windoze_path_home])
  gwenhywfar_cv_path_home_mingw="$aq_windoze_path"
])
WIN_PATH_HOME_MINGW="$gwenhywfar_cv_path_home_mingw"
AC_MSG_RESULT([$WIN_PATH_HOME_MINGW])

# windows directory
AC_MSG_CHECKING([for windoze windows path (program)])
AC_CACHE_VAL(gwenhywfar_cv_path_windows,
[
  AQ_WINDOZE_GETPATH(windows, [$aq_windoze_path_windows])
  gwenhywfar_cv_path_windows="$aq_windoze_path"
])
WIN_PATH_WINDOWS="$gwenhywfar_cv_path_windows"
AC_MSG_RESULT([$WIN_PATH_WINDOWS])

AC_MSG_CHECKING([for windoze windows path (mingw)])
AC_CACHE_VAL(gwenhywfar_cv_path_windows_mingw,
[
  AQ_WINDOZE_GETPATH_MINGW(windows, [$aq_windoze_path_windows])
  gwenhywfar_cv_path_windows_mingw="$aq_windoze_path"
])
WIN_PATH_WINDOWS_MINGW="$gwenhywfar_cv_path_windows_mingw"
AC_MSG_RESULT([$WIN_PATH_WINDOWS_MINGW])

# windows system directory
AC_MSG_CHECKING([for windoze system path (program)])
AC_CACHE_VAL(gwenhywfar_cv_path_system,
[
  AQ_WINDOZE_GETPATH(system, [$aq_windoze_path_system])
  gwenhywfar_cv_path_system="$aq_windoze_path"
])
WIN_PATH_SYSTEM="$gwenhywfar_cv_path_system"
AC_MSG_RESULT([$WIN_PATH_SYSTEM])

AC_MSG_CHECKING([for windoze system path (mingw)])
AC_CACHE_VAL(gwenhywfar_cv_path_system_mingw,
[
  AQ_WINDOZE_GETPATH_MINGW(system, [$aq_windoze_path_system])
  gwenhywfar_cv_path_system_mingw="$aq_windoze_path"
])
WIN_PATH_SYSTEM_MINGW="$gwenhywfar_cv_path_system_mingw"
AC_MSG_RESULT([$WIN_PATH_SYSTEM_MINGW])

# finish variables
AC_SUBST(WIN_PATH_HOME)
AC_DEFINE_UNQUOTED(WIN_PATH_HOME, "$WIN_PATH_HOME", [home path])
AC_SUBST(WIN_PATH_HOME_MINGW)
AC_SUBST(WIN_PATH_WINDOWS)
AC_DEFINE_UNQUOTED(WIN_PATH_WINDOWS, "$WIN_PATH_WINDOWS", [windows path])
AC_SUBST(WIN_PATH_WINDOWS_MINGW)
AC_SUBST(WIN_PATH_SYSTEM)
AC_DEFINE_UNQUOTED(WIN_PATH_SYSTEM, "$WIN_PATH_SYSTEM", [system path])
AC_SUBST(WIN_PATH_SYSTEM_MINGW)
])
