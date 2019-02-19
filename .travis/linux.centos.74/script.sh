#!/usr/bin/env bash

source scl_source enable devtoolset-7

set -e

/veyon/.travis/common/linux-build.sh /veyon /build
/veyon/.travis/common/finalize-rpm.sh "centos-74"
