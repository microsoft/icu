#!/bin/bash

# -e            Exit immediately if a command exits with a non-zero status
# -u            Treat unset variables as an error
# -x            Echo commands when executing them
# -o pipefile   All commands in a pipeline must pass with non-zero status
set -euxo pipefail

# Disable color output
export TERM=xterm

# ----------------------------------------------------------------------------------
# Common variables
# The docker container should be setup with the ICU source under /src
# Output will be under /dist

# The top-level directory for the repo.
ICU_SRC="/src"

# Number of CPU Cores to use for make
CPU_CORES=$(nproc)

# This is the location CrossRootFS location in the .NET docker image
# This location should contain the GNU GCC toolchain, libs, etc. for ARM64.
CROSSROOT="/crossrootfs/arm64"

# This is the target-triple that is used by clang/gcc, and the crossrootfs.
TARGET_ARCH="aarch64-linux-gnu"

# Build ICU under this location (out-of-source build).

BUILD_DIR=/tmp/build
HOST_BUILD_DIR="${BUILD_DIR}/host"
TARGET_BUILD_DIR="${BUILD_DIR}/target"

# Install the target ICU build under this location.
DEST_DIR="/dist/icu"

# ----------------------------------------------------------------------------------
# Remove libicu-dev from the crossrootfs
# The .NET Core docker image has the icu-dev package installed in the crossrootfs,
# which we don't want. (.NET needs it for building, but we are actually building
# ICU itself, so we don't want it to conflict).
# Note that we can not use package managers like `apt` or `yum` to remove it
# since they will only change or remove things from the _host_ filesystem.
# Whereas the crossrootfs filesystem is really just a special directory that is
# already built/setup ahead of time. This means that we must *manually* remove it.

# icu-config
rm -f ${CROSSROOT}/usr/bin/icu-config
rm -f ${CROSSROOT}/usr/bin/icuinfo

# headers
rm -rf ${CROSSROOT}/usr/include/${TARGET_ARCH}/unicode/*
rm -rf ${CROSSROOT}/usr/include/${TARGET_ARCH}/layout/*

# libs
rm -rf ${CROSSROOT}/usr/lib/${TARGET_ARCH}/icu/*
rm -rf ${CROSSROOT}/usr/lib/${TARGET_ARCH}/libicu*

# pkgconfig
rm -rf ${CROSSROOT}/usr/lib/${TARGET_ARCH}/pkgconfig/icu-*

# config
rm -rf ${CROSSROOT}/usr/share/icu/*

# ----------------------------------------------------------------------------------
# We need to use Clang for cross-compiling.
# Note that this particular .NET docker image doesn't have a symlink for 'clang',
# so we need to specify the exact version here instead.
export CC=clang-9
export CXX=clang++-9

# ----------------------------------------------------------------------------------
# Host build

# For the host build, we mostly just need the tools, so we can disable some options
# that aren't needed in order to save a bit of build time.
mkdir -p ${HOST_BUILD_DIR} && cd ${HOST_BUILD_DIR} && ${ICU_SRC}/icu/icu4c/source/runConfigureICU Linux \
 --disable-icu-config \
 --disable-extras \
 --disable-tests \
 --disable-layout \
 --disable-layoutex \
 --disable-samples \

make -j${CPU_CORES} all

# ----------------------------------------------------------------------------------
# Target build

# Note: We must set both the "sysroot" and the "gcc-toolchain".
SHARED_COMPILER_ARGS="--target=$TARGET_ARCH --sysroot=$CROSSROOT --gcc-toolchain=${CROSSROOT}/usr"

# We want to produce debugging symbols for "release" builds, but we don't want to use
# the "--enable-debug" option with runConfigureICU, as that will turn on all kinds of
# debug asserts inside the ICU library code.
DEFINES="-g"

export CFLAGS="$DEFINES $SHARED_COMPILER_ARGS"
export CXXFLAGS="$DEFINES $SHARED_COMPILER_ARGS"

# Linker flags
# We need to tell the linker to look under the crossrootfs for shared libraries.
export LDFLAGS="-Wl,--rpath-link=${CROSSROOT}/lib/${TARGET_ARCH}:${CROSSROOT}/usr/lib/${TARGET_ARCH}"

# For building ICU for the ARM64 Nuget, we only really need: libicuuc, libicuin, libicudata
# So we can turn of unneeded options in order to save a bit of build time.
ICU_CONFIG_ARGS="--disable-icu-config --disable-extras --disable-tests --disable-layout --disable-layoutex --disable-samples --disable-icuio"

# IMPORTANT NOTE: This is rather confusing. Clang uses the term 'host' to refer to the platform that
# is doing the building, and 'target' to refer to the platform on which the resulting binaries will run.
#
# However, the GNU AutoConf script used by ICU *swaps* these terms, and uses the term 'host' to mean
# the platform on which the resulting binaries will run.
# That's why we have the confusing option `host=target` in the argument list below.
ICU_CONFIG_CROSSBUILD_ARGS="--with-cross-build=$HOST_BUILD_DIR --host=$TARGET_ARCH"

# Clean the output ARM64 build dir.
rm -rf $TARGET_BUILD_DIR && mkdir -p $TARGET_BUILD_DIR && cd $TARGET_BUILD_DIR

${ICU_SRC}/icu/icu4c/source/configure $ICU_CONFIG_ARGS $ICU_CONFIG_CROSSBUILD_ARGS \
 CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS" LDFLAGS="$LDFLAGS" CC="$CC" CXX="$CXX"

make -j${CPU_CORES} all 2>&1 | tee make-output.log

# Clean the output dir.
rm -rf $DEST_DIR && mkdir -p $DEST_DIR

# Install into DEST_DIR.
echo "Done building, installing into $DEST_DIR ..."
make -j${CPU_CORES} DESTDIR=${DEST_DIR} releaseDist 2>&1 | tee make-install.log

# Split out the debugging symbols from the libs
ls -al ${DEST_DIR}/usr/local/lib

for file in ${DEST_DIR}/usr/local/lib/lib*.so*; do
    if [[ -L "$file" ]]; then echo "Skipping symlink $file";
    else
        echo "Stripping symbols for $file"
        aarch64-linux-gnu-objcopy --only-keep-debug "$file" "$file.debug"
        aarch64-linux-gnu-objcopy --strip-debug "$file"
        aarch64-linux-gnu-objcopy --add-gnu-debuglink="$file.debug" "$file"
    fi
done

ls -al ${DEST_DIR}/usr/local/lib
