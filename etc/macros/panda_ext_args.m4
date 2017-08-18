AC_DEFUN([AC_EXT_ARGS],[
AC_ARG_ENABLE(flopoco,             [  --enable-flopoco             compile FloPoCo external library],                     [panda_USE_FLOPOCO="$enableval"])
AC_ARG_ENABLE(icarus,              [  --enable-icarus              compile Icarus Verilog compiler],                      [panda_USE_ICARUS="$enableval"])
AC_ARG_ENABLE(verilator,           [  --enable-verilator           compile Verilator Verilog compiler],                   [panda_USE_VERILATOR="$enableval"])
AC_ARG_ENABLE(trng,                [  --enable-trng                compile the TRNG external library],                    [panda_USE_TRNG="$enableval"])
AC_ARG_ENABLE(grlib,               [  --enable-grlib               use GRLIB],                                             [panda_USE_GRLIB="$enableval"])
])

