#!/bin/bash

# echo commands when executing them
set -x

# Disable color output
export TERM=xterm

# Number of CPU Cores to use for make
export CPUCORES=$(nproc)

echo Building in: $(pwd)

# Configure ICU for building. Skip layout[ex] and samples
/src/icu/icu4c/source/runConfigureICU Linux --disable-layout --disable-layoutex --disable-samples || exit 1

# Build, run the tests, then install into DESTDIR.
make -j${CORES} check && make -j${CORES} DESTDIR=/dist/icu releaseDist || exit 1

# Test that icuinfo works with the built libs in the installed location
LD_LIBRARY_PATH=/dist/icu/usr/local/lib /dist/icu/usr/local/bin/icuinfo || exit 1

# Copy OS Release (name of the distro) if it exists
if [ -f /etc/os-release ];
then
    cat /etc/os-release
    cp /etc/os-release /dist/icu
fi

# Copy the MS-ICU version number
cp /src/version.txt /dist/icu

# Pack up the binaries into a tarball
cd /dist && tar -czpf icu-binaries.tar.gz -C /dist/icu .
