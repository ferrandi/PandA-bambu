#!/bin/bash
tmp1=`echo $0 | awk -F'/[^/]*$' '{print $1}'`
TOPSRCDIR=$tmp1/../..
BUILDDIR=`pwd`
echo ""
echo ""
echo "Configuring $TOPSRCDIR/$1/configure" with arguments ${@:2}
echo ""
echo ""

if test -f $TOPSRCDIR/$1/Makefile.cvs && test ! -f $TOPSRCDIR/$1/configure; then
   cd $TOPSRCDIR/$1
   make -f Makefile.cvs
   if test $? != 0; then
      echo "Error in generation of autotools file"
      exit 1;
   fi
   cd $BUILDDIR
fi;

if test -f $TOPSRCDIR/$1/Makefile.autotools && test ! -f $TOPSRCDIR/$1/configure; then
   cd $TOPSRCDIR/$1
   make -f Makefile.autotools
   if test $? != 0; then
      echo "Error in generation of autotools file"
      exit 1;
   fi
   cd $BUILDDIR
fi;

if test -f $TOPSRCDIR/$1/autogen.sh && test ! -f $TOPSRCDIR/$1/configure; then
   cd $TOPSRCDIR/$1
   ./autogen.sh
   if test $? != 0; then
      echo "Error in generation of autotools file"
      exit 1;
   fi
   cd $BUILDDIR
fi;


if test ! -d $1; then
   mkdir $1
fi

cd $1
eval bash $TOPSRCDIR/$1/configure "${@:2}"
if test $? != 0; then
   exit 1
fi
cd $BUILDDIR
echo ""
echo ""
echo "Configured $1"
echo ""
echo ""
