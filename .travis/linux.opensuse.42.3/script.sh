#!/usr/bin/env bash

export CC=gcc-6
export CXX=g++-6

/veyon/.travis/common/linux-build.sh /veyon /build

cd /build

# add distribution name to file name
rename '.x86_64' '.opensuse-42.3.x86_64' /build/*.rpm

# show dependencies
rpm -qpR *.rpm

# move to Docker volume
mv -v *.rpm /veyon

