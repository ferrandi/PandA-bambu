cmake_minimum_required(VERSION 3.16)

# Build and install libbambu support archives (libbambu, libm, softfloat, softint)
# using bambu-cc with the same clang ABI matrix detected for plugins.

if(NOT TARGET bambu-cc)
   message(FATAL_ERROR "bambu-cc target is required to build libbambu support libraries. Enable PANDA_BUILD_CC.")
endif()

# Reuse the selected LIBBAMBU compiler/ABI from the clang probe (mirrors ConfigHeaders logic)
set(_panda_libbambu_supported_order 13 19 16 12 11 10 9 8 7 6 5 4)
set(_panda_libbambu_auto_order 16 19 12 11 10 9 8 7 6 5 4)
set(_libbambu_choice "")
if(PANDA_LIBBAMBU_COMPILER)
   string(REGEX REPLACE "I386_CLANG" "" _sel_ver "${PANDA_LIBBAMBU_COMPILER}")
   list(FIND _panda_libbambu_supported_order "${_sel_ver}" _idx)
   if(_idx GREATER -1 AND PANDA_CLANG${_sel_ver}_FOUND)
      set(_libbambu_choice "${_sel_ver}")
   elseif("${PANDA_LIBBAMBU_COMPILER}" STREQUAL "I386_CLANG13")
      foreach(ver IN LISTS _panda_libbambu_auto_order)
         if(PANDA_CLANG${ver}_FOUND)
            set(_libbambu_choice "${ver}")
            message(STATUS "Requested PANDA_LIBBAMBU_COMPILER=I386_CLANG13 not available; falling back to I386_CLANG${ver}")
            break()
         endif()
      endforeach()
   else()
      message(FATAL_ERROR "Requested PANDA_LIBBAMBU_COMPILER=${PANDA_LIBBAMBU_COMPILER} not available")
   endif()
else()
   foreach(ver IN LISTS _panda_libbambu_auto_order)
      if(PANDA_CLANG${ver}_FOUND)
         set(_libbambu_choice "${ver}")
         break()
      endif()
   endforeach()
endif()
if(NOT _libbambu_choice)
   message(FATAL_ERROR "No supported clang compiler found for libbambu support libraries")
endif()

set(LIBBAMBU_COMPILER_ID "I386_CLANG${_libbambu_choice}")
set(LIBBAMBU_COMPILER_DIR "${PANDA_CLANG${_libbambu_choice}_PLUGIN_DIR}")
set(LIBBAMBU_M64 ${PANDA_CLANG${_libbambu_choice}_M64})
set(LIBBAMBU_M32 ${PANDA_CLANG${_libbambu_choice}_M32})
set(LIBBAMBU_MX32 ${PANDA_CLANG${_libbambu_choice}_MX32})

set(BAMBUCC "${CMAKE_BINARY_DIR}/src/bambu-cc")
set(LIBBAMBU_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(LIBBAMBU_INC "-I${LIBBAMBU_ROOT} -I${CMAKE_SOURCE_DIR}/etc/bambu_directives")
set(BOOST_INC_FLAGS "")
foreach(_inc IN LISTS Boost_INCLUDE_DIRS)
   list(APPEND BOOST_INC_FLAGS "-I${_inc}")
endforeach()

# Source lists (mirrors Automake)
set(LIBBAMBU_BUILTINS_SOURCES
   builtins/bswap32.c
   builtins/huge_val.c
   builtins/inf.c
   builtins/infinity.c
   builtins/isfinite.c
   builtins/isgreater.c
   builtins/isgreaterequal.c
   builtins/isinf.c
   builtins/isless.c
   builtins/islessequal.c
   builtins/islessgreater.c
   builtins/isnan.c
   builtins/isnormal.c
   builtins/nan.c
   builtins/nans.c
   builtins/puts.c
   builtins/signbit.c
)
set(LIBBAMBU_SOFTCOMPLEX_SOURCES
   softcomplex/divdc3.c
   softcomplex/divsc3.c
)
set(LIBBAMBU_STRING_SOURCES
   string/bcopy.c
   string/bzero.c
   string/memchr.c
   string/memcmp.c
   string/memcpy.c
   string/memmove.c
   string/mempcpy.c
   string/memrchr.c
   string/memset.c
   string/rawmemchr.c
   string/stpcpy.c
   string/stpncpy.c
   string/strcasecmp.c
   string/strcasestr.c
   string/strcat.c
   string/strchr.c
   string/strchrnul.c
   string/strcmp.c
   string/strcpy.c
   string/strcspn.c
   string/strdup.c
   string/strlen.c
   string/strncasecmp.c
   string/strncat.c
   string/strncmp.c
   string/strncpy.c
   string/strndup.c
   string/strnlen.c
   string/strpbrk.c
   string/strrchr.c
   string/strsep.c
   string/strspn.c
   string/strstr.c
   string/strtok.c
)
set(LIBBAMBU_LL_SOURCES mul32.ll mul64.ll)

set(LIBSOFTFLOAT_SOURCES
   softfloat/fpclassify.c softfloat/huge_val.c softfloat/inf.c softfloat/infinity.c softfloat/isfinite.c softfloat/isgreater.c softfloat/isgreaterequal.c
   softfloat/isinf.c softfloat/isless.c softfloat/islessequal.c softfloat/islessgreater.c softfloat/isnan.c softfloat/isnormal.c softfloat/nan.c softfloat/nans.c
   softfloat/signbit.c softfloat/softfloat.c
)

set(LIBSOFTINT_SOURCES softint/divsi3.c softint/divdi3.c softint/divdi36432.c)
set(LIBSOFTINT_NR_SOURCES softint/divsi3nr.c softint/divdi3nr.c softint/divdi3nr6432.c)
set(LIBSOFTINT_AS_SOURCES softint/divsi3as.c softint/divdi3as.c softint/divdi3as6432.c)

# libm sources
set(LIBM_MUSL_SOURCES
   libm/musl/__cos.c libm/musl/__cosdf.c libm/musl/__fpclassify.c libm/musl/__fpclassifyf.c libm/musl/__rem_pio2.c libm/musl/__rem_pio2f.c libm/musl/__signbit.c libm/musl/__signbitf.c libm/musl/__sin.c libm/musl/__sindf.c
   libm/musl/acos.c libm/musl/acosf.c libm/musl/acosh.c libm/musl/acoshf.c libm/musl/asin.c libm/musl/asinf.c libm/musl/asinh.c libm/musl/asinhf.c libm/musl/atan.c libm/musl/atan2.c libm/musl/atan2f.c libm/musl/atanf.c libm/musl/atanh.c libm/musl/atanhf.c
   libm/musl/cbrt.c libm/musl/cbrtf.c libm/musl/ceil.c libm/musl/ceilf.c libm/musl/copysign.c libm/musl/copysignf.c libm/musl/cos.c libm/musl/cosf.c libm/musl/cosh.c libm/musl/coshf.c libm/musl/erf.c libm/musl/erff.c
   libm/musl/exp.c libm/musl/exp10.c libm/musl/exp10f.c libm/musl/exp2.c libm/musl/exp2f.c libm/musl/expf.c libm/musl/expm1.c libm/musl/expm1f.c libm/musl/fabs.c libm/musl/fabsf.c libm/musl/fdim.c libm/musl/fdimf.c
   libm/musl/finite.c libm/musl/finitef.c libm/musl/floor.c libm/musl/floorf.c libm/musl/fma.c libm/musl/fmaf.c libm/musl/fmax.c libm/musl/fmaxf.c libm/musl/fmin.c libm/musl/fminf.c libm/musl/fmod.c libm/musl/fmodf.c
   libm/musl/frexp.c libm/musl/frexpf.c libm/musl/hypot.c libm/musl/hypotf.c libm/musl/ilogb.c libm/musl/ilogbf.c libm/musl/j0.c libm/musl/j0f.c libm/musl/j1.c libm/musl/j1f.c libm/musl/jn.c libm/musl/jnf.c
   libm/musl/ldexp.c libm/musl/ldexpf.c libm/musl/lgamma_r.c libm/musl/lgammaf_r.c libm/musl/llrint.c libm/musl/llrintf.c libm/musl/llround.c libm/musl/llroundf.c libm/musl/log.c libm/musl/log10.c libm/musl/log10f.c libm/musl/log1p.c libm/musl/log1pf.c libm/musl/log2.c libm/musl/log2f.c libm/musl/logb.c libm/musl/logbf.c libm/musl/logf.c
   libm/musl/lrint.c libm/musl/lrintf.c libm/musl/lround.c libm/musl/lroundf.c libm/musl/modf.c libm/musl/modff.c libm/musl/nan.c libm/musl/nanf.c libm/musl/nearbyint.c libm/musl/nearbyintf.c libm/musl/nextafter.c libm/musl/nextafterf.c
   libm/musl/pow.c libm/musl/powf.c libm/musl/remainder.c libm/musl/remainderf.c libm/musl/remquo.c libm/musl/remquof.c libm/musl/rint.c libm/musl/rintf.c libm/musl/round.c libm/musl/roundf.c libm/musl/scalb.c libm/musl/scalbf.c libm/musl/scalbln.c libm/musl/scalblnf.c libm/musl/scalbn.c libm/musl/scalbnf.c libm/musl/significand.c libm/musl/significandf.c
   libm/musl/sin.c libm/musl/sincos.c libm/musl/sincosf.c libm/musl/sinf.c libm/musl/sinh.c libm/musl/sinhf.c libm/musl/sqrt.c libm/musl/sqrt_data.c libm/musl/sqrtf.c libm/musl/tan.c libm/musl/tanf.c libm/musl/tanh.c libm/musl/tanhf.c libm/musl/tgamma.c libm/musl/tgammaf.c libm/musl/trunc.c libm/musl/truncf.c
)

set(LIBM_NEWLIB_COMMON_SOURCES
   libm/newlib/e_acos.c libm/newlib/e_acosh.c libm/newlib/e_asin.c libm/newlib/e_atan2.c libm/newlib/e_atanh.c libm/newlib/e_cosh.c libm/newlib/e_exp.c libm/newlib/e_fmod.c libm/newlib/e_hypot.c libm/newlib/e_j0.c libm/newlib/e_j1.c libm/newlib/e_jn.c libm/newlib/e_log.c libm/newlib/e_log10.c libm/newlib/e_pow.c libm/newlib/e_rem_pio2.c libm/newlib/e_remainder.c libm/newlib/e_sinh.c libm/newlib/e_tgamma.c
   libm/newlib/ef_acos.c libm/newlib/ef_acosh.c libm/newlib/ef_asin.c libm/newlib/ef_atan2.c libm/newlib/ef_atanh.c libm/newlib/ef_cosh.c libm/newlib/ef_fmod.c libm/newlib/ef_hypot.c libm/newlib/ef_j0.c libm/newlib/ef_j1.c libm/newlib/ef_jn.c libm/newlib/ef_log10.c libm/newlib/ef_remainder.c libm/newlib/ef_scalb.c libm/newlib/ef_sinh.c libm/newlib/ef_tgamma.c
   libm/newlib/er_lgamma.c libm/newlib/erf_lgamma.c libm/newlib/k_cos.c libm/newlib/k_rem_pio2.c libm/newlib/k_sin.c libm/newlib/k_tan.c libm/newlib/kf_cos.c libm/newlib/kf_sin.c
   libm/newlib/s_asinh.c libm/newlib/s_atan.c libm/newlib/s_cbrt.c libm/newlib/s_ceil.c libm/newlib/s_cos.c libm/newlib/s_erf.c libm/newlib/s_expm1.c libm/newlib/s_fdim.c libm/newlib/s_floor.c libm/newlib/s_fmax.c libm/newlib/s_fmin.c libm/newlib/s_frexp.c libm/newlib/s_ilogb.c libm/newlib/s_ldexp.c libm/newlib/s_log1p.c libm/newlib/s_logb.c libm/newlib/s_modf.c libm/newlib/s_nextafter.c libm/newlib/s_rint.c libm/newlib/s_scalbn.c libm/newlib/s_signif.c libm/newlib/s_sin.c libm/newlib/s_sincos.c libm/newlib/s_tan.c libm/newlib/s_tanh.c
   libm/newlib/sf_asinh.c libm/newlib/sf_atan.c libm/newlib/sf_cbrt.c libm/newlib/sf_ceil.c libm/newlib/sf_erf.c libm/newlib/sf_expm1.c libm/newlib/sf_fdim.c libm/newlib/sf_floor.c libm/newlib/sf_fmax.c libm/newlib/sf_fmin.c libm/newlib/sf_frexp.c libm/newlib/sf_ilogb.c libm/newlib/sf_ldexp.c libm/newlib/sf_log1p.c libm/newlib/sf_logb.c libm/newlib/sf_modf.c libm/newlib/sf_nextafter.c libm/newlib/sf_rint.c libm/newlib/sf_scalbn.c libm/newlib/sf_signif.c libm/newlib/sf_tanh.c
   libm/newlib/w_acos.c libm/newlib/w_acosh.c libm/newlib/w_asin.c libm/newlib/w_atan2.c libm/newlib/w_atanh.c libm/newlib/w_cosh.c libm/newlib/w_drem.c libm/newlib/w_exp.c libm/newlib/w_exp2.c libm/newlib/w_fmod.c libm/newlib/w_hypot.c libm/newlib/w_j0.c libm/newlib/w_j1.c libm/newlib/w_jn.c libm/newlib/w_log.c libm/newlib/w_log10.c libm/newlib/w_pow.c libm/newlib/w_remainder.c libm/newlib/w_sinh.c libm/newlib/w_sqrt.c libm/newlib/w_tgamma.c
   libm/newlib/wf_acos.c libm/newlib/wf_acosh.c libm/newlib/wf_asin.c libm/newlib/wf_atan2.c libm/newlib/wf_atanh.c libm/newlib/wf_cosh.c libm/newlib/wf_drem.c libm/newlib/wf_exp.c libm/newlib/wf_exp2.c libm/newlib/wf_fmod.c libm/newlib/wf_hypot.c libm/newlib/wf_j0.c libm/newlib/wf_j1.c libm/newlib/wf_jn.c libm/newlib/wf_log.c libm/newlib/wf_log10.c libm/newlib/wf_log2.c libm/newlib/wf_pow.c libm/newlib/wf_remainder.c libm/newlib/wf_sinh.c libm/newlib/wf_sqrt.c libm/newlib/wf_tgamma.c libm/newlib/wr_gamma.c libm/newlib/wr_lgamma.c libm/newlib/wrf_gamma.c libm/newlib/wrf_lgamma.c
)
set(LIBM_NEWLIB_SOURCES
   ${LIBM_NEWLIB_COMMON_SOURCES}
   libm/newlib/e_sqrt.c libm/newlib/ef_exp.c libm/newlib/ef_log.c libm/newlib/ef_pow.c libm/newlib/ef_rem_pio2.c libm/newlib/ef_sqrt.c libm/newlib/kf_rem_pio2.c libm/newlib/kf_tan.c libm/newlib/s_copysign.c libm/newlib/s_fabs.c libm/newlib/s_finite.c libm/newlib/s_fpclassify.c libm/newlib/sf_copysign.c libm/newlib/sf_cos.c libm/newlib/sf_fabs.c libm/newlib/sf_finite.c libm/newlib/sf_fpclassify.c libm/newlib/sf_sin.c libm/newlib/sf_sincos.c libm/newlib/sf_tan.c
)
set(LIBM_HLS_EXT_SOURCES libm/hls_ext/ef_pow.c)
set(LIBM_HLS_SOURCES
   ${LIBM_NEWLIB_COMMON_SOURCES}
   ${LIBM_HLS_EXT_SOURCES}
   libm/newlib/ef_exp.c libm/newlib/ef_log.c
   libm/hls/e_sqrt.c libm/hls/ef_sqrt.c libm/hls/s_copysign.c libm/hls/s_fabs.c libm/hls/s_finite.c libm/hls/s_fpclassify.c libm/hls/sf_copysign.c libm/hls/sf_cos.c libm/hls/sf_fabs.c libm/hls/sf_finite.c libm/hls/sf_fpclassify.c libm/hls/sf_sin.c libm/hls/sf_sincos.c libm/hls/sf_tan.c libm/hls/t_copysign.c libm/hls/t_fabs.c libm/hls/t_finite.c
)

function(_bambu_cc_object out_list arch_label arch_flag extra_flags src suffix)
   get_filename_component(_base "${src}" NAME_WE)
   get_filename_component(_src_dir "${src}" DIRECTORY)
   if(_src_dir STREQUAL "")
      set(obj "${CMAKE_CURRENT_BINARY_DIR}/${arch_label}/${_base}${suffix}.o")
   else()
      set(obj "${CMAKE_CURRENT_BINARY_DIR}/${arch_label}/${_src_dir}/${_base}${suffix}.o")
   endif()
   get_filename_component(obj_dir "${obj}" DIRECTORY)
   string(REPLACE ";" " " _eflags "${extra_flags}")
   add_custom_command(OUTPUT "${obj}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${obj_dir}"
      # Pass build root; bambu-cc appends the clang-XX directory itself
      COMMAND bash -lc "set -euo pipefail; \"${BAMBUCC}\" --output-temporary-directory=/tmp -iplugindir=\"${CMAKE_BINARY_DIR}\" --compiler=${LIBBAMBU_COMPILER_ID} ${arch_flag} ${_eflags} -c \"${LIBBAMBU_ROOT}/${src}\" -o \"${obj}\""
      DEPENDS "${LIBBAMBU_ROOT}/${src}" bambu-cc
      VERBATIM
   )
   set(${out_list} ${${out_list}} "${obj}" PARENT_SCOPE)
endfunction()

function(_bambu_cc_objects out_list arch_label arch_flag cflags sources)
   foreach(src IN LISTS sources)
      _bambu_cc_object(${out_list} "${arch_label}" "${arch_flag}" "${cflags}" "${src}" "")
   endforeach()
   set(${out_list} ${${out_list}} PARENT_SCOPE)
endfunction()

function(_panda_archive lib_path objects)
   add_custom_command(OUTPUT "${lib_path}"
      COMMAND "${CMAKE_AR}" qc "${lib_path}" ${objects}
      COMMAND "${CMAKE_RANLIB}" "${lib_path}"
      DEPENDS ${objects}
      VERBATIM
   )
endfunction()

function(_panda_add_arch arch arch_flag)
   set(_arch_dir "${CMAKE_CURRENT_BINARY_DIR}/${arch}")
   # Common flags
   set(_libbambu_cflags "-nostdlib -Os ${LIBBAMBU_INC} ${BOOST_INC_FLAGS}")
   set(_softfloat_cppflags "${LIBBAMBU_INC} ${BOOST_INC_FLAGS} -DNO_PARAMETRIC -DNO_SIGNALLING")
   set(_softfloat_common_cflags "-O2 -finline-functions -Werror=uninitialized -fno-math-errno -fno-signaling-nans -fno-trapping-math")
   set(_softfloat_cflags "-D_FLT_NO_DENORMALS ${_softfloat_cppflags} ${_softfloat_common_cflags}")
   set(_softfloat_s_cflags "${_softfloat_cppflags} ${_softfloat_common_cflags}")
   set(_softint_cflags "-Os -finline-functions --extra-cc-options=\"-mllvm -inline-threshold=10000\" ${LIBBAMBU_INC} ${BOOST_INC_FLAGS}")
   set(_libm_cppflags "${BOOST_INC_FLAGS} ${LIBBAMBU_INC} -DNO_RAISE_EXCEPTIONS")
   set(_libm_common_cflags "-O2 -finline-functions -finline-hint-functions -fno-builtin -fno-math-errno -fno-signaling-nans -fno-trapping-math")
   set(_libm_cflags "-D_FLT_NO_DENORMALS ${_libm_cppflags} ${_libm_common_cflags}")
   set(_libm_s_cflags "${_libm_cppflags} ${_libm_common_cflags}")

   # libbambu objects
   set(libbambu_objs)
   _bambu_cc_objects(libbambu_objs "${arch}" "${arch_flag}" "${_libbambu_cflags}" "${LIBBAMBU_BUILTINS_SOURCES}")
   _bambu_cc_objects(libbambu_objs "${arch}" "${arch_flag}" "${_libbambu_cflags}" "${LIBBAMBU_SOFTCOMPLEX_SOURCES}")
   _bambu_cc_objects(libbambu_objs "${arch}" "${arch_flag}" "${_libbambu_cflags}" "${LIBBAMBU_STRING_SOURCES}")
   _bambu_cc_objects(libbambu_objs "${arch}" "${arch_flag}" "${_libbambu_cflags}" "assert.c;memmgr.c;mul32.ll;mul64.ll")
   set(libbambu_lib "${_arch_dir}/libbambu.a")
   _panda_archive("${libbambu_lib}" "${libbambu_objs}")

   # softfloat
   set(softfloat_objs)
   _bambu_cc_objects(softfloat_objs "${arch}" "${arch_flag}" "${_softfloat_cflags}" "${LIBSOFTFLOAT_SOURCES}")
   set(softfloat_s_objs)
   foreach(src IN LISTS LIBSOFTFLOAT_SOURCES)
      _bambu_cc_object(softfloat_s_objs "${arch}" "${arch_flag}" "${_softfloat_s_cflags}" "${src}" "_s")
   endforeach()
   set(libsoftfloat "${_arch_dir}/libsoftfloat.a")
   set(libsoftfloat_s "${_arch_dir}/libsoftfloat_s.a")
   _panda_archive("${libsoftfloat}" "${softfloat_objs}")
   _panda_archive("${libsoftfloat_s}" "${softfloat_s_objs}")

   # softint
   set(softint_nr_objs)
   _bambu_cc_objects(softint_nr_objs "${arch}" "${arch_flag}" "${_softint_cflags}" "${LIBSOFTINT_SOURCES}")
   set(softint_nr1_objs)
   foreach(src IN LISTS LIBSOFTINT_NR_SOURCES)
      _bambu_cc_object(softint_nr1_objs "${arch}" "${arch_flag}" "${_softint_cflags} -DUNROLL_FACTOR=1" "${src}" "_nr1")
   endforeach()
   set(softint_nr2_objs)
   foreach(src IN LISTS LIBSOFTINT_NR_SOURCES)
      _bambu_cc_object(softint_nr2_objs "${arch}" "${arch_flag}" "${_softint_cflags} -DUNROLL_FACTOR=2" "${src}" "_nr2")
   endforeach()
   set(softint_as_objs)
   _bambu_cc_objects(softint_as_objs "${arch}" "${arch_flag}" "${_softint_cflags}" "${LIBSOFTINT_AS_SOURCES}")
   set(libsoftint_nr "${_arch_dir}/libsoftintNR.a")
   set(libsoftint_nr1 "${_arch_dir}/libsoftintnr1.a")
   set(libsoftint_nr2 "${_arch_dir}/libsoftintnr2.a")
   set(libsoftint_as "${_arch_dir}/libsoftintas.a")
   _panda_archive("${libsoftint_nr}" "${softint_nr_objs}")
   _panda_archive("${libsoftint_nr1}" "${softint_nr1_objs}")
   _panda_archive("${libsoftint_nr2}" "${softint_nr2_objs}")
   _panda_archive("${libsoftint_as}" "${softint_as_objs}")

   # libm
   set(libm_hls_objs)
   _bambu_cc_objects(libm_hls_objs "${arch}" "${arch_flag}" "${_libm_cflags}" "${LIBM_HLS_SOURCES}")
   set(libm_hls_s_objs)
   foreach(src IN LISTS LIBM_HLS_SOURCES)
      _bambu_cc_object(libm_hls_s_objs "${arch}" "${arch_flag}" "${_libm_s_cflags}" "${src}" "_s")
   endforeach()
   set(libm_musl_objs)
   _bambu_cc_objects(libm_musl_objs "${arch}" "${arch_flag}" "${_libm_cflags}" "${LIBM_MUSL_SOURCES}")
   set(libm_newlib_objs)
   _bambu_cc_objects(libm_newlib_objs "${arch}" "${arch_flag}" "${_libm_cflags}" "${LIBM_NEWLIB_SOURCES}")

   set(libm_hls "${_arch_dir}/libm_hls.a")
   set(libm_hls_s "${_arch_dir}/libm_hls_s.a")
   set(libm_musl "${_arch_dir}/libm_musl.a")
   set(libm_newlib "${_arch_dir}/libm_newlib.a")
   _panda_archive("${libm_hls}" "${libm_hls_objs}")
   _panda_archive("${libm_hls_s}" "${libm_hls_s_objs}")
   _panda_archive("${libm_musl}" "${libm_musl_objs}")
   _panda_archive("${libm_newlib}" "${libm_newlib_objs}")

   add_custom_target(libbambu_${arch} ALL DEPENDS
      "${libbambu_lib}"
      "${libsoftfloat}" "${libsoftfloat_s}"
      "${libsoftint_nr}" "${libsoftint_nr1}" "${libsoftint_nr2}" "${libsoftint_as}"
      "${libm_hls}" "${libm_hls_s}" "${libm_musl}" "${libm_newlib}"
   )
   add_dependencies(libbambu_${arch} bambu-cc)
   if(TARGET panda_clang_plugins)
      add_dependencies(libbambu_${arch} panda_clang_plugins)
   endif()
   install(CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" --build \"${CMAKE_BINARY_DIR}\" --target libbambu_${arch} --config \"\${CMAKE_INSTALL_CONFIG_NAME}\")")

   # Install: libraries under lib/panda/<compiler_dir>/<arch>
   set(_install_dir "${PANDA_LIB_INSTALLDIR_REL}/${LIBBAMBU_COMPILER_DIR}/${arch}")
   install(FILES
      "${libbambu_lib}"
      "${libsoftfloat}" "${libsoftfloat_s}"
      "${libsoftint_nr}" "${libsoftint_nr1}" "${libsoftint_nr2}" "${libsoftint_as}"
      "${libm_hls}" "${libm_hls_s}" "${libm_musl}" "${libm_newlib}"
      DESTINATION "${_install_dir}"
   )
   # libm aliases (libm.a -> libm_hls.a, libm_s.a -> libm_hls_s.a)
   install(CODE "
      file(MAKE_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}/${_install_dir}\")
      execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink \"libm_hls.a\" \"\${CMAKE_INSTALL_PREFIX}/${_install_dir}/libm.a\")
      execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink \"libm_hls_s.a\" \"\${CMAKE_INSTALL_PREFIX}/${_install_dir}/libm_s.a\")
   ")
   # Compatibility symlink: lib/panda/<arch> -> lib/panda/<compiler_dir>/<arch>
   set(_compat_dir "${PANDA_LIB_INSTALLDIR_REL}/${arch}")
   install(CODE "
      file(MAKE_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}/${PANDA_LIB_INSTALLDIR_REL}\")
      execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink \"${LIBBAMBU_COMPILER_DIR}/${arch}\" \"\${CMAKE_INSTALL_PREFIX}/${_compat_dir}\")
   ")
endfunction()

if(LIBBAMBU_M64)
   _panda_add_arch(64 -m64)
endif()
if(LIBBAMBU_M32)
   _panda_add_arch(32 -m32)
endif()
if(LIBBAMBU_MX32)
   _panda_add_arch(x32 -mx32)
endif()
