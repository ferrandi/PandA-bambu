ARG BASE=ubuntu:xenial

FROM ${BASE} AS gcc
ARG GCC_VERSION=13.3.0
ARG GLIBC_VERSION=2.12
ARG DEBIAN_FRONTEND=noninteractive
ARG DEBCONF_NONINTERACTIVE_SEEN=true
COPY gcc-${GCC_VERSION}*.patch .
RUN { echo 'tzdata tzdata/Areas select Etc'; echo 'tzdata tzdata/Zones/Etc select UTC'; } | debconf-set-selections \
   && apt-get update && apt-get install -y --no-install-recommends \
   bison bzip2 ca-certificates file flex g++ gcc gcc-multilib make patch texinfo tzdata wget xz-utils \
   && rm -rf /var/lib/apt/lists/* \
   && update-ca-certificates \
   && wget "https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.xz" -qO- | tar xJ \
   && cd gcc-${GCC_VERSION} \
   && if test -e /gcc-${GCC_VERSION}.patch; then patch -p0 < /gcc-${GCC_VERSION}.patch; fi \
   && ./contrib/download_prerequisites \
   && mkdir build \
   && cd build \
   && ../configure --prefix=/usr --enable-languages=c,c++ --enable-multilib --with-multilib-list=m64,m32,mx32 \
   --disable-werror --disable-gcov --disable-nls --with-tune=generic --with-glibc-version=${GLIBC_VERSION} \
   && make -j32 \
   && make DESTDIR="/opt/gcc" install -j32 \
   && cd ../.. \
   && rm -rf gcc-* \
   && mkdir -p /opt/gcc/usr/lib/x86_64-linux-gnu \
   && cp -P `find /opt/gcc/usr/lib64 -name 'libstdc++*' -not -name '*.la'` /opt/gcc/usr/lib/x86_64-linux-gnu/

FROM ${BASE} AS core
ARG DEBIAN_FRONTEND=noninteractive
ARG DEBCONF_NONINTERACTIVE_SEEN=true
RUN { echo 'tzdata tzdata/Areas select Etc'; echo 'tzdata tzdata/Zones/Etc select UTC'; } | debconf-set-selections \
   && apt-get update && apt-get install -y --no-install-recommends \
   binutils bison bzip2 ca-certificates file libc-dev make texinfo tzdata wget xz-utils \
   && rm -rf /var/lib/apt/lists/* \
   && update-ca-certificates
COPY --from=gcc /opt/gcc /

FROM core AS zlib
ARG ZLIB_VERSION=1.3.1
RUN wget https://www.zlib.net/zlib-${ZLIB_VERSION}.tar.gz -qO- | tar xz \
   && cd zlib-${ZLIB_VERSION} \
   && ./configure --prefix=/usr \
   && make -j16 \
   && make DESTDIR="/opt/sysroot" install \
   && cd .. \
   && rm -rf zlib-*

FROM core AS libtool
ARG LIBTOOL_VERSION=2.4.7
RUN wget https://release.bambuhls.eu/libs/libtool-${LIBTOOL_VERSION}.tar.gz -qO- | tar xz \
   && cd libtool-${LIBTOOL_VERSION} \
   && ./configure --prefix=/usr --libdir=/usr/lib64 \
   && make -j16 \
   && make DESTDIR="/opt/sysroot" install \
   && cd .. \
   && rm -rf libtool-*

FROM core AS libgmp
ARG GMP_VERSION=6.3.0
RUN wget https://gmplib.org/download/gmp/gmp-${GMP_VERSION}.tar.xz -qO- | tar xJ \
   && cd gmp-${GMP_VERSION} \
   && ./configure --prefix=/usr --enable-cxx \
   && make -j16 \
   && make DESTDIR="/opt/sysroot" install \
   && cd .. \
   && rm -rf gmp-*

FROM core AS flex
ARG FLEX_VERSION=2.6.4
RUN wget https://github.com/westes/flex/releases/download/v${FLEX_VERSION}/flex-${FLEX_VERSION}.tar.gz -qO- | tar xz \
   && cd flex-${FLEX_VERSION} \
   && ./configure --prefix=/usr \
   && make -j16 \
   && make DESTDIR="/opt/sysroot" install \
   && cd .. \
   && rm -rf flex-*

FROM core AS boost
ARG BOOST_VERSION=1.82.0
RUN wget https://github.com/boostorg/boost/releases/download/boost-${BOOST_VERSION}/boost-${BOOST_VERSION}.tar.xz -qO- | tar xJ \
   && cd boost-${BOOST_VERSION} \
   && ./bootstrap.sh --with-libraries=headers,test --without-icu --prefix=/opt/sysroot/usr \
   && ./b2 release install \
   && cd .. \
   && rm -rf boost*

FROM core AS libssl
ARG LIBSSL_VERSION=3.3.1
RUN wget https://github.com/openssl/openssl/releases/download/openssl-${LIBSSL_VERSION}/openssl-${LIBSSL_VERSION}.tar.gz -qO- | tar xz \
   && cd openssl-${LIBSSL_VERSION} \
   && ./Configure --prefix=/usr --libdir=lib/x86_64-linux-gnu \
   && make -j16 \
   && make DESTDIR="/opt/sysroot" install \
   && cd .. \
   && rm -rf openssl-*

FROM core AS cmake
COPY --link --from=libssl /opt/sysroot/ /
ARG CMAKE_VERSION=3.26.6
RUN wget https://gitlab.kitware.com/cmake/cmake/-/archive/v${CMAKE_VERSION}/cmake-v${CMAKE_VERSION}.tar.gz -qO- | tar xz \
   && cd cmake-v${CMAKE_VERSION} \
   && ./bootstrap --parallel=16 --prefix=/usr -- -DCMAKE_BUILD_TYPE:STRING=Release \
   && make -j16 \
   && make DESTDIR="/opt/sysroot" install \
   && cd .. \
   && rm -rf cmake-*

FROM core AS libmpfr
COPY --link --from=libgmp /opt/sysroot/ /
ARG MPFR_VERSION=4.2.1
RUN wget https://www.mpfr.org/mpfr-current/mpfr-${MPFR_VERSION}.tar.xz -qO- | tar xJ \
   && cd mpfr-${MPFR_VERSION} \
   && ./configure --prefix=/usr \
   && make -j16 \
   && make DESTDIR="/opt/sysroot" install \
   && cd .. \
   && rm -rf mpfr-*

# GCC plugin dependency
FROM core AS libmpc
COPY --link --from=libgmp /opt/sysroot/ /
COPY --link --from=libmpfr /opt/sysroot/ /
ARG MPC_VERSION=1.3.1
RUN wget https://ftp.gnu.org/gnu/mpc/mpc-${MPC_VERSION}.tar.gz -qO- | tar xz \
   && cd mpc-${MPC_VERSION} \
   && ./configure --prefix=/usr \
   && make -j16 \
   && make DESTDIR="/opt/sysroot" install \
   && cd .. \
   && rm -rf mpc-*

FROM core AS python
COPY --link --from=zlib /opt/sysroot /
COPY --link --from=libssl /opt/sysroot /
ARG DEBIAN_FRONTEND=noninteractive
ARG DEBCONF_NONINTERACTIVE_SEEN=true
ARG PYTHON_VERSION=3.11.11
COPY python-${PYTHON_VERSION}*.patch .
RUN apt-get update && apt-get install -y --no-install-recommends libffi-dev patch \
   && rm -rf /var/lib/apt/lists/* \
   && wget https://www.python.org/ftp/python/${PYTHON_VERSION}/Python-${PYTHON_VERSION}.tar.xz -qO- | tar xJ \
   && cd Python-${PYTHON_VERSION} \
   && if test -e /python-${PYTHON_VERSION}.patch; then patch -p0 < /python-${PYTHON_VERSION}.patch; fi \
   && ./configure --prefix=/usr --enable-shared --disable-ipv6 \
   && make -j16 \
   && make DESTDIR="/opt/sysroot" install \
   && cd .. \
   && rm -rf Python-*

FROM core AS pcre
ARG DEBIAN_FRONTEND=noninteractive
ARG DEBCONF_NONINTERACTIVE_SEEN=true
ARG PCRE_VERSION=8.45
RUN wget https://sourceforge.net/projects/pcre/files/pcre/${PCRE_VERSION}/pcre-${PCRE_VERSION}.tar.gz/download -qO- | tar xz \
   && cd pcre-${PCRE_VERSION} \
   && ./configure --prefix="/usr" \
   && make -j16 DESTDIR="/opt/sysroot" install \
   && cd .. \
   && rm -rf pcre-*

FROM core AS cppcheck
COPY --link --from=zlib /opt/sysroot /
COPY --link --from=libssl /opt/sysroot /
COPY --link --from=python /opt/sysroot /
COPY --link --from=pcre /opt/sysroot /
ARG DEBIAN_FRONTEND=noninteractive
ARG DEBCONF_NONINTERACTIVE_SEEN=true
ARG CPPCHECK_VERSION=2.14.1
RUN wget https://github.com/danmar/cppcheck/archive/${CPPCHECK_VERSION}.tar.gz -qO- | tar xz \
   && cd cppcheck-${CPPCHECK_VERSION} \
   && make -j16 MATCHCOMPILER=yes FILESDIR=/usr/share/cppcheck HAVE_RULES=yes CXXFLAGS="-O2 -DNDEBUG -Wall -Wno-sign-compare -Wno-unused-function" DESTDIR="/opt/sysroot" install \
   && cd .. \ 
   && rm -rf cppcheck-*

FROM core AS clang-tidy
COPY --link --from=zlib /opt/sysroot /
COPY --link --from=libssl /opt/sysroot /
COPY --link --from=python /opt/sysroot /
COPY --link --from=cmake /opt/sysroot /
ARG LLVM_VERSION=18.1.8
RUN wget https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}/llvm-project-${LLVM_VERSION}.src.tar.xz -qO- | tar xJ \
   && cd llvm-project-${LLVM_VERSION}.src \
   && cmake -B build llvm -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DLLVM_TARGETS_TO_BUILD=X86 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr \
   && cmake --build build --target clang-tidy --parallel 16 \
   && cmake --build build --target install-clang-tidy DESTDIR="/opt/sysroot" \
   && cd .. \ 
   && rm -rf llvm-project-*

FROM core AS bear
COPY --link --from=libssl /opt/sysroot /
COPY --link --from=cmake /opt/sysroot /
ARG DEBIAN_FRONTEND=noninteractive
ARG DEBCONF_NONINTERACTIVE_SEEN=true
ARG BEAR_VERSION=3.1.4
RUN apt-get update && apt-get install -y --no-install-recommends git pkg-config \
   && rm -rf /var/lib/apt/lists/* \
   && wget https://github.com/rizsotto/Bear/archive/refs/tags/${BEAR_VERSION}.tar.gz -qO- | tar xz \
   && cd Bear-${BEAR_VERSION} \
   && cmake -B build . -DENABLE_UNIT_TESTS=OFF -DENABLE_FUNC_TESTS=OFF -DCMAKE_INSTALL_PREFIX=/usr \
   && cmake --build build --parallel 16 \
   && cmake --build build --target install DESTDIR="/opt/sysroot" \
   && cd .. \
   && rm -rf Bear-*


FROM core AS codechecks
COPY --link --from=libssl /opt/sysroot /
COPY --link --from=cmake /opt/sysroot /
COPY --link --from=zlib /opt/sysroot /
COPY --link --from=python /opt/sysroot /
COPY --link --from=boost /opt/sysroot /
COPY --link --from=clang-tidy /opt/sysroot /
COPY --link --from=pcre /opt/sysroot /
COPY --link --from=cppcheck /opt/sysroot /
RUN python3 -m pip install pygments

FROM core AS toolchain
ARG DEBIAN_FRONTEND=noninteractive
ARG DEBCONF_NONINTERACTIVE_SEEN=true
RUN apt-get update && apt-get install -y --no-install-recommends chrpath libc6-dev-i386 libc6-dev-x32 libffi-dev git \
   && rm -rf /var/lib/apt/lists/*
COPY --link --from=zlib /opt/sysroot /
COPY --link --from=flex /opt/sysroot /
COPY --link --from=libssl /opt/sysroot /
COPY --link --from=cmake /opt/sysroot /
COPY --link --from=python /opt/sysroot /

FROM ${BASE} AS dev
ARG DEBIAN_FRONTEND=noninteractive
ARG DEBCONF_NONINTERACTIVE_SEEN=true
RUN { echo 'tzdata tzdata/Areas select Etc'; echo 'tzdata tzdata/Zones/Etc select UTC'; } | debconf-set-selections \
   && apt-get update && apt-get install -y --no-install-recommends \
   appstream autoconf autoconf-archive automake binutils bison bzip2 ca-certificates \
   ccache curl doxygen ghostscript git graphviz libc-dev libc6-dev-i386 libc6-dev-x32 \
   locales lsb-release make pkg-config rsync texinfo time tzdata wget xz-utils \
   texlive-fonts-recommended texlive-latex-extra texlive-latex-recommended \
   libtinfo5 \
   && rm -rf /var/lib/apt/lists/* \
   && update-ca-certificates \
   && if test -e /var/cache/app-info/xapian/default; then touch /var/cache/app-info/xapian/default; fi \
   && locale-gen en_US.UTF-8 \
   && ln -sf /usr/bin/ccache /usr/lib/ccache/gcc \
   && ln -sf /usr/bin/ccache /usr/lib/ccache/g++ \
   # Install AppImage tools
   && wget "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage" \
   && chmod +x appimagetool-x86_64.AppImage \
   && ./appimagetool-x86_64.AppImage --appimage-extract \
   && mv squashfs-root /opt/appimage-tool.AppDir \
   && ln -sf /opt/appimage-tool.AppDir/AppRun /usr/bin/appimagetool \
   && rm -f appimagetool-x86_64.AppImage
COPY --link --from=gcc /opt/gcc /
COPY --link --from=zlib /opt/sysroot /
COPY --link --from=libtool /opt/sysroot /
COPY --link --from=libgmp /opt/sysroot /
COPY --link --from=flex /opt/sysroot /
COPY --link --from=boost /opt/sysroot /
COPY --link --from=libssl /opt/sysroot /
COPY --link --from=cmake /opt/sysroot /
COPY --link --from=bear /opt/sysroot /
COPY --link --from=libmpfr /opt/sysroot /
COPY --link --from=libmpc /opt/sysroot /
COPY --link --from=libmpfi /opt/sysroot /
COPY --link --from=libxml2 /opt/sysroot /

CMD [ "/bin/bash" ]
