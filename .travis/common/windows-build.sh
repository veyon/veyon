#!/usr/bin/env bash

CPUS=$(nproc)

apt-get update && apt-get -y dist-upgrade

cd /veyon

.travis/common/strip-ultravnc-sources.sh

mkdir build
cd build

../cmake/build_mingw$1

echo Building on $CPUS CPUs

if [ -z "$2" ] ; then
	make -j$CPUS

	make win-nsi
	mv -v veyon*setup.exe /veyon
else
	make $2 -j$CPUS
fi

