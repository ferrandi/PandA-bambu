#!/bin/bash
#####################################################################
# Appimage build script
#
# The following script takes as arguments the appimage package 
# directory, the appimage output filename, and the configure 
# arguments for the PandA Bambu HLS build configuration.
#
#####################################################################
set -e
script_dir="$(dirname $(readlink -e $0))"
repo_dir="${script_dir}/../.."
build_dir="${repo_dir}/build"

pkg_dir="$1"
shift
out_file="$1"
shift
config_args="$@"

parallel=""
if [ ! -z "$J" ]; then
   parallel="-j$J"
fi

echo "Configure build environment"
mkdir -p ${build_dir}
if [ -d "/usr/lib/ccache" ]; then
   export PATH=/usr/lib/ccache/:$PATH
fi
export LIBRARY_PATH=/usr/lib/x86_64-linux-gnu/:$LIBRARY_PATH
if [ "$(cat ${build_dir}/current_build)" != "${config_args}" ]; then
   echo "Configuration necessary for ${config_args}"
   cd ${build_dir} && ${repo_dir}/configure --prefix=/usr -C ${config_args} && echo "${config_args}" | tee "${build_dir}/current_build"
fi

echo "Build PandA Bambu HLS"
make --directory=${build_dir} ${parallel} install-strip DESTDIR="${pkg_dir}"

echo "Generating AppImage..."
mkdir -p "$(dirname ${out_file})"
ARCH=x86_64 appimagetool ${pkg_dir} ${out_file} 2> /dev/null

echo "AppImage built succesfully"
