# AC_COMPILE_STDCXX_17
AC_DEFUN([AC_COMPILE_STDCXX_17], [
  AC_CACHE_CHECK(if g++ supports C++17 features without additional flags,
  ac_cv_cxx_compile_cxx17_native,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_COMPILE([
  #include <optional>
  #include <tuple>
  #include <unordered_map>
  #include <unordered_set>
  template <typename T>
    struct check final
    {
      static constexpr T value{ __cplusplus };
    };

    typedef check<check<bool>> right_angle_brackets;

    int a;
    decltype(a) b;

    typedef check<int> check_type;
    check_type c{};
    check_type&& cr = static_cast<check_type&&>(c);

    static_assert(check_type::value == 201703L, "C++17 compiler");
    enum class Enum;
    enum class Enum
    {
       ENUM
    };
    ],,
  ac_cv_cxx_compile_cxx17_native=yes, ac_cv_cxx_compile_cxx17_native=no)
  AC_LANG_RESTORE
  ])

  AC_CACHE_CHECK(if g++ supports C++17 features with -std=c++17,
  ac_cv_cxx_compile_cxx17_cxx,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  ac_save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -std=c++17"
  AC_TRY_COMPILE([
  #include <optional>
  #include <tuple>
  #include <unordered_map>
  #include <unordered_set>
  template <typename T>
    struct check final
    {
      static constexpr T value{ __cplusplus };
    };

    typedef check<check<bool>> right_angle_brackets;

    int a;
    decltype(a) b;

    typedef check<int> check_type;
    check_type c{};
    check_type&& cr = static_cast<check_type&&>(c);

    static_assert(check_type::value == 201703L, "C++17 compiler");
    enum class Enum;
    enum class Enum
    {
       ENUM
    };
    ],,
  ac_cv_cxx_compile_cxx17_cxx=yes, ac_cv_cxx_compile_cxx17_cxx=no)
  CXXFLAGS="$ac_save_CXXFLAGS"
  AC_LANG_RESTORE
  ])

  AC_CACHE_CHECK(if g++ supports C++17 features with -std=gnu++17,
  ac_cv_cxx_compile_cxx17_gxx,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  ac_save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -std=gnu++17"
  AC_TRY_COMPILE([
  #include <tuple>
  #include <optional>
  #include <unordered_map>
  #include <unordered_set>
  template <typename T>
    struct check final
    {
      static constexpr T value{ __cplusplus };
    };

    typedef check<check<bool>> right_angle_brackets;

    int a;
    decltype(a) b;

    typedef check<int> check_type;
    check_type c{};
    check_type&& cr = static_cast<check_type&&>(c);

    static_assert(check_type::value == 201703L, "C++17 compiler");
    enum class Enum;
    enum class Enum
    {
       ENUM
    };
    ],,
  ac_cv_cxx_compile_cxx17_gxx=yes, ac_cv_cxx_compile_cxx17_gxx=no)
  CXXFLAGS="$ac_save_CXXFLAGS"
  AC_LANG_RESTORE
  STDCXX_17=no
  ])

  AC_CACHE_CHECK(if g++ supports C++17 features with -std=c++1z,
  ac_cv_cxx_compile_cxx17_cxx1z,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  ac_save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -std=c++1z"
  AC_TRY_COMPILE([
  #include <tuple>
  #include <optional>
  #include <unordered_map>
  #include <unordered_set>
  template <typename T>
    struct check final
    {
      static constexpr T value{ __cplusplus };
    };

    typedef check<check<bool>> right_angle_brackets;

    int a;
    decltype(a) b;

    typedef check<int> check_type;
    check_type c{};
    check_type&& cr = static_cast<check_type&&>(c);

    static_assert(check_type::value == 201703L, "C++17 compiler");
    enum class Enum;
    enum class Enum
    {
       ENUM
    };
    ],,
  ac_cv_cxx_compile_cxx17_cxx1z=yes, ac_cv_cxx_compile_cxx17_cxx1z=no)
  CXXFLAGS="$ac_save_CXXFLAGS"
  AC_LANG_RESTORE
  STDCXX_17=no
  ])

  if test "$ac_cv_cxx_compile_cxx17_native" = yes ||
     test "$ac_cv_cxx_compile_cxx17_cxx" = yes ||
     test "$ac_cv_cxx_compile_cxx17_gxx" = yes ||
     test "$ac_cv_cxx_compile_cxx17_cxx1z" = yes; then
    AC_DEFINE(HAVE_STDCXX_17,1,[Define if g++ supports C++17 features. ])
    STDCXX_17=yes
  fi
])

# AC_COMPILE_STDCXX_11
AC_DEFUN([AC_COMPILE_STDCXX_11], [
  AC_CACHE_CHECK(if g++ supports C++11 features without additional flags,
  ac_cv_cxx_compile_cxx11_native,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_COMPILE([
  #include <tuple>
  #include <unordered_map>
  #include <unordered_set>
  template <typename T>
    struct check final
    {
      static constexpr T value{ __cplusplus };
    };

    typedef check<check<bool>> right_angle_brackets;

    int a;
    decltype(a) b;

    typedef check<int> check_type;
    check_type c{};
    check_type&& cr = static_cast<check_type&&>(c);

    static_assert(check_type::value == 201103L, "C++11 compiler");
    enum class Enum;
    enum class Enum
    {
       ENUM
    };
    ],,
  ac_cv_cxx_compile_cxx11_native=yes, ac_cv_cxx_compile_cxx11_native=no)
  AC_LANG_RESTORE
  ])

  AC_CACHE_CHECK(if g++ supports C++11 features with -std=c++11,
  ac_cv_cxx_compile_cxx11_cxx,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  ac_save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -std=c++11"
  AC_TRY_COMPILE([
  #include <tuple>
  #include <unordered_map>
  #include <unordered_set>
  template <typename T>
    struct check final
    {
      static constexpr T value{ __cplusplus };
    };

    typedef check<check<bool>> right_angle_brackets;

    int a;
    decltype(a) b;

    typedef check<int> check_type;
    check_type c{};
    check_type&& cr = static_cast<check_type&&>(c);

    static_assert(check_type::value == 201103L, "C++11 compiler");
    enum class Enum;
    enum class Enum
    {
       ENUM
    };
    ],,
  ac_cv_cxx_compile_cxx11_cxx=yes, ac_cv_cxx_compile_cxx11_cxx=no)
  CXXFLAGS="$ac_save_CXXFLAGS"
  AC_LANG_RESTORE
  ])

  AC_CACHE_CHECK(if g++ supports C++11 features with -std=gnu++11,
  ac_cv_cxx_compile_cxx11_gxx,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  ac_save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -std=gnu++11"
  AC_TRY_COMPILE([
  #include <tuple>
  #include <unordered_map>
  #include <unordered_set>
  template <typename T>
    struct check final
    {
      static constexpr T value{ __cplusplus };
    };

    typedef check<check<bool>> right_angle_brackets;

    int a;
    decltype(a) b;

    typedef check<int> check_type;
    check_type c{};
    check_type&& cr = static_cast<check_type&&>(c);

    static_assert(check_type::value == 201103L, "C++11 compiler");
    enum class Enum;
    enum class Enum
    {
       ENUM
    };
    ],,
  ac_cv_cxx_compile_cxx11_gxx=yes, ac_cv_cxx_compile_cxx11_gxx=no)
  CXXFLAGS="$ac_save_CXXFLAGS"
  AC_LANG_RESTORE
  STDCXX_11=no
  ])

  AC_CACHE_CHECK(if g++ supports C++11 features with -std=c++1x,
  ac_cv_cxx_compile_cxx11_cxx1x,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  ac_save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -std=c++1x"
  AC_TRY_COMPILE([
  #include <tuple>
  #include <unordered_map>
  #include <unordered_set>
  template <typename T>
    struct check final
    {
      static constexpr T value{ __cplusplus };
    };

    typedef check<check<bool>> right_angle_brackets;

    int a;
    decltype(a) b;

    typedef check<int> check_type;
    check_type c{};
    check_type&& cr = static_cast<check_type&&>(c);

    static_assert(check_type::value == 201103L, "C++11 compiler");
    enum class Enum;
    enum class Enum
    {
       ENUM
    };
    ],,
  ac_cv_cxx_compile_cxx11_cxx1x=yes, ac_cv_cxx_compile_cxx11_cxx1x=no)
  CXXFLAGS="$ac_save_CXXFLAGS"
  AC_LANG_RESTORE
  STDCXX_11=no
  ])

  if test "$ac_cv_cxx_compile_cxx11_native" = yes ||
     test "$ac_cv_cxx_compile_cxx11_cxx" = yes ||
     test "$ac_cv_cxx_compile_cxx11_gxx" = yes ||
     test "$ac_cv_cxx_compile_cxx11_cxx1x" = yes; then
    AC_DEFINE(HAVE_STDCXX_11,1,[Define if g++ supports C++11 features. ])
    STDCXX_11=yes
  fi
])

AC_DEFUN([AC_COMPILE_STDCXX_0X], [
  AC_CACHE_CHECK(if g++ supports C++0x features without additional flags,
  ac_cv_cxx_compile_cxx0x_native,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_COMPILE([
  #include <tuple>
  #include <unordered_map>
  #include <unordered_set>
  int a;],,
  ac_cv_cxx_compile_cxx0x_native=yes, ac_cv_cxx_compile_cxx0x_native=no)
  AC_LANG_RESTORE
  ])

  AC_CACHE_CHECK(if g++ supports C++0x features with -std=c++0x,
  ac_cv_cxx_compile_cxx0x_cxx,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  ac_save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -std=c++0x"
  AC_TRY_COMPILE([
  #include <tuple>
  #include <unordered_map>
  #include <unordered_set>
  enum class Enum;
  enum class Enum
  {
     ENUM
  };
  int a;],,
  ac_cv_cxx_compile_cxx0x_cxx=yes, ac_cv_cxx_compile_cxx0x_cxx=no)
  CXXFLAGS="$ac_save_CXXFLAGS"
  AC_LANG_RESTORE
  ])

  if test "$ac_cv_cxx_compile_cxx0x_native" = yes ||
     test "$ac_cv_cxx_compile_cxx0x_cxx" = yes; then
    AC_DEFINE(HAVE_STDCXX_0X,1,[Define if g++ supports C++0X features. ])
    STDCXX_0X=yes
  fi
])


AC_DEFUN([AC_HEXFLOAT], [
  AC_CACHE_CHECK(if g++ supports hexfloat features without additional flags,
  ac_cv_cxx_compile_hexfloat_native,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_COMPILE([
  #include <sstream>
  #include <iostream>
  ],[std::stringstream ssX;ssX << std::hexfloat << 1.5;return 0;],
  ac_cv_cxx_compile_hexfloat_native=yes, ac_cv_cxx_compile_hexfloat_native=no)
  AC_LANG_RESTORE
  ])
  AC_CACHE_CHECK(if g++ supports hexfloat features with -std=c++0x flag,
  ac_cv_cxx_compile_hexfloat_c0x,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  ac_save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -std=c++0x"
  AC_TRY_COMPILE([
  #include <sstream>
  #include <iostream>
  ],[std::stringstream ssX;ssX << std::hexfloat << 1.5;return 0;],
  ac_cv_cxx_compile_hexfloat_c0x=yes, ac_cv_cxx_compile_hexfloat_c0x=no)
  CXXFLAGS="$ac_save_CXXFLAGS"
  AC_LANG_RESTORE
  ])

  AC_CACHE_CHECK(if g++ supports hexfloat features with -std=c++11 flag,
  ac_cv_cxx_compile_hexfloat_c11,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  ac_save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -std=c++11"
  AC_TRY_COMPILE([
  #include <sstream>
  #include <iostream>
  ],[std::stringstream ssX;ssX << std::hexfloat << 1.5;return 0;],
  ac_cv_cxx_compile_hexfloat_c11=yes, ac_cv_cxx_compile_hexfloat_c11=no)
  CXXFLAGS="$ac_save_CXXFLAGS"
  AC_LANG_RESTORE
  ])

  if test "$ac_cv_cxx_compile_hexfloat_native" = yes || test "$ac_cv_cxx_compile_hexfloat_c0x" = yes ||
     test "$ac_cv_cxx_compile_hexfloat_c11" = yes; then
    AC_DEFINE(HAVE_HEXFLOAT,1,[Define if g++ supports std::hexfloat features. ])
  fi
])
