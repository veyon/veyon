#!/usr/bin/env bash

set -e

/veyon/.travis/common/linux-build.sh /veyon /build

cd /build

rename "s/_amd64/-ubuntu-bionic_amd64/g" *.deb

dpkg -c *.deb
dpkg -I *.deb

mv -v *.deb /veyon

