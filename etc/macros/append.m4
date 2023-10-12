
dnl
dnl Append a second tag to a first one. If first one has not been created, it is with only the second tag up to now
dnl
AC_DEFUN([AC_APPEND],
    [if test -z "$$1"; then
        $1="$2"
    else
        $1="$$1 $2"
    fi
])
