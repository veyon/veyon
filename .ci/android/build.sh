#!/usr/bin/env bash

set -e

CPUS=$(nproc)
BASEDIR=$(pwd)
BUILDDIR=/tmp/build

rm -rf $BUILDDIR
mkdir $BUILDDIR
cd $BUILDDIR

cmake $BASEDIR -DCMAKE_TOOLCHAIN_FILE=$BASEDIR/cmake/modules/AndroidToolchain.cmake -DANDROID_NDK=/opt/android/ndk

echo Building on $CPUS CPUs

if [ -z "$2" ] ; then
	make -j$CPUS
else
	make ${@:2} -j$CPUS
fi

