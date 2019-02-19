#!/usr/bin/env bash

set -e

export CC=gcc-6
export CXX=g++-6

/veyon/.travis/common/linux-build.sh /veyon /build
/veyon/.travis/common/finalize-rpm.sh "opensuse-42.3"
