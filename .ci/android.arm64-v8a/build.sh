#!/usr/bin/env bash

set -e

SRC=$1
BUILD=$2
CPUS=$(nproc)

mkdir -p $BUILD
cd $BUILD

cmake -DCMAKE_TOOLCHAIN_FILE=$SRC/cmake/modules/AndroidToolchain.cmake \
	-DANDROID_NDK=/opt/android/ndk \
	-DANDROID_ABI="arm64-v8a" \
	-DANDROID_TOOLCHAIN_MACHINE_NAME="aarch64-linux-android" \
	$SRC

echo Building on $CPUS CPUs

if [ -z "$2" ] ; then
	make -j$CPUS
else
	make ${@:2} -j$CPUS
fi

