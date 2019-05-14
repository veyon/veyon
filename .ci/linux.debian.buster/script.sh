#!/usr/bin/env bash

set -e

/veyon/.ci/common/linux-build.sh /veyon /build
/veyon/.ci/common/finalize-deb.sh "debian-buster"
