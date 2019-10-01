#!/bin/bash

# Exit immediately on error
set -e

# Get our method as the name of this script, url and args
URL="$1"
shift

# Set installation prefix, default to /usr/local unless an external power says otherwise
PREFIX=${PREFIX:-"/usr/local"}

BUILD_FOLDER="/var/tmp/build"

# Pull in the toolchain arguments
. /usr/local/toolchain.sh

# Setup the temporary build directories
mkdir -p "${BUILD_FOLDER}"
cd "${BUILD_FOLDER}"

# Download the source code
download-and-extract ${URL}

# Apply patches
SOURCE_DIR=$(find . -maxdepth 1 -type d | tail -n1)
cd "${SOURCE_DIR}"
wget "https://github.com/intel/opencl-clang/commit/a6e69b30a6a2c925254784be808ae3171ecd75ea.patch" -O - | patch -Np1
wget "https://github.com/intel/opencl-clang/commit/94af090661d7c953c516c97a25ed053c744a0737.patch" -O - | patch -Np1

ARGS="$@"

echo "Configuring using cmake"

# Find the closest configure file to the root
CMAKELISTS_FILE=$(find -type f -name 'CMakeLists.txt' -printf '%d\t%P\n' | sort -nk1 | cut -f2- | head -n 1)
cd $(dirname ${CMAKELISTS_FILE})

echo "Building linux resource linker"
cd "linux_linker"
"${CXX}" -mtune=generic ./linux_resource_linker.cpp -o "${PREFIX}/bin/linux_resource_linker"

echo "Configuring using cmake file ${CMAKELISTS_FILE}"
cd ..

# Do an out of source build
mkdir -p build
cd build

# Configure using cmake
cmake .. \
    -DCMAKE_BUILD_TYPE="Release" \
    -DCMAKE_C_FLAGS="${CFLAGS}" \
    -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
    -DCMAKE_INSTALL_PREFIX:PATH="${PREFIX}" \
    -DPKG_CONFIG_USE_CMAKE_PREFIX_PATH=ON \
    -DCMAKE_PREFIX_PATH:PATH="${PREFIX}" \
    -DCMAKE_INSTALL_LIBDIR=lib \
    -DLINUX_RESOURCE_LINKER_COMMAND:PATH="${PREFIX}/bin/linux_resource_linker" \
    -DLLVMSPIRV_INCLUDED_IN_LLVM=OFF \
    -DSPIRV_TRANSLATOR_DIR=/usr/local \
    -DLLVM_NO_DEAD_STRIP=ON \
    -DUSE_PREBUILT_LLVM=ON \
    -Wno-dev

# Run make
make -j$(nproc)

# Now install
make install

# Now that we have built, cleanup the build directory
rm -rf "${BUILD_FOLDER}"
