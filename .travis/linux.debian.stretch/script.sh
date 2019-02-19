#!/usr/bin/env bash

set -e

/veyon/.travis/common/linux-build.sh /veyon /build
/veyon/.travis/common/finalize-deb.sh "debian-stretch"

# generate source tarball
cd /veyon
VERSION=$(git describe --tags --abbrev=0 | sed -e 's/^v//g')
cp /build/CONTRIBUTORS .

.travis/common/strip-kitemmodels-sources.sh
.travis/common/strip-kldap-sources.sh
.travis/common/strip-libvncserver-sources.sh
.travis/common/strip-ultravnc-sources.sh
.travis/common/strip-x11vnc-sources.sh

cd /
tar --transform "s,^veyon,veyon-$VERSION," --exclude=".git" --exclude="*.deb" -cjf /build/veyon-$VERSION-src.tar.bz2 veyon

mv -v /build/*.tar.bz2 /veyon
