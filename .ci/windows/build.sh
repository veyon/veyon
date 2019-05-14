#!/usr/bin/env bash

set -e

CPUS=$(nproc)
BASEDIR=$(pwd)

apt-get update && apt-get -y dist-upgrade

.ci/common/strip-ultravnc-sources.sh

mkdir /tmp/build
cd /tmp/build

cmake $BASEDIR -DCMAKE_TOOLCHAIN_FILE=$BASEDIR/cmake/modules/Win${1}Toolchain.cmake -DCMAKE_MODULE_PATH=$BASEDIR/cmake/modules/

echo Building on $CPUS CPUs

if [ -z "$2" ] ; then
	make -j$CPUS

	make win-nsi
	mv -v veyon*setup.exe $BASEDIR
else
	make ${@:2} -j$CPUS
fi

