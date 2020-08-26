#!/bin/bash

# -e            Exit immediately if a command exits with a non-zero status
# -u            Treat unset variables as an error
# -x            Echo commands when executing them
# -o pipefile   All commands in a pipeline must pass with non-zero status
set -euxo pipefail

# Disable color output
export TERM=xterm

echo Building in: $(pwd)

# Install ICU into this location
export DESTDIR=/dist/icu

# Number of CPU Cores to use for make
export CPUCORES=$(nproc)

# We want to produce debugging symbols for "release" builds, but we don't want to use
# the "--enable-debug" option with runConfigureICU, as that will turn on all kinds of
# debug asserts inside the ICU library code.
DEFINES="-g"
# Set for both C and C++
export CFLAGS="$DEFINES"
export CXXFLAGS="$DEFINES"

# Configure ICU for building. Skip layout[ex] and samples
/src/icu/icu4c/source/runConfigureICU Linux --disable-layout --disable-layoutex --disable-samples

# Build and run the tests
make -j${CPUCORES} check

# Install into DESTDIR.
echo "Done building, installing into $DESTDIR ..."
make -j${CPUCORES} DESTDIR=${DESTDIR} releaseDist

# Split out the debugging symbols from the libs
ls -al ${DESTDIR}/usr/local/lib

for file in ${DESTDIR}/usr/local/lib/lib*.so*; do
    if [[ -L "$file" ]]; then echo "Skipping symlink $file";
    else
        echo "Stripping symbols for $file"
        objcopy --only-keep-debug "$file" "$file.debug"
        objcopy --strip-debug "$file"
        objcopy --add-gnu-debuglink="$file.debug" "$file"
    fi
done

ls -al ${DESTDIR}/usr/local/lib

# Test that icuinfo works with the stripped libs in the installed location
LD_LIBRARY_PATH=${DESTDIR}/usr/local/lib ${DESTDIR}/usr/local/bin/icuinfo
