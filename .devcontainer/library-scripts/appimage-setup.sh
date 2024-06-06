#!/bin/bash
#####################################################################
# AppImage build environment setup
#
# The following script takes as arguments the appimage package 
# directory and the list of compilers to include into the appimage.
#####################################################################
set -e
script_dir="$(dirname $(readlink -e $0))"
repo_dir="${script_dir}/../.."

pkg_dir="$1"

mkdir -p ${pkg_dir}

echo "Inflating libraries..."
mkdir -p ${pkg_dir}/usr/lib/
mkdir -p ${pkg_dir}/lib/x86_64-linux-gnu/
cp -d $(dirname `gcc -print-file-name=libtinfo.so.5`)/libtinfo.so* ${pkg_dir}/lib/x86_64-linux-gnu/
cp -d $(dirname `gcc -print-file-name=libicuio.so`)/libicu*.so* ${pkg_dir}/lib/x86_64-linux-gnu/
cp -d `gcc -print-file-name=libbdd.so`* ${pkg_dir}/usr/lib/

echo "Inflating metadata..."
cp ${repo_dir}/style/img/panda.png.in ${pkg_dir}/bambu.png
cat > ${pkg_dir}/bambu.desktop << EOF
[Desktop Entry]
Name=bambu
Exec=tool_select.sh
Icon=bambu
Type=Application
Terminal=true
Categories=Development;
EOF
# Change back to OWD - Original Working Directory
# Set by AppImage
# see https://github.com/AppImage/AppImageKit/blob/master/src/runtime.c
mkdir -p ${pkg_dir}/usr/bin
cat > ${pkg_dir}/usr/bin/tool_select.sh << EOF
#!/bin/bash
export LC_ALL="C"
unset PYTHONHOME  # Python is not bundled with this AppImage
unset PYTHONPATH
BINARY_NAME=\$(basename "\${ARGV0}")
BINARY_PATH="\${APPDIR}/usr/bin/\${BINARY_NAME}"
if [ "\$BINARY_NAME" == "\$(basename \\"\${SHELL}\\")" ]; then
   BINARY_PATH="\${SHELL}"
fi
if [ ! -e "\${BINARY_PATH}" ]; then
   BINARY_PATH="\${APPDIR}/usr/bin/bambu"
fi
cd "\${OWD}"
\${BINARY_PATH} "\$@"
EOF
chmod a+x ${pkg_dir}/usr/bin/tool_select.sh

echo "Inflating AppImage runtime..."
curl -L https://github.com/AppImage/AppImageKit/releases/download/continuous/AppRun-x86_64 -o ${pkg_dir}/AppRun -s
chmod +x ${pkg_dir}/AppRun

echo "AppImage package environment ready"
