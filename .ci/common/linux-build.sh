#!/usr/bin/env bash

set -e

SRC=$1
BUILD=$2

mkdir -p $BUILD
cd $BUILD

if [ ! -z "$CI_COMMIT_TAG" -o ! -z "$TRAVIS_TAG" ] ; then
	BUILD_TYPE="RelWithDebInfo"
	LTO="ON"
else
	BUILD_TYPE="Debug"
	LTO="OFF"
fi

cmake -G Ninja -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_INSTALL_PREFIX=/usr -DWITH_LTO=$LTO $CMAKE_FLAGS $SRC

if [ -z "$3" ] ; then
	ninja

	fakeroot ninja package

	LIBDIR=$(grep VEYON_LIB_DIR CMakeCache.txt |cut -d "=" -f2)
	BUILD_PWD=$(pwd)

	mkdir -p $LIBDIR
	cd $LIBDIR
	find $BUILD_PWD/plugins -name "*.so" -exec ln -s '{}' ';'
	cd $BUILD_PWD

	./cli/veyon-cli help
	./cli/veyon-cli about
else
	fakeroot ninja package
fi

