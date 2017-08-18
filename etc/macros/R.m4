dnl
dnl check if a particular R package is installed
dnl
AC_DEFUN([AC_CHECK_R_PACKAGE],[

echo "a<-installed.packages()" > R_temp
echo "packages<-a[[,1]]" >> R_temp
echo "is.element(\"$1\", packages)" >> R_temp
R CMD BATCH R_temp
true=`grep TRUE R_temp.Rout | wc -l`
if test "x$true" != x1; then
   AC_MSG_ERROR("Package $1 not installed in R")
fi
rm R_temp R_temp.Rout
])

dnl
dnl check if R is installed
dnl
AC_DEFUN([AC_CHECK_R],[

RLOCALPACKAGEDIR=/usr/local/lib/R/site-library
RPACKAGEDIR=/usr/lib/R/site-library
RDIR=/usr/share/R


echo "Checking if R is installed..."

if test -f $RDIR/include/R.h; then
   echo "Checking for R.h in $RDIR/include... yes"
else
   echo "Checking for R.h in $RDIR/include... no"
   AC_MSG_ERROR("R is not installed")
fi

echo "Checking if RInside is installed..."

if test -f $RLOCALPACKAGEDIR/RInside/include/RInside.h; then
   echo "Checking for RInside.h in $RLOCALPACKAGEDIR/RInside/include... yes"
else
   echo "Checking for RInside.h in $RLOCALPACKAGEDIR/RInside/include... no"
   AC_MSG_ERROR("RInside is not installed")
fi

if test -f $RLOCALPACKAGEDIR/RInside/lib/libRInside.a; then
   echo "Checking for libRInside in $RLOCALPACKAGEDIR/RInside/lib... yes"
else
   echo "Checking for libRinside in $RLOCALPACKAGEDIR/RInside/lib... no"
   AC_MSG_ERROR("RInside is not installed")
fi

echo "Checking if Rcpp is installed..."

if test -f $RLOCALPACKAGEDIR/Rcpp/include/Rcpp.h; then
   echo "Checking for Rcpp.h in $RLOCALPACKAGEDIR/Rcpp/include... yes"
else
   echo "Checking for Rcpp.h in $RLOCALPACKAGEDIR/Rcpp/include... no"
   AC_MSG_ERROR("Rcpp is not installed")
fi

AC_CHECK_R_PACKAGE(lmtest)
AC_CHECK_R_PACKAGE(zoo)

AC_SUBST(RLOCALPACKAGEDIR)
AC_SUBST(RPACKAGEDIR)
AC_SUBST(RDIR)

AC_DEFINE(HAVE_R, 1, "Define if R is used")


AC_PROVIDE([$0])
])

