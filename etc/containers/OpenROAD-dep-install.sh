#!/bin/bash

set -euo pipefail

lastDir="$(pwd)"
# tools versions
cmakeVersionBig=3.14
cmakeVersionSmall=${cmakeVersionBig}.0
swigVersion=4.0.1
boostVersionBig=1.72
boostVersionSmall=${boostVersionBig}.0
lemonVersion=1.3.1
spdlogVersion=1.8.1
# temp dir to download and compile
baseDir=/tmp/installers
mkdir -p "${baseDir}"

# CMake
if [[ -z $(cmake --version | grep ${cmakeVersionBig}) ]]; then
    cd "${baseDir}"
    wget https://cmake.org/files/v${cmakeVersionBig}/cmake-${cmakeVersionSmall}-Linux-x86_64.sh
    md5sum -c <(echo "73041a43d27a30cdcbfdfdb61310d081  cmake-${cmakeVersionSmall}-Linux-x86_64.sh") || exit 1
    chmod +x cmake-${cmakeVersionSmall}-Linux-x86_64.sh
    ./cmake-${cmakeVersionSmall}-Linux-x86_64.sh --skip-license --prefix=/usr/local
else
    echo "CMake already installed."
fi

# SWIG
if [[ -z $(swig -version | grep ${swigVersion}) ]]; then
    cd "${baseDir}"
    wget https://github.com/swig/swig/archive/rel-${swigVersion}.tar.gz
    md5sum -c <(echo "ef6a6d1dec755d867e7f5e860dc961f7  rel-${swigVersion}.tar.gz") || exit 1
    tar xfz rel-${swigVersion}.tar.gz
    cd swig-rel-${swigVersion}
    ./autogen.sh
    ./configure --prefix=/usr
    make -j $(nproc)
    make -j $(nproc) install
else
    echo "Swig already installed."
fi

# boost
if [[ -z $(grep "BOOST_LIB_VERSION \"${boostVersionBig//./_}\"" /usr/local/include/boost/version.hpp) ]]; then
    cd "${baseDir}"
    boostVersionUnderscore=${boostVersionSmall//./_}
    wget https://boostorg.jfrog.io/artifactory/main/release/${boostVersionSmall}/source/boost_${boostVersionUnderscore}.tar.gz
    md5sum -c <(echo "e2b0b1eac302880461bcbef097171758  boost_${boostVersionUnderscore}.tar.gz") || exit 1
    tar -xf boost_${boostVersionUnderscore}.tar.gz
    cd boost_${boostVersionUnderscore}
    ./bootstrap.sh
    ./b2 install -j $(nproc)
else
    echo "Boost already installed."
fi

# lemon
if [[ -z $(grep "LEMON_VERSION \"${lemonVersion}\"" /usr/local/include/lemon/config.h) ]]; then
    cd "${baseDir}"
    wget http://lemon.cs.elte.hu/pub/sources/lemon-${lemonVersion}.tar.gz
    md5sum -c <(echo "e89f887559113b68657eca67cf3329b5  lemon-${lemonVersion}.tar.gz") || exit 1
    tar -xf lemon-${lemonVersion}.tar.gz
    cd lemon-${lemonVersion}
    cmake -B build .
    cmake --build build -j $(nproc) --target install
else
    echo "Lemon already installed."
fi

# spdlog
if [[ -z $(grep "PACKAGE_VERSION \"${spdlogVersion}\"" ${spdlogFolder}) ]]; then
    cd "${baseDir}"
    git clone -b "v${spdlogVersion}" https://github.com/gabime/spdlog.git
    cd spdlog
    cmake -B build .
    cmake --build build -j $(nproc) --target install
else
    echo "spdlog already installed."
fi
cd "$lastDir"
rm -rf "${baseDir}"