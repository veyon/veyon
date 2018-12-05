#!/usr/bin/env bash

set -e

/veyon/.travis/common/linux-build.sh /veyon /build

cd /build

# add distribution name to file name
rename '.x86_64' '.opensuse-15.0.x86_64' /build/*.rpm

# show files
rpm -qlp *.rpm

# show dependencies
rpm -qpR *.rpm

# move to Docker volume
mv -v *.rpm /veyon

