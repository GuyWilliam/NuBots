##############################################
#   ___  ____    ____       _                #
#  / _ \/ ___|  / ___|  ___| |_ _   _ _ __   #
# | | | \___ \  \___ \ / _ \ __| | | | '_ \  #
# | |_| |___) |  ___) |  __/ |_| |_| | |_) | #
#  \___/|____/  |____/ \___|\__|\__,_| .__/  #
#                                    |_|     #
##############################################
FROM archlinux/base:latest

# Create the nubots user, and setup sudo so no password is required
RUN groupadd -r nubots && useradd --no-log-init -r -g nubots nubots
COPY --chown=root:root etc/sudoers.d/user /etc/sudoers.d/user

# Add a script that installs packages
COPY --chown=nubots:nubots usr/local/bin/install-package /usr/local/bin/install-package

# Install base packages needed for building general toolchain
# If you have a tool that's needed for a specific module install it before that module
RUN install-package \
    sudo \
    wget \
    python \
    python-pip \
    base-devel \
    ninja \
    cmake \
    meson \
    git

# Get python to look in /usr/local for packages
RUN sed "s/^\(PREFIXES\s=\s\)\[\([^]]*\)\]/\1[\2, '\/usr\/local']/" -i /usr/lib/python3.7/site.py \
    && mkdir -p /usr/local/lib/python3.7/site-packages
COPY --chown=root:root etc/pip.conf /etc/pip.conf

# Make sure /usr/local is checked for libraries and binaries
COPY --chown=root:root etc/ld.so.conf.d/usrlocal.conf /etc/ld.so.conf.d/usrlocal.conf
RUN ldconfig

# Create the home directory owned by nubots
RUN mkdir -p /home/nubots && chown -R nubots:nubots /home/nubots

# Setup /usr/local owned by nubots and swap to the nubots user
RUN chown -R nubots:nubots /usr/local
USER nubots

# Make a symlink from /usr/local/lib to /usr/local/lib64 so library install location is irrelevant
RUN ln -s /usr/local/lib /usr/local/lib64

# Copy across the generic toolchain file for building tools
COPY --chown=nubots:nubots usr/local/toolchain/generic.sh /usr/local/toolchain.sh

# Copy over a tool to install simple standard conforming libraries from source
COPY --chown=nubots:nubots usr/local/bin/install-from-source /usr/local/bin/install-from-source
RUN ln -s /usr/local/bin/install-from-source /usr/local/bin/install-header-from-source \
    && ln -s /usr/local/bin/install-from-source /usr/local/bin/install-cmake-from-source \
    && ln -s /usr/local/bin/install-from-source /usr/local/bin/install-autotools-from-source \
    && ln -s /usr/local/bin/install-from-source /usr/local/bin/install-bjam-from-source \
    && ln -s /usr/local/bin/install-from-source /usr/local/bin/install-make-from-source \
    && ln -s /usr/local/bin/install-from-source /usr/local/bin/install-meson-from-source \
    && ln -s /usr/local/bin/install-from-source /usr/local/bin/install-python-from-source \
    && ln -s /usr/local/bin/install-from-source /usr/local/bin/install-from-source-with-patches

# Install build tools
# RUN install-from-source https://github.com/Kitware/CMake/releases/download/v3.15.2/cmake-3.15.2.tar.gz
# https://github.com/ninja-build/ninja/archive/v1.9.0.tar.gz

################################################
#  _____           _      _           _        #
# |_   _|__   ___ | | ___| |__   __ _(_)_ __   #
#   | |/ _ \ / _ \| |/ __| '_ \ / _` | | '_ \  #
#   | | (_) | (_) | | (__| | | | (_| | | | | | #
#   |_|\___/ \___/|_|\___|_| |_|\__,_|_|_| |_| #
################################################
ARG platform=generic

# Copy across the specific toolchain file for this image
COPY --chown=nubots:nubots usr/local/toolchain/${platform}.sh /usr/local/toolchain.sh
COPY --chown=nubots:nubots usr/local/meson/${platform}.cross /usr/local/meson.cross

# zlib
RUN install-from-source https://www.zlib.net/zlib-1.2.11.tar.gz

# OpenBLAS
RUN install-package gcc-fortran
COPY --chown=nubots:nubots usr/local/package/openblas/${platform}.sh usr/local/package/openblas.sh
RUN /usr/local/package/openblas.sh https://github.com/xianyi/OpenBLAS/archive/v0.3.7.tar.gz

# Armadillo
RUN install-cmake-from-source https://downloads.sourceforge.net/project/arma/armadillo-9.600.6.tar.xz \
    -DDETECT_HDF5=OFF \
    -DBUILD_SHARED_LIBS=ON
COPY --chown=nubots:nubots usr/local/include/armadillo_bits/config.hpp /usr/local/include/armadillo_bits/config.hpp

# Eigen3
RUN install-from-source http://bitbucket.org/eigen/eigen/get/3.3.7.tar.bz2

# tcmalloc
RUN install-from-source https://github.com/gperftools/gperftools/releases/download/gperftools-2.7/gperftools-2.7.tar.gz \
    --with-tcmalloc-pagesize=64 \
    --enable-minimal

# Protobuf
RUN install-package protobuf
RUN install-from-source https://github.com/google/protobuf/releases/download/v3.7.0/protobuf-cpp-3.7.0.tar.gz \
    --with-zlib=/usr/local \
    --with-protoc=/usr/bin/protoc
RUN PROTOC=/usr/bin/protoc install-python-from-source \
    https://github.com/google/protobuf/releases/download/v3.7.0/protobuf-python-3.7.0.tar.gz --cpp_implementation

# Libjpeg
RUN install-package yasm
RUN install-from-source https://github.com/libjpeg-turbo/libjpeg-turbo/archive/2.0.2.tar.gz

# yaml-cpp
RUN install-from-source https://github.com/jbeder/yaml-cpp/archive/yaml-cpp-0.6.2.tar.gz \
    -DYAML_CPP_BUILD_TESTS=OFF \
    -DBUILD_SHARED_LIBS=OFF

# fmt formatting library
RUN  install-from-source https://github.com/fmtlib/fmt/archive/5.3.0.tar.gz \
    -DFMT_DOC=OFF \
    -DFMT_TEST=OFF

# Catch unit testing library
RUN install-header-from-source https://github.com/catchorg/Catch2/releases/download/v2.9.2/catch.hpp

# Aravis
RUN install-from-source http://xmlsoft.org/sources/libxml2-2.9.3.tar.gz --with-zlib=/usr/local --without-python
RUN install-from-source https://github.com/libffi/libffi/archive/v3.3-rc0.tar.gz
RUN install-from-source https://mirrors.edge.kernel.org/pub/linux/utils/util-linux/v2.34/util-linux-2.34.tar.xz \
    --disable-all-programs \
    --enable-libblkid \
    --enable-libmount \
    --enable-libuuid
RUN install-from-source https://gitlab.gnome.org/GNOME/glib/-/archive/2.61.2/glib-2.61.2.tar.gz \
    --cross-file=/usr/local/meson.cross \
    -Ddefault_library=both \
    -Dinternal_pcre=true \
    && cp /usr/local/lib/glib-2.0/include/glibconfig.h /usr/local/include/glibconfig.h
RUN install-meson-from-source https://github.com/AravisProject/aravis/archive/ARAVIS_0_6_3.tar.gz \
    -Ddefault_library=both \
    -Dviewer=false \
    -Dgst-plugin=false \
    -Dusb=true \
    -Ddocumentation=false \
    -Dintrospection=false

# FSWatch
RUN install-from-source https://github.com/emcrisostomo/fswatch/releases/download/1.14.0/fswatch-1.14.0.tar.gz

# LibUV
RUN install-cmake-from-source https://github.com/libuv/libuv/archive/v1.31.0.tar.gz \
    -Dlibuv_buildtests=OFF \
    -DBUILD_TESTING=OFF

# NUClear!
RUN install-from-source https://github.com/Fastcode/NUClear/archive/master.tar.gz \
    -DBUILD_TESTS=OFF

# LibBacktrace
RUN install-from-source https://github.com/ianlancetaylor/libbacktrace/archive/master.tar.gz \
    --without-system-libunwind \
    --enable-shared \
    --enable-static

# Intel Compute Runtime (OpenCL) and Intel Media Driver
RUN install-package llvm \
    clang \
    libva \
    libpciaccess \
    ruby
RUN install-from-source-with-patches https://github.com/KhronosGroup/SPIRV-LLVM-Translator/archive/v8.0.1-2.tar.gz \
    https://raw.githubusercontent.com/intel/opencl-clang/94af090661d7c953c516c97a25ed053c744a0737/patches/spirv/0001-Update-LowerOpenCL-pass-to-handle-new-blocks-represn.patch \
    https://raw.githubusercontent.com/intel/opencl-clang/94af090661d7c953c516c97a25ed053c744a0737/patches/spirv/0002-Remove-extra-semicolon.patch \
    --
RUN install-from-source-with-patches https://github.com/intel/opencl-clang/archive/v8.0.1.tar.gz \
    https://github.com/intel/opencl-clang/commit/a6e69b30a6a2c925254784be808ae3171ecd75ea.patch \
    https://github.com/intel/opencl-clang/commit/94af090661d7c953c516c97a25ed053c744a0737.patch \
    -- \
    -DLLVMSPIRV_INCLUDED_IN_LLVM=OFF \
    -DSPIRV_TRANSLATOR_DIR=/usr/local \
    -DLLVM_NO_DEAD_STRIP=ON
COPY --chown=nubots:nubots usr/local/package/intel-graphics-compiler/python3.patch /usr/local/package/intel-graphics-compiler/
RUN install-from-source-with-patches https://github.com/intel/intel-graphics-compiler/archive/igc-1.0.10.tar.gz \
    /usr/local/package/intel-graphics-compiler/python3.patch \
    -- \
    -DIGC_OPTION__ARCHITECTURE_TARGET='Linux64' \
    -DIGC_PREFERRED_LLVM_VERSION='8.0.0'
RUN install-from-source https://github.com/intel/gmmlib/archive/intel-gmmlib-19.2.3.tar.gz \
    -DRUN_TEST_SUITE=OFF
COPY --chown=root:root etc/OpenCL/vendors/intel.icd /etc/OpenCL/vendors/intel.icd
COPY --chown=nubots:nubots usr/local/package/intel-compute-runtime/install-from-source /usr/local/package/intel-compute-runtime.sh
RUN /usr/local/package/intel-compute-runtime.sh \
    https://github.com/intel/compute-runtime/archive/19.32.13826/intel-compute-runtime-19.32.13826.tar.gz \
    -DNEO_DRIVER_VERSION=19.32.13826 \
    -DSKIP_ALL_ULT=ON \
    -DSKIP_UNIT_TESTS=ON \
    -DIGDRCL__IGC_LIBRARY_PATH="/usr/local/lib"
COPY --chown=nubots:nubots usr/local/package/opencl-headers/install-from-source /usr/local/package/opencl-headers.sh
RUN /usr/local/package/opencl-headers.sh https://github.com/KhronosGroup/OpenCL-Headers/archive/master.tar.gz
COPY --chown=nubots:nubots usr/local/package/opencl-clhpp/install-from-source /usr/local/package/opencl-clhpp.sh
RUN /usr/local/package/opencl-clhpp.sh https://github.com/KhronosGroup/OpenCL-CLHPP/archive/master.tar.gz
COPY --chown=nubots:nubots usr/local/package/ocl-icd/install-from-source /usr/local/package/ocl-icd.sh
RUN /usr/local/package/ocl-icd.sh https://github.com/OCL-dev/ocl-icd/archive/v2.2.12.tar.gz

# Install python libraries
RUN pip install \
    stringcase \
    Pillow

# Install tools needed for building individual modules as well as development tools
RUN install-package \
    arm-none-eabi-gcc \
    arm-none-eabi-newlib \
    openssh \
    gdb \
    valgrind

# Copy ssh keys over to the system
COPY home/nubots/.ssh/id_rsa /home/nubots/.ssh/id_rsa
COPY home/nubots/.ssh/id_rsa.pub /home/nubots/.ssh/id_rsa.pub
COPY home/nubots/.ssh/ssh_config /home/nubots/.ssh/ssh_config

# Setup the locations where we will mount our folders
RUN mkdir -p /home/nubots/NUbots/build
WORKDIR /home/nubots/NUbots
