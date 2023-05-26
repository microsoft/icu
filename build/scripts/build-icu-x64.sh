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

# Install ICU into this location.
DEST_DIR=/dist/icu

# Build ICU in this location (out-of-source build).
BUILD_DIR=/tmp/build

# Number of CPU Cores to use for make
CPU_CORES=$(nproc)

# We want to produce debugging symbols for "release" builds, but we don't want to use
# the "--enable-debug" option with runConfigureICU, as that will turn on all kinds of
# debug asserts inside the ICU library code.
DEFINES="-g"
# Set for both C and C++
export CFLAGS="$DEFINES"
export CXXFLAGS="$DEFINES"

# Configure ICU for building. Skip layout[ex] and samples
mkdir -p ${BUILD_DIR} && cd ${BUILD_DIR} && ${ICU_SRC}/icu/icu4c/source/runConfigureICU Linux --disable-layout --disable-layoutex --disable-samples

# Build and run the tests
make -j${CPU_CORES} check 2>&1 | tee make-output.log

# Install into DEST_DIR.
echo "Done building, installing into $DEST_DIR ..."
make -j${CPU_CORES} DESTDIR=${DEST_DIR} releaseDist 2>&1 | tee make-install.log

# Split out the debugging symbols from the libs
ls -al ${DEST_DIR}/usr/local/lib

for file in ${DEST_DIR}/usr/local/lib/lib*.so*; do
    if [[ -L "$file" ]]; then echo "Skipping symlink $file";
    else
        echo "Stripping symbols for $file"
        objcopy --only-keep-debug "$file" "$file.debug"
        objcopy --strip-debug "$file"
        objcopy --add-gnu-debuglink="$file.debug" "$file"
    fi
done

ls -al ${DEST_DIR}/usr/local/lib

# Test that icuinfo works with the stripped libs in the installed location
LD_LIBRARY_PATH=${DEST_DIR}/usr/local/lib ${DEST_DIR}/usr/local/bin/icuinfo
