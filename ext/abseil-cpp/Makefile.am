# Makefile for flopoco/src
AUTOMAKE_OPTIONS = subdir-objects
METASOURCES = AUTO

EXTRA_DIST = \
             absl/algorithm/algorithm_test.cc absl/algorithm/BUILD.bazel absl/algorithm/CMakeLists.txt absl/algorithm/container_test.cc absl/algorithm/equal_benchmark.cc \
             absl/base/internal/exception_testing.h absl/base/internal/exception_safety_testing.h absl/base/internal/exception_safety_testing.cc absl/base/exception_safety_testing_test.cc \
             absl/base/internal/atomic_hook_test_helper.cc absl/base/internal/atomic_hook_test_helper.h absl/base/internal/atomic_hook_test.cc \
             absl/base/bit_cast_test.cc absl/base/throw_delegate_test.cc absl/base/internal/inline_variable_testing.h absl/base/inline_variable_test.cc absl/base/inline_variable_test_a.cc absl/base/inline_variable_test_b.cc \
             absl/base/invoke_test.cc absl/base/spinlock_test_common.cc absl/base/internal/endian_test.cc absl/base/config_test.cc absl/base/call_once_test.cc absl/base/raw_logging_test.cc absl/base/internal/sysinfo_test.cc \
             absl/base/internal/low_level_alloc_test.cc absl/base/internal/periodic_sampler_benchmark.cc absl/base/internal/periodic_sampler_test.cc absl/base/internal/thread_identity_test.cc absl/base/internal/bits_test.cc absl/base/internal/exponential_biased_test.cc absl/base/internal/scoped_set_env_test.cc \
             absl/base/internal/cmake_thread_test.cc absl/base/log_severity_test.cc absl/base/BUILD.bazel absl/base/CMakeLists.txt absl/container/internal/compressed_tuple_test.cc absl/container/fixed_array_test.cc \
             absl/container/btree_test.h absl/container/btree_test.cc absl/container/fixed_array_exception_safety_test.cc absl/container/inlined_vector_test.cc absl/container/inlined_vector_exception_safety_test.cc absl/container/internal/test_instance_tracker.h absl/container/internal/test_instance_tracker.cc \
             absl/container/flat_hash_map_test.cc absl/container/flat_hash_set_test.cc absl/container/node_hash_map_test.cc absl/container/node_hash_set_test.cc absl/container/internal/container_memory_test.cc \
             absl/container/internal/hash_function_defaults_test.cc absl/container/internal/hash_generator_testing.h absl/container/internal/hash_generator_testing.cc absl/container/internal/hash_policy_testing.h \
             absl/container/internal/hash_policy_testing_test.cc absl/container/internal/hash_policy_traits_test.cc absl/container/internal/hashtablez_sampler_test.cc absl/container/internal/node_hash_policy_test.cc \
             absl/container/internal/raw_hash_set_test.cc absl/container/internal/raw_hash_set_allocator_test.cc absl/container/internal/layout_test.cc absl/container/internal/unordered_map_constructor_test.h absl/container/internal/unordered_map_lookup_test.h \
             absl/container/internal/unordered_map_members_test.h absl/container/internal/unordered_map_modifiers_test.h absl/container/internal/unordered_set_constructor_test.h absl/container/internal/unordered_set_lookup_test.h \
             absl/container/internal/unordered_set_members_test.h absl/container/internal/unordered_set_modifiers_test.h absl/container/internal/unordered_set_test.cc absl/container/internal/unordered_map_test.cc \
             absl/container/BUILD.bazel absl/container/CMakeLists.txt absl/copts/GENERATED_copts.bzl absl/copts/AbseilConfigureCopts.cmake absl/copts/copts.py absl/copts/generate_copts.py absl/copts/GENERATED_AbseilCopts.cmake absl/copts/configure_copts.bzl\
             absl/debugging/symbolize_test.cc absl/debugging/failure_signal_handler_test.cc absl/debugging/internal/demangle_test.cc absl/debugging/leak_check_test.cc absl/debugging/leak_check_fail_test.cc \
             absl/debugging/internal/stack_consumption_test.cc absl/debugging/BUILD.bazel absl/debugging/CMakeLists.txt \
             absl/flags/config_test.cc absl/flags/flag_test.cc absl/flags/flag_test_defs.cc absl/flags/marshalling_test.cc absl/flags/parse_test.cc absl/flags/internal/path_util_test.cc\
             absl/flags/internal/program_name_test.cc absl/flags/usage_config_test.cc absl/flags/internal/usage_test.cc absl/flags/BUILD.bazel absl/flags/CMakeLists.txt\
             absl/functional/bind_front_test.cc absl/functional/BUILD.bazel absl/functional/function_ref_benchmark.cc absl/functional/function_ref_test.cc\
             absl/hash/hash_testing.h absl/hash/hash_test.cc absl/hash/internal/city_test.cc absl/hash/BUILD.bazel absl/hash/CMakeLists.txt\
             absl/memory/memory_test.cc absl/memory/memory_exception_safety_test.cc absl/memory/BUILD.bazel absl/memory/CMakeLists.txt \
             absl/meta/type_traits_test.cc absl/meta/BUILD.bazel absl/meta/CMakeLists.txt \
             absl/numeric/int128_stream_test.cc absl/numeric/int128_test.cc \
             absl/random/bernoulli_distribution_test.cc absl/random/beta_distribution_test.cc absl/random/bit_gen_ref_test.cc absl/random/distributions_test.cc absl/random/generators_test.cc absl/random/log_uniform_int_distribution_test.cc \
             absl/random/mock_distributions_test.cc absl/random/mocking_bit_gen_test.cc \
             absl/random/discrete_distribution_test.cc absl/random/poisson_distribution_test.cc absl/random/exponential_distribution_test.cc absl/random/gaussian_distribution_test.cc absl/random/uniform_int_distribution_test.cc \
             absl/random/uniform_real_distribution_test.cc absl/random/zipf_distribution_test.cc absl/random/examples_test.cc absl/random/seed_sequences_test.cc absl/random/internal/chi_square.cc \
             absl/random/internal/distribution_test_util.cc absl/random/internal/chi_square.h absl/random/internal/distribution_test_util.h absl/random/internal/traits_test.cc absl/random/internal/generate_real_test.cc \
             absl/random/internal/distribution_test_util_test.cc absl/random/internal/fastmath_test.cc absl/random/internal/explicit_seed_seq_test.cc absl/random/internal/salted_seed_seq_test.cc \
             absl/random/internal/chi_square_test.cc absl/random/internal/fast_uniform_bits_test.cc absl/random/internal/nonsecure_base_test.cc absl/random/internal/seed_material_test.cc absl/random/internal/pool_urbg_test.cc\
             absl/random/internal/pcg_engine_test.cc absl/random/internal/randen_engine_test.cc absl/random/internal/randen_test.cc absl/random/internal/randen_slow_test.cc absl/random/internal/randen_hwaes_test.cc\
             absl/random/internal/iostream_state_saver_test.cc absl/random/internal/wide_multiply_test.cc absl/random/internal/randen_hwaes.cc absl/random/BUILD.bazel absl/random/CMakeLists.txt\
             absl/strings/match_test.cc absl/strings/escaping_test.cc absl/strings/ascii_test.cc absl/strings/internal/memutil.h absl/strings/internal/memutil_test.cc absl/strings/internal/utf8_test.cc \
             absl/strings/string_view_test.cc absl/strings/substitute_test.cc absl/strings/str_replace_test.cc absl/strings/str_split_test.cc absl/strings/internal/ostringstream_test.cc absl/strings/internal/resize_uninitialized.h \
             absl/strings/internal/resize_uninitialized_test.cc absl/strings/str_join_test.cc absl/strings/str_cat_test.cc absl/strings/internal/numbers_test_common.h absl/strings/numbers_test.cc \
             absl/strings/strip_test.cc absl/strings/internal/char_map_test.cc absl/strings/charconv_test.cc absl/strings/internal/charconv_parse.h absl/strings/internal/charconv_parse_test.cc \
             absl/strings/internal/charconv_bigint.h absl/strings/internal/charconv_bigint_test.cc absl/strings/internal/charconv_parse.h absl/strings/str_format_test.cc absl/strings/internal/str_format/extension_test.cc \
             absl/strings/internal/str_format/arg_test.cc absl/strings/internal/str_format/bind_test.cc absl/strings/internal/str_format/checker_test.cc absl/strings/internal/str_format/convert_test.cc absl/strings/internal/str_format/output_test.cc \
             absl/strings/internal/str_format/parser_test.cc absl/strings/internal/pow10_helper_test.cc absl/strings/BUILD.bazel absl/strings/CMakeLists.txt\
             absl/synchronization/barrier_test.cc absl/synchronization/blocking_counter_test.cc absl/synchronization/internal/graphcycles_test.cc absl/synchronization/mutex_test.cc absl/synchronization/notification_test.cc\
             absl/synchronization/internal/per_thread_sem_test.cc absl/synchronization/internal/per_thread_sem_test.cc absl/synchronization/lifetime_test.cc absl/synchronization/BUILD.bazel absl/synchronization/CMakeLists.txt\
             absl/time/internal/test_util.h absl/time/internal/test_util.cc absl/time/internal/zoneinfo.inc absl/time/civil_time_test.cc absl/time/clock_test.cc absl/time/duration_test.cc absl/time/format_test.cc \
             absl/time/time_test.cc absl/time/time_zone_test.cc absl/time/BUILD.bazel absl/time/CMakeLists.txt \
             absl/types/any_test.cc absl/types/span_test.cc absl/types/optional_test.cc absl/types/optional_exception_safety_test.cc absl/types/variant_test.cc absl/types/compare_test.cc absl/types/variant_exception_safety_test.cc\
             absl/types/internal/conformance_testing_test.cc \
             absl/types/BUILD.bazel absl/types/CMakeLists.txt\
             absl/utility/utility_test.cc absl/utility/BUILD.bazel absl/utility/CMakeLists.txt\
             absl/BUILD.bazel absl/CMakeLists.txt absl/compiler_config_setting.bzl absl/abseil.podspec.gen.py\
             ci/linux_clang-latest_libcxx_tsan_bazel.sh ci/macos_xcode_bazel.sh ci/linux_clang-latest_libstdcxx_bazel.sh ci/linux_clang-latest_libcxx_bazel.sh ci/cmake_install_test.sh ci/macos_xcode_cmake.sh ci/linux_gcc-latest_libstdcxx_bazel.sh\
             ci/linux_clang-latest_libcxx_asan_bazel.sh ci/linux_gcc-4.9_libstdcxx_bazel.sh ci/linux_gcc-latest_libstdcxx_cmake.sh ci/absl_alternate_options.h ci/linux_docker_containers.sh \
             CMake/README.md CMake/AbseilInstallDirs.cmake CMake/abslConfig.cmake.in CMake/install_test_project CMake/install_test_project/CMakeLists.txt CMake/install_test_project/test.sh CMake/install_test_project/simple.cc\
             CMake/AbseilHelpers.cmake CMake/Googletest CMake/Googletest/DownloadGTest.cmake CMake/Googletest/CMakeLists.txt.in \
             ABSEIL_ISSUE_TEMPLATE.md AUTHORS CMakeLists.txt  conanfile.py  configure.ac  CONTRIBUTING.md  LICENSE  LTS.md  Makefile.am  README.md  UPGRADES.md  WORKSPACE Makefile.autotools\
             BUILD.bazel CMake/AbseilDll.cmake FAQ.md \
             absl/base/internal/errno_saver_test.cc \
             absl/base/internal/strerror_test.cc \
             absl/base/internal/fast_type_id_test.cc \
             absl/base/internal/strerror_benchmark.cc \
             absl/base/internal/unique_small_name_test.cc \
             absl/base/optimization_test.cc \
             absl/container/btree_benchmark.cc \
             absl/flags/flag_benchmark.cc \
             absl/flags/commandlineflag_test.cc \
             absl/flags/reflection_test.cc \
             absl/functional/CMakeLists.txt \
             absl/strings/cord_test.cc \
             absl/strings/cord_test_helpers.h \
             absl/status/BUILD.bazel \
             absl/status/CMakeLists.txt \
             absl/status/status.cc \
             absl/status/status.h \
             absl/status/status_payload_printer.cc \
             absl/status/status_payload_printer.h \
             absl/status/status_test.cc


noinst_HEADERS = \
          absl/algorithm/algorithm.h absl/algorithm/container.h\
          absl/base/options.h \
          absl/base/internal/atomic_hook.h absl/base/log_severity.h absl/base/internal/scheduling_mode.h absl/base/internal/spinlock_wait.h absl/base/internal/spinlock_akaros.inc absl/base/internal/spinlock_linux.inc absl/base/internal/spinlock_posix.inc absl/base/internal/spinlock_win32.inc \
          absl/base/config.h absl/base/policy_checks.h absl/base/dynamic_annotations.h absl/base/attributes.h absl/base/const_init.h absl/base/macros.h absl/base/optimization.h absl/base/port.h absl/base/thread_annotations.h absl/base/internal/thread_annotations.h \
          absl/base/internal/direct_mmap.h absl/base/internal/low_level_alloc.h absl/base/internal/hide_ptr.h absl/base/internal/identity.h absl/base/internal/inline_variable.h absl/base/internal/invoke.h \
          absl/base/call_once.h absl/base/casts.h absl/base/internal/cycleclock.h absl/base/internal/low_level_scheduling.h absl/base/internal/per_thread_tls.h absl/base/internal/periodic_sampler.h absl/base/internal/raw_logging.h absl/base/internal/spinlock.h absl/base/internal/sysinfo.h absl/base/internal/thread_identity.h absl/base/internal/tsan_mutex_interface.h absl/base/internal/unscaledcycleclock.h absl/base/log_severity.h \
          absl/base/internal/throw_delegate.h absl/base/internal/pretty_function.h absl/base/internal/endian.h absl/base/internal/unaligned_access.h absl/base/internal/bits.h absl/base/internal/exponential_biased.h \
          absl/base/internal/scoped_set_env.h \
          absl/base/internal/dynamic_annotations.h \
          absl/base/internal/errno_saver.h \
          absl/base/internal/fast_type_id.h \
          absl/base/internal/strerror.cc \
          absl/base/internal/strerror.h \
          absl/container/btree_map.h absl/container/btree_set.h absl/container/internal/btree.h absl/container/internal/btree_container.h absl/container/internal/compressed_tuple.h absl/container/fixed_array.h absl/container/internal/inlined_vector.h absl/container/inlined_vector.h absl/container/internal/counting_allocator.h \
          absl/container/flat_hash_map.h absl/container/flat_hash_set.h absl/container/node_hash_map.h absl/container/node_hash_set.h absl/container/internal/container_memory.h absl/container/internal/hash_function_defaults.h \
          absl/container/internal/hash_policy_traits.h absl/container/internal/hashtablez_sampler.h absl/container/internal/hashtable_debug.h absl/container/internal/hashtable_debug_hooks.h absl/container/internal/have_sse.h absl/container/internal/common.h\
          absl/container/internal/node_hash_policy.h absl/container/internal/raw_hash_map.h absl/container/internal/raw_hash_set.h absl/container/internal/layout.h absl/container/internal/tracked.h \
          absl/debugging/stacktrace.h absl/debugging/symbolize.h absl/debugging/internal/symbolize.h absl/debugging/symbolize_elf.inc absl/debugging/symbolize_unimplemented.inc absl/debugging/symbolize_win32.inc \
          absl/debugging/internal/examine_stack.h absl/debugging/failure_signal_handler.h absl/debugging/internal/address_is_readable.h absl/debugging/internal/elf_mem_image.h absl/debugging/internal/stacktrace_aarch64-inl.inc \
          absl/debugging/internal/stacktrace_arm-inl.inc absl/debugging/internal/stacktrace_config.h absl/debugging/internal/stacktrace_generic-inl.inc absl/debugging/internal/stacktrace_powerpc-inl.inc \
          absl/debugging/internal/stacktrace_unimplemented-inl.inc absl/debugging/internal/stacktrace_win32-inl.inc absl/debugging/internal/stacktrace_x86-inl.inc absl/debugging/internal/vdso_support.h\
          absl/debugging/internal/demangle.h absl/debugging/leak_check.h absl/debugging/internal/stack_consumption.h \
          absl/debugging/symbolize_darwin.inc \
          absl/flags/internal/path_util.h absl/flags/internal/program_name.h absl/flags/config.h absl/flags/usage_config.h absl/flags/marshalling.h absl/flags/internal/commandlineflag.h absl/flags/internal/registry.h \
          absl/flags/declare.h absl/flags/flag.h absl/flags/internal/flag.h absl/flags/internal/usage.h absl/flags/usage.h absl/flags/internal/parse.h absl/flags/parse.h \
          absl/flags/commandlineflag.cc \
          absl/flags/commandlineflag.h \
          absl/flags/internal/private_handle_accessor.cc \
          absl/flags/internal/private_handle_accessor.h \
          absl/flags/reflection.cc \
          absl/flags/reflection.h \
          absl/functional/bind_front.h absl/functional/function_ref.h absl/functional/internal/front_binder.h absl/functional/internal/function_ref.h \
          absl/hash/hash.h absl/hash/internal/hash.h absl/hash/internal/spy_hash_state.h absl/hash/internal/city.h \
          absl/memory/memory.h absl/meta/type_traits.h \
          absl/numeric/int128.h absl/numeric/int128_have_intrinsic.inc absl/numeric/int128_no_intrinsic.inc \
          absl/random/random.h absl/random/bernoulli_distribution.h absl/random/beta_distribution.h absl/random/bit_gen_ref.h absl/random/discrete_distribution.h absl/random/distributions.h \
          absl/random/exponential_distribution.h absl/random/gaussian_distribution.h absl/random/log_uniform_int_distribution.h absl/random/mock_distributions.h absl/random/mocking_bit_gen.h absl/random/poisson_distribution.h absl/random/uniform_int_distribution.h \
          absl/random/uniform_real_distribution.h absl/random/zipf_distribution.h absl/random/seed_gen_exception.h absl/random/seed_sequences.h absl/random/internal/traits.h absl/random/internal/distribution_caller.h \
          absl/random/internal/fast_uniform_bits.h absl/random/internal/seed_material.h absl/random/internal/pool_urbg.h \
          absl/random/internal/sequence_urbg.h absl/random/internal/salted_seed_seq.h absl/random/internal/iostream_state_saver.h absl/random/internal/mock_overload_set.h \
          absl/random/internal/generate_real.h absl/random/internal/wide_multiply.h \
          absl/random/internal/fastmath.h \
          absl/random/internal/nonsecure_base.h absl/random/internal/pcg_engine.h absl/random/internal/randen_engine.h absl/random/internal/randen_traits.h absl/random/internal/platform.h \
          absl/random/internal/randen.h absl/random/internal/randen_slow.h absl/random/internal/randen_detect.h absl/random/internal/randen_hwaes.h absl/random/internal/randen_hwaes.h absl/random/internal/uniform_helper.h\
          absl/random/internal/mock_helpers.h \
          absl/random/internal/randen_round_keys.cc \
          absl/strings/ascii.h absl/strings/charconv.h absl/strings/escaping.h absl/strings/match.h absl/strings/numbers.h absl/strings/str_cat.h absl/strings/str_join.h absl/strings/str_replace.h absl/strings/str_split.h\
          absl/strings/string_view.h absl/strings/strip.h absl/strings/substitute.h absl/strings/internal/charconv_parse.h absl/strings/internal/escaping.h absl/strings/internal/memutil.h absl/strings/internal/stl_type_traits.h absl/strings/internal/str_join_internal.h \
          absl/strings/internal/str_split_internal.h absl/strings/internal/char_map.h absl/strings/internal/ostringstream.h absl/strings/internal/resize_uninitialized.h absl/strings/internal/utf8.h \
          absl/strings/str_format.h absl/strings/internal/str_format/arg.h absl/strings/internal/str_format/bind.h absl/strings/internal/str_format/checker.h absl/strings/internal/str_format/extension.h \
          absl/strings/internal/str_format/float_conversion.h absl/strings/internal/str_format/output.h absl/strings/internal/str_format/parser.h absl/strings/internal/pow10_helper.h \
          absl/strings/cord.cc \
          absl/strings/cord.h \
          absl/strings/internal/cord_internal.h \
          absl/synchronization/internal/graphcycles.h absl/synchronization/barrier.h absl/synchronization/blocking_counter.h absl/synchronization/internal/create_thread_identity.h absl/synchronization/internal/kernel_timeout.h\
          absl/synchronization/internal/mutex_nonprod.inc absl/synchronization/internal/per_thread_sem.h absl/synchronization/internal/waiter.h absl/synchronization/mutex.h absl/synchronization/notification.h\
          absl/synchronization/internal/thread_pool.h \
          absl/time/civil_time.h absl/time/clock.h absl/time/time.h absl/time/internal/get_current_time_chrono.inc absl/time/internal/get_current_time_posix.inc absl/time/internal/cctz/include/cctz/civil_time.h \
          absl/time/internal/cctz/include/cctz/civil_time_detail.h absl/time/internal/cctz/include/cctz/time_zone.h absl/time/internal/cctz/include/cctz/zone_info_source.h absl/time/internal/cctz/src/time_zone_fixed.h \
          absl/time/internal/cctz/src/time_zone_if.h absl/time/internal/cctz/src/time_zone_impl.h absl/time/internal/cctz/src/time_zone_info.h absl/time/internal/cctz/src/time_zone_libc.h \
          absl/time/internal/cctz/src/time_zone_posix.h absl/time/internal/cctz/src/tzfile.h \
          absl/types/any.h absl/types/bad_any_cast.h absl/types/bad_any_cast.h absl/types/internal/span.h absl/types/optional.h absl/types/span.h absl/types/internal/optional.h absl/types/bad_optional_access.h \
          absl/types/bad_variant_access.h absl/types/variant.h absl/types/internal/variant.h absl/types/compare.h\
          absl/types/internal/conformance_aliases.h absl/types/internal/conformance_archetype.h absl/types/internal/conformance_profile.h \
          absl/types/internal/conformance_testing.h \
          absl/types/internal/conformance_testing_helpers.h \
          absl/types/internal/parentheses.h \
          absl/types/internal/transform_args.h \
          absl/utility/utility.h


noinst_LTLIBRARIES = libabseil.la

libabseil_la_SOURCES = \
                       absl/base/log_severity.cc \
                       absl/base/internal/spinlock_wait.cc absl/base/dynamic_annotations.cc absl/base/internal/low_level_alloc.cc \
                       absl/base/internal/cycleclock.cc absl/base/internal/periodic_sampler.cc absl/base/internal/raw_logging.cc absl/base/internal/spinlock.cc absl/base/internal/sysinfo.cc absl/base/internal/exponential_biased.cc absl/base/internal/thread_identity.cc \
                       absl/base/internal/unscaledcycleclock.cc \
                       absl/base/internal/throw_delegate.cc absl/base/internal/scoped_set_env.cc\
                       absl/container/internal/hashtablez_sampler.cc absl/container/internal/hashtablez_sampler_force_weak_definition.cc absl/container/internal/raw_hash_set.cc \
                       absl/debugging/stacktrace.cc absl/debugging/symbolize.cc absl/debugging/internal/examine_stack.cc absl/debugging/failure_signal_handler.cc \
                       absl/debugging/internal/address_is_readable.cc absl/debugging/internal/elf_mem_image.cc absl/debugging/internal/vdso_support.cc absl/debugging/internal/demangle.cc\
                       absl/debugging/leak_check.cc absl/debugging/leak_check_disable.cc absl/debugging/internal/stack_consumption.cc \
                       absl/flags/internal/flag.cc absl/flags/internal/program_name.cc absl/flags/usage_config.cc absl/flags/marshalling.cc \
                       absl/flags/flag.cc absl/flags/internal/usage.cc absl/flags/usage.cc absl/flags/parse.cc \
                       absl/hash/internal/hash.cc absl/hash/internal/city.cc \
                       absl/numeric/int128.cc  \
                       absl/random/discrete_distribution.cc absl/random/gaussian_distribution.cc absl/random/seed_gen_exception.cc absl/random/seed_sequences.cc absl/random/internal/seed_material.cc absl/random/internal/pool_urbg.cc\
                       absl/random/internal/randen.cc absl/random/internal/randen_slow.cc absl/random/internal/randen_detect.cc \
                       absl/strings/ascii.cc absl/strings/charconv.cc absl/strings/escaping.cc absl/strings/internal/charconv_bigint.cc absl/strings/internal/charconv_bigint.h absl/strings/internal/charconv_parse.cc\
                       absl/strings/internal/escaping.cc absl/strings/internal/memutil.cc absl/strings/match.cc absl/strings/numbers.cc absl/strings/str_cat.cc absl/strings/str_replace.cc absl/strings/str_split.cc absl/strings/string_view.cc \
                       absl/strings/substitute.cc absl/strings/internal/ostringstream.cc absl/strings/internal/utf8.cc absl/strings/internal/str_format/arg.cc absl/strings/internal/str_format/bind.cc \
                       absl/strings/internal/str_format/extension.cc absl/strings/internal/str_format/float_conversion.cc absl/strings/internal/str_format/output.cc absl/strings/internal/str_format/parser.cc\
                       absl/strings/internal/pow10_helper.cc\
                       absl/synchronization/internal/graphcycles.cc absl/synchronization/barrier.cc absl/synchronization/blocking_counter.cc absl/synchronization/internal/create_thread_identity.cc \
                       absl/synchronization/internal/per_thread_sem.cc absl/synchronization/internal/waiter.cc absl/synchronization/notification.cc absl/synchronization/mutex.cc\
                       absl/time/civil_time.cc absl/time/clock.cc absl/time/duration.cc absl/time/format.cc absl/time/time.cc absl/time/internal/cctz/src/civil_time_detail.cc\
                       absl/time/internal/cctz/src/time_zone_fixed.cc absl/time/internal/cctz/src/time_zone_format.cc absl/time/internal/cctz/src/time_zone_if.cc absl/time/internal/cctz/src/time_zone_impl.cc \
                       absl/time/internal/cctz/src/time_zone_info.cc absl/time/internal/cctz/src/time_zone_libc.cc absl/time/internal/cctz/src/time_zone_lookup.cc absl/time/internal/cctz/src/time_zone_posix.cc \
                       absl/time/internal/cctz/src/zone_info_source.cc \
                       absl/types/bad_any_cast.cc absl/types/bad_optional_access.cc absl/types/bad_variant_access.cc
 

libabseil_la_CPPFLAGS = -D__CLANG_SUPPORT_DYN_ANNOTATION__  $(CXXFLAGS_EXT)


