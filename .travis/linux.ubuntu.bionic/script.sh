#!/usr/bin/env bash

set -e

/veyon/.travis/common/linux-build.sh /veyon /build
/veyon/.travis/common/finalize-deb.sh "ubuntu-bionic"
