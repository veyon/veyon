#!/usr/bin/env bash

set -e

/veyon/.ci/common/linux-build.sh /veyon /build
/veyon/.ci/common/finalize-deb.sh "debian-stretch"

# generate source tarball
cd /veyon
VERSION=$(git describe --tags --abbrev=0 | sed -e 's/^v//g')
cp /build/CONTRIBUTORS .

.ci/common/strip-kitemmodels-sources.sh
.ci/common/strip-kldap-sources.sh
.ci/common/strip-libvncserver-sources.sh
.ci/common/strip-ultravnc-sources.sh
.ci/common/strip-x11vnc-sources.sh

cd /
tar --transform "s,^veyon,veyon-$VERSION," --exclude=".git" --exclude="*.deb" -cjf /build/veyon-$VERSION-src.tar.bz2 veyon

mv -v /build/*.tar.bz2 /veyon
